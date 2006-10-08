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

#include <libgnomevfs/gnome-vfs-mime.h>
#include "file-utils.h"
#include "gtk-utils.h"
#include "pixbuf-utils.h"
#include "rotation-utils.h"
#include "jpegutils/jpeg-data.h"
#include "jpegutils/transupp.h"
#include "jpegutils/jpegtran.h"


RotationData*
rotation_data_new ()
{
	RotationData *rd;
	
	rd = g_new0 (RotationData, 1);
	rd->rot_type = GTH_TRANSFORM_ROTATE_0;
	rd->tran_type = GTH_TRANSFORM_NONE;
	
	return rd;
}


void
update_rotation_from_exif_data (const char   *path,
				RotationData *rot_data)
{
	GthExifOrientation orientation;

	rot_data->rot_type = GTH_TRANSFORM_ROTATE_0;
	rot_data->tran_type = GTH_TRANSFORM_NONE;

	path = get_file_path_from_uri (path);
	if (path == NULL)
		return;
	orientation = get_exif_tag_short (path, EXIF_TAG_ORIENTATION);

	switch (orientation) {
	case GTH_EXIF_ORIENTATION_TOP_LEFT:
		rot_data->rot_type = GTH_TRANSFORM_ROTATE_0;
		rot_data->tran_type = GTH_TRANSFORM_NONE;
		break;
	case GTH_EXIF_ORIENTATION_TOP_RIGHT:
		rot_data->rot_type = GTH_TRANSFORM_ROTATE_0;
		rot_data->tran_type = GTH_TRANSFORM_MIRROR;
		break;
	case GTH_EXIF_ORIENTATION_BOTTOM_RIGHT:
		rot_data->rot_type = GTH_TRANSFORM_ROTATE_180;
		rot_data->tran_type = GTH_TRANSFORM_NONE;
		break;
	case GTH_EXIF_ORIENTATION_BOTTOM_LEFT:
		rot_data->rot_type = GTH_TRANSFORM_ROTATE_180;
		rot_data->tran_type = GTH_TRANSFORM_MIRROR;
		break;
	case GTH_EXIF_ORIENTATION_LEFT_TOP:
		rot_data->rot_type = GTH_TRANSFORM_ROTATE_90;
		rot_data->tran_type = GTH_TRANSFORM_MIRROR;
		break;
	case GTH_EXIF_ORIENTATION_RIGHT_TOP:
		rot_data->rot_type = GTH_TRANSFORM_ROTATE_90;
		rot_data->tran_type = GTH_TRANSFORM_NONE;
		break;
	case GTH_EXIF_ORIENTATION_RIGHT_BOTTOM:
		rot_data->rot_type = GTH_TRANSFORM_ROTATE_90;
		rot_data->tran_type = GTH_TRANSFORM_FLIP;
		break;
	case GTH_EXIF_ORIENTATION_LEFT_BOTTOM:
		rot_data->rot_type = GTH_TRANSFORM_ROTATE_270;
		rot_data->tran_type = GTH_TRANSFORM_NONE;
		break;
	default:
		rot_data->rot_type = GTH_TRANSFORM_ROTATE_0;
		rot_data->tran_type = GTH_TRANSFORM_NONE;
		break;
	}
}

void
apply_transformation_exif (GtkWindow    *win,
			   const char   *path,
			   RotationData *rot_data)

{
	if (get_exif_tag_short(path,EXIF_TAG_ORIENTATION)) 
		update_orientation_field (path, rot_data);
	else 
		apply_transformation_jpeg(win, path, rot_data);
}


void
reset_orientation_field (const char *path)
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
			exif_set_short (entry->data, byte_order, 1);
		}
	}

	jpeg_data_save_file (jdata, path);

	exif_data_unref (edata);
	jpeg_data_unref (jdata);
}


void
apply_transformation_jpeg (GtkWindow    *win,
			   const char   *path,
			   RotationData *rot_data)
{
	int          rot_type = rot_data->rot_type; 
	int          tran_type = rot_data->tran_type; 
	char        *line;
	char        *tmp1, *tmp2;
	static int   count = 0;
	GError      *err = NULL;
	JXFORM_CODE  transf;
	char        *e1, *e2;

	path = get_file_path_from_uri (path);
	if (path == NULL)
		return;
	
	if ((rot_type == GTH_TRANSFORM_ROTATE_0) && (tran_type == GTH_TRANSFORM_NONE))
		return;

	if (rot_type == GTH_TRANSFORM_ROTATE_0)
		tmp1 = g_strdup (path);
	else {
		tmp1 = g_strdup_printf ("%s/gthumb.%d.%d",
					g_get_tmp_dir (), 
					getpid (),
					count++);

		switch (rot_type) {
		case GTH_TRANSFORM_ROTATE_90:
			transf = JXFORM_ROT_90;
			break;
		case GTH_TRANSFORM_ROTATE_180:
			transf = JXFORM_ROT_180;
			break;
		case GTH_TRANSFORM_ROTATE_270:
			transf = JXFORM_ROT_270;
			break;
		default:
			transf = JXFORM_NONE;
			break;
		}

		if (jpegtran ((char*)path, tmp1, transf, &err) != 0) {
			g_free (tmp1);
			if (err != NULL) 
				_gtk_error_dialog_from_gerror_run (win, &err);
			return;
		}
	}

	if (tran_type == GTH_TRANSFORM_NONE)
		tmp2 = g_strdup (tmp1);
	else {
		tmp2 = g_strdup_printf ("%s/gthumb.%d.%d",
					g_get_tmp_dir (), 
					getpid (),
					count++);

		switch (tran_type) {
		case GTH_TRANSFORM_MIRROR:
			transf = JXFORM_FLIP_H;
			break;
		case GTH_TRANSFORM_FLIP:
			transf = JXFORM_FLIP_V;
			break;
		default:
			transf = JXFORM_NONE;
			break;
		}

		if (jpegtran (tmp1, tmp2, transf, &err) != 0) {
			g_free (tmp1);
			if (err != NULL) 
				_gtk_error_dialog_from_gerror_run (win, &err);
			return;
		}

	}

	e1 = shell_escape (tmp2);
	e2 = shell_escape (path);

	line = g_strdup_printf ("mv -f %s %s", e1, e2);
	g_spawn_command_line_sync (line, NULL, NULL, NULL, &err);  

	if (err != NULL) {
		_gtk_error_dialog_from_gerror_run (win, &err);

	} else {
		if ((rot_type == GTH_TRANSFORM_ROTATE_90) || (rot_type == GTH_TRANSFORM_ROTATE_270))
			swap_xy_exif_fields (path);
		reset_orientation_field (path);
	}

	g_free (e1);
	g_free (e2);
	g_free (line);
	g_free (tmp1);
	g_free (tmp2);
}


void
apply_transformation_generic (GtkWindow    *win,
			      const char   *path,
			      RotationData *rot_data)
{
	int         rot_type = rot_data->rot_type; 
	int         tran_type = rot_data->tran_type; 
	GdkPixbuf  *pixbuf1, *pixbuf2;
	const char *mime_type;

	path = get_file_path_from_uri (path);
	if (path == NULL)
		return;

	if ((rot_type == GTH_TRANSFORM_ROTATE_0) && (tran_type == GTH_TRANSFORM_NONE))
		return;

	pixbuf1 = gdk_pixbuf_new_from_file (path, NULL);
	if (pixbuf1 == NULL)
		return;

	switch (rot_type) {
	case GTH_TRANSFORM_ROTATE_90:
		pixbuf2 = _gdk_pixbuf_copy_rotate_90 (pixbuf1, FALSE);
		break;
	case GTH_TRANSFORM_ROTATE_180:
		pixbuf2 = _gdk_pixbuf_copy_mirror (pixbuf1, TRUE, TRUE);
		break;
	case GTH_TRANSFORM_ROTATE_270:
		pixbuf2 = _gdk_pixbuf_copy_rotate_90 (pixbuf1, TRUE);
		break;
	default:
		pixbuf2 = pixbuf1;
		g_object_ref (pixbuf2);
		break;
	}
	g_object_unref (pixbuf1);

	switch (tran_type) {
	case GTH_TRANSFORM_MIRROR:
		pixbuf1 = _gdk_pixbuf_copy_mirror (pixbuf2, TRUE, FALSE);
		break;
	case GTH_TRANSFORM_FLIP:
		pixbuf1 = _gdk_pixbuf_copy_mirror (pixbuf2, FALSE, TRUE);
		break;
	default:
		pixbuf1 = pixbuf2;
		g_object_ref (pixbuf1);
		break;
	}
	g_object_unref (pixbuf2);

	mime_type = gnome_vfs_mime_type_from_name (path);
	if ((mime_type != NULL) && is_mime_type_writable (mime_type)) {
		GError      *error = NULL;
		const char  *image_type = mime_type + 6;
		if (! _gdk_pixbuf_save (pixbuf1, 
					path, 
					image_type, 
					&error, 
					NULL))
			_gtk_error_dialog_from_gerror_run (win, &error);
	}
	g_object_unref (pixbuf1);
}


ExifShort
get_next_value_rotation_90 (int value)
{
	static ExifShort new_value [8] = {6, 7, 8, 5, 2, 3, 4, 1};
	return new_value[value - 1];
}


ExifShort
get_next_value_mirror (int value)
{
	static ExifShort new_value [8] = {2, 1, 4, 3, 6, 5, 8, 7};
	return new_value[value - 1];
}


ExifShort
get_next_value_flip (int value)
{
	static ExifShort new_value [8] = {4, 3, 2, 1, 8, 7, 6, 5};
	return new_value[value - 1];
}


void
update_orientation_field (const char   *path,
			  RotationData *rot_data)
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
			ExifShort     value;

			byte_order = exif_data_get_byte_order (edata);
			value = exif_get_short (entry->data, byte_order);

			switch (rot_data->rot_type) {
			case GTH_TRANSFORM_ROTATE_90:
				value = get_next_value_rotation_90 (value);
				break;
			case GTH_TRANSFORM_ROTATE_180:
				value = get_next_value_rotation_90 (value);
				value = get_next_value_rotation_90 (value);
				break;
			case GTH_TRANSFORM_ROTATE_270:
				value = get_next_value_rotation_90 (value);
				value = get_next_value_rotation_90 (value);
				value = get_next_value_rotation_90 (value);
				break;
			default:
				break;
			}
			
			switch (rot_data->tran_type) {
			case GTH_TRANSFORM_MIRROR:
				value = get_next_value_mirror (value);
				break;
			case GTH_TRANSFORM_FLIP:
				value = get_next_value_flip (value);
				break;
			default:
				break;
			}

			exif_set_short (entry->data, byte_order, value);
		}
	}

	jpeg_data_save_file (jdata, path);

	exif_data_unref (edata);
	jpeg_data_unref (jdata);
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

