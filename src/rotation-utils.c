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

	set_orientation_in_exif_data (transform, edata);

	jpeg_data_save_file (jdata, path);

	exif_data_unref (edata);
	jpeg_data_unref (jdata);
}


static boolean
jpeg_mcu_dialog (JXFORM_CODE *transform, boolean *trim, GtkWindow *parent)
{
	/*
	 * Image dimensions are not multiples of the jpeg minimal coding unit (mcu).
	 * Warn about possible image distortions along one or more edges.
	 */

 	GtkWidget *d =  _gtk_message_dialog_new (parent,
		GTK_DIALOG_MODAL,
		GTK_STOCK_DIALOG_WARNING,
		_("This transformation may introduce small image distortions along "
		"one or more edges, because the image dimensions are not multiples of 8.\n\nThe distortion "
		"is reversible, however. If the resulting image is unacceptable, simply apply the reverse "
		"transformation to return to the original image.\n\ngThumb can also discard (or trim) any "
		"untransformable edge pixels. For practical use, this mode gives the best looking results, "
		"but the transformation is not strictly lossless anymore.\n\nTo avoid this problem in the "
		"future, consider disabling the \"Apply physical transform\" option in the rotation dialog."),
		NULL,
		_("Trim"), 1,
		GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
		GTK_STOCK_OK, GTK_RESPONSE_OK,
		NULL);

	gint result = gtk_dialog_run (GTK_DIALOG (d));

 	gtk_widget_destroy (d);

	switch (result) {
	case GTK_RESPONSE_OK:
		return TRUE;
	case GTK_RESPONSE_CANCEL:
		return FALSE;
	case 1:
		*trim = TRUE;
		return TRUE;
	default:
		return FALSE;
	}
}


void
apply_transformation_jpeg (GtkWindow    *win,
			   const char   *path,
			   GthTransform  transform)
{
	char        *line;
	char        *tmp;
	char        *tmpdir;
	GError      *err = NULL;
	JXFORM_CODE  transf;
	char        *e1, *e2;

	path = get_file_path_from_uri (path);
	if (path == NULL)
		return;
	
	tmpdir = get_temp_dir_name ();
	if (tmpdir == NULL)
	{
		_gtk_error_dialog_run (GTK_WINDOW (win), 
				      _("Could not create a temporary folder"));
		return;
	}
	
	tmp = get_temp_file_name (tmpdir, NULL);

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

	if (jpegtran ((char*)path, tmp, transf, (jpegtran_mcu_callback) jpeg_mcu_dialog, win, &err) != 0) {
		g_free (tmp);
		if (err != NULL) 
			_gtk_error_dialog_from_gerror_run (win, &err);
		dir_remove (tmpdir);
		g_free (tmpdir);
		return;
	}

	e1 = shell_escape (tmp);
	e2 = shell_escape (path);

	line = g_strdup_printf ("mv -f %s %s", e1, e2);
	g_spawn_command_line_sync (line, NULL, NULL, NULL, &err);  
	dir_remove (tmpdir);
	g_free (tmpdir);

	if (err != NULL)
		_gtk_error_dialog_from_gerror_run (win, &err);

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

	pixbuf1 = gth_pixbuf_new_from_uri (path, NULL);
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
