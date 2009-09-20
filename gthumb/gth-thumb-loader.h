/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2001, 2003, 2007, 2008 The Free Software Foundation, Inc.
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

#ifndef GTH_THUMB_LOADER_H
#define GTH_THUMB_LOADER_H

#include <glib.h>
#include <gdk/gdk.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include "gth-image-loader.h"
#include "gth-file-data.h"

G_BEGIN_DECLS

#define GTH_TYPE_THUMB_LOADER            (gth_thumb_loader_get_type ())
#define GTH_THUMB_LOADER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTH_TYPE_THUMB_LOADER, GthThumbLoader))
#define GTH_THUMB_LOADER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GTH_TYPE_THUMB_LOADER, GthThumbLoaderClass))
#define GTH_IS_THUMB_LOADER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTH_TYPE_THUMB_LOADER))
#define GTH_IS_THUMB_LOADER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GTH_TYPE_THUMB_LOADER))
#define GTH_THUMB_LOADER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), GTH_TYPE_THUMB_LOADER, GthThumbLoaderClass))

typedef struct _GthThumbLoader            GthThumbLoader;
typedef struct _GthThumbLoaderClass       GthThumbLoaderClass;
typedef struct _GthThumbLoaderPrivateData GthThumbLoaderPrivateData;

struct _GthThumbLoader
{
	GObject  __parent;
	GthThumbLoaderPrivateData *priv;
};

struct _GthThumbLoaderClass
{
	GObjectClass __parent_class;

	/* -- Signals -- */

	void (* ready) (GthThumbLoader *il,
			GError         *error);
};

GType            gth_thumb_loader_get_type           (void);
GthThumbLoader * gth_thumb_loader_new                (int             width,
					              int             height);
void             gth_thumb_loader_set_thumb_size     (GthThumbLoader *tl,
					              int             width,
					              int             height);
void             gth_thumb_loader_use_cache          (GthThumbLoader *tl,
					              gboolean        use);
void             gth_thumb_loader_save_thumbnails    (GthThumbLoader *tl,
					              gboolean        save);
void             gth_thumb_loader_set_max_file_size  (GthThumbLoader *tl,
					              goffset         size);
void             gth_thumb_loader_set_file           (GthThumbLoader *tl,
					              GthFileData    *fd);
void             gth_thumb_loader_set_uri            (GthThumbLoader *tl,
					              const char     *uri,
					              const char     *mime_type);
GdkPixbuf *      gth_thumb_loader_get_pixbuf         (GthThumbLoader *tl);
void             gth_thumb_loader_load               (GthThumbLoader *tl);
void             gth_thumb_loader_cancel             (GthThumbLoader *tl,
					              DataFunc        func,
					              gpointer        data);

G_END_DECLS

#endif /* GTH_THUMB_LOADER_H */
