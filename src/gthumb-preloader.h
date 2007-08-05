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

#ifndef GTHUMB_PRELOADER_H
#define GTHUMB_PRELOADER_H


#include <time.h>
#include "image-loader.h"
#include "file-data.h"

#define GTHUMB_TYPE_PRELOADER            (gthumb_preloader_get_type ())
#define GTHUMB_PRELOADER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTHUMB_TYPE_PRELOADER, GThumbPreloader))
#define GTHUMB_PRELOADER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GTHUMB_TYPE_PRELOADER, GThumbPreloaderClass))
#define GTHUMB_IS_PRELOADER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTHUMB_TYPE_PRELOADER))
#define GTHUMB_IS_PRELOADER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GTHUMB_TYPE_PRELOADER))
#define GTHUMB_PRELOADER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), GTHUMB_TYPE_PRELOADER, GThumbPreloaderrClass))


typedef struct _GThumbPreloader      GThumbPreloader;
typedef struct _GThumbPreloaderClass GThumbPreloaderClass;


#define N_LOADERS 3


typedef struct {
	FileData        *file;
	ImageLoader     *loader;
	gboolean         loaded;
	gboolean         error;
	GThumbPreloader *gploader;
} PreLoader;


struct _GThumbPreloader {
	GObject __parent;

	PreLoader   *loader[N_LOADERS];       /* Array of loaders, each loader
					       * will load an image. */
	int          requested;               /* This is the loader with the
					       * requested image.  The 
					       * requested image is the image 
					       * the user has expressly 
					       * requested to view, when this 
					       * image is loaded correctly a 
					       * requested_done signal is 
					       * emitted, otherwise a 
					       * requested_error is emitted.
					       * Other images do not trigger
					       * any signal. */
	int          current;                 /* This is the loader that has
					       * a loading underway. */
	gboolean     stopped;                 /* Whether the preloader has 
					       * been stopped. */
	DoneFunc     done_func;               /* Function to call after 
					       * stopping the loader. */
	gpointer     done_func_data;
	guint        load_id;
};


struct _GThumbPreloaderClass {
	GObjectClass __parent_class;

	/*< signals >*/

	void      (* requested_done)  (GThumbPreloader *gploader);
	void      (* requested_error) (GThumbPreloader *gploader);
};


GType               gthumb_preloader_get_type   (void) G_GNUC_CONST;

GThumbPreloader    *gthumb_preloader_new        (void);

/* images get loaded in the following order : requested, next1, prev1. 
 */

void                gthumb_preloader_start      (GThumbPreloader  *gploader,
						 const char       *requested,
						 const char       *next1,
						 const char       *prev1);
void                gthumb_preloader_load       (GThumbPreloader  *gploader,
						 FileData         *requested,
						 FileData         *next1,
						 FileData         *prev1);
void                gthumb_preloader_stop       (GThumbPreloader  *gploader,
						 DoneFunc          done_func,
						 gpointer          done_func_data);
ImageLoader        *gthumb_preloader_get_loader (GThumbPreloader  *gploader,
						 const char       *path);


#endif /* GTHUMB_PRELOADER_H */
