/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2009 Free Software Foundation, Inc.
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
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include "gio-utils.h"
#include "glib-utils.h"
#include "gth-error.h"
#include "gth-hook.h"
#include "gth-main.h"
#include "gth-image-saver.h"


G_DEFINE_TYPE (GthImageSaver, gth_image_saver, G_TYPE_OBJECT)


static GtkWidget *
base_get_control (GthImageSaver *self)
{
	return gtk_label_new (_("No options available for this file type"));
}


static void
base_save_options (GthImageSaver *self)
{
	/* void */
}


static gboolean
base_can_save (GthImageSaver *self,
	       const char    *mime_type)
{
	return FALSE;
}


static gboolean
base_save_image (GthImageSaver  *self,
		 GthImage       *image,
		 char          **buffer,
		 gsize          *buffer_size,
		 const char     *mime_type,
		 GError        **error)
{
	return FALSE;
}


static void
gth_image_saver_class_init (GthImageSaverClass *klass)
{
	klass->id = "";
	klass->display_name = "";
	klass->get_control = base_get_control;
	klass->save_options = base_save_options;
	klass->can_save = base_can_save;
	klass->save_image = base_save_image;
}


static void
gth_image_saver_init (GthImageSaver *self)
{
	/* void */
}


const char *
gth_image_saver_get_id (GthImageSaver *self)
{
	return GTH_IMAGE_SAVER_GET_CLASS (self)->id;
}


const char *
gth_image_saver_get_display_name (GthImageSaver *self)
{
	return GTH_IMAGE_SAVER_GET_CLASS (self)->display_name;
}


const char *
gth_image_saver_get_mime_type (GthImageSaver *self)
{
	return GTH_IMAGE_SAVER_GET_CLASS (self)->mime_type;
}


const char *
gth_image_saver_get_extensions (GthImageSaver *self)
{
	return GTH_IMAGE_SAVER_GET_CLASS (self)->extensions;
}


const char *
gth_image_saver_get_default_ext (GthImageSaver *self)
{
	if (GTH_IMAGE_SAVER_GET_CLASS (self)->get_default_ext != NULL)
		return GTH_IMAGE_SAVER_GET_CLASS (self)->get_default_ext (self);
	else
		return gth_image_saver_get_extensions (self);
}


GtkWidget *
gth_image_saver_get_control (GthImageSaver *self)
{
	return GTH_IMAGE_SAVER_GET_CLASS (self)->get_control (self);
}


void
gth_image_saver_save_options (GthImageSaver *self)
{
	GTH_IMAGE_SAVER_GET_CLASS (self)->save_options (self);
}


gboolean
gth_image_saver_can_save (GthImageSaver *self,
			  const char    *mime_type)
{
	return GTH_IMAGE_SAVER_GET_CLASS (self)->can_save (self, mime_type);
}


static gboolean
gth_image_saver_save_image (GthImageSaver  *self,
			    GthImage       *image,
			    char          **buffer,
			    gsize          *buffer_size,
			    const char     *mime_type,
			    GError        **error)
{
	return GTH_IMAGE_SAVER_GET_CLASS (self)->save_image (self,
							     image,
							     buffer,
							     buffer_size,
							     mime_type,
							     error);
}


static GthImageSaveData *
_gth_image_save_to_buffer_common (GthImage     *image,
				  const char   *mime_type,
				  GthFileData  *file_data,
				  GError      **p_error)
{
	GthImageSaver    *saver;
	char             *buffer;
	gsize             buffer_size;
	GError           *error = NULL;
	GthImageSaveData *save_data = NULL;

	saver = gth_main_get_image_saver (mime_type);
	if (saver == NULL) {
		if (p_error != NULL)
			*p_error = g_error_new (GTH_ERROR, GTH_ERROR_GENERIC, _("Could not find a suitable module to save the image as \"%s\""), mime_type);
		return NULL;
	}

	if (gth_image_saver_save_image (saver,
					image,
					&buffer,
					&buffer_size,
					mime_type,
					&error))
	{
		save_data = g_new0 (GthImageSaveData, 1);
		save_data->file_data = _g_object_ref (file_data);
		save_data->image = gth_image_copy (image);
		save_data->mime_type = mime_type;
		save_data->buffer = buffer;
		save_data->buffer_size = buffer_size;
		save_data->files = NULL;
		save_data->error = NULL;

		if (save_data->file_data != NULL)
			gth_hook_invoke ("save-image", save_data);

		if ((save_data->error != NULL) && (p_error != NULL))
			*p_error = g_error_copy (*save_data->error);
	}
	else {
		if (p_error != NULL)
			*p_error = error;
		else
			_g_error_free (error);
	}

	g_object_unref (saver);

	return save_data;
}


static void
gth_image_save_file_free (GthImageSaveFile *file)
{
	g_object_unref (file->file);
	g_free (file->buffer);
	g_free (file);
}


static void
gth_image_save_data_free (GthImageSaveData *data)
{
	_g_object_unref (data->file_data);
	g_object_unref (data->image);
	g_list_foreach (data->files, (GFunc) gth_image_save_file_free, NULL);
	g_list_free (data->files);
	g_free (data);
}


gboolean
gth_image_save_to_buffer (GthImage     *image,
			  const char   *mime_type,
			  GthFileData  *file_data,
			  char        **buffer,
			  gsize        *buffer_size,
			  GError      **p_error)
{
	GthImageSaveData *save_data;

	g_return_val_if_fail (image != NULL, FALSE);

	save_data = _gth_image_save_to_buffer_common (image,
						      mime_type,
						      file_data,
						      p_error);

	if (save_data != NULL) {
		*buffer = save_data->buffer;
		*buffer_size = save_data->buffer_size;
		gth_image_save_data_free (save_data);
		return TRUE;
	}

	return FALSE;

}


/* -- gth_image_save_to_file -- */


typedef struct {
	GthImageSaveData *data;
	GthFileDataFunc   ready_func;
	gpointer          ready_data;
	GList            *current;
} SaveData;


static void
save_completed (SaveData *save_data)
{
	if (save_data->data->error != NULL)
		(*save_data->ready_func) (save_data->data->file_data, *save_data->data->error, save_data->ready_data);
	else
		(*save_data->ready_func) (save_data->data->file_data, NULL, save_data->ready_data);
	gth_image_save_data_free (save_data->data);
	g_free (save_data);
}


static void save_current_file (SaveData *save_data);


static void
file_saved_cb (void     **buffer,
	       gsize      count,
	       GError    *error,
	       gpointer   user_data)
{
	SaveData *save_data = user_data;

	*buffer = NULL; /* do not free the buffer, it's owned by file->buffer */

	if (error != NULL) {
		save_data->data->error = &error;
		save_completed (save_data);
		return;
	}

	save_data->current = save_data->current->next;
	save_current_file (save_data);
}


static void
save_current_file (SaveData *save_data)
{
	GthImageSaveFile *file;

	if (save_data->current == NULL) {
		save_completed (save_data);
		return;
	}

	file = save_data->current->data;
	_g_file_write_async (file->file,
			     file->buffer,
			     file->buffer_size,
			     (g_file_equal (save_data->data->file_data->file, file->file) ? save_data->data->replace : TRUE),
			     G_PRIORITY_DEFAULT,
			     NULL,
			     file_saved_cb,
			     save_data);
}


static void
save_files (GthImageSaveData *data,
	    GthFileDataFunc   ready_func,
	    gpointer          ready_data)
{
	SaveData *save_data;

	save_data = g_new0 (SaveData, 1);
	save_data->data = data;
	save_data->ready_func = ready_func;
	save_data->ready_data = ready_data;

	save_data->current = save_data->data->files;
	save_current_file (save_data);
}


void
gth_image_save_to_file (GthImage        *image,
			const char      *mime_type,
			GthFileData     *file_data,
			gboolean         replace,
			GthFileDataFunc  ready_func,
			gpointer         user_data)
{
	GthImageSaveData *data;
	GError           *error = NULL;

	data = _gth_image_save_to_buffer_common (image,
						 mime_type,
						 file_data,
						 &error);

	if (data == NULL) {
		gth_file_data_ready_with_error (file_data, ready_func, user_data, error);
		return;
	}

	data->replace = replace;

	if (data->error == NULL) {
		GthImageSaveFile *file;

		file = g_new0 (GthImageSaveFile, 1);
		file->file = g_object_ref (data->file_data->file);
		file->buffer = data->buffer;
		file->buffer_size = data->buffer_size;
		data->files = g_list_prepend (data->files, file);
	}
	else {
		gth_image_save_data_free (data);
		gth_file_data_ready_with_error (file_data, ready_func, user_data, error);
		return;
	}

	save_files (data, ready_func, user_data);
}
