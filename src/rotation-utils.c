/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2005 Free Software Foundation, Inc.
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

#include <config.h>
#include <unistd.h>
#include <sys/types.h>

#include <glib/gi18n.h>
#include <libgnomevfs/gnome-vfs-mime.h>
#include "file-utils.h"
#include "gtk-utils.h"
#include "pixbuf-utils.h"
#include "rotation-utils.h"
#include "jpegutils/jpeg-data.h"
#include "jpegutils/transupp.h"
#include "jpegutils/jpegtran.h"

GthTransform
read_orientation_field (const char *path)
{
	ExifShort orientation;

	path = get_file_path_from_uri (path);
	if (path == NULL)
		return GTH_TRANSFORM_NONE;

	orientation = get_exif_tag_short (path, EXIF_TAG_ORIENTATION);
	if (orientation >= 1 && orientation <= 8)
		return orientation;
	else
		return GTH_TRANSFORM_NONE;
}


void
write_orientation_field (const char   *path,
			 GthTransform  transform)
{
	JPEGData     *jdata;
	ExifData     *edata;
	unsigned int  i;

	path = get_file_path_from_uri (path);
	if (path == NULL)
		return;

	jdata = jpeg_data_new_from_file (path);
	if (jdata == NULL)
		return;

	edata = jpeg_data_get_exif_data (jdata);
	if (edata == NULL) {
		jpeg_data_unref (jdata);
		return;
	}

	for (i = 0; i < EXIF_IFD_COUNT; i++) {
		ExifContent *content = edata->ifd[i];
		ExifEntry   *entry;

		if ((content == NULL) || (content->count == 0)) 
			continue;

		entry = exif_content_get_entry (content, EXIF_TAG_ORIENTATION);
		if (entry != NULL) {
			ExifByteOrder byte_order;
			byte_order = exif_data_get_byte_order (edata);
			exif_set_short (entry->data, byte_order, transform);
		}
	}

	jpeg_data_save_file (jdata, path);

	exif_data_unref (edata);
	jpeg_data_unref (jdata);
}


void
apply_transformation_jpeg (GtkWindow    *win,
			   const char   *path,
			   GthTransform transform)
{
	char        *line;
	char        *tmp;
	static int   count = 0;
	GError      *err = NULL;
	JXFORM_CODE  transf;
	char        *e1, *e2;

	path = get_file_path_from_uri (path);
	if (path == NULL)
		return;
	
	tmp = g_strdup_printf ("%s/gthumb.%d.%d",
				g_get_tmp_dir (), 
				getpid (),
				count++);

	switch (transform) {
	case GTH_TRANSFORM_NONE:
		transf = JXFORM_NONE;
		break;
	case GTH_TRANSFORM_FLIP_H:
		transf = JXFORM_FLIP_H;
		break;
	case GTH_TRANSFORM_ROTATE_180:
		transf = JXFORM_ROT_180;
		break;
	case GTH_TRANSFORM_FLIP_V:
		transf = JXFORM_FLIP_V;
		break;
	case GTH_TRANSFORM_TRANSPOSE:
		transf = JXFORM_TRANSPOSE;
		break;
	case GTH_TRANSFORM_ROTATE_90:
		transf = JXFORM_ROT_90;
		break;
	case GTH_TRANSFORM_TRANSVERSE:
		transf = JXFORM_TRANSVERSE;
		break;
	case GTH_TRANSFORM_ROTATE_270:
		transf = JXFORM_ROT_270;
		break;
	default:
		transf = JXFORM_NONE;
		break;
	}

	if (jpegtran ((char*)path, tmp, transf, FALSE, &err) != 0) {
		g_free (tmp);
		if (err != NULL) 
			_gtk_error_dialog_from_gerror_run (win, &err);
		return;
	}

	e1 = shell_escape (tmp);
	e2 = shell_escape (path);

	line = g_strdup_printf ("mv -f %s %s", e1, e2);
	g_spawn_command_line_sync (line, NULL, NULL, NULL, &err);  

	if (err != NULL) {
		_gtk_error_dialog_from_gerror_run (win, &err);

	} else {
		GthTransform rot_type = get_rotation_part(transform);
		if ((rot_type == GTH_TRANSFORM_ROTATE_90) || 
				(rot_type == GTH_TRANSFORM_ROTATE_270))
			swap_xy_exif_fields (path);
		write_orientation_field (path, GTH_TRANSFORM_NONE);
	}

	g_free (e1);
	g_free (e2);
	g_free (line);
	g_free (tmp);
}


void
apply_transformation_generic (GtkWindow    *win,
			      const char   *path,
			      GthTransform transform)
{
	GdkPixbuf  *pixbuf1, *pixbuf2;
	const char *mime_type;

	path = get_file_path_from_uri (path);
	if (path == NULL)
		return;

	if (transform == GTH_TRANSFORM_NONE)
		return;

	pixbuf1 = gdk_pixbuf_new_from_file (path, NULL);
	if (pixbuf1 == NULL)
		return;

	pixbuf2 = _gdk_pixbuf_transform (pixbuf1, transform);

	mime_type = gnome_vfs_mime_type_from_name (path);
	if ((mime_type != NULL) && is_mime_type_writable (mime_type)) {
		GError      *error = NULL;
		const char  *image_type = mime_type + 6;
		if (! _gdk_pixbuf_save (pixbuf2, 
					path, 
					image_type, 
					&error, 
					NULL))
			_gtk_error_dialog_from_gerror_run (win, &error);
	} else
		_gtk_error_dialog_run (win,
				       _("Image type not supported: %s"),
				       mime_type);

	g_object_unref (pixbuf1);
	g_object_unref (pixbuf2);
}


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
get_next_transformation(GthTransform original, GthTransform transform)
{			
	GthTransform result = (original >= 1 && original <= 8 ? 
									original : 
									GTH_TRANSFORM_NONE);
	
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


GthTransform
get_rotation_part(GthTransform transform)
{
	static GthTransform lookup [8] = {
		GTH_TRANSFORM_NONE, GTH_TRANSFORM_NONE, 
		GTH_TRANSFORM_ROTATE_180, GTH_TRANSFORM_NONE, 
		GTH_TRANSFORM_ROTATE_90, GTH_TRANSFORM_ROTATE_90, 
		GTH_TRANSFORM_ROTATE_90, GTH_TRANSFORM_ROTATE_270};

	if (transform >= 1 && transform <= 8)
		return lookup[transform - 1];
	else
		return GTH_TRANSFORM_NONE;
}


GthTransform
get_mirror_or_flip_part(GthTransform transform)
{
	static GthTransform lookup [8] = {
		GTH_TRANSFORM_NONE, GTH_TRANSFORM_FLIP_H, 
		GTH_TRANSFORM_NONE, GTH_TRANSFORM_FLIP_V, 
		GTH_TRANSFORM_FLIP_H, GTH_TRANSFORM_NONE, 
		GTH_TRANSFORM_FLIP_V, GTH_TRANSFORM_NONE};

	if (transform >= 1 && transform <= 8)
		return lookup[transform - 1];
	else
		return GTH_TRANSFORM_NONE;
}


gboolean
swap_fields (ExifContent *content,
	     ExifTag      tag1,
	     ExifTag      tag2)
{
	ExifEntry     *entry1;
	ExifEntry     *entry2;
	unsigned char *data;
	unsigned int   size;
	
	entry1 = exif_content_get_entry (content, tag1);
	if (entry1 == NULL) 
		return FALSE;
	
	entry2 = exif_content_get_entry (content, tag2);
	if (entry2 == NULL)
		return FALSE;

	data = entry1->data;
	size = entry1->size;
	
	entry1->data = entry2->data;
	entry1->size = entry2->size;
	
	entry2->data = data;
	entry2->size = size;

	return TRUE;
}


void
swap_xy_exif_fields (const char *filename)
{
	JPEGData     *jdata;
	ExifData     *edata;
	unsigned int  i;

	filename = get_file_path_from_uri (filename);
	if (filename == NULL)
		return;

	jdata = jpeg_data_new_from_file (filename);
	if (jdata == NULL)
		return;

	edata = jpeg_data_get_exif_data (jdata);
	if (edata == NULL) {
		jpeg_data_unref (jdata);
		return;
	}

	for (i = 0; i < EXIF_IFD_COUNT; i++) {
		ExifContent *content = edata->ifd[i];

		if ((content == NULL) || (content->count == 0)) 
			continue;
		
		swap_fields (content, 
			     EXIF_TAG_RELATED_IMAGE_WIDTH,
			     EXIF_TAG_RELATED_IMAGE_LENGTH);
		swap_fields (content, 
			     EXIF_TAG_IMAGE_WIDTH,
			     EXIF_TAG_IMAGE_LENGTH);
		swap_fields (content, 
			     EXIF_TAG_PIXEL_X_DIMENSION,
			     EXIF_TAG_PIXEL_Y_DIMENSION);
		swap_fields (content, 
			     EXIF_TAG_X_RESOLUTION,
			     EXIF_TAG_Y_RESOLUTION);
		swap_fields (content, 
			     EXIF_TAG_FOCAL_PLANE_X_RESOLUTION,
			     EXIF_TAG_FOCAL_PLANE_Y_RESOLUTION);
	}

	jpeg_data_save_file (jdata, filename);

	exif_data_unref (edata);
	jpeg_data_unref (jdata);
}

