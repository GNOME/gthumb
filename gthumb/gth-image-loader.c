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
#include "glib-utils.h"
#include "gth-error.h"
#include "gth-image-loader.h"
#include "gth-main.h"

/*
#include "file-utils.h"
#include "glib-utils.h"
#include "gconf-utils.h"
#include "gth-exif-utils.h"
#include "pixbuf-utils.h"
#include "preferences.h"
*/

#define REFRESH_RATE 5
#define GTH_IMAGE_LOADER_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), GTH_TYPE_IMAGE_LOADER, GthImageLoaderPrivate))
G_LOCK_DEFINE_STATIC (pixbuf_loader_lock);


struct _GthImageLoaderPrivate {
	GthFileData        *file;
	int                 requested_size;
	GdkPixbuf          *pixbuf;
	GdkPixbufAnimation *animation;
	int                 original_width;
	int                 original_height;

	gboolean            as_animation; /* Whether to load the image in a
					   * GdkPixbufAnimation structure. */
	gboolean            ready;
	GError             *error;
	gboolean            loader_ready;
	gboolean            cancelled;
	gboolean            loading;
	gboolean            emit_signal;

	DataFunc            done_func;
	gpointer            done_func_data;

	guint               check_id;
	guint               idle_id;

	GThread            *thread;

	GMutex             *data_mutex;

	gboolean            exit_thread;
	GMutex             *exit_thread_mutex;

	gboolean            start_loading;
	GMutex             *start_loading_mutex;
	GCond              *start_loading_cond;

	PixbufLoader        loader;
	gpointer            loader_data;
	GCancellable       *cancellable;
};


enum {
	READY,
	LAST_SIGNAL
};


static gpointer parent_class = NULL;
static guint gth_image_loader_signals[LAST_SIGNAL] = { 0 };


static void
gth_image_loader_finalize__step2 (GObject *object)
{
	GthImageLoader *self;

	self = GTH_IMAGE_LOADER (object);

	g_mutex_lock (self->priv->data_mutex);
	if (self->priv->file != NULL) {
		g_object_unref (self->priv->file);
		self->priv->file = NULL;
	}
	if (self->priv->pixbuf != NULL) {
		g_object_unref (G_OBJECT (self->priv->pixbuf));
		self->priv->pixbuf = NULL;
	}

	if (self->priv->animation != NULL) {
		g_object_unref (G_OBJECT (self->priv->animation));
		self->priv->animation = NULL;
	}
	g_mutex_unlock (self->priv->data_mutex);

	g_mutex_lock (self->priv->exit_thread_mutex);
	self->priv->exit_thread = TRUE;
	g_mutex_unlock (self->priv->exit_thread_mutex);

	g_mutex_lock (self->priv->start_loading_mutex);
	self->priv->start_loading = TRUE;
	g_cond_signal (self->priv->start_loading_cond);
	g_mutex_unlock (self->priv->start_loading_mutex);

	g_thread_join (self->priv->thread);

	g_cond_free  (self->priv->start_loading_cond);
	g_mutex_free (self->priv->data_mutex);
	g_mutex_free (self->priv->start_loading_mutex);
	g_mutex_free (self->priv->exit_thread_mutex);

	g_object_unref (self->priv->cancellable);

	/* Chain up */
	G_OBJECT_CLASS (parent_class)->finalize (object);
}


static void _gth_image_loader_stop (GthImageLoader *self,
				    DataFunc        done_func,
				    gpointer        done_func_data,
				    gboolean        emit_sig,
				    gboolean        use_idle_cb);


static void
gth_image_loader_finalize (GObject *object)
{
	GthImageLoader *self;

	g_return_if_fail (object != NULL);
	g_return_if_fail (GTH_IS_IMAGE_LOADER (object));

	self = GTH_IMAGE_LOADER (object);

	if (self->priv->idle_id != 0) {
		g_source_remove (self->priv->idle_id);
		self->priv->idle_id = 0;
	}

	if (self->priv->check_id != 0) {
		g_source_remove (self->priv->check_id);
		self->priv->check_id = 0;
	}

	_gth_image_loader_stop (self,
				(DataFunc) gth_image_loader_finalize__step2,
				object,
				FALSE,
				FALSE);
}


static void
gth_image_loader_class_init (GthImageLoaderClass *class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (class);

	parent_class = g_type_class_peek_parent (class);
	g_type_class_add_private (class, sizeof (GthImageLoaderPrivate));

	object_class->finalize = gth_image_loader_finalize;

	gth_image_loader_signals[READY] =
		g_signal_new ("ready",
			      G_TYPE_FROM_CLASS (class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (GthImageLoaderClass, ready),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__POINTER,
			      G_TYPE_NONE,
			      1,
			      G_TYPE_POINTER);
}


static void *
load_image_thread (void *thread_data)
{
	GthImageLoader     *self = thread_data;
	GdkPixbufAnimation *animation;
	GError             *error;

	for (;;) {
		GthFileData *file;
		gboolean     exit_thread;
		int          requested_size;
		int          original_width;
		int          original_height;

		g_mutex_lock (self->priv->start_loading_mutex);
		while (! self->priv->start_loading)
			g_cond_wait (self->priv->start_loading_cond, self->priv->start_loading_mutex);
		self->priv->start_loading = FALSE;
		g_mutex_unlock (self->priv->start_loading_mutex);

		g_mutex_lock (self->priv->exit_thread_mutex);
		exit_thread = self->priv->exit_thread;
		requested_size = self->priv->requested_size;
		g_mutex_unlock (self->priv->exit_thread_mutex);

		if (exit_thread)
			break;

		file = gth_image_loader_get_file (self);

		G_LOCK (pixbuf_loader_lock);

		original_width = -1;
		original_height = -1;
		animation = NULL;
		if (file != NULL) {
			error = NULL;
			if (self->priv->loader != NULL) {
				animation = (*self->priv->loader) (file,
								      requested_size,
								      &original_width,
								      &original_height,
								      self->priv->loader_data,
								      self->priv->cancellable,
								      &error);
			}
			else  {
				PixbufLoader loader;

				loader = gth_main_get_pixbuf_loader (gth_file_data_get_mime_type (file));
				if (loader != NULL)
					animation = loader (file,
							    requested_size,
							    &original_width,
							    &original_height,
							    NULL,
							    self->priv->cancellable,
							    &error);
				else
					error = g_error_new_literal (G_IO_ERROR, G_IO_ERROR_NOT_SUPPORTED, _("No suitable loader available for this file type"));
			}
		}

		G_UNLOCK (pixbuf_loader_lock);

		g_mutex_lock (self->priv->data_mutex);

		self->priv->loader_ready = TRUE;

		if ((animation == NULL) || (error != NULL)) {
			self->priv->error = error;
			self->priv->ready = FALSE;
		}
		else if ((self->priv->file != NULL)
			 && _g_file_equal_uris (file->file, self->priv->file->file)
			 && (requested_size == self->priv->requested_size))
		{
			_g_object_unref (self->priv->animation);
			self->priv->animation = g_object_ref (animation);
			self->priv->original_width = original_width;
			self->priv->original_height = original_height;

			self->priv->error = NULL;
			self->priv->ready = TRUE;
		}

		_g_object_unref (animation);

		g_mutex_unlock (self->priv->data_mutex);

		g_object_unref (file);
	}

	return NULL;
}


static void
gth_image_loader_init (GthImageLoader *self)
{
	self->priv = GTH_IMAGE_LOADER_GET_PRIVATE (self);

	self->priv->pixbuf = NULL;
	self->priv->animation = NULL;
	self->priv->file = NULL;
	self->priv->requested_size = -1;

	self->priv->ready = FALSE;
	self->priv->error = NULL;
	self->priv->loader_ready = FALSE;
	self->priv->cancelled = FALSE;

	self->priv->done_func = NULL;
	self->priv->done_func_data = NULL;

	self->priv->check_id = 0;
	self->priv->idle_id = 0;

	self->priv->data_mutex = g_mutex_new ();

	self->priv->exit_thread = FALSE;
	self->priv->exit_thread_mutex = g_mutex_new ();

	self->priv->start_loading = FALSE;
	self->priv->start_loading_mutex = g_mutex_new ();
	self->priv->start_loading_cond = g_cond_new ();

	self->priv->loader = NULL;
	self->priv->loader_data = NULL;

	self->priv->cancellable = g_cancellable_new ();

	/*self->priv->thread = g_thread_create (load_image_thread, self, TRUE, NULL);*/

	/* The g_thread_create function assigns a very large default stacksize for each
	   thread (10 MB on FC6), which is probably excessive. 16k seems to be
	   sufficient. To be conversative, we'll try 32k. Use g_thread_create_full to
	   manually specify a small stack size. See Bug 310749 - Memory usage.
	   This reduces the virtual memory requirements, and the "writeable/private"
	   figure reported by "pmap -d". */

	/* Update: 32k caused crashes with svg images. Boosting to 512k. Bug 410827. */

	self->priv->thread = g_thread_create_full (load_image_thread,
						      self,
						      (512 * 1024),
						      TRUE,
						      TRUE,
						      G_THREAD_PRIORITY_NORMAL,
						      NULL);
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
gth_image_loader_new (gboolean as_animation)
{
	GthImageLoader *self;

	self = g_object_new (GTH_TYPE_IMAGE_LOADER, NULL);
	g_mutex_lock (self->priv->data_mutex);
	self->priv->as_animation = as_animation;
	g_mutex_unlock (self->priv->data_mutex);

	return self;
}


void
gth_image_loader_set_loader (GthImageLoader *self,
			     PixbufLoader    loader,
			     gpointer        loader_data)
{
	g_return_if_fail (self != NULL);

	g_mutex_lock (self->priv->data_mutex);
	self->priv->loader = loader;
	self->priv->loader_data = loader_data;
	g_mutex_unlock (self->priv->data_mutex);
}


void
gth_image_loader_set_file_data (GthImageLoader *self,
				GthFileData    *file)
{
	g_mutex_lock (self->priv->data_mutex);
	_g_object_unref (self->priv->file);
	self->priv->file = NULL;
	if (file != NULL)
		self->priv->file = gth_file_data_dup (file);
	g_mutex_unlock (self->priv->data_mutex);
}


void
gth_image_loader_set_file (GthImageLoader *self,
			   GFile          *file,
			   const char     *mime_type)
{
	GthFileData *file_data;

	file_data = gth_file_data_new (file, NULL);
	if (mime_type != NULL)
		gth_file_data_set_mime_type (file_data, mime_type);
	else
		gth_file_data_update_mime_type (file_data, TRUE); /* FIXME: always fast mime type is not good */

	gth_image_loader_set_file_data (self, file_data);

	g_object_unref (file_data);
}


GthFileData *
gth_image_loader_get_file (GthImageLoader *self)
{
	GthFileData *file = NULL;

	g_mutex_lock (self->priv->data_mutex);
	if (self->priv->file != NULL)
		file = gth_file_data_dup (self->priv->file);
	g_mutex_unlock (self->priv->data_mutex);

	return file;
}


void
gth_image_loader_set_pixbuf (GthImageLoader *self,
			     GdkPixbuf      *pixbuf)
{
	g_return_if_fail (self != NULL);

	g_mutex_lock (self->priv->data_mutex);

	if (self->priv->animation != NULL) {
		g_object_unref (self->priv->animation);
		self->priv->animation = NULL;
	}

	if (self->priv->pixbuf != NULL) {
		g_object_unref (self->priv->pixbuf);
		self->priv->pixbuf = NULL;
	}

	self->priv->pixbuf = pixbuf;
	if (pixbuf != NULL)
		g_object_ref (pixbuf);

	g_mutex_unlock (self->priv->data_mutex);
}


static void
_gth_image_loader_sync_pixbuf (GthImageLoader *self)
{
	GdkPixbuf *pixbuf;

	g_mutex_lock (self->priv->data_mutex);

	if (self->priv->animation == NULL) {
		if (self->priv->pixbuf != NULL)
			g_object_unref (self->priv->pixbuf);
		self->priv->pixbuf = NULL;
		g_mutex_unlock (self->priv->data_mutex);
		return;
	}

	pixbuf = gdk_pixbuf_animation_get_static_image (self->priv->animation);
	g_object_ref (pixbuf);

	if (self->priv->pixbuf == pixbuf) {
		g_object_unref (pixbuf);
		g_mutex_unlock (self->priv->data_mutex);
		return;
	}

	if (self->priv->pixbuf != NULL) {
		g_object_unref (self->priv->pixbuf);
		self->priv->pixbuf = NULL;
	}

	if (pixbuf != NULL) {
		g_object_ref (pixbuf);
		self->priv->pixbuf = pixbuf;
	}

	g_object_unref (pixbuf);

	g_mutex_unlock (self->priv->data_mutex);
}


static void
_gth_image_loader_cancelled (GthImageLoader *self)
{
	g_mutex_lock (self->priv->data_mutex);
	self->priv->error = g_error_new_literal (G_IO_ERROR, G_IO_ERROR_CANCELLED, "");
	g_mutex_unlock (self->priv->data_mutex);

	_gth_image_loader_stop (self,
				self->priv->done_func,
				self->priv->done_func_data,
				TRUE,
				TRUE);
}


static void
_gth_image_loader_ready (GthImageLoader *self)
{
	g_mutex_lock (self->priv->data_mutex);
	self->priv->error = NULL;
	g_mutex_unlock (self->priv->data_mutex);

	_gth_image_loader_stop (self, NULL, NULL, TRUE, TRUE);
}


static void
_gth_image_loader_error (GthImageLoader *self,
			 GError         *error)
{
	g_mutex_lock (self->priv->data_mutex);

	if (self->priv->error != error) {
		if (self->priv->error != NULL) {
			g_error_free (self->priv->error);
			self->priv->error = NULL;
		}
		self->priv->error = error;
	}

	g_mutex_unlock (self->priv->data_mutex);

	_gth_image_loader_stop (self, NULL, NULL, TRUE, TRUE);
}


static gboolean
check_thread (gpointer data)
{
	GthImageLoader *self = data;
	gboolean        ready, loader_done;
	GError         *error = NULL;

	/* Remove check. */

	g_source_remove (self->priv->check_id);
	self->priv->check_id = 0;

	/**/

	g_mutex_lock (self->priv->data_mutex);

	ready = self->priv->ready;
	if (self->priv->error != NULL)
		error = g_error_copy (self->priv->error);
	loader_done = self->priv->loader_ready;

	g_mutex_unlock (self->priv->data_mutex);

	/**/

	if (loader_done && self->priv->cancelled)
		_gth_image_loader_cancelled (self);

	else if (loader_done && ready)
		_gth_image_loader_ready (self);

	else if (loader_done && (error != NULL))
		_gth_image_loader_error (self, error);

	else	/* Add the check again. */
		self->priv->check_id = g_timeout_add (REFRESH_RATE, check_thread, self);

	return FALSE;
}


static void
_gth_image_loader_load__step2 (GthImageLoader *self)
{
	g_mutex_lock (self->priv->data_mutex);

	self->priv->ready = FALSE;
	self->priv->error = NULL;
	self->priv->loader_ready = FALSE;
	self->priv->loading = TRUE;
	if (self->priv->pixbuf != NULL) {
		g_object_unref (self->priv->pixbuf);
		self->priv->pixbuf = NULL;
	}
	if (self->priv->animation != NULL) {
		g_object_unref (self->priv->animation);
		self->priv->animation = NULL;
	}
	g_cancellable_reset (self->priv->cancellable);

	g_mutex_unlock (self->priv->data_mutex);

	/**/

	g_mutex_lock (self->priv->start_loading_mutex);

	self->priv->start_loading = TRUE;
	g_cond_signal (self->priv->start_loading_cond);

	g_mutex_unlock (self->priv->start_loading_mutex);

	/**/

	self->priv->check_id = g_timeout_add (REFRESH_RATE,
						 check_thread,
						 self);
}


void
gth_image_loader_load (GthImageLoader *self)
{
	gth_image_loader_load_at_size (self, -1);
}


void
gth_image_loader_load_at_size (GthImageLoader *self,
			       int             requested_size)
{
	gboolean no_file = FALSE;

	g_return_if_fail (self != NULL);

	g_mutex_lock (self->priv->data_mutex);
	if (self->priv->file == NULL)
		no_file = TRUE;
	self->priv->requested_size = requested_size;
	g_mutex_unlock (self->priv->data_mutex);

	if (no_file)
		return;

	_gth_image_loader_stop (self,
				(DataFunc) _gth_image_loader_load__step2,
				self,
				FALSE,
				TRUE);
}



/* -- gth_image_loader_stop -- */


static void
_gth_image_loader_stop__step2 (GthImageLoader *self,
			       gboolean        use_idle_cb)
{
	DataFunc  done_func;
	GError   *error;

	g_mutex_lock (self->priv->data_mutex);
	error = self->priv->error;
	self->priv->error = NULL;
	self->priv->ready = TRUE;
	g_mutex_unlock (self->priv->data_mutex);

	if ((error == NULL) && ! self->priv->cancelled && self->priv->loading)
		_gth_image_loader_sync_pixbuf (self);
	self->priv->loading = FALSE;

	done_func = self->priv->done_func;
	self->priv->done_func = NULL;
	if (done_func != NULL) {
		IdleCall *call;

		call = idle_call_new (done_func, self->priv->done_func_data);
		if (self->priv->idle_id != 0)
			g_source_remove (self->priv->idle_id);
		self->priv->idle_id = idle_call_exec (call, use_idle_cb);
	}

	if (! self->priv->emit_signal || self->priv->cancelled) {
		self->priv->cancelled = FALSE;
		return;
	}
	self->priv->cancelled = FALSE;

	g_signal_emit (G_OBJECT (self),
		       gth_image_loader_signals[READY],
		       0,
		       error);
}


static void
_gth_image_loader_stop (GthImageLoader *self,
			DataFunc        done_func,
			gpointer        done_func_data,
			gboolean        emit_signal,
			gboolean        use_idle_cb)
{
	self->priv->done_func = done_func;
	self->priv->done_func_data = done_func_data;
	self->priv->emit_signal = emit_signal;

	_gth_image_loader_stop__step2 (self, use_idle_cb);
}


void
gth_image_loader_cancel (GthImageLoader *self,
			 DataFunc        done_func,
			 gpointer        done_func_data)
{
	g_mutex_lock (self->priv->data_mutex);
	self->priv->error = FALSE;
	g_mutex_unlock (self->priv->data_mutex);

	if (self->priv->loading) {
		self->priv->emit_signal = TRUE;
		self->priv->cancelled = TRUE;
		self->priv->done_func = done_func;
		self->priv->done_func_data = done_func_data;
		g_cancellable_cancel (self->priv->cancellable);
	}
	else
		_gth_image_loader_stop (self,
					done_func,
					done_func_data,
					FALSE,
					TRUE);
}


void
gth_image_loader_cancel_with_error (GthImageLoader *self,
				    DataFunc        done_func,
				    gpointer        done_func_data)
{
	g_mutex_lock (self->priv->data_mutex);
	self->priv->error = g_error_new_literal (GTH_ERROR, 0, "cancelled");
	g_mutex_unlock (self->priv->data_mutex);

	_gth_image_loader_stop (self, done_func, done_func_data, TRUE, TRUE);
}


GdkPixbuf *
gth_image_loader_get_pixbuf (GthImageLoader *self)
{
	return self->priv->pixbuf;
}


GdkPixbufAnimation *
gth_image_loader_get_animation (GthImageLoader *self)
{
	GdkPixbufAnimation *animation;

	g_mutex_lock (self->priv->data_mutex);
	animation = self->priv->animation;
	if (animation != NULL)
		g_object_ref (animation);
	g_mutex_unlock (self->priv->data_mutex);

	return animation;
}


void
gth_image_loader_get_original_size (GthImageLoader *self,
				    int            *width,
				    int            *height)
{
	g_mutex_lock (self->priv->data_mutex);
	if (width != NULL)
		*width = self->priv->original_width;
	if (height != NULL)
		*height = self->priv->original_height;
	g_mutex_unlock (self->priv->data_mutex);
}
