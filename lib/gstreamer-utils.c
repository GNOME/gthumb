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
#include <gst/video/video.h>
#include "lib/gstreamer-utils.h"
#include "lib/util.h"
#include "lib/gth-metadata.h"
#include "lib/io/load-png.h"

#define MAX_WAITING_TIME (10 * GST_SECOND)

static gboolean gstreamer_initialized = FALSE;


typedef struct {
	GstElement *playbin;
	GstTagList *tagcache;
	gboolean has_audio;
	gboolean has_video;
	gint video_height;
	gint video_width;
	gint video_fps_n;
	gint video_fps_d;
	gint video_bitrate;
	char *video_codec;
	gint audio_channels;
	gint audio_samplerate;
	gint audio_bitrate;
	char *audio_codec;
	GCancellable *cancellable;
} MetadataExtractor;


static void reset_extractor_data (MetadataExtractor *extractor) {
	if (extractor->tagcache != NULL) {
		gst_tag_list_unref (extractor->tagcache);
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

	extractor->cancellable = NULL;
}


static MetadataExtractor* metadata_extractor_new (GFile *file, GCancellable *cancellable) {
	MetadataExtractor *extractor = g_slice_new0 (MetadataExtractor);
	reset_extractor_data (extractor);
	if (cancellable != NULL) {
		extractor->cancellable = g_object_ref (cancellable);
	}

	extractor->playbin = gst_element_factory_make ("playbin", "playbin");
	char *uri = g_file_get_uri (file);
	g_object_set (G_OBJECT (extractor->playbin),
		      "uri", uri,
		      "audio-sink", gst_element_factory_make ("fakesink", "fakesink-audio"),
		      "video-sink", gst_element_factory_make ("fakesink", "fakesink-video"),
		      NULL);
	g_free (uri);

	return extractor;
}


static void metadata_extractor_free (MetadataExtractor *extractor) {
	reset_extractor_data (extractor);
	gst_element_set_state (extractor->playbin, GST_STATE_NULL);
	gst_element_get_state (extractor->playbin, NULL, NULL, MAX_WAITING_TIME);
	gst_object_unref (GST_OBJECT (extractor->playbin));
	if (extractor->cancellable != NULL) {
		g_object_unref (extractor->cancellable);
	}
	g_slice_free (MetadataExtractor, extractor);
}


gboolean gstreamer_init (void) {
	if (!gstreamer_initialized) {
		GError *error = NULL;
		if (!gst_init_check (NULL, NULL, &error)) {
			g_warning ("%s", error->message);
			g_error_free (error);
			return FALSE;
		}
		gstreamer_initialized = TRUE;
	}
	return TRUE;
}


static void add_metadata (GFileInfo *info, const char *key, char *raw, char *formatted) {
	if (raw == NULL)
		return;

	if (strcmp (key, "Frame::Pixels") == 0) {
		g_file_info_set_attribute_string (info, key, raw);
		return;
	}
	else if (strcmp (key, "Metadata::Duration") == 0) {
		int secs;

		g_free (formatted);
		sscanf (raw, "%i", &secs);
		formatted = _g_format_duration_for_display (secs * 1000, NULL, NULL);
	}
	else if (strcmp (key, "Media::Bitrate") == 0) {
		int bps;

		g_free (formatted);
		sscanf (raw, "%i", &bps);
		formatted = g_strdup_printf ("%d kbps", bps / 1000);
	}

	GthMetadata *metadata = gth_metadata_new ();
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


static void add_metadata_from_tag (GFileInfo *info, const GstTagList *list, const char *tag, const char *tag_key) {
	GType tag_type = gst_tag_get_type (tag);
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
		guint ret = 0;
		if (gst_tag_list_get_uint (list, tag, &ret))
			add_metadata (info, tag_key, g_strdup_printf ("%u", ret), NULL);
	}

	if (tag_type == G_TYPE_CHAR) {
		int ret = 0;
		if (gst_tag_list_get_int (list, tag, &ret))
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
		guint64 ret = 0;
		if (gst_tag_list_get_uint64 (list, tag, &ret))
			add_metadata (info, tag_key, g_strdup_printf ("%" G_GUINT64_FORMAT, ret), NULL);
	}

	if (tag_type == G_TYPE_LONG) {
		gint64 ret = 0;
		if (gst_tag_list_get_int64 (list, tag, &ret))
			add_metadata (info, tag_key, g_strdup_printf ("%" G_GINT64_FORMAT, ret), NULL);
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


static void tag_iterate (const GstTagList *list, const char *tag, GFileInfo *info) {
	const char *tag_key = NULL;

	if (strcmp (tag, "container-format") == 0) {
		tag_key = "Metadata::Format";
	}
	else if (strcmp (tag, "bitrate") == 0) {
		tag_key = "Media::Bitrate";
	}
	else if (strcmp (tag, "encoder") == 0) {
		tag_key = "Media::Encoder";
	}
	else if (strcmp (tag, "title") == 0) {
		tag_key = "Metadata::Title";
	}
	else if (strcmp (tag, "artist") == 0) {
		tag_key = "Media::Artist";
	}
	else if (strcmp (tag, "album") == 0) {
		tag_key = "Media::Album";
	}
	else if (strcmp (tag, "audio-codec") == 0) {
		tag_key = "Media::Audio::Codec";
	}
	else if (strcmp (tag, "video-codec") == 0) {
		tag_key = "Media::Video::Codec";
	}

	char *attribute = NULL;
	if (tag_key == NULL) {
		attribute = g_strconcat ("Media::Other::", tag, NULL);

		GthMetadataInfo *metadata_info = gth_metadata_info_get (attribute);
		if (metadata_info == NULL) {
			metadata_info = gth_metadata_info_register (
				attribute,
				gst_tag_get_nick (tag),
				"Media::Other",
				GTH_METADATA_ALLOW_IN_PROPERTIES_VIEW,
				NULL
			);
			metadata_info->sort_order = 500;
		}
		tag_key = attribute;
	}

	add_metadata_from_tag (info, list, tag, tag_key);

	g_free (attribute);
}


static gint64 get_media_duration (MetadataExtractor *extractor) {
	g_return_val_if_fail (extractor, -1);
	g_return_val_if_fail (extractor->playbin, -1);

	GstFormat fmt = GST_FORMAT_TIME;
	gint64 duration = -1;
	if (gst_element_query_duration (extractor->playbin, fmt, &duration) && (duration >= 0)) {
		return duration / GST_SECOND;
	}
	else {
		return -1;
	}
}


static void extract_metadata (MetadataExtractor *extractor, GFileInfo *info) {
	if (extractor->audio_channels >= 0) {
		add_metadata (info,
			"Media::Audio::Channels",
			g_strdup_printf ("%d", (guint) extractor->audio_channels),
			g_strdup (extractor->audio_channels == 2 ? _("Stereo") : _("Mono")));
	}

	if (extractor->audio_samplerate >= 0) {
		add_metadata (info,
			"Media::Audio::SampleRate",
			g_strdup_printf ("%d", (guint) extractor->audio_samplerate),
			g_strdup_printf ("%d Hz", (guint) extractor->audio_samplerate));
	}

	if (extractor->audio_bitrate >= 0) {
		add_metadata (info,
			"Media::Audio::Bitrate",
			g_strdup_printf ("%d", (guint) extractor->audio_bitrate),
			g_strdup_printf ("%d bps", (guint) extractor->audio_bitrate));
	}

	if (extractor->video_height >= 0) {
		add_metadata (info,
			"Media::Video::Height",
			g_strdup_printf ("%d", (guint) extractor->video_height),
			NULL);
		g_file_info_set_attribute_int32 (info, "Frame::Height", extractor->video_height);
	}

	if (extractor->video_width >= 0) {
		add_metadata (info,
			"Media::Video::Width",
			g_strdup_printf ("%d", (guint) extractor->video_width),
			NULL);
		g_file_info_set_attribute_int32 (info, "Frame::Width", extractor->video_width);
	}

	if ((extractor->video_height >= 0) && (extractor->video_width >= 0)) {
		add_metadata (info,
			"Frame::Pixels",
			g_strdup_printf (_("%d × %d"), (guint) extractor->video_width, (guint) extractor->video_height),
			NULL);
	}

	if ((extractor->video_fps_n >= 0) && (extractor->video_fps_d >= 0)) {
		add_metadata (info,
			"Media::Video::FrameRate",
			g_strdup_printf ("%.7g", (gdouble) extractor->video_fps_n / (gdouble) extractor->video_fps_d),
			g_strdup_printf ("%.7g fps", (gdouble) extractor->video_fps_n / (gdouble) extractor->video_fps_d));
	}

	if (extractor->video_bitrate >= 0) {
		add_metadata (info,
			"Media::Video::Bitrate",
			g_strdup_printf ("%d", (guint) extractor->video_bitrate),
			g_strdup_printf ("%d bps", (guint) extractor->video_bitrate));
	}

	gint64 duration = get_media_duration (extractor);
	if (duration >= 0) {
		add_metadata (info,
			"Metadata::Duration",
			g_strdup_printf ("%" G_GINT64_FORMAT, duration),
			g_strdup_printf ("%" G_GINT64_FORMAT " sec", duration));
	}

	if (extractor->tagcache != NULL) {
		gst_tag_list_foreach (extractor->tagcache, (GstTagForeachFunc) tag_iterate, info);
	}
}


static void caps_set (GstPad *pad, MetadataExtractor *extractor, const char *type) {
	GstCaps *caps;
	if ((caps = gst_pad_get_current_caps (pad)) == NULL)
		return;

	GstStructure *structure = gst_caps_get_structure (caps, 0);
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


static void update_stream_info (MetadataExtractor *extractor) {
	GstElement *audio_sink;
	GstElement *video_sink;

	g_object_get (extractor->playbin,
		"audio-sink", &audio_sink,
		"video-sink", &video_sink,
		NULL);

	if (audio_sink != NULL) {
		GstPad *audio_pad = gst_element_get_static_pad (GST_ELEMENT (audio_sink), "sink");
		if (audio_pad != NULL) {
			GstCaps *caps;
			if ((caps = gst_pad_get_current_caps (audio_pad)) != NULL) {
				extractor->has_audio = TRUE;
				caps_set (audio_pad, extractor, "audio");
				gst_caps_unref (caps);
			}
		}
	}

	if (video_sink != NULL) {
		GstPad *video_pad;

		video_pad = gst_element_get_static_pad (GST_ELEMENT (video_sink), "sink");
		if (video_pad != NULL) {
			GstCaps *caps;

			if ((caps = gst_pad_get_current_caps (video_pad)) != NULL) {
				extractor->has_video = TRUE;
				caps_set (video_pad, extractor, "video");
				gst_caps_unref (caps);
			}
		}
	}
}


static gboolean message_loop_to_state_change (MetadataExtractor *extractor, GstState state) {
	g_return_val_if_fail (extractor, FALSE);
	g_return_val_if_fail (extractor->playbin, FALSE);

	GstBus *bus = gst_element_get_bus (extractor->playbin);
	GstMessageType events = (GST_MESSAGE_TAG | GST_MESSAGE_STATE_CHANGED | GST_MESSAGE_ERROR | GST_MESSAGE_EOS);

	for (;;) {
		GstMessage *message = gst_bus_timed_pop_filtered (bus, MAX_WAITING_TIME, events);
		if (message == NULL) {
			goto timed_out;
		}

		switch (GST_MESSAGE_TYPE (message)) {
		case GST_MESSAGE_STATE_CHANGED: {
			GstState old_state = GST_STATE_NULL;
			GstState new_state = GST_STATE_NULL;

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
			GstTagList *tag_list = NULL;
			gst_message_parse_tag (message, &tag_list);

			GstTagList *result = gst_tag_list_merge (extractor->tagcache, tag_list, GST_TAG_MERGE_KEEP);
			if (extractor->tagcache != NULL)
				gst_tag_list_unref (extractor->tagcache);
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


gboolean read_video_metadata (GFile *file, GFileInfo *info, GCancellable *cancellable, GError **error) {
	if (!gstreamer_init ()) {
		return FALSE;
	}
	MetadataExtractor *extractor = metadata_extractor_new (file, cancellable);
	gst_element_set_state (extractor->playbin, GST_STATE_PAUSED);
	if (message_loop_to_state_change (extractor, GST_STATE_PAUSED)) {
		extract_metadata (extractor, info);
	}
	metadata_extractor_free (extractor);
	return TRUE;
}
