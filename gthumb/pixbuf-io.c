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
#include "pixbuf-utils.h"


#undef USE_PIXBUF_LOADER


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
	SavePixbufFile *file;

	if (save_data->current == NULL) {
		save_completed (save_data);
		return;
	}

	file = save_data->current->data;
	g_write_file_async (file->file,
			    file->buffer,
			    file->buffer_size,
			    (g_file_equal (save_data->data->file_data->file, file->file) ? save_data->data->replace : TRUE),
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
			gboolean          replace,
			GthFileDataFunc   ready_func,
			gpointer          ready_data)
{
	GthPixbufSaver *saver;
	GError         *error = NULL;
	void           *buffer;
	gsize           buffer_size;
	GdkPixbuf      *tmp_pixbuf;
	SavePixbufData *data;

	saver = gth_main_get_pixbuf_saver (mime_type);
	if (saver == NULL) {
		error = g_error_new (GTH_ERROR, GTH_ERROR_GENERIC, _("Could not find a suitable module to save the image as \"%s\""), mime_type);
		gth_file_data_ready_with_error (file_data, ready_func, ready_data, error);
		return;
	}

	tmp_pixbuf = gdk_pixbuf_copy (pixbuf);
	if (! gth_pixbuf_saver_save_pixbuf (saver,
					    tmp_pixbuf,
					    (char **)&buffer,
					    &buffer_size,
					    mime_type,
					    &error))
	{
		g_object_unref (saver);
		g_object_unref (tmp_pixbuf);
		gth_file_data_ready_with_error (file_data, ready_func, ready_data, error);
		return;
	}

	g_object_unref (saver);

	data = g_new0 (SavePixbufData, 1);
	data->file_data = g_object_ref (file_data);
	data->pixbuf = tmp_pixbuf;
	data->mime_type = mime_type;
	data->replace = replace;
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
		save_pixbuf_data_free (data);
		g_free (buffer);
		gth_file_data_ready_with_error (file_data, ready_func, ready_data, error);
		return;
	}

	save_files (data, ready_func, ready_data);
}


#ifdef USE_PIXBUF_LOADER


#define LOAD_BUFFER_SIZE (64*1024)


typedef struct {
	int requested_size;
	int original_width;
	int original_height;
	int loader_width;
	int loader_height;
} ScaleData;


static GdkPixbuf *
load_from_stream (GdkPixbufLoader  *loader,
		  GInputStream     *stream,
		  int               requested_size,
		  GCancellable     *cancellable,
		  GError          **error)
{
	GdkPixbuf *pixbuf;
	gssize     n_read;
	guchar     buffer[LOAD_BUFFER_SIZE];
	gboolean   res;

	res = TRUE;
	while (1) {
		if (requested_size > 0) {
			if (gdk_pixbuf_loader_get_pixbuf (loader) != NULL) {
				pixbuf = gdk_pixbuf_loader_get_pixbuf (loader);
				g_object_ref (pixbuf);
				gdk_pixbuf_loader_close (loader, NULL);
				return pixbuf;
			}
		}

		n_read = g_input_stream_read (stream,
					      buffer,
					      sizeof (buffer),
					      cancellable,
					      error);

		if (n_read < 0) {
			res = FALSE;
			error = NULL; /* Ignore further errors */
			break;
		}

		if (n_read == 0)
			break;

		if (!gdk_pixbuf_loader_write (loader,
					      buffer,
					      n_read,
					      error)) {
			res = FALSE;
			error = NULL;
			break;
		}
	}

	if (!gdk_pixbuf_loader_close (loader, error)) {
		res = FALSE;
		error = NULL;
	}

	pixbuf = NULL;
	if (res) {
		pixbuf = gdk_pixbuf_loader_get_pixbuf (loader);
		if (pixbuf)
			g_object_ref (pixbuf);
	}

	return pixbuf;
}


static void
pixbuf_loader_size_prepared_cb (GdkPixbufLoader *loader,
				int              width,
				int              height,
				gpointer         user_data)
{
	ScaleData *scale_data = user_data;

	scale_data->original_width = width;
	scale_data->original_height = height;
	scale_data->loader_width = width;
	scale_data->loader_height = height;

	if (scale_data->requested_size == -1)
		return;

	if (scale_keeping_ratio (&scale_data->loader_width,
				 &scale_data->loader_height,
				 scale_data->requested_size,
				 scale_data->requested_size,
				 FALSE))
	{
		gdk_pixbuf_loader_set_size (loader,
					    scale_data->loader_width,
					    scale_data->loader_height);
	}
}


#endif


GthImage *
gth_pixbuf_new_from_file (GthFileData   *file_data,
			  int            requested_size,
			  int           *original_width,
			  int           *original_height,
			  gboolean       scale_to_original,
			  GCancellable  *cancellable,
			  GError       **error)
{
#ifndef USE_PIXBUF_LOADER
	GthImage  *image;
	GdkPixbuf *pixbuf = NULL;
	char      *path;
	gboolean   scale_pixbuf;
	int        original_w;
	int        original_h;

	if (original_width != NULL)
		*original_width = -1;

	if (original_height != NULL)
		*original_height = -1;

	if (file_data == NULL)
		return NULL;

	path = g_file_get_path (file_data->file);

	scale_pixbuf = FALSE;
	original_w = -1;
	original_h = -1;

	if (requested_size > 0) {
		if (gdk_pixbuf_get_file_info (path, &original_w, &original_h) == NULL) {
			original_w = -1;
			original_h = -1;
		}
		if ((original_w > requested_size) || (original_h > requested_size))
			scale_pixbuf = TRUE;
	}

	if (scale_pixbuf)
		pixbuf = gdk_pixbuf_new_from_file_at_scale (path,
							    requested_size,
							    requested_size,
							    TRUE,
							    error);
	else
		pixbuf = gdk_pixbuf_new_from_file (path, error);

	if (pixbuf != NULL) {
		GdkPixbuf *rotated;

		rotated = gdk_pixbuf_apply_embedded_orientation (pixbuf);
		if (rotated != NULL) {
			original_w = gdk_pixbuf_get_width (rotated);
			original_h = gdk_pixbuf_get_height (rotated);
			g_object_unref (pixbuf);
			pixbuf = rotated;
		}
	}

	if (original_width != NULL)
		*original_width = original_w;
	if (original_height != NULL)
		*original_height = original_h;

	image = gth_image_new_for_pixbuf (pixbuf);

	_g_object_unref (pixbuf);
	g_free (path);

	return image;

#else

	GdkPixbuf       *pixbuf = NULL;
	ScaleData        scale_data;
	GInputStream    *stream;
	GdkPixbufLoader *pixbuf_loader;

	if (original_width != NULL)
		*original_width = -1;
	if (original_height != NULL)
		*original_height = -1;

	if (file_data == NULL)
		return NULL;

	stream = (GInputStream *) g_file_read (file_data->file, cancellable, error);
	if (stream == NULL)
		return NULL;

	scale_data.requested_size = requested_size;
	scale_data.original_width = -1;
	scale_data.original_height = -1;
	scale_data.loader_width = -1;
	scale_data.loader_height = -1;

	pixbuf_loader = gdk_pixbuf_loader_new ();
	g_signal_connect (pixbuf_loader,
			  "size-prepared",
			  G_CALLBACK (pixbuf_loader_size_prepared_cb),
			  &scale_data);

	pixbuf = load_from_stream (pixbuf_loader, stream, requested_size, cancellable, error);

	g_object_unref (pixbuf_loader);
	g_object_unref (stream);

	if ((pixbuf != NULL) && scale_to_original) {
		GdkPixbuf *tmp;

		tmp = gdk_pixbuf_scale_simple (pixbuf, scale_data.original_width, scale_data.original_height, GDK_INTERP_NEAREST);
		g_object_unref (pixbuf);
		pixbuf = tmp;
	}

	if (pixbuf != NULL) {
		GdkPixbuf *rotated;

		rotated = gdk_pixbuf_apply_embedded_orientation (pixbuf);
		if (rotated != NULL) {
			scale_data.original_width = gdk_pixbuf_get_width (rotated);
			scale_data.original_height = gdk_pixbuf_get_height (rotated);
			g_object_unref (pixbuf);
			pixbuf = rotated;
		}
	}

	if (original_width != NULL)
		*original_width = scale_data.original_width;
	if (original_height != NULL)
		*original_height = scale_data.original_height;

	return pixbuf;

#endif
}


GthImage *
gth_pixbuf_animation_new_from_file (GthFileData   *file_data,
				    int            requested_size,
				    int           *original_width,
				    int           *original_height,
				    gpointer       user_data,
				    GCancellable  *cancellable,
				    GError       **error)
{
	GthImage   *image = NULL;
	const char *mime_type;

	mime_type = gth_file_data_get_mime_type (file_data);
	if (mime_type == NULL)
		return NULL;

	if (g_content_type_equals (mime_type, "image/gif")) {
		GdkPixbufAnimation *animation;
		char               *path;

		path = g_file_get_path (file_data->file);
		if (path != NULL)
			animation = gdk_pixbuf_animation_new_from_file (path, error);

		image = gth_image_new ();
		gth_image_set_pixbuf_animation (image, animation);

		g_object_unref (animation);
		g_free (path);
	}
	else
		image = gth_pixbuf_new_from_file (file_data,
						  requested_size,
						  original_width,
						  original_height,
						  FALSE,
						  cancellable,
						  error);

	return image;
}
