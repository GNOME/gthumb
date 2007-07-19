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
#include <libgnomevfs/gnome-vfs-ops.h>
#include <libgnomevfs/gnome-vfs-utils.h>
#include "file-utils.h"
#include "gconf-utils.h"
#include "gtk-utils.h"
#include "pixbuf-utils.h"
#include "rotation-utils.h"
#include "preferences.h"
#include "jpegutils/jpeg-data.h"
#include "jpegutils/transupp.h"
#include "jpegutils/jpegtran.h"

#define RESPONSE_TRIM 1


typedef struct {
	const char  *path;
	GtkWindow   *parent;
} jpeg_mcu_dialog_data;


static boolean
jpeg_mcu_dialog (JXFORM_CODE *transform,
		 boolean     *trim,
		 jpeg_mcu_dialog_data *userdata)
{
	char      *display_name;
	char      *msg;
	GtkWidget *d;
	int        result;

	/* If the user disabled the warning dialog trim the image */

	if (! eel_gconf_get_boolean (PREF_MSG_JPEG_MCU_WARNING, TRUE)) {
		*trim = TRUE;
		return TRUE;
	}

	/*
	 * Image dimensions are not multiples of the jpeg minimal coding unit (mcu).
	 * Warn about possible image distortions along one or more edges.
	 */

	display_name = basename_for_display (userdata->path);
	msg = g_strdup_printf (_("Problem transforming the image: %s"), display_name);
	d = _gtk_message_dialog_with_checkbutton_new (
		userdata->parent,
		GTK_DIALOG_MODAL,
		GTK_STOCK_DIALOG_WARNING,
		msg,
		_("This transformation may introduce small image distortions along "
		  "one or more edges, because the image dimensions are not multiples of 8.\n\nThe distortion "
		  "is reversible, however. If the resulting image is unacceptable, simply apply the reverse "
		  "transformation to return to the original image.\n\nYou can also choose to discard (or trim) any "
		  "untransformable edge pixels. For practical use, this mode gives the best looking results, "
		  "but the transformation is not strictly lossless anymore."),
 		PREF_MSG_JPEG_MCU_WARNING,
		_("_Do not display this message again"),
		_("_Trim"), RESPONSE_TRIM,
		GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
		GTK_STOCK_OK, GTK_RESPONSE_OK,
		NULL);

	g_free (display_name);
	g_free (msg);

	result = gtk_dialog_run (GTK_DIALOG (d));
 	gtk_widget_destroy (d);

	switch (result) {
	case GTK_RESPONSE_OK:
		return TRUE;
	case RESPONSE_TRIM:
		*trim = TRUE;
		return TRUE;
	default:
		return FALSE;
	}
}


gboolean
apply_transformation_jpeg (GtkWindow    *win,
			   const char   *path,
			   GthTransform  transform)
{
	char                 *tmp;
	char                 *tmpdir;
	GError               *err = NULL;
	JXFORM_CODE           transf;
	jpeg_mcu_dialog_data  userdata = {path, win};
	gboolean	      is_local;
	gboolean	      remote_copy_ok;
	char		     *local_file_to_modify;
	gchar                *escaped_local_file;
	GnomeVFSFileInfo     *info;


	if (path == NULL)
		return FALSE;

	tmpdir = get_temp_dir_name ();
	if (tmpdir == NULL)
	{
		_gtk_error_dialog_run (GTK_WINDOW (win),
				      _("Could not create a temporary folder"));
		return FALSE;
	}
	tmp = get_temp_file_name (tmpdir, NULL);
	g_free (tmpdir);

	/* If the original file is stored on a remote VFS location, copy it to a local
	      temp file, modify it, then copy it back. This is easier than modifying the
	      underlying jpeg code (and other code) to handle VFS URIs. */

	is_local = is_local_file (path);
	local_file_to_modify = obtain_local_file (path);

	if (local_file_to_modify == NULL) {
		_gtk_error_dialog_run (win,
				       _("Could not create a local temporary copy of the remote file."));
		return FALSE;
	}

	if (!is_local) {
		info = gnome_vfs_file_info_new ();
		gnome_vfs_get_file_info (path, info, GNOME_VFS_FILE_INFO_GET_ACCESS_RIGHTS|GNOME_VFS_FILE_INFO_FOLLOW_LINKS);
	}

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

	if (jpegtran (local_file_to_modify, tmp, transf, (jpegtran_mcu_callback) jpeg_mcu_dialog, &userdata, &err) != 0) {
		if (err != NULL)
			_gtk_error_dialog_from_gerror_run (win, &err);
		remove_temp_file_and_dir (tmp);
		g_free (tmp);
		return FALSE;
	}

	escaped_local_file = gnome_vfs_escape_path_string (local_file_to_modify);
	if (!file_move (tmp, escaped_local_file)) {
		_gtk_error_dialog_run (win,
			_("Could not move temporary file to local destination. Check folder permissions."));
		remove_temp_file_and_dir (tmp);
		g_free (tmp);
		g_free (escaped_local_file);
		return FALSE;
	}

	if (!is_local)
		remote_copy_ok = copy_cache_file_to_remote_uri (escaped_local_file, path);

	g_free (escaped_local_file);
	g_free (local_file_to_modify);

	if (!is_local) {
		if (!remote_copy_ok) {
			_gtk_error_dialog_run (win, _("Could not move temporary file to remote location. Check remote permissions."));
	                remove_temp_file_and_dir (tmp);
        	        g_free (tmp);
                	return FALSE;
		} else {
		       gnome_vfs_set_file_info (path, info, GNOME_VFS_SET_FILE_INFO_PERMISSIONS|GNOME_VFS_SET_FILE_INFO_OWNER);
		}
		gnome_vfs_file_info_unref (info);
	}

	remove_temp_file_and_dir (tmp);
	g_free (tmp);

	/* report success */
	return TRUE;
}


gboolean
apply_transformation_generic (GtkWindow    *win,
			      const char   *path,
			      GthTransform transform)
{
	GdkPixbuf  *pixbuf1, *pixbuf2;
	const char *mime_type;
	gboolean     success = TRUE;

	if (path == NULL)
		return FALSE;

	if (transform == GTH_TRANSFORM_NONE)
		return FALSE;

	pixbuf1 = gth_pixbuf_new_from_uri (path, NULL, 0, 0, NULL);
	if (pixbuf1 == NULL)
		return FALSE;

	pixbuf2 = _gdk_pixbuf_transform (pixbuf1, transform);

	mime_type = gnome_vfs_mime_type_from_name (path);
	if ((mime_type != NULL) && is_mime_type_writable (mime_type)) {
		GError      *error = NULL;
		const char  *image_type = mime_type + 6;
		if (! _gdk_pixbuf_save (pixbuf2,
					path,
					image_type,
					&error,
					NULL)) {
			_gtk_error_dialog_from_gerror_run (win, &error);
			success = FALSE;
		}
	} else {
		_gtk_error_dialog_run (win,
				       _("Image type not supported: %s"),
				       mime_type);
		success = FALSE;
	}

	g_object_unref (pixbuf1);
	g_object_unref (pixbuf2);

	return success;
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
