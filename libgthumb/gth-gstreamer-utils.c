/*  GThumb
 *
 *  Copyright (C) 2007 Free Software Foundation, Inc.
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

#include <string.h>
#include <glib.h>
#include "gth-exif-utils.h"

#ifdef HAVE_GSTREAMER
#include <gst/gst.h>
#endif


#ifdef HAVE_GSTREAMER
typedef struct {
	GstElement	*playbin;

	GstTagList	*tagcache;
	GstTagList	*audiotags;
	GstTagList	*videotags;

	gboolean	has_audio;
	gboolean	has_video;

	gint		video_height;
	gint		video_width;
	gint		video_fps_n;
	gint		video_fps_d;
	gint		audio_channels;
	gint		audio_samplerate;
} MetadataExtractor;

static gboolean gstreamer_initialized = FALSE;

static void
caps_set (GObject *obj, MetadataExtractor *extractor, const gchar *type)
{
	GstPad		*pad;
	GstStructure	*s;
	GstCaps		*caps;

	pad = GST_PAD (obj);

	if (!(caps = gst_pad_get_negotiated_caps (pad))) {
		return;
	}

	s = gst_caps_get_structure (caps, 0);

	if (s) {
		if (!strcmp (type, "audio")) {
			if ((extractor->audio_channels != -1 && extractor->audio_samplerate != -1) ||
			    !(gst_structure_get_int (s, "channels", &extractor->audio_channels) &&
			      (gst_structure_get_int (s, "rate", &extractor->audio_samplerate)))) {

				return;
			}
		} else if (!strcmp (type, "video")) {
			if ((extractor->video_fps_n != -1 && extractor->video_fps_d != -1 && extractor->video_width != -1 && extractor->video_height != -1) ||
			    !(gst_structure_get_fraction (s, "framerate", &extractor->video_fps_n, &extractor->video_fps_d) &&
			      gst_structure_get_int (s, "width", &extractor->video_width) &&
			      gst_structure_get_int (s, "height", &extractor->video_height))) {

				return;
			}
		} else {
			g_assert_not_reached ();
		}
	}

	gst_caps_unref (caps);
}


static void
caps_set_audio (GObject *obj, MetadataExtractor *extractor)
{
        g_return_if_fail (obj);
        g_return_if_fail (extractor);

	caps_set (obj, extractor, "audio");
}


static void
caps_set_video (GObject *obj, MetadataExtractor *extractor)
{
        g_return_if_fail (obj);
        g_return_if_fail (extractor);

	caps_set (obj, extractor, "video");
}


static void
update_stream_info (MetadataExtractor *extractor)
{
	GList	*streaminfo;
	GstPad	*audiopad, *videopad;

	g_return_if_fail (extractor);

	streaminfo = NULL;
	audiopad = videopad = NULL;

	g_object_get (extractor->playbin, "stream-info", &streaminfo, NULL);
	streaminfo = g_list_copy (streaminfo);
	g_list_foreach (streaminfo, (GFunc) g_object_ref, NULL);

	for ( ; streaminfo; streaminfo = streaminfo->next) {
		GObject		*info;
		gint		type;
		GParamSpec	*pspec;
		GEnumValue	*val;

		info = streaminfo->data;

		if (!info) {
			continue;
		}

                type = -1;

		g_object_get (info, "type", &type, NULL);
		pspec = g_object_class_find_property (G_OBJECT_GET_CLASS (info), "type");
		val = g_enum_get_value (G_PARAM_SPEC_ENUM (pspec)->enum_class, type);

		if (!strcmp (val->value_nick, "audio")) {
			extractor->has_audio = TRUE;
			if (!audiopad) {
				g_object_get (info, "object", &audiopad, NULL);
			}
		} else if (!strcmp (val->value_nick, "video")) {
			extractor->has_video = TRUE;
			if (!videopad) {
				g_object_get (info, "object", &videopad, NULL);
			}
		}
	}

	if (audiopad) {
		GstCaps *caps;

		if ((caps = gst_pad_get_negotiated_caps (audiopad))) {
			caps_set_audio (G_OBJECT (audiopad), extractor);
			gst_caps_unref (caps);
		}
	}

	if (videopad) {
		GstCaps *caps;

		if ((caps = gst_pad_get_negotiated_caps (videopad))) {
			caps_set_video (G_OBJECT (videopad), extractor);
			gst_caps_unref (caps);
		}
	}

	g_list_foreach (streaminfo, (GFunc) g_object_unref, NULL);
	g_list_free (streaminfo);
}


/* We have a simple element. Add any metadata we know about to the hash table  */
static GList *
add_metadata (GList *metadata,
              gchar *key,
              gchar *value)
{
        GthMetadata *new_entry;

	if (value != NULL) {
		new_entry = g_new (GthMetadata, 1);
        	new_entry->category = GTH_METADATA_CATEGORY_GSTREAMER;
		new_entry->full_name = g_strdup (key);
		new_entry->display_name = key;
        	new_entry->formatted_value = value;
		new_entry->raw_value = NULL;
	       	new_entry->position = 0;
		new_entry->writeable = FALSE;
        	metadata = g_list_prepend (metadata, new_entry);
	}

        return metadata;
}


static gint64
get_media_duration (MetadataExtractor *extractor)
{
	gint64	  duration;
	GstFormat fmt;

	g_return_val_if_fail (extractor, -1);
	g_return_val_if_fail (extractor->playbin, -1);

	fmt = GST_FORMAT_TIME;

	duration = -1;

	if (gst_element_query_duration (extractor->playbin, &fmt, &duration) && duration >= 0) {
		return duration / GST_SECOND;
	} else {
		return -1;
	}
}


void tag_iterate (const GstTagList *list, const gchar *tag, GList **metadata)
{
	GType  tag_type;
	char  *tag_name;

	tag_type = gst_tag_get_type (tag);
	tag_name = g_strdup (gst_tag_get_nick (tag));


	/* ----- G_TYPE_BOOLEAN ----- */
	if (tag_type == G_TYPE_BOOLEAN) {
		gboolean ret;
		if (gst_tag_list_get_boolean (list, tag, &ret)) {
			if (ret) {
				*metadata = add_metadata (*metadata, tag_name, g_strdup ("TRUE"));
			} 
			else {
				*metadata = add_metadata (*metadata, tag_name, g_strdup ("FALSE"));
			}			
		}
		else
			g_free (tag_name);
	}


	/* ----- G_TYPE_STRING ----- */
	if (tag_type == G_TYPE_STRING) {
		char *ret = NULL;
		if (gst_tag_list_get_string (list, tag, &ret))
			*metadata = add_metadata (*metadata, tag_name, ret);
		else
			g_free (tag_name);
	}


	/* ----- G_TYPE_UCHAR ----- */
        if (tag_type == G_TYPE_UCHAR) {
                guchar ret = 0;
                if (gst_tag_list_get_uchar (list, tag, &ret))
                        *metadata = add_metadata (*metadata, tag_name, g_strdup_printf ("%u", ret));
                else
                        g_free (tag_name);
        }


        /* ----- G_TYPE_CHAR ----- */
        if (tag_type == G_TYPE_CHAR) {
                gchar ret = 0;
                if (gst_tag_list_get_char (list, tag, &ret))
                        *metadata = add_metadata (*metadata, tag_name, g_strdup_printf ("%d", ret));
                else
                        g_free (tag_name);
        }


        /* ----- G_TYPE_UINT ----- */
        if (tag_type == G_TYPE_UINT) {
                guint ret = 0;
                if (gst_tag_list_get_uint (list, tag, &ret))
                        *metadata = add_metadata (*metadata, tag_name, g_strdup_printf ("%u", ret));
                else
                        g_free (tag_name);
        }


        /* ----- G_TYPE_INT ----- */
        if (tag_type == G_TYPE_INT) {
                gint ret = 0;
                if (gst_tag_list_get_int (list, tag, &ret))
                        *metadata = add_metadata (*metadata, tag_name, g_strdup_printf ("%d", ret));
                else
                        g_free (tag_name);
        }


        /* ----- G_TYPE_ULONG ----- */
        if (tag_type == G_TYPE_ULONG) {
                gulong ret = 0;
                if (gst_tag_list_get_ulong (list, tag, &ret))
                        *metadata = add_metadata (*metadata, tag_name, g_strdup_printf ("%lu", ret));
                else
                        g_free (tag_name);
        }


        /* ----- G_TYPE_LONG ----- */
        if (tag_type == G_TYPE_LONG) {
                glong ret = 0;
                if (gst_tag_list_get_long (list, tag, &ret))
                        *metadata = add_metadata (*metadata, tag_name, g_strdup_printf ("%ld", ret));
                else
                        g_free (tag_name);
        }


        /* ----- G_TYPE_INT64 ----- */
        if (tag_type == G_TYPE_INT64) {
                gint64 ret = 0;
                if (gst_tag_list_get_int64 (list, tag, &ret))
                        *metadata = add_metadata (*metadata, tag_name, g_strdup_printf ("%" G_GINT64_FORMAT, ret));
                else
                        g_free (tag_name);
        }


        /* ----- G_TYPE_UINT64 ----- */
        if (tag_type == G_TYPE_UINT64) {
                guint64 ret = 0;
                if (gst_tag_list_get_uint64 (list, tag, &ret))
                        *metadata = add_metadata (*metadata, tag_name, g_strdup_printf ("%" G_GUINT64_FORMAT, ret));
                else
                        g_free (tag_name);
        }


        /* ----- G_TYPE_DOUBLE ----- */
        if (tag_type == G_TYPE_DOUBLE) {
                gdouble ret = 0;
                if (gst_tag_list_get_double (list, tag, &ret))
                        *metadata = add_metadata (*metadata, tag_name, g_strdup_printf ("%f", ret));
                else
                        g_free (tag_name);
        }


        /* ----- G_TYPE_FLOAT ----- */
        if (tag_type == G_TYPE_FLOAT) {
                gfloat ret = 0;
                if (gst_tag_list_get_float (list, tag, &ret))
                        *metadata = add_metadata (*metadata, tag_name, g_strdup_printf ("%f", ret));
                else
                        g_free (tag_name);
        }


        /* ----- G_TYPE_DATE ----- */
        if (tag_type == G_TYPE_DATE) {
                GDate *ret = NULL;
                if (gst_tag_list_get_date (list, tag, &ret)) {
			if (ret) {
				gchar buf[11];
				g_date_strftime (buf, 10, "%F", ret);
				*metadata = add_metadata (*metadata, tag_name, g_strdup (buf));
				}
			g_free (ret);
			}
                else
                        g_free (tag_name);
        }
	
}



static GList *
extract_metadata (MetadataExtractor *extractor, GList *metadata)
{
	gint64 duration;

        g_return_val_if_fail (extractor, NULL);

	if (extractor->audio_channels >= 0) {
		metadata = add_metadata (metadata,
					 g_strdup ("Structure:Audio:Channels"), 
					 g_strdup_printf ("%d", (guint) extractor->audio_channels));
	}

	
        if (extractor->audio_samplerate >= 0) {
                metadata = add_metadata (metadata,
                                         g_strdup ("Structure:Audio:Samplerate"),
                                         g_strdup_printf ("%d", (guint) extractor->audio_samplerate));
        }

        if (extractor->video_height >= 0) {
                metadata = add_metadata (metadata,
                                         g_strdup ("Structure:Video:Height"),
                                         g_strdup_printf ("%d", (guint) extractor->video_height));
        }

        if (extractor->video_width >= 0) {
                metadata = add_metadata (metadata,
                                         g_strdup ("Structure:Video:Width"),
                                         g_strdup_printf ("%d", (guint) extractor->video_width));
        }

        if (extractor->video_fps_n >= 0 && extractor->video_fps_d >= 0) {
                metadata = add_metadata (metadata,
                                         g_strdup ("Structure:Video:FrameRate"),
                                         g_strdup_printf ("%.7g", (gdouble) extractor->video_fps_n / (gdouble) extractor->video_fps_d));
        }

	duration = get_media_duration (extractor);
	if (duration >= 0) 
		metadata = add_metadata (metadata, g_strdup ("Structure:Duration"), g_strdup_printf ("%" G_GINT64_FORMAT, duration));

	if (extractor->tagcache)
		gst_tag_list_foreach (extractor->tagcache, (GstTagForeachFunc) tag_iterate, &metadata);

	return metadata;
}


static gboolean
message_loop_to_state_change (MetadataExtractor *extractor, GstState state)
{
	GstBus *bus;
	GstMessageType events;

	g_return_val_if_fail (extractor, FALSE);
	g_return_val_if_fail (extractor->playbin, FALSE);

	bus = gst_element_get_bus (extractor->playbin);

	events = (GST_MESSAGE_TAG | GST_MESSAGE_STATE_CHANGED | GST_MESSAGE_ERROR | GST_MESSAGE_EOS);

	for (;;) {
		GstMessage *message;

		message = gst_bus_timed_pop_filtered (bus, GST_SECOND * 5, events);

		if (!message) {
			goto timed_out;
		}

		switch (GST_MESSAGE_TYPE (message)) {
		case GST_MESSAGE_STATE_CHANGED: {
			GstState old_state, new_state;

			old_state = new_state = GST_STATE_NULL;

			gst_message_parse_state_changed (message, &old_state, &new_state, NULL);

			if (old_state == new_state) {
				break;
			}

			/* we only care about playbin (pipeline) state changes */
			if (GST_MESSAGE_SRC (message) != GST_OBJECT (extractor->playbin)) {
				break;
			}

			if (old_state == GST_STATE_READY && new_state == GST_STATE_PAUSED) {
				update_stream_info (extractor);

			} else if (old_state == GST_STATE_PAUSED && new_state == GST_STATE_READY) {
				/* clean metadata cache */

				if (extractor->tagcache) {
					gst_tag_list_free (extractor->tagcache);
					extractor->tagcache = NULL;
				}

				if (extractor->audiotags) {
					gst_tag_list_free (extractor->audiotags);
					extractor->audiotags = NULL;
				}

				if (extractor->videotags) {
					gst_tag_list_free (extractor->videotags);
					extractor->videotags = NULL;
				}

				extractor->has_audio = extractor->has_video = FALSE;

				extractor->video_fps_n = extractor->video_fps_d = -1;
				extractor->video_height = extractor->video_width = -1;
				extractor->audio_channels = -1;
				extractor->audio_samplerate = -1;
			}

			if (new_state == state) {
				gst_message_unref (message);
				goto success;
			}

			break;
		}

		case GST_MESSAGE_TAG: {
			GstTagList	  *tag_list, *result;
			GstElementFactory *f;

			tag_list = NULL;

			gst_message_parse_tag (message, &tag_list);

			GST_DEBUG ("Tags: %" GST_PTR_FORMAT, tag_list);

			/* all tags */
			result = gst_tag_list_merge (extractor->tagcache, tag_list, GST_TAG_MERGE_KEEP);

			if (extractor->tagcache) {
				gst_tag_list_free (extractor->tagcache);
			}

			extractor->tagcache = result;

			/* media-type-specific tags */
			if (GST_IS_ELEMENT (message->src) && (f = gst_element_get_factory (GST_ELEMENT (message->src)))) {
				const gchar *klass;
				GstTagList  **cache;

				klass = gst_element_factory_get_klass (f);

				cache = NULL;

				if (g_strrstr (klass, "Audio")) {
					cache = &extractor->audiotags;
				} else if (g_strrstr (klass, "Video")) {
					cache = &extractor->videotags;
				}

				if (cache) {
					result = gst_tag_list_merge (*cache, tag_list, GST_TAG_MERGE_KEEP);
					if (*cache) {
						gst_tag_list_free (*cache);
					}
					*cache = result;
				}
			}

			/* clean up */
			gst_tag_list_free (tag_list);

			break;
		}

		case GST_MESSAGE_ERROR: {
			gchar  *debug    = NULL;
			GError *gsterror = NULL;

			gst_message_parse_error (message, &gsterror, &debug);

			g_warning ("Error: %s (%s)", gsterror->message, debug);

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
#endif


GList *
gth_read_gstreamer (const gchar *uri, GList *metadata)
{
#ifdef HAVE_GSTREAMER	
	MetadataExtractor *extractor;
	gchar		  *mrl;
	GstElement	  *fakesink_audio, *fakesink_video;

	g_return_val_if_fail (uri, NULL);

	metadata = g_list_reverse (metadata);

	if(!gstreamer_initialized) {
		gst_init (NULL, NULL);
		gstreamer_initialized = TRUE;
	}

	/* set up */
	extractor = g_slice_new0 (MetadataExtractor);

	extractor->tagcache = NULL;
	extractor->audiotags = extractor->videotags = NULL;

	extractor->has_audio = extractor->has_video = FALSE;

	extractor->video_fps_n = extractor->video_fps_d = -1;
	extractor->video_height = extractor->video_width = -1;
	extractor->audio_channels = -1;
	extractor->audio_samplerate = -1;

	extractor->playbin = gst_element_factory_make ("playbin", "playbin");


	mrl = g_strconcat ("file://", uri, NULL);

	/* set playbin object */
	g_object_set (G_OBJECT (extractor->playbin), "uri", mrl, NULL);
	g_free (mrl);

	fakesink_audio = gst_element_factory_make ("fakesink", "fakesink-audio");
	g_object_set (G_OBJECT (extractor->playbin), "audio-sink", fakesink_audio, NULL);

	fakesink_video = gst_element_factory_make ("fakesink", "fakesink-video");
	g_object_set (G_OBJECT (extractor->playbin), "video-sink", fakesink_video, NULL);


	/* start to parse infos and extract them */
	gst_element_set_state (extractor->playbin, GST_STATE_PAUSED);

	message_loop_to_state_change (extractor, GST_STATE_PAUSED);

	metadata = extract_metadata (extractor, metadata);

	/* also clean up */
	gst_element_set_state (extractor->playbin, GST_STATE_NULL);

	gst_object_unref (GST_OBJECT (extractor->playbin));

	g_slice_free (MetadataExtractor, extractor);

	/* return to gthumb */
	metadata = g_list_reverse (metadata);
#endif

	return metadata;
}

