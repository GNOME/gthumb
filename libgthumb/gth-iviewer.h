/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2006 Free Software Foundation, Inc.
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

#ifndef GTH_IVIEWER_H
#define GTH_IVIEWER_H

#include <glib-object.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gtk/gtk.h>

#define GTH_TYPE_IVIEWER                (gth_iviewer_get_type ())
#define GTH_IVIEWER(obj)                (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTH_TYPE_IVIEWER, GthIViewer))
#define GTH_IS_IVIEWER(obj)             (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTH_TYPE_IVIEWER))
#define GTH_IVIEWER_GET_INTERFACE(inst) (G_TYPE_INSTANCE_GET_INTERFACE ((inst), GTH_TYPE_IVIEWER, GthIViewerInterface))

typedef struct _GthIViewer GthIViewer;
typedef struct _GthIViewerInterface GthIViewerInterface;

struct _GthIViewerInterface {
	GTypeInterface parent;

	/*< signals >*/

	void        (* size_changed)      (GthIViewer *viewer);

	/*< virtual functions >*/

	double      (*get_zoom)           (GthIViewer *self);
	void        (*set_zoom)           (GthIViewer *self,
					   double      zoom);
	void        (*zoom_in)            (GthIViewer *self);
	void        (*zoom_out)           (GthIViewer *self);
	void        (*zoom_to_fit)        (GthIViewer *self);

	GdkPixbuf * (*get_image)          (GthIViewer *self);
	void        (*get_adjustments)    (GthIViewer     *self,
					   GtkAdjustment **hadj,
					   GtkAdjustment **vadj);
};

GType          gth_iviewer_get_type           (void);
double         gth_iviewer_get_zoom           (GthIViewer     *iviewer);
void           gth_iviewer_set_zoom           (GthIViewer     *iviewer,
					       double          zoom);
void           gth_iviewer_zoom_in            (GthIViewer     *iviewer);
void           gth_iviewer_zoom_out           (GthIViewer     *iviewer);
void           gth_iviewer_zoom_to_fit        (GthIViewer     *iviewer);
GdkPixbuf *    gth_iviewer_get_image          (GthIViewer     *iviewer);
void           gth_iviewer_get_adjustments    (GthIViewer     *self,
					       GtkAdjustment **hadj,
					       GtkAdjustment **vadj);
void           gth_iviewer_size_changed       (GthIViewer     *self);


/* help functions */

int            gth_iviewer_get_image_width    (GthIViewer *iviewer);
int            gth_iviewer_get_image_height   (GthIViewer *iviewer);
gboolean       gth_iviewer_is_void            (GthIViewer *iviewer);
void           gth_iviewer_scroll_to          (GthIViewer *iviewer,
					       int         x_offset,
					       int         y_offset);
void           gth_iviewer_get_scroll_offset  (GthIViewer *iviewer,
					       int        *x,
					       int        *y);
#endif /*GTH_IVIEWER_H*/
