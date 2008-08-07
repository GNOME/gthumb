/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
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

#include <config.h>
#include <unistd.h>
#include <sys/types.h>

#include <glib/gi18n.h>
#include <gio/gio.h>
#include "file-utils.h"
#include "gconf-utils.h"
#include "gtk-utils.h"
#include "pixbuf-utils.h"
#include "rotation-utils.h"
#include "gthumb-error.h"
#include "preferences.h"
#include "jpegutils/jpeg-data.h"
#include "jpegutils/transupp.h"
#include "jpegutils/jpegtran.h"


#define RESPONSE_TRIM 1


typedef struct {
	GtkWidget        *dialog;
	GtkWindow        *parent_window;
	gboolean          parent_is_modal;
	TrimResponseFunc  done_func;
	gpointer          done_data;
} AskTrimData;


static void 
ask_whether_to_trim_response_cb (GtkDialog *dialog,
                                 gint       response,
                                 gpointer   user_data)
{
	AskTrimData *data = user_data;
	
 	gtk_widget_destroy (data->dialog);
	if (data->parent_is_modal)
		gtk_window_set_modal (data->parent_window, TRUE);

	if (data->done_func != NULL) {
		JpegMcuAction action;

		action = (response == RESPONSE_TRIM) ? JPEG_MCU_ACTION_TRIM : JPEG_MCU_ACTION_DONT_TRIM;
		data->done_func (action, data->done_data);
	}
	
	g_free (data);
}


void
ask_whether_to_trim (GtkWindow        *parent_window,
		     FileData         *file,
		     TrimResponseFunc  done_func,
		     gpointer          done_data)
{
	AskTrimData *data;
	char        *display_name;
	char        *msg;
	
	/* If the user disabled the warning dialog trim the image */

	if (! eel_gconf_get_boolean (PREF_MSG_JPEG_MCU_WARNING, TRUE)) {
		if (done_func != NULL)
			done_func (JPEG_MCU_ACTION_TRIM, done_data);
		return;
	}

	/*
	 * Image dimensions are not multiples of the jpeg minimal coding unit (mcu).
	 * Warn about possible image distortions along one or more edges.
	 */

	data = g_new0 (AskTrimData, 1);
	data->done_func = done_func;
	data->done_data = done_data;
	data->parent_window = parent_window;
	data->parent_is_modal = FALSE;
	
	if (parent_window != NULL) {
		data->parent_is_modal = gtk_window_get_modal (parent_window);
		if (data->parent_is_modal)
			gtk_window_set_modal (data->parent_window, FALSE);
	}

	display_name = basename_for_display (file->path);
	msg = g_strdup_printf (_("Problem transforming the image: %s"), display_name);
	data->dialog = _gtk_message_dialog_with_checkbutton_new (parent_window,
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

	g_signal_connect (G_OBJECT (data->dialog), "response",
			  G_CALLBACK (ask_whether_to_trim_response_cb),
			  data);
	gtk_widget_show (data->dialog);
}


gboolean
apply_transformation_jpeg (FileData       *file,
			   GthTransform    transform,
			   JpegMcuAction   mcu_action,
			   GError        **error)
{
	gboolean          result = TRUE;
	char             *tmp_dir = NULL;
	char             *tmp_output_file = NULL;
	JXFORM_CODE       transf;
	char		 *local_file;
	GFile            *gfile;
	GFileInfo        *info;

	if (file == NULL)
		return FALSE;

	if (transform == GTH_TRANSFORM_NONE)
		return TRUE;

	tmp_dir = get_temp_dir_name ();
	if (tmp_dir == NULL) {
		if (error != NULL)
			*error = g_error_new (GTHUMB_ERROR, 0, "%s", _("Could not create a temporary folder"));
		return FALSE;
	}

	local_file = get_cache_filename_from_uri (file->path);
	if (local_file == NULL) {
		if (error != NULL)
			*error = g_error_new (GTHUMB_ERROR, 0, "%s", _("Could not create a local temporary copy of the remote file."));
		result = FALSE;
		goto apply_transformation_jpeg__free_and_close;
	}
	gfile = g_file_new_for_uri (file->path);
	info = g_file_query_info (gfile, "owner::*,access::*", G_FILE_QUERY_INFO_NONE, NULL, NULL);
	g_object_unref (gfile);

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

	tmp_output_file = get_temp_file_name (tmp_dir, NULL);
	if (! jpegtran (local_file, tmp_output_file, transf, mcu_action, error)) {
		result = FALSE;
		goto apply_transformation_jpeg__free_and_close;
	}

	if (! local_file_move (tmp_output_file, local_file)) {
		if (error != NULL)
			*error = g_error_new (GTHUMB_ERROR, 0, "%s", _("Could not move temporary file to local destination. Check folder permissions."));
		result = FALSE;
		goto apply_transformation_jpeg__free_and_close;	
	}

	if (info != NULL) {
		char *local_uri;
		
		local_uri = get_uri_from_local_path (local_file);
		gfile = g_file_new_for_uri (file->path);
		g_file_set_attributes_from_info (gfile, info, G_FILE_QUERY_INFO_NONE, NULL, NULL);
		g_object_unref (info);
		g_object_unref (gfile);
		g_free (local_uri);
	}

apply_transformation_jpeg__free_and_close:

	local_dir_remove_recursive (tmp_dir);
	g_free (tmp_output_file);
	g_free (tmp_dir);

	return result;
}


gboolean
apply_transformation_generic (FileData     *file,
			      GthTransform  transform,
			      GError       **error)
{
	gboolean   success = TRUE;
	GdkPixbuf *original_pixbuf;
	GdkPixbuf *transformed_pixbuf;
	
	if (file == NULL)
		return FALSE;

	if (transform == GTH_TRANSFORM_NONE)
		return TRUE;

	original_pixbuf = gth_pixbuf_new_from_file (file, error, 0, 0, NULL);
	if (original_pixbuf == NULL)
		return FALSE;

	transformed_pixbuf = _gdk_pixbuf_transform (original_pixbuf, transform);
		
	if (is_mime_type_writable (file->mime_type)) {
		const char *image_type = file->mime_type + 6;
		char	   *local_file;
		
		image_type = file->mime_type + 6;
		local_file = get_cache_filename_from_uri (file->path);
	
		update_metadata (file);	
		success = _gdk_pixbuf_save (transformed_pixbuf,
					    local_file,
					    file->metadata,
					    image_type,
					    error,
					    NULL);
					    
		g_free (local_file);
	} 
	else {
		if (error != NULL)
			*error = g_error_new (GTHUMB_ERROR, 0, _("Image type not supported: %s"), file->mime_type);
		success = FALSE;
	}
	
	g_object_unref (transformed_pixbuf);
	g_object_unref (original_pixbuf);
	
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
