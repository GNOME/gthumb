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

/* Based upon gnome-icon-item.c, the original copyright note follows :
 *
 * Copyright (C) 1998, 1999, 2000, 2001 Free Software Foundation
 * Copyright (C) 2001 Anders Carlsson
 * All rights reserved.
 *
 * This file is part of the Gnome Library.
 *
 * The Gnome Library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * The Gnome Library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with the Gnome Library; see the file COPYING.LIB.  If not,
 * write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 * Author: Anders Carlsson <andersca@gnu.org>
 *
 * Based on the GNOME 1.0 icon item by Miguel de Icaza and Federico Mena.
 */

#include <config.h>
#include <math.h>
#include <libgnome/gnome-macros.h>
#include <gtk/gtkentry.h>
#include <gtk/gtksignal.h>
#include <gtk/gtkwindow.h>
#include <gdk/gdkkeysyms.h>
#include <gdk/gdkrgb.h>
#include <gdk/gdkx.h>
#include <libart_lgpl/art_rgb_affine.h>
#include <libart_lgpl/art_rgb_rgba_affine.h>
#include <string.h>

#include "gthumb-text-item.h"
#include "gthumb-marshal.h"

#define MARGIN_X 2
#define MARGIN_Y 2

#define ROUND(n) (floor ((n) + .5))

/* Private part of the GThumbTextItem structure */
struct _GThumbTextItemPrivate {
	char *markup_str;    /* string used to create the marked text. */
	char *parsed_text;   /**/

	/* Our layout */

	PangoLayout *layout;
	int layout_width, layout_height;
	GtkWidget *entry_win;
	GtkWidget *entry;

	guint selection_start;
	guint min_width;
	guint min_height;

	/* Whether the user pressed the mouse while the item was unselected */
	guint unselected_click : 1;

	/* Whether we need to update the position */
	guint need_pos_update : 1;

	/* Whether we need to update the font */
	guint need_font_update : 1;

	/* Whether we need to update the text */
	guint need_text_update : 1;

	/* Whether we need to update because the editing/selected state
	 * changed */
	guint need_state_update : 1;

	/* Whether selection is occuring */
	guint selecting         : 1;
};

enum {
	TEXT_CHANGED,
	HEIGHT_CHANGED,
	WIDTH_CHANGED,
	EDITING_STARTED,
	EDITING_STOPPED,
	SELECTION_STARTED,
	SELECTION_STOPPED,
	LAST_SIGNAL
};

static guint iti_signals [LAST_SIGNAL] = { 0 };

GNOME_CLASS_BOILERPLATE (GThumbTextItem, gthumb_text_item,
			 GnomeCanvasItem, GNOME_TYPE_CANVAS_ITEM);

/* Updates the pango layout width */
static void
update_pango_layout (GThumbTextItem *iti,
		     gboolean emit)
{
	GThumbTextItemPrivate *priv;
	PangoRectangle bounds;
	int old_width, old_height;

	priv = iti->_priv;

	g_return_if_fail (priv->parsed_text != NULL);

	pango_layout_set_text (priv->layout, priv->parsed_text, strlen (priv->parsed_text));
	pango_layout_set_width (priv->layout, iti->width * PANGO_SCALE);
	pango_layout_get_pixel_extents (priv->layout, NULL, &bounds);

	old_width = priv->layout_width;
	old_height = priv->layout_height;
	priv->layout_width = bounds.width;
	priv->layout_height = bounds.height;

	if (! emit)
		return;

	if (old_width != priv->layout_width)
		g_signal_emit (G_OBJECT (iti), 
			       iti_signals[WIDTH_CHANGED], 
			       0);

	if (old_height != priv->layout_height)
		g_signal_emit (G_OBJECT (iti), 
			       iti_signals[HEIGHT_CHANGED], 
			       0);
}


/* Stops the editing state of an icon text item */
static void
iti_stop_editing (GThumbTextItem *iti)
{
	GThumbTextItemPrivate *priv;

	priv = iti->_priv;
	iti->editing = FALSE;
	update_pango_layout (iti, TRUE);
	gnome_canvas_item_request_update (GNOME_CANVAS_ITEM (iti));
	g_signal_emit (iti, iti_signals[EDITING_STOPPED], 0);
}


static void
update_text (GThumbTextItem  *iti, 
	     const char      *new_text)
{
	GThumbTextItemPrivate *priv = iti->_priv;
	PangoAttrList         *attr_list = NULL;
        GError                *error = NULL;
	char                  *marked_text;
	char                  *parsed_text;
	char                  *utf8_text;

	g_return_if_fail (new_text != NULL);

	utf8_text = g_locale_to_utf8 (new_text, -1, NULL, NULL, NULL);
	g_return_if_fail (utf8_text != NULL);

	if (priv->markup_str != NULL) {
		char *escaped_text = g_markup_escape_text (utf8_text, -1);
		marked_text = g_strdup_printf (priv->markup_str, escaped_text);
		g_free (escaped_text);
	} else
		marked_text = g_markup_escape_text (utf8_text, -1);

	g_free (utf8_text);

	if (! pango_parse_markup (marked_text, -1,
				  0,
				  &attr_list, 
				  &parsed_text, 
				  NULL,
				  &error)) {
		g_warning ("Failed to set text from markup due to error parsing markup: %s\nThis is the text that caused the error : %s",  error->message, new_text);
		g_error_free (error);
		g_free (marked_text);
		return;
	}
	g_free (marked_text);

	if (iti->text != NULL)
		g_free (iti->text);
	iti->text = g_strdup (new_text);

	if (priv->parsed_text != NULL)
		g_free (priv->parsed_text);
	priv->parsed_text = parsed_text;

	pango_layout_set_attributes (priv->layout, attr_list);
        pango_attr_list_unref (attr_list);
}


/* Accepts the text in the off-screen entry of an icon text item */
static void
iti_edition_accept (GThumbTextItem *iti)
{
	GThumbTextItemPrivate *priv;
	gboolean accept;

	priv = iti->_priv;
	accept = TRUE;

	g_signal_emit (iti, iti_signals [TEXT_CHANGED], 0, &accept);

	if (iti->editing){
		if (accept) 
			update_text (iti, gtk_entry_get_text (GTK_ENTRY (priv->entry)));
		iti_stop_editing (iti);
	}
	update_pango_layout (iti, TRUE);
	priv->need_text_update = TRUE;

	gnome_canvas_item_request_update (GNOME_CANVAS_ITEM (iti));
}


/* Ensure the item gets focused (both globally, and local to Gtk) */
static void
iti_ensure_focus (GnomeCanvasItem *item)
{
	GtkWidget *toplevel;

        /* gnome_canvas_item_grab_focus still generates focus out/in
         * events when focused_item == item
         */
        if (GNOME_CANVAS_ITEM (item)->canvas->focused_item != item) {
        	gnome_canvas_item_grab_focus (GNOME_CANVAS_ITEM (item));
        }

	toplevel = gtk_widget_get_toplevel (GTK_WIDGET (item->canvas));
	if (toplevel != NULL && GTK_WIDGET_REALIZED (toplevel)) {
		gdk_window_focus (toplevel->window, GDK_CURRENT_TIME);
	}
}


static void
iti_entry_activate (GtkObject *widget, gpointer data)
{
	GThumbTextItem *item = GTHUMB_TEXT_ITEM (data);
	GThumbTextItemPrivate *priv=item->_priv;

	gtk_widget_hide (priv->entry_win);
	iti_edition_accept (item);
}


static void
gthumb_text_item_realize (GnomeCanvasItem *item)
{
  /*GThumbTextItem *iti = GTHUMB_TEXT_ITEM (item);*/
	GNOME_CALL_PARENT (GNOME_CANVAS_ITEM_CLASS, realize, (item));
}


static void
gthumb_text_item_unrealize (GnomeCanvasItem *item)
{
  /*	GThumbTextItem *iti = GTHUMB_TEXT_ITEM (item);*/
	GNOME_CALL_PARENT (GNOME_CANVAS_ITEM_CLASS, unrealize, (item));
}


/* Starts the editing state of an icon text item */
static void
iti_start_editing (GThumbTextItem *iti)
{
	GThumbTextItemPrivate *priv = iti->_priv;
	double dx, dy;

	if (iti->editing)
		return;

	/* We place an entry offscreen to handle events and selections.  */
	if (priv->entry_win == NULL) {
		priv->entry = gtk_entry_new ();
		g_signal_connect (priv->entry, 
				  "activate",
				  G_CALLBACK (iti_entry_activate), 
				  iti);

		priv->entry_win = gtk_window_new (GTK_WINDOW_POPUP);
		gtk_container_add (GTK_CONTAINER (priv->entry_win),
				   GTK_WIDGET (priv->entry));
	}
	gtk_entry_set_text (GTK_ENTRY (priv->entry), iti->text);
	gtk_editable_select_region (GTK_EDITABLE (priv->entry), 0, -1);
	gtk_widget_grab_focus (priv->entry);

	gnome_canvas_window_to_world (GNOME_CANVAS_ITEM (iti)->canvas, 
				      (double) iti->x, (double) iti->y, 
				      &dx, &dy);
	gtk_window_move (GTK_WINDOW (priv->entry_win), (int) dx, (int) dy);
	gtk_widget_show_all (priv->entry_win);
		
	iti->editing = TRUE;
	priv->need_state_update = TRUE;

	gnome_canvas_item_request_update (GNOME_CANVAS_ITEM (iti));
	g_signal_emit (iti, iti_signals[EDITING_STARTED], 0);
}


/* Recomputes the bounding box of an icon text item */
static void
recompute_bounding_box (GThumbTextItem *iti)
{

	GnomeCanvasItem *item;
	double x1, y1, x2, y2;
	int width_c, height_c;
	int width_w, height_w;
	int max_width_w;

	item = GNOME_CANVAS_ITEM (iti);

	width_c = iti->_priv->layout_width + 2 * MARGIN_X;
	height_c = iti->_priv->layout_height + 2 * MARGIN_Y;
	width_w = ROUND (width_c / item->canvas->pixels_per_unit);
	height_w = ROUND (height_c / item->canvas->pixels_per_unit);
	max_width_w = ROUND (iti->width / item->canvas->pixels_per_unit);

	x1 = iti->x;
	y1 = iti->y;

	gnome_canvas_item_i2w (item, &x1, &y1);

	x1 += ROUND ((max_width_w - width_w) / 2);
	x2 = x1 + width_w;
	y2 = y1 + height_w;

	gnome_canvas_w2c_d (item->canvas, x1, y1, &item->x1, &item->y1);
	gnome_canvas_w2c_d (item->canvas, x2, y2, &item->x2, &item->y2);
}


static void
gthumb_text_item_update (GnomeCanvasItem *item, 
			 double *affine, 
			 ArtSVP *clip_path, 
			 int flags)
{
	GThumbTextItem *iti;
	GThumbTextItemPrivate *priv;

	iti = GTHUMB_TEXT_ITEM (item);
	priv = iti->_priv;
 
	GNOME_CALL_PARENT (GNOME_CANVAS_ITEM_CLASS, update, (item, affine, clip_path, flags));

	/* If necessary, queue a redraw of the old bounding box */
	if ((flags & GNOME_CANVAS_UPDATE_VISIBILITY)
	    || (flags & GNOME_CANVAS_UPDATE_AFFINE)
	    || priv->need_pos_update
	    || priv->need_font_update
	    || priv->need_text_update)
		gnome_canvas_request_redraw (item->canvas, item->x1, item->y1, item->x2, item->y2);

	/* Compute new bounds */
	if (priv->need_pos_update
	    || priv->need_font_update
	    || priv->need_text_update)
		recompute_bounding_box (iti);

	/* Queue redraw */
	gnome_canvas_request_redraw (item->canvas, item->x1, item->y1, item->x2, item->y2);

	priv->need_pos_update = FALSE;
	priv->need_font_update = FALSE;
	priv->need_text_update = FALSE;
	priv->need_state_update = FALSE;
}


/* Draw method handler for the icon text item */
static void
gthumb_text_item_draw (GnomeCanvasItem *item, GdkDrawable *drawable,
		       int x, int y, int width, int height)
{
	GtkWidget *widget;
	GtkStyle *style;
	GdkGC *gc, *bgc;
	int xofs, yofs;
	int text_xofs, text_yofs;
	int w, h;
	GThumbTextItem *iti;
	GThumbTextItemPrivate *priv;
	GtkStateType state;

	widget = GTK_WIDGET (item->canvas);
	iti = GTHUMB_TEXT_ITEM (item);
	priv = iti->_priv;

	style = GTK_WIDGET (GNOME_CANVAS_ITEM (iti)->canvas)->style;

	gc = style->fg_gc [GTK_STATE_NORMAL];
	bgc = style->bg_gc [GTK_STATE_NORMAL];

	w = priv->layout_width + 2 * MARGIN_X;
	h = priv->layout_height + 2 * MARGIN_Y;

	xofs = item->x1 - x;
	yofs = item->y1 - y;

	text_xofs = xofs - (iti->width - priv->layout_width - 2 * MARGIN_X) / 2;
	text_yofs = yofs + MARGIN_Y;


	if (GTK_WIDGET_HAS_FOCUS (widget))
		state = GTK_STATE_SELECTED;
	else
		state = GTK_STATE_ACTIVE;

	if (iti->selected && !iti->editing)
		gdk_draw_rectangle (drawable,
				    style->base_gc[state],
				    TRUE,
				    xofs, yofs,
				    w - 1, h - 1);

	if (GTK_WIDGET_HAS_FOCUS (widget) 
	    && iti->focused 
	    && ! iti->editing) {
		GdkRectangle r;

		r.x = xofs;
		r.y = yofs;
		r.width = w - 1;
		r.height = h - 1;
		
		gtk_paint_focus (style, drawable, widget->state, &r, 
				 widget, NULL,
				 xofs, yofs, w - 1, h - 1);
	}

	if (iti->editing) {
		gdk_draw_rectangle (drawable,
				    style->base_gc[GTK_STATE_NORMAL],
				    TRUE,
				    xofs, yofs, w - 1, h - 1);
		gdk_draw_rectangle (drawable,
				    style->fg_gc[GTK_STATE_NORMAL],
				    FALSE,
				    xofs, yofs, w - 1, h - 1);
	}

	gdk_draw_layout (drawable,
			 style->text_gc[iti->selected ? state : GTK_STATE_NORMAL],
			 text_xofs,
			 text_yofs,
			 priv->layout);
}


static void
draw_pixbuf_aa (GdkPixbuf *pixbuf, 
		GnomeCanvasBuf *buf, 
		double affine[6], 
		int x_offset, 
		int y_offset)
{
	void (* affine_function)
		(art_u8 *dst, int x0, int y0, int x1, int y1, int dst_rowstride,
		 const art_u8 *src, int src_width, int src_height, int src_rowstride,
		 const double affine[6],
		 ArtFilterLevel level,
		 ArtAlphaGamma *alpha_gamma);

	affine[4] += x_offset;
	affine[5] += y_offset;

	affine_function = gdk_pixbuf_get_has_alpha (pixbuf)
		? art_rgb_rgba_affine
		: art_rgb_affine;

	(* affine_function)
		(buf->buf,
		 buf->rect.x0, buf->rect.y0,
		 buf->rect.x1, buf->rect.y1,
		 buf->buf_rowstride,
		 gdk_pixbuf_get_pixels (pixbuf),
		 gdk_pixbuf_get_width (pixbuf),
		 gdk_pixbuf_get_height (pixbuf),
		 gdk_pixbuf_get_rowstride (pixbuf),
		 affine,
		 ART_FILTER_NEAREST,
		 NULL);

	affine[4] -= x_offset;
	affine[5] -= y_offset;
}

static void
gthumb_text_item_render (GnomeCanvasItem *item, GnomeCanvasBuf *buffer)
{
	GdkVisual *visual;
	GdkPixmap *pixmap;
	GdkPixbuf *text_pixbuf;
	double affine[6];
	int width, height;

	visual = gdk_rgb_get_visual ();
	art_affine_identity(affine);
	width  = ROUND (item->x2 - item->x1);
	height = ROUND (item->y2 - item->y1);

	pixmap = gdk_pixmap_new (NULL, width, height, visual->depth);

	gdk_draw_rectangle (pixmap, GTK_WIDGET (item->canvas)->style->white_gc,
			    TRUE,
			    0, 0,
			    width,
			    height);

	/* use a common routine to draw the label into the pixmap */
	gthumb_text_item_draw (item, pixmap,
				   ROUND (item->x1), ROUND (item->y1),
				   width, height);

	/* turn it into a pixbuf */
	text_pixbuf = gdk_pixbuf_get_from_drawable
		(NULL, pixmap, gdk_rgb_get_colormap (),
		 0, 0,
		 0, 0,
		 width,
		 height);

	/* draw the pixbuf containing the label */
	draw_pixbuf_aa (text_pixbuf, buffer, affine, ROUND (item->x1), ROUND (item->y1));
	g_object_unref (text_pixbuf);

	buffer->is_bg = FALSE;
	buffer->is_buf = TRUE;
}


/* Bounds method handler for the icon text item */
static void
gthumb_text_item_bounds (GnomeCanvasItem *item, double *x1, double *y1, double *x2, double *y2)
{
	GThumbTextItem *iti;
	GThumbTextItemPrivate *priv;
	int width, height;

	iti = GTHUMB_TEXT_ITEM (item);
	priv = iti->_priv;

	width = priv->layout_width + 2 * MARGIN_X;
	height = priv->layout_height + 2 * MARGIN_Y;

	*x1 = iti->x + (iti->width - width) / 2;
	*y1 = iti->y;
	*x2 = *x1 + width;
	*y2 = *y1 + height;
}


/* Point method handler for the icon text item */
static double
gthumb_text_item_point (GnomeCanvasItem *item, double x, double y, int cx, int cy, GnomeCanvasItem **actual_item)
{
	double dx, dy;

	*actual_item = item;

	if (cx < item->x1)
		dx = item->x1 - cx;
	else if (cx > item->x2)
		dx = cx - item->x2;
	else
		dx = 0.0;

	if (cy < item->y1)
		dy = item->y1 - cy;
	else if (cy > item->y2)
		dy = cy - item->y2;
	else
		dy = 0.0;

	return sqrt (dx * dx + dy * dy);
}


static gboolean
gthumb_text_item_event (GnomeCanvasItem *item, GdkEvent *event)
{
	GThumbTextItem *iti;
	GThumbTextItemPrivate *priv;

	iti = GTHUMB_TEXT_ITEM (item);
	priv = iti->_priv;

	switch (event->type) {
	  /*
	case GDK_KEY_EVENT:
		return FALSE;
	  */		
	case GDK_BUTTON_PRESS:
		return FALSE;

	case GDK_MOTION_NOTIFY:
		return FALSE;

	case GDK_BUTTON_RELEASE:
		return FALSE;
	default:
		break;
	}

	return FALSE;
}

static void
gthumb_text_item_destroy (GtkObject *object)
{
	GThumbTextItem  *iti = GTHUMB_TEXT_ITEM (object);
	GnomeCanvasItem *item = GNOME_CANVAS_ITEM (object);

	gnome_canvas_request_redraw (item->canvas,
				     ROUND (item->x1),
				     ROUND (item->y1),
				     ROUND (item->x2),
				     ROUND (item->y2));

	if (iti->text != NULL) {
		g_free (iti->text);
		iti->text = NULL;
	}

	if (iti->fontname) {
		g_free (iti->fontname);
		iti->fontname = NULL;
	}

	if (iti->_priv != NULL) {
		GThumbTextItemPrivate *priv = iti->_priv;

		if (priv->markup_str != NULL) {
			g_free (priv->markup_str);
			priv->markup_str = NULL;
		}

		if (priv->parsed_text != NULL) {
			g_free (priv->parsed_text);
			priv->parsed_text = NULL;
		}

		if (priv->layout) {
			g_object_unref (priv->layout);
			priv->layout = NULL;
		}
		
		if (priv->entry_win) {
			gtk_widget_destroy (priv->entry_win);
			priv->entry_win = NULL;
		}
		
		g_free (iti->_priv);
		iti->_priv = NULL;
	}

	GNOME_CALL_PARENT (GTK_OBJECT_CLASS, destroy, (object));
}

static void
gthumb_text_item_class_init (GThumbTextItemClass *klass)
{
	GnomeCanvasItemClass *canvas_item_class;
	GtkObjectClass *object_class;

	object_class = (GtkObjectClass *)klass;
	canvas_item_class = (GnomeCanvasItemClass *)klass;

	iti_signals [TEXT_CHANGED] =
		g_signal_new ("text_changed",
			      G_TYPE_FROM_CLASS (object_class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (GThumbTextItemClass, text_changed),
			      NULL, NULL,
			      gthumb_marshal_BOOLEAN__VOID,
			      G_TYPE_BOOLEAN, 0);

	iti_signals [HEIGHT_CHANGED] =
		g_signal_new ("height_changed",
			      G_TYPE_FROM_CLASS (object_class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (GThumbTextItemClass, height_changed),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__VOID,
			      G_TYPE_NONE, 0);

	iti_signals [WIDTH_CHANGED] =
		g_signal_new ("width_changed",
			      G_TYPE_FROM_CLASS (object_class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (GThumbTextItemClass, width_changed),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__VOID,
			      G_TYPE_NONE, 0);

	iti_signals[EDITING_STARTED] =
		g_signal_new ("editing_started",
			      G_TYPE_FROM_CLASS (object_class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (GThumbTextItemClass, editing_started),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__VOID,
			      G_TYPE_NONE, 0);

	iti_signals[EDITING_STOPPED] =
		g_signal_new ("editing_stopped",
			      G_TYPE_FROM_CLASS (object_class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (GThumbTextItemClass, editing_stopped),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__VOID,
			      G_TYPE_NONE, 0);

	object_class->destroy = gthumb_text_item_destroy;

	canvas_item_class->update = gthumb_text_item_update;
	canvas_item_class->draw = gthumb_text_item_draw;
	canvas_item_class->render = gthumb_text_item_render;
	canvas_item_class->bounds = gthumb_text_item_bounds;
	canvas_item_class->point = gthumb_text_item_point;
	canvas_item_class->event = gthumb_text_item_event;
	canvas_item_class->realize = gthumb_text_item_realize;
	canvas_item_class->unrealize = gthumb_text_item_unrealize;
}


static void
gthumb_text_item_instance_init (GThumbTextItem *item)
{
	item->_priv = g_new0 (GThumbTextItemPrivate, 1);
	item->fontname = NULL;
	item->text = NULL;
}


/**
 * gthumb_text_item_configure:
 * @iti: An icon text item.
 * @x: X position in which to place the item.
 * @y: Y position in which to place the item.
 * @width: Maximum width allowed for this item, to be used for word wrapping.
 * @fontname: Name of the fontset that should be used to display the text.
 * @text: Text that is going to be displayed.
 * @is_editable: Deprecated.
 * @is_static: Whether @text points to a static string or not.
 *
 * This routine is used to configure a &GThumbTextItem.
 *
 * @x and @y specify the cordinates where the item is placed inside the canvas.
 * The @x coordinate should be the leftmost position that the icon text item can
 * assume at any one time, that is, the left margin of the column in which the
 * icon is to be placed.  The @y coordinate specifies the top of the icon text
 * item.
 *
 * @width is the maximum width allowed for this icon text item.  The coordinates
 * define the upper-left corner of an icon text item with maximum width; this may
 * actually be outside the bounding box of the item if the text is narrower than
 * the maximum width.
 *
 * If @is_static is true, it means that there is no need for the item to
 * allocate memory for the string (it is a guarantee that the text is allocated
 * by the caller and it will not be deallocated during the lifetime of this
 * item).  This is an optimization to reduce memory usage for large icon sets.
 */
void
gthumb_text_item_configure (GThumbTextItem *iti, 
			    int x, 
			    int y,
			    int width, 
			    const char *fontname,
			    const char *text,
			    gboolean is_editable, 
			    gboolean is_static)
{
	GThumbTextItemPrivate *priv = iti->_priv;
 
	g_return_if_fail (GTHUMB_IS_TEXT_ITEM (iti));
	g_return_if_fail (width > 2 * MARGIN_X);
	g_return_if_fail (text != NULL);

	iti->x = x;
	iti->y = y;
	iti->width = width;
	iti->is_editable = is_editable != FALSE;

	if (iti->editing)
		iti_stop_editing (iti);

	if (iti->fontname)
		g_free (iti->fontname);
	iti->fontname = (fontname != NULL) ? g_strdup (fontname) : NULL;

	/* Create our new PangoLayout */

	if (priv->layout != NULL)
		g_object_unref (priv->layout);
	priv->layout = gtk_widget_create_pango_layout (GTK_WIDGET (GNOME_CANVAS_ITEM (iti)->canvas), NULL);

	pango_layout_set_wrap (priv->layout, PANGO_WRAP_CHAR);
	pango_layout_set_font_description (priv->layout, GTK_WIDGET (GNOME_CANVAS_ITEM (iti)->canvas)->style->font_desc);
	pango_layout_set_alignment (priv->layout, PANGO_ALIGN_CENTER);

	update_text (iti, text);
	update_pango_layout (iti, TRUE);

	priv->need_pos_update = TRUE;
	priv->need_font_update = TRUE;
	priv->need_text_update = TRUE;
	gnome_canvas_item_request_update (GNOME_CANVAS_ITEM (iti)); 
}


/**
 * gthumb_text_item_set_markup_str:
 * @iti: 
 * @markup_str: 
 * 
 * 
 **/
void
gthumb_text_item_set_markup_str (GThumbTextItem *iti,
				 char           *markup_str)
{
	g_return_if_fail (GTHUMB_IS_TEXT_ITEM (iti));

	if (iti->_priv->markup_str != NULL)
		g_free (iti->_priv->markup_str);
	iti->_priv->markup_str = g_strdup (markup_str);
}


/**
 * gthumb_text_item_setxy:
 * @iti:  An icon text item.
 * @x: X position.
 * @y: Y position.
 *
 * Sets the coordinates at which the icon text item should be placed.
 *
 * See also: gthumb_text_item_configure().
 */
void
gthumb_text_item_setxy (GThumbTextItem *iti, int x, int y)
{
	GThumbTextItemPrivate *priv;

	g_return_if_fail (GTHUMB_IS_TEXT_ITEM (iti));

	priv = iti->_priv;

	iti->x = x;
	iti->y = y;

	priv->need_pos_update = TRUE;
	gnome_canvas_item_request_update (GNOME_CANVAS_ITEM (iti));
}


void
gthumb_text_item_setxy_width (GThumbTextItem *iti,
			      int             x, 
			      int             y,
			      int             width)
{
	GThumbTextItemPrivate *priv;

	g_return_if_fail (GTHUMB_IS_TEXT_ITEM (iti));

	priv = iti->_priv;

	iti->x = x;
	iti->y = y;
	iti->width = width;

	update_pango_layout (iti, FALSE);
	priv->need_pos_update = TRUE;
	priv->need_text_update = TRUE;
	gnome_canvas_item_request_update (GNOME_CANVAS_ITEM (iti));
}


void
gthumb_text_item_set_width (GThumbTextItem *iti,
			    int             width)
{
	GThumbTextItemPrivate *priv;

	g_return_if_fail (GTHUMB_IS_TEXT_ITEM (iti));

	priv = iti->_priv;

	iti->width = width;

	update_pango_layout (iti, FALSE);
	priv->need_pos_update = TRUE;
	priv->need_text_update = TRUE;
	gnome_canvas_item_request_update (GNOME_CANVAS_ITEM (iti));
}


void
gthumb_text_item_focus (GThumbTextItem *iti,
			gboolean focused)
{
	GThumbTextItemPrivate *priv;

	g_return_if_fail (GTHUMB_IS_TEXT_ITEM (iti));

	priv = iti->_priv;

	if (!iti->focused == !focused)
		return;

	iti->focused = focused ? TRUE : FALSE;

	priv->need_state_update = TRUE;
	gnome_canvas_item_request_update (GNOME_CANVAS_ITEM (iti));
}


/**
 * gthumb_text_item_select:
 * @iti: An icon text item
 * @sel: Whether the icon text item should be displayed as selected.
 *
 * This function is used to control whether an icon text item is displayed as
 * selected or not.  Mouse events are ignored by the item when it is 
 * unselected;
 * when the user clicks on a selected icon text item, it will start the text
 * editing process.
 */
void
gthumb_text_item_select (GThumbTextItem *iti, gboolean sel)
{
	GThumbTextItemPrivate *priv;

	g_return_if_fail (GTHUMB_IS_TEXT_ITEM (iti));

	priv = iti->_priv;

	if (!iti->selected == !sel)
		return;

	iti->selected = sel ? TRUE : FALSE;

#ifdef FIXME
	if (!iti->selected && iti->editing)
		iti_edition_accept (iti);
#endif
	priv->need_state_update = TRUE;
	gnome_canvas_item_request_update (GNOME_CANVAS_ITEM (iti));
}


/**
 * gthumb_text_item_get_text:
 * @iti: An icon text item.
 *
 * Returns the current text string in an icon text item.  The client should not
 * free this string, as it is internal to the icon text item.
 */
const char *
gthumb_text_item_get_text (GThumbTextItem *iti)
{
	GThumbTextItemPrivate *priv;

	g_return_val_if_fail (GTHUMB_IS_TEXT_ITEM (iti), NULL);

	priv = iti->_priv;

	if (iti->editing) {
		return gtk_entry_get_text (GTK_ENTRY (priv->entry));
	} else {
		return iti->text;
	}
}


int
gthumb_text_item_get_text_width (GThumbTextItem *iti)
{
	GThumbTextItemPrivate *priv;
	priv = iti->_priv;
	return priv->layout_width;
}


int
gthumb_text_item_get_text_height (GThumbTextItem *iti)
{
	GThumbTextItemPrivate *priv;
	priv = iti->_priv;
	return priv->layout_height;
}


/**
 * gthumb_text_item_start_editing:
 * @iti: An icon text item.
 *
 * Starts the editing state of an icon text item.
 **/
void
gthumb_text_item_start_editing (GThumbTextItem *iti)
{
	g_return_if_fail (GTHUMB_IS_TEXT_ITEM (iti));

	if (iti->editing)
		return;

	iti->selected = TRUE; /* Ensure that we are selected */
	iti_ensure_focus (GNOME_CANVAS_ITEM (iti));
	iti_start_editing (iti);
}


/**
 * gthumb_text_item_stop_editing:
 * @iti: An icon text item.
 * @accept: Whether to accept the current text or to discard it.
 *
 * Terminates the editing state of an icon text item.  The @accept argument
 * controls whether the item's current text should be accepted or discarded.
 * If it is discarded, then the icon's original text will be restored.
 **/
void
gthumb_text_item_stop_editing (GThumbTextItem *iti,
			       gboolean accept)
{
	g_return_if_fail (GTHUMB_IS_TEXT_ITEM (iti));

	if (!iti->editing)
		return;

	if (accept)
		iti_edition_accept (iti);
	else
		iti_stop_editing (iti);
}
