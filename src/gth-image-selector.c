/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2003 Free Software Foundation, Inc.
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

#include <stdlib.h>
#include <math.h>
#include "gth-image-selector.h"
#include "gthumb-marshal.h"
#include "glib-utils.h"
#include "pixbuf-utils.h"


#define PREVIEW_SIZE 300
#define DRAG_THRESHOLD 1
#define BORDER 3
#define BORDER2 (BORDER * 2)


typedef struct {
	int           ref_count;
	int           id;
	GdkRectangle  area;
	GdkCursor    *cursor;
} EventArea;


static EventArea *
event_area_new (int           id,
		GdkCursorType cursor_type)
{
	EventArea *event_area;

	event_area = g_new0 (EventArea, 1);

	event_area->ref_count = 1;
	event_area->id = id;
	event_area->area.x = 0;
	event_area->area.y = 0;
	event_area->area.width = 0;
	event_area->area.height = 0;
	event_area->cursor = gdk_cursor_new_for_display (gdk_display_get_default (), cursor_type);

	return event_area;
}


/* FIXME
static void
event_area_ref (EventArea *event_area)
{
	event_area->ref_count++;
}
*/


static void
event_area_unref (EventArea *event_area)
{
	event_area->ref_count--;

	if (event_area->ref_count > 0) 
		return;

	if (event_area->cursor != NULL)
		gdk_cursor_unref (event_area->cursor);
	g_free (event_area);
}


 /**/


enum {
	C_SELECTION_AREA,
	C_TOP_AREA,
	C_BOTTOM_AREA,
	C_LEFT_AREA,
	C_RIGHT_AREA,
	C_TOP_LEFT_AREA,
	C_TOP_RIGHT_AREA,
	C_BOTTOM_LEFT_AREA,
	C_BOTTOM_RIGHT_AREA
};

enum {
	SELECTION_CHANGED,
	LAST_SIGNAL
};


static GtkWidgetClass *parent_class = NULL;
static guint signals[LAST_SIGNAL] = { 0 };


struct _GthImageSelectorPriv {
	GdkPixbuf     *pixbuf, *background;
	int            pixbuf_width, pixbuf_height;
	GdkRectangle   image_area;

	gboolean       use_ratio;
	double         ratio;
	double         zoom;
	double         drag_start_x, drag_start_y;
	double         drag_x, drag_y;
	gboolean       pressed;
	gboolean       double_click;
	gboolean       dragging;

	GdkGC         *selection_gc;
	GdkRectangle   selection_area;
	GdkRectangle   drag_start_selection_area;

	GdkCursor     *default_cursor;
	GList         *event_list;
	EventArea     *current_area;
};


static void
add_event_area (GthImageSelector *selector,
		int               area_id,
		GdkCursorType     cursor_type)
{
	GthImageSelectorPriv *priv = selector->priv;
	EventArea            *event_area;

	event_area = event_area_new (area_id, cursor_type);
	priv->event_list = g_list_prepend (priv->event_list, event_area);
}


static void
free_event_area_list (GthImageSelector *selector)
{
	GthImageSelectorPriv *priv = selector->priv;

	if (priv->event_list != NULL) {
		g_list_foreach (priv->event_list, 
				(GFunc) event_area_unref, 
				NULL);
		g_list_free (priv->event_list);
		priv->event_list = NULL;
	}
}


static gboolean
point_in_rectangle (int          x,
		    int          y,
		    GdkRectangle rect)
{
	return ((x >= rect.x) 
		&& (x <= rect.x + rect.width)
		&& (y >= rect.y) 
		&& (y <= rect.y + rect.height));
}


static gboolean
rectangle_in_rectangle (GdkRectangle r1,
			GdkRectangle r2)
{
	return (point_in_rectangle (r1.x, r1.y, r2)
		&& point_in_rectangle (r1.x + r1.width, 
				       r1.y + r1.height, 
				       r2));
}


static EventArea*
get_event_area_from_position (GthImageSelector *selector,
			      int x,
			      int y)
{
	GthImageSelectorPriv *priv = selector->priv;
	GList                *scan;

	for (scan = priv->event_list; scan; scan = scan->next) {
		EventArea    *event_area = scan->data;
		GdkRectangle  widget_area;

		widget_area = event_area->area;
		widget_area.x += priv->image_area.x;
		widget_area.y += priv->image_area.y;

		if (point_in_rectangle (x, y, widget_area))
			return event_area;
	}

	return NULL;
}


static EventArea*
get_event_area_from_id (GthImageSelector *selector,
			int         event_id)
{
	GthImageSelectorPriv *priv = selector->priv;
	GList                *scan;

	for (scan = priv->event_list; scan; scan = scan->next) {
		EventArea *event_area = scan->data;
		if (event_area->id == event_id)
			return event_area;
	}

	return NULL;
}


 /**/


static void
update_event_areas (GthImageSelector *selector)
{
	GthImageSelectorPriv *priv = selector->priv;
	EventArea            *event_area;
	int                   x, y, width, height;

	if (! GTK_WIDGET_REALIZED (selector))
		return;

	x = priv->selection_area.x + 1;
	y = priv->selection_area.y + 1;
	width = priv->selection_area.width - 2;
	height = priv->selection_area.height - 2;

	event_area = get_event_area_from_id (selector, C_SELECTION_AREA);
	event_area->area.x = x + BORDER;
	event_area->area.y = y + BORDER;
	event_area->area.width = width - BORDER2;
	event_area->area.height = height - BORDER2;

	event_area = get_event_area_from_id (selector, C_TOP_AREA);
	event_area->area.x = x + BORDER;
	event_area->area.y = y - BORDER;
	event_area->area.width = width - BORDER2;
	event_area->area.height = BORDER2;

	event_area = get_event_area_from_id (selector, C_BOTTOM_AREA);
	event_area->area.x = x + BORDER;
	event_area->area.y = y + height - BORDER;
	event_area->area.width = width - BORDER2;
	event_area->area.height = BORDER2;

	event_area = get_event_area_from_id (selector, C_LEFT_AREA);
	event_area->area.x = x - BORDER;
	event_area->area.y = y + BORDER;
	event_area->area.width = BORDER2;
	event_area->area.height = height - BORDER2;

	event_area = get_event_area_from_id (selector, C_RIGHT_AREA);
	event_area->area.x = x + width - BORDER;
	event_area->area.y = y + BORDER;
	event_area->area.width = BORDER2;
	event_area->area.height = height - BORDER2;

	event_area = get_event_area_from_id (selector, C_TOP_LEFT_AREA);
	event_area->area.x = x - BORDER;
	event_area->area.y = y - BORDER;
	event_area->area.width = BORDER2;
	event_area->area.height = BORDER2;

	event_area = get_event_area_from_id (selector, C_TOP_RIGHT_AREA);
	event_area->area.x = x + width - BORDER;
	event_area->area.y = y - BORDER;
	event_area->area.width = BORDER2;
	event_area->area.height = BORDER2;

	event_area = get_event_area_from_id (selector, C_BOTTOM_LEFT_AREA);
	event_area->area.x = x - BORDER;
	event_area->area.y = y + height - BORDER;
	event_area->area.width = BORDER2;
	event_area->area.height = BORDER2;

	event_area = get_event_area_from_id (selector, C_BOTTOM_RIGHT_AREA);
	event_area->area.x = x + width - BORDER;
	event_area->area.y = y + height - BORDER;
	event_area->area.width = BORDER2;
	event_area->area.height = BORDER2;
}


static void
selection_changed (GthImageSelector *selector)
{
	update_event_areas (selector);
	g_signal_emit (G_OBJECT (selector), 
		       signals[SELECTION_CHANGED],
		       0);
}


static gboolean
expose (GtkWidget      *widget, 
	GdkEventExpose *event)
{
	GthImageSelector     *selector = GTH_IMAGE_SELECTOR (widget);
	GthImageSelectorPriv *priv = selector->priv;
	GdkRectangle          exposed_area;

	if ((priv->background != NULL) 
	    && gdk_rectangle_intersect (&priv->image_area, 
					&event->area, 
					&exposed_area))
		gdk_draw_pixbuf (widget->window,
				 widget->style->black_gc,
				 priv->background,
				 exposed_area.x - priv->image_area.x,
				 exposed_area.y - priv->image_area.y,
				 exposed_area.x,
				 exposed_area.y,
				 exposed_area.width,
				 exposed_area.height,
				 GDK_RGB_DITHER_NORMAL,
				 0, 0);

	exposed_area = priv->selection_area;
	exposed_area.x += priv->image_area.x;
	exposed_area.y += priv->image_area.y;
	if (gdk_rectangle_intersect (&event->area, 
				     &exposed_area, 
				     &exposed_area)) {
		gdk_draw_pixbuf (widget->window,
				 widget->style->black_gc,
				 priv->pixbuf,
				 exposed_area.x - priv->image_area.x,
				 exposed_area.y - priv->image_area.y,
				 exposed_area.x,
				 exposed_area.y,
				 exposed_area.width,
				 exposed_area.height,
				 GDK_RGB_DITHER_NORMAL,
				 0, 0);
		
		/* Paint outline */

		gdk_gc_set_clip_rectangle (priv->selection_gc, &exposed_area);
		gdk_draw_rectangle (widget->window,
				    priv->selection_gc,
				    FALSE,
				    priv->selection_area.x + priv->image_area.x,
				    priv->selection_area.y + priv->image_area.y,
				    priv->selection_area.width - 1,
				    priv->selection_area.height - 1);
	}

#if 0 /* FIXME */
	{
		GList *scan;
		for (scan = priv->event_list; scan; scan = scan->next) {
			EventArea *event_area = scan->data;
			GdkRectangle area;

			area = event_area->area;
			area.x += priv->image_area.x;
			area.y += priv->image_area.y;

			gdk_draw_rectangle (widget->window,
					    widget->style->black_gc,
					    FALSE,
					    area.x,
					    area.y,
					    area.width,
					    area.height);
		}
	}
#endif

	return FALSE;
}


static gboolean
button_press (GtkWidget      *widget, 
	      GdkEventButton *event)
{
	GthImageSelector     *selector = GTH_IMAGE_SELECTOR (widget);
	GthImageSelectorPriv *priv = selector->priv;

	if (! GTK_WIDGET_HAS_FOCUS (widget)) {
		gtk_widget_grab_focus (widget);
	}

	if (priv->dragging)
		return FALSE;

	if ((event->type == GDK_2BUTTON_PRESS) || 
	    (event->type == GDK_3BUTTON_PRESS)) {
		priv->double_click = TRUE;
		return FALSE;
	} else
		priv->double_click = FALSE;

	if (event->button == 1) {
		GdkModifierType mods;
		int             x, y;

		gdk_window_get_pointer (widget->window, &x, &y, &mods);
		priv->drag_start_x = x;
		priv->drag_start_y = y;
		priv->drag_start_selection_area = priv->selection_area;
		priv->pressed = TRUE;

		return TRUE;
	}

	return FALSE;
}


static void
update_cursor (GthImageSelector *selector,
	       int               x,
	       int               y)
{
	GtkWidget            *widget = GTK_WIDGET (selector);
	GthImageSelectorPriv *priv = selector->priv;
	EventArea            *event_area;

	event_area = get_event_area_from_position (selector, x, y);

	if (priv->current_area != event_area) 
		priv->current_area = event_area;
		
	if (priv->current_area == NULL)
		gdk_window_set_cursor (widget->window,
				       priv->default_cursor);
	else
		gdk_window_set_cursor (widget->window,
				       priv->current_area->cursor);
}


static gboolean
button_release  (GtkWidget      *widget, 
		 GdkEventButton *event)
{
	GthImageSelector     *selector = GTH_IMAGE_SELECTOR (widget);
	GthImageSelectorPriv *priv = selector->priv;

	if (priv->dragging)
		gdk_pointer_ungrab (event->time);
	priv->dragging = FALSE;
	priv->pressed = FALSE;
	update_cursor (selector, priv->drag_x, priv->drag_y);

	return FALSE;
}


static void
queue_draw (GthImageSelector *selector,
	    GdkRectangle      area)
{
	if (! GTK_WIDGET_REALIZED (selector))
		return;

	gtk_widget_queue_draw_area (
		    GTK_WIDGET (selector),
		    selector->priv->image_area.x + area.x - BORDER,
		    selector->priv->image_area.y + area.y - BORDER,
		    area.width + BORDER2,
		    area.height + BORDER2);
}


static gboolean
rectangle_equal (GdkRectangle r1,
		 GdkRectangle r2)
{
	return ((r1.x == r2.x) 
		&& (r1.y == r2.y) 
		&& (r1.width == r2.width)
		&& (r1.height == r2.height));
}


static void
set_selection (GthImageSelector *selector,
	       GdkRectangle      new_selection)
{
	GthImageSelectorPriv *priv = selector->priv;
	GdkRectangle          old_selection;

	if (rectangle_equal (priv->selection_area, new_selection))
		return;

	old_selection = priv->selection_area;
	priv->selection_area = new_selection;
	queue_draw (selector, old_selection);
	queue_draw (selector, priv->selection_area);

	selection_changed (selector);
}


static void
grow_upward (GdkRectangle *bound,
	     GdkRectangle *r,
	     int           dy,
	     gboolean      check)
{
	if (check && (r->y + dy < 0))
		dy = -r->y;
	r->y += dy;
	r->height -= dy;
}


static void
grow_downward (GdkRectangle *bound,
	       GdkRectangle *r,
	       int           dy,
	       gboolean      check)
{
	if (check && (r->y + r->height + dy > bound->height))
		dy = bound->height - (r->y + r->height);
	r->height += dy;
}


static void
grow_leftward (GdkRectangle *bound,
	       GdkRectangle *r,
	       int           dx,
	       gboolean      check)
{
	if (check && (r->x + dx < 0))
		dx = -r->x;
	r->x += dx;
	r->width -= dx;
}


static void
grow_rightward (GdkRectangle *bound,
		GdkRectangle *r,
		int           dx,
		gboolean      check)
{
	if (check && (r->x + r->width + dx > bound->width))
		dx = bound->width - (r->x + r->width);
	r->width += dx;
}


static int
get_semiplane_no (int x1,
		  int y1,
		  int x2,
		  int y2,
		  int px,
		  int py)
{
	double a, b;

	a = atan ((double) (y1 - y2) / (x2 - x1));
	b = atan ((double) (y1 - py) / (px - x1));

	return (a + G_PI >= b) && (b >= a);
}


static void
check_and_select (GthImageSelector *selector,
		  GdkRectangle      new_selection)
{
	GthImageSelectorPriv *priv = selector->priv;

	if (((priv->current_area == NULL)
	     || (priv->current_area->id != C_SELECTION_AREA))
	    && priv->use_ratio) {
		new_selection.x += priv->image_area.x;
		new_selection.y += priv->image_area.y;
		if (rectangle_in_rectangle (new_selection, priv->image_area)) {
			new_selection.x -= priv->image_area.x;
			new_selection.y -= priv->image_area.y;
			set_selection (selector, new_selection);
		}
		return;
	} 

	if (new_selection.x < 0) 
		new_selection.x = 0;
	if (new_selection.y < 0)
		new_selection.y = 0;
	if (new_selection.width > priv->image_area.width) 
		new_selection.width = priv->image_area.width;
	if (new_selection.height > priv->image_area.height) 
		new_selection.height = priv->image_area.height;
	
	if (new_selection.x + new_selection.width > priv->image_area.width)
		new_selection.x = priv->image_area.width - new_selection.width;
	if (new_selection.y + new_selection.height > priv->image_area.height)
		new_selection.y = priv->image_area.height - new_selection.height;
	set_selection (selector, new_selection);
}


static int
motion_notify (GtkWidget      *widget, 
	       GdkEventMotion *event)
{
	GthImageSelector     *selector = GTH_IMAGE_SELECTOR (widget);
	GthImageSelectorPriv *priv = selector->priv;
	GdkModifierType       mods;
	int                   x, y;
	GdkRectangle          new_selection, tmp;
	int                   dx, dy;
	int                   semiplane;
	gboolean              check = !priv->use_ratio;

	gdk_window_get_pointer (widget->window, &x, &y, &mods);

	priv->drag_x = x;
	priv->drag_y = y;

	dx = priv->drag_x - priv->drag_start_x;
	dy = priv->drag_y - priv->drag_start_y;

	if (! priv->dragging
	    && priv->pressed
	    && ((abs (dx) > DRAG_THRESHOLD) || (abs (dy) > DRAG_THRESHOLD))
	    && (priv->current_area != NULL)) {
		int retval;

		retval = gdk_pointer_grab (widget->window,
					   FALSE,
					   (GDK_POINTER_MOTION_MASK
					    | GDK_POINTER_MOTION_HINT_MASK
					    | GDK_BUTTON_RELEASE_MASK),
					   NULL,
					   priv->current_area->cursor,
					   event->time);
		if (retval == 0)
			priv->dragging = TRUE;

		return FALSE;
	} 

	if (! priv->dragging) {
		update_cursor (selector, priv->drag_x, priv->drag_y);
		return FALSE;
	}

	/* dragging == TRUE */

	new_selection = priv->drag_start_selection_area;

	switch (priv->current_area->id) {
	case C_SELECTION_AREA:
		new_selection.x += dx;
		new_selection.y += dy;
		break;
	case C_TOP_AREA:
		grow_upward (&priv->image_area, &new_selection, dy, check);
		if (priv->use_ratio)
			grow_rightward (&priv->image_area, 
					&new_selection, 
					-dy * priv->ratio,
					check);
		break;
	case C_BOTTOM_AREA:
		grow_downward (&priv->image_area, &new_selection, dy, check);
		if (priv->use_ratio)
			grow_leftward (&priv->image_area, 
				       &new_selection, 
				       -dy * priv->ratio, 
				       check);
		break;
	case C_LEFT_AREA:
		grow_leftward (&priv->image_area, &new_selection, dx, check);
		if (priv->use_ratio)
			grow_downward (&priv->image_area, 
				       &new_selection, 
				       -dx / priv->ratio, 
				       check);
		break;
	case C_RIGHT_AREA:
		grow_rightward (&priv->image_area, &new_selection, dx, check);
		if (priv->use_ratio)
			grow_upward (&priv->image_area, 
				     &new_selection, 
				     -dx / priv->ratio, 
				     check);
		break;
	case C_TOP_LEFT_AREA:
		if (priv->use_ratio) {
			tmp = priv->drag_start_selection_area;
			semiplane = get_semiplane_no (
				      tmp.x + tmp.width,
				      tmp.y + tmp.height,
				      tmp.x,
				      tmp.y,
				      priv->drag_x - priv->image_area.x,
				      priv->drag_y - priv->image_area.y);
			if (semiplane == 1) 
				dy = dx / priv->ratio;
			else 
				dx = dy * priv->ratio;
		} 
		grow_upward (&priv->image_area, &new_selection, dy, check);
		grow_leftward (&priv->image_area, &new_selection, dx, check);
		break;
	case C_TOP_RIGHT_AREA:
		if (priv->use_ratio) {
			tmp = priv->drag_start_selection_area;
			semiplane = get_semiplane_no (
				      tmp.x,
				      tmp.y + tmp.height,
				      tmp.x + tmp.width,
				      tmp.y,
				      priv->drag_x - priv->image_area.x,
				      priv->drag_y - priv->image_area.y);
			if (semiplane == 1) 
				dx = -dy * priv->ratio;
			else 
				dy = -dx / priv->ratio;
		} 
		grow_upward (&priv->image_area, &new_selection, dy, check);
		grow_rightward (&priv->image_area, &new_selection, dx, check);
		break;
	case C_BOTTOM_LEFT_AREA:
		if (priv->use_ratio) {
			tmp = priv->drag_start_selection_area;
			semiplane = get_semiplane_no (
				      tmp.x + tmp.width,
				      tmp.y,
				      tmp.x,
				      tmp.y + tmp.height,
				      priv->drag_x - priv->image_area.x,
				      priv->drag_y - priv->image_area.y);
			if (semiplane == 1) 
				dx = -dy * priv->ratio;
			else 
				dy = -dx / priv->ratio;
		} 
		grow_downward (&priv->image_area, &new_selection, dy, check);
		grow_leftward (&priv->image_area, &new_selection, dx, check);
		break;
	case C_BOTTOM_RIGHT_AREA:
		if (priv->use_ratio) {
			tmp = priv->drag_start_selection_area;
			semiplane = get_semiplane_no (
				      tmp.x,
				      tmp.y,
				      tmp.x + tmp.width,
				      tmp.y + tmp.height,
				      priv->drag_x - priv->image_area.x,
				      priv->drag_y - priv->image_area.y);
			if (semiplane == 1) 
				dy = dx / priv->ratio;
			else 
				dx = dy * priv->ratio;
		} 
		grow_downward (&priv->image_area, &new_selection, dy, check);
		grow_rightward (&priv->image_area, &new_selection, dx, check);
		break;
	default:
		break;
	}

	check_and_select (selector, new_selection);
	return FALSE;
}


static void
finalize (GObject *object)
{
	GthImageSelector *selector;

        g_return_if_fail (GTH_IS_IMAGE_SELECTOR (object));
  
	selector = GTH_IMAGE_SELECTOR (object);

	if (selector->priv != NULL) {
		GthImageSelectorPriv *priv = selector->priv;

		if (priv->pixbuf != NULL) {
			g_object_unref (priv->pixbuf);
			priv->pixbuf = NULL;
		}

		if (priv->background != NULL) {
			g_object_unref (priv->background);
			priv->background = NULL;
		}

		g_free (selector->priv);
		selector->priv = NULL;
	}

        /* Chain up */
	G_OBJECT_CLASS (parent_class)->finalize (object);
}


static void 
realize (GtkWidget *widget)
{
	GthImageSelector     *selector;
	GthImageSelectorPriv *priv;
	GdkWindowAttr         attributes;
	int                   attributes_mask;
 
	g_return_if_fail (GTH_IS_IMAGE_SELECTOR (widget));

	selector = GTH_IMAGE_SELECTOR (widget);
	priv = selector->priv;

	GTK_WIDGET_SET_FLAGS (widget, GTK_REALIZED);

	attributes.window_type = GDK_WINDOW_CHILD;
	attributes.x           = widget->allocation.x;
	attributes.y           = widget->allocation.y;
	attributes.width       = widget->allocation.width;
	attributes.height      = widget->allocation.height;
	attributes.wclass      = GDK_INPUT_OUTPUT;
	attributes.visual      = gtk_widget_get_visual (widget);
	attributes.colormap    = gtk_widget_get_colormap (widget);
	attributes.event_mask  = (gtk_widget_get_events (widget)
				  | GDK_EXPOSURE_MASK
				  | GDK_BUTTON_PRESS_MASK 
				  | GDK_BUTTON_RELEASE_MASK 
				  | GDK_POINTER_MOTION_MASK 
				  | GDK_POINTER_MOTION_HINT_MASK 
				  | GDK_BUTTON_MOTION_MASK);
	attributes_mask        = (GDK_WA_X 
				  | GDK_WA_Y 
				  | GDK_WA_VISUAL 
				  | GDK_WA_COLORMAP);
	widget->window = gdk_window_new (gtk_widget_get_parent_window (widget),
					 &attributes, 
					 attributes_mask);
	gdk_window_set_user_data (widget->window, selector);

	widget->style = gtk_style_attach (widget->style, widget->window);
	gtk_style_set_background (widget->style, widget->window, GTK_STATE_NORMAL);

	/**/

	priv->selection_gc = gdk_gc_new (widget->window);
	gdk_gc_copy (priv->selection_gc, widget->style->white_gc);

	priv->default_cursor = gdk_cursor_new_for_display (gdk_display_get_default (), GDK_LEFT_PTR);
	gdk_window_set_cursor (widget->window, priv->default_cursor);

	add_event_area (selector, C_SELECTION_AREA, GDK_FLEUR);
	add_event_area (selector, C_TOP_AREA, GDK_TOP_SIDE);
	add_event_area (selector, C_BOTTOM_AREA, GDK_BOTTOM_SIDE);
	add_event_area (selector, C_LEFT_AREA, GDK_LEFT_SIDE);
	add_event_area (selector, C_RIGHT_AREA, GDK_RIGHT_SIDE);
	add_event_area (selector, C_TOP_LEFT_AREA, GDK_TOP_LEFT_CORNER);
	add_event_area (selector, C_TOP_RIGHT_AREA, GDK_TOP_RIGHT_CORNER);
	add_event_area (selector, C_BOTTOM_LEFT_AREA, GDK_BOTTOM_LEFT_CORNER);
	add_event_area (selector, C_BOTTOM_RIGHT_AREA, GDK_BOTTOM_RIGHT_CORNER);
		
	update_event_areas (selector);
}


static void 
unrealize (GtkWidget *widget)
{
	GthImageSelector     *selector;
	GthImageSelectorPriv *priv;

        g_return_if_fail (GTH_IS_IMAGE_SELECTOR (widget));

	selector = GTH_IMAGE_SELECTOR (widget);
	priv = selector->priv;

	if (priv != NULL) {
		if (priv->default_cursor != NULL) {
			gdk_cursor_unref (priv->default_cursor);
			priv->default_cursor = NULL;
		}
		
		if (priv->selection_gc != NULL) {
			g_object_unref (priv->selection_gc);
			priv->selection_gc = NULL;
		}
		
		free_event_area_list (selector);
	}

	GTK_WIDGET_CLASS (parent_class)->unrealize (widget);
}


static void
size_request (GtkWidget      *widget,
	      GtkRequisition *requisition)
{
	GthImageSelector     *selector = GTH_IMAGE_SELECTOR (widget);
	GthImageSelectorPriv *priv = selector->priv;

	if (priv->pixbuf != NULL) {
		widget->requisition.width = gdk_pixbuf_get_width (priv->pixbuf);
		widget->requisition.height = gdk_pixbuf_get_height (priv->pixbuf);
	}

	/* Chain up to default that simply reads current requisition */
	GTK_WIDGET_CLASS (parent_class)->size_request (widget, requisition);
}


static void 
size_allocate (GtkWidget       *widget, 
	       GtkAllocation   *allocation)
{
	GthImageSelector     *selector;
	GthImageSelectorPriv *priv;

	selector = GTH_IMAGE_SELECTOR (widget);
	priv = selector->priv;

	widget->allocation = *allocation;

	if (GTK_WIDGET_REALIZED (widget)) {
		gdk_window_move_resize (widget->window,
					allocation->x, allocation->y,
					allocation->width, allocation->height);
	}

	if (priv->pixbuf == NULL) {
		priv->image_area.x = 0;
		priv->image_area.y = 0;
		priv->image_area.width = 0;
		priv->image_area.height = 0;
		return;
	}
	
	priv->image_area.width = gdk_pixbuf_get_width (priv->pixbuf);
	priv->image_area.height = gdk_pixbuf_get_height (priv->pixbuf);
	priv->image_area.x = (allocation->width - priv->image_area.width) / 2.0;
	priv->image_area.y = (allocation->height - priv->image_area.height) / 2.0;

	selection_changed (selector);
}


static gint 
focus_in (GtkWidget     *widget,
	  GdkEventFocus *event)
{
	GTK_WIDGET_SET_FLAGS (widget, GTK_HAS_FOCUS);
	gtk_widget_queue_draw (widget);
	return TRUE;
}


static gint 
focus_out (GtkWidget     *widget,
	   GdkEventFocus *event)
{
	GTK_WIDGET_UNSET_FLAGS (widget, GTK_HAS_FOCUS);
	gtk_widget_queue_draw (widget);
	return TRUE;
}


static void
class_init (GthImageSelectorClass *class)
{
	GObjectClass   *gobject_class;
	GtkWidgetClass *widget_class;
	
	parent_class = g_type_class_peek_parent (class);
	widget_class = (GtkWidgetClass*) class;
	gobject_class = (GObjectClass*) class;

	signals[SELECTION_CHANGED] = g_signal_new (
			 "selection_changed",
			 G_TYPE_FROM_CLASS (class),
			 G_SIGNAL_RUN_LAST,
			 G_STRUCT_OFFSET (GthImageSelectorClass, selection_changed),
			 NULL, NULL,
			 gthumb_marshal_VOID__VOID,
			 G_TYPE_NONE, 
			 0);
		
	gobject_class->finalize = finalize;

	widget_class->realize         = realize;
	widget_class->unrealize       = unrealize;
	widget_class->size_request    = size_request;
	widget_class->size_allocate   = size_allocate;
	widget_class->focus_in_event  = focus_in;
	widget_class->focus_out_event = focus_out;

	widget_class->expose_event         = expose;
	widget_class->button_press_event   = button_press;
	widget_class->button_release_event = button_release;
	widget_class->motion_notify_event  = motion_notify;

	class->selection_changed = NULL;
}


static void
init (GthImageSelector *selector)
{
	GTK_WIDGET_SET_FLAGS (selector, GTK_CAN_FOCUS);
	selector->priv = g_new0 (GthImageSelectorPriv, 1);
	selector->priv->zoom = 1.0;
	selector->priv->ratio = 1.0;
}





GType
gth_image_selector_get_type ()
{
        static guint type = 0;

        if (! type) {
                GTypeInfo type_info = {
			sizeof (GthImageSelectorClass),
			NULL,
			NULL,
			(GClassInitFunc) class_init,
			NULL,
			NULL,
			sizeof (GthImageSelector),
			0,
			(GInstanceInitFunc) init
		};

		type = g_type_register_static (GTK_TYPE_WIDGET,
					       "GthImageSelector",
					       &type_info,
					       0);
	}

        return type;
}


GtkWidget*     
gth_image_selector_new (GdkPixbuf *pixbuf)
{
	GthImageSelector *selector;

	selector = (GthImageSelector*) g_object_new (GTH_IMAGE_SELECTOR_TYPE, NULL);
	gth_image_selector_set_pixbuf (selector, pixbuf);

	return (GtkWidget*) selector;
}


void
gth_image_selector_set_pixbuf (GthImageSelector *selector, 
			       GdkPixbuf        *pixbuf)
{
	GthImageSelectorPriv *priv = selector->priv;
	int                   width, height;
	GdkRectangle          area;

	if (priv->pixbuf != NULL)
		g_object_unref (priv->pixbuf);
	priv->pixbuf = NULL;

	if (priv->background != NULL)
		g_object_unref (priv->background);
	priv->background = NULL;

	if (pixbuf == NULL) {
		priv->pixbuf_width = 0;
		priv->pixbuf_height = 0;
		priv->zoom = 1.0;
		return;
	}

	priv->pixbuf_width = width = gdk_pixbuf_get_width (pixbuf);
	priv->pixbuf_height = height = gdk_pixbuf_get_height (pixbuf);
	if (scale_keepping_ratio (&width, 
				  &height, 
				  PREVIEW_SIZE, 
				  PREVIEW_SIZE)) {
		priv->zoom = MIN ((double)PREVIEW_SIZE / gdk_pixbuf_get_width (pixbuf), (double)PREVIEW_SIZE / gdk_pixbuf_get_height (pixbuf));
		priv->pixbuf = gdk_pixbuf_scale_simple (pixbuf, width, height, GDK_INTERP_BILINEAR);
	} else {
		priv->zoom = 1.0;
		priv->pixbuf = pixbuf;
		g_object_ref (priv->pixbuf);
	}

	priv->background = gdk_pixbuf_composite_color_simple (
			      priv->pixbuf,
			      gdk_pixbuf_get_width (priv->pixbuf),
			      gdk_pixbuf_get_height (priv->pixbuf),
			      GDK_INTERP_NEAREST,
			      128,
			      10,
			      0x00000000,
			      0x00000000);

	area.x = width * 0.25;
	area.y = height * 0.25;
	area.width = width * 0.50;
	area.height = height * 0.50;
	set_selection (selector, area);
}


GdkPixbuf*
gth_image_selector_get_pixbuf (GthImageSelector *selector)
{
	return selector->priv->pixbuf;
}


static int
real_to_selector (GthImageSelector *selector,
		  int               value)
{
	return (int) floor (selector->priv->zoom * value + 0.5);
}


void
gth_image_selector_set_selection_width (GthImageSelector *selector,
					int               width)
{
	GthImageSelectorPriv *priv = selector->priv;
	GdkRectangle          area;

	area = priv->selection_area;
	area.width = real_to_selector (selector, width);
	if (priv->use_ratio)
		area.height = area.width / priv->ratio;
	check_and_select (selector, area);
}


void
gth_image_selector_set_selection_height (GthImageSelector *selector,
					 int               height)
{
	GthImageSelectorPriv *priv = selector->priv;
	GdkRectangle          area;

	area = priv->selection_area;
	area.height = real_to_selector (selector, height);
	if (priv->use_ratio)
		area.width = area.height / priv->ratio;
	check_and_select (selector, area);
}


void
gth_image_selector_set_selection (GthImageSelector *selector,
				  GdkRectangle      selection)
{
	GthImageSelectorPriv *priv = selector->priv;

	selection.x = real_to_selector (selector, selection.x);
	selection.y = real_to_selector (selector, selection.y);
	selection.width = real_to_selector (selector, selection.width);
	selection.height = real_to_selector (selector, selection.height);

	if (! rectangle_equal (priv->selection_area, selection))
		check_and_select (selector, selection);
}


static int
selector_to_real (GthImageSelector *selector,
		  int               value)
{
	return (int) floor ((double) value / selector->priv->zoom + 0.5);
}


void
gth_image_selector_get_selection (GthImageSelector *selector,
				  GdkRectangle     *selection)
{
	GthImageSelectorPriv *priv = selector->priv;

	selection->x = selector_to_real (selector, priv->selection_area.x);
	selection->y = selector_to_real (selector, priv->selection_area.y);
	selection->width = selector_to_real (selector, priv->selection_area.width);
	selection->height = selector_to_real (selector, priv->selection_area.height);

	selection->x = MAX (selection->x, 0);
	selection->y = MAX (selection->y, 0);
	selection->width = MIN (selection->width, priv->pixbuf_width);
	selection->height = MIN (selection->height, priv->pixbuf_height);
}


void
gth_image_selector_set_ratio (GthImageSelector *selector,
			      gboolean          use_ratio,
			      double            ratio)
{
	GthImageSelectorPriv *priv = selector->priv;
	GdkRectangle          area;

	priv->use_ratio = use_ratio;
	priv->ratio = ratio;

	if (! priv->use_ratio)
		return;

	if (priv->ratio > 1.0) {
		area.width = priv->image_area.width * 0.5;
		area.height = area.width / priv->ratio;
	} else {
		area.height = priv->image_area.height * 0.5;
		area.width = area.height * priv->ratio;
	}

	area.x = (priv->image_area.width - area.width) / 2.0;
	area.y = (priv->image_area.height - area.height) / 2.0;
	check_and_select (selector, area);
}


double
gth_image_selector_get_ratio (GthImageSelector *selector)
{
	return selector->priv->ratio;
}


gboolean
gth_image_selector_get_use_ratio (GthImageSelector *selector)
{
	return selector->priv->use_ratio;
}
