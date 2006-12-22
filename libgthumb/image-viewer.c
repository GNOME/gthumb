/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2001 Free Software Foundation, Inc.
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

#include <glib/gi18n.h>
#include <gdk/gdkkeysyms.h>
#include <gtk/gtkadjustment.h>
#include <gtk/gtkhscrollbar.h>
#include <gtk/gtkvscrollbar.h>
#include <libgnomevfs/gnome-vfs-types.h>
#include <libgnomevfs/gnome-vfs-uri.h>
#include <libgnomevfs/gnome-vfs-utils.h>

#include "gth-iviewer.h"
#include "image-viewer.h"
#include "cursors.h"
#include "pixbuf-utils.h"
#include "gthumb-marshal.h"
#include "glib-utils.h"

#define COLOR_GRAY_00   0x00000000
#define COLOR_GRAY_33   0x00333333
#define COLOR_GRAY_66   0x00666666
#define COLOR_GRAY_99   0x00999999
#define COLOR_GRAY_CC   0x00cccccc
#define COLOR_GRAY_FF   0x00ffffff

#define DRAG_THRESHOLD  1     /* When dragging the image ignores movements
			       * smaller than this value. */
#define MINIMUM_DELAY   10    /* When an animation frame has a 0 milli seconds
			       * delay use this delay instead. */
#define STEP_INCREMENT  20.0  /* Scroll increment. */

#define FLOAT_EQUAL(a,b) (fabs (a - b) < 1e-6)

enum {
	CLICKED,
	IMAGE_LOADED,
	ZOOM_CHANGED,
	REPAINTED,
	MOUSE_WHEEL_SCROLL,
	SCROLL,
	LAST_SIGNAL
};

static void     image_viewer_class_init       (ImageViewerClass *class);
static void     image_viewer_init             (ImageViewer      *viewer);
static gboolean image_viewer_expose           (GtkWidget        *widget,
					       GdkEventExpose   *event);
static gboolean image_viewer_button_press     (GtkWidget        *widget,
					       GdkEventButton   *event);
static gboolean image_viewer_button_release   (GtkWidget        *widget,
					       GdkEventButton   *event);
static gboolean image_viewer_motion_notify    (GtkWidget        *widget,
					       GdkEventMotion   *event);
static void     image_viewer_finalize         (GObject          *viewer);
static void     image_viewer_realize          (GtkWidget        *widget);
static void     image_viewer_unrealize        (GtkWidget        *widget);
static void     image_viewer_size_allocate    (GtkWidget        *widget,
					       GtkAllocation    *allocation);
static gint     image_viewer_focus_in         (GtkWidget        *widget,
					       GdkEventFocus    *event);
static gint     image_viewer_focus_out        (GtkWidget        *widget,
					       GdkEventFocus    *event);
static gint     image_viewer_scroll_event     (GtkWidget        *widget,
					       GdkEventScroll   *event);
static void     image_viewer_style_set        (GtkWidget        *widget,
					       GtkStyle         *previous_style);
static void     get_zoomed_size               (ImageViewer      *viewer,
					       gint             *width,
					       gint             *height,
					       gdouble           zoom_level);
static void     scroll_to                     (ImageViewer      *viewer,
					       gint             *x_offset,
					       gint             *y_offset);
static void     set_scroll_adjustments        (GtkWidget        *widget,
                                               GtkAdjustment    *hadj,
					       GtkAdjustment    *vadj);
static void     scroll_signal                 (GtkWidget        *widget,
					       GtkScrollType     xscroll_type,
					       GtkScrollType     yscroll_type);
static void     halt_animation                (ImageViewer      *viewer);

static GtkWidgetClass *parent_class = NULL;
static guint image_viewer_signals[LAST_SIGNAL] = { 0 };


static gint
image_viewer_key_press (GtkWidget *widget,
			GdkEventKey *event)
{
	gboolean handled;

	handled = gtk_bindings_activate (GTK_OBJECT (widget),
					 event->keyval,
					 event->state);
	if (handled)
		return TRUE;

	if (GTK_WIDGET_CLASS (parent_class)->key_press_event &&
	    GTK_WIDGET_CLASS (parent_class)->key_press_event (widget, event))
		return TRUE;

	return FALSE;
}


static void
image_viewer_class_init (ImageViewerClass *class)
{
	GObjectClass   *gobject_class;
	GtkWidgetClass *widget_class;
	GtkBindingSet  *binding_set;

	parent_class = g_type_class_peek_parent (class);
	widget_class = (GtkWidgetClass*) class;
	gobject_class = (GObjectClass*) class;

	image_viewer_signals[CLICKED] =
		g_signal_new ("clicked",
			      G_TYPE_FROM_CLASS (class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (ImageViewerClass, clicked),
			      NULL, NULL,
			      gthumb_marshal_VOID__VOID,
			      G_TYPE_NONE,
			      0);
	image_viewer_signals[IMAGE_LOADED] =
                g_signal_new ("image_loaded",
			      G_TYPE_FROM_CLASS (class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (ImageViewerClass, image_loaded),
			      NULL, NULL,
			      gthumb_marshal_VOID__VOID,
			      G_TYPE_NONE,
			      0);
	image_viewer_signals[ZOOM_CHANGED] =
		g_signal_new ("zoom_changed",
			      G_TYPE_FROM_CLASS (class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (ImageViewerClass, zoom_changed),
			      NULL, NULL,
			      gthumb_marshal_VOID__VOID,
			      G_TYPE_NONE,
			      0);
	image_viewer_signals[REPAINTED] =
		g_signal_new ("repainted",
			      G_TYPE_FROM_CLASS (class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (ImageViewerClass, repainted),
			      NULL, NULL,
			      gthumb_marshal_VOID__VOID,
			      G_TYPE_NONE,
			      0);
        image_viewer_signals[MOUSE_WHEEL_SCROLL] =
		g_signal_new ("mouse_wheel_scroll",
			      G_TYPE_FROM_CLASS (class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (ImageViewerClass, mouse_wheel_scroll),
			      NULL, NULL,
			      gthumb_marshal_VOID__ENUM,
			      G_TYPE_NONE,
			      1, GDK_TYPE_SCROLL_DIRECTION);

	class->set_scroll_adjustments = set_scroll_adjustments;
        widget_class->set_scroll_adjustments_signal =
		g_signal_new ("set_scroll_adjustments",
			      G_TYPE_FROM_CLASS (class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (ImageViewerClass, set_scroll_adjustments),
			      NULL, NULL,
			      gthumb_marshal_VOID__POINTER_POINTER,
			      G_TYPE_NONE,
			      2, GTK_TYPE_ADJUSTMENT, GTK_TYPE_ADJUSTMENT);
	image_viewer_signals[SCROLL] =
		g_signal_new ("scroll",
			      G_TYPE_FROM_CLASS (class),
			      G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
			      G_STRUCT_OFFSET (ImageViewerClass, scroll),
			      NULL, NULL,
			      gthumb_marshal_VOID__ENUM_ENUM,
			      G_TYPE_NONE,
			      2, GTK_TYPE_SCROLL_TYPE, GTK_TYPE_SCROLL_TYPE);

	gobject_class->finalize = image_viewer_finalize;

	widget_class->realize         = image_viewer_realize;
	widget_class->unrealize       = image_viewer_unrealize;
	widget_class->size_allocate   = image_viewer_size_allocate;
	widget_class->focus_in_event  = image_viewer_focus_in;
	widget_class->focus_out_event = image_viewer_focus_out;
	widget_class->key_press_event = image_viewer_key_press;

	widget_class->expose_event = image_viewer_expose;
	widget_class->button_press_event = image_viewer_button_press;
	widget_class->button_release_event = image_viewer_button_release;
	widget_class->motion_notify_event = image_viewer_motion_notify;

	widget_class->scroll_event = image_viewer_scroll_event;
	widget_class->style_set    = image_viewer_style_set;

	class->clicked      = NULL;
	class->image_loaded = NULL;
	class->zoom_changed = NULL;
	class->scroll       = scroll_signal;

	/* Add key bindings */

        binding_set = gtk_binding_set_by_class (class);

	gtk_binding_entry_add_signal (binding_set, GDK_Right, 0,
			      "scroll", 2,
			      GTK_TYPE_SCROLL_TYPE, GTK_SCROLL_STEP_RIGHT,
			      GTK_TYPE_SCROLL_TYPE, GTK_SCROLL_NONE);
	gtk_binding_entry_add_signal (binding_set, GDK_Left, 0,
			      "scroll", 2,
			      GTK_TYPE_SCROLL_TYPE, GTK_SCROLL_STEP_LEFT,
			      GTK_TYPE_SCROLL_TYPE, GTK_SCROLL_NONE);
	gtk_binding_entry_add_signal (binding_set, GDK_Down, 0,
			      "scroll", 2,
			      GTK_TYPE_SCROLL_TYPE, GTK_SCROLL_NONE,
			      GTK_TYPE_SCROLL_TYPE, GTK_SCROLL_STEP_DOWN);
	gtk_binding_entry_add_signal (binding_set, GDK_Up, 0,
			      "scroll", 2,
			      GTK_TYPE_SCROLL_TYPE, GTK_SCROLL_NONE,
			      GTK_TYPE_SCROLL_TYPE, GTK_SCROLL_STEP_UP);

	gtk_binding_entry_add_signal (binding_set, GDK_Right, GDK_SHIFT_MASK,
			      "scroll", 2,
			      GTK_TYPE_SCROLL_TYPE, GTK_SCROLL_PAGE_RIGHT,
			      GTK_TYPE_SCROLL_TYPE, GTK_SCROLL_NONE);
	gtk_binding_entry_add_signal (binding_set, GDK_Left, GDK_SHIFT_MASK,
			      "scroll", 2,
			      GTK_TYPE_SCROLL_TYPE, GTK_SCROLL_PAGE_LEFT,
			      GTK_TYPE_SCROLL_TYPE, GTK_SCROLL_NONE);
	gtk_binding_entry_add_signal (binding_set, GDK_Down, GDK_SHIFT_MASK,
			      "scroll", 2,
			      GTK_TYPE_SCROLL_TYPE, GTK_SCROLL_NONE,
			      GTK_TYPE_SCROLL_TYPE, GTK_SCROLL_PAGE_DOWN);
	gtk_binding_entry_add_signal (binding_set, GDK_Up, GDK_SHIFT_MASK,
			      "scroll", 2,
			      GTK_TYPE_SCROLL_TYPE, GTK_SCROLL_NONE,
			      GTK_TYPE_SCROLL_TYPE, GTK_SCROLL_PAGE_UP);

	gtk_binding_entry_add_signal (binding_set, GDK_Page_Down, 0,
			      "scroll", 2,
			      GTK_TYPE_SCROLL_TYPE, GTK_SCROLL_NONE,
			      GTK_TYPE_SCROLL_TYPE, GTK_SCROLL_PAGE_DOWN);
	gtk_binding_entry_add_signal (binding_set, GDK_Page_Up, 0,
			      "scroll", 2,
			      GTK_TYPE_SCROLL_TYPE, GTK_SCROLL_NONE,
			      GTK_TYPE_SCROLL_TYPE, GTK_SCROLL_PAGE_UP);
}


static void image_loaded (ImageLoader* il, gpointer data);
static void image_error  (ImageLoader *il, gpointer data);


static gboolean
hadj_value_changed (GtkObject   *adj,
		    ImageViewer *viewer)
{
	int x_ofs, y_ofs;

	x_ofs = (gint) GTK_ADJUSTMENT (adj)->value;
	y_ofs = viewer->y_offset;
	scroll_to (viewer, &x_ofs, &y_ofs);

	return FALSE;
}


static gboolean
vadj_value_changed (GtkObject   *adj,
		    ImageViewer *viewer)
{
	int x_ofs, y_ofs;

	x_ofs = viewer->x_offset;
	y_ofs = (gint) GTK_ADJUSTMENT (adj)->value;
	scroll_to (viewer, &x_ofs, &y_ofs);

	return FALSE;
}


static void
image_viewer_init (ImageViewer *viewer)
{
	GTK_WIDGET_SET_FLAGS (viewer, GTK_CAN_FOCUS);
	GTK_WIDGET_UNSET_FLAGS (viewer, GTK_DOUBLE_BUFFERED);

	/* Initialize data. */

	viewer->check_type = GTH_CHECK_TYPE_MIDTONE;
	viewer->check_size = GTH_CHECK_SIZE_LARGE;
	viewer->check_color1 = COLOR_GRAY_66;
	viewer->check_color2 = COLOR_GRAY_99;

	viewer->is_animation = FALSE;
	viewer->play_animation = TRUE;
	viewer->rendering = FALSE;
	viewer->cursor_visible = TRUE;

	viewer->frame_visible = TRUE;
	viewer->frame_border = FRAME_BORDER;
	viewer->frame_border2 = FRAME_BORDER2;

	viewer->frame_pixbuf = NULL;
	viewer->frame_delay = 0;
	viewer->anim_id = 0;

	viewer->loader = IMAGE_LOADER (image_loader_new (NULL, TRUE));
	g_signal_connect (G_OBJECT (viewer->loader),
			  "image_done",
			  G_CALLBACK (image_loaded),
			  viewer);
	g_signal_connect (G_OBJECT (viewer->loader),
			  "image_error",
			  G_CALLBACK (image_error),
			  viewer);

	viewer->anim = NULL;
	viewer->iter = NULL;

	viewer->zoom_level = 1.0;
	viewer->zoom_quality = GTH_ZOOM_QUALITY_HIGH;
	viewer->zoom_change = GTH_ZOOM_CHANGE_KEEP_PREV;
	viewer->fit = GTH_FIT_SIZE_IF_LARGER;
	viewer->doing_zoom_fit = FALSE;

	viewer->skip_zoom_change = FALSE;
	viewer->skip_size_change = FALSE;
	viewer->next_scroll_repaint = FALSE;

	viewer->is_void = TRUE;
	viewer->x_offset = 0;
	viewer->y_offset = 0;
	viewer->dragging = FALSE;
	viewer->double_click = FALSE;
	viewer->just_focused = FALSE;

	viewer->black_bg = FALSE;

	viewer->area_pixbuf = NULL;
	viewer->area_max_width = 0;
	viewer->area_max_height = 0;
	viewer->area_bps = 0;
	viewer->area_color_space = GDK_COLORSPACE_RGB;

	viewer->cursor = NULL;
	viewer->cursor_void = NULL;

	viewer->reset_scrollbars = TRUE;

	/* Create the widget. */

	viewer->hadj = GTK_ADJUSTMENT (gtk_adjustment_new (0.0, 1.0, 0.0,
							   1.0, 1.0, 1.0));
	viewer->vadj = GTK_ADJUSTMENT (gtk_adjustment_new (0.0, 1.0, 0.0,
							   1.0, 1.0, 1.0));

	g_object_ref (viewer->hadj);
	gtk_object_sink (GTK_OBJECT (viewer->hadj));
	g_object_ref (viewer->vadj);
	gtk_object_sink (GTK_OBJECT (viewer->vadj));

	g_signal_connect (G_OBJECT (viewer->hadj),
			  "value_changed",
			  G_CALLBACK (hadj_value_changed),
			  viewer);
	g_signal_connect (G_OBJECT (viewer->vadj),
			  "value_changed",
			  G_CALLBACK (vadj_value_changed),
			  viewer);
}


static void
paint (ImageViewer *viewer,
       gint         src_x,
       gint         src_y,
       gint         dest_x,
       gint         dest_y,
       gint         width,
       gint         height,
       gint         interp_type)
{
	GdkPixbuf     *pixbuf;
	double         zoom_level;
	int            bits_per_sample;
	GdkColorspace  color_space;

	pixbuf = image_viewer_get_current_pixbuf (viewer);
	zoom_level = viewer->zoom_level;

	color_space = gdk_pixbuf_get_colorspace (pixbuf);
	bits_per_sample = gdk_pixbuf_get_bits_per_sample (pixbuf);

	if ((viewer->area_pixbuf == NULL)
	    || (viewer->area_max_width < width)
	    || (viewer->area_max_height < height)
	    || (viewer->area_bps != bits_per_sample)
	    || (viewer->area_color_space != color_space)) {
		if (viewer->area_pixbuf != NULL)
			g_object_unref (viewer->area_pixbuf);
		viewer->area_pixbuf = gdk_pixbuf_new (color_space,
						      FALSE,
						      bits_per_sample,
						      width,
						      height);
		g_return_if_fail (viewer->area_pixbuf != NULL);

		viewer->area_max_width = width;
		viewer->area_max_height = height;
		viewer->area_color_space = color_space;
		viewer->area_bps = bits_per_sample;
	}

	if (gdk_pixbuf_get_has_alpha (pixbuf))
		gdk_pixbuf_composite_color (pixbuf,
					    viewer->area_pixbuf,
					    0, 0,
					    width, height,
					    (double) -src_x,
					    (double) -src_y,
					    zoom_level,
					    zoom_level,
					    interp_type,
					    255,
					    src_x, src_y,
					    viewer->check_size,
					    viewer->check_color1,
					    viewer->check_color2);
	else {
		gdk_pixbuf_scale (pixbuf,
				  viewer->area_pixbuf,
				  0, 0,
				  width, height,
				  (double) -src_x,
				  (double) -src_y,
				  zoom_level,
				  zoom_level,
				  interp_type);
	}

	gdk_draw_rgb_image_dithalign (GTK_WIDGET (viewer)->window,
				      GTK_WIDGET (viewer)->style->black_gc,
				      dest_x, dest_y,
				      width, height,
				      GDK_RGB_DITHER_MAX,
				      gdk_pixbuf_get_pixels (viewer->area_pixbuf),
				      gdk_pixbuf_get_rowstride (viewer->area_pixbuf),
				      dest_x, dest_y);

#if 0
	gdk_draw_rectangle (GTK_WIDGET (viewer)->window,
			    GTK_WIDGET (viewer)->style->black_gc,
			    FALSE,
			    dest_x, dest_y,
			    width, height);
#endif
}


static gboolean change_frame_cb (gpointer data);


static void
add_change_frame_timeout (ImageViewer *viewer)
{
	if (! viewer->is_void
	    && viewer->is_animation
	    && viewer->play_animation
	    && (viewer->anim_id == 0))
		viewer->anim_id = g_timeout_add (
			MAX (MINIMUM_DELAY, viewer->frame_delay),
			change_frame_cb,
			viewer);
}


static gint
image_viewer_expose (GtkWidget      *widget,
		     GdkEventExpose *event)
{
	ImageViewer  *viewer;
	int           src_x, src_y;    /* Start to draw the image from this
					* point. */
	int           pixbuf_width;    /* Zoomed size of the image. */
	int           pixbuf_height;
	GdkRectangle  image_area;      /* Inside this rectangle there is the
					* visible part of the image. */
	GdkRectangle  paint_area;      /* The intersection between image_area
					* and event->area. */
	int           gdk_width;
	int           gdk_height;
	int           alloc_width;
	int           alloc_height;

	viewer = IMAGE_VIEWER (widget);

	if (viewer->rendering)
		return FALSE;

	viewer->rendering = TRUE;

	get_zoomed_size (viewer, &pixbuf_width, &pixbuf_height,
			 viewer->zoom_level);

	src_x        = viewer->x_offset;
	src_y        = viewer->y_offset;
	alloc_width  = widget->allocation.width;
	alloc_height = widget->allocation.height;
	gdk_width    = alloc_width - viewer->frame_border2;
	gdk_height   = alloc_height - viewer->frame_border2;

	image_area.x      = MAX (viewer->frame_border, (gdk_width - pixbuf_width) / 2);
	image_area.y      = MAX (viewer->frame_border, (gdk_height - pixbuf_height) / 2);
	image_area.width  = MIN (pixbuf_width, gdk_width);
	image_area.height = MIN (pixbuf_height, gdk_height);

	/* Draw the background. */

	if ((image_area.x > viewer->frame_border)
	    || (image_area.y > viewer->frame_border)
	    || (image_area.width < gdk_width)
	    || (image_area.height < gdk_height)) {
		int    rx, ry, rw, rh;
		GdkGC *gc;

		if (viewer->black_bg)
			gc = widget->style->black_gc;
		else
			gc = widget->style->bg_gc[GTK_STATE_NORMAL];

		if (image_viewer_get_current_pixbuf (viewer) == NULL) {
			gdk_draw_rectangle (widget->window,
					    gc,
					    TRUE,
					    0, 0,
					    alloc_width, alloc_height);

		} else {
			/* If an image is present draw in four phases to avoid
			 * flickering. */

			/* Top rectangle. */
			rx = 0;
			ry = 0;
			rw = alloc_width;
			rh = image_area.y;
			if ((rw > 0) && (rh > 0))
				gdk_draw_rectangle (widget->window,
						    gc,
						    TRUE,
						    rx, ry,
						    rw, rh);

			/* Bottom rectangle. */
			rx = 0;
			ry = image_area.y + image_area.height;
			rw = alloc_width;
			rh = alloc_height - image_area.y - image_area.height;
			if ((rw > 0) && (rh > 0))
				gdk_draw_rectangle (widget->window,
						    gc,
						    TRUE,
						    rx, ry,
						    rw, rh);

			/* Left rectangle. */
			rx = 0;
			ry = image_area.y - 1;
			rw = image_area.x;
			rh = image_area.height + 2;
			if ((rw > 0) && (rh > 0))
				gdk_draw_rectangle (widget->window,
						    gc,
						    TRUE,
						    rx, ry,
						    rw, rh);

			/* Right rectangle. */
			rx = image_area.x + image_area.width;
			ry = image_area.y - 1;
			rw = alloc_width - image_area.x - image_area.width;
			rh = image_area.height + 2;
			if ((rw > 0) && (rh > 0))
				gdk_draw_rectangle (widget->window,
						    gc,
						    TRUE,
						    rx, ry,
						    rw, rh);
		}
	}

	/* Draw the frame. */

	if ((viewer->frame_border > 0) && (image_viewer_get_current_pixbuf (viewer) != NULL)) {
		int    x1, y1, x2, y2;
		GdkGC *gc;

		if (viewer->black_bg)
			gc = widget->style->black_gc;
		else
			gc = widget->style->light_gc[GTK_STATE_NORMAL];
			/*gc = widget->style->dark_gc[GTK_STATE_NORMAL];*/

		x1 = image_area.x + image_area.width;
		y1 = image_area.y - 1;
		x2 = image_area.x + image_area.width;
		y2 = image_area.y + image_area.height;
		gdk_draw_line (widget->window,
			       gc,
			       x1, y1,
			       x2, y2);

		x1 = image_area.x - 1;
		y1 = image_area.y + image_area.height;
		x2 = image_area.x + image_area.width;
		y2 = image_area.y + image_area.height;
		gdk_draw_line (widget->window,
			       gc,
			       x1, y1,
			       x2, y2);

		if (viewer->black_bg)
			gc = widget->style->black_gc;
		else
			gc = widget->style->dark_gc[GTK_STATE_NORMAL];

		x1 = image_area.x - 1;
		y1 = image_area.y - 1;
		x2 = image_area.x - 1;
		y2 = image_area.y + image_area.height;
		gdk_draw_line (widget->window,
			       gc,
			       x1, y1,
			       x2, y2);

		x1 = image_area.x - 1;
		y1 = image_area.y - 1;
		x2 = image_area.x + image_area.width;
		y2 = image_area.y - 1;
		gdk_draw_line (widget->window,
			       gc,
			       x1, y1,
			       x2, y2);
	}

	if ((image_viewer_get_current_pixbuf (viewer) != NULL)
	    && gdk_rectangle_intersect (&event->area,
					&image_area,
					&paint_area)) {
		int interp_type;

		if (viewer->zoom_quality == GTH_ZOOM_QUALITY_LOW)
			interp_type = GDK_INTERP_NEAREST;
		else
			interp_type = GDK_INTERP_BILINEAR;
/*			interp_type = GDK_INTERP_TILES;*/
/*			interp_type = GDK_INTERP_HYPER;*/

		if (FLOAT_EQUAL (viewer->zoom_level, 1.0))
			interp_type = GDK_INTERP_NEAREST;

		src_x += paint_area.x - image_area.x;
		src_y += paint_area.y - image_area.y;

		paint (viewer,
		       src_x,
		       src_y,
		       paint_area.x,
		       paint_area.y,
		       paint_area.width,
		       paint_area.height,
		       interp_type);
	}
	viewer->rendering = FALSE;

	/* Draw the focus. */

#if 0
	if (GTK_WIDGET_HAS_FOCUS (widget)) {
		GdkRectangle r;

		r.x = 0;
		r.y = 0;
		r.width = gdk_width + 2;
		r.height = gdk_height + 2;

		gtk_paint_focus (widget->style,
				 widget->window,
				 widget->state,
				 &r,
				 widget, NULL,
				 0, 0, gdk_width + 2, gdk_height + 2);
	}
#endif

	add_change_frame_timeout (viewer);

	return FALSE;
}


static void
expose_area (ImageViewer *viewer,
	     int          x,
	     int          y,
	     int          width,
	     int          height)
{
	GdkEventExpose event;

	if (width == 0 || height == 0)
		return;

	event.area.x = x;
	event.area.y = y;
	event.area.width = width;
	event.area.height = height;

	image_viewer_expose (GTK_WIDGET (viewer), &event);
}


void
image_viewer_scroll_to (ImageViewer *viewer,
			int          x_offset,
			int          y_offset)
{
	g_return_if_fail (viewer != NULL);

	if (image_viewer_get_current_pixbuf (viewer) == NULL)
		return;

	if (viewer->rendering)
		return;

	scroll_to (viewer, &x_offset, &y_offset);

	g_signal_handlers_block_by_data (G_OBJECT (viewer->hadj), viewer);
	g_signal_handlers_block_by_data (G_OBJECT (viewer->vadj), viewer);
	gtk_adjustment_set_value (viewer->hadj, viewer->x_offset);
	gtk_adjustment_set_value (viewer->vadj, viewer->y_offset);
	g_signal_handlers_unblock_by_data (G_OBJECT (viewer->hadj), viewer);
	g_signal_handlers_unblock_by_data (G_OBJECT (viewer->vadj), viewer);
}


static void
scroll_to (ImageViewer *viewer,
	   int         *x_offset,
	   int         *y_offset)
{
	GdkDrawable *drawable;
	int          width, height;
	int          delta_x, delta_y;
	GdkEvent    *event;
	gboolean     replay_animation;
	int          gdk_width, gdk_height;

	g_return_if_fail (viewer != NULL);

	if (image_viewer_get_current_pixbuf (viewer) == NULL)
		return;

	if (viewer->rendering)
		return;

	get_zoomed_size (viewer, &width, &height, viewer->zoom_level);

	drawable = GTK_WIDGET (viewer)->window;
	gdk_width = GTK_WIDGET (viewer)->allocation.width - viewer->frame_border2;
	gdk_height = GTK_WIDGET (viewer)->allocation.height - viewer->frame_border2;

	if (width > gdk_width)
		*x_offset = CLAMP (*x_offset, 0, width - gdk_width);
	else
		*x_offset = viewer->x_offset;

	if (height > gdk_height)
		*y_offset = CLAMP (*y_offset, 0, height - gdk_height);
	else
		*y_offset = viewer->y_offset;

	if ((*x_offset == viewer->x_offset) && (*y_offset == viewer->y_offset))
		return;

	delta_x = *x_offset - viewer->x_offset;
	delta_y = *y_offset - viewer->y_offset;

	if (viewer->next_scroll_repaint) {
		viewer->next_scroll_repaint = FALSE;

		viewer->x_offset = *x_offset;
		viewer->y_offset = *y_offset;

		g_signal_emit (G_OBJECT (viewer),
			       image_viewer_signals[REPAINTED],
			       0);

		expose_area (viewer, 0, 0,
			     GTK_WIDGET (viewer)->allocation.width,
			     GTK_WIDGET (viewer)->allocation.height);

		return;
	}

	if ((delta_x != 0) || (delta_y != 0)) {
		GdkGC *gc = GTK_WIDGET (viewer)->style->black_gc;
		int    src_x, dest_x;
		int    src_y, dest_y;

		if (delta_x < 0) {
			src_x = 0;
			dest_x = -delta_x;
		} else {
			src_x = delta_x;
			dest_x = 0;
		}

		if (delta_y < 0) {
			src_y = 0;
			dest_y = -delta_y;
		} else {
			src_y = delta_y;
			dest_y = 0;
		}

		gc = gdk_gc_new (drawable);
		gdk_gc_set_exposures (gc, TRUE);

		dest_x += viewer->frame_border;
		dest_y += viewer->frame_border;
		src_x += viewer->frame_border;
		src_y += viewer->frame_border;

		gdk_draw_drawable (drawable,
				   gc,
				   drawable,
				   src_x, src_y,
				   dest_x, dest_y,
				   gdk_width - abs (delta_x),
				   gdk_height - abs (delta_y));

		g_object_unref (gc);
	}

	viewer->x_offset = *x_offset;
	viewer->y_offset = *y_offset;

	expose_area (viewer,
		     viewer->frame_border,
		     (delta_y < 0) ? viewer->frame_border : viewer->frame_border + gdk_height - abs (delta_y),
		     gdk_width,
		     abs (delta_y));

	expose_area (viewer,
		     (delta_x < 0) ? viewer->frame_border : viewer->frame_border + gdk_width - abs (delta_x),
		     viewer->frame_border,
		     abs (delta_x),
		     gdk_height);

	/* Process graphics exposures */

	replay_animation = viewer->play_animation;
	viewer->play_animation = FALSE;
        while ((event = gdk_event_get_graphics_expose (drawable)) != NULL) {
		GdkEventExpose *expose = (GdkEventExpose*) event;

		expose_area (viewer,
			     expose->area.x,
			     expose->area.y,
			     expose->area.width,
			     expose->area.height);

                if (expose->count == 0) {
                        gdk_event_free (event);
                        break;
                }
                gdk_event_free (event);
        }
	viewer->play_animation = replay_animation;
}


static gint
image_viewer_button_press (GtkWidget      *widget,
			   GdkEventButton *event)
{
	ImageViewer *viewer = IMAGE_VIEWER (widget);

	if (! GTK_WIDGET_HAS_FOCUS (widget)) {
		gtk_widget_grab_focus (widget);
		viewer->just_focused = TRUE;
	}

	if (viewer->dragging)
		return FALSE;

	if ((event->type == GDK_2BUTTON_PRESS) ||
	    (event->type == GDK_3BUTTON_PRESS)) {
		viewer->double_click = TRUE;
		return FALSE;
	} else
		viewer->double_click = FALSE;

	if (event->button == 1) {
		GdkCursor *cursor;
		int        retval;

		cursor = cursor_get (widget->window, CURSOR_HAND_CLOSED);
		retval = gdk_pointer_grab (widget->window,
					   FALSE,
					   (GDK_POINTER_MOTION_MASK
					    | GDK_POINTER_MOTION_HINT_MASK
					    | GDK_BUTTON_RELEASE_MASK),
					   NULL,
					   cursor,
					   event->time);
		gdk_cursor_unref (cursor);

		if (retval != 0)
			return FALSE;

		viewer->drag_realx = viewer->drag_x = event->x;
		viewer->drag_realy = viewer->drag_y = event->y;
		viewer->pressed = TRUE;

		return TRUE;
	}

	return FALSE;
}


static gint
image_viewer_button_release  (GtkWidget *widget,
			      GdkEventButton *event)
{
	ImageViewer *viewer = IMAGE_VIEWER (widget);

	if (event->button != 1) {
		viewer->just_focused = FALSE;
		return FALSE;
	}

	gdk_pointer_ungrab (event->time);

	if (! viewer->dragging
	    && ! viewer->double_click
	    && ! viewer->just_focused) {
		g_signal_emit (G_OBJECT (viewer),
			       image_viewer_signals[CLICKED],
			       0);

	}

	viewer->just_focused = FALSE;
	viewer->pressed = FALSE;
	viewer->dragging = FALSE;

	return FALSE;
}


static gint
image_viewer_motion_notify (GtkWidget *widget,
			    GdkEventMotion *event)
{
	ImageViewer     *viewer = IMAGE_VIEWER (widget);
	GdkModifierType  mods;
	gint             x, y;

	if (! viewer->pressed)
		return FALSE;

	if (viewer->rendering)
		return FALSE;

	viewer->dragging = TRUE;

	if (event->is_hint)
                gdk_window_get_pointer (widget->window, &x, &y, &mods);
        else
                return FALSE;

	viewer->drag_realx = x;
	viewer->drag_realy = y;

	if ((abs (viewer->drag_realx - viewer->drag_x) < DRAG_THRESHOLD)
	    && (abs (viewer->drag_realy - viewer->drag_y) < DRAG_THRESHOLD))
		return FALSE;

	x = viewer->x_offset + (viewer->drag_x - viewer->drag_realx);
	y = viewer->y_offset + (viewer->drag_y - viewer->drag_realy);

	scroll_to (viewer, &x, &y);

	g_signal_handlers_block_by_data (G_OBJECT (viewer->hadj), viewer);
	g_signal_handlers_block_by_data (G_OBJECT (viewer->vadj), viewer);
	gtk_adjustment_set_value (viewer->hadj, x);
	gtk_adjustment_set_value (viewer->vadj, y);
	g_signal_handlers_unblock_by_data (G_OBJECT(viewer->hadj), viewer);
	g_signal_handlers_unblock_by_data (G_OBJECT(viewer->vadj), viewer);

	viewer->drag_x = viewer->drag_realx;
	viewer->drag_y = viewer->drag_realy;

	return FALSE;
}


static void
create_pixbuf_from_iter (ImageViewer *viewer)
{
	GdkPixbufAnimationIter *iter = viewer->iter;
	viewer->frame_pixbuf = gdk_pixbuf_animation_iter_get_pixbuf (iter);
	viewer->frame_delay = gdk_pixbuf_animation_iter_get_delay_time (iter);
}


static void
create_first_pixbuf (ImageViewer *viewer)
{
	g_return_if_fail (viewer != NULL);

	viewer->frame_pixbuf = NULL;
	viewer->frame_delay = 0;

	if (viewer->iter != NULL)
		g_object_unref (viewer->iter);

	g_get_current_time (&viewer->time);

	viewer->iter = gdk_pixbuf_animation_get_iter (viewer->anim,
						      &viewer->time);
	create_pixbuf_from_iter (viewer);
}


static gboolean
change_frame_cb (gpointer data)
{
	ImageViewer *viewer = data;

	if (viewer->anim_id != 0) {
		g_source_remove (viewer->anim_id);
		viewer->anim_id = 0;
	}

	g_time_val_add (&viewer->time, (glong) viewer->frame_delay * 1000);
	gdk_pixbuf_animation_iter_advance (viewer->iter, &viewer->time);

	create_pixbuf_from_iter (viewer);

	viewer->skip_zoom_change = TRUE;
	viewer->skip_size_change = TRUE;

	image_viewer_update_view (viewer);

	return FALSE;
}


static void
image_viewer_finalize (GObject *object)
{
        ImageViewer* viewer;

        g_return_if_fail (object != NULL);
        g_return_if_fail (IS_IMAGE_VIEWER (object));

        viewer = IMAGE_VIEWER (object);

	if (viewer->anim_id != 0) {
		g_source_remove (viewer->anim_id);
		viewer->anim_id = 0;
	}

	if (viewer->loader != NULL) {
		g_object_unref (viewer->loader);
		viewer->loader = NULL;
	}

	if (viewer->anim != NULL) {
		g_object_unref (viewer->anim);
		viewer->anim = NULL;
	}

	if (viewer->iter != NULL) {
		g_object_unref (viewer->iter);
		viewer->iter = NULL;
	}

	if (viewer->cursor != NULL) {
		gdk_cursor_unref (viewer->cursor);
		viewer->cursor = NULL;
	}

	if (viewer->cursor_void != NULL) {
		gdk_cursor_unref (viewer->cursor_void);
		viewer->cursor_void = NULL;
	}

	if (viewer->hadj != NULL) {
		g_signal_handlers_disconnect_by_data (G_OBJECT (viewer->hadj),
						      viewer);
		g_object_unref (viewer->hadj);
		viewer->hadj = NULL;
	}
	if (viewer->vadj != NULL) {
		g_signal_handlers_disconnect_by_data (G_OBJECT (viewer->vadj),
						      viewer);
		g_object_unref (viewer->vadj);
		viewer->vadj = NULL;
	}

	if (viewer->area_pixbuf != NULL) {
		g_object_unref (viewer->area_pixbuf);
		viewer->area_pixbuf = NULL;
	}

        /* Chain up */
	G_OBJECT_CLASS (parent_class)->finalize (object);
}


static int
to_255 (int v)
{
	return v * 255 / 65535;
}


static void
image_viewer_realize (GtkWidget *widget)
{
	ImageViewer   *viewer;
	GdkWindowAttr  attributes;
	int            attributes_mask;

	g_return_if_fail (IS_IMAGE_VIEWER (widget));

	viewer = IMAGE_VIEWER (widget);
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
	gdk_window_set_user_data (widget->window, viewer);

	widget->style = gtk_style_attach (widget->style, widget->window);
	gtk_style_set_background (widget->style, widget->window, GTK_STATE_NORMAL);

	viewer->cursor = cursor_get (widget->window, CURSOR_HAND_OPEN);
	viewer->cursor_void = cursor_get (widget->window, CURSOR_VOID);
	gdk_window_set_cursor (widget->window, viewer->cursor);

	if (viewer->transp_type == GTH_TRANSP_TYPE_NONE) {
		GdkColor color;
		guint    base_color;

		color = GTK_WIDGET (viewer)->style->bg[GTK_STATE_NORMAL];
		base_color = (0xFF000000
			      | (to_255 (color.red) << 16)
			      | (to_255 (color.green) << 8)
			      | (to_255 (color.blue) << 0));

		viewer->check_color1 = base_color;
		viewer->check_color2 = base_color;
	}
}


static void
image_viewer_unrealize (GtkWidget *widget)
{
	ImageViewer *viewer;

	g_return_if_fail (IS_IMAGE_VIEWER (widget));

	viewer = IMAGE_VIEWER (widget);

	if (viewer->cursor) {
		gdk_cursor_unref (viewer->cursor);
		viewer->cursor = NULL;
	}
	if (viewer->cursor_void) {
		gdk_cursor_unref (viewer->cursor_void);
		viewer->cursor_void = NULL;
	}

	GTK_WIDGET_CLASS (parent_class)->unrealize (widget);
}


GdkPixbuf*
image_viewer_get_current_pixbuf (ImageViewer *viewer)
{
	g_return_val_if_fail (viewer != NULL, NULL);

	if (viewer->is_void)
		return NULL;

	if (! viewer->is_animation)
		return image_loader_get_pixbuf (viewer->loader);

	return viewer->frame_pixbuf;
}


static void
zoom_to_fit (ImageViewer *viewer)
{
	GdkPixbuf *buf;
	int        gdk_width, gdk_height;
	double     x_level, y_level;
	double     new_zoom_level;

	buf = image_viewer_get_current_pixbuf (viewer);

	gdk_width = GTK_WIDGET (viewer)->allocation.width - viewer->frame_border2;
	gdk_height = GTK_WIDGET (viewer)->allocation.height - viewer->frame_border2;
	x_level = (double) gdk_width / gdk_pixbuf_get_width (buf);
	y_level = (double) gdk_height / gdk_pixbuf_get_height (buf);

	new_zoom_level = (x_level < y_level) ? x_level : y_level;
	if (new_zoom_level > 0.0) {
		viewer->doing_zoom_fit = TRUE;
		image_viewer_set_zoom (viewer, new_zoom_level);
		viewer->doing_zoom_fit = FALSE;
	}
}


static void
zoom_to_fit_width (ImageViewer *viewer)
{
        GdkPixbuf *buf;
        double     new_zoom_level;
        int        gdk_width;

        buf = image_viewer_get_current_pixbuf (viewer);

        gdk_width = GTK_WIDGET (viewer)->allocation.width - viewer->frame_border2;
        new_zoom_level = (double) gdk_width / gdk_pixbuf_get_width (buf);

        if (new_zoom_level > 0.0) {
        	viewer->doing_zoom_fit = TRUE;
                image_viewer_set_zoom (viewer, new_zoom_level);
                viewer->doing_zoom_fit = FALSE;
        }
}


static void
image_viewer_size_allocate (GtkWidget       *widget,
			    GtkAllocation   *allocation)
{
	ImageViewer *viewer;
	int          gdk_width, gdk_height;
	GdkPixbuf   *current_pixbuf;

	viewer = IMAGE_VIEWER (widget);

	widget->allocation = *allocation;
	gdk_width = allocation->width - viewer->frame_border2;
	gdk_height = allocation->height - viewer->frame_border2;

	current_pixbuf = image_viewer_get_current_pixbuf (viewer);

	/* If a fit type is active update the zoom level. */

	if (! viewer->is_void && (current_pixbuf != NULL)) {
		switch (viewer->fit) {
		case GTH_FIT_SIZE:
			zoom_to_fit (viewer);
			break;
		case GTH_FIT_SIZE_IF_LARGER:
			if ((gdk_width < gdk_pixbuf_get_width (current_pixbuf))
		    	    || (gdk_height < gdk_pixbuf_get_height (current_pixbuf)))
				zoom_to_fit (viewer);
			else {
				viewer->doing_zoom_fit = TRUE;
				image_viewer_set_zoom (viewer, 1.0);
				viewer->doing_zoom_fit = FALSE;
			}
			break;
		case GTH_FIT_WIDTH:
			zoom_to_fit_width (viewer);
			break;
		case GTH_FIT_WIDTH_IF_LARGER:
			if (gdk_width < gdk_pixbuf_get_width (current_pixbuf))
				zoom_to_fit_width (viewer);
			else {
				viewer->doing_zoom_fit = TRUE;
				image_viewer_set_zoom (viewer, 1.0);
				viewer->doing_zoom_fit = FALSE;
			}
			break;
		default:
			break;
		}
	}

	/* Check whether the offset is still valid. */

	if (current_pixbuf != NULL) {
		int width, height;

		get_zoomed_size (viewer, &width, &height, viewer->zoom_level);

		if (width > gdk_width)
			viewer->x_offset = CLAMP (viewer->x_offset,
						  0,
						  width - gdk_width);
		else
			viewer->x_offset = 0;

		if (height > gdk_height)
			viewer->y_offset = CLAMP (viewer->y_offset,
						  0,
						  height - gdk_height);
		else
			viewer->y_offset = 0;

		if ((width != viewer->hadj->upper) || (height != viewer->vadj->upper))
			gth_iviewer_size_changed (GTH_IVIEWER (viewer));

		/* Change adjustment values. */

		viewer->hadj->lower          = 0.0;
		viewer->hadj->upper          = width;
		viewer->hadj->value          = viewer->x_offset;
		viewer->hadj->step_increment = STEP_INCREMENT;
		viewer->hadj->page_increment = gdk_width / 2;
		viewer->hadj->page_size      = gdk_width;

		viewer->vadj->lower          = 0.0;
		viewer->vadj->upper          = height;
		viewer->vadj->value          = viewer->y_offset;
		viewer->vadj->step_increment = STEP_INCREMENT;
		viewer->vadj->page_increment = gdk_height / 2;
		viewer->vadj->page_size      = gdk_height;
	} else {
		viewer->hadj->lower     = 0.0;
		viewer->hadj->upper     = 1.0;
		viewer->hadj->value     = 0.0;
		viewer->hadj->page_size = 1.0;

		viewer->vadj->lower     = 0.0;
		viewer->vadj->upper     = 1.0;
		viewer->vadj->value     = 0.0;
		viewer->vadj->page_size = 1.0;
	}

	g_signal_handlers_block_by_data (G_OBJECT (viewer->hadj), viewer);
	g_signal_handlers_block_by_data (G_OBJECT (viewer->vadj), viewer);
	gtk_adjustment_changed (viewer->hadj);
	gtk_adjustment_changed (viewer->vadj);
	g_signal_handlers_unblock_by_data (G_OBJECT (viewer->hadj), viewer);
	g_signal_handlers_unblock_by_data (G_OBJECT (viewer->vadj), viewer);

	/**/

	if (GTK_WIDGET_REALIZED (widget))
		gdk_window_move_resize (widget->window,
					allocation->x, allocation->y,
					allocation->width, allocation->height);

	if (! viewer->skip_size_change)
		gth_iviewer_size_changed (GTH_IVIEWER (viewer));
	else
		viewer->skip_size_change = FALSE;
}


static gint
image_viewer_focus_in (GtkWidget     *widget,
		       GdkEventFocus *event)
{
	GTK_WIDGET_SET_FLAGS (widget, GTK_HAS_FOCUS);
	gtk_widget_queue_draw (widget);
	return TRUE;
}


static gint
image_viewer_focus_out (GtkWidget     *widget,
			GdkEventFocus *event)
{
	GTK_WIDGET_UNSET_FLAGS (widget, GTK_HAS_FOCUS);
	gtk_widget_queue_draw (widget);
	return TRUE;
}


static gdouble zooms[] = {                  0.05, 0.07, 0.10,
			  0.15, 0.20, 0.30, 0.50, 0.75, 1.0,
			  1.5 , 2.0 , 3.0 , 5.0 , 7.5,  10.0,
			  15.0, 20.0, 30.0, 50.0, 75.0, 100.0};

static const gint nzooms = sizeof (zooms) / sizeof (gdouble);


static gdouble
get_next_zoom (gdouble zoom)
{
	gint i;

	i = 0;
	while ((i < nzooms) && (zooms[i] <= zoom))
		i++;
	i = CLAMP (i, 0, nzooms - 1);

	return zooms[i];
}


static gdouble
get_prev_zoom (gdouble zoom)
{
	gint i;

	i = nzooms - 1;
	while ((i >= 0) && (zooms[i] >= zoom))
		i--;
	i = CLAMP (i, 0, nzooms - 1);

	return zooms[i];
}


static void
set_zoom (ImageViewer *viewer,
	  gdouble      zoom_level,
	  int          center_x,
	  int          center_y)
{
	GtkWidget *widget = (GtkWidget*) viewer;
	gdouble    zoom_ratio;
	int        gdk_width, gdk_height;

	g_return_if_fail (viewer != NULL);
	g_return_if_fail (viewer->loader != NULL);

	gdk_width = widget->allocation.width - viewer->frame_border2;
	gdk_height = widget->allocation.height - viewer->frame_border2;

	/* try to keep the center of the view visible. */

	zoom_ratio = zoom_level / viewer->zoom_level;
	viewer->x_offset = ((viewer->x_offset + center_x) * zoom_ratio - gdk_width / 2);
	viewer->y_offset = ((viewer->y_offset + center_y) * zoom_ratio - gdk_height / 2);

 	/* reset zoom_fit unless we are performing a zoom to fit. */

	if (! viewer->doing_zoom_fit)
		viewer->fit = GTH_FIT_NONE;

	viewer->zoom_level = zoom_level;

	if (! viewer->doing_zoom_fit) {
		gtk_widget_queue_resize (GTK_WIDGET (viewer));
		gtk_widget_queue_draw (GTK_WIDGET (viewer));
	}

	if (! viewer->skip_zoom_change)
		g_signal_emit (G_OBJECT (viewer),
			       image_viewer_signals[ZOOM_CHANGED],
			       0);
	else
		viewer->skip_zoom_change = FALSE;
}


void
image_viewer_set_zoom (ImageViewer *viewer,
		       gdouble      zoom_level)
{
	GtkWidget *widget = (GtkWidget*) viewer;

	set_zoom (viewer,
		  zoom_level,
		  (widget->allocation.width - viewer->frame_border2) / 2,
		  (widget->allocation.height - viewer->frame_border2) / 2);
}


gdouble
image_viewer_get_zoom (ImageViewer *viewer)
{
	g_return_val_if_fail (viewer != NULL, FALSE);
	return viewer->zoom_level;
}


static gint
image_viewer_scroll_event (GtkWidget        *widget,
			   GdkEventScroll   *event)
{
	ImageViewer   *viewer = IMAGE_VIEWER (widget);
	GtkAdjustment *adj;
	gdouble        new_value;

	g_return_val_if_fail (IS_IMAGE_VIEWER (widget), FALSE);
	g_return_val_if_fail (event != NULL, FALSE);

	if (event->state & GDK_CONTROL_MASK) {
		if (event->direction == GDK_SCROLL_UP) {
			set_zoom (viewer,
				  get_next_zoom (viewer->zoom_level),
				  (int) event->x,
				  (int) event->y);
			return TRUE;
		}
		if (event->direction == GDK_SCROLL_DOWN) {
			set_zoom (viewer,
				  get_prev_zoom (viewer->zoom_level),
				  (int) event->x,
				  (int) event->y);
			return TRUE;
		}
	}

	if (event->direction == GDK_SCROLL_UP || event->direction == GDK_SCROLL_DOWN) {
		g_signal_emit (G_OBJECT (viewer),
			            image_viewer_signals[MOUSE_WHEEL_SCROLL],
		 	            0, event->direction);
		return TRUE;
	}

	adj = viewer->hadj;

	if (event->direction == GDK_SCROLL_LEFT)
		new_value = adj->value - adj->page_increment / 2;
	else if (event->direction == GDK_SCROLL_RIGHT)
		new_value = adj->value + adj->page_increment / 2;

	new_value = CLAMP (new_value, adj->lower, adj->upper - adj->page_size);
	gtk_adjustment_set_value (adj, new_value);

	return TRUE;
}


void
image_viewer_get_adjustments (GthIViewer     *self,
			      GtkAdjustment **hadj,
			      GtkAdjustment **vadj)
{
	ImageViewer *viewer = IMAGE_VIEWER (self);
	if (hadj != NULL)
		*hadj = viewer->hadj;
	if (vadj != NULL)
		*vadj = viewer->vadj;
}


static void
image_viewer_zoom_to_fit (ImageViewer *viewer)
{
	image_viewer_set_fit_mode (viewer, GTH_FIT_SIZE_IF_LARGER);
}


static void
gth_iviewer_interface_init (gpointer   g_iface,
			    gpointer   iface_data)
{
	GthIViewerInterface *iface = (GthIViewerInterface *)g_iface;

	iface->get_zoom = (double (*) (GthIViewer *)) image_viewer_get_zoom;
	iface->set_zoom = (void (*) (GthIViewer *, double)) image_viewer_set_zoom;
	iface->zoom_in = (void (*) (GthIViewer *)) image_viewer_zoom_in;
	iface->zoom_out = (void (*) (GthIViewer *)) image_viewer_zoom_out;
	iface->zoom_to_fit = (void (*) (GthIViewer *)) image_viewer_zoom_to_fit;
	iface->get_image = (GdkPixbuf * (*) (GthIViewer *)) image_viewer_get_current_pixbuf;
	iface->get_adjustments = (void (*) (GthIViewer *, GtkAdjustment **, GtkAdjustment **)) image_viewer_get_adjustments;
}





GType
image_viewer_get_type ()
{
        static GType type = 0;

        if (! type) {
                GTypeInfo type_info = {
			sizeof (ImageViewerClass),
			NULL,
			NULL,
			(GClassInitFunc) image_viewer_class_init,
			NULL,
			NULL,
			sizeof (ImageViewer),
			0,
			(GInstanceInitFunc) image_viewer_init
		};
		static const GInterfaceInfo iviewer_info = {
			(GInterfaceInitFunc) gth_iviewer_interface_init,
			NULL,
			NULL
		};

		type = g_type_register_static (GTK_TYPE_WIDGET,
					       "ImageViewer",
					       &type_info,
					       0);
		g_type_add_interface_static (type,
					     GTH_TYPE_IVIEWER,
					     &iviewer_info);
	}

        return type;
}


GtkWidget*
image_viewer_new (void)
{
	return GTK_WIDGET (g_object_new (IMAGE_VIEWER_TYPE, NULL));
}


/* -- image_viewer_load_image -- */


typedef struct {
	ImageViewer *viewer;
	char        *path;
} LoadImageData;


void
load_image__step2 (LoadImageData *lidata)
{
	image_loader_set_path (lidata->viewer->loader, lidata->path);
	image_loader_start (lidata->viewer->loader);

	g_free (lidata->path);
	g_free (lidata);
}


void
image_viewer_load_image (ImageViewer *viewer,
			 const gchar *path)
{
	LoadImageData *lidata;

	g_return_if_fail (viewer != NULL);
	g_return_if_fail (path != NULL);

	viewer->is_void = FALSE;

	halt_animation (viewer);

	lidata = g_new (LoadImageData, 1);
	lidata->viewer = viewer;
	lidata->path = g_strdup (path);
	image_loader_stop (viewer->loader, (DoneFunc) load_image__step2, lidata);
}


/* -- image_viewer_load_from_pixbuf_loader -- */


typedef struct {
	ImageViewer *viewer;
	gpointer     data;
} ImageViewerLoadData;


void
load_from_pixbuf_loader__step2 (ImageViewerLoadData *ivl_data)
{
	ImageViewer     *viewer = ivl_data->viewer;
	GdkPixbufLoader *pixbuf_loader = ivl_data->data;

	image_loader_load_from_pixbuf_loader (viewer->loader, pixbuf_loader);
	g_object_unref (pixbuf_loader);
	g_free (ivl_data);
}


void
image_viewer_load_from_pixbuf_loader (ImageViewer *viewer,
				      GdkPixbufLoader *pixbuf_loader)
{
	ImageViewerLoadData *ivl_data;

	g_return_if_fail (viewer != NULL);
	g_return_if_fail (pixbuf_loader != NULL);

	viewer->is_void = FALSE;
	halt_animation (viewer);

	g_object_ref (pixbuf_loader);

	ivl_data = g_new (ImageViewerLoadData, 1);
	ivl_data->viewer = viewer;
	ivl_data->data = pixbuf_loader;
	image_loader_stop (viewer->loader,
			   (DoneFunc) load_from_pixbuf_loader__step2,
			   ivl_data);
}


/* -- image_viewer_load_from_image_loader -- */


void
load_from_image_loader__step2 (ImageViewerLoadData *ivl_data)
{
	ImageViewer *viewer = ivl_data->viewer;
	ImageLoader *image_loader = ivl_data->data;

	image_loader_load_from_image_loader (viewer->loader, image_loader);
	g_object_unref (image_loader);
	g_free (ivl_data);
}


void
image_viewer_load_from_image_loader (ImageViewer *viewer,
				     ImageLoader *image_loader)
{
	ImageViewerLoadData *ivl_data;

	g_return_if_fail (viewer != NULL);
	g_return_if_fail (image_loader != NULL);

	viewer->is_void = FALSE;
	halt_animation (viewer);

	g_object_ref (image_loader);

	ivl_data = g_new (ImageViewerLoadData, 1);
	ivl_data->viewer = viewer;
	ivl_data->data = image_loader;
	image_loader_stop (viewer->loader,
			   (DoneFunc) load_from_image_loader__step2,
			   ivl_data);
}


void
image_viewer_set_pixbuf (ImageViewer *viewer,
			 GdkPixbuf   *pixbuf)
{
	g_return_if_fail (viewer != NULL);

	if (viewer->is_animation)
		return;

        if (viewer->rendering)
		return;

	viewer->is_void = (pixbuf == NULL);

	image_loader_set_pixbuf (viewer->loader, pixbuf);
	image_viewer_update_view (viewer);
}


void
image_viewer_set_void (ImageViewer *viewer)
{
	g_return_if_fail (viewer != NULL);

	viewer->is_void = TRUE;
	viewer->is_animation = FALSE;

	halt_animation (viewer);

	viewer->frame_pixbuf = NULL;

        if (viewer->reset_scrollbars) {
                viewer->x_offset = 0;
                viewer->y_offset = 0;
        }

	gtk_widget_queue_resize (GTK_WIDGET (viewer));
	gtk_widget_queue_draw (GTK_WIDGET (viewer));
}


gboolean
image_viewer_is_void (ImageViewer *viewer)
{
	g_return_val_if_fail (viewer != NULL, TRUE);
	return viewer->is_void;
}


void
image_viewer_update_view (ImageViewer *viewer)
{
	g_return_if_fail (viewer != NULL);

	if (viewer->fit == GTH_FIT_NONE)
		image_viewer_set_zoom (viewer, viewer->zoom_level);
	else
		image_viewer_set_fit_mode (viewer, viewer->fit);
}


char*
image_viewer_get_image_filename (ImageViewer *viewer)
{
	g_return_val_if_fail (viewer != NULL, NULL);
	return image_loader_get_path (viewer->loader);
}


int
image_viewer_get_image_width (ImageViewer *viewer)
{
	GdkPixbuf *pixbuf;

	g_return_val_if_fail (viewer != NULL, 0);

	if (viewer->anim != NULL)
		return gdk_pixbuf_animation_get_width (viewer->anim);

	pixbuf = image_loader_get_pixbuf (viewer->loader);
	if (pixbuf != NULL)
		return gdk_pixbuf_get_width (pixbuf);

	return 0;
}


gint
image_viewer_get_image_height (ImageViewer *viewer)
{
	GdkPixbuf *pixbuf;

	g_return_val_if_fail (viewer != NULL, 0);

	if (viewer->anim != NULL)
		return gdk_pixbuf_animation_get_height (viewer->anim);

	pixbuf = image_loader_get_pixbuf (viewer->loader);
	if (pixbuf != NULL)
		return gdk_pixbuf_get_height (pixbuf);

	return 0;
}


gint
image_viewer_get_image_bps (ImageViewer *viewer)
{
	GdkPixbuf *pixbuf;

	g_return_val_if_fail (viewer != NULL, 0);

	if (viewer->iter != NULL)
		pixbuf = gdk_pixbuf_animation_iter_get_pixbuf (viewer->iter);
	else
		pixbuf = image_loader_get_pixbuf (viewer->loader);

	if (pixbuf != NULL)
		return gdk_pixbuf_get_bits_per_sample (pixbuf);

	return 0;
}


gboolean
image_viewer_get_has_alpha (ImageViewer *viewer)
{
	GdkPixbuf *pixbuf;

	g_return_val_if_fail (viewer != NULL, 0);

	if (viewer->iter != NULL)
		pixbuf = gdk_pixbuf_animation_iter_get_pixbuf (viewer->iter);
	else
		pixbuf = image_loader_get_pixbuf (viewer->loader);

	if (pixbuf != NULL)
		return gdk_pixbuf_get_has_alpha (pixbuf);

	return FALSE;
}


static void
init_animation (ImageViewer *viewer)
{
	g_return_if_fail (viewer != NULL);

	if (! viewer->is_animation)
		return;

	if (viewer->anim != NULL)
		g_object_unref (viewer->anim);

	viewer->anim = image_loader_get_animation (viewer->loader);
	if (viewer->anim == NULL) {
		viewer->is_animation = FALSE;
		return;
	}

	create_first_pixbuf (viewer);
}


void
image_viewer_start_animation (ImageViewer *viewer)
{
	g_return_if_fail (viewer != NULL);
	viewer->play_animation = TRUE;
	image_viewer_update_view (viewer);
}


static void
halt_animation (ImageViewer *viewer)
{
	g_return_if_fail (viewer != NULL);

	if (viewer->anim_id == 0)
		return;

	g_source_remove (viewer->anim_id);
	viewer->anim_id = 0;
}


void
image_viewer_stop_animation (ImageViewer *viewer)
{
	g_return_if_fail (viewer != NULL);
	viewer->play_animation = FALSE;
	halt_animation (viewer);
}


void
image_viewer_step_animation (ImageViewer *viewer)
{
	g_return_if_fail (viewer != NULL);

	if (!viewer->is_animation) return;
	if (viewer->play_animation) return;
	if (viewer->rendering) return;

	change_frame_cb (viewer);
}


gboolean
image_viewer_is_animation (ImageViewer *viewer)
{
	g_return_val_if_fail (viewer != NULL, FALSE);
	return viewer->is_animation;
}


gboolean
image_viewer_is_playing_animation (ImageViewer *viewer)
{
	g_return_val_if_fail (viewer != NULL, FALSE);
	return viewer->is_animation && viewer->play_animation;
}


static void
image_error (ImageLoader *il,
	     gpointer data)
{
	ImageViewer *viewer = data;

	image_viewer_set_void (viewer);

	g_signal_emit (G_OBJECT (viewer),
		       image_viewer_signals[IMAGE_LOADED],
		       0);
}


static void
image_loaded (ImageLoader *il,
	      gpointer data)
{
	ImageViewer *viewer = data;
	GdkPixbufAnimation *anim;

	halt_animation (viewer);

	if (viewer->reset_scrollbars) {
		viewer->x_offset = 0;
		viewer->y_offset = 0;
	}

	if (viewer->anim != NULL) {
		g_object_unref (viewer->anim);
		viewer->anim = NULL;
	}

	anim = image_loader_get_animation (viewer->loader);
	viewer->is_animation = (anim != NULL) && ! gdk_pixbuf_animation_is_static_image (anim);
	g_object_unref (anim);

	if (viewer->is_animation)
		init_animation (viewer);

	switch (viewer->zoom_change) {
	case GTH_ZOOM_CHANGE_ACTUAL_SIZE:
		image_viewer_set_zoom (viewer, 1.0);
		add_change_frame_timeout (viewer);
		break;

	case GTH_ZOOM_CHANGE_KEEP_PREV:
		image_viewer_update_view (viewer);
		break;

	case GTH_ZOOM_CHANGE_FIT_SIZE:
		image_viewer_set_fit_mode (viewer, GTH_FIT_SIZE);
		add_change_frame_timeout (viewer);
		break;

	case GTH_ZOOM_CHANGE_FIT_SIZE_IF_LARGER:
		image_viewer_set_fit_mode (viewer, GTH_FIT_SIZE_IF_LARGER);
		add_change_frame_timeout (viewer);
		break;

	case GTH_ZOOM_CHANGE_FIT_WIDTH_IF_LARGER:
		image_viewer_set_fit_mode (viewer, GTH_FIT_WIDTH_IF_LARGER);
		add_change_frame_timeout (viewer);
		break;
	}

	g_signal_emit (G_OBJECT (viewer),
		       image_viewer_signals[IMAGE_LOADED],
		       0);
}


void
image_viewer_set_zoom_quality (ImageViewer    *viewer,
			       GthZoomQuality  quality)
{
	g_return_if_fail (viewer != NULL);
	viewer->zoom_quality = quality;
}


GthZoomQuality
image_viewer_get_zoom_quality (ImageViewer *viewer)
{
	g_return_val_if_fail (viewer != NULL, -1);
	return viewer->zoom_quality;
}


void
image_viewer_set_zoom_change (ImageViewer   *viewer,
			      GthZoomChange  zoom_change)
{
	g_return_if_fail (viewer != NULL);

	if (zoom_change == viewer->zoom_change)
		return;

	viewer->zoom_change = zoom_change;
}


GthZoomChange
image_viewer_get_zoom_change (ImageViewer *viewer)
{
	g_return_val_if_fail (viewer != NULL, -1);
	return viewer->zoom_change;
}


void
image_viewer_zoom_in (ImageViewer *viewer)
{
	g_return_if_fail (viewer != NULL);
	g_return_if_fail (viewer->loader != NULL);

	if (image_viewer_get_current_pixbuf (viewer) == NULL)
		return;
	image_viewer_set_zoom (viewer, get_next_zoom (viewer->zoom_level));
}


void
image_viewer_zoom_out (ImageViewer *viewer)
{
	g_return_if_fail (viewer != NULL);
	g_return_if_fail (viewer->loader != NULL);

	if (image_viewer_get_current_pixbuf (viewer) == NULL)
		return;
	image_viewer_set_zoom (viewer, get_prev_zoom (viewer->zoom_level));
}


void
image_viewer_set_fit_mode (ImageViewer     *viewer,
			   GthFit           fit_mode)
{
	g_return_if_fail (viewer != NULL);
	g_return_if_fail (viewer->loader != NULL);

	viewer->fit = fit_mode;
	if (viewer->is_void)
		return;
	gtk_widget_queue_resize (GTK_WIDGET (viewer));
}


static void
image_viewer_style_set (GtkWidget *widget,
			GtkStyle  *previous_style)
{
	ImageViewer *viewer = IMAGE_VIEWER (widget);

	GTK_WIDGET_CLASS (parent_class)->style_set (widget, previous_style);

	if (viewer->transp_type == GTH_TRANSP_TYPE_NONE) {
		GdkColor color;
		guint    base_color;

		color = GTK_WIDGET (viewer)->style->bg[GTK_STATE_NORMAL];
		base_color = (0xFF000000
			      | (to_255 (color.red) << 16)
			      | (to_255 (color.green) << 8)
			      | (to_255 (color.blue) << 0));

		viewer->check_color1 = base_color;
		viewer->check_color2 = base_color;
	}
}


void
image_viewer_set_transp_type (ImageViewer   *viewer,
			      GthTranspType  transp_type)
{
	GdkColor color;
	guint    base_color;

	g_return_if_fail (viewer != NULL);

	viewer->transp_type = transp_type;

	color = GTK_WIDGET (viewer)->style->bg[GTK_STATE_NORMAL];
	base_color = (0xFF000000
		      | (to_255 (color.red) << 16)
		      | (to_255 (color.green) << 8)
		      | (to_255 (color.blue) << 0));

	switch (transp_type) {
	case GTH_TRANSP_TYPE_BLACK:
		viewer->check_color1 = COLOR_GRAY_00;
		viewer->check_color2 = COLOR_GRAY_00;
		break;

	case GTH_TRANSP_TYPE_NONE:
		viewer->check_color1 = base_color;
		viewer->check_color2 = base_color;
		break;

	case GTH_TRANSP_TYPE_WHITE:
		viewer->check_color1 = COLOR_GRAY_FF;
		viewer->check_color2 = COLOR_GRAY_FF;
		break;

	case GTH_TRANSP_TYPE_CHECKED:
		switch (viewer->check_type) {
		case GTH_CHECK_TYPE_DARK:
			viewer->check_color1 = COLOR_GRAY_00;
			viewer->check_color2 = COLOR_GRAY_33;
			break;

		case GTH_CHECK_TYPE_MIDTONE:
			viewer->check_color1 = COLOR_GRAY_66;
			viewer->check_color2 = COLOR_GRAY_99;
			break;

		case GTH_CHECK_TYPE_LIGHT:
			viewer->check_color1 = COLOR_GRAY_CC;
			viewer->check_color2 = COLOR_GRAY_FF;
			break;
		}
		break;
	}
}


GthTranspType
image_viewer_get_transp_type (ImageViewer *viewer)
{
	g_return_val_if_fail (viewer != NULL, FALSE);
	return viewer->transp_type;
}


void
image_viewer_set_check_type (ImageViewer  *viewer,
			     GthCheckType  check_type)
{
	g_return_if_fail (viewer != NULL);
	viewer->check_type = check_type;
}


GthCheckType
image_viewer_get_check_type (ImageViewer *viewer)
{
	g_return_val_if_fail (viewer != NULL, FALSE);
	return viewer->check_type;
}


void
image_viewer_set_check_size (ImageViewer  *viewer,
			     GthCheckSize  check_size)
{
	g_return_if_fail (viewer != NULL);
	viewer->check_size = check_size;
}


GthCheckSize
image_viewer_get_check_size (ImageViewer *viewer)
{
	g_return_val_if_fail (viewer != NULL, FALSE);
	return viewer->check_size;
}


void
image_viewer_clicked (ImageViewer *viewer)
{
	g_return_if_fail (viewer != NULL);
	g_return_if_fail (IS_IMAGE_VIEWER (viewer));
	g_signal_emit (G_OBJECT (viewer), image_viewer_signals[CLICKED], 0);
}


static void
get_zoomed_size (ImageViewer *viewer,
		 int         *width,
		 int         *height,
		 double       zoom_level)
{
	if (image_viewer_get_current_pixbuf (viewer) == NULL) {
		*width = 0;
		*height = 0;
	} else {
		int w, h;

		w = image_viewer_get_image_width (viewer);
		h = image_viewer_get_image_height (viewer);

		*width  = (int) floor ((double) w * zoom_level);
		*height = (int) floor ((double) h * zoom_level);
	}
}


/* set_scroll_adjustments handler for the image view */
static void
set_scroll_adjustments (GtkWidget *widget,
			GtkAdjustment *hadj,
			GtkAdjustment *vadj)
{
        ImageViewer *viewer;

        g_return_if_fail (widget != NULL);
        g_return_if_fail (IS_IMAGE_VIEWER (widget));

        viewer = IMAGE_VIEWER (widget);

        if (hadj)
                g_return_if_fail (GTK_IS_ADJUSTMENT (hadj));
        else
                hadj = GTK_ADJUSTMENT (gtk_adjustment_new (0.0, 0.0, 0.0,
							   0.0, 0.0, 0.0));

        if (vadj)
                g_return_if_fail (GTK_IS_ADJUSTMENT (vadj));
        else
                vadj = GTK_ADJUSTMENT (gtk_adjustment_new (0.0, 0.0, 0.0,
							   0.0, 0.0, 0.0));

        if (viewer->hadj && viewer->hadj != hadj) {
		g_signal_handlers_disconnect_by_data (G_OBJECT (viewer->hadj),
						      viewer);
		g_object_unref (viewer->hadj);

		viewer->hadj = NULL;
        }

        if (viewer->vadj && viewer->vadj != vadj) {
		g_signal_handlers_disconnect_by_data (G_OBJECT (viewer->vadj),
						      viewer);
		g_object_unref (viewer->vadj);
		viewer->vadj = NULL;
        }

        if (viewer->hadj != hadj) {
                viewer->hadj = hadj;
                g_object_ref (viewer->hadj);
                gtk_object_sink (GTK_OBJECT (viewer->hadj));

		g_signal_connect (G_OBJECT (viewer->hadj),
				  "value_changed",
				  G_CALLBACK (hadj_value_changed),
				  viewer);
        }

        if (viewer->vadj != vadj) {
		viewer->vadj = vadj;
		g_object_ref (viewer->vadj);
		gtk_object_sink (GTK_OBJECT (viewer->vadj));

		g_signal_connect (G_OBJECT (viewer->vadj),
				  "value_changed",
				  G_CALLBACK (vadj_value_changed),
				  viewer);
        }
}


static void
scroll_relative (ImageViewer *viewer,
		 int          delta_x,
		 int          delta_y)
{
	image_viewer_scroll_to (viewer,
				viewer->x_offset + delta_x,
				viewer->y_offset + delta_y);
}


static void
scroll_signal (GtkWidget     *widget,
	       GtkScrollType  xscroll_type,
	       GtkScrollType  yscroll_type)
{
	ImageViewer *viewer = IMAGE_VIEWER (widget);
	int          xstep, ystep;

	switch (xscroll_type) {
	case GTK_SCROLL_STEP_LEFT:
		xstep = -viewer->hadj->step_increment;
		break;
	case GTK_SCROLL_STEP_RIGHT:
		xstep = viewer->hadj->step_increment;
		break;
	case GTK_SCROLL_PAGE_LEFT:
		xstep = -viewer->hadj->page_increment;
		break;
	case GTK_SCROLL_PAGE_RIGHT:
		xstep = viewer->hadj->page_increment;
		break;
	default:
		xstep = 0;
		break;
	}

	switch (yscroll_type) {
	case GTK_SCROLL_STEP_UP:
		ystep = -viewer->vadj->step_increment;
		break;
	case GTK_SCROLL_STEP_DOWN:
		ystep = viewer->vadj->step_increment;
		break;
	case GTK_SCROLL_PAGE_UP:
		ystep = -viewer->vadj->page_increment;
		break;
	case GTK_SCROLL_PAGE_DOWN:
		ystep = viewer->vadj->page_increment;
		break;
	default:
		ystep = 0;
		break;
	}

	scroll_relative (viewer, xstep, ystep);
}


void
image_viewer_scroll_step_x (ImageViewer *viewer,
			    gboolean     increment)
{
	g_return_if_fail (IS_IMAGE_VIEWER (viewer));
	scroll_relative (viewer,
			 (increment ? 1 : -1) * viewer->hadj->step_increment,
			 0);
}


void
image_viewer_scroll_step_y (ImageViewer *viewer,
			    gboolean     increment)
{
	g_return_if_fail (IS_IMAGE_VIEWER (viewer));
	scroll_relative (viewer,
			 0,
			 (increment ? 1 : -1) * viewer->vadj->step_increment);
}


void
image_viewer_scroll_page_x (ImageViewer *viewer,
			    gboolean     increment)
{
	g_return_if_fail (IS_IMAGE_VIEWER (viewer));
	scroll_relative (viewer,
			 (increment ? 1 : -1) * viewer->hadj->page_increment,
			 0);
}


void
image_viewer_scroll_page_y (ImageViewer *viewer,
			    gboolean     increment)
{
	g_return_if_fail (IS_IMAGE_VIEWER (viewer));
	scroll_relative (viewer,
			 0,
			 (increment ? 1 : -1) * viewer->vadj->page_increment);
}


void
image_viewer_get_scroll_offset  (ImageViewer *viewer,
				 int         *x,
				 int         *y)
{
	g_return_if_fail (IS_IMAGE_VIEWER (viewer));
	*x = viewer->x_offset;
	*y = viewer->y_offset;
}


void
image_viewer_set_reset_scrollbars (ImageViewer *viewer,
  				   gboolean     reset)
{
	viewer->reset_scrollbars = reset;
}


gboolean
image_viewer_get_reset_scrollbars (ImageViewer *viewer)
{
	return viewer->reset_scrollbars;
}


void
image_viewer_size (ImageViewer *viewer,
		   int          width,
		   int          height)
{
	g_return_if_fail (IS_IMAGE_VIEWER (viewer));

	GTK_WIDGET (viewer)->requisition.width = width;
	GTK_WIDGET (viewer)->requisition.height = height;

	gtk_widget_queue_resize (GTK_WIDGET (viewer));
}


void
image_viewer_show_cursor (ImageViewer *viewer)
{
	g_return_if_fail (IS_IMAGE_VIEWER (viewer));

	viewer->cursor_visible = TRUE;
	gdk_window_set_cursor (GTK_WIDGET (viewer)->window, viewer->cursor);
}


void
image_viewer_hide_cursor (ImageViewer *viewer)
{
	g_return_if_fail (IS_IMAGE_VIEWER (viewer));

	viewer->cursor_visible = FALSE;
	gdk_window_set_cursor (GTK_WIDGET (viewer)->window, viewer->cursor_void);
}


gboolean
image_viewer_is_cursor_visible (ImageViewer *viewer)
{
	g_return_val_if_fail (viewer != NULL, FALSE);
	return viewer->cursor_visible;
}


void
image_viewer_set_black_background (ImageViewer *viewer,
				   gboolean     set_black)
{
	g_return_if_fail (IS_IMAGE_VIEWER (viewer));

	viewer->black_bg = set_black;
	gtk_widget_queue_draw (GTK_WIDGET (viewer));
}


gboolean
image_viewer_is_black_background (ImageViewer *viewer)
{
	g_return_val_if_fail (viewer != NULL, FALSE);
	return viewer->black_bg;
}


void
image_viewer_show_frame (ImageViewer *viewer)
{
	g_return_if_fail (viewer != NULL);
	viewer->frame_visible = TRUE;
	viewer->frame_border = FRAME_BORDER;
	viewer->frame_border2 = FRAME_BORDER2;
	gtk_widget_queue_resize (GTK_WIDGET (viewer));
}


void
image_viewer_hide_frame (ImageViewer *viewer)
{
	g_return_if_fail (viewer != NULL);
	viewer->frame_visible = FALSE;
	viewer->frame_border = 0;
	viewer->frame_border2 = 0;
	gtk_widget_queue_resize (GTK_WIDGET (viewer));
}


gboolean
image_viewer_is_frame_visible (ImageViewer *viewer)
{
	g_return_val_if_fail (viewer != NULL, FALSE);
	return viewer->frame_visible;
}
