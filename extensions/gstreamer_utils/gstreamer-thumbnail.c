/* -*- Mode: CPP; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2022 Free Software Foundation, Inc.
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
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <config.h>
#include <gst/gst.h>
#include <gst/video/video.h>
#include <gthumb.h>
#include "gstreamer-thumbnail.h"

#define MAX_WAITING_TIME (10 * GST_SECOND)


static void
destroy_pixbuf (guchar *pix, gpointer data)
{
	gst_sample_unref (GST_SAMPLE (data));
}


GdkPixbuf *
gstreamer_generate_thumbnail (GFile   *file,
			      GError **error)
{
	GstElement   *playbin = NULL;
	GstElement   *videoflip;
	char         *uri;
	gint64        duration;
	gint64        thumbnail_position;
	GstSample    *sample = NULL;
	GstCaps      *caps;
	GstCaps      *sample_caps;
	GstStructure *cap_struct;
	const char   *format;
	int           width, height;
	GdkPixbuf    *pixbuf = NULL;

	/* Create the playbin. */
	playbin = gst_element_factory_make ("playbin", "playbin");
	videoflip = gst_element_factory_make ("videoflip", "videoflip");
	if (videoflip != NULL) {
		g_object_set (videoflip, "video-direction", GST_VIDEO_ORIENTATION_AUTO, NULL);
		g_object_set (playbin, "video-filter", videoflip, NULL);
	}
	uri = g_file_get_uri (file);
	g_object_set (G_OBJECT (playbin),
		      "uri", uri,
		      "audio-sink", gst_element_factory_make ("fakesink", "fakesink-audio"),
		      "video-sink", gst_element_factory_make ("fakesink", "fakesink-video"),
		      NULL);
	g_free (uri);

	/* Set the playbin to paused to query duration. */
	gst_element_set_state (playbin, GST_STATE_PAUSED);
	gst_element_get_state (playbin, NULL, NULL, MAX_WAITING_TIME);

	/* Get the video length. */
	if (! gst_element_query_duration (playbin, GST_FORMAT_TIME, &duration)) {
		g_set_error_literal (error,
				     GDK_PIXBUF_ERROR,
				     GDK_PIXBUF_ERROR_UNSUPPORTED_OPERATION,
				     "Could not get the media length.");
		goto error;
	}

	/* Seek to the thumbnail position. */
	thumbnail_position = duration / 3;
	if (! gst_element_seek_simple (playbin,
				       GST_FORMAT_TIME,
				       GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_ACCURATE,
				       thumbnail_position))
	{
		g_set_error_literal (error,
				     GDK_PIXBUF_ERROR,
				     GDK_PIXBUF_ERROR_UNSUPPORTED_OPERATION,
				     "Seek failed.");
		goto error;
	}
	gst_element_get_state (playbin, NULL, NULL, MAX_WAITING_TIME);

	/* Get the sample. */
	caps = gst_caps_new_full (
		gst_structure_new ("video/x-raw",
			"format", G_TYPE_STRING, "RGB",
			NULL),
		gst_structure_new ("video/x-raw",
			"format", G_TYPE_STRING, "RGBA",
			NULL),
		NULL);
	g_signal_emit_by_name (playbin, "convert-sample", caps, &sample);
	if (sample == NULL) {
		g_set_error_literal (error,
				     G_IO_ERROR,
				     G_IO_ERROR_FAILED,
				     "Failed to convert the video frame.");
		goto error;
	}
	gst_caps_unref (caps);

	/* Check the sample format. */
	sample_caps = gst_sample_get_caps (sample);
	if (sample_caps == NULL) {
		g_set_error_literal (error,
				     G_IO_ERROR,
				     G_IO_ERROR_FAILED,
				     "No caps on output buffer.");
		goto error;
	}

	cap_struct = gst_caps_get_structure (sample_caps, 0);
	format = gst_structure_get_string (cap_struct, "format");
	if (! _g_str_equal (format, "RGB") && ! _g_str_equal (format, "RGBA")) {
		g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_FAILED, "Wrong sample format.");
		goto error;
	}

	/* Create the pixbuf from the sample data. */
	gst_structure_get_int (cap_struct, "width", &width);
	gst_structure_get_int (cap_struct, "height", &height);
	if ((width > 0) && (height > 0)) {
		GstMemory  *memory;
		GstMapInfo  info;
		gboolean    with_alpha = _g_str_equal (format, "RGBA");

		memory = gst_buffer_get_memory (gst_sample_get_buffer (sample), 0);
		if (gst_memory_map (memory, &info, GST_MAP_READ))
			pixbuf = gdk_pixbuf_new_from_data (
				info.data,
				GDK_COLORSPACE_RGB,
				with_alpha,
				8,
				width,
				height,
				GST_ROUND_UP_4 (width * (with_alpha ? 4 : 3)),
				destroy_pixbuf,
				sample);

		gst_memory_unmap (memory, &info);
		gst_memory_unref (memory);
	}

	if (pixbuf == NULL) {
		g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_FAILED, "Could not create the pixbuf.");
		goto error;
	}

 error:
	if ((pixbuf == NULL) && (sample != NULL))
		gst_sample_unref (sample);

	if (playbin != NULL) {
		gst_element_set_state (playbin, GST_STATE_NULL);
		gst_element_get_state (playbin, NULL, NULL, GST_CLOCK_TIME_NONE);
		gst_object_unref (GST_OBJECT (playbin));
	}

	return pixbuf;
}
