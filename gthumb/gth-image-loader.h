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

#ifndef GTH_IMAGE_LOADER_H
#define GTH_IMAGE_LOADER_H

#include <glib.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include "typedefs.h"
#include "gth-file-data.h"

G_BEGIN_DECLS

#define GTH_TYPE_IMAGE_LOADER            (gth_image_loader_get_type ())
#define GTH_IMAGE_LOADER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTH_TYPE_IMAGE_LOADER, GthImageLoader))
#define GTH_IMAGE_LOADER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GTH_TYPE_IMAGE_LOADER, GthImageLoaderClass))
#define GTH_IS_IMAGE_LOADER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTH_TYPE_IMAGE_LOADER))
#define GTH_IS_IMAGE_LOADER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GTH_TYPE_IMAGE_LOADER))
#define GTH_IMAGE_LOADER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), GTH_TYPE_IMAGE_LOADER, GthImageLoaderClass))

typedef struct _GthImageLoader        GthImageLoader;
typedef struct _GthImageLoaderClass   GthImageLoaderClass;
typedef struct _GthImageLoaderPrivate GthImageLoaderPrivate;

struct _GthImageLoader
{
	GObject __parent;
	GthImageLoaderPrivate *priv;
};

struct _GthImageLoaderClass
{
	GObjectClass __parent_class;

	/* -- Signals -- */

	void (* ready) (GthImageLoader *iloader,
			GError         *error);
};

typedef GdkPixbufAnimation * (*LoaderFunc) (GthFileData *file, GError **error, gpointer data);

GType                gth_image_loader_get_type                (void);
GthImageLoader *     gth_image_loader_new                     (gboolean           as_animation);
void                 gth_image_loader_set_loader              (GthImageLoader    *iloader,
						               LoaderFunc         loader,
						               gpointer           data);
void                 gth_image_loader_set_file_data           (GthImageLoader    *iloader,
						               GthFileData       *file);
void                 gth_image_loader_set_file                (GthImageLoader    *iloader,
							       GFile             *file,
							       const char        *mime_type);               
GthFileData *        gth_image_loader_get_file                (GthImageLoader    *iloader);
void                 gth_image_loader_set_pixbuf              (GthImageLoader    *iloader,
						               GdkPixbuf         *pixbuf);
GdkPixbuf *          gth_image_loader_get_pixbuf              (GthImageLoader    *iloader);
GdkPixbufAnimation * gth_image_loader_get_animation           (GthImageLoader    *iloader);
gboolean             gth_image_loader_is_ready                (GthImageLoader    *iloader);
void                 gth_image_loader_load                    (GthImageLoader    *iloader);
void                 gth_image_loader_cancel                  (GthImageLoader    *iloader,
						               DataFunc           done_func,
						               gpointer           done_func_data);
void                 gth_image_loader_cancel_with_error       (GthImageLoader    *iloader,
						               DataFunc           done_func,
						               gpointer           done_func_data);
void                 gth_image_loader_load_from_pixbuf_loader (GthImageLoader    *iloader,
				                               GdkPixbufLoader   *pixbuf_loader);						               
void                 gth_image_loader_load_from_image_loader  (GthImageLoader    *to,
							       GthImageLoader    *from);

G_END_DECLS

#endif /* GTH_IMAGE_LOADER_H */
