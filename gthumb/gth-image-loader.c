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
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Street #330, Boston, MA 02111-1307, USA.
 */

#include <config.h>
#include <glib/gi18n.h>
#define GDK_PIXBUF_ENABLE_BACKEND
#include <gdk-pixbuf/gdk-pixbuf-animation.h>
#include <gtk/gtk.h>
#include "gth-file-data.h"
#include "glib-utils.h"
#include "gth-image-loader.h"
#include "gth-main.h"
#include "gthumb-error.h"

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
	GdkPixbuf          *pixbuf;
	GdkPixbufAnimation *animation;

	gboolean            as_animation; /* Whether to load the image in a
					   * GdkPixbufAnimation structure. */
	gboolean            ready;
	GError             *error;
	gboolean            loader_ready;
	gboolean            cancelled;
	gboolean            loading;
	gboolean            emit_signal;

	DoneFunc            done_func;
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

	LoaderFunc          loader;
	gpointer            loader_data;
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
	GthImageLoader *iloader;

	iloader = GTH_IMAGE_LOADER (object);

	g_mutex_lock (iloader->priv->data_mutex);
	if (iloader->priv->file != NULL) {
		g_object_unref (iloader->priv->file);
		iloader->priv->file = NULL;
	}
	if (iloader->priv->pixbuf != NULL) {
		g_object_unref (G_OBJECT (iloader->priv->pixbuf));
		iloader->priv->pixbuf = NULL;
	}

	if (iloader->priv->animation != NULL) {
		g_object_unref (G_OBJECT (iloader->priv->animation));
		iloader->priv->animation = NULL;
	}
	g_mutex_unlock (iloader->priv->data_mutex);

	g_mutex_lock (iloader->priv->exit_thread_mutex);
	iloader->priv->exit_thread = TRUE;
	g_mutex_unlock (iloader->priv->exit_thread_mutex);

	g_mutex_lock (iloader->priv->start_loading_mutex);
	iloader->priv->start_loading = TRUE;
	g_cond_signal (iloader->priv->start_loading_cond);
	g_mutex_unlock (iloader->priv->start_loading_mutex);

	g_thread_join (iloader->priv->thread);

	g_cond_free  (iloader->priv->start_loading_cond);
	g_mutex_free (iloader->priv->data_mutex);
	g_mutex_free (iloader->priv->start_loading_mutex);
	g_mutex_free (iloader->priv->exit_thread_mutex);

	/* Chain up */
	G_OBJECT_CLASS (parent_class)->finalize (object);
}


static void _gth_image_loader_stop (GthImageLoader *iloader,
				    DoneFunc        done_func,
				    gpointer        done_func_data,
				    gboolean        emit_sig,
				    gboolean        use_idle_cb);


static void
gth_image_loader_finalize (GObject *object)
{
	GthImageLoader *iloader;

	g_return_if_fail (object != NULL);
	g_return_if_fail (GTH_IS_IMAGE_LOADER (object));

	iloader = GTH_IMAGE_LOADER (object);

	if (iloader->priv->idle_id != 0) {
		g_source_remove (iloader->priv->idle_id);
		iloader->priv->idle_id = 0;
	}

	if (iloader->priv->check_id != 0) {
		g_source_remove (iloader->priv->check_id);
		iloader->priv->check_id = 0;
	}

	_gth_image_loader_stop (iloader,
				(DoneFunc) gth_image_loader_finalize__step2,
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
	GthImageLoader     *iloader = thread_data;
	GdkPixbufAnimation *animation;
	GError             *error;

	for (;;) {
		GthFileData *file;
		gboolean     exit_thread;

		g_mutex_lock (iloader->priv->start_loading_mutex);
		while (! iloader->priv->start_loading)
			g_cond_wait (iloader->priv->start_loading_cond, iloader->priv->start_loading_mutex);
		iloader->priv->start_loading = FALSE;
		g_mutex_unlock (iloader->priv->start_loading_mutex);

		g_mutex_lock (iloader->priv->exit_thread_mutex);
		exit_thread = iloader->priv->exit_thread;
		g_mutex_unlock (iloader->priv->exit_thread_mutex);

		if (exit_thread)
			break;

		file = gth_image_loader_get_file (iloader);

		G_LOCK (pixbuf_loader_lock);

		animation = NULL;
		if (file != NULL) {
			error = NULL;
			if (iloader->priv->loader != NULL) {
				animation = (*iloader->priv->loader) (file, &error, iloader->priv->loader_data);
			}
			else  {
				FileLoader loader;

				loader = gth_main_get_file_loader (gth_file_data_get_mime_type (file));
				if (loader != NULL)
					animation = loader (file, &error, -1, -1);
				else
					error = g_error_new_literal (G_IO_ERROR, G_IO_ERROR_NOT_SUPPORTED, _("No suitable loader available for this file type"));
			}
		}

		G_UNLOCK (pixbuf_loader_lock);

		g_mutex_lock (iloader->priv->data_mutex);

		iloader->priv->loader_ready = TRUE;
		if (iloader->priv->animation != NULL)
			g_object_unref (iloader->priv->animation);
		iloader->priv->animation = animation;
		if ((animation == NULL) || (error != NULL)) {
			iloader->priv->error = error;
			iloader->priv->ready = FALSE;
		}
		else {
			iloader->priv->error = NULL;
			iloader->priv->ready = TRUE;
		}

		g_mutex_unlock (iloader->priv->data_mutex);

		g_object_unref (file);
	}

	return NULL;
}


static void
gth_image_loader_init (GthImageLoader *iloader)
{
	iloader->priv = GTH_IMAGE_LOADER_GET_PRIVATE (iloader);

	iloader->priv->pixbuf = NULL;
	iloader->priv->animation = NULL;
	iloader->priv->file = NULL;

	iloader->priv->ready = FALSE;
	iloader->priv->error = NULL;
	iloader->priv->loader_ready = FALSE;
	iloader->priv->cancelled = FALSE;

	iloader->priv->done_func = NULL;
	iloader->priv->done_func_data = NULL;

	iloader->priv->check_id = 0;
	iloader->priv->idle_id = 0;

	iloader->priv->data_mutex = g_mutex_new ();

	iloader->priv->exit_thread = FALSE;
	iloader->priv->exit_thread_mutex = g_mutex_new ();

	iloader->priv->start_loading = FALSE;
	iloader->priv->start_loading_mutex = g_mutex_new ();
	iloader->priv->start_loading_cond = g_cond_new ();

	iloader->priv->loader = NULL;
	iloader->priv->loader_data = NULL;

	/*iloader->priv->thread = g_thread_create (load_image_thread, iloader, TRUE, NULL);*/

	/* The g_thread_create function assigns a very large default stacksize for each
	   thread (10 MB on FC6), which is probably excessive. 16k seems to be
	   sufficient. To be conversative, we'll try 32k. Use g_thread_create_full to
	   manually specify a small stack size. See Bug 310749 - Memory usage.
	   This reduces the virtual memory requirements, and the "writeable/private"
	   figure reported by "pmap -d". */

	/* Update: 32k caused crashes with svg images. Boosting to 512k. Bug 410827. */

	iloader->priv->thread = g_thread_create_full (load_image_thread,
						      iloader,
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
	GthImageLoader *iloader;

	iloader = g_object_new (GTH_TYPE_IMAGE_LOADER, NULL);
	g_mutex_lock (iloader->priv->data_mutex);
	iloader->priv->as_animation = as_animation;
	g_mutex_unlock (iloader->priv->data_mutex);

	return iloader;
}


void
gth_image_loader_set_loader (GthImageLoader *iloader,
			     LoaderFunc      loader,
			     gpointer        loader_data)
{
	g_return_if_fail (iloader != NULL);

	g_mutex_lock (iloader->priv->data_mutex);
	iloader->priv->loader = loader;
	iloader->priv->loader_data = loader_data;
	g_mutex_unlock (iloader->priv->data_mutex);
}


void
gth_image_loader_set_file_data (GthImageLoader *iloader,
				GthFileData    *file)
{
	g_mutex_lock (iloader->priv->data_mutex);
	_g_object_unref (iloader->priv->file);
	iloader->priv->file = NULL;
	if (file != NULL)
		iloader->priv->file = gth_file_data_dup (file);
	g_mutex_unlock (iloader->priv->data_mutex);
}


void
gth_image_loader_set_file (GthImageLoader *iloader,
			   GFile          *file,
			   const char     *mime_type)
{
	GthFileData *file_data;

	file_data = gth_file_data_new (file, NULL);
	if (mime_type != NULL)
		gth_file_data_set_mime_type (file_data, mime_type);
	else
		gth_file_data_update_mime_type (file_data, TRUE); /* FIXME: always fast mime type is not good */

	gth_image_loader_set_file_data (iloader, file_data);

	g_object_unref (file_data);
}


GthFileData *
gth_image_loader_get_file (GthImageLoader *iloader)
{
	GthFileData *file = NULL;

	g_mutex_lock (iloader->priv->data_mutex);
	if (iloader->priv->file != NULL)
		file = gth_file_data_dup (iloader->priv->file);
	g_mutex_unlock (iloader->priv->data_mutex);

	return file;
}


void
gth_image_loader_set_pixbuf (GthImageLoader *iloader,
			     GdkPixbuf      *pixbuf)
{
	g_return_if_fail (iloader != NULL);

	g_mutex_lock (iloader->priv->data_mutex);

	if (iloader->priv->animation != NULL) {
		g_object_unref (iloader->priv->animation);
		iloader->priv->animation = NULL;
	}

	if (iloader->priv->pixbuf != NULL) {
		g_object_unref (iloader->priv->pixbuf);
		iloader->priv->pixbuf = NULL;
	}

	iloader->priv->pixbuf = pixbuf;
	if (pixbuf != NULL)
		g_object_ref (pixbuf);

	g_mutex_unlock (iloader->priv->data_mutex);
}


static void
_gth_image_loader_sync_pixbuf (GthImageLoader *iloader)
{
	GdkPixbuf *pixbuf;

	g_mutex_lock (iloader->priv->data_mutex);

	if (iloader->priv->animation == NULL) {
		if (iloader->priv->pixbuf != NULL)
			g_object_unref (iloader->priv->pixbuf);
		iloader->priv->pixbuf = NULL;
		g_mutex_unlock (iloader->priv->data_mutex);
		return;
	}

	pixbuf = gdk_pixbuf_animation_get_static_image (iloader->priv->animation);
	g_object_ref (pixbuf);

	if (iloader->priv->pixbuf == pixbuf) {
		g_object_unref (pixbuf);
		g_mutex_unlock (iloader->priv->data_mutex);
		return;
	}

	if (iloader->priv->pixbuf != NULL) {
		g_object_unref (iloader->priv->pixbuf);
		iloader->priv->pixbuf = NULL;
	}

	if (pixbuf != NULL) {
		g_object_ref (pixbuf);
		iloader->priv->pixbuf = pixbuf;
	}

	g_object_unref (pixbuf);

	g_mutex_unlock (iloader->priv->data_mutex);
}


static void
_gth_image_loader_sync_from_loader (GthImageLoader  *iloader,
				    GdkPixbufLoader *pbloader)
{
	GdkPixbuf *pixbuf;

	g_return_if_fail (iloader != NULL);

	g_mutex_lock (iloader->priv->data_mutex);

	if (iloader->priv->as_animation) {
		if (iloader->priv->animation != NULL)
			g_object_unref (iloader->priv->animation);
		iloader->priv->animation = gdk_pixbuf_loader_get_animation (pbloader);

		if ((iloader->priv->animation != NULL)
		    && ! gdk_pixbuf_animation_is_static_image (iloader->priv->animation))
		{
			g_object_ref (iloader->priv->animation);
			g_mutex_unlock (iloader->priv->data_mutex);
			return;
		}
		else
			iloader->priv->animation = NULL;
	}

	pixbuf = gdk_pixbuf_loader_get_pixbuf (pbloader);
	g_object_ref (pixbuf);

	if (iloader->priv->pixbuf == pixbuf) {
		g_object_unref (iloader->priv->pixbuf);
		g_mutex_unlock (iloader->priv->data_mutex);
		return;
	}

	if (iloader->priv->pixbuf != NULL) {
		g_object_unref (iloader->priv->pixbuf);
		iloader->priv->pixbuf = NULL;
	}

	if (pixbuf != NULL) {
		g_object_ref (pixbuf);
		iloader->priv->pixbuf = pixbuf;
		/*priv->pixbuf = gdk_pixbuf_copy (pixbuf);*/
	}

	g_object_unref (pixbuf);

	g_mutex_unlock (iloader->priv->data_mutex);
}


static void
_gth_image_loader_cancelled (GthImageLoader *iloader)
{
	g_mutex_lock (iloader->priv->data_mutex);
	iloader->priv->error = g_error_new_literal (G_IO_ERROR, G_IO_ERROR_CANCELLED, "");
	g_mutex_unlock (iloader->priv->data_mutex);

	_gth_image_loader_stop (iloader,
				iloader->priv->done_func,
				iloader->priv->done_func_data,
				TRUE,
				TRUE);
}


static void
_gth_image_loader_ready (GthImageLoader *iloader)
{
	g_mutex_lock (iloader->priv->data_mutex);
	iloader->priv->error = NULL;
	g_mutex_unlock (iloader->priv->data_mutex);

	_gth_image_loader_stop (iloader, NULL, NULL, TRUE, TRUE);
}


static void
_gth_image_loader_error (GthImageLoader *iloader,
			 GError         *error)
{
	g_mutex_lock (iloader->priv->data_mutex);

	if (iloader->priv->error != error) {
		if (iloader->priv->error != NULL) {
			g_error_free (iloader->priv->error);
			iloader->priv->error = NULL;
		}
		iloader->priv->error = error;
	}

	g_mutex_unlock (iloader->priv->data_mutex);

	_gth_image_loader_stop (iloader, NULL, NULL, TRUE, TRUE);
}


static gboolean
check_thread (gpointer data)
{
	GthImageLoader *iloader = data;
	gboolean        ready, loader_done;
	GError         *error = NULL;

	/* Remove check. */

	g_source_remove (iloader->priv->check_id);
	iloader->priv->check_id = 0;

	/**/

	g_mutex_lock (iloader->priv->data_mutex);

	ready = iloader->priv->ready;
	if (iloader->priv->error != NULL)
		error = g_error_copy (iloader->priv->error);
	loader_done = iloader->priv->loader_ready;

	g_mutex_unlock (iloader->priv->data_mutex);

	/**/

	if (loader_done && iloader->priv->cancelled)
		_gth_image_loader_cancelled (iloader);

	else if (loader_done && ready)
		_gth_image_loader_ready (iloader);

	else if (loader_done && (error != NULL))
		_gth_image_loader_error (iloader, error);

	else	/* Add the check again. */
		iloader->priv->check_id = g_timeout_add (REFRESH_RATE,
							 check_thread,
							 iloader);

	return FALSE;
}


static void
_gth_image_loader_load__step2 (GthImageLoader *iloader)
{
	g_mutex_lock (iloader->priv->data_mutex);

	iloader->priv->ready = FALSE;
	iloader->priv->error = NULL;
	iloader->priv->loader_ready = FALSE;
	iloader->priv->loading = TRUE;
	if (iloader->priv->pixbuf != NULL) {
		g_object_unref (iloader->priv->pixbuf);
		iloader->priv->pixbuf = NULL;
	}
	if (iloader->priv->animation != NULL) {
		g_object_unref (iloader->priv->animation);
		iloader->priv->animation = NULL;
	}

	g_mutex_unlock (iloader->priv->data_mutex);

	/**/

	g_mutex_lock (iloader->priv->start_loading_mutex);

	iloader->priv->start_loading = TRUE;
	g_cond_signal (iloader->priv->start_loading_cond);

	g_mutex_unlock (iloader->priv->start_loading_mutex);

	/**/

	iloader->priv->check_id = g_timeout_add (REFRESH_RATE,
						 check_thread,
						 iloader);
}


void
gth_image_loader_load (GthImageLoader *iloader)
{
	gboolean no_file = FALSE;

	g_return_if_fail (iloader != NULL);

	g_mutex_lock (iloader->priv->data_mutex);
	if (iloader->priv->file == NULL)
		no_file = TRUE;
	g_mutex_unlock (iloader->priv->data_mutex);

	if (no_file)
		return;

	_gth_image_loader_stop (iloader,
				(DoneFunc) _gth_image_loader_load__step2,
				iloader,
				FALSE,
				TRUE);
}


/* -- gth_image_loader_stop -- */


static void
_gth_image_loader_stop__step2 (GthImageLoader *iloader,
			       gboolean        use_idle_cb)
{
	DoneFunc  done_func = iloader->priv->done_func;
	GError   *error;

	g_mutex_lock (iloader->priv->data_mutex);
	error = iloader->priv->error;
	iloader->priv->error = NULL;
	iloader->priv->ready = TRUE;
	g_mutex_unlock (iloader->priv->data_mutex);

	if ((error == NULL) && ! iloader->priv->cancelled && iloader->priv->loading)
		_gth_image_loader_sync_pixbuf (iloader);
	iloader->priv->loading = FALSE;

	iloader->priv->done_func = NULL;
	if (done_func != NULL) {
		IdleCall *call;

		call = idle_call_new (done_func, iloader->priv->done_func_data);
		if (iloader->priv->idle_id != 0)
			g_source_remove (iloader->priv->idle_id);
		iloader->priv->idle_id = idle_call_exec (call, use_idle_cb);
	}

	if (! iloader->priv->emit_signal || iloader->priv->cancelled) {
		iloader->priv->cancelled = FALSE;
		return;
	}
	iloader->priv->cancelled = FALSE;

	g_signal_emit (G_OBJECT (iloader),
		       gth_image_loader_signals[READY],
		       0,
		       error);
}


static void
_gth_image_loader_stop (GthImageLoader *iloader,
			DoneFunc        done_func,
			gpointer        done_func_data,
			gboolean        emit_signal,
			gboolean        use_idle_cb)
{
	iloader->priv->done_func = done_func;
	iloader->priv->done_func_data = done_func_data;
	iloader->priv->emit_signal = emit_signal;

	_gth_image_loader_stop__step2 (iloader, use_idle_cb);
}


void
gth_image_loader_cancel (GthImageLoader *iloader,
			  DoneFunc        done_func,
			 gpointer        done_func_data)
{
	g_mutex_lock (iloader->priv->data_mutex);
	iloader->priv->error = FALSE;
	g_mutex_unlock (iloader->priv->data_mutex);

	if (iloader->priv->loading) {
		iloader->priv->emit_signal = TRUE;
		iloader->priv->cancelled = TRUE;
		iloader->priv->done_func = done_func;
		iloader->priv->done_func_data = done_func_data;
	}
	else
		_gth_image_loader_stop (iloader,
					done_func,
					done_func_data,
					FALSE,
					TRUE);
}


void
gth_image_loader_cancel_with_error (GthImageLoader *iloader,
				    DoneFunc        done_func,
				    gpointer        done_func_data)
{
	g_mutex_lock (iloader->priv->data_mutex);
	iloader->priv->error = g_error_new_literal (GTHUMB_ERROR, 0, "cancelled");
	g_mutex_unlock (iloader->priv->data_mutex);

	_gth_image_loader_stop (iloader, done_func, done_func_data, TRUE, TRUE);
}


GdkPixbuf *
gth_image_loader_get_pixbuf (GthImageLoader *iloader)
{
	return iloader->priv->pixbuf;
}


GdkPixbufAnimation *
gth_image_loader_get_animation (GthImageLoader *iloader)
{
	GdkPixbufAnimation *animation;

	g_mutex_lock (iloader->priv->data_mutex);
	animation = iloader->priv->animation;
	if (animation != NULL)
		g_object_ref (animation);
	g_mutex_unlock (iloader->priv->data_mutex);

	return animation;
}


gboolean
gth_image_loader_is_ready (GthImageLoader *iloader)
{
	gboolean is_ready;

	g_mutex_lock (iloader->priv->data_mutex);
	is_ready = iloader->priv->ready && iloader->priv->loader_ready;
	g_mutex_unlock (iloader->priv->data_mutex);

	return is_ready;
}


void
gth_image_loader_load_from_pixbuf_loader (GthImageLoader  *iloader,
					  GdkPixbufLoader *pixbuf_loader)
{
	GError *error = NULL;

	_gth_image_loader_sync_from_loader (iloader, pixbuf_loader);

	g_mutex_lock (iloader->priv->data_mutex);
	if ((iloader->priv->pixbuf == NULL) && (iloader->priv->animation == NULL))
		error = g_error_new_literal (GTHUMB_ERROR, 0, "No image available");
	g_mutex_unlock (iloader->priv->data_mutex);

	g_signal_emit (G_OBJECT (iloader), gth_image_loader_signals[READY], 0, error);
}


void
gth_image_loader_load_from_image_loader (GthImageLoader *to,
					 GthImageLoader *from)
{
	GError *error = NULL;

	g_return_if_fail (to != NULL);
	g_return_if_fail (from != NULL);

	g_mutex_lock (to->priv->data_mutex);
	g_mutex_lock (from->priv->data_mutex);

	if (to->priv->file != NULL) {
		g_object_unref (to->priv->file);
		to->priv->file = NULL;
	}
	if (from->priv->file != NULL)
		to->priv->file = gth_file_data_dup (from->priv->file);

	if (to->priv->pixbuf) {
		g_object_unref (to->priv->pixbuf);
		to->priv->pixbuf = NULL;
	}
	if (from->priv->pixbuf) {
		g_object_ref (from->priv->pixbuf);
		to->priv->pixbuf = from->priv->pixbuf;
	}

	/**/

	if (to->priv->animation) {
		g_object_unref (to->priv->animation);
		to->priv->animation = NULL;
	}
	if (from->priv->animation) { /* FIXME: check thread issues. */
		g_object_ref (from->priv->animation);
		to->priv->animation = from->priv->animation;
	}

	if ((to->priv->pixbuf == NULL) && (to->priv->animation == NULL))
		error = g_error_new_literal (GTHUMB_ERROR, 0, "No image available");

	g_mutex_unlock (to->priv->data_mutex);
	g_mutex_unlock (from->priv->data_mutex);

	g_signal_emit (G_OBJECT (to), gth_image_loader_signals[READY], 0, error);
}
