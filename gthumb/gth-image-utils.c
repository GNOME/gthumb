/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2001-2009 The Free Software Foundation, Inc.
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
#include <math.h>
#include <glib.h>
#include <glib/gi18n.h>
#include "cairo-scale.h"
#include "cairo-utils.h"
#include "glib-utils.h"
#include "gth-image-utils.h"
#include "gth-main.h"


gboolean
_g_buffer_resize_image (void          *buffer,
		        gsize          count,
		        GthFileData   *file_data,
		        int            max_width,
		        int            max_height,
		        void         **resized_buffer,
		        gsize         *resized_count,
		        GCancellable  *cancellable,
		        GError       **error)
{
	GInputStream       *istream;
	const char         *mime_type;
	GthImageLoaderFunc  loader_func;
	GthImage           *image;
	int                 width;
	int                 height;
	cairo_surface_t    *surface;
	cairo_surface_t    *scaled;
	gboolean            result;

	if ((max_width == -1) || (max_height == -1)) {
		*error = NULL;
		return FALSE;
	}

	istream = g_memory_input_stream_new_from_data (buffer, count, NULL);
	mime_type = _g_content_type_get_from_stream (istream, (file_data != NULL ? file_data->file : NULL), cancellable, NULL);
	if (mime_type == NULL) {
		g_set_error (error, G_IO_ERROR, G_IO_ERROR_NOT_SUPPORTED, "%s", _("No suitable loader available for this file type"));
		return FALSE;
	}

	loader_func = gth_main_get_image_loader_func (mime_type, GTH_IMAGE_FORMAT_CAIRO_SURFACE);
	if (loader_func == NULL) {
		g_set_error (error, G_IO_ERROR, G_IO_ERROR_NOT_SUPPORTED, "%s", _("No suitable loader available for this file type"));
		g_object_unref (istream);
		return FALSE;
	}

	image = loader_func (istream,
			     NULL,
			     -1,
			     &width,
			     &height,
			     NULL,
			     NULL,
			     cancellable,
			     error);
	if (image == NULL) {
		g_object_unref (istream);
		return FALSE;
	}

	if (! scale_keeping_ratio (&width, &height, max_width, max_height, FALSE)) {
		error = NULL;
		g_object_unref (image);
		g_object_unref (istream);
		return FALSE;
	}

	surface = gth_image_get_cairo_surface (image);
	scaled = _cairo_image_surface_scale (surface, width, height, SCALE_FILTER_BEST, NULL);
	if (scaled == NULL) {
		cairo_surface_destroy (surface);
		g_object_unref (image);
		g_object_unref (istream);
		return FALSE;
	}

	gth_image_set_cairo_surface (image, scaled);
	result = gth_image_save_to_buffer (image,
					   mime_type,
					   file_data,
					   (char **) resized_buffer,
					   resized_count,
					   cancellable,
					   error);

	cairo_surface_destroy (scaled);
	cairo_surface_destroy (surface);
	g_object_unref (image);
	g_object_unref (istream);

	return result;
}
