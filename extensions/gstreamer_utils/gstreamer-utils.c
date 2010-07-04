/* -*- Mode: CPP; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2008-2009 Free Software Foundation, Inc.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Street #330, Boston, MA 02111-1307, USA.
 */

/* This was based on a file from Tracker */
/*
 * Tracker - audio/video metadata extraction based on GStreamer
 * Copyright (C) 2006, Laurent Aguerreche (laurent.aguerreche@free.fr)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA  02110-1301, USA.
 */

#include <config.h>
#include <gst/gst.h>
#include <gthumb.h>
#include "gstreamer-utils.h"
#include "gstscreenshot.h"


static gboolean gstreamer_initialized = FALSE;


typedef struct {
	GstElement *playbin;
	GstTagList *tagcache;
	gboolean    has_audio;
	gboolean    has_video;
	gint        video_height;
	gint        video_width;
	gint        video_fps_n;
	gint        video_fps_d;
	gint        video_bitrate;
	char       *video_codec;
	gint        audio_channels;
	gint        audio_samplerate;
	gint        audio_bitrate;
	char       *audio_codec;
} MetadataExtractor;


static void
reset_extractor_data (MetadataExtractor *extractor)
{
	if (extractor->tagcache != NULL) {
		gst_tag_list_free (extractor->tagcache);
		extractor->tagcache = NULL;
	}

	g_free (extractor->audio_codec);
	extractor->audio_codec = NULL;

	g_free (extractor->video_codec);
	extractor->video_codec = NULL;

	extractor->has_audio = FALSE;
	extractor->has_video = FALSE;
	extractor->video_fps_n = -1;
	extractor->video_fps_d = -1;
	extractor->video_height = -1;
	extractor->video_width = -1;
	extractor->video_bitrate = -1;
	extractor->audio_channels = -1;
	extractor->audio_samplerate = -1;
	extractor->audio_bitrate = -1;
}


static void
metadata_extractor_free (MetadataExtractor *extractor)
{
	reset_extractor_data (extractor);
	gst_element_set_state (extractor->playbin, GST_STATE_NULL);
	gst_object_unref (GST_OBJECT (extractor->playbin));
	g_slice_free (MetadataExtractor, extractor);
}


gboolean
gstreamer_init (void)
{
	if (! gstreamer_initialized) {
		if (! gst_init_check (NULL, NULL, NULL))
			return FALSE;
		gstreamer_initialized = TRUE;
	}

	return TRUE;
}


static void
add_metadata (GFileInfo  *info,
              const char *key,
              char       *raw,
              char       *formatted)
{
	GthMetadata *metadata;

	if (raw == NULL)
		return;

	if (strcmp (key, "general::dimensions") == 0) {
		g_file_info_set_attribute_string (info, key, raw);
		return;
	}
	else if (strcmp (key, "general::duration") == 0) {
		int secs;

		g_free (formatted);
		sscanf (raw, "%i", &secs);
		formatted = _g_format_duration_for_display (secs * 1000);
	}
	else if (strcmp (key, "audio-video::general::bitrate") == 0) {
		int bps;

		g_free (formatted);
		sscanf (raw, "%i", &bps);
		formatted = g_strdup_printf ("%d kbps", bps / 1000);
	}

	metadata = gth_metadata_new ();
	g_object_set (metadata,
		      "id", key,
		      "formatted", formatted != NULL ? formatted : raw,
		      "raw", raw,
		      NULL);
	g_file_info_set_attribute_object (info, key, G_OBJECT (metadata));

	g_object_unref (metadata);
	g_free (raw);
	g_free (formatted);
}


static void
add_metadata_from_tag (GFileInfo         *info,
		       const GstTagList  *list,
		       const char        *tag,
		       const char        *tag_key)
{
	GType tag_type;

	tag_type = gst_tag_get_type (tag);

	if (tag_type == G_TYPE_BOOLEAN) {
		gboolean ret;
		if (gst_tag_list_get_boolean (list, tag, &ret)) {
			if (ret)
				add_metadata (info, tag_key, g_strdup ("TRUE"), NULL);
			else
				add_metadata (info, tag_key, g_strdup ("FALSE"), NULL);
		}
	}

	if (tag_type == G_TYPE_STRING) {
		char *ret = NULL;
		if (gst_tag_list_get_string (list, tag, &ret))
			add_metadata (info, tag_key, ret, NULL);
	}

        if (tag_type == G_TYPE_UCHAR) {
                guchar ret = 0;
                if (gst_tag_list_get_uchar (list, tag, &ret))
                        add_metadata (info, tag_key, g_strdup_printf ("%u", ret), NULL);
        }

        if (tag_type == G_TYPE_CHAR) {
                gchar ret = 0;
                if (gst_tag_list_get_char (list, tag, &ret))
                        add_metadata (info, tag_key, g_strdup_printf ("%d", ret), NULL);
        }

        if (tag_type == G_TYPE_UINT) {
                guint ret = 0;
                if (gst_tag_list_get_uint (list, tag, &ret))
                        add_metadata (info, tag_key, g_strdup_printf ("%u", ret), NULL);
        }

        if (tag_type == G_TYPE_INT) {
                gint ret = 0;
                if (gst_tag_list_get_int (list, tag, &ret))
                        add_metadata (info, tag_key, g_strdup_printf ("%d", ret), NULL);
        }

        if (tag_type == G_TYPE_ULONG) {
                gulong ret = 0;
                if (gst_tag_list_get_ulong (list, tag, &ret))
                        add_metadata (info, tag_key, g_strdup_printf ("%lu", ret), NULL);
        }

        if (tag_type == G_TYPE_LONG) {
                glong ret = 0;
                if (gst_tag_list_get_long (list, tag, &ret))
                        add_metadata (info, tag_key, g_strdup_printf ("%ld", ret), NULL);
        }

        if (tag_type == G_TYPE_INT64) {
                gint64 ret = 0;
                if (gst_tag_list_get_int64 (list, tag, &ret))
                        add_metadata (info, tag_key, g_strdup_printf ("%" G_GINT64_FORMAT, ret), NULL);
        }

        if (tag_type == G_TYPE_UINT64) {
                guint64 ret = 0;
                if (gst_tag_list_get_uint64 (list, tag, &ret))
                        add_metadata (info, tag_key, g_strdup_printf ("%" G_GUINT64_FORMAT, ret), NULL);
        }

        if (tag_type == G_TYPE_DOUBLE) {
                gdouble ret = 0;
                if (gst_tag_list_get_double (list, tag, &ret))
                        add_metadata (info, tag_key, g_strdup_printf ("%f", ret), NULL);
        }

        if (tag_type == G_TYPE_FLOAT) {
                gfloat ret = 0;
                if (gst_tag_list_get_float (list, tag, &ret))
                        add_metadata (info, tag_key, g_strdup_printf ("%f", ret), NULL);
        }

        if (tag_type == G_TYPE_DATE) {
                GDate *ret = NULL;
                if (gst_tag_list_get_date (list, tag, &ret)) {
			if (ret != NULL) {
				char  buf[128];
				char *raw;
				char *formatted;

				g_date_strftime (buf, 10, "%F %T", ret);
				raw = g_strdup (buf);

				g_date_strftime (buf, 10, "%x %X", ret);
				formatted = g_strdup (buf);
				add_metadata (info, tag_key, raw, formatted);
			}
			g_free (ret);
		}
        }
}


static void
tag_iterate (const GstTagList *list,
	     const char       *tag,
	     GFileInfo        *info)
{
	const char *tag_key;
	char       *attribute = NULL;

	tag_key = NULL;

	if (strcmp (tag, "container-format") == 0) {
		tag_key = "general::format";
	}
	else if (strcmp (tag, "bitrate") == 0) {
		tag_key = "audio-video::general::bitrate";
	}
	else if (strcmp (tag, "encoder") == 0) {
		tag_key = "audio-video::general::encoder";
	}
	else if (strcmp (tag, "title") == 0) {
		tag_key = "general::title";
	}
	else if (strcmp (tag, "artist") == 0) {
		tag_key = "audio-video::general::artist";
	}
	else if (strcmp (tag, "album") == 0) {
		tag_key = "audio-video::general::album";
	}
	else if (strcmp (tag, "audio-codec") == 0) {
		tag_key = "audio-video::audio::codec";
	}
	else if (strcmp (tag, "video-codec") == 0) {
		tag_key = "audio-video::video::codec";
	}

	if (tag_key == NULL) {
		GthMetadataInfo *metadata_info;

		attribute = g_strconcat ("audio-video::other::", tag, NULL);
		metadata_info = gth_main_get_metadata_info (attribute);
		if (metadata_info == NULL) {
			GthMetadataInfo *info;

			info = g_new0 (GthMetadataInfo, 1);
			info->id = attribute;
			info->display_name = gst_tag_get_nick (tag);
			info->category = "audio-video::other";
			info->sort_order = 500;
			info->flags = GTH_METADATA_ALLOW_IN_PROPERTIES_VIEW;
			metadata_info = gth_main_register_metadata_info (info);

			g_free (info);
		}

		tag_key = attribute;
	}

	add_metadata_from_tag (info, list, tag, tag_key);

	g_free (attribute);
}


static gint64
get_media_duration (MetadataExtractor *extractor)
{
	GstFormat fmt;
	gint64	  duration;

	g_return_val_if_fail (extractor, -1);
	g_return_val_if_fail (extractor->playbin, -1);

	fmt = GST_FORMAT_TIME;
	duration = -1;
	if (gst_element_query_duration (extractor->playbin, &fmt, &duration) && (duration >= 0))
		return duration / GST_SECOND;
	else
		return -1;
}


static void
extract_metadata (MetadataExtractor *extractor,
		  GFileInfo         *info)
{
	gint64 duration;

	if (extractor->audio_channels >= 0)
		add_metadata (info,
			      "audio-video::audio::channels",
			      g_strdup_printf ("%d", (guint) extractor->audio_channels),
			      g_strdup (extractor->audio_channels == 2 ? _("Stereo") : _("Mono")));

        if (extractor->audio_samplerate >= 0)
                add_metadata (info,
                              "audio-video::audio::samplerate",
                              g_strdup_printf ("%d", (guint) extractor->audio_samplerate),
                              g_strdup_printf ("%d Hz", (guint) extractor->audio_samplerate));

        if (extractor->audio_bitrate >= 0)
                add_metadata (info,
                              "audio-video::audio::bitrate",
                              g_strdup_printf ("%d", (guint) extractor->audio_bitrate),
                              g_strdup_printf ("%d bps", (guint) extractor->audio_bitrate));

        if (extractor->video_height >= 0) {
                add_metadata (info,
                	      "audio-video::video::height",
                	      g_strdup_printf ("%d", (guint) extractor->video_height),
                	      NULL);
		g_file_info_set_attribute_int32 (info, "frame::height", extractor->video_height);
        }

        if (extractor->video_width >= 0) {
                add_metadata (info,
                	      "audio-video::video::width",
                	      g_strdup_printf ("%d", (guint) extractor->video_width),
                	      NULL);
		g_file_info_set_attribute_int32 (info, "frame::width", extractor->video_width);
        }

        if ((extractor->video_height >= 0) && (extractor->video_width >= 0))
                add_metadata (info,
                	      "general::dimensions",
                	      g_strdup_printf ("%d × %d", (guint) extractor->video_width, (guint) extractor->video_height),
                	      NULL);

        if ((extractor->video_fps_n >= 0) && (extractor->video_fps_d >= 0))
                add_metadata (info,
                              "audio-video::video::framerate",
                              g_strdup_printf ("%.7g", (gdouble) extractor->video_fps_n / (gdouble) extractor->video_fps_d),
                              g_strdup_printf ("%.7g fps", (gdouble) extractor->video_fps_n / (gdouble) extractor->video_fps_d));

        if (extractor->video_bitrate >= 0)
                add_metadata (info,
                              "audio-video::video::bitrate",
                              g_strdup_printf ("%d", (guint) extractor->video_bitrate),
                              g_strdup_printf ("%d bps", (guint) extractor->video_bitrate));

	duration = get_media_duration (extractor);
	if (duration >= 0)
		add_metadata (info,
			      "general::duration",
			      g_strdup_printf ("%" G_GINT64_FORMAT, duration),
			      g_strdup_printf ("%" G_GINT64_FORMAT " sec", duration));

	if (extractor->tagcache != NULL)
		gst_tag_list_foreach (extractor->tagcache, (GstTagForeachFunc) tag_iterate, info);
}


static void
caps_set (GstPad           *pad,
	  MetadataExtractor *extractor,
	  const char        *type)
{
	GstCaps	     *caps;
	GstStructure *structure;

	if ((caps = gst_pad_get_negotiated_caps (pad)) == NULL)
		return;

	structure = gst_caps_get_structure (caps, 0);
	if (structure == NULL) {
		gst_caps_unref (caps);
		return;
	}

	if (strcmp (type, "audio") == 0) {
		gst_structure_get_int (structure, "channels", &extractor->audio_channels);
		gst_structure_get_int (structure, "rate", &extractor->audio_samplerate);
		gst_structure_get_int (structure, "bitrate", &extractor->audio_bitrate);
	}
	else if (strcmp (type, "video") == 0) {
		gst_structure_get_fraction (structure, "framerate", &extractor->video_fps_n, &extractor->video_fps_d);
		gst_structure_get_int (structure, "bitrate", &extractor->video_bitrate);
		gst_structure_get_int (structure, "width", &extractor->video_width);
		gst_structure_get_int (structure, "height", &extractor->video_height);
	}

	gst_caps_unref (caps);
}


static void
update_stream_info (MetadataExtractor *extractor)
{
	GList  *streaminfo;
	GstPad *audiopad;
	GstPad *videopad;

	g_return_if_fail (extractor);

	streaminfo = NULL;
	audiopad = videopad = NULL;

	g_object_get (extractor->playbin, "stream-info", &streaminfo, NULL);
	streaminfo = g_list_copy (streaminfo);
	g_list_foreach (streaminfo, (GFunc) g_object_ref, NULL);

	for (/* void */ ; streaminfo; streaminfo = streaminfo->next) {
		GObject    *info;
		int         type;
		GParamSpec *pspec;
		GEnumValue *val;

		info = streaminfo->data;
		if (info == NULL)
			continue;

                type = -1;

		g_object_get (info, "type", &type, NULL);
		pspec = g_object_class_find_property (G_OBJECT_GET_CLASS (info), "type");
		val = g_enum_get_value (G_PARAM_SPEC_ENUM (pspec)->enum_class, type);

		if (strcmp (val->value_nick, "audio") == 0) {
			extractor->has_audio = TRUE;
			if (audiopad == NULL)
				g_object_get (info, "object", &audiopad, NULL);
		}
		else if (strcmp (val->value_nick, "video") == 0) {
			extractor->has_video = TRUE;
			if (videopad == NULL)
				g_object_get (info, "object", &videopad, NULL);
		}
	}

	if (audiopad != NULL) {
		GstCaps *caps;

		if ((caps = gst_pad_get_negotiated_caps (audiopad)) != NULL) {
			caps_set (audiopad, extractor, "audio");
			gst_caps_unref (caps);
		}
	}

	if (videopad != NULL) {
		GstCaps *caps;

		if ((caps = gst_pad_get_negotiated_caps (videopad)) != NULL) {
			caps_set (videopad, extractor, "video");
			gst_caps_unref (caps);
		}
	}

	g_list_foreach (streaminfo, (GFunc) g_object_unref, NULL);
	g_list_free (streaminfo);
}


static gboolean
message_loop_to_state_change (MetadataExtractor *extractor,
			      GstState           state)
{
	GstBus         *bus;
	GstMessageType  events;

	g_return_val_if_fail (extractor, FALSE);
	g_return_val_if_fail (extractor->playbin, FALSE);

	bus = gst_element_get_bus (extractor->playbin);

	events = (GST_MESSAGE_TAG | GST_MESSAGE_STATE_CHANGED | GST_MESSAGE_ERROR | GST_MESSAGE_EOS);

	for (;;) {
		GstMessage *message;

		message = gst_bus_timed_pop_filtered (bus, GST_SECOND * 5, events);
		if (message == NULL)
			goto timed_out;

		switch (GST_MESSAGE_TYPE (message)) {
		case GST_MESSAGE_STATE_CHANGED: {
			GstState old_state;
			GstState new_state;

			old_state = new_state = GST_STATE_NULL;

			gst_message_parse_state_changed (message, &old_state, &new_state, NULL);
			if (old_state == new_state)
				break;

			/* we only care about playbin (pipeline) state changes */
			if (GST_MESSAGE_SRC (message) != GST_OBJECT (extractor->playbin))
				break;

			if ((old_state == GST_STATE_READY) && (new_state == GST_STATE_PAUSED))
				update_stream_info (extractor);
			else if ((old_state == GST_STATE_PAUSED) && (new_state == GST_STATE_READY))
				reset_extractor_data (extractor);

			if (new_state == state) {
				gst_message_unref (message);
				goto success;
			}

			break;
		}

		case GST_MESSAGE_TAG: {
			GstTagList *tag_list;
			GstTagList *result;

			tag_list = NULL;
			gst_message_parse_tag (message, &tag_list);
			result = gst_tag_list_merge (extractor->tagcache, tag_list, GST_TAG_MERGE_KEEP);
			if (extractor->tagcache != NULL)
				gst_tag_list_free (extractor->tagcache);
			extractor->tagcache = result;

			gst_tag_list_free (tag_list);

			break;
		}

		case GST_MESSAGE_ERROR: {
			gchar  *debug    = NULL;
			GError *gsterror = NULL;

			gst_message_parse_error (message, &gsterror, &debug);

			/*g_warning ("Error: %s (%s)", gsterror->message, debug);*/

			g_error_free (gsterror);
			gst_message_unref (message);
			g_free (debug);
			goto error;
		}
			break;

		case GST_MESSAGE_EOS: {
			g_warning ("Media file could not be played.");
			gst_message_unref (message);
			goto error;
		}
			break;

		default:
			g_assert_not_reached ();
			break;
		}

		gst_message_unref (message);
	}

	g_assert_not_reached ();

 success:
	/* state change succeeded */
	GST_DEBUG ("state change to %s succeeded", gst_element_state_get_name (state));
	return TRUE;

 timed_out:
	/* it's taking a long time to open  */
	GST_DEBUG ("state change to %s timed out, returning success", gst_element_state_get_name (state));
	return TRUE;

 error:
	GST_DEBUG ("error while waiting for state change to %s", gst_element_state_get_name (state));
	/* already set *error */
	return FALSE;
}


gboolean
gstreamer_read_metadata_from_file (GFile       *file,
				   GFileInfo   *info,
				   GError     **error)
{
	char              *uri;
	MetadataExtractor *extractor;

	if (! gstreamer_init ())
		return FALSE;

	uri = g_file_get_uri (file);
	g_return_val_if_fail (uri != NULL, FALSE);

	extractor = g_slice_new0 (MetadataExtractor);
	reset_extractor_data (extractor);

	extractor->playbin = gst_element_factory_make ("playbin", "playbin");
	g_object_set (G_OBJECT (extractor->playbin),
		      "uri", uri,
		      "audio-sink", gst_element_factory_make ("fakesink", "fakesink-audio"),
		      "video-sink", gst_element_factory_make ("fakesink", "fakesink-video"),
		      NULL);

	gst_element_set_state (extractor->playbin, GST_STATE_PAUSED);
	message_loop_to_state_change (extractor, GST_STATE_PAUSED);
	extract_metadata (extractor, info);

	metadata_extractor_free (extractor);
	g_free (uri);

	return TRUE;
}


/* -- _gst_playbin_get_current_frame -- */

/* this is voodoo code taken from totem, kudos to the authors, license is GPL */


typedef struct {

	GdkPixbuf          *pixbuf;
	FrameReadyCallback  cb;
	gpointer            user_data;
} ScreenshotData;


static void
screenshot_data_finalize (ScreenshotData *data)
{
	if (data->cb != NULL)
		data->cb (data->pixbuf, data->user_data);
	g_free (data);
}


static void
destroy_pixbuf (guchar *pix, gpointer data)
{
	gst_buffer_unref (GST_BUFFER (data));
}


static void
get_current_frame_step2 (GstBuffer *buf,
			 gpointer   user_data)
{
	ScreenshotData *data = user_data;
	GstStructure   *s;
	int             outwidth = 0;
	int             outheight = 0;

	if (buf == NULL) {
		g_warning ("Could not take screenshot: %s", "conversion failed");
		screenshot_data_finalize (data);
		return;
	}

	if (GST_BUFFER_CAPS (buf) == NULL) {
		g_warning ("Could not take screenshot: %s", "no caps on output buffer");
		screenshot_data_finalize (data);
		return;
	}

	s = gst_caps_get_structure (GST_BUFFER_CAPS (buf), 0);
	gst_structure_get_int (s, "width", &outwidth);
	gst_structure_get_int (s, "height", &outheight);

	g_return_if_fail (outwidth > 0 && outheight > 0);

	/* create pixbuf from that - use our own destroy function */

	data->pixbuf = gdk_pixbuf_new_from_data (GST_BUFFER_DATA (buf),
						 GDK_COLORSPACE_RGB,
						 FALSE,
						 8,
						 outwidth,
						 outheight,
						 GST_ROUND_UP_4 (outwidth * 3),
						 destroy_pixbuf,
						 buf);
	if (data->pixbuf == NULL) {
		g_warning ("Could not take screenshot: %s", "could not create pixbuf");
		gst_buffer_unref (buf);
	}

	screenshot_data_finalize (data);
}


gboolean
_gst_playbin_get_current_frame (GstElement          *playbin,
				int                  video_fps_n,
				int                  video_fps_d,
				FrameReadyCallback   cb,
				gpointer             user_data)
{
	ScreenshotData *data;
	GstBuffer      *buf;
	GstCaps        *to_caps;

	g_object_get (playbin, "frame", &buf, NULL);

	if (buf == NULL) {
		g_warning ("Could not take screenshot: %s", "no last video frame");
		return FALSE;
	}

	if (GST_BUFFER_CAPS (buf) == NULL) {
		g_warning ("Could not take screenshot: %s", "no caps on buffer");
		return FALSE;
	}

	/* convert to our desired format (RGB24) */

	data = g_new0 (ScreenshotData, 1);
	data->cb = cb;
	data->user_data = user_data;

	to_caps = gst_caps_new_simple ("video/x-raw-rgb",
				       "bpp", G_TYPE_INT, 24,
				       "depth", G_TYPE_INT, 24,
				       /* Note: we don't ask for a specific width/height here, so that
				        * videoscale can adjust dimensions from a non-1/1 pixel aspect
				        * ratio to a 1/1 pixel-aspect-ratio */
				       "pixel-aspect-ratio", GST_TYPE_FRACTION, 1, 1,
				       "endianness", G_TYPE_INT, G_BIG_ENDIAN,
				       "red_mask", G_TYPE_INT, 0xff0000,
				       "green_mask", G_TYPE_INT, 0x00ff00,
				       "blue_mask", G_TYPE_INT, 0x0000ff,
				       NULL);

	if (video_fps_n > 0 && video_fps_d > 0) {
		gst_caps_set_simple (to_caps, "framerate",
				     GST_TYPE_FRACTION, video_fps_n, video_fps_d,
				     NULL);
	}

	return bvw_frame_conv_convert (buf, to_caps, get_current_frame_step2, data);
}

