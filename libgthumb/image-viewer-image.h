/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2008 The Free Software Foundation, Inc.
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

#ifndef IMAGE_VIEWER_IMAGE_H
#define IMAGE_VIEWER_IMAGE_H

#include <gtk/gtk.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include "file-data.h"
#include "image-viewer-enums.h"

G_BEGIN_DECLS

#define IMAGE_VIEWER_IMAGE_TYPE            (image_viewer_image_get_type ())
#define IMAGE_VIEWER_IMAGE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), IMAGE_VIEWER_IMAGE_TYPE, ImageViewerImage))
#define IMAGE_VIEWER_IMAGE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), IMAGE_VIEWER_IMAGE_TYPE, ImageViewerImageClass))
#define IS_IMAGE_VIEWER_IMAGE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), IMAGE_VIEWER_IMAGE_TYPE))
#define IS_IMAGE_VIEWER_IMAGE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), IMAGE_VIEWER_IMAGE_TYPE))
#define IMAGE_VIEWER_IMAGE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), IMAGE_VIEWER_IMAGE_TYPE, ImageViewerImageClass))

typedef struct _ImageViewerImage           ImageViewerImage;
typedef struct _ImageViewerImageClass      ImageViewerImageClass;

struct _ImageViewerImage
{
	GObject                    parent;
};

struct _ImageViewerImageClass
{
	GObjectClass               parent;

	/* -- Signals -- */
	void (* zoom_changed)      (ImageViewerImage *image);
};

GType             image_viewer_image_get_type                 (void);

ImageViewerImage* image_viewer_image_new                      (ImageViewer* viewer,
							       ImageLoader* loader);

GthFit            image_viewer_image_get_fit_mode             (ImageViewerImage *image);
gboolean          image_viewer_image_get_has_alpha            (ImageViewerImage *image);
gboolean          image_viewer_image_get_is_animation         (ImageViewerImage *image);
gint              image_viewer_image_get_height               (ImageViewerImage *image);
gchar*            image_viewer_image_get_path                 (ImageViewerImage *image);
GdkPixbuf*        image_viewer_image_get_pixbuf               (ImageViewerImage *image);
void              image_viewer_image_get_scroll_offsets       (ImageViewerImage *image,
							       gdouble          *x_offset,
							       gdouble          *y_offset);
gint              image_viewer_image_get_width                (ImageViewerImage *image);
gint              image_viewer_image_get_x_offset             (ImageViewerImage *image);
gint              image_viewer_image_get_y_offset             (ImageViewerImage *image);
gdouble           image_viewer_image_get_zoom_level           (ImageViewerImage *image);
void              image_viewer_image_get_zoomed_size          (ImageViewerImage *image,
							       gdouble          *width,
							       gdouble          *height);

void              image_viewer_image_paint                    (ImageViewerImage *image,
							       cairo_t          *cr);

void              image_viewer_image_scroll_relative          (ImageViewerImage *image,
							       gdouble           delta_x,
							       gdouble           delta_y);

void              image_viewer_image_set_fit_mode             (ImageViewerImage *image,
							       GthFit           fit_mode);
void              image_viewer_image_set_scroll_offsets       (ImageViewerImage *image,
							       gdouble           x_offset,
							       gdouble           y_offset);

void              image_viewer_image_start_animation          (ImageViewerImage *image);
void              image_viewer_image_step_animation           (ImageViewerImage *image);
void              image_viewer_image_stop_animation           (ImageViewerImage *image);

void              image_viewer_image_zoom_at_point            (ImageViewerImage *image,
							       gdouble           zoom_level,
							       gdouble           x,
							       gdouble           y);
void              image_viewer_image_zoom_centered            (ImageViewerImage *image,
							       gdouble           zoom_level);
void              image_viewer_image_zoom_in_at_point         (ImageViewerImage *image,
							       gdouble           x,
							       gdouble           y);
void              image_viewer_image_zoom_in_centered         (ImageViewerImage *image);
void              image_viewer_image_zoom_out_at_point        (ImageViewerImage *image,
							       gdouble           x,
							       gdouble           y);
void              image_viewer_image_zoom_out_centered        (ImageViewerImage *image);

G_END_DECLS

#endif /* IMAGE_VIEWER_IMAGE_H */

