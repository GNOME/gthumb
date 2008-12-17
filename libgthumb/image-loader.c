/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2001, 2003, 2007 The Free Software Foundation, Inc.
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
#include <gtk/gtk.h>
#include <libgnomeui/gnome-thumbnail.h>
#include <libgnomevfs/gnome-vfs-async-ops.h>
#include <libgnomevfs/gnome-vfs-uri.h>
#include <libgnomevfs/gnome-vfs-utils.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <sys/stat.h>
#include "image-loader.h"
#include "gthumb-marshal.h"
#include "file-utils.h"
#include "glib-utils.h"
#include "gconf-utils.h"
#include "gth-exif-utils.h"
#include "pixbuf-utils.h"
#include "preferences.h"


#define DEF_THUMB_SIZE        128
#define THUMBNAIL_NORMAL_SIZE 128
#define REFRESH_RATE          5

G_LOCK_DEFINE_STATIC (pixbuf_loader_lock);


struct _ImageLoaderPrivateData {
	FileData              *file;
	GdkPixbuf             *pixbuf;
	GdkPixbufAnimation    *animation;

	GnomeThumbnailFactory *thumb_factory;

	gboolean               as_animation; /* Whether to load the image in a
					      * GdkPixbufAnimation structure. */

	gboolean               done;
	gboolean               error;
	gboolean               loader_done;
	gboolean               interrupted;
	gboolean               loading;

	int                    priority;

	DoneFunc               done_func;
	gpointer               done_func_data;

	gboolean               emit_signal;

	guint                  check_id;
	guint                  idle_id;

	GThread               *thread;

	GMutex                *data_mutex;

	gboolean               exit_thread;
	GMutex                *exit_thread_mutex;

	gboolean               start_loading;
	GMutex                *start_loading_mutex;
	GCond                 *start_loading_cond;

	LoaderFunc             loader;
	gpointer               loader_data;
	
	CopyData              *copy_data;
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

	g_mutex_lock (priv->data_mutex);
	if (priv->file != NULL) {
		file_data_unref (priv->file);
		priv->file = NULL;
	}	
	if (priv->pixbuf != NULL) {
		g_object_unref (G_OBJECT (priv->pixbuf));
		priv->pixbuf = NULL;
	}

	if (priv->animation != NULL) {
		g_object_unref (G_OBJECT (priv->animation));
		priv->animation = NULL;
	}
	g_mutex_unlock (priv->data_mutex);

	g_mutex_lock (priv->exit_thread_mutex);
	priv->exit_thread = TRUE;
	g_mutex_unlock (priv->exit_thread_mutex);

	g_mutex_lock (priv->start_loading_mutex);
	priv->start_loading = TRUE;
	g_cond_signal (priv->start_loading_cond);
	g_mutex_unlock (priv->start_loading_mutex);

	g_thread_join (priv->thread);

	g_cond_free  (priv->start_loading_cond);
	g_mutex_free (priv->data_mutex);
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

	if (priv->thumb_factory != NULL)
		g_object_unref (priv->thumb_factory);

	if (priv->idle_id != 0) {
		g_source_remove (priv->idle_id);
		priv->idle_id = 0;
	}

	if (priv->check_id != 0) {
		g_source_remove (priv->check_id);
		priv->check_id = 0;
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

	priv->thumb_factory = NULL;
	priv->pixbuf = NULL;
	priv->animation = NULL;
	priv->file = NULL;

	priv->done = FALSE;
	priv->error = FALSE;
	priv->loader_done = FALSE;
	priv->interrupted = FALSE;

	priv->priority = GNOME_VFS_PRIORITY_DEFAULT;

	priv->done_func = NULL;
	priv->done_func_data = NULL;

	priv->check_id = 0;
	priv->idle_id = 0;

	priv->data_mutex = g_mutex_new ();

	priv->exit_thread = FALSE;
	priv->exit_thread_mutex = g_mutex_new ();

	priv->start_loading = FALSE;
	priv->start_loading_mutex = g_mutex_new ();
	priv->start_loading_cond = g_cond_new ();

	priv->loader = NULL;
	priv->loader_data = NULL;

	/*priv->thread = g_thread_create (load_image_thread, il, TRUE, NULL);*/

	/* The g_thread_create function assigns a very large default stacksize for each
	   thread (10 MB on FC6), which is probably excessive. 16k seems to be
	   sufficient. To be conversative, we'll try 32k. Use g_thread_create_full to
	   manually specify a small stack size. See Bug 310749 - Memory usage.
	   This reduces the virtual memory requirements, and the "writeable/private"
	   figure reported by "pmap -d". */

	/* Update: 32k caused crashes with svg images. Boosting to 512k. Bug 410827. */

	priv->thread = g_thread_create_full (load_image_thread,
					     il,
					     524288,
					     TRUE,
					     TRUE,
					     G_THREAD_PRIORITY_NORMAL,
					     NULL);
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
image_loader_new (gboolean as_animation)
{
	ImageLoaderPrivateData *priv;
	ImageLoader            *il;
	int                     size;

	il = IMAGE_LOADER (g_object_new (IMAGE_LOADER_TYPE, NULL));
	priv = (ImageLoaderPrivateData*) il->priv;

	priv->as_animation = as_animation;
	image_loader_set_file (il, NULL);

	size = eel_gconf_get_integer (PREF_THUMBNAIL_SIZE, DEF_THUMB_SIZE);
	if (size <= THUMBNAIL_NORMAL_SIZE)
		priv->thumb_factory = gnome_thumbnail_factory_new (GNOME_THUMBNAIL_SIZE_NORMAL);
	else
		priv->thumb_factory = gnome_thumbnail_factory_new (GNOME_THUMBNAIL_SIZE_LARGE);

	return G_OBJECT (il);
}


void
image_loader_set_loader (ImageLoader *il,
			 LoaderFunc   loader,
			 gpointer     loader_data)
{
	g_return_if_fail (il != NULL);

	g_mutex_lock (il->priv->data_mutex);
	il->priv->loader = loader;
	il->priv->loader_data = loader_data;
	g_mutex_unlock (il->priv->data_mutex);
}


void
image_loader_set_file (ImageLoader *il,
		       FileData    *file)
{
	g_mutex_lock (il->priv->data_mutex);
	file_data_unref (il->priv->file);
	if (file != NULL)
		il->priv->file = file_data_dup (file);
	else
		il->priv->file = NULL;
	g_mutex_unlock (il->priv->data_mutex);
}


FileData *
image_loader_get_file (ImageLoader *il)
{
	FileData *file = NULL;
	
	g_mutex_lock (il->priv->data_mutex);
	if (il->priv->file != NULL)
		file = file_data_dup (il->priv->file);
	g_mutex_unlock (il->priv->data_mutex);	
	
	return file;
}


void
image_loader_set_path (ImageLoader *il,
		       const char  *path,
		       const char  *mime_type)
{
	FileData *file;
	
	file = file_data_new (path, NULL);
	if (mime_type != NULL)
		file->mime_type = get_static_string (mime_type);
	else
		file_data_update_mime_type (file, TRUE); /* FIXME: always fast mime type is not good */
	image_loader_set_file (il, file);
	file_data_unref (file);
}


void
image_loader_set_pixbuf (ImageLoader *il,
			 GdkPixbuf   *pixbuf)
{
	g_return_if_fail (il != NULL);

	g_mutex_lock (il->priv->data_mutex);
	if (il->priv->animation != NULL) {
		g_object_unref (il->priv->animation);
		il->priv->animation = NULL;
	}
	if (il->priv->pixbuf != NULL) {
		g_object_unref (il->priv->pixbuf);
		il->priv->pixbuf = NULL;
	}

	il->priv->pixbuf = pixbuf;
	if (pixbuf != NULL) {
		g_object_ref (pixbuf);
	}
	g_mutex_unlock (il->priv->data_mutex);
}


static void
image_loader_sync_pixbuf (ImageLoader *il)
{
	GdkPixbuf              *pixbuf;
	ImageLoaderPrivateData *priv;

	g_return_if_fail (il != NULL);

	priv = il->priv;

	g_mutex_lock (priv->data_mutex);

	if (priv->animation == NULL) {
		if (priv->pixbuf != NULL)
			g_object_unref (priv->pixbuf);
		priv->pixbuf = NULL;
		g_mutex_unlock (priv->data_mutex);
		return;
	}

	pixbuf = gdk_pixbuf_animation_get_static_image (priv->animation);
	g_object_ref (pixbuf);
	
	if (priv->pixbuf == pixbuf) {
		g_object_unref (pixbuf);
		g_mutex_unlock (priv->data_mutex);
		return;
	}

	if (priv->pixbuf != NULL) {
		g_object_unref (priv->pixbuf);
		priv->pixbuf = NULL;
	}

	if (pixbuf != NULL) {
		g_object_ref (pixbuf);
		priv->pixbuf = pixbuf;
	}

	g_object_unref (pixbuf);
	g_mutex_unlock (priv->data_mutex);
}


static void
image_loader_sync_pixbuf_from_loader (ImageLoader     *il,
				      GdkPixbufLoader *pb_loader)
{
	GdkPixbuf              *pixbuf;
	ImageLoaderPrivateData *priv;

	g_return_if_fail (il != NULL);

	priv = il->priv;

	g_mutex_lock (priv->data_mutex);

	if (priv->as_animation) {
		if (priv->animation != NULL)
			g_object_unref (priv->animation);
		priv->animation = gdk_pixbuf_loader_get_animation (pb_loader);

		if ((priv->animation != NULL)
		    && ! gdk_pixbuf_animation_is_static_image (priv->animation)) {
			g_object_ref (G_OBJECT (priv->animation));
			g_mutex_unlock (priv->data_mutex);
			return;
		} else
			priv->animation = NULL;
	}

	pixbuf = gdk_pixbuf_loader_get_pixbuf (pb_loader);
	g_object_ref (pixbuf);

	if (priv->pixbuf == pixbuf) {
		g_object_unref (priv->pixbuf);
		g_mutex_unlock (priv->data_mutex);
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

	g_mutex_unlock (priv->data_mutex);
}


static void
image_loader_interrupted (ImageLoader *il)
{
	ImageLoaderPrivateData *priv = il->priv;

	g_mutex_lock (priv->data_mutex);
	priv->error = TRUE;
	g_mutex_unlock (priv->data_mutex);

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

	g_mutex_lock (priv->data_mutex);
	priv->error = FALSE;
	g_mutex_unlock (priv->data_mutex);

	image_loader_stop_common (il, NULL, NULL, TRUE, TRUE);
}


static void
image_loader_error (ImageLoader *il)
{
	ImageLoaderPrivateData *priv = il->priv;

	g_mutex_lock (priv->data_mutex);
	priv->error = TRUE;
	g_mutex_unlock (priv->data_mutex);

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
		FileData *file;
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

		file = image_loader_get_file (il);

		G_LOCK (pixbuf_loader_lock);

		animation = NULL;
		if (file != NULL) {
			if (priv->loader != NULL)
				animation = (*priv->loader) (file, 
							     &error,
							     priv->thumb_factory,
							     priv->loader_data);
        		else
		                animation = gth_pixbuf_animation_new_from_file (file,
									        &error,
									        0,
									        0,
									        priv->thumb_factory);
		}

		G_UNLOCK (pixbuf_loader_lock);

		g_mutex_lock (priv->data_mutex);

		priv->loader_done = TRUE;
		if (priv->animation != NULL)
			g_object_unref (priv->animation);
		priv->animation = animation;
		if ((animation == NULL) || (error != NULL)) {
			priv->error = TRUE;
			priv->done = FALSE;
			if (error != NULL)
				g_clear_error (&error);
		} 
		else {
			priv->error = FALSE;
			priv->done = TRUE;
		}

		g_mutex_unlock (priv->data_mutex);

		file_data_unref (file);
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

	g_mutex_lock (priv->data_mutex);
	error = priv->error;
	priv->done = TRUE;
	g_mutex_unlock (priv->data_mutex);

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

	g_mutex_lock (priv->data_mutex);
	done = priv->done;
	error = priv->error;
	loader_done = priv->loader_done;
	g_mutex_unlock (priv->data_mutex);

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
image_loader_start__step3 (const char     *uri,
			   GError         *error,
			   gpointer        data)
{
	ImageLoader *il = data;
	
	g_return_if_fail (il != NULL);

	if (error) {
		image_loader_error (il);
		return;
	}

	g_mutex_lock (il->priv->data_mutex);
	il->priv->done = FALSE;
	il->priv->error = FALSE;
	il->priv->loader_done = FALSE;
	il->priv->loading = TRUE;
	if (il->priv->pixbuf != NULL) {
		g_object_unref (il->priv->pixbuf);
		il->priv->pixbuf = NULL;
	}
	if (il->priv->animation != NULL) {
		g_object_unref (il->priv->animation);
		il->priv->animation = NULL;
	}
	g_mutex_unlock (il->priv->data_mutex);

	g_mutex_lock (il->priv->start_loading_mutex);
	il->priv->start_loading = TRUE;
	g_cond_signal (il->priv->start_loading_cond);
	g_mutex_unlock (il->priv->start_loading_mutex);

	il->priv->check_id = g_timeout_add (REFRESH_RATE, check_thread, il);
}


static void
image_loader_start__step2 (ImageLoader *il)
{
	FileData *file;
	
	g_mutex_lock (il->priv->data_mutex);
	file = file_data_dup (il->priv->file);
	g_mutex_unlock (il->priv->data_mutex);
	
	if (is_local_file (file->path)) 
		image_loader_start__step3 (file->path, NULL, il);
	else
		copy_remote_file_to_cache (file, image_loader_start__step3, il);
	file_data_unref (file);
}


void
image_loader_start (ImageLoader *il)
{
	g_return_if_fail (il != NULL);

	g_mutex_lock (il->priv->data_mutex);
	if (il->priv->file == NULL) {
		g_mutex_unlock (il->priv->data_mutex);
		return;
	}
	g_mutex_unlock (il->priv->data_mutex);

	image_loader_stop_common (il,
				  (DoneFunc) image_loader_start__step2,
				  il,
				  FALSE,
				  TRUE);
}


/* -- image_loader_stop -- */


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

	if (priv->copy_data != NULL) {
		copy_data_cancel (priv->copy_data);
		priv->copy_data = NULL;
	}

	priv->done_func = done_func;
	priv->done_func_data = done_func_data;
	priv->emit_signal = emit_sig;

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

	g_mutex_lock (priv->data_mutex);
	priv->error = FALSE;
	g_mutex_unlock (priv->data_mutex);

	if (priv->loading) {
		priv->emit_signal = TRUE;
		priv->interrupted = TRUE;
		priv->done_func = done_func;
		priv->done_func_data = done_func_data;
	} 
	else
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

	g_mutex_lock (priv->data_mutex);
	priv->error = TRUE;
	g_mutex_unlock (priv->data_mutex);

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

	g_mutex_lock (priv->data_mutex);
	animation = priv->animation;
	if (animation != NULL)
		g_object_ref (animation);
	g_mutex_unlock (priv->data_mutex);

	return animation;
}


gint
image_loader_get_is_done (ImageLoader *il)
{
	ImageLoaderPrivateData *priv;
	gboolean                is_done;

	g_return_val_if_fail (il != NULL, 0);
	priv = il->priv;

	g_mutex_lock (priv->data_mutex);
	is_done = priv->done && priv->loader_done;
	g_mutex_unlock (priv->data_mutex);

	return is_done;
}


char *
image_loader_get_path (ImageLoader *il)
{
	char *path;

	g_return_val_if_fail (il != NULL, NULL);

	g_mutex_lock (il->priv->data_mutex);
	if (il->priv->file == NULL) {
		g_mutex_unlock (il->priv->data_mutex);
                return NULL;
	}
        path = g_strdup (il->priv->file->path);
	g_mutex_unlock (il->priv->data_mutex);

        return path;
}


void
image_loader_load_from_pixbuf_loader (ImageLoader *il,
				      GdkPixbufLoader *pixbuf_loader)
{
	gboolean error;

	g_return_if_fail (il != NULL);

	image_loader_sync_pixbuf_from_loader (il, pixbuf_loader);

	g_mutex_lock (il->priv->data_mutex);
	error = (il->priv->pixbuf == NULL) && (il->priv->animation == NULL);
	g_mutex_unlock (il->priv->data_mutex);

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

	g_mutex_lock (to->priv->data_mutex);
	g_mutex_lock (from->priv->data_mutex);

	if (to->priv->file != NULL) {
		file_data_unref (to->priv->file);
		to->priv->file = NULL;
	}
	if (from->priv->file != NULL)
		to->priv->file = file_data_dup (from->priv->file);

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

	error = (to->priv->pixbuf == NULL) && (to->priv->animation == NULL);

	g_mutex_unlock (to->priv->data_mutex);
	g_mutex_unlock (from->priv->data_mutex);

	if (error)
		g_signal_emit (G_OBJECT (to), image_loader_signals[IMAGE_ERROR], 0);
	else
		g_signal_emit (G_OBJECT (to), image_loader_signals[IMAGE_DONE], 0);
}

