/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2009 Free Software Foundation, Inc.
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

#ifndef GTH_SLIDESHOW_H
#define GTH_SLIDESHOW_H

#include <gthumb.h>
#include <clutter/clutter.h>
#include <clutter-gtk/clutter-gtk.h>

G_BEGIN_DECLS

#define GTH_TYPE_SLIDESHOW              (gth_slideshow_get_type ())
#define GTH_SLIDESHOW(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTH_TYPE_SLIDESHOW, GthSlideshow))
#define GTH_SLIDESHOW_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), GTH_SLIDESHOW_TYPE, GthSlideshowClass))
#define GTH_IS_SLIDESHOW(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTH_TYPE_SLIDESHOW))
#define GTH_IS_SLIDESHOW_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), GTH_TYPE_SLIDESHOW))
#define GTH_SLIDESHOW_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS((obj), GTH_TYPE_SLIDESHOW, GthSlideshowClass))

typedef struct _GthSlideshow         GthSlideshow;
typedef struct _GthSlideshowClass    GthSlideshowClass;
typedef struct _GthSlideshowPrivate  GthSlideshowPrivate;

struct _GthSlideshow
{
	GtkWindow __parent;
	ClutterActor        *stage;
	ClutterActor        *current_image;
	ClutterActor        *next_image;
	ClutterGeometry      current_geometry;
	ClutterGeometry      next_geometry;
	gboolean             first_frame;
	GthSlideshowPrivate *priv;
};

struct _GthSlideshowClass
{
	GtkWindowClass __parent_class;
};

GType            gth_slideshow_get_type        (void);
GtkWidget *      gth_slideshow_new             (GthBrowser       *browser,
					        GList            *file_list /* GthFileData */);
void             gth_slideshow_set_delay       (GthSlideshow     *self,
					        guint             msecs);
void             gth_slideshow_set_automatic   (GthSlideshow     *self,
					        gboolean          automatic);
void             gth_slideshow_set_loop        (GthSlideshow     *self,
					        gboolean          loop);
void             gth_slideshow_set_transitions (GthSlideshow     *self,
					        GList            *transitions);

G_END_DECLS

#endif /* GTH_SLIDESHOW_H */
