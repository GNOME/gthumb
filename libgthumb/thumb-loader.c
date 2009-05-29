/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2001, 2003 Free Software Foundation, Inc.
 *  Copyright (C) 2006-2007 Hubert Figuiere <hub@figuiere.net>
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
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <gtk/gtk.h>
#include <libgnomeui/gnome-thumbnail.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "gthumb-init.h"
#include "thumb-loader.h"
#include "typedefs.h"
#include "thumb-cache.h"
#include "image-loader.h"
#include "pixbuf-utils.h"
#include "file-utils.h"
#include "glib-utils.h"
#include "gthumb-marshal.h"

#define DEFAULT_MAX_FILE_SIZE (4*1024*1024)

#define THUMBNAIL_LARGE_SIZE	256
#define THUMBNAIL_NORMAL_SIZE	128

struct _ThumbLoaderPrivateData
{
	FileData              *file;
	ImageLoader           *il;
	GnomeThumbnailFactory *thumb_factory;
	GdkPixbuf             *pixbuf;	   	 /* Contains the final (scaled
						  * if necessary) image when 
						  * done. */
	gboolean               use_cache : 1;
	gboolean               from_cache : 1;
	gboolean               save_thumbnails : 1;
	float                  percent_done;
	int                    max_w;
	int                    max_h;
	int                    cache_max_w;
	int                    cache_max_h;
	GnomeThumbnailSize     thumb_size;
	goffset                max_file_size;    /* If the file size is greater
					    	  * than this the thumbnail 
					    	  * will not be created, for
					    	  * functionality reasons. */
};


enum {
	THUMB_ERROR,
	THUMB_DONE,
	THUMB_PROGRESS,
	LAST_SIGNAL
};


static GObjectClass *parent_class = NULL;
static guint thumb_loader_signals[LAST_SIGNAL] = { 0 };

static void         thumb_loader_done_cb    (ImageLoader *il,
					     gpointer data);

static void         thumb_loader_error_cb   (ImageLoader *il,
					     gpointer data);

static gint         normalize_thumb         (gint *width,
					     gint *height,
					     gint max_w,
					     gint max_h,
					     gint cache_max_w,
					     gint cache_max_h);


static void
thumb_loader_finalize (GObject *object)
{
	ThumbLoader            *tl;
	ThumbLoaderPrivateData *priv;

	g_return_if_fail (object != NULL);
	g_return_if_fail (IS_THUMB_LOADER (object));

	tl = THUMB_LOADER (object);
	priv = tl->priv;

	if (priv->thumb_factory != NULL)
		g_object_unref (priv->thumb_factory);

	if (priv->pixbuf != NULL)
		g_object_unref (G_OBJECT (priv->pixbuf));

	g_object_unref (G_OBJECT (priv->il));

	file_data_unref (priv->file);

	g_free (priv);
	tl->priv = NULL;

	/* Chain up */
	G_OBJECT_CLASS (parent_class)->finalize (object);
}


static void
thumb_loader_class_init (ThumbLoaderClass *class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (class);

	parent_class = g_type_class_peek_parent (class);

	thumb_loader_signals[THUMB_ERROR] =
		g_signal_new ("thumb_error",
			      G_TYPE_FROM_CLASS (class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (ThumbLoaderClass, thumb_error),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__VOID,
			      G_TYPE_NONE,
			      0);
	thumb_loader_signals[THUMB_DONE] =
		g_signal_new ("thumb_done",
			      G_TYPE_FROM_CLASS (class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (ThumbLoaderClass, thumb_done),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__VOID,
			      G_TYPE_NONE,
			      0);
	thumb_loader_signals[THUMB_PROGRESS] =
		g_signal_new ("thumb_progress",
			      G_TYPE_FROM_CLASS (class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (ThumbLoaderClass, thumb_progress),
			      NULL, NULL,
			      gthumb_marshal_VOID__FLOAT,
			      G_TYPE_NONE,
			      1, G_TYPE_FLOAT);

	object_class->finalize = thumb_loader_finalize;

	class->thumb_error = NULL;
	class->thumb_done = NULL;
	class->thumb_progress = NULL;
}


static void
thumb_loader_init (ThumbLoader *tl)
{
	ThumbLoaderPrivateData *priv;

	tl->priv = g_new (ThumbLoaderPrivateData, 1);
	priv = tl->priv;

	priv->thumb_factory = NULL;
	priv->file = NULL;
	priv->pixbuf = NULL;
	priv->use_cache = TRUE;
	priv->save_thumbnails = TRUE;
	priv->from_cache = FALSE;
	priv->percent_done = 0.0;
	priv->max_file_size = 0;
}


GType
thumb_loader_get_type (void)
{
	static GType type = 0;

	if (! type) {
		GTypeInfo type_info = {
			sizeof (ThumbLoaderClass),
			NULL,
			NULL,
			(GClassInitFunc) thumb_loader_class_init,
			NULL,
			NULL,
			sizeof (ThumbLoader),
			0,
			(GInstanceInitFunc) thumb_loader_init
		};

		type = g_type_register_static (G_TYPE_OBJECT,
					       "ThumbLoader",
					       &type_info,
					       0);
	}

	return type;
}


static GdkPixbufAnimation*
thumb_loader (FileData               *file,
	      GError                **error,
	      GnomeThumbnailFactory  *thumb_factory,
	      gpointer                data)
{
	ThumbLoader *tl = data;

	return gth_pixbuf_animation_new_from_file (file,
						   error,
						   tl->priv->cache_max_w,
						   tl->priv->cache_max_h,
						   tl->priv->thumb_factory);
}


GObject*
thumb_loader_new (int width,
		  int height)
{
	ThumbLoaderPrivateData *priv;
	ThumbLoader *tl;

	tl = THUMB_LOADER (g_object_new (THUMB_LOADER_TYPE, NULL));
	priv = tl->priv;

	thumb_loader_set_thumb_size (tl, width, height);

	priv->il = IMAGE_LOADER (image_loader_new (FALSE));
	image_loader_set_loader (priv->il, thumb_loader, tl);
	g_signal_connect (G_OBJECT (priv->il),
			  "image_done",
			  G_CALLBACK (thumb_loader_done_cb),
			  tl);
	g_signal_connect (G_OBJECT (priv->il),
			  "image_error",
			  G_CALLBACK (thumb_loader_error_cb),
			  tl);

	return G_OBJECT (tl);
}


void
thumb_loader_set_thumb_size (ThumbLoader *tl,
			     int          width,
			     int          height)
{
	if (tl->priv->thumb_factory != NULL) {
		g_object_unref (tl->priv->thumb_factory);
		tl->priv->thumb_factory = NULL;
	}

	if ((width <= THUMBNAIL_NORMAL_SIZE) && (height <= THUMBNAIL_NORMAL_SIZE)) {
		tl->priv->cache_max_w = tl->priv->cache_max_h = THUMBNAIL_NORMAL_SIZE;
		tl->priv->thumb_size = GNOME_THUMBNAIL_SIZE_NORMAL;
	}
	else {
		tl->priv->cache_max_w = tl->priv->cache_max_h = THUMBNAIL_LARGE_SIZE;
		tl->priv->thumb_size = GNOME_THUMBNAIL_SIZE_LARGE;
	}

	tl->priv->thumb_factory = gnome_thumbnail_factory_new (tl->priv->thumb_size);
	
	tl->priv->max_w = width;
	tl->priv->max_h = height;
}


void
thumb_loader_use_cache (ThumbLoader *tl,
			gboolean use)
{
	g_return_if_fail (tl != NULL);
	tl->priv->use_cache = use;
}


void
thumb_loader_save_thumbnails (ThumbLoader *tl,
			      gboolean     save)
{
	g_return_if_fail (tl != NULL);
	tl->priv->save_thumbnails = save;
}


void
thumb_loader_set_max_file_size (ThumbLoader *tl,
				goffset      size)
{
	g_return_if_fail (tl != NULL);
	tl->priv->max_file_size = size;
}


void
thumb_loader_set_file (ThumbLoader *tl,
		       FileData    *fd)
{
	g_return_if_fail (tl != NULL);

	file_data_unref (tl->priv->file);
	tl->priv->file = NULL;
	
	if (fd != NULL) {
		tl->priv->file = file_data_dup (fd);
		if (is_local_file (tl->priv->file->utf8_path)) {
			char *resolved_path = NULL;
			resolved_path = resolve_all_symlinks (tl->priv->file->utf8_path);
			file_data_set_path (tl->priv->file, resolved_path);
			g_free (resolved_path);
		}
	}
	
	image_loader_set_file (tl->priv->il, tl->priv->file);
}


void
thumb_loader_set_path (ThumbLoader *tl,
		       const char  *path) /* FIXME: pass the mime_type too. */
{
	FileData *fd;
	
	g_return_if_fail (tl != NULL);
	g_return_if_fail (path != NULL);

	fd = file_data_new (path);
	file_data_update (fd); 
	thumb_loader_set_file (tl, fd);
}


GdkPixbuf *
thumb_loader_get_pixbuf (ThumbLoader *tl)
{
	g_return_val_if_fail (tl != NULL, NULL);
	return tl->priv->pixbuf;
}

	
static void
thumb_loader_start__step2 (ThumbLoader *tl)
{
	char *cache_path = NULL;

	g_return_if_fail (tl != NULL);
	
	if ((tl->priv->file == NULL) ||
	    (tl->priv->file->can_read == FALSE)) {
		g_signal_emit (G_OBJECT (tl),
			       thumb_loader_signals[THUMB_ERROR],
			       0);
		return;
	}
	
	if ((tl->priv->use_cache && ((time (NULL) - tl->priv->file->mtime) > (time_t) 5))) {
		cache_path = gnome_thumbnail_factory_lookup (tl->priv->thumb_factory,
							     tl->priv->file->uri,
							     tl->priv->file->mtime);

		if ((cache_path == NULL)  
		    && gnome_thumbnail_factory_has_valid_failed_thumbnail (tl->priv->thumb_factory,
									   tl->priv->file->uri,
									   tl->priv->file->mtime))
		{
			/* Use the existing "failed" thumbnail, if it is over
			   5 seconds old. Otherwise, try to thumbnail it again. 
			   The minimum age requirement addresses bug 432759, 
			   which occurs when a device like a scanner saves a file
			   slowly in chunks. */
			g_signal_emit (G_OBJECT (tl),
				       thumb_loader_signals[THUMB_ERROR],
				       0);
			return;
		}
	}

	if (cache_path != NULL) {
		tl->priv->from_cache = TRUE;
		image_loader_set_path (tl->priv->il, cache_path, "image/png");
		g_free (cache_path);
	} 
	else {
		tl->priv->from_cache = FALSE;
		image_loader_set_file (tl->priv->il, tl->priv->file);

		/* Check file dimensions. */

		if ((tl->priv->max_file_size > 0) && (tl->priv->file->size > tl->priv->max_file_size)) {
			if (tl->priv->pixbuf != NULL) {
				g_object_unref (tl->priv->pixbuf);
				tl->priv->pixbuf = NULL;
			}
			g_signal_emit (G_OBJECT (tl),
				       thumb_loader_signals[THUMB_DONE],
				       0);
			return;
		}
	}

	image_loader_start (tl->priv->il);
}


void
thumb_loader_start (ThumbLoader *tl)
{
	thumb_loader_stop (tl, (DoneFunc)thumb_loader_start__step2, tl);
}


void
thumb_loader_stop (ThumbLoader *tl,
		   DoneFunc     done_func,
		   gpointer     done_func_data)
{
	ThumbLoaderPrivateData *priv;

	g_return_if_fail (tl != NULL);
	priv = tl->priv;
	g_return_if_fail (priv->il != NULL);

	image_loader_stop (priv->il, done_func, done_func_data);
}


/* -- local functions -- */


static gint
thumb_loader_save_to_cache (ThumbLoader *tl)
{
	char *cache_file;
	char *cache_dir;
	
	if ((tl == NULL) || (tl->priv->pixbuf == NULL))
		return FALSE;

	if (is_local_file (tl->priv->file->utf8_path)) {
		char *cache_base_uri;

		/* Do not save thumbnails from the user's thumbnail directory,
		   or an endless loop of thumbnailing may be triggered. */
		   
		cache_base_uri = g_strconcat (get_home_uri (), "/.thumbnails", NULL);
		if (path_in_path (cache_base_uri, tl->priv->file->utf8_path)) { /* FIXME */
			g_free (cache_base_uri);	
			return FALSE;
		}
		g_free (cache_base_uri);
	}

	cache_file = gnome_thumbnail_path_for_uri (tl->priv->file->uri, tl->priv->thumb_size);
	if (cache_file == NULL)
		return FALSE;
		
	cache_dir = remove_level_from_path (cache_file);
	g_free (cache_file);

	if (ensure_dir_exists (cache_dir))	
		gnome_thumbnail_factory_save_thumbnail (tl->priv->thumb_factory,
							tl->priv->pixbuf,
							tl->priv->file->uri,
							tl->priv->file->mtime);
	g_free (cache_dir);

	return TRUE;
}


static void
thumb_loader_done_cb (ImageLoader *il,
		      gpointer     data)
{
	ThumbLoader            *tl = data;
	ThumbLoaderPrivateData *priv = tl->priv;
	GdkPixbuf              *pixbuf;
	int                     width, height;
	gboolean                modified;

	if (priv->pixbuf != NULL) {
		g_object_unref (priv->pixbuf);
		priv->pixbuf = NULL;
	}

	pixbuf = image_loader_get_pixbuf (priv->il);

	if (pixbuf == NULL) {
		gnome_thumbnail_factory_create_failed_thumbnail (priv->thumb_factory,
								 priv->file->uri,
								 priv->file->mtime);
		g_signal_emit (G_OBJECT (tl), thumb_loader_signals[THUMB_ERROR], 0);
		return;
	}

	priv->pixbuf = pixbuf;
	g_object_ref (pixbuf);

	width = gdk_pixbuf_get_width (pixbuf);
	height = gdk_pixbuf_get_height (pixbuf);

	if (priv->use_cache) {
		/* Thumbnails are always saved with the same size, then
		 * scaled if necessary. */

		/* Check whether to scale. */
		modified = scale_keeping_ratio (&width, &height,
						 priv->cache_max_w,
						 priv->cache_max_h,
						 FALSE);
		if (modified) {
			g_object_unref (priv->pixbuf);
			priv->pixbuf = gdk_pixbuf_scale_simple (pixbuf,
								width,
								height,
								GDK_INTERP_BILINEAR);
		}

		/* Save the thumbnail if necessary. */
		if (priv->save_thumbnails && ! priv->from_cache)
			thumb_loader_save_to_cache (tl);

		/* Scale if the user wants a different size. */
		modified = normalize_thumb (&width,
					    &height,
					    priv->max_w,
					    priv->max_h,
					    priv->cache_max_w,
					    priv->cache_max_h);
		if (modified) {
			pixbuf = priv->pixbuf;
			priv->pixbuf = gdk_pixbuf_scale_simple (pixbuf,
								width,
								height,
								GDK_INTERP_BILINEAR);
			g_object_unref (pixbuf);
		}
	} 
	else {
		modified = scale_keeping_ratio (&width, &height, priv->max_w, priv->max_h, FALSE);
		if (modified) {
			g_object_unref (priv->pixbuf);
			priv->pixbuf = gdk_pixbuf_scale_simple (pixbuf,
								width,
								height,
								GDK_INTERP_BILINEAR);
		}
	}

	g_signal_emit (G_OBJECT (tl), thumb_loader_signals[THUMB_DONE], 0);
}


static void
thumb_loader_error_cb (ImageLoader *il,
		       gpointer     data)
{
	ThumbLoader            *tl = data;
	ThumbLoaderPrivateData *priv = tl->priv;

	if (! priv->from_cache) {
		if (priv->pixbuf != NULL) {
			g_object_unref (priv->pixbuf);
			priv->pixbuf = NULL;
		}

		gnome_thumbnail_factory_create_failed_thumbnail (priv->thumb_factory,
								 priv->file->uri,
								 priv->file->mtime);

		g_signal_emit (G_OBJECT (tl), thumb_loader_signals[THUMB_ERROR], 0);

		return;
	}

	/* Try from the original if cache load attempt failed. */
	
	priv->from_cache = FALSE;

	image_loader_set_file (priv->il, priv->file);
	image_loader_start (priv->il);
}


static gint
normalize_thumb (gint *width,
		 gint *height,
		 gint max_width,
		 gint max_height,
		 gint cache_max_w,
		 gint cache_max_h)
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
	new_width  = MAX ((gint) (w * factor), 1);
	new_height = MAX ((gint) (h * factor), 1);

	modified = (new_width != *width) || (new_height != *height);

	*width = new_width;
	*height = new_height;

	return modified;
}
