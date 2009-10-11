/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2001-2008 Free Software Foundation, Inc.
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
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <signal.h>
#define GDK_PIXBUF_ENABLE_BACKEND
#include <gtk/gtk.h>
#include <gdk-pixbuf/gdk-pixbuf-animation.h>
#include "gio-utils.h"
#include "glib-utils.h"
#define GNOME_DESKTOP_USE_UNSTABLE_API
#include "gnome-desktop-thumbnail.h"
#include "gth-error.h"
#include "gth-image-loader.h"
#include "gth-main.h"
#include "gth-thumb-loader.h"
#include "pixbuf-io.h"
#include "pixbuf-utils.h"
#include "typedefs.h"

#define DEFAULT_MAX_FILE_SIZE     (4*1024*1024)
#define THUMBNAIL_LARGE_SIZE	  256
#define THUMBNAIL_NORMAL_SIZE	  128
#define THUMBNAIL_DIR_PERMISSIONS 0700
#define MAX_THUMBNAILER_LIFETIME  2000   /* kill the thumbnailer after this amount of time*/

struct _GthThumbLoaderPrivateData
{
	GthFileData      *file;
	GthImageLoader   *iloader;
	GdkPixbuf        *pixbuf;	   	 /* Contains the final (scaled
						  * if necessary) image when
						  * done. */
	guint             use_cache : 1;
	guint             from_cache : 1;
	guint             save_thumbnails : 1;
	int               max_w;
	int               max_h;
	int               cache_max_w;
	int               cache_max_h;
	goffset           max_file_size;         /* If the file size is greater
					    	  * than this the thumbnail
					    	  * will not be created, for
					    	  * functionality reasons. */

	GnomeDesktopThumbnailSize     thumb_size;
	GnomeDesktopThumbnailFactory *thumb_factory;
	char                         *thumbnailer_tmpfile;
	GPid                          thumbnailer_pid;
	guint                         thumbnailer_watch;
	guint                         thumbnailer_timeout;
};


enum {
	READY,
	LAST_SIGNAL
};


static GObjectClass *parent_class = NULL;
static guint gth_thumb_loader_signals[LAST_SIGNAL] = { 0 };


static void
gth_thumb_loader_finalize (GObject *object)
{
	GthThumbLoader *tloader;

	g_return_if_fail (object != NULL);
	g_return_if_fail (GTH_IS_THUMB_LOADER (object));

	tloader = GTH_THUMB_LOADER (object);

	if (tloader->priv != NULL) {
		g_free (tloader->priv->thumbnailer_tmpfile);
		_g_object_unref (tloader->priv->pixbuf);
		g_object_unref (tloader->priv->iloader);
		g_object_unref (tloader->priv->file);
		g_free (tloader->priv);
		tloader->priv = NULL;
	}

	/* Chain up */
	G_OBJECT_CLASS (parent_class)->finalize (object);
}


static void
gth_thumb_loader_class_init (GthThumbLoaderClass *class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (class);

	parent_class = g_type_class_peek_parent (class);

	object_class->finalize = gth_thumb_loader_finalize;

	gth_thumb_loader_signals[READY] =
		g_signal_new ("ready",
			      G_TYPE_FROM_CLASS (class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (GthThumbLoaderClass, ready),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__POINTER,
			      G_TYPE_NONE,
			      1,
			      G_TYPE_POINTER);
}


static void
gth_thumb_loader_init (GthThumbLoader *tloader)
{
	tloader->priv = g_new0 (GthThumbLoaderPrivateData, 1);
	tloader->priv->file = NULL;
	tloader->priv->pixbuf = NULL;
	tloader->priv->use_cache = TRUE;
	tloader->priv->save_thumbnails = TRUE;
	tloader->priv->from_cache = FALSE;
	tloader->priv->max_file_size = 0;
}


GType
gth_thumb_loader_get_type (void)
{
	static GType type = 0;

	if (! type) {
		GTypeInfo type_info = {
			sizeof (GthThumbLoaderClass),
			NULL,
			NULL,
			(GClassInitFunc) gth_thumb_loader_class_init,
			NULL,
			NULL,
			sizeof (GthThumbLoader),
			0,
			(GInstanceInitFunc) gth_thumb_loader_init
		};

		type = g_type_register_static (G_TYPE_OBJECT,
					       "GthThumbLoader",
					       &type_info,
					       0);
	}

	return type;
}


static int
normalize_thumb (int *width,
		 int *height,
		 int  max_width,
		 int  max_height,
		 int  cache_max_w,
		 int  cache_max_h)
{
	gboolean modified;
	float    max_w = max_width;
	float    max_h = max_height;
	float    w = *width;
	float    h = *height;
	float    factor;
	int      new_width, new_height;

	if ((max_width > cache_max_w) && (max_height > cache_max_h)) {
		if ((*width < cache_max_w - 1) && (*height < cache_max_h - 1))
			return FALSE;
	}
	else if ((*width < max_width - 1) && (*height < max_height - 1))
		return FALSE;

	factor = MIN (max_w / w, max_h / h);
	new_width  = MAX ((int) (w * factor), 1);
	new_height = MAX ((int) (h * factor), 1);

	modified = (new_width != *width) || (new_height != *height);

	*width = new_width;
	*height = new_height;

	return modified;
}


static gboolean
_gth_thumb_loader_save_to_cache (GthThumbLoader *tloader)
{
	char  *uri;
	char  *cache_path;
	GFile *cache_file;
	GFile *cache_dir;

	if ((tloader == NULL) || (tloader->priv->pixbuf == NULL))
		return FALSE;

	uri = g_file_get_uri (tloader->priv->file->file);

	if (g_file_is_native (tloader->priv->file->file)) {
		char *cache_base_uri;

		/* Do not save thumbnails from the user's thumbnail directory,
		   or an endless loop of thumbnailing may be triggered. */

		cache_base_uri = g_strconcat (get_home_uri (), "/.thumbnails", NULL);
		if (_g_uri_parent_of_uri (cache_base_uri, uri)) {
			g_free (cache_base_uri);
			g_free (uri);
			return FALSE;
		}
		g_free (cache_base_uri);
	}

	cache_path = gnome_desktop_thumbnail_path_for_uri (uri, tloader->priv->thumb_size);
	if (cache_path == NULL) {
		g_free (uri);
		return FALSE;
	}

	cache_file = g_file_new_for_path (cache_path);
	cache_dir = g_file_get_parent (cache_file);

	if (_g_directory_make (cache_dir, THUMBNAIL_DIR_PERMISSIONS, NULL)) {
		char *uri;

		uri = g_file_get_uri (tloader->priv->file->file);
		gnome_desktop_thumbnail_factory_save_thumbnail (tloader->priv->thumb_factory,
								tloader->priv->pixbuf,
								uri,
								gth_file_data_get_mtime (tloader->priv->file));
		g_free (uri);
	}

	g_object_unref (cache_dir);
	g_object_unref (cache_file);
	g_free (cache_path);
	g_free (uri);

	return TRUE;
}


static void
image_loader_loaded (GthImageLoader *iloader,
		     GdkPixbuf      *pixbuf,
		     gpointer        data)
{
	GthThumbLoader *tloader = data;
	int             width, height;
	gboolean        modified;

	if (tloader->priv->pixbuf != NULL) {
		g_object_unref (tloader->priv->pixbuf);
		tloader->priv->pixbuf = NULL;
	}

	if (pixbuf == NULL) {
		char *uri;

		uri = g_file_get_uri (tloader->priv->file->file);
		gnome_desktop_thumbnail_factory_create_failed_thumbnail (tloader->priv->thumb_factory,
									 uri,
									 gth_file_data_get_mtime (tloader->priv->file));
		g_signal_emit (G_OBJECT (tloader), gth_thumb_loader_signals[READY], 0, NULL);
		g_free (uri);

		return;
	}

	g_object_ref (pixbuf);
	tloader->priv->pixbuf = pixbuf;

	width = gdk_pixbuf_get_width (pixbuf);
	height = gdk_pixbuf_get_height (pixbuf);

	if (tloader->priv->use_cache) {
		/* Thumbnails are always saved with the same size, then
		 * scaled if necessary. */

		/* Check whether to scale. */
		modified = scale_keeping_ratio (&width,
						&height,
						tloader->priv->cache_max_w,
						tloader->priv->cache_max_h,
						FALSE);
		if (modified) {
			g_object_unref (tloader->priv->pixbuf);
			tloader->priv->pixbuf = gdk_pixbuf_scale_simple (pixbuf, width, height, GDK_INTERP_BILINEAR);
		}

		/* Save the thumbnail if necessary. */
		if (tloader->priv->save_thumbnails && ! tloader->priv->from_cache)
			_gth_thumb_loader_save_to_cache (tloader);

		/* Scale if the user wants a different size. */
		modified = normalize_thumb (&width,
					    &height,
					    tloader->priv->max_w,
					    tloader->priv->max_h,
					    tloader->priv->cache_max_w,
					    tloader->priv->cache_max_h);
		if (modified) {
			pixbuf = tloader->priv->pixbuf;
			tloader->priv->pixbuf = gdk_pixbuf_scale_simple (pixbuf, width, height, GDK_INTERP_BILINEAR);
			g_object_unref (pixbuf);
		}
	}
	else {
		modified = scale_keeping_ratio (&width, &height, tloader->priv->max_w, tloader->priv->max_h, FALSE);
		if (modified) {
			g_object_unref (tloader->priv->pixbuf);
			tloader->priv->pixbuf = gdk_pixbuf_scale_simple (pixbuf, width, height, GDK_INTERP_BILINEAR);
		}
	}

	g_signal_emit (G_OBJECT (tloader), gth_thumb_loader_signals[READY], 0, NULL);
}


static void
image_loader_error (GthImageLoader *iloader,
		    GError         *error,
		    gpointer        data)
{
	GthThumbLoader *tloader = data;

	g_return_if_fail (error != NULL);

	if (! tloader->priv->from_cache) {
		char *uri;

		if (tloader->priv->pixbuf != NULL) {
			g_object_unref (tloader->priv->pixbuf);
			tloader->priv->pixbuf = NULL;
		}

		uri = g_file_get_uri (tloader->priv->file->file);
		gnome_desktop_thumbnail_factory_create_failed_thumbnail (tloader->priv->thumb_factory,
									 uri,
									 gth_file_data_get_mtime (tloader->priv->file));
		g_free (uri);
		g_signal_emit (G_OBJECT (tloader), gth_thumb_loader_signals[READY], 0, error);
		return;
	}

	g_error_free (error);

	/* Try to load the original image if cache version failed. */

	tloader->priv->from_cache = FALSE;
	gth_image_loader_set_file_data (tloader->priv->iloader, tloader->priv->file);
	gth_image_loader_load (tloader->priv->iloader);
}


static gboolean
kill_thumbnailer_cb (gpointer data)
{
	GthThumbLoader *tloader = data;

	g_source_remove (tloader->priv->thumbnailer_timeout);
	tloader->priv->thumbnailer_timeout = 0;

	if (tloader->priv->thumbnailer_pid != 0) {
		/*g_source_remove (tloader->priv->thumbnailer_watch);
		tloader->priv->thumbnailer_watch = 0;*/
		kill (tloader->priv->thumbnailer_pid, SIGTERM);
		/*tloader->priv->thumbnailer_pid = 0;*/
	}

	return FALSE;
}


static void
watch_thumbnailer_cb (GPid     pid,
                      int      status,
                      gpointer data)
{
	GthThumbLoader *tloader = data;
	GdkPixbuf      *pixbuf;
	GError         *error;

	if (tloader->priv->thumbnailer_timeout != 0) {
		g_source_remove (tloader->priv->thumbnailer_timeout);
		tloader->priv->thumbnailer_timeout = 0;
	}

	g_spawn_close_pid (pid);
	tloader->priv->thumbnailer_pid = 0;
	tloader->priv->thumbnailer_watch = 0;

	if (status != 0) {
		error = g_error_new_literal (GTH_ERROR, 0, "cannot generate the thumbnail");
		image_loader_error (NULL, error, data);
		return;
	}

	pixbuf = gnome_desktop_thumbnail_factory_load_from_tempfile (tloader->priv->thumb_factory,
								     &tloader->priv->thumbnailer_tmpfile);
	if (pixbuf != NULL) {
		image_loader_loaded (NULL, pixbuf, data);
		g_object_unref (pixbuf);
	}
	else {
		error = g_error_new_literal (GTH_ERROR, 0, "cannot generate the thumbnail");
		image_loader_error (NULL, error, data);
	}
}


static void
image_loader_ready_cb (GthImageLoader *iloader,
		       GError         *error,
		       gpointer        data)
{
	GthThumbLoader *tloader = data;
	char           *uri;

	if (error == NULL) {
		image_loader_loaded (iloader, gth_image_loader_get_pixbuf (tloader->priv->iloader), data);
		return;
	}

	if (tloader->priv->from_cache) {
		image_loader_error (iloader, error, data);
		return;
	}

	/* try with the system thumbnailer as fallback */

	g_clear_error (&error);
	g_free (tloader->priv->thumbnailer_tmpfile);
	tloader->priv->thumbnailer_tmpfile = NULL;
	uri = g_file_get_uri (tloader->priv->file->file);
	if (gnome_desktop_thumbnail_factory_generate_thumbnail_async (tloader->priv->thumb_factory,
								      uri,
								      gth_file_data_get_mime_type (tloader->priv->file),
								      &tloader->priv->thumbnailer_pid,
								      &tloader->priv->thumbnailer_tmpfile,
								      &error))
	{
		tloader->priv->thumbnailer_watch = g_child_watch_add (tloader->priv->thumbnailer_pid,
								      watch_thumbnailer_cb,
								      tloader);
		tloader->priv->thumbnailer_timeout = g_timeout_add (MAX_THUMBNAILER_LIFETIME,
								    kill_thumbnailer_cb,
								    tloader);
	}
	else {
		if (error == NULL)
			error = g_error_new_literal (GTH_ERROR, 0, "cannot generate the thumbnail");
		image_loader_error (iloader, error, data);
	}

	g_free (uri);
}


static GdkPixbufAnimation*
thumb_loader (GthFileData  *file,
	      GError      **error,
	      gpointer      data)
{
	GthThumbLoader     *tloader = data;
	GdkPixbufAnimation *animation = NULL;
	GdkPixbuf          *pixbuf = NULL;

	if (tloader->priv->from_cache) {
		pixbuf = gth_pixbuf_new_from_file (file, error, -1, -1);
	}
	else {
		/* try with a custom thumbnailer first */

		if (! tloader->priv->from_cache) {
			FileLoader thumbnailer;

			thumbnailer = gth_main_get_file_loader (gth_file_data_get_mime_type (file));
			if (thumbnailer != NULL)
				animation = thumbnailer (file, error, tloader->priv->cache_max_w, tloader->priv->cache_max_h);

			if (animation != NULL)
				return animation;
		}
	}

	if (pixbuf != NULL) {
		g_clear_error (error);
		animation = gdk_pixbuf_non_anim_new (pixbuf);
		g_object_unref (pixbuf);
	}
	else
		*error = g_error_new_literal (GTH_ERROR, 0, "cannot generate the thumbnail");

	return animation;
}


static void
gth_thumb_loader_construct (GthThumbLoader *tloader,
			    int             width,
			    int             height)
{
	gth_thumb_loader_set_thumb_size (tloader, width, height);

	tloader->priv->iloader = gth_image_loader_new (FALSE);
	gth_image_loader_set_loader (tloader->priv->iloader, thumb_loader, tloader);
	g_signal_connect (G_OBJECT (tloader->priv->iloader),
			  "ready",
			  G_CALLBACK (image_loader_ready_cb),
			  tloader);
}


GthThumbLoader *
gth_thumb_loader_new (int width,
		      int height)
{
	GthThumbLoader *tloader;

	tloader = g_object_new (GTH_TYPE_THUMB_LOADER, NULL);
	gth_thumb_loader_construct (tloader, width, height);

	return tloader;
}


void
gth_thumb_loader_set_thumb_size (GthThumbLoader *tloader,
				 int             width,
				 int             height)
{
	if (tloader->priv->thumb_factory != NULL) {
		g_object_unref (tloader->priv->thumb_factory);
		tloader->priv->thumb_factory = NULL;
	}

	if ((width <= THUMBNAIL_NORMAL_SIZE) && (height <= THUMBNAIL_NORMAL_SIZE)) {
		tloader->priv->cache_max_w = tloader->priv->cache_max_h = THUMBNAIL_NORMAL_SIZE;
		tloader->priv->thumb_size = GNOME_DESKTOP_THUMBNAIL_SIZE_NORMAL;
	}
	else {
		tloader->priv->cache_max_w = tloader->priv->cache_max_h = THUMBNAIL_LARGE_SIZE;
		tloader->priv->thumb_size = GNOME_DESKTOP_THUMBNAIL_SIZE_LARGE;
	}

	tloader->priv->thumb_factory = gnome_desktop_thumbnail_factory_new (tloader->priv->thumb_size);

	tloader->priv->max_w = width;
	tloader->priv->max_h = height;
}


void
gth_thumb_loader_use_cache (GthThumbLoader *tloader,
			    gboolean        use)
{
	g_return_if_fail (tloader != NULL);
	tloader->priv->use_cache = use;
}


void
gth_thumb_loader_save_thumbnails (GthThumbLoader *tloader,
				  gboolean        save)
{
	g_return_if_fail (tloader != NULL);
	tloader->priv->save_thumbnails = save;
}


void
gth_thumb_loader_set_max_file_size (GthThumbLoader *tloader,
				    goffset         size)
{
	g_return_if_fail (tloader != NULL);
	tloader->priv->max_file_size = size;
}


void
gth_thumb_loader_set_file (GthThumbLoader *tloader,
			   GthFileData    *fd)
{
	g_return_if_fail (tloader != NULL);

	_g_object_unref (tloader->priv->file);
	tloader->priv->file = NULL;

	if (fd != NULL) {
		GFile  *real_file = NULL;
		GError *error = NULL;

		tloader->priv->file = gth_file_data_dup (fd);

		real_file = _g_file_resolve_all_symlinks (tloader->priv->file->file, &error);
		if (real_file == NULL) {
			g_warning ("%s", error->message);
			g_clear_error (&error);
			return;
		}

		gth_file_data_set_file (tloader->priv->file, real_file);

		g_object_unref (real_file);
	}

	gth_image_loader_set_file_data (tloader->priv->iloader, tloader->priv->file);
}


void
gth_thumb_loader_set_uri (GthThumbLoader *tloader,
			  const char     *uri,
			  const char     *mime_type)
{
	GFile       *file;
	GthFileData *fd;

	g_return_if_fail (tloader != NULL);
	g_return_if_fail (uri != NULL);

	file = g_file_new_for_uri (uri);
	fd = gth_file_data_new (file, NULL);
	gth_file_data_update_info (fd, NULL);
	gth_file_data_set_mime_type (fd, mime_type);

	gth_thumb_loader_set_file (tloader, fd);

	g_object_unref (file);
}


GdkPixbuf *
gth_thumb_loader_get_pixbuf (GthThumbLoader *tloader)
{
	g_return_val_if_fail (tloader != NULL, NULL);
	return tloader->priv->pixbuf;
}


static void
gth_thumb_loader_load__step2 (GthThumbLoader *tloader)
{
	char *cache_path = NULL;

	g_return_if_fail (tloader != NULL);

	if (tloader->priv->use_cache) {
		char   *uri;
		time_t  mtime;

		uri = g_file_get_uri (tloader->priv->file->file);
		mtime = gth_file_data_get_mtime (tloader->priv->file);
		cache_path = gnome_desktop_thumbnail_factory_lookup (tloader->priv->thumb_factory, uri, mtime);
		if ((cache_path == NULL)
		    && gnome_desktop_thumbnail_factory_has_valid_failed_thumbnail (tloader->priv->thumb_factory, uri, mtime))
		{
			g_signal_emit (G_OBJECT (tloader),
				       gth_thumb_loader_signals[READY],
				       0,
				       g_error_new_literal (GTH_ERROR, 0, "failed thumbnail"));
			g_free (uri);
			return;
		}
		g_free (uri);
	}

	if (cache_path != NULL) {
		GFile *file;

		tloader->priv->from_cache = TRUE;
		file = g_file_new_for_path (cache_path);
		gth_image_loader_set_file (tloader->priv->iloader, file, "image/png");

		g_object_unref (file);
		g_free (cache_path);
	}
	else {
		tloader->priv->from_cache = FALSE;
		gth_image_loader_set_file_data (tloader->priv->iloader, tloader->priv->file);

		/* Check file dimensions. */

		if ((tloader->priv->max_file_size > 0)
		    && (g_file_info_get_size (tloader->priv->file->info) > tloader->priv->max_file_size))
		{
			if (tloader->priv->pixbuf != NULL) {
				g_object_unref (tloader->priv->pixbuf);
				tloader->priv->pixbuf = NULL;
			}
			g_signal_emit (G_OBJECT (tloader),
				       gth_thumb_loader_signals[READY],
				       0,
				       NULL);
			return;
		}
	}

	gth_image_loader_load (tloader->priv->iloader);
}


void
gth_thumb_loader_load (GthThumbLoader *tloader)
{
	gth_thumb_loader_cancel (tloader, (DataFunc) gth_thumb_loader_load__step2, tloader);
}


void
gth_thumb_loader_cancel (GthThumbLoader *tloader,
			 DataFunc        done_func,
			 gpointer        done_func_data)
{
	g_return_if_fail (tloader->priv->iloader != NULL);

	if (tloader->priv->thumbnailer_timeout != 0) {
		g_source_remove (tloader->priv->thumbnailer_timeout);
		tloader->priv->thumbnailer_timeout = 0;
	}

	if (tloader->priv->thumbnailer_pid != 0) {
		g_source_remove (tloader->priv->thumbnailer_watch);
		tloader->priv->thumbnailer_watch = 0;
		kill (tloader->priv->thumbnailer_pid, SIGTERM);
		tloader->priv->thumbnailer_pid = 0;
	}

	gth_image_loader_cancel (tloader->priv->iloader, done_func, done_func_data);
}
