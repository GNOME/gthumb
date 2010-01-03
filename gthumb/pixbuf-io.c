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
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Street #330, Boston, MA 02111-1307, USA.
 */

#include <config.h>
#include <string.h>
#include <glib/gi18n.h>
#define GDK_PIXBUF_ENABLE_BACKEND 1
#include "gio-utils.h"
#include "glib-utils.h"
#include "gth-error.h"
#include "gth-hook.h"
#include "gth-main.h"
#include "gth-pixbuf-saver.h"
#include "pixbuf-io.h"


char *
get_pixbuf_type_from_mime_type (const char *mime_type)
{
	if (mime_type == NULL)
		return NULL;

	if (g_str_has_prefix (mime_type, "image/x-"))
		return g_strdup (mime_type + strlen ("image/x-"));
	else if (g_str_has_prefix (mime_type, "image/"))
		return g_strdup (mime_type + strlen ("image/"));
	else
		return g_strdup (mime_type);
}


typedef struct {
	SavePixbufData  *data;
	GthFileDataFunc  ready_func;
	gpointer         ready_data;
	GList           *current;
} SaveData;


static void
save_pixbuf_file_free (SavePixbufFile *file)
{
	g_object_unref (file->file);
	g_free (file->buffer);
	g_free (file);
}


static void
save_pixbuf_data_free (SavePixbufData *data)
{
	g_object_unref (data->file_data);
	g_object_unref (data->pixbuf);
	g_list_foreach (data->files, (GFunc) save_pixbuf_file_free, NULL);
	g_list_free (data->files);
	g_free (data);
}


static void
save_completed (SaveData *save_data)
{
	if (save_data->data->error != NULL)
		(*save_data->ready_func) (save_data->data->file_data, *save_data->data->error, save_data->ready_data);
	else
		(*save_data->ready_func) (save_data->data->file_data, NULL, save_data->ready_data);
	save_pixbuf_data_free (save_data->data);
	g_free (save_data);
}


static void save_current_file (SaveData *save_data);


static void
file_saved_cb (void     *buffer,
	       gsize     count,
	       GError   *error,
	       gpointer  user_data)
{
	SaveData *save_data = user_data;

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
	SavePixbufFile *file;

	if (save_data->current == NULL) {
		save_completed (save_data);
		return;
	}

	file = save_data->current->data;
	g_write_file_async (file->file,
			    file->buffer,
			    file->buffer_size,
			    G_PRIORITY_DEFAULT,
			    NULL,
			    file_saved_cb,
			    save_data);
}


static void
save_files (SavePixbufData  *data,
	    GthFileDataFunc  ready_func,
	    gpointer         ready_data)
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
_gdk_pixbuf_save_async (GdkPixbuf        *pixbuf,
			GthFileData      *file_data,
			const char       *mime_type,
			GthFileDataFunc   ready_func,
			gpointer          ready_data)
{
	GthPixbufSaver *saver;
	GError         *error = NULL;
	void           *buffer;
	gsize           buffer_size;
	SavePixbufData *data;

	saver = gth_main_get_pixbuf_saver (mime_type);
	if (saver == NULL) {
		error = g_error_new (GTH_ERROR, GTH_ERROR_GENERIC, _("Could not find a suitable module to save the image as \"%s\""), mime_type);
		gth_file_data_ready_with_error (file_data, ready_func, ready_data, error);
		return;
	}

	if (! gth_pixbuf_saver_save_pixbuf (saver,
					    pixbuf,
					    (char **)&buffer,
					    &buffer_size,
					    mime_type,
					    &error))
	{
		gth_file_data_ready_with_error (file_data, ready_func, ready_data, error);
		return;
	}

	data = g_new0 (SavePixbufData, 1);
	data->file_data = g_object_ref (file_data);
	data->pixbuf = g_object_ref (pixbuf);
	data->mime_type = mime_type;
	data->buffer = buffer;
	data->buffer_size = buffer_size;
	data->files = NULL;
	data->error = NULL;
	gth_hook_invoke ("save-pixbuf", data);

	if (data->error == NULL) {
		SavePixbufFile *file;

		file = g_new0 (SavePixbufFile, 1);
		file->file = g_object_ref (data->file_data->file);
		file->buffer = data->buffer;
		file->buffer_size = data->buffer_size;
		data->files = g_list_prepend (data->files, file);
	}
	else {
		g_free (buffer);
		gth_file_data_ready_with_error (file_data, ready_func, ready_data, error);
		return;
	}

	save_files (data, ready_func, ready_data);
}


GdkPixbuf*
gth_pixbuf_new_from_file (GthFileData  *file_data,
			  GError      **error,
			  int           requested_width,
			  int           requested_height)
{
	GdkPixbuf *pixbuf = NULL;
	char      *path;
	gboolean   scale_pixbuf;

	if (file_data == NULL)
		return NULL;

	path = g_file_get_path (file_data->file);

	scale_pixbuf = FALSE;
	if (requested_width > 0) {
		int w, h;

		if (gdk_pixbuf_get_file_info (path, &w, &h) == NULL) {
			w = -1;
			h = -1;
		}
		if ((w > requested_width) || (h > requested_height))
			scale_pixbuf = TRUE;
	}

	if (scale_pixbuf)
		pixbuf = gdk_pixbuf_new_from_file_at_scale (path,
							    requested_width,
							    requested_height,
							    TRUE,
							    error);
	else
		pixbuf = gdk_pixbuf_new_from_file (path, error);

	if (pixbuf != NULL) {
		GdkPixbuf *rotated;

		rotated = gdk_pixbuf_apply_embedded_orientation (pixbuf);
		if (rotated != NULL) {
			g_object_unref (pixbuf);
			pixbuf = rotated;
		}
	}

	g_free (path);

	return pixbuf;
}


GdkPixbufAnimation*
gth_pixbuf_animation_new_from_file (GthFileData  *file_data,
				    GError      **error,
				    int           requested_width,
				    int           requested_height)
{
	GdkPixbufAnimation *animation = NULL;
	const char         *mime_type;

	mime_type = gth_file_data_get_mime_type (file_data);
	if (mime_type == NULL)
		return NULL;

	if (g_content_type_equals (mime_type, "image/gif")) {
		char *path;

		path = g_file_get_path (file_data->file);
		animation = gdk_pixbuf_animation_new_from_file (path, error);

		g_free (path);

		return animation;
	}
 	else {
 		GdkPixbuf *pixbuf;

		pixbuf = gth_pixbuf_new_from_file (file_data,
						   error,
						   requested_width,
						   requested_height);

		if (pixbuf != NULL) {
			animation = gdk_pixbuf_non_anim_new (pixbuf);
			g_object_unref (pixbuf);
		}
 	}

	return animation;
}
