/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

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
#include <extensions/gstreamer_utils/gstreamer-utils.h>
#include <extensions/gstreamer_utils/gstreamer-thumbnail.h>

#define DEFAULT_THUMBNAIL_SIZE 256
#define INPUT_FILE 0
#define OUTPUT_FILE 1


static int thumbnail_size = 0;
static gchar **filenames = NULL;
static GOptionEntry entries[] = {
	{ "size", 's', 0, G_OPTION_ARG_INT, &thumbnail_size, "Thumbnail size (default 256)", "N" },
	{ G_OPTION_REMAINING, 0, 0, G_OPTION_ARG_FILENAME_ARRAY, &filenames, "The video file and the thumbnail file", "VIDEO_URI THUMBNAIL_PATH" },
	{ NULL },
};


int main (int argc, char *argv[])
{
	GError *error = NULL;
	GOptionContext *context;

	gst_init (0, NULL);

	/* Read the command line arguments. */

	context = g_option_context_new ("- generate a thumbnail for a video using gstreamer");
	g_option_context_add_main_entries (context, entries, GETTEXT_PACKAGE);
	if (!g_option_context_parse (context, &argc, &argv, &error)) {
		g_warning ("Option parsing failed: %s\n", error->message);
		return 1;
	}
	if ((filenames == NULL) || (filenames[INPUT_FILE] == NULL)) {
		g_warning ("Video file not specified.\n");
		return 2;
	}
	if (filenames[OUTPUT_FILE] == NULL) {
		g_warning ("Thumbnail file not specified.\n");
		return 3;
	}

	if (thumbnail_size <= 0)
		thumbnail_size = DEFAULT_THUMBNAIL_SIZE;

	/* Generate the thumbnail. */

	GFile *file = g_file_new_for_commandline_arg (filenames[INPUT_FILE]);
	if (file == NULL) {
		g_warning ("Invalid filename: '%s'\n", filenames[INPUT_FILE]);
		return 4;
	}

	GdkPixbuf *thumbnail = gstreamer_generate_thumbnail (file, &error);
	if (thumbnail == NULL) {
		g_warning ("Could not generate the thumbail for %s: %s\n", g_file_get_uri (file), error->message);
		return 5;
	}

	/* Scale to the requested size. */

	cairo_surface_t *surface = _cairo_image_surface_create_from_pixbuf (thumbnail);
	int width = cairo_image_surface_get_width (surface);
	int height = cairo_image_surface_get_height (surface);
	scale_keeping_ratio (&width, &height, thumbnail_size, thumbnail_size, FALSE);
	cairo_surface_t *scaled = _cairo_image_surface_scale (surface, width, height, SCALE_FILTER_BEST, NULL);
	if (scaled == NULL) {
		g_warning ("Could not scale the thumbail to %d\n", thumbnail_size);
		return 6;
	}

	/* Save the thumbnail. */

	if (cairo_surface_write_to_png (scaled, filenames[OUTPUT_FILE]) != CAIRO_STATUS_SUCCESS) {
		g_warning ("Could not save the thumbail to '%s'.\n", filenames[OUTPUT_FILE]);
		return 7;
	}

	/* Free the allocated resources. */

	cairo_surface_destroy (scaled);
	cairo_surface_destroy (surface);
	g_object_unref (thumbnail);
	g_object_unref (file);
	g_strfreev (filenames);
	g_option_context_free (context);

	return 0;
}
