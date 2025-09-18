/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2007, 2009 Free Software Foundation, Inc.
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
#include <unistd.h>
#include <sys/types.h>
#include <glib/gi18n.h>
#include <gio/gio.h>
#include <extensions/exiv2_tools/exiv2-utils.h>
#include "rotation-utils.h"


/* -- get_next_transformation -- */


static GthTransform
get_next_value_rotation_90 (GthTransform value)
{
	static GthTransform new_value [8] = {6, 7, 8, 5, 2, 3, 4, 1};
	return new_value[value - 1];
}


static GthTransform
get_next_value_mirror (GthTransform value)
{
	static GthTransform new_value [8] = {2, 1, 4, 3, 6, 5, 8, 7};
	return new_value[value - 1];
}


static GthTransform
get_next_value_flip (GthTransform value)
{
	static GthTransform new_value [8] = {4, 3, 2, 1, 8, 7, 6, 5};
	return new_value[value - 1];
}


GthTransform
get_next_transformation (GthTransform original,
			 GthTransform transform)
{
	GthTransform result;

	result = ((original >= 1) && (original <= 8)) ? original : GTH_TRANSFORM_NONE;
	switch (transform) {
	case GTH_TRANSFORM_NONE:
		break;
	case GTH_TRANSFORM_ROTATE_90:
		result = get_next_value_rotation_90 (result);
		break;
	case GTH_TRANSFORM_ROTATE_180:
		result = get_next_value_rotation_90 (result);
		result = get_next_value_rotation_90 (result);
		break;
	case GTH_TRANSFORM_ROTATE_270:
		result = get_next_value_rotation_90 (result);
		result = get_next_value_rotation_90 (result);
		result = get_next_value_rotation_90 (result);
		break;
	case GTH_TRANSFORM_FLIP_H:
		result = get_next_value_mirror (result);
		break;
	case GTH_TRANSFORM_FLIP_V:
		result = get_next_value_flip (result);
		break;
	case GTH_TRANSFORM_TRANSPOSE:
		result = get_next_value_rotation_90 (result);
		result = get_next_value_mirror (result);
		break;
	case GTH_TRANSFORM_TRANSVERSE:
		result = get_next_value_rotation_90 (result);
		result = get_next_value_flip (result);
		break;
	}

	return result;
}


/* -- apply_transformation_async -- */


typedef struct {
	GthFileData *file_data;
	GthTransform transform;
	GthTransformFlags flags;
} TransformationData;


static void
transformation_data_free (TransformationData *tdata)
{
	_g_object_unref (tdata->file_data);
	g_free (tdata);
}


static void
_apply_transformation_async_thread (GTask        *task,
				    gpointer      source_object,
				    gpointer      task_data,
				    GCancellable *cancellable)
{
	TransformationData *tdata = g_task_get_task_data (task);
	GError *error = NULL;
	guint8 *buffer;
	gsize size;

	// Load the file
	if (!_g_file_load_in_buffer (tdata->file_data->file, (void **) &buffer, &size, cancellable, &error)) {
		g_task_return_error (task, error);
		return;
	}

	// Read the exif orientation
	if (!(tdata->flags & GTH_TRANSFORM_FLAG_SKIP_METADATA)) {
		if (!exiv2_read_metadata_from_buffer (buffer, size, tdata->file_data->info, FALSE, &error)) {
			g_free (buffer);
			g_task_return_error (task, error);
			return;
		}
	}

	// Change orientation
	GthMetadata *orientation_tag = NULL;
	if (g_file_info_has_attribute (tdata->file_data->info, "Exif::Image::Orientation")) {
		orientation_tag = (GthMetadata *) g_file_info_get_attribute_object (tdata->file_data->info, "Exif::Image::Orientation");
	}
	GthTransform orientation = GTH_TRANSFORM_NONE;
	if (tdata->flags & GTH_TRANSFORM_FLAG_RESET) {
		orientation = tdata->transform;
	}
	else {
		if ((orientation_tag != NULL) && (gth_metadata_get_raw (orientation_tag) != NULL)) {
			orientation = strtol (gth_metadata_get_raw (orientation_tag), (char **) NULL, 10);
		}
		orientation = get_next_transformation (orientation, tdata->transform);
	}
	char *raw_orientation = g_strdup_printf ("%d", orientation);
	if (orientation_tag == NULL) {
		orientation_tag = g_object_new (GTH_TYPE_METADATA,
			"id", "Exif::Image::Orientation",
			"value-type", "Short",
			"raw", raw_orientation,
			NULL);
		g_file_info_set_attribute_object (tdata->file_data->info,
			"Exif::Image::Orientation",
			G_OBJECT (orientation_tag));
	}
	else {
		g_object_set (orientation_tag, "raw", raw_orientation, NULL);
	}
	g_free (raw_orientation);

	const char *mime_type = gth_file_data_get_mime_type (tdata->file_data);
	gboolean change_orientation_tag = !(tdata->flags & GTH_TRANSFORM_FLAG_CHANGE_IMAGE) &&
		(g_content_type_equals (mime_type, "image/jpeg")
			|| g_content_type_equals (mime_type, "image/tiff")
			|| g_content_type_equals (mime_type, "image/webp"));

	if (change_orientation_tag) {
		// Change the exif orientation.
		update_exif_dimensions (tdata->file_data->info, tdata->transform);
		if (!exiv2_write_metadata_to_buffer ((void **) &buffer, &size, tdata->file_data->info, NULL, &error)) {
			g_free (buffer);
			g_task_return_error (task, error);
			return;
		}
	}
	else if ((tdata->transform != GTH_TRANSFORM_NONE) || (tdata->flags & GTH_TRANSFORM_FLAG_ALWAYS_SAVE)) {
		// Rotate the image
		GInputStream *istream = g_memory_input_stream_new_from_data (buffer, size, NULL);
		GthImage *image = gth_image_new_from_stream (istream, -1, NULL, NULL, cancellable, &error);
		g_object_unref (istream);
		g_free (buffer);
		if (image == NULL) {
			g_task_return_error (task, error);
			return;
		}

		if (tdata->transform != GTH_TRANSFORM_NONE) {
			cairo_surface_t *surface = gth_image_get_cairo_surface (image);
			cairo_surface_t *transformed = _cairo_image_surface_transform (surface, tdata->transform);
			gth_image_set_cairo_surface (image, transformed);
			cairo_surface_destroy (transformed);
			cairo_surface_destroy (surface);
		}

		buffer = NULL;
		gboolean saved = gth_image_save_to_buffer (image,
			mime_type,
			tdata->file_data,
			(char**) &buffer,
			&size,
			cancellable,
			&error);

		g_object_unref (image);

		if (!saved) {
			g_free (buffer);
			g_task_return_error (task, error);
			return;
		}
	}
	else {
		// No changes
		g_free (buffer);
		g_task_return_boolean (task, TRUE);
		return;
	}

	// Save buffer to file.
	gboolean file_saved = _g_file_write (tdata->file_data->file,
		FALSE,
		G_FILE_CREATE_REPLACE_DESTINATION,
		buffer,
		size,
		cancellable,
		&error);
	g_free (buffer);

	if (!file_saved) {
		g_task_return_error (task, error);
	}
	else {
		g_task_return_boolean (task, TRUE);
	}
}


void
apply_transformation_async (GthFileData         *file_data,
			    GthTransform         transform,
			    GthTransformFlags    flags,
			    GCancellable        *cancellable,
			    GAsyncReadyCallback  callback,
			    gpointer             user_data)
{
	TransformationData *tdata;

	tdata = g_new0 (TransformationData, 1);
	tdata->file_data = g_object_ref (file_data);
	tdata->transform = transform;
	tdata->flags = flags;

	GTask *task = g_task_new (NULL, cancellable, callback, user_data);
	g_task_set_task_data (task, tdata, (GDestroyNotify) transformation_data_free);
	g_task_run_in_thread (task, _apply_transformation_async_thread);

	g_object_unref (task);
}


gboolean
apply_transformation_finish (GAsyncResult *result,
			     GError **error)
{
	return g_task_propagate_boolean (G_TASK (result), error);
}
