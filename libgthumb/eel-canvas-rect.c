/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */

/* eel-canvas-rect.c: Rectangle canvas item with AA support.

   Copyright (C) 2002 Alexander Larsson.

   The Gnome Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   The Gnome Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with the Gnome Library; see the file COPYING.LIB.  If not,
   write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.

   Authors: Alexander Larsson <alla@lysator.liu.se>
*/

#include <config.h>
#include "eel-canvas-rect.h"

#include <math.h>
#include <string.h>

#include <glib/gstring.h>
#include <libgnome/gnome-macros.h>
#include <libgnomecanvas/gnome-canvas-util.h>
#include <gdk/gdkx.h>

#ifdef HAVE_RENDER
#include <X11/extensions/Xrender.h>
#endif

/*
 * name			type		read/write	description
 * ------------------------------------------------------------------------------------------
 * x1			double		RW		Leftmost coordinate of rectangle or ellipse
 * y1			double		RW		Topmost coordinate of rectangle or ellipse
 * x2			double		RW		Rightmost coordinate of rectangle or ellipse
 * y2			double		RW		Bottommost coordinate of rectangle or ellipse
 * fill_color_rgba	uint		RW		RGBA color for fill,
 * outline_color_rgba	uint		RW		RGBA color for outline
 * width_pixels		uint		RW		Width of the outline in pixels.  The outline will
 *							not be scaled when the canvas zoom factor is changed.
 */

enum {
	PROP_0,

	PROP_X1,
	PROP_Y1,
	PROP_X2,
	PROP_Y2,

	PROP_FILL_COLOR_RGBA,
	PROP_OUTLINE_COLOR_RGBA,

	PROP_WIDTH_PIXELS
};

struct EelCanvasRectDetails {
	double x1, y1, x2, y2;
	guint fill_color;
	guint outline_color;
	guint width_pixels;

	ArtDRect last_update_rect;
	ArtDRect last_outline_update_rect;
	GdkGC *fill_gc;		/* GC for fill, lazily allocated */
	GdkGC *outline_gc;	/* GC for outline */

#ifdef HAVE_RENDER
	gboolean use_render;
	XRenderPictFormat *format;
#endif
};

GNOME_CLASS_BOILERPLATE (EelCanvasRect, eel_canvas_rect,
			 GnomeCanvasItem, GNOME_TYPE_CANVAS_ITEM);


static ArtDRect  make_drect (double x0, double y0, double x1, double y1);
static void      diff_rects (ArtDRect r1, ArtDRect r2, int *count, ArtDRect result[4]);

static void
eel_canvas_rect_update_fill_gc (EelCanvasRect *rect,
				gboolean       create)
{
	EelCanvasRectDetails *details;
	GnomeCanvasItem *item;
	GdkColor c;

	item = GNOME_CANVAS_ITEM (rect);
	
	details = rect->details;
	
	if (details->fill_gc == NULL) {
		if (!create) {
			return;
		}
		details->fill_gc =
			gdk_gc_new (GTK_WIDGET (item->canvas)->window);
	}
	
	c.pixel = gnome_canvas_get_color_pixel (item->canvas,
						details->fill_color);
	gdk_gc_set_foreground (details->fill_gc, &c);
}

static void
eel_canvas_rect_update_outline_gc (EelCanvasRect *rect,
				   gboolean       create)
{
	EelCanvasRectDetails *details;
	GnomeCanvasItem *item;
	GdkColor c;

	item = GNOME_CANVAS_ITEM (rect);
	
	details = rect->details;
	
	if (details->outline_gc == NULL) {
		if (!create) {
			return;
		}
		details->outline_gc =
			gdk_gc_new (GTK_WIDGET (item->canvas)->window);
	}
	
	c.pixel = gnome_canvas_get_color_pixel (item->canvas,
						details->outline_color);
	gdk_gc_set_foreground (details->outline_gc, &c);
	gdk_gc_set_line_attributes (details->outline_gc,
				    details->width_pixels,
				    GDK_LINE_SOLID,
				    GDK_CAP_BUTT,
				    GDK_JOIN_MITER);
}


static void
eel_canvas_rect_instance_init (EelCanvasRect *rect)
{
	rect->details = g_new0 (EelCanvasRectDetails, 1);
}


static void
eel_canvas_rect_finalize (GObject *object)
{
	EelCanvasRect *rect;

	g_return_if_fail (EEL_IS_CANVAS_RECT (object));
	
	rect = EEL_CANVAS_RECT (object);

	g_free (rect->details);

	G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
eel_canvas_rect_set_property (GObject      *object,
			      guint         param_id,
			      const GValue *value,
			      GParamSpec   *pspec)
{
	GnomeCanvasItem *item;
	EelCanvasRect *rect;
	EelCanvasRectDetails *details;

	item = GNOME_CANVAS_ITEM (object);
	rect = EEL_CANVAS_RECT (object);
	details = rect->details;

	switch (param_id) {
	case PROP_X1:
		details->x1 = g_value_get_double (value);
		gnome_canvas_item_request_update (item);
		break;

	case PROP_Y1:
		details->y1 = g_value_get_double (value);
		gnome_canvas_item_request_update (item);
		break;

	case PROP_X2:
		details->x2 = g_value_get_double (value);
		gnome_canvas_item_request_update (item);
		break;

	case PROP_Y2:
		details->y2 = g_value_get_double (value);
		gnome_canvas_item_request_update (item);
		break;

	case PROP_FILL_COLOR_RGBA:
		details->fill_color = g_value_get_uint (value);

		eel_canvas_rect_update_fill_gc (rect, FALSE);
		
		gnome_canvas_item_request_update (item);
		break;
		
	case PROP_OUTLINE_COLOR_RGBA:
		details->outline_color = g_value_get_uint (value);

		eel_canvas_rect_update_outline_gc (rect, FALSE);
		
		gnome_canvas_item_request_update (item);
		break;

	case PROP_WIDTH_PIXELS:
		details->width_pixels = g_value_get_uint (value);
		eel_canvas_rect_update_outline_gc (rect, FALSE);
		gnome_canvas_item_request_update (item);
		break;

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
		break;
	}
	
}

static void
eel_canvas_rect_get_property (GObject     *object,
			      guint        param_id,
			      GValue      *value,
			      GParamSpec  *pspec)
{
	EelCanvasRect *rect;
	EelCanvasRectDetails *details;

	rect = EEL_CANVAS_RECT (object);
	details = rect->details;

	switch (param_id) {
	case PROP_X1:
		g_value_set_double (value,  details->x1);
		break;

	case PROP_Y1:
		g_value_set_double (value,  details->y1);
		break;

	case PROP_X2:
		g_value_set_double (value,  details->x2);
		break;

	case PROP_Y2:
		g_value_set_double (value,  details->y2);
		break;
		
	case PROP_FILL_COLOR_RGBA:
		g_value_set_uint (value, details->fill_color);
		break;
		
	case PROP_OUTLINE_COLOR_RGBA:
		g_value_set_uint (value, details->outline_color);
		break;

	case PROP_WIDTH_PIXELS:
		g_value_set_uint (value, details->width_pixels);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
		break;
	}	
}

static void
request_redraw_borders (GnomeCanvas *canvas,
			ArtDRect    *update_rect,
			double       width)
{
	gnome_canvas_request_redraw (canvas,
				     update_rect->x0, update_rect->y0,
				     update_rect->x1, update_rect->y0 + width);
	gnome_canvas_request_redraw (canvas,
				     update_rect->x0, update_rect->y1-width,
				     update_rect->x1, update_rect->y1);
	gnome_canvas_request_redraw (canvas,
				     update_rect->x0,       update_rect->y0,
				     update_rect->x0+width, update_rect->y1);
	gnome_canvas_request_redraw (canvas,
				     update_rect->x1-width, update_rect->y0,
				     update_rect->x1,       update_rect->y1);
}

static void
eel_canvas_rect_update (GnomeCanvasItem *item,
			double *affine,
			ArtSVP *clip_path,
			int flags)
{
	EelCanvasRect *rect;
	EelCanvasRectDetails *details;
	double x1, y1, x2, y2;
	int cx1, cy1, cx2, cy2;
	ArtDRect update_rect, repaint_rects[4];
	int repaint_rects_count, i;
	double width_lt, width_rb;

	rect = EEL_CANVAS_RECT (item);
	details = rect->details;

	if (parent_class->update) {
		(* parent_class->update) (item, affine, clip_path, flags);
	}

	/* Update bounding box: */
	width_lt = floor (details->width_pixels / 2.0) / item->canvas->pixels_per_unit;
	width_rb = ceil (details->width_pixels / 2.0) / item->canvas->pixels_per_unit;
	
	x1 = details->x1;
	y1 = details->y1;
	x2 = details->x2;
	y2 = details->y2;

	gnome_canvas_item_i2w (item, &x1, &y1);
	gnome_canvas_item_i2w (item, &x2, &y2);

	/* Inner box: */
	gnome_canvas_w2c (item->canvas, x1 + width_rb, y1 + width_rb, &cx1, &cy1);
	gnome_canvas_w2c (item->canvas, x2 - width_lt, y2 - width_lt, &cx2, &cy2);

	update_rect = make_drect (cx1, cy1, cx2, cy2);
	diff_rects (update_rect, details->last_update_rect,
		    &repaint_rects_count, repaint_rects);
	for (i = 0; i < repaint_rects_count; i++) {
		gnome_canvas_request_redraw (item->canvas,
					     repaint_rects[i].x0, repaint_rects[i].y0,
					     repaint_rects[i].x1, repaint_rects[i].y1);
	}
	details->last_update_rect = update_rect;

	
	/* Outline and bounding box */
	gnome_canvas_w2c (item->canvas, x1 - width_lt, y1 - width_lt, &cx1, &cy1);
	gnome_canvas_w2c (item->canvas, x2 + width_rb, y2 + width_rb, &cx2, &cy2);
	
	update_rect = make_drect (cx1, cy1, cx2, cy2);
	request_redraw_borders (item->canvas, &details->last_outline_update_rect,
				(width_lt + width_rb)*item->canvas->pixels_per_unit);
	request_redraw_borders (item->canvas, &update_rect,
				(width_lt + width_rb)*item->canvas->pixels_per_unit);
	details->last_outline_update_rect = update_rect;
	
	item->x1 = cx1;
	item->y1 = cy1;
	item->x2 = cx2;
	item->y2 = cy2;
}

static void
eel_canvas_rect_realize (GnomeCanvasItem *item)
{
	EelCanvasRect *rect;
	EelCanvasRectDetails *details;
#ifdef HAVE_RENDER
	int event_base, error_base;
#endif
	rect = EEL_CANVAS_RECT (item);
	details = rect->details;
	
	eel_canvas_rect_update_outline_gc (rect, TRUE);

#ifdef HAVE_RENDER
	details->use_render = XRenderQueryExtension (gdk_display, &event_base, &error_base);

	if (details->use_render) {
		Display *dpy;
		GdkVisual *gdk_visual;
		Visual *visual;

		dpy = gdk_x11_drawable_get_xdisplay (GTK_WIDGET (item->canvas)->window);
		gdk_visual = gtk_widget_get_visual (GTK_WIDGET (item->canvas));
		visual = gdk_x11_visual_get_xvisual (gdk_visual);

		details->format = XRenderFindVisualFormat (dpy, visual);
	}
#endif

	if (parent_class->realize) {
		(* parent_class->realize) (item);
	}
}

static void
eel_canvas_rect_unrealize (GnomeCanvasItem *item)
{
	EelCanvasRect *rect;
	EelCanvasRectDetails *details;

	rect = EEL_CANVAS_RECT (item);
	details = rect->details;

	if (details->outline_gc) {
		g_object_unref (details->outline_gc);
		details->outline_gc = NULL;
	}
	
	if (details->fill_gc) {
		g_object_unref (details->fill_gc);
		details->fill_gc = NULL;
	}

	if (parent_class->unrealize) {
		(* parent_class->unrealize) (item);
	}
}

static void
eel_canvas_rect_render (GnomeCanvasItem *item,
			GnomeCanvasBuf  *buf)
{
	g_assert_not_reached ();
}

static double
eel_canvas_rect_point (GnomeCanvasItem *item,
		       double x, double y,
		       int cx, int cy,
		       GnomeCanvasItem **actual_item)
{
	EelCanvasRect *rect;
	EelCanvasRectDetails *details;
	double x1, y1, x2, y2;
	double hwidth;
	double dx, dy;

	rect = EEL_CANVAS_RECT (item);
	details = rect->details;

	*actual_item = item;

	hwidth = (details->width_pixels / item->canvas->pixels_per_unit) / 2.0;
	x1 = details->x1 - hwidth;
	y1 = details->y1 - hwidth;
	x2 = details->x2 + hwidth;
	y2 = details->y2 + hwidth;


	if ((x >= x1) && (y >= y1) && (x <= x2) && (y <= y2)) {
		return 0.0;
	}

	/* Point is outside rectangle */
	if (x < x1)
		dx = x1 - x;
	else if (x > x2)
		dx = x - x2;
	else
		dx = 0.0;

	if (y < y1)
		dy = y1 - y;
	else if (y > y2)
		dy = y - y2;
	else
		dy = 0.0;

	return sqrt (dx * dx + dy * dy);
}

static void
render_rect_alpha (EelCanvasRect *rect,
		   GdkDrawable *drawable,
		   int x, int y,
		   int width, int height,
		   guint32 rgba)
{
	GdkPixbuf *pixbuf;
	guchar *data;
	int rowstride, i;
	guchar r, g, b, a;
	EelCanvasRectDetails *details;

	if (width <= 0 || height <= 0 ) {
		return;
	}
	
	details = rect->details;

	r = (rgba >> 24) & 0xff;
	g = (rgba >> 16) & 0xff;
	b = (rgba >> 8) & 0xff;
	a = (rgba >> 0) & 0xff;

#ifdef HAVE_RENDER
	if (details->use_render) {
		Display *dpy;
		Picture  pict;
		XRenderPictureAttributes attributes;
		XRenderColor color;
	
		dpy = gdk_x11_drawable_get_xdisplay (drawable);

		pict = XRenderCreatePicture (dpy,
					     gdk_x11_drawable_get_xid (drawable),
					     details->format,
					     0,
					     &attributes);


		/* Convert to premultiplied alpha: */
		r = r * a / 255;
		g = g * a / 255;
		b = b * a / 255;
		
		color.red = (r << 8) + r;
		color.green = (g << 8) + g;
		color.blue = (b << 8) + b;
		color.alpha = (a << 8) + a;
		
		XRenderFillRectangle (dpy,
				      PictOpOver,
				      pict,
				      &color,
				      x, y,
				      width, height);
		XRenderFreePicture (dpy, pict);

		return;
	}
#endif
	pixbuf = gdk_pixbuf_new (GDK_COLORSPACE_RGB, TRUE, 8, width, height);
	data = gdk_pixbuf_get_pixels (pixbuf);
	rowstride = gdk_pixbuf_get_rowstride (pixbuf);
	
	r = (rgba >> 24) & 0xff;
	g = (rgba >> 16) & 0xff;
	b = (rgba >> 8) & 0xff;
	a = (rgba >> 0) & 0xff;
	
	for (i = 0; i < width*4; ) {
		data[i++] = r;
		data[i++] = g;
		data[i++] = b;
		data[i++] = a;
	}
	
	for (i = 1; i < height; i++) {
		memcpy (data + i*rowstride, data, width*4);
	}
	
	gdk_pixbuf_render_to_drawable_alpha (pixbuf,
					     drawable,
					     0, 0,
					     x, y,
					     width, height,
					     GDK_PIXBUF_ALPHA_FULL,
					     255,
					     GDK_RGB_DITHER_NONE, 0, 0);
	g_object_unref (pixbuf);
}


static void
eel_canvas_rect_draw (GnomeCanvasItem *item,
		      GdkDrawable *drawable,
		      int x, int y,
		      int width, int height)
{
	EelCanvasRect *rect;
	EelCanvasRectDetails *details;
	double x1, y1, x2, y2;
	int cx1, cy1, cx2, cy2;
	double width_lt, width_rb;

	rect = EEL_CANVAS_RECT (item);
	details = rect->details;

	/* Update bounding box: */
	width_lt = floor (details->width_pixels / 2.0) / item->canvas->pixels_per_unit;
	width_rb = ceil (details->width_pixels / 2.0) / item->canvas->pixels_per_unit;

	x1 = details->x1;
	y1 = details->y1;
	x2 = details->x2;
	y2 = details->y2;

	gnome_canvas_item_i2w (item, &x1, &y1);
	gnome_canvas_item_i2w (item, &x2, &y2);

	/* Inner box: */
	gnome_canvas_w2c (item->canvas, x1 + width_rb, y1 + width_rb, &cx1, &cy1);
	gnome_canvas_w2c (item->canvas, x2 - width_lt, y2 - width_lt, &cx2, &cy2);

	/* Clip and offset to dest drawable */
	cx1 = MAX (0, cx1 - x);
	cy1 = MAX (0, cy1 - y);
	cx2 = MIN (width, cx2 - x);
	cy2 = MIN (height, cy2 - y);
	
	if ((details->fill_color & 0xff) != 255) {
		render_rect_alpha (rect,
				   drawable,
				   cx1, cy1,
				   cx2 - cx1, cy2 - cy1,
				   details->fill_color);
	} else {
		if (details->fill_gc == NULL) {
			eel_canvas_rect_update_fill_gc (rect, TRUE);
		}
		gdk_draw_rectangle (drawable,
				    details->fill_gc,
				    TRUE,
				    cx1, cy1,
				    cx2, cy2);
	}

	/* Box outline: */
	
	gnome_canvas_w2c (item->canvas, x1, y1, &cx1, &cy1);
	gnome_canvas_w2c (item->canvas, x2, y2, &cx2, &cy2);
	
	/* FIXME: Doesn't handle alpha for outline. Nautilus doesn't currently use this */
	gdk_draw_rectangle (drawable,
			    details->outline_gc,
			    FALSE,
			    cx1 - x, cy1 - y,
			    cx2 - cx1, cy2 - cy1);
}

static void
eel_canvas_rect_bounds (GnomeCanvasItem *item,
			double *x1, double *y1,
			double *x2, double *y2)
{
	EelCanvasRect *rect;
	EelCanvasRectDetails *details;
	double hwidth;

	rect = EEL_CANVAS_RECT (item);
	details = rect->details;

	hwidth = (details->width_pixels / item->canvas->pixels_per_unit) / 2.0;
	
	*x1 = details->x1 - hwidth;
	*y1 = details->y1 - hwidth;
	*x2 = details->x2 + hwidth;
	*y2 = details->y2 + hwidth;
}

static void
eel_canvas_rect_class_init (EelCanvasRectClass *class)
{
	GObjectClass *gobject_class;
	GnomeCanvasItemClass *item_class;

	gobject_class = G_OBJECT_CLASS (class);
	item_class = GNOME_CANVAS_ITEM_CLASS (class);
	
	gobject_class->finalize = eel_canvas_rect_finalize;
	gobject_class->set_property = eel_canvas_rect_set_property;
	gobject_class->get_property = eel_canvas_rect_get_property;

	item_class->update = eel_canvas_rect_update;
	item_class->realize = eel_canvas_rect_realize;
	item_class->unrealize = eel_canvas_rect_unrealize;
	item_class->draw = eel_canvas_rect_draw;
	item_class->point = eel_canvas_rect_point;
	item_class->render = eel_canvas_rect_render;
	item_class->bounds = eel_canvas_rect_bounds;
	
        g_object_class_install_property (gobject_class,
					 PROP_X1,
					 g_param_spec_double ("x1", NULL, NULL,
							      -G_MAXDOUBLE, G_MAXDOUBLE, 0,
							      (G_PARAM_READABLE | G_PARAM_WRITABLE)));
        g_object_class_install_property (gobject_class,
					 PROP_Y1,
					 g_param_spec_double ("y1", NULL, NULL,
							      -G_MAXDOUBLE, G_MAXDOUBLE, 0,
							      (G_PARAM_READABLE | G_PARAM_WRITABLE)));
        g_object_class_install_property (gobject_class,
					 PROP_X2,
					 g_param_spec_double ("x2", NULL, NULL,
							      -G_MAXDOUBLE, G_MAXDOUBLE, 0,
							      (G_PARAM_READABLE | G_PARAM_WRITABLE)));
        g_object_class_install_property (gobject_class,
					 PROP_Y2,
					 g_param_spec_double ("y2", NULL, NULL,
							      -G_MAXDOUBLE, G_MAXDOUBLE, 0,
							      (G_PARAM_READABLE | G_PARAM_WRITABLE)));
	
        g_object_class_install_property (gobject_class,
                                         PROP_FILL_COLOR_RGBA,
                                         g_param_spec_uint ("fill_color_rgba", NULL, NULL,
                                                            0, G_MAXUINT, 0,
                                                            (G_PARAM_READABLE | G_PARAM_WRITABLE)));
        g_object_class_install_property (gobject_class,
                                         PROP_OUTLINE_COLOR_RGBA,
                                         g_param_spec_uint ("outline_color_rgba", NULL, NULL,
                                                            0, G_MAXUINT, 0,
                                                            (G_PARAM_READABLE | G_PARAM_WRITABLE)));
        g_object_class_install_property (gobject_class,
                                         PROP_WIDTH_PIXELS,
                                         g_param_spec_uint ("width_pixels", NULL, NULL,
                                                            0, G_MAXUINT, 0,
                                                            (G_PARAM_READABLE | G_PARAM_WRITABLE)));
}


static ArtDRect
make_drect (double x0, double y0, double x1, double y1)
{
	ArtDRect r;

	r.x0 = x0;
	r.y0 = y0;
	r.x1 = x1;
	r.y1 = y1;
	return r;
}

static gboolean
rects_intersect (ArtDRect r1, ArtDRect r2)
{
	if (r1.x0 >= r2.x1) {
		return FALSE;
	}
	if (r2.x0 >= r1.x1) {
		return FALSE;
	}
	if (r1.y0 >= r2.y1) {
		return FALSE;
	}
	if (r2.y0 >= r1.y1) {
		return FALSE;
	}
	return TRUE;
}

static void
diff_rects_guts (ArtDRect ra, ArtDRect rb, int *count, ArtDRect result[4])
{
	if (ra.x0 < rb.x0) {
		result[(*count)++] = make_drect (ra.x0, ra.y0, rb.x0, ra.y1);
	}
	if (ra.y0 < rb.y0) {
		result[(*count)++] = make_drect (ra.x0, ra.y0, ra.x1, rb.y0);
	}
	if (ra.x1 < rb.x1) {
		result[(*count)++] = make_drect (ra.x1, rb.y0, rb.x1, rb.y1);
	}
	if (ra.y1 < rb.y1) {
		result[(*count)++] = make_drect (rb.x0, ra.y1, rb.x1, rb.y1);
	}
}

static void
diff_rects (ArtDRect r1, ArtDRect r2, int *count, ArtDRect result[4])
{
	g_assert (count != NULL);
	g_assert (result != NULL);

	*count = 0;

	if (rects_intersect (r1, r2)) {
		diff_rects_guts (r1, r2, count, result);
		diff_rects_guts (r2, r1, count, result);
	} else {
		if (!art_drect_empty (&r1)) {
			result[(*count)++] = r1;
		}
		if (!art_drect_empty (&r2)) {
			result[(*count)++] = r2;
		}
	}
}
