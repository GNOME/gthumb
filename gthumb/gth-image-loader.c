/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2001-2008 The Free Software Foundation, Inc.
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
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gtk/gtk.h>
#include "gth-file-data.h"
#include "gth-image-loader.h"
#include "gth-main.h"


struct _GthImageLoaderPrivate {
	gboolean     as_animation;  /* Whether to load the image in a
				     * GdkPixbufAnimation structure. */
	PixbufLoader loader_func;
	gpointer     loader_data;
};


static gpointer parent_class = NULL;


static void
gth_image_loader_finalize (GObject *object)
{
	G_OBJECT_CLASS (parent_class)->finalize (object);
}


static void
gth_image_loader_class_init (GthImageLoaderClass *class)
{
	GObjectClass *object_class;

	parent_class = g_type_class_peek_parent (class);
	g_type_class_add_private (class, sizeof (GthImageLoaderPrivate));

	object_class = G_OBJECT_CLASS (class);
	object_class->finalize = gth_image_loader_finalize;

}


static void
gth_image_loader_init (GthImageLoader *self)
{
	self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, GTH_TYPE_IMAGE_LOADER, GthImageLoaderPrivate);
	self->priv->as_animation = FALSE;
	self->priv->loader_func = NULL;
	self->priv->loader_data = NULL;
}


GType
gth_image_loader_get_type (void)
{
	static GType type = 0;

	if (! type) {
		GTypeInfo type_info = {
			sizeof (GthImageLoaderClass),
			NULL,
			NULL,
			(GClassInitFunc) gth_image_loader_class_init,
			NULL,
			NULL,
			sizeof (GthImageLoader),
			0,
			(GInstanceInitFunc) gth_image_loader_init
		};

		type = g_type_register_static (G_TYPE_OBJECT,
					       "GthImageLoader",
					       &type_info,
					       0);
	}

	return type;
}


GthImageLoader *
gth_image_loader_new (PixbufLoader loader_func,
		      gpointer     loader_data)
{
	GthImageLoader *self;

	self = g_object_new (GTH_TYPE_IMAGE_LOADER, NULL);
	gth_image_loader_set_loader_func (self, loader_func, loader_data);

	return self;
}


void
gth_image_loader_set_loader_func (GthImageLoader *self,
				  PixbufLoader    loader_func,
				  gpointer        loader_data)
{
	g_return_if_fail (self != NULL);

	self->priv->loader_func = loader_func;
	self->priv->loader_data = loader_data;
}


typedef struct {
	GthFileData *file_data;
	int          requested_size;
} LoadData;


static LoadData *
load_data_new (GthFileData *file_data,
	       int          requested_size)
{
	LoadData *load_data;

	load_data = g_new0 (LoadData, 1);
	load_data->file_data = g_object_ref (file_data);
	load_data->requested_size = requested_size;

	return load_data;
}


static void
load_data_unref (LoadData *load_data)
{
	g_object_unref (load_data->file_data);
	g_free (load_data);
}


typedef struct {
	GdkPixbufAnimation *animation;
	int                 original_width;
	int                 original_height;
} LoadResult;


static void
load_result_unref (LoadResult *load_result)
{
	if (load_result->animation != NULL)
		g_object_unref (load_result->animation);
	g_free (load_result);
}


static void
load_pixbuf_thread (GSimpleAsyncResult *result,
		    GObject            *object,
		    GCancellable       *cancellable)
{
	GthImageLoader     *self = GTH_IMAGE_LOADER (object);
	LoadData           *load_data;
	GdkPixbufAnimation *animation;
	int                 original_width;
	int                 original_height;
	GError             *error = NULL;
	LoadResult         *load_result;

	load_data = g_simple_async_result_get_op_res_gpointer (result);
	animation = NULL;
	original_width = -1;
	original_height = -1;

	if (self->priv->loader_func != NULL) {
		animation = (*self->priv->loader_func) (load_data->file_data,
						        load_data->requested_size,
						        &original_width,
						        &original_height,
						        self->priv->loader_data,
						        cancellable,
						        &error);
	}
	else  {
		PixbufLoader loader_func;

		loader_func = gth_main_get_pixbuf_loader (gth_file_data_get_mime_type (load_data->file_data));
		if (loader_func != NULL)
			animation = loader_func (load_data->file_data,
				        	 load_data->requested_size,
				        	 &original_width,
				        	 &original_height,
				        	 NULL,
				        	 cancellable,
				        	 &error);
		else
			error = g_error_new_literal (G_IO_ERROR, G_IO_ERROR_NOT_SUPPORTED, _("No suitable loader available for this file type"));
	}

	load_result = g_new0 (LoadResult, 1);
	load_result->animation = animation;
	load_result->original_width = original_width;
	load_result->original_height = original_height;

	if (error != NULL) {
		g_simple_async_result_set_from_error (result, error);
		g_error_free (error);
	}
	else
		g_simple_async_result_set_op_res_gpointer (result, load_result, (GDestroyNotify) load_result_unref);
}


void
gth_image_loader_load (GthImageLoader      *loader,
		       GthFileData         *file_data,
		       int                  requested_size,
		       int                  io_priority,
		       GCancellable        *cancellable,
		       GAsyncReadyCallback  callback,
		       gpointer             user_data)
{
	GSimpleAsyncResult *result;

	result = g_simple_async_result_new (G_OBJECT (loader), callback, user_data, gth_image_loader_load);
	g_simple_async_result_set_op_res_gpointer (result,
						   load_data_new (file_data, requested_size),
						   (GDestroyNotify) load_data_unref);
	g_simple_async_result_run_in_thread (result,
					     load_pixbuf_thread,
					     io_priority,
					     cancellable);
	g_object_unref (result);
}


gboolean
gth_image_loader_load_animation_finish (GthImageLoader      *loader,
					GAsyncResult        *result,
					GdkPixbufAnimation **animation,
					int                 *original_width,
					int                 *original_height,
					GError             **error)
{
	  GSimpleAsyncResult *simple;
	  LoadResult         *load_result;

	  g_return_val_if_fail (g_simple_async_result_is_valid (result, G_OBJECT (loader), gth_image_loader_load), FALSE);

	  simple = G_SIMPLE_ASYNC_RESULT (result);

	  if (g_simple_async_result_propagate_error (simple, error))
		  return FALSE;

	  load_result = g_simple_async_result_get_op_res_gpointer (simple);
	  if (animation != NULL)
		  *animation = g_object_ref (load_result->animation);
	  if (original_width != NULL)
	  	  *original_width = load_result->original_width;
	  if (original_height != NULL)
	  	  *original_height = load_result->original_height;

	  return TRUE;
}


gboolean
gth_image_loader_load_image_finish (GthImageLoader  *loader,
				    GAsyncResult    *res,
				    GdkPixbuf      **pixbuf,
				    int             *original_width,
				    int             *original_height,
				    GError         **error)
{
	GdkPixbufAnimation *animation;
	GdkPixbuf          *static_image;

	if (! gth_image_loader_load_animation_finish (loader,
						      res,
						      &animation,
						      original_width,
						      original_height,
						      error))
	{
		return FALSE;
	}

	static_image = gdk_pixbuf_animation_get_static_image (animation);
	if (static_image != NULL) {
		*pixbuf = gdk_pixbuf_copy (static_image);
	}
	else {
		*pixbuf = NULL;
		if (error != NULL)
			*error = g_error_new_literal (GDK_PIXBUF_ERROR, GDK_PIXBUF_ERROR_FAILED, "No image");
	}

	g_object_unref (animation);

	return TRUE;
}
