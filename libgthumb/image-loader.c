/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2001, 2003 The Free Software Foundation, Inc.
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
#define GDK_PIXBUF_ENABLE_BACKEND
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <gtk/gtkmain.h>
#include <gtk/gtksignal.h>
#include <libgnomevfs/gnome-vfs-async-ops.h>
#include <libgnomevfs/gnome-vfs-uri.h>
#include <libgnomevfs/gnome-vfs-utils.h>
#include <gdk-pixbuf/gdk-pixbuf-animation.h>
#include <sys/stat.h>
#include "image-loader.h"
#include "gthumb-marshal.h"
#include "file-utils.h"
#include "glib-utils.h"
#include "gth-exif-utils.h"
#include "pixbuf-utils.h"


#define REFRESH_RATE 5
G_LOCK_DEFINE_STATIC (pixbuf_loader_lock);


struct _ImageLoaderPrivateData {
	GdkPixbuf            *pixbuf;
	GdkPixbufAnimation   *animation;

	gboolean              as_animation; /* Whether to load the image in a
					     * GdkPixbufAnimation structure. */

	GnomeVFSURI          *uri;

	GnomeVFSAsyncHandle  *info_handle;

	GnomeVFSFileSize      bytes_read;
	GnomeVFSFileSize      bytes_total;

	gboolean              done;
	gboolean              error;
	gboolean              loader_done;
	gboolean              interrupted;
	gboolean              loading;

	GTimer               *timer;
	int                   priority;

	DoneFunc              done_func;
	gpointer              done_func_data;

	gboolean              emit_signal;

	guint                 check_id;
	guint                 idle_id;

	GThread              *thread;

	GMutex               *yes_or_no;

	gboolean              exit_thread;
	GMutex               *exit_thread_mutex;

	gboolean              start_loading;
	GMutex               *start_loading_mutex;
	GCond                *start_loading_cond;

	LoaderFunc            loader;
	gpointer              loader_data;
};


enum {
	IMAGE_ERROR,
	IMAGE_DONE,
	IMAGE_PROGRESS,
	LAST_SIGNAL
};


static GObjectClass *parent_class = NULL;
static guint image_loader_signals[LAST_SIGNAL] = { 0 };


void
image_loader_finalize__step2 (GObject *object)
{
	ImageLoader            *il;
	ImageLoaderPrivateData *priv;

        il = IMAGE_LOADER (object);
	priv = il->priv;

	g_mutex_lock (priv->yes_or_no);
	if (priv->pixbuf != NULL)
		g_object_unref (G_OBJECT (priv->pixbuf));

	if (priv->animation != NULL)
		g_object_unref (G_OBJECT (priv->animation));

	if (priv->uri != NULL) {
		gnome_vfs_uri_unref (priv->uri);
		priv->uri = NULL;
	}
	g_mutex_unlock (priv->yes_or_no);

	g_timer_destroy (priv->timer);

	g_mutex_lock (priv->exit_thread_mutex);
	priv->exit_thread = TRUE;
	g_mutex_unlock (priv->exit_thread_mutex);

	g_mutex_lock (priv->start_loading_mutex);
	priv->start_loading = TRUE;
	g_cond_signal (priv->start_loading_cond);
	g_mutex_unlock (priv->start_loading_mutex);

	g_thread_join (priv->thread);

	g_cond_free  (priv->start_loading_cond);
	g_mutex_free (priv->yes_or_no);
	g_mutex_free (priv->start_loading_mutex);
	g_mutex_free (priv->exit_thread_mutex);

	g_free (priv);
	il->priv = NULL;

	/* Chain up */
	G_OBJECT_CLASS (parent_class)->finalize (object);
}


static void image_loader_stop_common (ImageLoader *il,
				      DoneFunc     done_func,
				      gpointer     done_func_data,
				      gboolean     emit_sig,
				      gboolean     use_idle_cb);


static void
image_loader_finalize (GObject *object)
{
	ImageLoader            *il;
	ImageLoaderPrivateData *priv;

        g_return_if_fail (object != NULL);
        g_return_if_fail (IS_IMAGE_LOADER (object));

        il = IMAGE_LOADER (object);
	priv = il->priv;

	if (priv->idle_id != 0) {
		g_source_remove (priv->idle_id);
		priv->idle_id = 0;
	}

	image_loader_stop_common (il,
				  (DoneFunc) image_loader_finalize__step2,
				  object,
				  FALSE,
				  FALSE);
}


static void
image_loader_class_init (ImageLoaderClass *class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (class);

	parent_class = g_type_class_peek_parent (class);

	image_loader_signals[IMAGE_ERROR] =
		g_signal_new ("image_error",
			      G_TYPE_FROM_CLASS (class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (ImageLoaderClass, image_error),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__VOID,
			      G_TYPE_NONE,
			      0);
	image_loader_signals[IMAGE_DONE] =
		g_signal_new ("image_done",
			      G_TYPE_FROM_CLASS (class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (ImageLoaderClass, image_done),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__VOID,
			      G_TYPE_NONE,
			      0);
	image_loader_signals[IMAGE_PROGRESS] =
		g_signal_new ("image_progress",
			      G_TYPE_FROM_CLASS (class),
			      G_SIGNAL_RUN_FIRST,
			      G_STRUCT_OFFSET (ImageLoaderClass, image_progress),
			      NULL, NULL,
			      gthumb_marshal_VOID__FLOAT,
			      G_TYPE_NONE,
			      1, G_TYPE_FLOAT);

	object_class->finalize = image_loader_finalize;

	class->image_error = NULL;
	class->image_done = NULL;
	class->image_progress = NULL;
}

static void * load_image_thread (void *thread_data);

static void
image_loader_init (ImageLoader *il)
{
	ImageLoaderPrivateData *priv;

	il->priv = g_new0 (ImageLoaderPrivateData, 1);
	priv = il->priv;

	priv->pixbuf = NULL;
	priv->animation = NULL;
	priv->uri = NULL;

	priv->bytes_read = 0;
	priv->bytes_total = 0;

	priv->info_handle = NULL;

	priv->done = FALSE;
	priv->error = FALSE;
	priv->loader_done = FALSE;
	priv->interrupted = FALSE;

	priv->timer = g_timer_new ();
	priv->priority = GNOME_VFS_PRIORITY_DEFAULT;

	priv->done_func = NULL;
	priv->done_func_data = NULL;

	priv->check_id = 0;
	priv->idle_id = 0;

	priv->yes_or_no = g_mutex_new ();

	priv->exit_thread = FALSE;
	priv->exit_thread_mutex = g_mutex_new ();

	priv->start_loading = FALSE;
	priv->start_loading_mutex = g_mutex_new ();
	priv->start_loading_cond = g_cond_new ();

	priv->loader = NULL;
	priv->loader_data = NULL;

	priv->thread = g_thread_create (load_image_thread, il, TRUE, NULL);
}


GType
image_loader_get_type ()
{
        static GType type = 0;

        if (! type) {
                GTypeInfo type_info = {
			sizeof (ImageLoaderClass),
			NULL,
			NULL,
			(GClassInitFunc) image_loader_class_init,
			NULL,
			NULL,
			sizeof (ImageLoader),
			0,
			(GInstanceInitFunc) image_loader_init
		};

		type = g_type_register_static (G_TYPE_OBJECT,
					       "ImageLoader",
					       &type_info,
					       0);
	}

        return type;
}


GObject*
image_loader_new (const gchar *path,
		  gboolean     as_animation)
{
	ImageLoaderPrivateData *priv;
	ImageLoader            *il;

	il = IMAGE_LOADER (g_object_new (IMAGE_LOADER_TYPE, NULL));
	priv = (ImageLoaderPrivateData*) il->priv;

	priv->as_animation = as_animation;
	image_loader_set_path (il, path);

	return G_OBJECT (il);
}


void
image_loader_set_loader (ImageLoader *il,
			 LoaderFunc   loader,
			 gpointer     loader_data)
{
	g_return_if_fail (il != NULL);

	g_mutex_lock (il->priv->yes_or_no);
	il->priv->loader = loader;
	il->priv->loader_data = loader_data;
	g_mutex_unlock (il->priv->yes_or_no);
}


void
image_loader_set_path (ImageLoader *il,
		       const char  *path)
{
	ImageLoaderPrivateData *priv;

	g_return_if_fail (il != NULL);

	priv = il->priv;

	g_mutex_lock (priv->yes_or_no);

	if (priv->uri != NULL) {
		gnome_vfs_uri_unref (priv->uri);
		priv->uri = NULL;
	}
	if (path != NULL)
		priv->uri = new_uri_from_path (path);

	g_mutex_unlock (priv->yes_or_no);
}


void
image_loader_set_uri (ImageLoader       *il,
		      const GnomeVFSURI *uri)
{
	ImageLoaderPrivateData *priv;

	g_return_if_fail (il != NULL);

	priv = il->priv;

	g_mutex_lock (priv->yes_or_no);

	if (priv->uri != NULL) {
		gnome_vfs_uri_unref (priv->uri);
		priv->uri = NULL;
	}
	if (uri != NULL)
		priv->uri = gnome_vfs_uri_dup (uri);

	g_mutex_unlock (priv->yes_or_no);
}


void
image_loader_set_pixbuf (ImageLoader *il,
			 GdkPixbuf   *pixbuf)
{
	ImageLoaderPrivateData *priv;

	g_return_if_fail (il != NULL);
	g_return_if_fail (pixbuf != NULL);

	priv = il->priv;

	g_mutex_lock (priv->yes_or_no);

	if (priv->pixbuf != NULL) {
		g_object_unref (priv->pixbuf);
		priv->pixbuf = NULL;
	}

	if (pixbuf != NULL) {
		g_object_ref (pixbuf);
		priv->pixbuf = pixbuf;
		/*priv->pixbuf = gdk_pixbuf_copy (pixbuf);*/
	}

	g_mutex_unlock (priv->yes_or_no);
}

static void
image_loader_sync_pixbuf (ImageLoader *il)
{
	GdkPixbuf              *pixbuf;
	GdkPixbuf              *temp;
	ImageLoaderPrivateData *priv;
	ExifShort				orientation;
	GthTransform			transform;

	g_return_if_fail (il != NULL);

	priv = il->priv;

	orientation = get_exif_tag_short (image_loader_get_path(il), EXIF_TAG_ORIENTATION);
	transform =  (orientation >= 1 && orientation <= 8 ?
						orientation :
						GTH_TRANSFORM_NONE);

	g_mutex_lock (priv->yes_or_no);

	if (priv->animation == NULL) {
		if (priv->pixbuf != NULL)
			g_object_unref (priv->pixbuf);
		priv->pixbuf = NULL;
		g_mutex_unlock (priv->yes_or_no);
		return;
	}

	temp = gdk_pixbuf_animation_get_static_image (priv->animation);
	pixbuf = _gdk_pixbuf_transform (temp, transform);

	if (priv->pixbuf == pixbuf) {
		g_object_unref (pixbuf);
		g_mutex_unlock (priv->yes_or_no);
		return;
	}

	if (priv->pixbuf != NULL) {
		g_object_unref (priv->pixbuf);
		priv->pixbuf = NULL;
	}
	if (pixbuf != NULL) {
		g_object_ref (pixbuf);
		priv->pixbuf = pixbuf;
		/*priv->pixbuf = gdk_pixbuf_copy (pixbuf);*/
	}
	g_object_unref (pixbuf);

	g_mutex_unlock (priv->yes_or_no);
}


static void
image_loader_sync_pixbuf_from_loader (ImageLoader     *il,
				      GdkPixbufLoader *pb_loader)
{
	GdkPixbuf              *pixbuf;
	ImageLoaderPrivateData *priv;

	g_return_if_fail (il != NULL);

	priv = il->priv;

	g_mutex_lock (priv->yes_or_no);

	if (priv->as_animation) {
		if (priv->animation != NULL)
			g_object_unref (priv->animation);
		priv->animation = gdk_pixbuf_loader_get_animation (pb_loader);

		if ((priv->animation != NULL)
		    && ! gdk_pixbuf_animation_is_static_image (priv->animation)) {
			g_object_ref (G_OBJECT (priv->animation));
			g_mutex_unlock (priv->yes_or_no);
			return;
		} else
			priv->animation = NULL;
	}

	pixbuf = gdk_pixbuf_loader_get_pixbuf (pb_loader);
	g_object_ref (pixbuf);

	if (priv->pixbuf == pixbuf) {
		g_object_unref (priv->pixbuf);
		g_mutex_unlock (priv->yes_or_no);
		return;
	}

	if (priv->pixbuf != NULL) {
		g_object_unref (priv->pixbuf);
		priv->pixbuf = NULL;
	}
	if (pixbuf != NULL) {
		g_object_ref (pixbuf);
		priv->pixbuf = pixbuf;
		/*priv->pixbuf = gdk_pixbuf_copy (pixbuf);*/
	}
	g_object_unref (pixbuf);

	g_mutex_unlock (priv->yes_or_no);
}


static void
image_loader_interrupted (ImageLoader *il)
{
	ImageLoaderPrivateData *priv = il->priv;

	g_mutex_lock (priv->yes_or_no);
	priv->error = TRUE;
	g_mutex_unlock (priv->yes_or_no);

	image_loader_stop_common (il,
				  priv->done_func,
				  priv->done_func_data,
				  TRUE,
				  TRUE);
}


static void
image_loader_done (ImageLoader *il)
{
	ImageLoaderPrivateData *priv = il->priv;

	g_mutex_lock (priv->yes_or_no);
	priv->error = FALSE;
	g_mutex_unlock (priv->yes_or_no);

	image_loader_stop_common (il, NULL, NULL, TRUE, TRUE);
}


static void
image_loader_error (ImageLoader *il)
{
	ImageLoaderPrivateData *priv = il->priv;

	g_mutex_lock (priv->yes_or_no);
	priv->error = TRUE;
	g_mutex_unlock (priv->yes_or_no);

	image_loader_stop_common (il, NULL, NULL, TRUE, TRUE);
}


static void *
load_image_thread (void *thread_data)
{
	ImageLoader            *il = thread_data;
	ImageLoaderPrivateData *priv = il->priv;
	GdkPixbufAnimation     *animation = NULL;
	GError                 *error = NULL;

	for (;;) {
		char     *path;
		gboolean  exit_thread;

		g_mutex_lock (priv->start_loading_mutex);
		while (! priv->start_loading)
			g_cond_wait (priv->start_loading_cond, priv->start_loading_mutex);
		priv->start_loading = FALSE;
		g_mutex_unlock (priv->start_loading_mutex);

		g_mutex_lock (priv->exit_thread_mutex);
		exit_thread = priv->exit_thread;
		g_mutex_unlock (priv->exit_thread_mutex);

		if (exit_thread)
			break;

		path = image_loader_get_path (il);

		g_mutex_lock (priv->yes_or_no);

		G_LOCK (pixbuf_loader_lock);

		animation = NULL;
		if (path != NULL) {
			if (priv->loader != NULL)
				animation = (*priv->loader) (path, &error, priv->loader_data);
			else {
				if (image_is_gif__accurate (path))
					animation = gdk_pixbuf_animation_new_from_file (path, &error);
				else {
					GdkPixbuf *pixbuf;
					pixbuf = gdk_pixbuf_new_from_file (path, &error);
					if (pixbuf != NULL) {
						animation = gdk_pixbuf_non_anim_new (pixbuf);
						g_object_unref (pixbuf);
					}
				}
			}
		}

		G_UNLOCK (pixbuf_loader_lock);

		priv->loader_done = TRUE;

		if (priv->animation != NULL)
			g_object_unref (priv->animation);
		priv->animation = animation;

		if ((animation == NULL) || (error != NULL)) {
			priv->error = TRUE;
			priv->done = FALSE;
			if (error != NULL)
				g_clear_error (&error);
		} else {
			priv->error = FALSE;
			priv->done = TRUE;
		}

		g_mutex_unlock (priv->yes_or_no);

		g_free (path);
	}

	return NULL;
}


static void
image_loader_stop__final_step (ImageLoader *il,
			       gboolean     use_idle_cb)
{
	ImageLoaderPrivateData *priv = il->priv;
	DoneFunc                done_func = priv->done_func;
	gboolean                error;

	g_mutex_lock (priv->yes_or_no);
	error = priv->error;
	priv->done = TRUE;
	g_mutex_unlock (priv->yes_or_no);

	if (! error && ! priv->interrupted && priv->loading)
		image_loader_sync_pixbuf (il);
	priv->loading = FALSE;

	priv->done_func = NULL;
	if (done_func != NULL) {
		IdleCall* call = idle_call_new (done_func, priv->done_func_data);
		if (priv->idle_id != 0)
			g_source_remove (priv->idle_id);
		priv->idle_id = idle_call_exec (call, use_idle_cb);
	}

	if (! priv->emit_signal || priv->interrupted) {
		priv->interrupted = FALSE;
		return;
	}

	priv->interrupted = FALSE;

	if (error)
		g_signal_emit (G_OBJECT (il),
			       image_loader_signals[IMAGE_ERROR], 0);
	else
		g_signal_emit (G_OBJECT (il),
			       image_loader_signals[IMAGE_DONE], 0);
}


static gboolean
check_thread (gpointer data)
{
	ImageLoader            *il = data;
	gboolean                done, error, loader_done;
	ImageLoaderPrivateData *priv = il->priv;

	/* Remove check. */

	g_source_remove (priv->check_id);
	priv->check_id = 0;

	/**/

	g_mutex_lock (priv->yes_or_no);
	done = priv->done;
	error = priv->error;
	loader_done = priv->loader_done;
	g_mutex_unlock (priv->yes_or_no);

	/**/

	if (loader_done && priv->interrupted)
		image_loader_interrupted (il);

	else if (loader_done && done)
		image_loader_done (il);

	else if (loader_done && error)
		image_loader_error (il);

	else	/* Add check again. */
		priv->check_id = g_timeout_add (REFRESH_RATE, check_thread, il);

	return FALSE;
}


static void
get_file_info_cb (GnomeVFSAsyncHandle *handle,
		  GList               *results,
		  gpointer             data)
{
	GnomeVFSGetFileInfoResult *info_result;
	ImageLoader               *il = data;
	ImageLoaderPrivateData    *priv = il->priv;

	info_result = results->data;
	priv->info_handle = NULL;

	if (info_result->result != GNOME_VFS_OK) {
		image_loader_error (il);
		return;
	}

	priv->bytes_total = info_result->file_info->size;
	priv->bytes_read = 0;

	if (priv->pixbuf != NULL) {
		g_object_unref (priv->pixbuf);
		priv->pixbuf = NULL;
	}

	g_mutex_lock (priv->yes_or_no);
	if (priv->animation != NULL) {
		g_object_unref (priv->animation);
		priv->animation = NULL;
	}
	g_mutex_unlock (priv->yes_or_no);

	g_mutex_lock (priv->start_loading_mutex);
	priv->start_loading = TRUE;
	g_cond_signal (priv->start_loading_cond);
	g_mutex_unlock (priv->start_loading_mutex);

	il->priv->check_id = g_timeout_add (REFRESH_RATE, check_thread, il);
}


static void
image_loader_start__step2 (ImageLoader *il)
{
	GList                  *uri_list;
	ImageLoaderPrivateData *priv;

	g_return_if_fail (il != NULL);

	priv = il->priv;

	g_mutex_lock (priv->yes_or_no);

	g_return_if_fail (priv->uri != NULL);

	g_timer_reset (priv->timer);
	g_timer_start (priv->timer);

	priv->done = FALSE;
	priv->error = FALSE;
	priv->loader_done = FALSE;
	priv->loading = TRUE;
	uri_list = g_list_prepend (NULL, gnome_vfs_uri_dup (priv->uri));
	g_mutex_unlock (priv->yes_or_no);

	gnome_vfs_async_get_file_info (& (priv->info_handle),
				       uri_list,
				       (GNOME_VFS_FILE_INFO_DEFAULT
					| GNOME_VFS_FILE_INFO_FOLLOW_LINKS),
				       priv->priority,
				       get_file_info_cb,
				       il);

	{
		GList *scan;
		for (scan = uri_list; scan; scan = scan->next)
			gnome_vfs_uri_unref((GnomeVFSURI*) scan->data);
		g_list_free (uri_list);
	}
}


void
image_loader_start (ImageLoader *il)
{
	ImageLoaderPrivateData *priv;

	g_return_if_fail (il != NULL);

	priv = il->priv;

	g_mutex_lock (priv->yes_or_no);
	if (priv->uri == NULL) {
		g_mutex_unlock (priv->yes_or_no);
		return;
	}
	g_mutex_unlock (priv->yes_or_no);

	image_loader_stop_common (il,
				  (DoneFunc) image_loader_start__step2,
				  il,
				  FALSE,
				  TRUE);
}


/* -- image_loader_stop -- */



static void
close_info_cb (GnomeVFSAsyncHandle *handle,
	       GnomeVFSResult       result,
	       gpointer             data)
{
	/* FIXME */

	/*
	ImageLoader            *il = data;
	ImageLoaderPrivateData *priv = il->priv;
	priv->info_handle = NULL;
	image_loader_stop__final_step (il);
	*/
}


static void
image_loader_stop_common (ImageLoader *il,
			  DoneFunc     done_func,
			  gpointer     done_func_data,
			  gboolean     emit_sig,
			  gboolean     use_idle_cb)
{
	ImageLoaderPrivateData *priv;

	g_return_if_fail (il != NULL);
	priv = il->priv;

	g_timer_stop (priv->timer);

	priv->done_func = done_func;
	priv->done_func_data = done_func_data;
	priv->emit_signal = emit_sig;

	if (priv->info_handle != NULL)
		gnome_vfs_async_close (priv->info_handle, close_info_cb, il);

	priv->info_handle = NULL;
	image_loader_stop__final_step (il, use_idle_cb);
}


void
image_loader_stop (ImageLoader *il,
		   DoneFunc     done_func,
		   gpointer     done_func_data)
{
	ImageLoaderPrivateData *priv;

	g_return_if_fail (il != NULL);

	priv = il->priv;

	g_mutex_lock (priv->yes_or_no);
	priv->error = FALSE;
	g_mutex_unlock (priv->yes_or_no);

	if (priv->loading) {
		priv->emit_signal = TRUE;
		priv->interrupted = TRUE;
		priv->done_func = done_func;
		priv->done_func_data = done_func_data;
	} else
		image_loader_stop_common (il,
					  done_func,
					  done_func_data,
					  FALSE,
					  TRUE);
}


void
image_loader_stop_with_error (ImageLoader *il,
			      DoneFunc     done_func,
			      gpointer     done_func_data)
{
	ImageLoaderPrivateData *priv;

	g_return_if_fail (il != NULL);

	priv = il->priv;

	g_mutex_lock (priv->yes_or_no);
	priv->error = TRUE;
	g_mutex_unlock (priv->yes_or_no);

	image_loader_stop_common (il, done_func, done_func_data, TRUE, TRUE);
}


GdkPixbuf *
image_loader_get_pixbuf (ImageLoader *il)
{
	g_return_val_if_fail (il != NULL, NULL);
	return il->priv->pixbuf;
}


void
image_loader_set_priority (ImageLoader *il,
			   int          priority)
{
	ImageLoaderPrivateData *priv;

	g_return_if_fail (il != NULL);
	priv = il->priv;
	priv->priority = priority;
}


GdkPixbufAnimation *
image_loader_get_animation (ImageLoader *il)
{
	ImageLoaderPrivateData *priv;
	GdkPixbufAnimation     *animation;

	g_return_val_if_fail (il != NULL, NULL);
	priv = il->priv;

	g_mutex_lock (priv->yes_or_no);
	animation = priv->animation;
	if (animation != NULL)
		g_object_ref (animation);
	g_mutex_unlock (priv->yes_or_no);

	return animation;
}


float
image_loader_get_percent (ImageLoader *il)
{
	ImageLoaderPrivateData *priv;

	g_return_val_if_fail (il != NULL, 0.0);
	priv = il->priv;

	if (priv->bytes_total == 0)
		return 0.0;
	else
		return (float) priv->bytes_read / priv->bytes_total;
}


gint
image_loader_get_is_done (ImageLoader *il)
{
	ImageLoaderPrivateData *priv;
	gboolean                is_done;

	g_return_val_if_fail (il != NULL, 0);
	priv = il->priv;

	g_mutex_lock (priv->yes_or_no);
	is_done = priv->done && priv->loader_done;
	g_mutex_unlock (priv->yes_or_no);

	return is_done;
}


gchar *
image_loader_get_path (ImageLoader *il)
{
	ImageLoaderPrivateData *priv;
	char                   *path;
        char                   *esc_path;
	GnomeVFSURI            *uri;

	g_return_val_if_fail (il != NULL, 0);
	priv = il->priv;

	g_mutex_lock (priv->yes_or_no);

	if (priv->uri == NULL) {
		g_mutex_unlock (priv->yes_or_no);
                return NULL;
	}

	uri = gnome_vfs_uri_dup (priv->uri);

        path = gnome_vfs_uri_to_string (uri, GNOME_VFS_URI_HIDE_TOPLEVEL_METHOD);
        esc_path = gnome_vfs_unescape_string (path, NULL);
        g_free (path);
	gnome_vfs_uri_unref (uri);

	g_mutex_unlock (priv->yes_or_no);

        return esc_path;
}


GnomeVFSURI *
image_loader_get_uri (ImageLoader *il)
{
	ImageLoaderPrivateData *priv;
	GnomeVFSURI            *uri = NULL;

	g_return_val_if_fail (il != NULL, 0);
	priv = il->priv;

	g_mutex_lock (priv->yes_or_no);
	if (priv->uri != NULL)
		uri = gnome_vfs_uri_dup (priv->uri);
	g_mutex_unlock (priv->yes_or_no);

	return uri;
}


GTimer *
image_loader_get_timer (ImageLoader *il)
{
	g_return_val_if_fail (il != NULL, 0);
	return il->priv->timer;
}


void
image_loader_load_from_pixbuf_loader (ImageLoader *il,
				      GdkPixbufLoader *pixbuf_loader)
{
	gboolean error;

	g_return_if_fail (il != NULL);

	image_loader_sync_pixbuf_from_loader (il, pixbuf_loader);

	g_mutex_lock (il->priv->yes_or_no);
	error = (il->priv->pixbuf == NULL) && (il->priv->animation == NULL);
	g_mutex_unlock (il->priv->yes_or_no);

	if (error)
		g_signal_emit (G_OBJECT (il), image_loader_signals[IMAGE_ERROR], 0);
	else
		g_signal_emit (G_OBJECT (il), image_loader_signals[IMAGE_DONE], 0);
}


void
image_loader_load_from_image_loader (ImageLoader *to,
				     ImageLoader *from)
{
	gboolean error;

	g_return_if_fail (to != NULL);
	g_return_if_fail (from != NULL);

	g_mutex_lock (to->priv->yes_or_no);
	g_mutex_lock (from->priv->yes_or_no);

	if (to->priv->uri != NULL) {
		gnome_vfs_uri_unref (to->priv->uri);
		to->priv->uri = NULL;
	}

	if (from->priv->uri != NULL)
		to->priv->uri = gnome_vfs_uri_dup (from->priv->uri);

	/**/

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

	if (from->priv->animation) { /* FIXME*/
		g_object_ref (from->priv->animation);
		to->priv->animation = from->priv->animation;
	}

	error = (to->priv->pixbuf == NULL) && (to->priv->animation == NULL);

	g_mutex_unlock (to->priv->yes_or_no);
	g_mutex_unlock (from->priv->yes_or_no);

	if (error)
		g_signal_emit (G_OBJECT (to), image_loader_signals[IMAGE_ERROR], 0);
	else
		g_signal_emit (G_OBJECT (to), image_loader_signals[IMAGE_DONE], 0);
}

