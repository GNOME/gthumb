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

#include <config.h>
#include <math.h>
#include <libgnomecanvas/gnome-canvas.h>
#include <libgnomecanvas/gnome-canvas-util.h>
#include "gnome-canvas-thumb.h"
#include "gthumb-slide.h"


enum {
	PROP_0,
	PROP_PIXBUF,
	PROP_X,
	PROP_Y,
	PROP_WIDTH,
	PROP_HEIGHT
};


#define DEFAULT_WIDTH  105
#define DEFAULT_HEIGHT 105

#define COLOR_WHITE   0x00ffffff
#define CHECK_SIZE    50

static void gnome_canvas_thumb_class_init    (GnomeCanvasThumbClass *class);
static void gnome_canvas_thumb_init          (GnomeCanvasThumb      *image);
static void gnome_canvas_thumb_destroy       (GtkObject             *object);
static void gnome_canvas_thumb_set_property  (GObject *object,
					      guint param_id,
					      const GValue *value,
					      GParamSpec *pspec);
static void gnome_canvas_thumb_get_property  (GObject *object,
					      guint param_id,
					      GValue *value,
					      GParamSpec *pspec);

static void   gnome_canvas_thumb_update      (GnomeCanvasItem *item, 
					      double *affine, 
					      ArtSVP *clip_path, 
					      int flags);

static void   gnome_canvas_thumb_realize     (GnomeCanvasItem *item);
static void   gnome_canvas_thumb_unrealize   (GnomeCanvasItem *item);

static void   gnome_canvas_thumb_draw        (GnomeCanvasItem *item, 
					      GdkDrawable *drawable,
					      int x, 
					      int y, 
					      int width, 
					      int height);
static double gnome_canvas_thumb_point       (GnomeCanvasItem *item, 
					      double x, 
					      double y,
					      int cx, 
					      int cy, 
					      GnomeCanvasItem **actual_item);

static void   gnome_canvas_thumb_bounds      (GnomeCanvasItem *item, 
					      double *x1, 
					      double *y1, 
					      double *x2, 
					      double *y2);
static void   gnome_canvas_thumb_render      (GnomeCanvasItem *item, 
					      GnomeCanvasBuf *buf);


static GnomeCanvasItemClass *parent_class;


GType
gnome_canvas_thumb_get_type (void)
{
	static guint type = 0;

	if (! type) {
		GTypeInfo info = {
			sizeof (GnomeCanvasClass),
			NULL,
			NULL,
			(GClassInitFunc) gnome_canvas_thumb_class_init,
			NULL,
			NULL,
			sizeof (GnomeCanvasThumb),
			0,
			(GInstanceInitFunc) gnome_canvas_thumb_init
		};

		type = g_type_register_static (GNOME_TYPE_CANVAS_ITEM,
					       "GnomeCanvasThumb",
					       &info,
					       0);
	}

	return type;
}


static void
gnome_canvas_thumb_class_init (GnomeCanvasThumbClass *class)
{
	GObjectClass *gobject_class;
	GtkObjectClass *object_class;
	GnomeCanvasItemClass *item_class;

	parent_class = g_type_class_peek_parent (class);
	gobject_class = (GObjectClass *) class;
	object_class = (GtkObjectClass *) class;
	item_class = (GnomeCanvasItemClass *) class;

	gobject_class->set_property = gnome_canvas_thumb_set_property;
        gobject_class->get_property = gnome_canvas_thumb_get_property;

	g_object_class_install_property
                (gobject_class,
                 PROP_PIXBUF,
                 g_param_spec_object ("pixbuf", NULL, NULL,
                                      GDK_TYPE_PIXBUF,
                                      (G_PARAM_READABLE | G_PARAM_WRITABLE)));
	g_object_class_install_property
                (gobject_class,
                 PROP_X,
                 g_param_spec_double ("x", NULL, NULL,
                                      -G_MAXDOUBLE, G_MAXDOUBLE, 0,
                                    (G_PARAM_READABLE | G_PARAM_WRITABLE)));
	g_object_class_install_property
                (gobject_class,
                 PROP_Y,
                 g_param_spec_double ("y", NULL, NULL,
                                      -G_MAXDOUBLE, G_MAXDOUBLE, 0,
                                      (G_PARAM_READABLE | G_PARAM_WRITABLE)));
        g_object_class_install_property
                (gobject_class,
                 PROP_WIDTH,
                 g_param_spec_double ("width", NULL, NULL,
                                      -G_MAXDOUBLE, G_MAXDOUBLE, 0,
                                      (G_PARAM_READABLE | G_PARAM_WRITABLE)));
	g_object_class_install_property
                (gobject_class,
                 PROP_HEIGHT,
                 g_param_spec_double ("height", NULL, NULL,
                                      -G_MAXDOUBLE, G_MAXDOUBLE, 0,
                                      (G_PARAM_READABLE | G_PARAM_WRITABLE)));

	object_class->destroy = gnome_canvas_thumb_destroy;

	item_class->realize = gnome_canvas_thumb_realize;
	item_class->unrealize = gnome_canvas_thumb_unrealize;
	item_class->update = gnome_canvas_thumb_update;
	item_class->draw = gnome_canvas_thumb_draw;
	item_class->render = gnome_canvas_thumb_render;
	item_class->point = gnome_canvas_thumb_point;
	item_class->bounds = gnome_canvas_thumb_bounds;
}


static void
gnome_canvas_thumb_init (GnomeCanvasThumb *image)
{
	image->pixbuf = NULL;
	image->x = 0.0;
	image->y = 0.0;
	image->width = DEFAULT_WIDTH;
	image->height = DEFAULT_HEIGHT;
	image->need_recalc = TRUE;
}


static void
free_pixmap_and_mask (GnomeCanvasThumb *image)
{
	if (image->pixmap) {
		g_object_unref (image->pixmap);
		image->pixmap = NULL;
	}

	if (image->mask) {
		g_object_unref (image->mask);
		image->mask = NULL;
	}

	image->cwidth = 0;
	image->cheight = 0;
}


static void
gnome_canvas_thumb_destroy (GtkObject *object)
{
	GnomeCanvasThumb *image;

	g_return_if_fail (object != NULL);
	g_return_if_fail (GNOME_IS_CANVAS_THUMB (object));

	image = GNOME_CANVAS_THUMB (object);

	free_pixmap_and_mask (image);

	if (image->pixbuf != NULL) {
		g_object_unref (G_OBJECT (image->pixbuf));
		image->pixbuf = NULL;
	}

	if (GTK_OBJECT_CLASS (parent_class)->destroy)
		GTK_OBJECT_CLASS (parent_class)->destroy (object);
}


static void
render_to_pixmap (GnomeCanvasThumb *image,
		  GdkPixbuf *pixbuf)
{
	if (image->pixbuf != NULL) {
		g_object_unref (G_OBJECT (image->pixbuf));
		image->pixbuf = NULL;
	}

	image->iwidth = gdk_pixbuf_get_width (pixbuf);
	image->iheight = gdk_pixbuf_get_height (pixbuf);

	if (gdk_pixbuf_get_has_alpha (pixbuf))
		image->pixbuf = gdk_pixbuf_composite_color_simple (
					   pixbuf, 
					   image->iwidth,
					   image->iheight, 
					   GDK_INTERP_NEAREST,
					   255,
					   CHECK_SIZE, 
					   COLOR_WHITE, 
					   COLOR_WHITE);
	else {
		image->pixbuf = pixbuf;
		g_object_ref (G_OBJECT (image->pixbuf));
	}
	
	free_pixmap_and_mask (image);
	gdk_pixbuf_render_pixmap_and_mask (image->pixbuf, 
					   &(image->pixmap), 
					   &(image->mask), 
					   112);
}


/* Set_property handler for the pixbuf canvas item */
static void
gnome_canvas_thumb_set_property (GObject       *object,
				 guint          param_id,
				 const GValue  *value,
				 GParamSpec    *pspec)
{
	GnomeCanvasItem *item;
	GnomeCanvasThumb *image;
	gint update;
	GdkPixbuf *pixbuf;

	item = GNOME_CANVAS_ITEM (object);
	image = GNOME_CANVAS_THUMB (object);

	update = FALSE;

        switch (param_id) {
        case PROP_PIXBUF:
		pixbuf = GDK_PIXBUF (g_value_get_object (value));
		g_return_if_fail (pixbuf != NULL);
		if (pixbuf == image->pixbuf) 
			return;

		render_to_pixmap (image, pixbuf);

		image->need_recalc = TRUE;
		update = TRUE;
		break;

	case PROP_X:
		image->x = g_value_get_double (value);
		update = TRUE;
		break;

	case PROP_Y:
		image->y = g_value_get_double (value);
		update = TRUE;
		break;

	case PROP_WIDTH:
		image->width = g_value_get_double (value);
		update = TRUE;
		break;

	case PROP_HEIGHT:
		image->height = g_value_get_double (value);
		update = TRUE;
		break;

	default:
		break;
	}

	if (update)
		gnome_canvas_item_request_update (item);
}


/* Get_property handler for the pixbuf canvasi item */
static void
gnome_canvas_thumb_get_property (GObject       *object,
				 guint          param_id,
				 GValue        *value,
				 GParamSpec    *pspec)
{
	GnomeCanvasThumb *image;

	image = GNOME_CANVAS_THUMB (object);

        switch (param_id) {
        case PROP_PIXBUF:
                g_value_set_object (value, G_OBJECT (image->pixbuf));
                break;

        case PROP_X:
                g_value_set_double (value, image->x);
                break;

        case PROP_Y:
                g_value_set_double (value, image->y);
                break;

        case PROP_WIDTH:
                g_value_set_double (value, image->width);
                break;

        case PROP_HEIGHT:
                g_value_set_double (value, image->height);
                break;

	default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
                break;
	}
}


static void
gnome_canvas_thumb_realize (GnomeCanvasItem *item)
{
	GnomeCanvasThumb *image;

	image = GNOME_CANVAS_THUMB (item);

	if (parent_class->realize)
		(* parent_class->realize) (item);

	if (! item->canvas->aa) 
		image->gc = gdk_gc_new (item->canvas->layout.bin_window);
}


static void
gnome_canvas_thumb_unrealize (GnomeCanvasItem *item)
{
	GnomeCanvasThumb *image;

	image = GNOME_CANVAS_THUMB (item);

	if (! item->canvas->aa) {
		g_object_unref (image->gc);
		image->gc = NULL;
	}

	if (parent_class->unrealize)
		(* parent_class->unrealize) (item);
}


static void 
get_bounds (GnomeCanvasThumb *image, 
	    double *px1, double *py1, 
	    double *px2, double *py2)
{
        GnomeCanvasItem *item;
	double i2c[6];
	ArtDRect i_bbox, c_bbox;

        item = GNOME_CANVAS_ITEM (image);

        i_bbox.x0 = image->x;
        i_bbox.y0 = image->y;
        i_bbox.x1 = image->x + image->width;
        i_bbox.y1 = image->y + image->height;

	gnome_canvas_item_i2c_affine (item, i2c);
	art_drect_affine_transform (&c_bbox, &i_bbox, i2c);

	/* add a fudge factor */
	*px1 = c_bbox.x0 - 1;
	*py1 = c_bbox.y0 - 1;
	*px2 = c_bbox.x1 + 1;
	*py2 = c_bbox.y1 + 1;
}


/* Get's the image bounds expressed as item-relative coordinates. */
static void
get_bounds_item_relative (GnomeCanvasThumb *image, 
			  double *px1, double *py1, 
			  double *px2, double *py2)
{
	*px1 = image->x;
	*py1 = image->y;
	*px2 = image->x + image->width;
	*py2 = image->y + image->height;
}


static void
recalc_bounds (GnomeCanvasThumb *image)
{
	GnomeCanvasItem *item = GNOME_CANVAS_ITEM (image);
	get_bounds (image, &item->x1, &item->y1, &item->x2, &item->y2);
}


static void
gnome_canvas_thumb_update (GnomeCanvasItem *item, 
			   double *affine, 
			   ArtSVP *clip_path, 
			   int flags)
{
	GnomeCanvasThumb *image = GNOME_CANVAS_THUMB (item);
	ArtDRect i_bbox, c_bbox;

	if (parent_class->update)
		(* parent_class->update) (item, affine, clip_path, flags);

	gnome_canvas_request_redraw (item->canvas, 
				     image->cx, 
				     image->cy, 
				     image->cx + image->cwidth + 1, 
				     image->cy + image->cheight + 1);
	
	recalc_bounds (image);
	get_bounds_item_relative (image, 
				  &i_bbox.x0, &i_bbox.y0, 
				  &i_bbox.x1, &i_bbox.y1);
	art_drect_affine_transform (&c_bbox, &i_bbox, affine);
	
	image->cx = c_bbox.x0;
	image->cy = c_bbox.y0;
	image->cwidth = (int) (c_bbox.x1 - c_bbox.x0);
	image->cheight = (int) (c_bbox.y1 - c_bbox.y0);
	
	c_bbox.x0--;
	c_bbox.y0--;
	c_bbox.x1++;
	c_bbox.y1++;
	
	c_bbox.x1++;
	c_bbox.y1++;
	
	gnome_canvas_request_redraw (item->canvas, c_bbox.x0, c_bbox.y0, c_bbox.x1, c_bbox.y1);
}


static void
recalc_if_needed (GnomeCanvasThumb *image)
{
	if (! image->need_recalc)
		return;
	image->need_recalc = FALSE;

	recalc_bounds (image);
	if (image->gc)
		gdk_gc_set_clip_mask (image->gc, image->mask);
}


static void
gnome_canvas_thumb_draw (GnomeCanvasItem *item, GdkDrawable *drawable,
			  int x, int y, int width, int height)
{
	GtkWidget        *widget = GTK_WIDGET (item->canvas);
	GnomeCanvasThumb *image;
	GtkStateType      state;
	GtkStyle         *style;
	GdkGC            *gc;
	/* GdkGCValues       values; FIXME */
        int               x1, y1;
	int               image_x1, image_y1;

	image = GNOME_CANVAS_THUMB (item);
	recalc_if_needed (image);
	
	x1 = image->cx - x;
	y1 = image->cy - y;

	style = GTK_WIDGET (item->canvas)->style;
	if (GTK_WIDGET_HAS_FOCUS (widget))
                state = GTK_STATE_SELECTED;
        else
                state = GTK_STATE_ACTIVE;
 	gc = image->selected ? style->bg_gc[state] : style->bg_gc[GTK_STATE_INSENSITIVE];

	/**/

	gthumb_draw_slide (x1, y1,
			   image->cwidth, image->cheight,
			   image->iwidth, image->iheight,
			   drawable,
			   gc,
			   style->black_gc,
			   style->dark_gc[GTK_STATE_NORMAL],
			   style->mid_gc[GTK_STATE_NORMAL],
			   style->light_gc[GTK_STATE_NORMAL]);

	image_x1 = x1 + (image->cwidth - image->iwidth) / 2 + 1;
	image_y1 = y1 + (image->cheight - image->iheight) / 2 + 1;

	/* FIXME
	gdk_gc_get_values (gc, &values);
	gthumb_draw_frame (x1 + (image->cwidth - image->iwidth) / 2 + 1, 
			   y1 + (image->cheight - image->iheight) / 2 + 1,
			   image->iwidth, image->iheight,
			   drawable,
			   &values.foreground);
	*/
	   
	if (image->pixmap) {
		if (image->mask) 
			gdk_gc_set_clip_origin (image->gc, image_x1, image_y1);
		
		gdk_draw_drawable (drawable,
				   image->gc,
				   image->pixmap,
				   0, 0,
				   image_x1,
				   image_y1,
				   image->iwidth,
				   image->iheight);
	}
}


static double
gnome_canvas_thumb_point (GnomeCanvasItem *item, 
			   double x, double y,
			   int cx, int cy, 
			   GnomeCanvasItem **actual_item)
{
	GnomeCanvasThumb *image;
	double x1, y1, x2, y2;
	double dx, dy;

	image = GNOME_CANVAS_THUMB (item);

	*actual_item = item;

	recalc_if_needed (image);

	x1 = image->cx - item->canvas->close_enough;
	y1 = image->cy - item->canvas->close_enough;
	x2 = image->cx + image->cwidth - 1 + item->canvas->close_enough;
	y2 = image->cy + image->cheight - 1 + item->canvas->close_enough;

	if ((cx >= x1) && (cy >= y1) && (cx <= x2) && (cy <= y2)) 
		return 0.0; 

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
gnome_canvas_thumb_bounds (GnomeCanvasItem *item, 
			    double *x1, double *y1, 
			    double *x2, double *y2)
{
	GnomeCanvasThumb *image;

	image = GNOME_CANVAS_THUMB (item);

	*x1 = image->x;
	*y1 = image->y;
	*x2 = *x1 + image->width;
	*y2 = *y1 + image->height;
}


static void
gnome_canvas_thumb_render (GnomeCanvasItem *item, GnomeCanvasBuf *buf)
{
}


void
gnome_canvas_thumb_select (GnomeCanvasThumb *image,
			    gboolean select)
{
	g_return_if_fail (image != NULL);
	g_return_if_fail (GNOME_IS_CANVAS_THUMB (image));

	if (image->selected == select)
		return;

	image->selected = select;
	gnome_canvas_item_request_update (GNOME_CANVAS_ITEM (image));
}
