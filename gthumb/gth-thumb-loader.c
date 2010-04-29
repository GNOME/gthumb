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
#include <gdk-pixbuf/gdk-pixbuf.h>
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
	GthFileData      *file_data;
	GthImageLoader   *iloader;
	GdkPixbuf        *pixbuf;	   	 /* Contains the final (scaled
						  * if necessary) image when
						  * done. */
	guint             use_cache : 1;
	guint             loading_from_cache : 1;
	guint             save_thumbnails : 1;
	int               requested_size;
	int               cache_max_size;
	goffset           max_file_size;         /* If the file size is greater
					    	  * than this the thumbnail
					    	  * will not be created, for
					    	  * functionality reasons. */
	GnomeDesktopThumbnailSize
			  thumb_size;
	GnomeDesktopThumbnailFactory
			 *thumb_factory;
	char             *thumbnailer_tmpfile;
	GPid              thumbnailer_pid;
	guint             thumbnailer_watch;
	guint             thumbnailer_timeout;
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
	GthThumbLoader *self;

	g_return_if_fail (object != NULL);
	g_return_if_fail (GTH_IS_THUMB_LOADER (object));

	self = GTH_THUMB_LOADER (object);

	if (self->priv != NULL) {
		g_free (self->priv->thumbnailer_tmpfile);
		_g_object_unref (self->priv->pixbuf);
		_g_object_unref (self->priv->iloader);
		_g_object_unref (self->priv->file_data);
		g_free (self->priv);
		self->priv = NULL;
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
gth_thumb_loader_init (GthThumbLoader *self)
{
	self->priv = g_new0 (GthThumbLoaderPrivateData, 1);
	self->priv->file_data = NULL;
	self->priv->pixbuf = NULL;
	self->priv->use_cache = TRUE;
	self->priv->save_thumbnails = TRUE;
	self->priv->loading_from_cache = FALSE;
	self->priv->max_file_size = 0;
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
		 int  max_size,
		 int  cache_max_size)
{
	gboolean modified;
	float    max_w = max_size;
	float    max_h = max_size;
	float    w = *width;
	float    h = *height;
	float    factor;
	int      new_width, new_height;

	if (max_size > cache_max_size) {
		if ((*width < cache_max_size - 1) && (*height < cache_max_size - 1))
			return FALSE;
	}
	else if ((*width < max_size - 1) && (*height < max_size - 1))
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
_gth_thumb_loader_save_to_cache (GthThumbLoader *self)
{
	char  *uri;
	char  *cache_path;
	GFile *cache_file;
	GFile *cache_dir;

	if ((self == NULL) || (self->priv->pixbuf == NULL))
		return FALSE;

	uri = g_file_get_uri (self->priv->file_data->file);

	if (g_file_is_native (self->priv->file_data->file)) {
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

	cache_path = gnome_desktop_thumbnail_path_for_uri (uri, self->priv->thumb_size);
	if (cache_path == NULL) {
		g_free (uri);
		return FALSE;
	}

	cache_file = g_file_new_for_path (cache_path);
	cache_dir = g_file_get_parent (cache_file);

	if (_g_directory_make (cache_dir, THUMBNAIL_DIR_PERMISSIONS, NULL)) {
		char *uri;

		uri = g_file_get_uri (self->priv->file_data->file);
		gnome_desktop_thumbnail_factory_save_thumbnail (self->priv->thumb_factory,
								self->priv->pixbuf,
								uri,
								gth_file_data_get_mtime (self->priv->file_data));
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
	GthThumbLoader *self = data;
	int             width, height;
	gboolean        modified;

	if (self->priv->pixbuf != NULL) {
		g_object_unref (self->priv->pixbuf);
		self->priv->pixbuf = NULL;
	}

	if (pixbuf == NULL) {
		char *uri;

		uri = g_file_get_uri (self->priv->file_data->file);
		gnome_desktop_thumbnail_factory_create_failed_thumbnail (self->priv->thumb_factory,
									 uri,
									 gth_file_data_get_mtime (self->priv->file_data));
		g_signal_emit (G_OBJECT (self), gth_thumb_loader_signals[READY], 0, NULL);
		g_free (uri);

		return;
	}

	g_object_ref (pixbuf);
	self->priv->pixbuf = pixbuf;

	width = gdk_pixbuf_get_width (pixbuf);
	height = gdk_pixbuf_get_height (pixbuf);

	if (self->priv->use_cache) {
		/* Thumbnails are always saved with the same size, then
		 * scaled if necessary. */

		/* Check whether to scale. */

		modified = scale_keeping_ratio (&width,
						&height,
						self->priv->cache_max_size,
						self->priv->cache_max_size,
						FALSE);
		if (modified) {
			g_object_unref (self->priv->pixbuf);
			self->priv->pixbuf = gdk_pixbuf_scale_simple (pixbuf, width, height, GDK_INTERP_BILINEAR);
		}

		/* Save the thumbnail if necessary. */

		if (self->priv->save_thumbnails && ! self->priv->loading_from_cache)
			_gth_thumb_loader_save_to_cache (self);

		/* Scale if the user wants a different size. */

		modified = normalize_thumb (&width,
					    &height,
					    self->priv->requested_size,
					    self->priv->cache_max_size);
		if (modified) {
			pixbuf = self->priv->pixbuf;
			self->priv->pixbuf = gdk_pixbuf_scale_simple (pixbuf, width, height, GDK_INTERP_BILINEAR);
			g_object_unref (pixbuf);
		}
	}
	else {
		modified = scale_keeping_ratio (&width,
						&height,
						self->priv->requested_size,
						self->priv->requested_size,
						FALSE);
		if (modified) {
			g_object_unref (self->priv->pixbuf);
			self->priv->pixbuf = gdk_pixbuf_scale_simple (pixbuf, width, height, GDK_INTERP_BILINEAR);
		}
	}

	g_signal_emit (G_OBJECT (self), gth_thumb_loader_signals[READY], 0, NULL);
}


static void
image_loader_error (GthImageLoader *iloader,
		    GError         *error,
		    gpointer        data)
{
	GthThumbLoader *self = data;

	g_return_if_fail (error != NULL);

	if (! self->priv->loading_from_cache) {
		char *uri;

		if (self->priv->pixbuf != NULL) {
			g_object_unref (self->priv->pixbuf);
			self->priv->pixbuf = NULL;
		}

		uri = g_file_get_uri (self->priv->file_data->file);
		gnome_desktop_thumbnail_factory_create_failed_thumbnail (self->priv->thumb_factory,
									 uri,
									 gth_file_data_get_mtime (self->priv->file_data));
		g_free (uri);
		g_signal_emit (G_OBJECT (self), gth_thumb_loader_signals[READY], 0, error);

		return;
	}

	/* ! loading_from_cache : try to load the original image if cache version failed. */

	g_error_free (error);
	self->priv->loading_from_cache = FALSE;
	gth_image_loader_set_file_data (self->priv->iloader, self->priv->file_data);
	gth_image_loader_load (self->priv->iloader);
}


static void
image_loader_ready_cb (GthImageLoader *iloader,
		       GError         *error,
		       gpointer        data)
{
	GthThumbLoader *self = data;

	if (error == NULL)
		image_loader_loaded (iloader, gth_image_loader_get_pixbuf (self->priv->iloader), data);
	else if (self->priv->loading_from_cache)
		gth_thumb_loader_load (self);
	else
		image_loader_error (iloader, error, data);
}


static GdkPixbufAnimation*
thumb_loader (GthFileData  *file_data,
	      GError      **error,
	      gpointer      data)
{
	GthThumbLoader     *self = data;
	GdkPixbuf          *pixbuf = NULL;
	GdkPixbufAnimation *animation;

	animation = NULL;
	if (! self->priv->loading_from_cache) {
		char *uri;

		uri = g_file_get_uri (file_data->file);
		pixbuf = gnome_desktop_thumbnail_factory_generate_no_script (self->priv->thumb_factory,
									     uri,
									     gth_file_data_get_mime_type (file_data));
		if (pixbuf == NULL) {
			PixbufLoader thumbnailer;

			thumbnailer = gth_main_get_pixbuf_loader (gth_file_data_get_mime_type (file_data));
			if (thumbnailer != NULL)
				animation = thumbnailer (file_data, self->priv->cache_max_size, error);
		}

		g_free (uri);
	}
	else
		pixbuf = gth_pixbuf_new_from_file (file_data, -1, error);

	if (pixbuf != NULL) {
		g_clear_error (error);
		animation = gdk_pixbuf_non_anim_new (pixbuf);
		g_object_unref (pixbuf);
	}

	if (animation == NULL)
		*error = g_error_new_literal (GTH_ERROR, 0, "Cannot generate the thumbnail");

	return animation;
}


static void
gth_thumb_loader_construct (GthThumbLoader *self,
			    int             size)
{
	gth_thumb_loader_set_requested_size (self, size);

	self->priv->iloader = gth_image_loader_new (FALSE);
	g_signal_connect (G_OBJECT (self->priv->iloader),
			  "ready",
			  G_CALLBACK (image_loader_ready_cb),
			  self);
	gth_image_loader_set_loader (self->priv->iloader, thumb_loader, self);
}


GthThumbLoader *
gth_thumb_loader_new (int size)
{
	GthThumbLoader *self;

	self = g_object_new (GTH_TYPE_THUMB_LOADER, NULL);
	gth_thumb_loader_construct (self, size);

	return self;
}


void
gth_thumb_loader_set_loader (GthThumbLoader *self,
			     LoaderFunc      loader)
{
	if (loader != NULL)
		gth_image_loader_set_loader (self->priv->iloader, loader, self);
	else
		gth_image_loader_set_loader (self->priv->iloader, thumb_loader, self);
}


void
gth_thumb_loader_set_requested_size (GthThumbLoader *self,
				     int             size)
{
	if (self->priv->thumb_factory != NULL) {
		g_object_unref (self->priv->thumb_factory);
		self->priv->thumb_factory = NULL;
	}

	self->priv->requested_size = size;
	if (self->priv->requested_size <= THUMBNAIL_NORMAL_SIZE) {
		self->priv->cache_max_size = THUMBNAIL_NORMAL_SIZE;
		self->priv->thumb_size = GNOME_DESKTOP_THUMBNAIL_SIZE_NORMAL;
	}
	else {
		self->priv->cache_max_size = THUMBNAIL_LARGE_SIZE;
		self->priv->thumb_size = GNOME_DESKTOP_THUMBNAIL_SIZE_LARGE;
	}
	self->priv->thumb_factory = gnome_desktop_thumbnail_factory_new (self->priv->thumb_size);
}


int
gth_thumb_loader_get_requested_size (GthThumbLoader *self)
{
	return self->priv->requested_size;
}


void
gth_thumb_loader_use_cache (GthThumbLoader *self,
			    gboolean        use)
{
	g_return_if_fail (self != NULL);
	self->priv->use_cache = use;
}


void
gth_thumb_loader_save_thumbnails (GthThumbLoader *self,
				  gboolean        save)
{
	g_return_if_fail (self != NULL);
	self->priv->save_thumbnails = save;
}


void
gth_thumb_loader_set_max_file_size (GthThumbLoader *self,
				    goffset         size)
{
	g_return_if_fail (self != NULL);
	self->priv->max_file_size = size;
}


void
gth_thumb_loader_set_file (GthThumbLoader *self,
			   GthFileData    *file_data)
{
	g_return_if_fail (self != NULL);

	_g_object_unref (self->priv->file_data);
	self->priv->file_data = NULL;

	if (file_data != NULL) {
		GFile  *real_file = NULL;
		GError *error = NULL;

		self->priv->file_data = gth_file_data_dup (file_data);

		real_file = _g_file_resolve_all_symlinks (self->priv->file_data->file, &error);
		if (real_file == NULL) {
			g_warning ("%s", error->message);
			g_clear_error (&error);
			return;
		}

		gth_file_data_set_file (self->priv->file_data, real_file);

		g_object_unref (real_file);
	}

	gth_image_loader_set_file_data (self->priv->iloader, self->priv->file_data);
}


void
gth_thumb_loader_set_uri (GthThumbLoader *self,
			  const char     *uri,
			  const char     *mime_type)
{
	GFile       *file;
	GthFileData *file_data;

	g_return_if_fail (self != NULL);
	g_return_if_fail (uri != NULL);

	file = g_file_new_for_uri (uri);
	file_data = gth_file_data_new (file, NULL);
	gth_file_data_update_info (file_data, NULL);
	gth_file_data_set_mime_type (file_data, mime_type);

	gth_thumb_loader_set_file (self, file_data);

	g_object_unref (file);
}


GdkPixbuf *
gth_thumb_loader_get_pixbuf (GthThumbLoader *self)
{
	g_return_val_if_fail (self != NULL, NULL);
	return self->priv->pixbuf;
}


static gboolean
kill_thumbnailer_cb (gpointer data)
{
	GthThumbLoader *self = data;

	if (self->priv->thumbnailer_timeout != 0) {
		g_source_remove (self->priv->thumbnailer_timeout);
		self->priv->thumbnailer_timeout = 0;
	}

	if (self->priv->thumbnailer_pid != 0) {
		/*g_source_remove (self->priv->thumbnailer_watch);
		self->priv->thumbnailer_watch = 0;*/
		kill (self->priv->thumbnailer_pid, SIGTERM);
		/*self->priv->thumbnailer_pid = 0;*/
	}

	return FALSE;
}


static void
watch_thumbnailer_cb (GPid     pid,
		      int      status,
		      gpointer data)
{
	GthThumbLoader *self = data;
	GdkPixbuf      *pixbuf = NULL;

	if (self->priv->thumbnailer_timeout != 0) {
		g_source_remove (self->priv->thumbnailer_timeout);
		self->priv->thumbnailer_timeout = 0;
	}

	g_spawn_close_pid (pid);
	self->priv->thumbnailer_pid = 0;
	self->priv->thumbnailer_watch = 0;

	if (status == 0)
		pixbuf = gnome_desktop_thumbnail_factory_load_from_tempfile (self->priv->thumb_factory,
									     &self->priv->thumbnailer_tmpfile);

	if (pixbuf != NULL) {
		image_loader_loaded (NULL, pixbuf, data);
		g_object_unref (pixbuf);
	}
	else {
		/* the system thumbnailer couldn't generate the thumbnail,
		 * try using the thumb_loader() function */
		gth_image_loader_load (self->priv->iloader);
	}
}


static void
gth_thumb_loader_load__step2 (GthThumbLoader *self)
{
	char *cache_path = NULL;
	char *uri;

	g_return_if_fail (self != NULL);

	if (self->priv->use_cache) {
		time_t mtime;

		uri = g_file_get_uri (self->priv->file_data->file);
		mtime = gth_file_data_get_mtime (self->priv->file_data);
		cache_path = gnome_desktop_thumbnail_factory_lookup (self->priv->thumb_factory, uri, mtime);
		if ((cache_path == NULL)
		    && gnome_desktop_thumbnail_factory_has_valid_failed_thumbnail (self->priv->thumb_factory, uri, mtime))
		{
			g_signal_emit (G_OBJECT (self),
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

		self->priv->loading_from_cache = TRUE;
		file = g_file_new_for_path (cache_path);
		gth_image_loader_set_file (self->priv->iloader, file, "image/png");

		g_object_unref (file);
		g_free (cache_path);
	}
	else {
		self->priv->loading_from_cache = FALSE;
		gth_image_loader_set_file_data (self->priv->iloader, self->priv->file_data);

		/* Check file dimensions. */

		if ((self->priv->max_file_size > 0)
		    && (g_file_info_get_size (self->priv->file_data->info) > self->priv->max_file_size))
		{
			_g_clear_object (&self->priv->pixbuf);
			g_signal_emit (G_OBJECT (self),
				       gth_thumb_loader_signals[READY],
				       0,
				       NULL);
			return;
		}
	}

	if (self->priv->loading_from_cache) {
		gth_image_loader_load (self->priv->iloader);
		return;
	}

	/* not loading from the cache: try with the system thumbnailer first */

	g_free (self->priv->thumbnailer_tmpfile);
	self->priv->thumbnailer_tmpfile = NULL;
	uri = g_file_get_uri (self->priv->file_data->file);
	if (gnome_desktop_thumbnail_factory_generate_from_script (self->priv->thumb_factory,
								  uri,
								  gth_file_data_get_mime_type (self->priv->file_data),
								  &self->priv->thumbnailer_pid,
								  &self->priv->thumbnailer_tmpfile,
								  NULL))
	{
		self->priv->thumbnailer_watch = g_child_watch_add (self->priv->thumbnailer_pid,
								   watch_thumbnailer_cb,
								   self);
		self->priv->thumbnailer_timeout = g_timeout_add (MAX_THUMBNAILER_LIFETIME,
								 kill_thumbnailer_cb,
								 self);
	}
	else /* if the system thumbnailer cannot generate the thumbnail,
	      * try using the thumb_loader() function */
		gth_image_loader_load (self->priv->iloader);

	g_free (uri);
}


void
gth_thumb_loader_load (GthThumbLoader *self)
{
	gth_thumb_loader_cancel (self, (DataFunc) gth_thumb_loader_load__step2, self);
}


void
gth_thumb_loader_cancel (GthThumbLoader *self,
			 DataFunc        done_func,
			 gpointer        done_func_data)
{
	g_return_if_fail (self->priv->iloader != NULL);

	if (self->priv->thumbnailer_watch != 0) {
		/* kill the thumbnailer script */
		g_source_remove (self->priv->thumbnailer_watch);
		self->priv->thumbnailer_watch = 0;
		kill_thumbnailer_cb (self);
	}

	gth_image_loader_cancel (self->priv->iloader, done_func, done_func_data);
}
