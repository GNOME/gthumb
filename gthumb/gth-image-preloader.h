/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2001-2009 The Free Software Foundation, Inc.
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

#ifndef GTH_IMAGE_PRELOADER_H
#define GTH_IMAGE_PRELOADER_H

#include "gth-image-loader.h"
#include "gth-file-data.h"


#define GTH_TYPE_IMAGE_PRELOADER            (gth_image_preloader_get_type ())
#define GTH_IMAGE_PRELOADER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTH_TYPE_IMAGE_PRELOADER, GthImagePreloader))
#define GTH_IMAGE_PRELOADER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GTH_TYPE_IMAGE_PRELOADER, GthImagePreloaderClass))
#define GTH_IS_IMAGE_PRELOADER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTH_TYPE_IMAGE_PRELOADER))
#define GTH_IS_IMAGE_PRELOADER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GTH_TYPE_IMAGE_PRELOADER))
#define GTH_IMAGE_PRELOADER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), GTH_TYPE_IMAGE_PRELOADER, GthImagePreloaderClass))


typedef struct _GthImagePreloader        GthImagePreloader;
typedef struct _GthImagePreloaderClass   GthImagePreloaderClass;
typedef struct _GthImagePreloaderPrivate GthImagePreloaderPrivate;


struct _GthImagePreloader {
	GObject __parent;
	GthImagePreloaderPrivate *priv;
};


struct _GthImagePreloaderClass {
	GObjectClass __parent_class;

	/*< signals >*/

	void      (* requested_ready)  (GthImagePreloader *preloader,
					GError            *error);
};


GType               gth_image_preloader_get_type      (void) G_GNUC_CONST;
GthImagePreloader * gth_image_preloader_new           (void);
/* images get loaded in the following order : requested, next1, prev1. */
void                gth_image_preloader_load          (GthImagePreloader  *preloader,
						       GthFileData        *requested,
						       GthFileData        *next1,
						       GthFileData        *prev1);
void                gth_image_preloader_stop          (GthImagePreloader  *preloader,
						       DataFunc            done_func,
						       gpointer            done_func_data);
GthImageLoader *    gth_image_preloader_get_loader    (GthImagePreloader  *preloader,
						       GthFileData        *file_data);
GthFileData *       gth_image_preloader_get_requested (GthImagePreloader  *preloader);
void                gth_image_preloader_set           (GthImagePreloader  *dest,
						       GthImagePreloader  *src);

#endif /* GTH_IMAGE_PRELOADER_H */
