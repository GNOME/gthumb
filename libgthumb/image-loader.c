/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2001 The Free Software Foundation, Inc.
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

#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <gtk/gtkmain.h>
#include <gtk/gtksignal.h>
#include <libgnomevfs/gnome-vfs-async-ops.h>
#include <libgnomevfs/gnome-vfs-uri.h>
#include <libgnomevfs/gnome-vfs-utils.h>
#include <sys/stat.h>
#include "image-loader.h"
#include "gthumb-marshal.h"


#define REFRESH_RATE 5


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

	GTimer               *timer;
	int                   priority;

	DoneFunc              done_func;
	gpointer              done_func_data;

	gboolean              emit_signal;

	guint                 check_id;

	GThread              *thread;

	GMutex               *yes_or_no;

	gboolean              exit_thread;
	GMutex               *exit_thread_mutex;

	gboolean              start_loading;
	GMutex               *start_loading_mutex;
	GCond                *start_loading_cond;
};


enum {
	ERROR,
	DONE,
	PROGRESS,
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

	if (priv->pixbuf)
		g_object_unref (G_OBJECT (priv->pixbuf));
	
	if (priv->animation)
		g_object_unref (G_OBJECT (priv->animation));

	if (priv->uri)
		gnome_vfs_uri_unref (priv->uri);
	
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
				      gboolean     emit_sig);


static void 
image_loader_finalize (GObject *object)
{
	ImageLoader            *il;
	ImageLoaderPrivateData *priv;

        g_return_if_fail (object != NULL);
        g_return_if_fail (IS_IMAGE_LOADER (object));
  
        il = IMAGE_LOADER (object);
	priv = il->priv;
	
	image_loader_stop_common (il, 
				  (DoneFunc) image_loader_finalize__step2, 
				  object,
				  FALSE);
}


static void
image_loader_class_init (ImageLoaderClass *class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (class);

	parent_class = g_type_class_peek_parent (class);

	image_loader_signals[ERROR] =
		g_signal_new ("error",
			      G_TYPE_FROM_CLASS (class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (ImageLoaderClass, error),
			      NULL, NULL,
			      gthumb_marshal_VOID__VOID,
			      G_TYPE_NONE, 
			      0);
	image_loader_signals[DONE] =
		g_signal_new ("done",
			      G_TYPE_FROM_CLASS (class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (ImageLoaderClass, done),
			      NULL, NULL,
			      gthumb_marshal_VOID__VOID,
			      G_TYPE_NONE, 
			      0);
	image_loader_signals[PROGRESS] =
		g_signal_new ("progress",
			      G_TYPE_FROM_CLASS (class),
			      G_SIGNAL_RUN_FIRST,
			      G_STRUCT_OFFSET (ImageLoaderClass, progress),
			      NULL, NULL,
			      gthumb_marshal_VOID__FLOAT,
			      G_TYPE_NONE, 
			      1, G_TYPE_FLOAT);

	object_class->finalize = image_loader_finalize;

	class->error = NULL;
	class->done = NULL;
	class->progress = NULL;
}

static void * load_image_thread (void *thread_data);

static void
image_loader_init (ImageLoader *il)
{
	ImageLoaderPrivateData *priv;

	il->priv = g_new (ImageLoaderPrivateData, 1);
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

	priv->timer = g_timer_new ();
	priv->priority = GNOME_VFS_PRIORITY_DEFAULT;

	priv->done_func = NULL;
	priv->done_func_data = NULL;

	priv->check_id = 0;

	priv->yes_or_no = g_mutex_new ();

	priv->exit_thread = FALSE;
	priv->exit_thread_mutex = g_mutex_new ();

	priv->start_loading = FALSE;
	priv->start_loading_mutex = g_mutex_new ();
	priv->start_loading_cond = g_cond_new ();

	priv->thread = g_thread_create (load_image_thread, il, TRUE, NULL);
}


GType
image_loader_get_type ()
{
        static guint type = 0;

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
image_loader_set_path (ImageLoader *il,
		       const gchar *path)
{
	ImageLoaderPrivateData *priv;
	char                   *escaped_path;

	g_return_if_fail (il != NULL);

	if (path == NULL)
		return;

	priv = il->priv;

	g_mutex_lock (priv->yes_or_no);

	if (priv->uri != NULL)
		gnome_vfs_uri_unref (priv->uri);
	escaped_path = gnome_vfs_escape_path_string (path);
	priv->uri = gnome_vfs_uri_new (escaped_path);
	g_free (escaped_path);

	g_mutex_unlock (priv->yes_or_no);
}


void
image_loader_set_uri (ImageLoader       *il,
		      const GnomeVFSURI *uri)
{
	ImageLoaderPrivateData *priv;

	g_return_if_fail (il != NULL);
	g_return_if_fail (uri != NULL);
	
	priv = il->priv;

	g_mutex_lock (priv->yes_or_no);

	if (priv->uri != NULL)
		gnome_vfs_uri_unref (priv->uri);
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

	if (priv->pixbuf != NULL)
		g_object_unref (priv->pixbuf);

	g_object_ref (pixbuf);
	priv->pixbuf = pixbuf;
}


static void 
image_loader_sync_pixbuf (ImageLoader *il)
{
	GdkPixbuf              *pixbuf;
	ImageLoaderPrivateData *priv;
  
	g_return_if_fail (il != NULL);
     
	priv = il->priv;

	if (priv->animation == NULL) {
		if (priv->pixbuf != NULL) 
			g_object_unref (priv->pixbuf);
		priv->pixbuf = NULL;
		return;
	}

	pixbuf = gdk_pixbuf_animation_get_static_image (priv->animation);

	if (priv->pixbuf == pixbuf)
		return;
		
	if (pixbuf != NULL) 
		g_object_ref (pixbuf);
	if (priv->pixbuf != NULL) 
		g_object_unref (priv->pixbuf);
     	priv->pixbuf = pixbuf;
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

	g_mutex_unlock (priv->yes_or_no);

	pixbuf = gdk_pixbuf_loader_get_pixbuf (pb_loader);

	if (priv->pixbuf == pixbuf)
		return;
		
	if (pixbuf != NULL) 
		g_object_ref (pixbuf);
	if (priv->pixbuf != NULL) 
		g_object_unref (priv->pixbuf);
     	priv->pixbuf = pixbuf;
}


static void 
image_loader_done (ImageLoader *il)
{
	ImageLoaderPrivateData *priv = il->priv;

	g_mutex_lock (priv->yes_or_no);
	priv->error = FALSE;
	g_mutex_unlock (priv->yes_or_no);

	image_loader_stop_common (il, NULL, NULL, TRUE);
}


static void 
image_loader_error (ImageLoader *il)
{
	ImageLoaderPrivateData *priv = il->priv;

	g_mutex_lock (priv->yes_or_no);
	priv->error = TRUE;
	g_mutex_unlock (priv->yes_or_no);

	image_loader_stop_common (il, NULL, NULL, TRUE);
}


static void *
load_image_thread (void *thread_data)
{
	ImageLoader            *il = thread_data;
	ImageLoaderPrivateData *priv = il->priv;
	GdkPixbufAnimation     *animation;
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

		g_mutex_lock (priv->yes_or_no);
		path = image_loader_get_path (il);
		g_mutex_unlock (priv->yes_or_no);

		animation = gdk_pixbuf_animation_new_from_file (path, &error);

		g_mutex_lock (priv->yes_or_no);
		
		priv->loader_done = TRUE;

		if (priv->animation != NULL)
			g_object_unref (priv->animation);
		priv->animation = animation;

		if ((animation == NULL) || (error != NULL)) {
			priv->error = TRUE;
			priv->done = FALSE;
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
image_loader_stop__final_step (ImageLoader *il) 
{
	ImageLoaderPrivateData *priv = il->priv;
	DoneFunc                done_func = priv->done_func;
	gboolean                error;

	error = priv->error;

	g_mutex_lock (priv->yes_or_no);
	priv->done = TRUE;
	g_mutex_unlock (priv->yes_or_no);
	
	if (! error) 
		image_loader_sync_pixbuf (il);
	
	priv->done_func = NULL;
	if (done_func != NULL)
		(*done_func) (priv->done_func_data);
	
	if (! priv->emit_signal)
		return;
		
	if (error)
		g_signal_emit (G_OBJECT (il), 
			       image_loader_signals[ERROR], 0);
	else
		g_signal_emit (G_OBJECT (il), 
			       image_loader_signals[DONE], 0);
}


static gint
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

	if (loader_done && done) 
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
	g_return_if_fail (priv->uri != NULL);

	g_timer_reset (priv->timer);
	g_timer_start (priv->timer);

	g_mutex_lock (priv->yes_or_no);

	priv->done = FALSE;
	priv->error = FALSE;
	priv->loader_done = FALSE;

	g_mutex_unlock (priv->yes_or_no);

	uri_list = g_list_prepend (NULL, priv->uri);
	gnome_vfs_async_get_file_info (& (priv->info_handle),
				       uri_list,
				       (GNOME_VFS_FILE_INFO_DEFAULT
					| GNOME_VFS_FILE_INFO_FOLLOW_LINKS),
				       GNOME_VFS_PRIORITY_DEFAULT,
				       get_file_info_cb,
				       il);
	g_list_free (uri_list);
}


void
image_loader_start (ImageLoader *il)
{
	ImageLoaderPrivateData *priv;

	g_return_if_fail (il != NULL);
	priv = il->priv;
	g_return_if_fail (priv->uri != NULL);

	image_loader_stop_common (il, (DoneFunc) image_loader_start__step2, il, FALSE);
}


/* -- image_loader_stop -- */


static void
close_info_cb (GnomeVFSAsyncHandle *handle,
	       GnomeVFSResult       result,
	       gpointer             data)
{
	ImageLoader            *il = data;
	ImageLoaderPrivateData *priv = il->priv;

	priv->info_handle = NULL;
	image_loader_stop__final_step (il);
}


static void
image_loader_stop_common (ImageLoader *il,
			  DoneFunc     done_func,
			  gpointer     done_func_data,
			  gboolean     emit_sig)
{
	ImageLoaderPrivateData *priv;

	g_return_if_fail (il != NULL);
	priv = il->priv;

	priv->done_func = done_func;
	priv->done_func_data = done_func_data;
	priv->emit_signal = emit_sig;
	g_timer_stop (priv->timer);

	if (priv->info_handle != NULL) 
		gnome_vfs_async_close (priv->info_handle, close_info_cb, il);
	else 
		image_loader_stop__final_step (il);
}


void 
image_loader_stop (ImageLoader *il,
		   DoneFunc     done_func,
		   gpointer     done_func_data)
{
	ImageLoaderPrivateData *priv;
	gboolean                emit_sig;

	g_return_if_fail (il != NULL);

	priv = il->priv;

	g_mutex_lock (priv->yes_or_no);
	priv->error = FALSE;
	g_mutex_unlock (priv->yes_or_no);

	/* emit a signal only if there is an operation to stop */
	emit_sig = priv->info_handle != NULL;

	image_loader_stop_common (il, done_func, done_func_data, emit_sig);
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

	image_loader_stop_common (il, done_func, done_func_data, TRUE);
}


GdkPixbuf *
image_loader_get_pixbuf (ImageLoader *il)
{
	ImageLoaderPrivateData *priv;

	g_return_val_if_fail (il != NULL, NULL);
	priv = il->priv;

	return priv->pixbuf;
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

	g_return_val_if_fail (il != NULL, NULL);
	priv = il->priv;

	return priv->animation;
}


gfloat 
image_loader_get_percent (ImageLoader *il)
{
	ImageLoaderPrivateData *priv;

	g_return_val_if_fail (il != NULL, 0.0);
	priv = il->priv;

	if (priv->bytes_total == 0) 
		return 0.0;
	else
		return (gfloat) priv->bytes_read / priv->bytes_total;
}


gint 
image_loader_get_is_done (ImageLoader *il)
{
	ImageLoaderPrivateData *priv;

	g_return_val_if_fail (il != NULL, 0);
	priv = il->priv;

	return priv->done && priv->loader_done;
}


gchar *
image_loader_get_path (ImageLoader *il)
{
	ImageLoaderPrivateData *priv;
	char                   *path;
        char                   *esc_path;

	g_return_val_if_fail (il != NULL, 0);
	priv = il->priv;

        if (priv->uri == NULL) 
                return NULL;

        path = gnome_vfs_uri_to_string (priv->uri, 
                                        GNOME_VFS_URI_HIDE_TOPLEVEL_METHOD);
        esc_path = gnome_vfs_unescape_string (path, NULL);
        g_free (path);

        return esc_path;
}


GnomeVFSURI *
image_loader_get_uri (ImageLoader *il)
{
	ImageLoaderPrivateData *priv;

	g_return_val_if_fail (il != NULL, 0);
	priv = il->priv;

	return priv->uri;
}


GTimer *
image_loader_get_timer (ImageLoader *il)
{
	ImageLoaderPrivateData *priv;

	g_return_val_if_fail (il != NULL, 0);
	priv = il->priv;

	return priv->timer;
}


void
image_loader_load_from_pixbuf_loader (ImageLoader *il,
				      GdkPixbufLoader *pixbuf_loader)
{
	g_return_if_fail (il != NULL);

	image_loader_sync_pixbuf_from_loader (il, pixbuf_loader);

	if ((il->priv->pixbuf == NULL) && (il->priv->animation == NULL))
		g_signal_emit (G_OBJECT (il), image_loader_signals[ERROR], 0);
	else
		g_signal_emit (G_OBJECT (il), image_loader_signals[DONE], 0);
}


void
image_loader_load_from_image_loader (ImageLoader *to,
				     ImageLoader *from)
{
	g_return_if_fail (to != NULL);
	g_return_if_fail (from != NULL);

	if (to->priv->uri != NULL)
		gnome_vfs_uri_unref (to->priv->uri);
	to->priv->uri = gnome_vfs_uri_ref (from->priv->uri);

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

	g_mutex_lock (to->priv->yes_or_no);

	if (to->priv->animation) {
		g_object_unref (to->priv->animation);
		to->priv->animation = NULL;
	}

	if (from->priv->animation) {
		g_object_ref (from->priv->animation);
		to->priv->animation = from->priv->animation;
	}

	g_mutex_unlock (to->priv->yes_or_no);

	if ((to->priv->pixbuf == NULL) && (to->priv->animation == NULL))
		g_signal_emit (G_OBJECT (to), image_loader_signals[ERROR], 0);
	else
		g_signal_emit (G_OBJECT (to), image_loader_signals[DONE], 0);
}

