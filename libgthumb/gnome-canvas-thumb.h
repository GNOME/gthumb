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

#ifndef GNOME_CANVAS_THUMB_H
#define GNOME_CANVAS_PIXAMP_H

#include <gtk/gtkenums.h> /* for GtkAnchorType */
#include <libart_lgpl/art_misc.h>
#include <libart_lgpl/art_pixbuf.h>
#include <libgnomecanvas/gnome-canvas.h>


G_BEGIN_DECLS


/* Image item for the canvas.
 * The following arguments are available:
 *
 * name		type			read/write	description
 * --------------------------------------------------------------------------
 * pixbuf       GdkPixbuf               RW              Poiner to a GdkPixbuf 
 * x		double			RW		X coo. of thumbnail
 * y		double			RW		Y coo. of thumbnail
 * width	double			RW		Thumbnail width
 * height	double			RW		Thumbnail height
 */


#define GNOME_TYPE_CANVAS_THUMB            (gnome_canvas_thumb_get_type ())
#define GNOME_CANVAS_THUMB(obj)            (GTK_CHECK_CAST ((obj), GNOME_TYPE_CANVAS_THUMB, GnomeCanvasThumb))
#define GNOME_CANVAS_THUMB_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), GNOME_TYPE_CANVAS_THUMB, GnomeCanvasThumbClass))
#define GNOME_IS_CANVAS_THUMB(obj)         (GTK_CHECK_TYPE ((obj), GNOME_TYPE_CANVAS_THUMB))
#define GNOME_IS_CANVAS_THUMB_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), GNOME_TYPE_CANVAS_THUMB))
#define GNOME_CANVAS_THUMB_GET_CLASS(obj)  (GTK_CHECK_GET_CLASS ((obj), GNOME_TYPE_CANVAS_THUMB, GnomeCanvasThumbClass))


typedef struct _GnomeCanvasThumb GnomeCanvasThumb;
typedef struct _GnomeCanvasThumbClass GnomeCanvasThumbClass;

struct _GnomeCanvasThumb {
	GnomeCanvasItem item;

	/*< private >*/

	GdkPixbuf *pixbuf;
	GdkPixmap *pixmap;		/* Pixmap rendered from the image */
	GdkBitmap *mask;		/* Mask rendered from the image */

	int iwidth, iheight;            /* Image size in pixels. */

	double x, y;			/* Position at anchor, item relative */
	double width, height;		/* Size of item. */

	int cx, cy;			/* Top-left canvas coo. for display */
	int cwidth, cheight;		/* Rendered size in pixels */

	GdkGC *gc;			/* GC for drawing image */

	unsigned int need_recalc : 1;	/* Do we need to rescale the image? */
	gboolean selected : 1;
};


struct _GnomeCanvasThumbClass {
	GnomeCanvasItemClass parent_class;
};


GType    gnome_canvas_thumb_get_type  (void);

void     gnome_canvas_thumb_select    (GnomeCanvasThumb *image,
				       gboolean select);


G_END_DECLS

#endif
