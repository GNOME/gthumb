/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2001, 2003 Free Software Foundation, Inc.
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
#include <gtk/gtkmain.h>
#include <libgnome/libgnome.h>
#include <libgnomeui/gnome-thumbnail.h>
#include <libgnomevfs/gnome-vfs-uri.h>
#include <libgnomevfs/gnome-vfs-utils.h>
#include <libgnomevfs/gnome-vfs-ops.h>
#define GDK_PIXBUF_ENABLE_BACKEND
#include <gdk-pixbuf/gdk-pixbuf-animation.h>

#include "gthumb-init.h"
#include "thumb-loader.h"
#include "typedefs.h"
#include "thumb-cache.h"
#include "image-loader.h"
#include "pixbuf-utils.h"
#include "file-utils.h"
#include "gconf-utils.h"
#include "jpeg-utils.h"
#include "gthumb-marshal.h"


#define DEFAULT_MAX_FILE_SIZE (4*1024*1024)

#define CACHE_MAX_W   128
#define CACHE_MAX_H   128

#define THUMBNAIL_DIR_PERMISSIONS 0700

typedef struct
{
	ImageLoader *il;

	GnomeThumbnailFactory *thumb_factory;

	GdkPixbuf *pixbuf;	   /* Contains final (scaled if necessary) 
				    * image when done */

	char *uri;
	char *path;

	gboolean use_cache : 1;
	gboolean from_cache : 1;

	float percent_done;

	int max_w;
	int max_h;

	GnomeVFSFileSize max_file_size;    /* If the file size is greater 
					    * than this the thumbnail will 
					    * not be created, for 
					    * functionality reasons. */
} ThumbLoaderPrivateData;


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
					     gint max_h);


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

	g_free (priv->uri);
	g_free (priv->path);

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
			      gthumb_marshal_VOID__VOID,
			      G_TYPE_NONE, 
			      0);
	thumb_loader_signals[THUMB_DONE] =
		g_signal_new ("thumb_done",
			      G_TYPE_FROM_CLASS (class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (ThumbLoaderClass, thumb_done),
			      NULL, NULL,
			      gthumb_marshal_VOID__VOID,
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

	priv->thumb_factory = gnome_thumbnail_factory_new (GNOME_THUMBNAIL_SIZE_NORMAL);

	priv->uri = NULL;
	priv->path = NULL;
	priv->pixbuf = NULL;
	priv->use_cache = TRUE;
	priv->from_cache = FALSE;
	priv->percent_done = 0.0;
	priv->max_file_size = 0;
}


GType
thumb_loader_get_type ()
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
thumb_loader (const char  *path,
	      GError     **error,
	      gpointer     data)
{
	ThumbLoader        *tl = data;
	GdkPixbufAnimation *animation = NULL;

#ifdef HAVE_LIBJPEG
	if (image_is_jpeg (path)) {
		ThumbLoaderPrivateData *priv = tl->priv;
		GdkPixbuf *pixbuf;

		pixbuf = f_load_scaled_jpeg (path, 
					     priv->max_w, 
					     priv->max_h, 
					     NULL, NULL);
		if (pixbuf != NULL) {
			animation = gdk_pixbuf_non_anim_new (pixbuf);
			g_object_unref (pixbuf);

			if (animation == NULL)
				g_print ("ANIMATION == NULL\n");

		} else
			g_print ("PIXBUF == NULL\n");

	} else
#endif
		animation = gdk_pixbuf_animation_new_from_file (path, error);

	return animation;
}


GObject*     
thumb_loader_new (const char *path, 
		  int width, 
		  int height)
{
	ThumbLoaderPrivateData *priv;
	ThumbLoader *tl;

	tl = THUMB_LOADER (g_object_new (THUMB_LOADER_TYPE, NULL));
	priv = tl->priv;

	priv->max_w = width;
	priv->max_h = height;

	if (path != NULL) 
		thumb_loader_set_path (tl, path);
	else {
		priv->uri = NULL;
		priv->path = NULL;
	}

	priv->il = IMAGE_LOADER (image_loader_new (path, FALSE));

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
thumb_loader_use_cache (ThumbLoader *tl,
			gboolean use)
{
	ThumbLoaderPrivateData *priv;

	g_return_if_fail (tl != NULL);

	priv = tl->priv;
	priv->use_cache = use;
}


void
thumb_loader_set_max_file_size (ThumbLoader      *tl,
				GnomeVFSFileSize  size)
{
	ThumbLoaderPrivateData *priv;

	g_return_if_fail (tl != NULL);

	priv = tl->priv;
	priv->max_file_size = size;
}


GnomeVFSFileSize
thumb_loader_get_max_file_size (ThumbLoader      *tl)
{
	ThumbLoaderPrivateData *priv;

	g_return_val_if_fail (tl != NULL, 0);

	priv = tl->priv;

	return priv->max_file_size;
}


void
thumb_loader_set_path (ThumbLoader *tl,
		       const char  *path)
{
	ThumbLoaderPrivateData *priv;
	char        *escaped_path;
	GnomeVFSURI *vfs_uri;

	g_return_if_fail (tl != NULL);
	g_return_if_fail (path != NULL);

	priv = tl->priv;
	g_free (priv->uri);
	g_free (priv->path);

	escaped_path = escape_uri (path);
	vfs_uri = gnome_vfs_uri_new (escaped_path);
	g_free (escaped_path);

	escaped_path = gnome_vfs_uri_to_string (vfs_uri, GNOME_VFS_URI_HIDE_NONE);
	priv->uri = gnome_vfs_unescape_string (escaped_path, NULL);
	g_free (escaped_path);

	escaped_path = gnome_vfs_uri_to_string (vfs_uri, GNOME_VFS_URI_HIDE_TOPLEVEL_METHOD);
	priv->path = gnome_vfs_unescape_string (escaped_path, NULL);
	g_free (escaped_path);

	gnome_vfs_uri_unref (vfs_uri);

	image_loader_set_path (priv->il, priv->path);
}


void
thumb_loader_set_uri (ThumbLoader       *tl,
		      const GnomeVFSURI *vfs_uri)
{
	ThumbLoaderPrivateData *priv;

	g_return_if_fail (tl != NULL);
	g_return_if_fail (vfs_uri != NULL);

	priv = tl->priv;
	g_free (priv->uri);
	g_free (priv->path);

	priv->uri = gnome_vfs_uri_to_string (vfs_uri, GNOME_VFS_URI_HIDE_NONE);
	priv->path = gnome_vfs_uri_to_string (vfs_uri, GNOME_VFS_URI_HIDE_TOPLEVEL_METHOD);

	image_loader_set_uri (priv->il, vfs_uri);
}


GnomeVFSURI *
thumb_loader_get_uri (ThumbLoader *tl)
{
	ThumbLoaderPrivateData *priv;
	GnomeVFSURI *uri;
	char        *escaped_path;

	g_return_val_if_fail (tl != NULL, NULL);

	priv = tl->priv;

	escaped_path = escape_uri (priv->path);
	uri = gnome_vfs_uri_new (escaped_path);
	g_free (escaped_path);

	return uri;
}


GdkPixbuf *
thumb_loader_get_pixbuf (ThumbLoader *tl)
{
	ThumbLoaderPrivateData *priv;

	g_return_val_if_fail (tl != NULL, NULL);

	priv = tl->priv;
	return priv->pixbuf;
}


ImageLoader *
thumb_loader_get_image_loader (ThumbLoader *tl)
{
	ThumbLoaderPrivateData *priv;

	g_return_val_if_fail (tl != NULL, NULL);

	priv = tl->priv;
	return priv->il;
}


char *
thumb_loader_get_path (ThumbLoader *tl)
{
	ThumbLoaderPrivateData *priv;

	g_return_val_if_fail (tl != NULL, NULL);

	priv = tl->priv;
	return priv->path;
}


void 
thumb_loader_start (ThumbLoader *tl)
{
	ThumbLoaderPrivateData *priv;
	char *cache_path = NULL;

	g_return_if_fail (tl != NULL);

	priv = tl->priv;

	g_return_if_fail (priv->path != NULL);

	if (priv->use_cache) {
		time_t  mtime;

		mtime = get_file_mtime (priv->path);
		if (gnome_thumbnail_factory_has_valid_failed_thumbnail (priv->thumb_factory, priv->uri, mtime)) {
			g_signal_emit (G_OBJECT (tl),
				       thumb_loader_signals[THUMB_ERROR],
				       0);
			return;
		}
		
		cache_path = gnome_thumbnail_factory_lookup (priv->thumb_factory, priv->uri, mtime);
	}

	if (cache_path != NULL) {
		priv->from_cache = TRUE;
		image_loader_set_path (priv->il, cache_path);
		g_free (cache_path);

	} else {
		priv->from_cache = FALSE;
		image_loader_set_path (priv->il, priv->path);

		/* Check file dimensions. */

		if ((priv->max_file_size != 0)
		    && get_file_size (priv->path) > priv->max_file_size) {
			if (priv->pixbuf != NULL) {
				g_object_unref (priv->pixbuf);
				priv->pixbuf = NULL;
			}
			g_signal_emit (G_OBJECT (tl), 
				       thumb_loader_signals[THUMB_DONE], 
				       0);
			return;
		}
	}

	image_loader_start (priv->il);
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


int 
thumb_from_xpm_d (const char **data, 
		  int          max_w, 
		  int          max_h, 
		  GdkPixmap  **pixmap, 
		  GdkBitmap  **mask)
{
	GdkPixbuf *pixbuf;
	int        w, h;

	pixbuf = gdk_pixbuf_new_from_xpm_data (data);
	w = gdk_pixbuf_get_width (pixbuf);
	h = gdk_pixbuf_get_height (pixbuf);

	if (scale_keepping_ratio (&w, &h, max_w, max_h)) {
		/* Scale */
		GdkPixbuf *tmp;
		
		tmp = pixbuf;
		pixbuf = gdk_pixbuf_scale_simple (tmp, w, h, GDK_INTERP_BILINEAR);
		g_object_unref (tmp);
	}

	gdk_pixbuf_render_pixmap_and_mask (pixbuf, pixmap, mask, 127);
	g_object_unref (pixbuf);

	return w;
}





/* -- local functions -- */


static gint 
thumb_loader_save_to_cache (ThumbLoader *tl)
{
	ThumbLoaderPrivateData *priv;
	time_t                  mtime;
	char                   *cache_dir;
	char                   *cache_path;

	if (tl == NULL) 
		return FALSE;

	priv = tl->priv;
	if (priv->pixbuf == NULL) 
		return FALSE;

	mtime = get_file_mtime (priv->path);
	
	cache_path = gnome_thumbnail_path_for_uri (priv->uri, GNOME_THUMBNAIL_SIZE_NORMAL);
	cache_dir = remove_level_from_path (cache_path);
	g_free (cache_path);

	if (cache_dir == NULL)
		return FALSE;

	if (ensure_dir_exists (cache_dir, THUMBNAIL_DIR_PERMISSIONS)) 
		gnome_thumbnail_factory_save_thumbnail (priv->thumb_factory,
							priv->pixbuf,
							priv->uri,
							mtime);

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
								 priv->uri,
								 get_file_mtime (priv->path));
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
		modified = scale_keepping_ratio (&width, &height, 
						 CACHE_MAX_W, CACHE_MAX_H);
		if (modified) {
			g_object_unref (priv->pixbuf);
			priv->pixbuf = gdk_pixbuf_scale_simple (pixbuf, 
								width, 
								height,
								GDK_INTERP_BILINEAR);
		}
		
		/* Save the thumbnail if necessary. */
		if (! priv->from_cache
		    && eel_gconf_get_boolean (PREF_SAVE_THUMBNAILS, TRUE))
			thumb_loader_save_to_cache (tl);
		
		/* Scale if the user wants a different size. */
		modified = normalize_thumb (&width, 
					    &height,
					    priv->max_w, 
					    priv->max_h);
		if (modified) {
			pixbuf = priv->pixbuf;
			priv->pixbuf = gdk_pixbuf_scale_simple (pixbuf, 
								width, 
								height, 
								GDK_INTERP_BILINEAR);
			g_object_unref (pixbuf);
		}
	} else {
		modified = scale_keepping_ratio (&width, &height, priv->max_w, priv->max_h);
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

		gnome_thumbnail_factory_create_failed_thumbnail (priv->thumb_factory, priv->uri, get_file_mtime (priv->path));
		g_signal_emit (G_OBJECT (tl), thumb_loader_signals[THUMB_ERROR], 0);

		return;
	}

	/* Try from the original if cache load attempt failed. */
	priv->from_cache = FALSE;
	g_warning ("Thumbnail image in cache failed to load, trying to recreate.");
	
	image_loader_set_path (priv->il, priv->path);
	image_loader_start (priv->il);
}


static gint 
normalize_thumb (gint *width, 
		 gint *height, 
		 gint max_width, 
		 gint max_height)
{
	gboolean modified;
	float    max_w = max_width;
	float    max_h = max_height;
	float    w = *width;
	float    h = *height;
	float    factor;
	int      new_width, new_height;

	if ((max_width > CACHE_MAX_W) && (max_height > CACHE_MAX_H)) {
		if ((*width < CACHE_MAX_W - 1) && (*height < CACHE_MAX_H - 1)) 
			return FALSE;
	} else if ((*width < max_width - 1) && (*height < max_height - 1)) 
		return FALSE;

	factor = MIN (max_w / w, max_h / h);
	new_width  = MAX ((gint) (w * factor - 0.5), 1);
	new_height = MAX ((gint) (h * factor - 0.5), 1);
	
	modified = (new_width != *width) || (new_height != *height);

	*width = new_width;
	*height = new_height;

	return modified;
}
