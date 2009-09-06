/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2001-2009 Free Software Foundation, Inc.
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
#include <stdlib.h>
#include <math.h>
#include <glib/gi18n.h>
#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>
#include "gth-cursors.h"
#include "gth-enum-types.h"
#include "gth-image-dragger.h"
#include "gth-image-viewer.h"
#include "gth-marshal.h"
#include "glib-utils.h"

#define COLOR_GRAY_00   0x00000000
#define COLOR_GRAY_33   0x00333333
#define COLOR_GRAY_66   0x00666666
#define COLOR_GRAY_99   0x00999999
#define COLOR_GRAY_CC   0x00cccccc
#define COLOR_GRAY_FF   0x00ffffff

#define GTH_IMAGE_VIEWER_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), GTH_TYPE_IMAGE_VIEWER, GthImageViewerPrivate))

#define DRAG_THRESHOLD  1     /* When dragging the image ignores movements
			       * smaller than this value. */
#define MINIMUM_DELAY   10    /* When an animation frame has a 0 milli seconds
			       * delay use this delay instead. */
#define STEP_INCREMENT  20.0  /* Scroll increment. */

enum {
	CLICKED,
	IMAGE_READY,
	ZOOM_IN,
	ZOOM_OUT,
	SET_ZOOM,
	SET_FIT_MODE,
	ZOOM_CHANGED,
	SIZE_CHANGED,
	REPAINTED,
	MOUSE_WHEEL_SCROLL,
	SCROLL,
	LAST_SIGNAL
};


struct _GthImageViewerPrivate {
	gboolean                is_animation;
	gboolean                play_animation;
	gboolean                rendering;
	gboolean                cursor_visible;

	gboolean                frame_visible;
	int                     frame_border;
	int                     frame_border2;

	GthTranspType           transp_type;
	GthCheckType            check_type;
	int                     check_size;
	guint32                 check_color1;
	guint32                 check_color2;

	guint                   anim_id;
	GdkPixbuf              *frame_pixbuf;
	int                     frame_delay;

	GthImageLoader         *loader;
	GdkPixbufAnimation     *anim;
	GdkPixbufAnimationIter *iter;
	GTimeVal                time;               /* Timer used to get the right
					             * frame. */

	GthImageViewerTool     *tool;

	GdkCursor              *cursor;
	GdkCursor              *cursor_void;

	double                  zoom_level;
	guint                   zoom_quality : 1;   /* A ZoomQualityType value. */
	guint                   zoom_change : 3;    /* A ZoomChangeType value. */

	GthFit                  fit;
	gboolean                doing_zoom_fit;     /* Whether we are performing
					             * a zoom to fit the window. */
	gboolean                is_void;            /* If TRUE do not show anything.
					             * It is reset to FALSE we an
					             * image is loaded. */

	gboolean                double_click;
	gboolean                just_focused;

	GdkPixbuf              *paint_pixbuf;
	int                     paint_max_width;
	int                     paint_max_height;
	int                     paint_bps;
	GdkColorspace           paint_color_space;

	gboolean                black_bg;

	gboolean                skip_zoom_change;
	gboolean                skip_size_change;

	gboolean                next_scroll_repaint; /* used in fullscreen mode to
					              * delete the comment before
					              * scrolling. */
	gboolean                reset_scrollbars;
};


static gpointer parent_class = NULL;
static guint gth_image_viewer_signals[LAST_SIGNAL] = { 0 };


static void
gth_image_viewer_finalize (GObject *object)
{
	GthImageViewer* viewer;

	g_return_if_fail (object != NULL);
	g_return_if_fail (GTH_IS_IMAGE_VIEWER (object));

	viewer = GTH_IMAGE_VIEWER (object);

	if (viewer->priv->anim_id != 0) {
		g_source_remove (viewer->priv->anim_id);
		viewer->priv->anim_id = 0;
	}

	if (viewer->priv->loader != NULL) {
		g_object_unref (viewer->priv->loader);
		viewer->priv->loader = NULL;
	}

	if (viewer->priv->anim != NULL) {
		g_object_unref (viewer->priv->anim);
		viewer->priv->anim = NULL;
	}

	if (viewer->priv->iter != NULL) {
		g_object_unref (viewer->priv->iter);
		viewer->priv->iter = NULL;
	}

	if (viewer->priv->cursor != NULL) {
		gdk_cursor_unref (viewer->priv->cursor);
		viewer->priv->cursor = NULL;
	}

	if (viewer->priv->cursor_void != NULL) {
		gdk_cursor_unref (viewer->priv->cursor_void);
		viewer->priv->cursor_void = NULL;
	}

	if (viewer->hadj != NULL) {
		g_signal_handlers_disconnect_by_data (G_OBJECT (viewer->hadj), viewer);
		g_object_unref (viewer->hadj);
		viewer->hadj = NULL;
	}
	if (viewer->vadj != NULL) {
		g_signal_handlers_disconnect_by_data (G_OBJECT (viewer->vadj), viewer);
		g_object_unref (viewer->vadj);
		viewer->vadj = NULL;
	}

	if (viewer->priv->paint_pixbuf != NULL) {
		g_object_unref (viewer->priv->paint_pixbuf);
		viewer->priv->paint_pixbuf = NULL;
	}

	/* Chain up */
	G_OBJECT_CLASS (parent_class)->finalize (object);
}


static gdouble zooms[] = {                  0.05, 0.07, 0.10,
			  0.15, 0.20, 0.30, 0.50, 0.75, 1.0,
			  1.5 , 2.0 , 3.0 , 5.0 , 7.5,  10.0,
			  15.0, 20.0, 30.0, 50.0, 75.0, 100.0};

static const int nzooms = sizeof (zooms) / sizeof (gdouble);


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
get_zoomed_size (GthImageViewer *viewer,
		 int            *width,
		 int            *height,
		 double          zoom_level)
{
	if (gth_image_viewer_get_current_pixbuf (viewer) == NULL) {
		*width = 0;
		*height = 0;
	}
	else {
		int w, h;

		w = gth_image_viewer_get_image_width (viewer);
		h = gth_image_viewer_get_image_height (viewer);

		*width  = (int) floor ((double) w * zoom_level);
		*height = (int) floor ((double) h * zoom_level);
	}
}


static void
_gth_image_viewer_update_image_area (GthImageViewer *viewer)
{
	GtkWidget *widget;
	int        pixbuf_width;
	int        pixbuf_height;
	int        gdk_width;
	int        gdk_height;

	widget = GTK_WIDGET (viewer);
	get_zoomed_size (viewer, &pixbuf_width, &pixbuf_height, viewer->priv->zoom_level);
	gdk_width = widget->allocation.width - viewer->priv->frame_border2;
	gdk_height = widget->allocation.height - viewer->priv->frame_border2;

	viewer->image_area.x = MAX (viewer->priv->frame_border, (gdk_width - pixbuf_width) / 2);
	viewer->image_area.y = MAX (viewer->priv->frame_border, (gdk_height - pixbuf_height) / 2);
	viewer->image_area.width  = MIN (pixbuf_width, gdk_width);
	viewer->image_area.height = MIN (pixbuf_height, gdk_height);
}


static void
set_zoom (GthImageViewer *viewer,
	  gdouble         zoom_level,
	  int             center_x,
	  int             center_y)
{
	GtkWidget *widget = (GtkWidget*) viewer;
	gdouble    zoom_ratio;
	int        gdk_width, gdk_height;

	g_return_if_fail (viewer != NULL);
	g_return_if_fail (viewer->priv->loader != NULL);

	gdk_width = widget->allocation.width - viewer->priv->frame_border2;
	gdk_height = widget->allocation.height - viewer->priv->frame_border2;

	/* try to keep the center of the view visible. */

	zoom_ratio = zoom_level / viewer->priv->zoom_level;
	viewer->x_offset = ((viewer->x_offset + center_x) * zoom_ratio - gdk_width / 2);
	viewer->y_offset = ((viewer->y_offset + center_y) * zoom_ratio - gdk_height / 2);

 	/* reset zoom_fit unless we are performing a zoom to fit. */

	if (! viewer->priv->doing_zoom_fit)
		viewer->priv->fit = GTH_FIT_NONE;

	viewer->priv->zoom_level = zoom_level;

	_gth_image_viewer_update_image_area (viewer);
	gth_image_viewer_tool_zoom_changed (viewer->priv->tool);

	if (! viewer->priv->doing_zoom_fit) {
		gtk_widget_queue_resize (GTK_WIDGET (viewer));
		gtk_widget_queue_draw (GTK_WIDGET (viewer));
	}

	if (! viewer->priv->skip_zoom_change)
		g_signal_emit (G_OBJECT (viewer),
			       gth_image_viewer_signals[ZOOM_CHANGED],
			       0);
	else
		viewer->priv->skip_zoom_change = FALSE;
}


static int
to_255 (int v)
{
	return v * 255 / 65535;
}


static void
gth_image_viewer_realize (GtkWidget *widget)
{
	GthImageViewer *viewer;
	GdkWindowAttr   attributes;
	int             attributes_mask;

	g_return_if_fail (GTH_IS_IMAGE_VIEWER (widget));

	viewer = GTH_IMAGE_VIEWER (widget);
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

	viewer->priv->cursor = gdk_cursor_new (GDK_LEFT_PTR);
	viewer->priv->cursor_void = gth_cursor_get (widget->window, GTH_CURSOR_VOID);
	gdk_window_set_cursor (widget->window, viewer->priv->cursor);

	if (viewer->priv->transp_type == GTH_TRANSP_TYPE_NONE) {
		GdkColor color;
		guint    base_color;

		color = GTK_WIDGET (viewer)->style->bg[GTK_STATE_NORMAL];
		base_color = (0xFF000000
			      | (to_255 (color.red) << 16)
			      | (to_255 (color.green) << 8)
			      | (to_255 (color.blue) << 0));

		viewer->priv->check_color1 = base_color;
		viewer->priv->check_color2 = base_color;
	}

	gth_image_viewer_tool_realize (viewer->priv->tool);
}


static void
gth_image_viewer_unrealize (GtkWidget *widget)
{
	GthImageViewer *viewer;

	g_return_if_fail (GTH_IS_IMAGE_VIEWER (widget));

	viewer = GTH_IMAGE_VIEWER (widget);

	if (viewer->priv->cursor) {
		gdk_cursor_unref (viewer->priv->cursor);
		viewer->priv->cursor = NULL;
	}
	if (viewer->priv->cursor_void) {
		gdk_cursor_unref (viewer->priv->cursor_void);
		viewer->priv->cursor_void = NULL;
	}

	gth_image_viewer_tool_unrealize (viewer->priv->tool);

	GTK_WIDGET_CLASS (parent_class)->unrealize (widget);
}


static void
gth_image_viewer_map (GtkWidget *widget)
{
	GthImageViewer *viewer;

	g_return_if_fail (GTH_IS_IMAGE_VIEWER (widget));

	GTK_WIDGET_CLASS (parent_class)->map (widget);

	viewer = GTH_IMAGE_VIEWER (widget);
	gth_image_viewer_tool_map (viewer->priv->tool);
}


static void
gth_image_viewer_unmap (GtkWidget *widget)
{
	GthImageViewer *viewer;

	g_return_if_fail (GTH_IS_IMAGE_VIEWER (widget));

	viewer = GTH_IMAGE_VIEWER (widget);
	gth_image_viewer_tool_unmap (viewer->priv->tool);

	GTK_WIDGET_CLASS (parent_class)->unmap (widget);
}


static void
zoom_to_fit (GthImageViewer *viewer)
{
	GdkPixbuf *buf;
	int        gdk_width, gdk_height;
	double     x_level, y_level;
	double     new_zoom_level;

	buf = gth_image_viewer_get_current_pixbuf (viewer);

	gdk_width = GTK_WIDGET (viewer)->allocation.width - viewer->priv->frame_border2;
	gdk_height = GTK_WIDGET (viewer)->allocation.height - viewer->priv->frame_border2;
	x_level = (double) gdk_width / gdk_pixbuf_get_width (buf);
	y_level = (double) gdk_height / gdk_pixbuf_get_height (buf);

	new_zoom_level = (x_level < y_level) ? x_level : y_level;
	if (new_zoom_level > 0.0) {
		viewer->priv->doing_zoom_fit = TRUE;
		gth_image_viewer_set_zoom (viewer, new_zoom_level);
		viewer->priv->doing_zoom_fit = FALSE;
	}
}


static void
zoom_to_fit_width (GthImageViewer *viewer)
{
	GdkPixbuf *buf;
	double     new_zoom_level;
	int        gdk_width;

	buf = gth_image_viewer_get_current_pixbuf (viewer);

	gdk_width = GTK_WIDGET (viewer)->allocation.width - viewer->priv->frame_border2;
	new_zoom_level = (double) gdk_width / gdk_pixbuf_get_width (buf);

	if (new_zoom_level > 0.0) {
		viewer->priv->doing_zoom_fit = TRUE;
		gth_image_viewer_set_zoom (viewer, new_zoom_level);
		viewer->priv->doing_zoom_fit = FALSE;
	}
}


static void
gth_image_viewer_size_allocate (GtkWidget       *widget,
				GtkAllocation   *allocation)
{
	GthImageViewer *viewer;
	int             gdk_width, gdk_height;
	GdkPixbuf      *current_pixbuf;

	viewer = GTH_IMAGE_VIEWER (widget);

	widget->allocation = *allocation;
	gdk_width = allocation->width - viewer->priv->frame_border2;
	gdk_height = allocation->height - viewer->priv->frame_border2;

	current_pixbuf = gth_image_viewer_get_current_pixbuf (viewer);

	/* If a fit type is active update the zoom level. */

	if (! viewer->priv->is_void && (current_pixbuf != NULL)) {
		switch (viewer->priv->fit) {
		case GTH_FIT_SIZE:
			zoom_to_fit (viewer);
			break;
		case GTH_FIT_SIZE_IF_LARGER:
			if ((gdk_width < gdk_pixbuf_get_width (current_pixbuf))
				|| (gdk_height < gdk_pixbuf_get_height (current_pixbuf)))
		    	{
				zoom_to_fit (viewer);
		    	}
		    	else {
				viewer->priv->doing_zoom_fit = TRUE;
				gth_image_viewer_set_zoom (viewer, 1.0);
				viewer->priv->doing_zoom_fit = FALSE;
			}
			break;
		case GTH_FIT_WIDTH:
			zoom_to_fit_width (viewer);
			break;
		case GTH_FIT_WIDTH_IF_LARGER:
			if (gdk_width < gdk_pixbuf_get_width (current_pixbuf)) {
				zoom_to_fit_width (viewer);
			}
			else {
				viewer->priv->doing_zoom_fit = TRUE;
				gth_image_viewer_set_zoom (viewer, 1.0);
				viewer->priv->doing_zoom_fit = FALSE;
			}
			break;
		default:
			break;
		}
	}

	/* Check whether the offset is still valid. */

	if (current_pixbuf != NULL) {
		int width, height;

		get_zoomed_size (viewer, &width, &height, viewer->priv->zoom_level);

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
			g_signal_emit (G_OBJECT (viewer),
				       gth_image_viewer_signals[SIZE_CHANGED],
				       0);

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
	}
	else {
		viewer->hadj->lower     = 0.0;
		viewer->hadj->upper     = 1.0;
		viewer->hadj->value     = 0.0;
		viewer->hadj->page_size = 1.0;

		viewer->vadj->lower     = 0.0;
		viewer->vadj->upper     = 1.0;
		viewer->vadj->value     = 0.0;
		viewer->vadj->page_size = 1.0;
	}

	_gth_image_viewer_update_image_area (viewer);

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

	gth_image_viewer_tool_size_allocate (viewer->priv->tool, allocation);

	if (! viewer->priv->skip_size_change)
		g_signal_emit (G_OBJECT (viewer),
			       gth_image_viewer_signals[SIZE_CHANGED],
			       0);
	else
		viewer->priv->skip_size_change = FALSE;
}


static gboolean
gth_image_viewer_focus_in (GtkWidget     *widget,
			   GdkEventFocus *event)
{
	GTK_WIDGET_SET_FLAGS (widget, GTK_HAS_FOCUS);
	gtk_widget_queue_draw (widget);

	return TRUE;
}


static gboolean
gth_image_viewer_focus_out (GtkWidget     *widget,
			    GdkEventFocus *event)
{
	GTK_WIDGET_UNSET_FLAGS (widget, GTK_HAS_FOCUS);
	gtk_widget_queue_draw (widget);

	return TRUE;
}


static int
gth_image_viewer_key_press (GtkWidget   *widget,
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
create_pixbuf_from_iter (GthImageViewer *viewer)
{
	GdkPixbufAnimationIter *iter;

	iter = viewer->priv->iter;
	viewer->priv->frame_pixbuf = gdk_pixbuf_animation_iter_get_pixbuf (iter);
	viewer->priv->frame_delay = gdk_pixbuf_animation_iter_get_delay_time (iter);
}


static gboolean
change_frame_cb (gpointer data)
{
	GthImageViewer *viewer = data;

	if (viewer->priv->anim_id != 0) {
		g_source_remove (viewer->priv->anim_id);
		viewer->priv->anim_id = 0;
	}

	g_time_val_add (&viewer->priv->time, (glong) viewer->priv->frame_delay * 1000);
	gdk_pixbuf_animation_iter_advance (viewer->priv->iter, &viewer->priv->time);

	create_pixbuf_from_iter (viewer);

	viewer->priv->skip_zoom_change = TRUE;
	viewer->priv->skip_size_change = TRUE;

	gth_image_viewer_update_view (viewer);

	return FALSE;
}


static void
queue_frame_change (GthImageViewer *viewer)
{
	if (! viewer->priv->is_void
	    && viewer->priv->is_animation
	    && viewer->priv->play_animation
	    && (viewer->priv->anim_id == 0))
	{
		viewer->priv->anim_id = g_timeout_add (MAX (MINIMUM_DELAY, viewer->priv->frame_delay),
						       change_frame_cb,
						       viewer);
	}
}


static int
gth_image_viewer_expose (GtkWidget      *widget,
			 GdkEventExpose *event)
{
	GthImageViewer *viewer;
	int             gdk_width;
	int             gdk_height;

	viewer = GTH_IMAGE_VIEWER (widget);

	if (viewer->priv->rendering)
		return FALSE;

	viewer->priv->rendering = TRUE;

	/* Draw the background. */

	gdk_width = widget->allocation.width - viewer->priv->frame_border2;
	gdk_height = widget->allocation.height - viewer->priv->frame_border2;

	if ((viewer->image_area.x > viewer->priv->frame_border)
	    || (viewer->image_area.y > viewer->priv->frame_border)
	    || (viewer->image_area.width < gdk_width)
	    || (viewer->image_area.height < gdk_height))
	{
		int    rx, ry, rw, rh;
		GdkGC *gc;

		if (viewer->priv->black_bg)
			gc = widget->style->black_gc;
		else
			gc = widget->style->bg_gc[GTK_STATE_NORMAL];

		if (gth_image_viewer_get_current_pixbuf (viewer) == NULL) {
			gdk_draw_rectangle (widget->window,
					    gc,
					    TRUE,
					    0, 0,
					    widget->allocation.width,
					    widget->allocation.height);
		}
		else {
			/* If an image is present draw in four phases to avoid
			 * flickering. */

			/* Top rectangle. */
			rx = 0;
			ry = 0;
			rw = widget->allocation.width;
			rh = viewer->image_area.y;
			if ((rw > 0) && (rh > 0))
				gdk_draw_rectangle (widget->window,
						    gc,
						    TRUE,
						    rx, ry,
						    rw, rh);

			/* Bottom rectangle. */
			rx = 0;
			ry = viewer->image_area.y + viewer->image_area.height;
			rw = widget->allocation.width;
			rh = widget->allocation.height - viewer->image_area.y - viewer->image_area.height;
			if ((rw > 0) && (rh > 0))
				gdk_draw_rectangle (widget->window,
						    gc,
						    TRUE,
						    rx, ry,
						    rw, rh);

			/* Left rectangle. */
			rx = 0;
			ry = viewer->image_area.y - 1;
			rw = viewer->image_area.x;
			rh = viewer->image_area.height + 2;
			if ((rw > 0) && (rh > 0))
				gdk_draw_rectangle (widget->window,
						    gc,
						    TRUE,
						    rx, ry,
						    rw, rh);

			/* Right rectangle. */
			rx = viewer->image_area.x + viewer->image_area.width;
			ry = viewer->image_area.y - 1;
			rw = widget->allocation.width - viewer->image_area.x - viewer->image_area.width;
			rh = viewer->image_area.height + 2;
			if ((rw > 0) && (rh > 0))
				gdk_draw_rectangle (widget->window,
						    gc,
						    TRUE,
						    rx, ry,
						    rw, rh);
		}
	}

	/* Draw the frame. */

	if ((viewer->priv->frame_border > 0)
	    && (gth_image_viewer_get_current_pixbuf (viewer) != NULL))
	{
		int    x1, y1, x2, y2;
		GdkGC *gc;

		if (viewer->priv->black_bg)
			gc = widget->style->black_gc;
		else
			gc = widget->style->light_gc[GTK_STATE_NORMAL];
			/*gc = widget->style->dark_gc[GTK_STATE_NORMAL];*/

		x1 = viewer->image_area.x + viewer->image_area.width;
		y1 = viewer->image_area.y - 1;
		x2 = viewer->image_area.x + viewer->image_area.width;
		y2 = viewer->image_area.y + viewer->image_area.height;
		gdk_draw_line (widget->window,
			       gc,
			       x1, y1,
			       x2, y2);

		x1 = viewer->image_area.x - 1;
		y1 = viewer->image_area.y + viewer->image_area.height;
		x2 = viewer->image_area.x + viewer->image_area.width;
		y2 = viewer->image_area.y + viewer->image_area.height;
		gdk_draw_line (widget->window,
			       gc,
			       x1, y1,
			       x2, y2);

		if (viewer->priv->black_bg)
			gc = widget->style->black_gc;
		else
			gc = widget->style->dark_gc[GTK_STATE_NORMAL];

		x1 = viewer->image_area.x - 1;
		y1 = viewer->image_area.y - 1;
		x2 = viewer->image_area.x - 1;
		y2 = viewer->image_area.y + viewer->image_area.height;
		gdk_draw_line (widget->window,
			       gc,
			       x1, y1,
			       x2, y2);

		x1 = viewer->image_area.x - 1;
		y1 = viewer->image_area.y - 1;
		x2 = viewer->image_area.x + viewer->image_area.width;
		y2 = viewer->image_area.y - 1;
		gdk_draw_line (widget->window,
			       gc,
			       x1, y1,
			       x2, y2);
	}

	gth_image_viewer_tool_expose (viewer->priv->tool, &event->area);

	viewer->priv->rendering = FALSE;

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

	queue_frame_change (viewer);

	return FALSE;
}


static gboolean
gth_image_viewer_button_press (GtkWidget      *widget,
			       GdkEventButton *event)
{
	GthImageViewer *viewer = GTH_IMAGE_VIEWER (widget);
	int             retval;

	if (! GTK_WIDGET_HAS_FOCUS (widget)) {
		gtk_widget_grab_focus (widget);
		viewer->priv->just_focused = TRUE;
	}

	if (viewer->dragging)
		return FALSE;

	if ((event->type == GDK_2BUTTON_PRESS)
	    || (event->type == GDK_3BUTTON_PRESS))
	{
		viewer->priv->double_click = TRUE;
		return FALSE;
	}
	else
		viewer->priv->double_click = FALSE;

	retval = gth_image_viewer_tool_button_press (viewer->priv->tool, event);

	if (viewer->pressed) {
		viewer->event_x_start = viewer->event_x_prev = event->x;
		viewer->event_y_start = viewer->event_y_prev = event->y;
		viewer->drag_x = viewer->drag_x_start = viewer->drag_x_prev = event->x + viewer->x_offset;
		viewer->drag_y = viewer->drag_y_start = viewer->drag_y_prev = event->y + viewer->y_offset;
	}

	return retval;
}


static gboolean
gth_image_viewer_button_release (GtkWidget      *widget,
				 GdkEventButton *event)
{
	GthImageViewer *viewer = GTH_IMAGE_VIEWER (widget);

	if ((event->button == 1)
	    && ! viewer->dragging
	    && ! viewer->priv->double_click
	    && ! viewer->priv->just_focused)
	{
		g_signal_emit (G_OBJECT (viewer),
			       gth_image_viewer_signals[CLICKED],
			       0);
	}

	gth_image_viewer_tool_button_release (viewer->priv->tool, event);

	viewer->priv->just_focused = FALSE;
	viewer->pressed = FALSE;
	viewer->dragging = FALSE;

	return FALSE;
}


static void
expose_area (GthImageViewer *viewer,
	     int             x,
	     int             y,
	     int             width,
	     int             height)
{
	GdkEventExpose event;

	if (width == 0 || height == 0)
		return;

	event.area.x = x;
	event.area.y = y;
	event.area.width = width;
	event.area.height = height;

	gth_image_viewer_expose (GTK_WIDGET (viewer), &event);
}


static void
scroll_to (GthImageViewer *viewer,
	   int            *x_offset,
	   int            *y_offset)
{
	GdkDrawable *drawable;
	int          width, height;
	int          delta_x, delta_y;
	GdkEvent    *event;
	gboolean     replay_animation;
	int          gdk_width, gdk_height;

	g_return_if_fail (viewer != NULL);

	if (gth_image_viewer_get_current_pixbuf (viewer) == NULL)
		return;

	if (viewer->priv->rendering)
		return;

	get_zoomed_size (viewer, &width, &height, viewer->priv->zoom_level);

	drawable = GTK_WIDGET (viewer)->window;
	gdk_width = GTK_WIDGET (viewer)->allocation.width - viewer->priv->frame_border2;
	gdk_height = GTK_WIDGET (viewer)->allocation.height - viewer->priv->frame_border2;

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

	if (viewer->priv->next_scroll_repaint) {
		viewer->priv->next_scroll_repaint = FALSE;

		viewer->x_offset = *x_offset;
		viewer->y_offset = *y_offset;

		g_signal_emit (G_OBJECT (viewer),
			       gth_image_viewer_signals[REPAINTED],
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
		}
		else {
			src_x = delta_x;
			dest_x = 0;
		}

		if (delta_y < 0) {
			src_y = 0;
			dest_y = -delta_y;
		}
		else {
			src_y = delta_y;
			dest_y = 0;
		}

		gc = gdk_gc_new (drawable);
		gdk_gc_set_exposures (gc, TRUE);

		dest_x += viewer->priv->frame_border;
		dest_y += viewer->priv->frame_border;
		src_x += viewer->priv->frame_border;
		src_y += viewer->priv->frame_border;

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
		     viewer->priv->frame_border,
		     (delta_y < 0) ? viewer->priv->frame_border : viewer->priv->frame_border + gdk_height - abs (delta_y),
		     gdk_width,
		     abs (delta_y));

	expose_area (viewer,
		     (delta_x < 0) ? viewer->priv->frame_border : viewer->priv->frame_border + gdk_width - abs (delta_x),
		     viewer->priv->frame_border,
		     abs (delta_x),
		     gdk_height);

	/* Process graphics exposures */

	replay_animation = viewer->priv->play_animation;
	viewer->priv->play_animation = FALSE;
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
	viewer->priv->play_animation = replay_animation;
}


static gboolean
gth_image_viewer_motion_notify (GtkWidget      *widget,
				GdkEventMotion *event)
{
	GthImageViewer *viewer = GTH_IMAGE_VIEWER (widget);

	if (viewer->priv->rendering)
		return FALSE;

	if (viewer->pressed) {
		viewer->drag_x = event->x + viewer->x_offset;
		viewer->drag_y = event->y + viewer->y_offset;
	}

	gth_image_viewer_tool_motion_notify (viewer->priv->tool, event);

	if (viewer->pressed) {
		viewer->event_x_prev = event->x;
		viewer->event_y_prev = event->y;
		viewer->drag_x_prev = viewer->drag_x;
		viewer->drag_y_prev = viewer->drag_y;
	}

	return FALSE;
}


static gboolean
gth_image_viewer_scroll_event (GtkWidget      *widget,
			       GdkEventScroll *event)
{
	GthImageViewer *viewer = GTH_IMAGE_VIEWER (widget);
	GtkAdjustment  *adj;
	gdouble         new_value = 0.0;

	g_return_val_if_fail (GTH_IS_IMAGE_VIEWER (widget), FALSE);
	g_return_val_if_fail (event != NULL, FALSE);

	if (event->state & GDK_CONTROL_MASK) {
		if (event->direction == GDK_SCROLL_UP) {
			set_zoom (viewer,
				  get_next_zoom (viewer->priv->zoom_level),
				  (int) event->x,
				  (int) event->y);
			return TRUE;
		}
		if (event->direction == GDK_SCROLL_DOWN) {
			set_zoom (viewer,
				  get_prev_zoom (viewer->priv->zoom_level),
				  (int) event->x,
				  (int) event->y);
			return TRUE;
		}
	}

	if (event->direction == GDK_SCROLL_UP || event->direction == GDK_SCROLL_DOWN) {
		g_signal_emit (G_OBJECT (viewer),
			       gth_image_viewer_signals[MOUSE_WHEEL_SCROLL],
				0,
				event->direction);
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


static void
gth_image_viewer_style_set (GtkWidget *widget,
			    GtkStyle  *previous_style)
{
	GthImageViewer *viewer = GTH_IMAGE_VIEWER (widget);

	GTK_WIDGET_CLASS (parent_class)->style_set (widget, previous_style);

	if (viewer->priv->transp_type == GTH_TRANSP_TYPE_NONE) {
		GdkColor color;
		guint    base_color;

		color = GTK_WIDGET (viewer)->style->bg[GTK_STATE_NORMAL];
		base_color = (0xFF000000
			      | (to_255 (color.red) << 16)
			      | (to_255 (color.green) << 8)
			      | (to_255 (color.blue) << 0));

		viewer->priv->check_color1 = base_color;
		viewer->priv->check_color2 = base_color;
	}
}


static void
scroll_relative (GthImageViewer *viewer,
		 int             delta_x,
		 int             delta_y)
{
	gth_image_viewer_scroll_to (viewer,
				    viewer->x_offset + delta_x,
				    viewer->y_offset + delta_y);
}


static void
scroll_signal (GtkWidget     *widget,
	       GtkScrollType  xscroll_type,
	       GtkScrollType  yscroll_type)
{
	GthImageViewer *viewer = GTH_IMAGE_VIEWER (widget);
	int             xstep, ystep;

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


static gboolean
hadj_value_changed (GtkObject      *adj,
		    GthImageViewer *viewer)
{
	int x_ofs, y_ofs;

	x_ofs = (int) GTK_ADJUSTMENT (adj)->value;
	y_ofs = viewer->y_offset;
	scroll_to (viewer, &x_ofs, &y_ofs);

	return FALSE;
}


static gboolean
vadj_value_changed (GtkObject      *adj,
		    GthImageViewer *viewer)
{
	int x_ofs, y_ofs;

	x_ofs = viewer->x_offset;
	y_ofs = (int) GTK_ADJUSTMENT (adj)->value;
	scroll_to (viewer, &x_ofs, &y_ofs);

	return FALSE;
}


static void
set_scroll_adjustments (GtkWidget     *widget,
			GtkAdjustment *hadj,
			GtkAdjustment *vadj)
{
	GthImageViewer *viewer;

	g_return_if_fail (widget != NULL);
	g_return_if_fail (GTH_IS_IMAGE_VIEWER (widget));

	viewer = GTH_IMAGE_VIEWER (widget);

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
		g_signal_handlers_disconnect_by_data (G_OBJECT (viewer->vadj), viewer);
		g_object_unref (viewer->vadj);
		viewer->vadj = NULL;
	}

	if (viewer->hadj != hadj) {
		viewer->hadj = hadj;
		g_object_ref (viewer->hadj);
		g_object_ref_sink (viewer->hadj);

		g_signal_connect (G_OBJECT (viewer->hadj),
				  "value_changed",
				  G_CALLBACK (hadj_value_changed),
				  viewer);
	}

	if (viewer->vadj != vadj) {
		viewer->vadj = vadj;
		g_object_ref (viewer->vadj);
		g_object_ref_sink (viewer->vadj);

		g_signal_connect (G_OBJECT (viewer->vadj),
				  "value_changed",
				  G_CALLBACK (vadj_value_changed),
				  viewer);
	}
}


static void
gth_image_viewer_class_init (GthImageViewerClass *class)
{
	GObjectClass   *gobject_class;
	GtkWidgetClass *widget_class;
	GtkBindingSet  *binding_set;

	parent_class = g_type_class_peek_parent (class);
	g_type_class_add_private (class, sizeof (GthImageViewerPrivate));

	widget_class = (GtkWidgetClass*) class;
	gobject_class = (GObjectClass*) class;

	gth_image_viewer_signals[CLICKED] =
		g_signal_new ("clicked",
			      G_TYPE_FROM_CLASS (class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (GthImageViewerClass, clicked),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__VOID,
			      G_TYPE_NONE,
			      0);
	gth_image_viewer_signals[IMAGE_READY] =
		g_signal_new ("image_ready",
			      G_TYPE_FROM_CLASS (class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (GthImageViewerClass, image_ready),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__VOID,
			      G_TYPE_NONE,
			      0);
	gth_image_viewer_signals[ZOOM_IN] =
		g_signal_new ("zoom_in",
			      G_TYPE_FROM_CLASS (class),
			      G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
			      G_STRUCT_OFFSET (GthImageViewerClass, zoom_in),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__VOID,
			      G_TYPE_NONE,
			      0);
	gth_image_viewer_signals[ZOOM_OUT] =
		g_signal_new ("zoom_out",
			      G_TYPE_FROM_CLASS (class),
			      G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
			      G_STRUCT_OFFSET (GthImageViewerClass, zoom_out),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__VOID,
			      G_TYPE_NONE,
			      0);
	gth_image_viewer_signals[SET_ZOOM] =
		g_signal_new ("set_zoom",
			      G_TYPE_FROM_CLASS (class),
			      G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
			      G_STRUCT_OFFSET (GthImageViewerClass, set_zoom),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__DOUBLE,
			      G_TYPE_NONE,
			      1,
			      G_TYPE_DOUBLE);
	gth_image_viewer_signals[SET_FIT_MODE] =
		g_signal_new ("set_fit_mode",
			      G_TYPE_FROM_CLASS (class),
			      G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
			      G_STRUCT_OFFSET (GthImageViewerClass, set_fit_mode),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__ENUM,
			      G_TYPE_NONE,
			      1,
			      GTH_TYPE_FIT);
	gth_image_viewer_signals[ZOOM_CHANGED] =
		g_signal_new ("zoom_changed",
			      G_TYPE_FROM_CLASS (class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (GthImageViewerClass, zoom_changed),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__VOID,
			      G_TYPE_NONE,
			      0);
	gth_image_viewer_signals[SIZE_CHANGED] =
		g_signal_new ("size_changed",
			      G_TYPE_FROM_CLASS (class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (GthImageViewerClass, size_changed),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__VOID,
			      G_TYPE_NONE,
			      0);
	gth_image_viewer_signals[REPAINTED] =
		g_signal_new ("repainted",
			      G_TYPE_FROM_CLASS (class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (GthImageViewerClass, repainted),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__VOID,
			      G_TYPE_NONE,
			      0);
	gth_image_viewer_signals[MOUSE_WHEEL_SCROLL] =
		g_signal_new ("mouse_wheel_scroll",
			      G_TYPE_FROM_CLASS (class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (GthImageViewerClass, mouse_wheel_scroll),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__ENUM,
			      G_TYPE_NONE,
			      1,
			      GDK_TYPE_SCROLL_DIRECTION);

	class->set_scroll_adjustments = set_scroll_adjustments;
	widget_class->set_scroll_adjustments_signal =
		g_signal_new ("set_scroll_adjustments",
			      G_TYPE_FROM_CLASS (class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (GthImageViewerClass, set_scroll_adjustments),
			      NULL, NULL,
			      gth_marshal_VOID__POINTER_POINTER,
			      G_TYPE_NONE,
			      2,
			      GTK_TYPE_ADJUSTMENT,
			      GTK_TYPE_ADJUSTMENT);
	gth_image_viewer_signals[SCROLL] =
		g_signal_new ("scroll",
			      G_TYPE_FROM_CLASS (class),
			      G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
			      G_STRUCT_OFFSET (GthImageViewerClass, scroll),
			      NULL, NULL,
			      gth_marshal_VOID__ENUM_ENUM,
			      G_TYPE_NONE,
			      2,
			      GTK_TYPE_SCROLL_TYPE,
			      GTK_TYPE_SCROLL_TYPE);

	gobject_class->finalize = gth_image_viewer_finalize;

	widget_class->realize         = gth_image_viewer_realize;
	widget_class->unrealize       = gth_image_viewer_unrealize;
	widget_class->map             = gth_image_viewer_map;
	widget_class->unmap           = gth_image_viewer_unmap;
	widget_class->size_allocate   = gth_image_viewer_size_allocate;
	widget_class->focus_in_event  = gth_image_viewer_focus_in;
	widget_class->focus_out_event = gth_image_viewer_focus_out;
	widget_class->key_press_event = gth_image_viewer_key_press;

	widget_class->expose_event         = gth_image_viewer_expose;
	widget_class->button_press_event   = gth_image_viewer_button_press;
	widget_class->button_release_event = gth_image_viewer_button_release;
	widget_class->motion_notify_event  = gth_image_viewer_motion_notify;

	widget_class->scroll_event = gth_image_viewer_scroll_event;
	widget_class->style_set    = gth_image_viewer_style_set;

	class->clicked      = NULL;
	class->image_ready  = NULL;
	class->zoom_changed = NULL;
	class->scroll       = scroll_signal;
	class->zoom_in      = gth_image_viewer_zoom_in;
	class->zoom_out     = gth_image_viewer_zoom_out;
	class->set_zoom     = gth_image_viewer_set_zoom;
	class->set_fit_mode = gth_image_viewer_set_fit_mode;

	/* Add key bindings */

	binding_set = gtk_binding_set_by_class (class);

	/* For scrolling */

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

	/* Zoom in */

	gtk_binding_entry_add_signal (binding_set, GDK_plus, 0,
				      "zoom_in", 0);
	gtk_binding_entry_add_signal (binding_set, GDK_equal, 0,
				      "zoom_in", 0);
	gtk_binding_entry_add_signal (binding_set, GDK_KP_Add, 0,
				      "zoom_in", 0);

	/* Zoom out */

	gtk_binding_entry_add_signal (binding_set, GDK_minus, 0,
				      "zoom_out", 0);
	gtk_binding_entry_add_signal (binding_set, GDK_KP_Subtract, 0,
				      "zoom_out", 0);

	/* Set zoom */

	gtk_binding_entry_add_signal (binding_set, GDK_KP_Divide, 0,
				      "set_zoom", 1,
				      G_TYPE_DOUBLE, 1.0);
	gtk_binding_entry_add_signal (binding_set, GDK_1, 0,
				      "set_zoom", 1,
				      G_TYPE_DOUBLE, 1.0);
	gtk_binding_entry_add_signal (binding_set, GDK_z, 0,
				      "set_zoom", 1,
				      G_TYPE_DOUBLE, 1.0);
	gtk_binding_entry_add_signal (binding_set, GDK_2, 0,
				      "set_zoom", 1,
				      G_TYPE_DOUBLE, 2.0);
	gtk_binding_entry_add_signal (binding_set, GDK_3, 0,
				      "set_zoom", 1,
				      G_TYPE_DOUBLE, 3.0);

	/* Set fit mode */

	gtk_binding_entry_add_signal (binding_set, GDK_x, 0,
				      "set_fit_mode", 1,
				      GTH_TYPE_FIT, GTH_FIT_SIZE_IF_LARGER);
	gtk_binding_entry_add_signal (binding_set, GDK_KP_Multiply, 0,
				      "set_fit_mode", 1,
				      GTH_TYPE_FIT, GTH_FIT_SIZE_IF_LARGER);
	gtk_binding_entry_add_signal (binding_set, GDK_x, GDK_SHIFT_MASK,
				      "set_fit_mode", 1,
				      GTH_TYPE_FIT, GTH_FIT_SIZE);
	gtk_binding_entry_add_signal (binding_set, GDK_w, 0,
				      "set_fit_mode", 1,
				      GTH_TYPE_FIT, GTH_FIT_WIDTH_IF_LARGER);
	gtk_binding_entry_add_signal (binding_set, GDK_w, GDK_SHIFT_MASK,
				      "set_fit_mode", 1,
				      GTH_TYPE_FIT, GTH_FIT_WIDTH);
}


static void
create_first_pixbuf (GthImageViewer *viewer)
{
	g_return_if_fail (viewer != NULL);

	viewer->priv->frame_pixbuf = NULL;
	viewer->priv->frame_delay = 0;

	if (viewer->priv->iter != NULL)
		g_object_unref (viewer->priv->iter);

	g_get_current_time (&viewer->priv->time);

	viewer->priv->iter = gdk_pixbuf_animation_get_iter (viewer->priv->anim, &viewer->priv->time);
	create_pixbuf_from_iter (viewer);
}


static void
init_animation (GthImageViewer *viewer)
{
	g_return_if_fail (viewer != NULL);

	if (! viewer->priv->is_animation)
		return;

	if (viewer->priv->anim != NULL)
		g_object_unref (viewer->priv->anim);

	viewer->priv->anim = gth_image_loader_get_animation (viewer->priv->loader);
	if (viewer->priv->anim == NULL) {
		viewer->priv->is_animation = FALSE;
		return;
	}

	create_first_pixbuf (viewer);
}


static void
halt_animation (GthImageViewer *viewer)
{
	g_return_if_fail (viewer != NULL);

	if (viewer->priv->anim_id == 0)
		return;

	g_source_remove (viewer->priv->anim_id);
	viewer->priv->anim_id = 0;
}


static void
image_loader_ready_cb (GthImageLoader *il,
		       GError         *error,
		       gpointer        data)
{
	GthImageViewer     *viewer = data;
	GdkPixbufAnimation *anim;

	if (error != NULL) {
		g_clear_error (&error);

		gth_image_viewer_set_void (viewer);
		g_signal_emit (G_OBJECT (viewer),
			       gth_image_viewer_signals[IMAGE_READY],
			       0);
		return;
	}

	halt_animation (viewer);

	if (viewer->priv->reset_scrollbars) {
		viewer->x_offset = 0;
		viewer->y_offset = 0;
	}

	if (viewer->priv->anim != NULL) {
		g_object_unref (viewer->priv->anim);
		viewer->priv->anim = NULL;
	}

	anim = gth_image_loader_get_animation (viewer->priv->loader);
	viewer->priv->is_animation = (anim != NULL) && ! gdk_pixbuf_animation_is_static_image (anim);
	g_object_unref (anim);

	if (viewer->priv->is_animation)
		init_animation (viewer);

	gth_image_viewer_tool_image_changed (viewer->priv->tool);

	switch (viewer->priv->zoom_change) {
	case GTH_ZOOM_CHANGE_ACTUAL_SIZE:
		gth_image_viewer_set_zoom (viewer, 1.0);
		queue_frame_change (viewer);
		break;

	case GTH_ZOOM_CHANGE_KEEP_PREV:
		gth_image_viewer_update_view (viewer);
		break;

	case GTH_ZOOM_CHANGE_FIT_SIZE:
		gth_image_viewer_set_fit_mode (viewer, GTH_FIT_SIZE);
		queue_frame_change (viewer);
		break;

	case GTH_ZOOM_CHANGE_FIT_SIZE_IF_LARGER:
		gth_image_viewer_set_fit_mode (viewer, GTH_FIT_SIZE_IF_LARGER);
		queue_frame_change (viewer);
		break;

	case GTH_ZOOM_CHANGE_FIT_WIDTH:
		gth_image_viewer_set_fit_mode (viewer, GTH_FIT_WIDTH);
		queue_frame_change (viewer);
		break;

	case GTH_ZOOM_CHANGE_FIT_WIDTH_IF_LARGER:
		gth_image_viewer_set_fit_mode (viewer, GTH_FIT_WIDTH_IF_LARGER);
		queue_frame_change (viewer);
		break;
	}

	g_signal_emit (G_OBJECT (viewer),
		       gth_image_viewer_signals[IMAGE_READY],
		       0);
}


static void
gth_image_viewer_instance_init (GthImageViewer *viewer)
{
	GTK_WIDGET_SET_FLAGS (viewer, GTK_CAN_FOCUS);
	/*GTK_WIDGET_UNSET_FLAGS (viewer, GTK_DOUBLE_BUFFERED); FIXME: check if this is correct */

	viewer->priv = GTH_IMAGE_VIEWER_GET_PRIVATE (viewer);

	/* Initialize data. */

	viewer->priv->check_type = GTH_CHECK_TYPE_MIDTONE;
	viewer->priv->check_size = GTH_CHECK_SIZE_LARGE;
	viewer->priv->check_color1 = COLOR_GRAY_66;
	viewer->priv->check_color2 = COLOR_GRAY_99;

	viewer->priv->is_animation = FALSE;
	viewer->priv->play_animation = TRUE;
	viewer->priv->rendering = FALSE;
	viewer->priv->cursor_visible = TRUE;

	viewer->priv->frame_visible = TRUE;
	viewer->priv->frame_border = GTH_IMAGE_VIEWER_FRAME_BORDER;
	viewer->priv->frame_border2 = GTH_IMAGE_VIEWER_FRAME_BORDER2;

	viewer->priv->frame_pixbuf = NULL;
	viewer->priv->frame_delay = 0;
	viewer->priv->anim_id = 0;

	viewer->priv->loader = gth_image_loader_new (TRUE);
	g_signal_connect (G_OBJECT (viewer->priv->loader),
			  "ready",
			  G_CALLBACK (image_loader_ready_cb),
			  viewer);

	viewer->priv->anim = NULL;
	viewer->priv->iter = NULL;

	viewer->priv->zoom_level = 1.0;
	viewer->priv->zoom_quality = GTH_ZOOM_QUALITY_HIGH;
	viewer->priv->zoom_change = GTH_ZOOM_CHANGE_KEEP_PREV;
	viewer->priv->fit = GTH_FIT_SIZE_IF_LARGER;
	viewer->priv->doing_zoom_fit = FALSE;

	viewer->priv->skip_zoom_change = FALSE;
	viewer->priv->skip_size_change = FALSE;
	viewer->priv->next_scroll_repaint = FALSE;

	viewer->priv->is_void = TRUE;
	viewer->x_offset = 0;
	viewer->y_offset = 0;
	viewer->dragging = FALSE;
	viewer->priv->double_click = FALSE;
	viewer->priv->just_focused = FALSE;

	viewer->priv->black_bg = FALSE;

	viewer->priv->paint_pixbuf = NULL;
	viewer->priv->paint_max_width = 0;
	viewer->priv->paint_max_height = 0;
	viewer->priv->paint_bps = 0;
	viewer->priv->paint_color_space = GDK_COLORSPACE_RGB;

	viewer->priv->cursor = NULL;
	viewer->priv->cursor_void = NULL;

	viewer->priv->reset_scrollbars = TRUE;

	viewer->priv->tool = gth_image_dragger_new (viewer);

	/* Create the widget. */

	viewer->hadj = GTK_ADJUSTMENT (gtk_adjustment_new (0.0, 1.0, 0.0, 1.0, 1.0, 1.0));
	viewer->vadj = GTK_ADJUSTMENT (gtk_adjustment_new (0.0, 1.0, 0.0, 1.0, 1.0, 1.0));

	g_object_ref (viewer->hadj);
	g_object_ref_sink (viewer->hadj);
	g_object_ref (viewer->vadj);
	g_object_ref_sink (viewer->vadj);

	g_signal_connect (G_OBJECT (viewer->hadj),
			  "value_changed",
			  G_CALLBACK (hadj_value_changed),
			  viewer);
	g_signal_connect (G_OBJECT (viewer->vadj),
			  "value_changed",
			  G_CALLBACK (vadj_value_changed),
			  viewer);
}


GType
gth_image_viewer_get_type (void)
{
	static GType type = 0;

	if (! type) {
		GTypeInfo type_info = {
			sizeof (GthImageViewerClass),
			NULL,
			NULL,
			(GClassInitFunc) gth_image_viewer_class_init,
			NULL,
			NULL,
			sizeof (GthImageViewer),
			0,
			(GInstanceInitFunc) gth_image_viewer_instance_init
		};

		type = g_type_register_static (GTK_TYPE_WIDGET,
					       "GthImageViewer",
					       &type_info,
					       0);
	}

	return type;
}


GtkWidget*
gth_image_viewer_new (void)
{
	return GTK_WIDGET (g_object_new (GTH_TYPE_IMAGE_VIEWER, NULL));
}


/* -- gth_image_viewer_load -- */


typedef struct {
	GthImageViewer *viewer;
	GthFileData    *file_data;
} LoadImageData;


static void
load_image_data_free (LoadImageData *lidata)
{
	g_object_unref (lidata->file_data);
	g_free (lidata);
}


static void
load_image__step2 (LoadImageData *lidata)
{
	gth_image_loader_set_file_data (lidata->viewer->priv->loader, lidata->file_data);
	gth_image_loader_load (lidata->viewer->priv->loader);

	load_image_data_free (lidata);
}


void
gth_image_viewer_load (GthImageViewer *viewer,
		       GthFileData    *file_data)
{
	LoadImageData *lidata;

	g_return_if_fail (viewer != NULL);
	g_return_if_fail (file_data != NULL);

	viewer->priv->is_void = FALSE;
	halt_animation (viewer);

	lidata = g_new0 (LoadImageData, 1);
	lidata->viewer = viewer;
	lidata->file_data = g_object_ref (file_data);
	gth_image_loader_cancel (viewer->priv->loader, (DoneFunc) load_image__step2, lidata);
}


void
gth_image_viewer_load_from_file (GthImageViewer *viewer,
				 GFile          *file)
{
	GthFileData *file_data;

	file_data = gth_file_data_new (file, NULL);
	gth_image_viewer_load (viewer, file_data);

	g_object_unref (file_data);
}


/* -- gth_image_viewer_load_from_pixbuf_loader -- */


typedef struct {
	GthImageViewer *viewer;
	gpointer        data;
} GthImageViewerLoadData;


static void
load_from_pixbuf_loader__step2 (GthImageViewerLoadData *ivl_data)
{
	GthImageViewer  *viewer = ivl_data->viewer;
	GdkPixbufLoader *pixbuf_loader = ivl_data->data;

	gth_image_loader_load_from_pixbuf_loader (viewer->priv->loader, pixbuf_loader);

	g_object_unref (pixbuf_loader);
	g_free (ivl_data);
}


void
gth_image_viewer_load_from_pixbuf_loader (GthImageViewer  *viewer,
					  GdkPixbufLoader *pixbuf_loader)
{
	GthImageViewerLoadData *ivl_data;

	g_return_if_fail (viewer != NULL);
	g_return_if_fail (pixbuf_loader != NULL);

	viewer->priv->is_void = FALSE;
	halt_animation (viewer);

	ivl_data = g_new0 (GthImageViewerLoadData, 1);
	ivl_data->viewer = viewer;
	ivl_data->data = g_object_ref (pixbuf_loader);

	gth_image_loader_cancel (viewer->priv->loader,
				 (DoneFunc) load_from_pixbuf_loader__step2,
				 ivl_data);
}


/* -- gth_image_viewer_load_from_image_loader -- */


static void
load_from_image_loader__step2 (GthImageViewerLoadData *ivl_data)
{
	GthImageViewer *viewer = ivl_data->viewer;
	GthImageLoader *image_loader = ivl_data->data;

	gth_image_loader_load_from_image_loader (viewer->priv->loader, image_loader);
	g_object_unref (image_loader);
	g_free (ivl_data);
}


void
gth_image_viewer_load_from_image_loader (GthImageViewer *viewer,
					 GthImageLoader *image_loader)
{
	GthImageViewerLoadData *ivl_data;

	g_return_if_fail (viewer != NULL);
	g_return_if_fail (image_loader != NULL);

	viewer->priv->is_void = FALSE;
	halt_animation (viewer);

	ivl_data = g_new0 (GthImageViewerLoadData, 1);
	ivl_data->viewer = viewer;
	ivl_data->data = g_object_ref (image_loader);

	gth_image_loader_cancel (viewer->priv->loader,
				 (DoneFunc) load_from_image_loader__step2,
				 ivl_data);
}


void
gth_image_viewer_set_pixbuf (GthImageViewer *viewer,
			     GdkPixbuf      *pixbuf)
{
	g_return_if_fail (viewer != NULL);

	if (viewer->priv->is_animation || viewer->priv->rendering)
		return;

	viewer->priv->is_void = (pixbuf == NULL);

	gth_image_loader_set_pixbuf (viewer->priv->loader, pixbuf);
	gth_image_viewer_update_view (viewer);
}


void
gth_image_viewer_set_void (GthImageViewer *viewer)
{
	viewer->priv->is_void = TRUE;
	viewer->priv->is_animation = FALSE;

	halt_animation (viewer);

	viewer->priv->frame_pixbuf = NULL;

	if (viewer->priv->reset_scrollbars) {
		viewer->x_offset = 0;
		viewer->y_offset = 0;
	}

	gth_image_viewer_tool_image_changed (viewer->priv->tool);

	gtk_widget_queue_resize (GTK_WIDGET (viewer));
	gtk_widget_queue_draw (GTK_WIDGET (viewer));
}


gboolean
gth_image_viewer_is_void (GthImageViewer *viewer)
{
	return viewer->priv->is_void;
}


void
gth_image_viewer_update_view (GthImageViewer *viewer)
{
	if (viewer->priv->fit == GTH_FIT_NONE)
		gth_image_viewer_set_zoom (viewer, viewer->priv->zoom_level);
	else
		gth_image_viewer_set_fit_mode (viewer, viewer->priv->fit);
}


GthFileData *
gth_image_viewer_get_file (GthImageViewer *viewer)
{
	return gth_image_loader_get_file (viewer->priv->loader);
}


int
gth_image_viewer_get_image_width (GthImageViewer *viewer)
{
	GdkPixbuf *pixbuf;

	g_return_val_if_fail (viewer != NULL, 0);

	if (viewer->priv->anim != NULL)
		return gdk_pixbuf_animation_get_width (viewer->priv->anim);

	pixbuf = gth_image_loader_get_pixbuf (viewer->priv->loader);
	if (pixbuf != NULL)
		return gdk_pixbuf_get_width (pixbuf);

	return 0;
}


int
gth_image_viewer_get_image_height (GthImageViewer *viewer)
{
	GdkPixbuf *pixbuf;

	g_return_val_if_fail (viewer != NULL, 0);

	if (viewer->priv->anim != NULL)
		return gdk_pixbuf_animation_get_height (viewer->priv->anim);

	pixbuf = gth_image_loader_get_pixbuf (viewer->priv->loader);
	if (pixbuf != NULL)
		return gdk_pixbuf_get_height (pixbuf);

	return 0;
}


int
gth_image_viewer_get_image_bps (GthImageViewer *viewer)
{
	GdkPixbuf *pixbuf;

	g_return_val_if_fail (viewer != NULL, 0);

	if (viewer->priv->iter != NULL)
		pixbuf = gdk_pixbuf_animation_iter_get_pixbuf (viewer->priv->iter);
	else
		pixbuf = gth_image_loader_get_pixbuf (viewer->priv->loader);

	if (pixbuf != NULL)
		return gdk_pixbuf_get_bits_per_sample (pixbuf);

	return 0;
}


gboolean
gth_image_viewer_get_has_alpha (GthImageViewer *viewer)
{
	GdkPixbuf *pixbuf;

	g_return_val_if_fail (viewer != NULL, 0);

	if (viewer->priv->iter != NULL)
		pixbuf = gdk_pixbuf_animation_iter_get_pixbuf (viewer->priv->iter);
	else
		pixbuf = gth_image_loader_get_pixbuf (viewer->priv->loader);

	if (pixbuf != NULL)
		return gdk_pixbuf_get_has_alpha (pixbuf);

	return FALSE;
}


GdkPixbuf*
gth_image_viewer_get_current_pixbuf (GthImageViewer *viewer)
{
	g_return_val_if_fail (viewer != NULL, NULL);

	if (viewer->priv->is_void)
		return NULL;

	if (! viewer->priv->is_animation)
		return gth_image_loader_get_pixbuf (viewer->priv->loader);

	return viewer->priv->frame_pixbuf;
}


void
gth_image_viewer_start_animation (GthImageViewer *viewer)
{
	g_return_if_fail (viewer != NULL);

	viewer->priv->play_animation = TRUE;
	gth_image_viewer_update_view (viewer);
}


void
gth_image_viewer_stop_animation (GthImageViewer *viewer)
{
	g_return_if_fail (viewer != NULL);

	viewer->priv->play_animation = FALSE;
	halt_animation (viewer);
}


void
gth_image_viewer_step_animation (GthImageViewer *viewer)
{
	g_return_if_fail (viewer != NULL);

	if (!viewer->priv->is_animation) return;
	if (viewer->priv->play_animation) return;
	if (viewer->priv->rendering) return;

	change_frame_cb (viewer);
}


gboolean
gth_image_viewer_is_animation (GthImageViewer *viewer)
{
	g_return_val_if_fail (viewer != NULL, FALSE);

	return viewer->priv->is_animation;
}


gboolean
gth_image_viewer_is_playing_animation (GthImageViewer *viewer)
{
	g_return_val_if_fail (viewer != NULL, FALSE);

	return viewer->priv->is_animation && viewer->priv->play_animation;
}


void
gth_image_viewer_set_zoom (GthImageViewer *viewer,
			   gdouble         zoom_level)
{
	GtkWidget *widget = (GtkWidget*) viewer;

	set_zoom (viewer,
		  zoom_level,
		  (widget->allocation.width - viewer->priv->frame_border2) / 2,
		  (widget->allocation.height - viewer->priv->frame_border2) / 2);
}


gdouble
gth_image_viewer_get_zoom (GthImageViewer *viewer)
{
	return viewer->priv->zoom_level;
}


void
gth_image_viewer_set_zoom_quality (GthImageViewer *viewer,
				   GthZoomQuality  quality)
{
	viewer->priv->zoom_quality = quality;
}


GthZoomQuality
gth_image_viewer_get_zoom_quality (GthImageViewer *viewer)
{
	return viewer->priv->zoom_quality;
}


void
gth_image_viewer_set_zoom_change (GthImageViewer *viewer,
				  GthZoomChange   zoom_change)
{
	viewer->priv->zoom_change = zoom_change;
}


GthZoomChange
gth_image_viewer_get_zoom_change (GthImageViewer *viewer)
{
	return viewer->priv->zoom_change;
}


void
gth_image_viewer_zoom_in (GthImageViewer *viewer)
{
	if (gth_image_viewer_get_current_pixbuf (viewer) == NULL)
		return;
	gth_image_viewer_set_zoom (viewer, get_next_zoom (viewer->priv->zoom_level));
}


void
gth_image_viewer_zoom_out (GthImageViewer *viewer)
{
	if (gth_image_viewer_get_current_pixbuf (viewer) == NULL)
		return;
	gth_image_viewer_set_zoom (viewer, get_prev_zoom (viewer->priv->zoom_level));
}


void
gth_image_viewer_set_fit_mode (GthImageViewer *viewer,
			       GthFit          fit_mode)
{
	viewer->priv->fit = fit_mode;
	if (viewer->priv->is_void)
		return;
	gtk_widget_queue_resize (GTK_WIDGET (viewer));
}


GthFit
gth_image_viewer_get_fit_mode (GthImageViewer *viewer)
{
	return viewer->priv->fit;
}


void
gth_image_viewer_set_transp_type (GthImageViewer *viewer,
				  GthTranspType   transp_type)
{
	GdkColor color;
	guint    base_color;

	g_return_if_fail (viewer != NULL);

	viewer->priv->transp_type = transp_type;

	color = GTK_WIDGET (viewer)->style->bg[GTK_STATE_NORMAL];
	base_color = (0xFF000000
		      | (to_255 (color.red) << 16)
		      | (to_255 (color.green) << 8)
		      | (to_255 (color.blue) << 0));

	switch (transp_type) {
	case GTH_TRANSP_TYPE_BLACK:
		viewer->priv->check_color1 = COLOR_GRAY_00;
		viewer->priv->check_color2 = COLOR_GRAY_00;
		break;

	case GTH_TRANSP_TYPE_NONE:
		viewer->priv->check_color1 = base_color;
		viewer->priv->check_color2 = base_color;
		break;

	case GTH_TRANSP_TYPE_WHITE:
		viewer->priv->check_color1 = COLOR_GRAY_FF;
		viewer->priv->check_color2 = COLOR_GRAY_FF;
		break;

	case GTH_TRANSP_TYPE_CHECKED:
		switch (viewer->priv->check_type) {
		case GTH_CHECK_TYPE_DARK:
			viewer->priv->check_color1 = COLOR_GRAY_00;
			viewer->priv->check_color2 = COLOR_GRAY_33;
			break;

		case GTH_CHECK_TYPE_MIDTONE:
			viewer->priv->check_color1 = COLOR_GRAY_66;
			viewer->priv->check_color2 = COLOR_GRAY_99;
			break;

		case GTH_CHECK_TYPE_LIGHT:
			viewer->priv->check_color1 = COLOR_GRAY_CC;
			viewer->priv->check_color2 = COLOR_GRAY_FF;
			break;
		}
		break;
	}
}


GthTranspType
gth_image_viewer_get_transp_type (GthImageViewer *viewer)
{
	return viewer->priv->transp_type;
}


void
gth_image_viewer_set_check_type (GthImageViewer *viewer,
				 GthCheckType    check_type)
{
	viewer->priv->check_type = check_type;
}


GthCheckType
gth_image_viewer_get_check_type (GthImageViewer *viewer)
{
	return viewer->priv->check_type;
}


void
gth_image_viewer_set_check_size (GthImageViewer *viewer,
				 GthCheckSize    check_size)
{
	viewer->priv->check_size = check_size;
}


GthCheckSize
gth_image_viewer_get_check_size (GthImageViewer *viewer)
{
	return viewer->priv->check_size;
}


void
gth_image_viewer_clicked (GthImageViewer *viewer)
{
	g_signal_emit (G_OBJECT (viewer), gth_image_viewer_signals[CLICKED], 0);
}


void
gth_image_viewer_set_size_request (GthImageViewer *viewer,
				   int             width,
				   int             height)
{
	GTK_WIDGET (viewer)->requisition.width = width;
	GTK_WIDGET (viewer)->requisition.height = height;
	gtk_widget_queue_resize (GTK_WIDGET (viewer));
}


void
gth_image_viewer_set_black_background (GthImageViewer *viewer,
				       gboolean        set_black)
{
	viewer->priv->black_bg = set_black;
	gtk_widget_queue_draw (GTK_WIDGET (viewer));
}


gboolean
gth_image_viewer_is_black_background (GthImageViewer *viewer)
{
	return viewer->priv->black_bg;
}


void
gth_image_viewer_get_adjustments (GthImageViewer  *self,
				  GtkAdjustment  **hadj,
				  GtkAdjustment  **vadj)
{
	GthImageViewer *viewer;

	viewer = GTH_IMAGE_VIEWER (self);
	if (hadj != NULL)
		*hadj = viewer->hadj;
	if (vadj != NULL)
		*vadj = viewer->vadj;
}


void
gth_image_viewer_set_tool (GthImageViewer     *viewer,
			   GthImageViewerTool *tool)
{
	_g_object_unref (viewer->priv->tool);
	if (tool == NULL)
		viewer->priv->tool = gth_image_dragger_new (viewer);
	else
		viewer->priv->tool = g_object_ref (tool);
	if (GTK_WIDGET_REALIZED (viewer))
		gth_image_viewer_tool_realize (viewer->priv->tool);
	gth_image_viewer_tool_image_changed (viewer->priv->tool);
	gtk_widget_queue_resize (GTK_WIDGET (viewer));
	gtk_widget_queue_draw (GTK_WIDGET (viewer));
}


void
gth_image_viewer_scroll_to (GthImageViewer *viewer,
			    int             x_offset,
			    int             y_offset)
{
	g_return_if_fail (viewer != NULL);

	if (gth_image_viewer_get_current_pixbuf (viewer) == NULL)
		return;

	if (viewer->priv->rendering)
		return;

	scroll_to (viewer, &x_offset, &y_offset);

	g_signal_handlers_block_by_data (G_OBJECT (viewer->hadj), viewer);
	g_signal_handlers_block_by_data (G_OBJECT (viewer->vadj), viewer);
	gtk_adjustment_set_value (viewer->hadj, viewer->x_offset);
	gtk_adjustment_set_value (viewer->vadj, viewer->y_offset);
	g_signal_handlers_unblock_by_data (G_OBJECT (viewer->hadj), viewer);
	g_signal_handlers_unblock_by_data (G_OBJECT (viewer->vadj), viewer);
}


void
gth_image_viewer_scroll_step_x (GthImageViewer *viewer,
				gboolean        increment)
{
	scroll_relative (viewer,
			 (increment ? 1 : -1) * viewer->hadj->step_increment,
			 0);
}


void
gth_image_viewer_scroll_step_y (GthImageViewer *viewer,
				gboolean        increment)
{
	scroll_relative (viewer,
			 0,
			 (increment ? 1 : -1) * viewer->vadj->step_increment);
}


void
gth_image_viewer_scroll_page_x (GthImageViewer *viewer,
				gboolean        increment)
{
	scroll_relative (viewer,
			 (increment ? 1 : -1) * viewer->hadj->page_increment,
			 0);
}


void
gth_image_viewer_scroll_page_y (GthImageViewer *viewer,
				gboolean        increment)
{
	scroll_relative (viewer,
			 0,
			 (increment ? 1 : -1) * viewer->vadj->page_increment);
}


void
gth_image_viewer_get_scroll_offset (GthImageViewer *viewer,
				    int            *x,
				    int            *y)
{
	*x = viewer->x_offset;
	*y = viewer->y_offset;
}


void
gth_image_viewer_set_reset_scrollbars (GthImageViewer *viewer,
				       gboolean        reset)
{
	viewer->priv->reset_scrollbars = reset;
}


gboolean
gth_image_viewer_get_reset_scrollbars (GthImageViewer *viewer)
{
	return viewer->priv->reset_scrollbars;
}


void
gth_image_viewer_show_cursor (GthImageViewer *viewer)
{
	if (viewer->priv->cursor_visible)
		return;

	viewer->priv->cursor_visible = TRUE;
	gdk_window_set_cursor (GTK_WIDGET (viewer)->window, viewer->priv->cursor);
}


void
gth_image_viewer_hide_cursor (GthImageViewer *viewer)
{
	if (! viewer->priv->cursor_visible)
		return;

	viewer->priv->cursor_visible = FALSE;
	gdk_window_set_cursor (GTK_WIDGET (viewer)->window, viewer->priv->cursor_void);
}


void
gth_image_viewer_set_cursor (GthImageViewer *viewer,
			     GdkCursor      *cursor)
{
	if (cursor != NULL)
		gdk_cursor_ref (cursor);

	if (viewer->priv->cursor != NULL) {
		gdk_cursor_unref (viewer->priv->cursor);
		viewer->priv->cursor = NULL;
	}
	if (cursor != NULL)
		viewer->priv->cursor = cursor;
	else
		viewer->priv->cursor = gdk_cursor_ref (viewer->priv->cursor_void);

	if (! GTK_WIDGET_REALIZED (viewer))
		return;

	if (viewer->priv->cursor_visible)
		gdk_window_set_cursor (GTK_WIDGET (viewer)->window, viewer->priv->cursor);
}


gboolean
gth_image_viewer_is_cursor_visible (GthImageViewer *viewer)
{
	return viewer->priv->cursor_visible;
}


void
gth_image_viewer_show_frame (GthImageViewer *viewer)
{
	viewer->priv->frame_visible = TRUE;
	viewer->priv->frame_border = GTH_IMAGE_VIEWER_FRAME_BORDER;
	viewer->priv->frame_border2 = GTH_IMAGE_VIEWER_FRAME_BORDER2;

	gtk_widget_queue_resize (GTK_WIDGET (viewer));
}


void
gth_image_viewer_hide_frame (GthImageViewer *viewer)
{
	viewer->priv->frame_visible = FALSE;
	viewer->priv->frame_border = 0;
	viewer->priv->frame_border2 = 0;

	gtk_widget_queue_resize (GTK_WIDGET (viewer));
}


gboolean
gth_image_viewer_is_frame_visible (GthImageViewer *viewer)
{
	return viewer->priv->frame_visible;
}


void
gth_image_viewer_paint (GthImageViewer *viewer,
			GdkPixbuf      *pixbuf,
			int             src_x,
			int             src_y,
			int             dest_x,
			int             dest_y,
			int             width,
			int             height,
			int             interp_type)
{
	double         zoom_level;
	int            bits_per_sample;
	GdkColorspace  color_space;

	zoom_level = viewer->priv->zoom_level;

	color_space = gdk_pixbuf_get_colorspace (pixbuf);
	bits_per_sample = gdk_pixbuf_get_bits_per_sample (pixbuf);

	if ((viewer->priv->paint_pixbuf == NULL)
	    || (viewer->priv->paint_max_width < width)
	    || (viewer->priv->paint_max_height < height)
	    || (viewer->priv->paint_bps != bits_per_sample)
	    || (viewer->priv->paint_color_space != color_space))
	{
		if (viewer->priv->paint_pixbuf != NULL)
			g_object_unref (viewer->priv->paint_pixbuf);
		viewer->priv->paint_pixbuf = gdk_pixbuf_new (color_space,
							     FALSE,
							     bits_per_sample,
							     width,
							     height);
		g_return_if_fail (viewer->priv->paint_pixbuf != NULL);

		viewer->priv->paint_max_width = width;
		viewer->priv->paint_max_height = height;
		viewer->priv->paint_color_space = color_space;
		viewer->priv->paint_bps = bits_per_sample;
	}

	if (gdk_pixbuf_get_has_alpha (pixbuf))
		gdk_pixbuf_composite_color (pixbuf,
					    viewer->priv->paint_pixbuf,
					    0, 0,
					    width, height,
					    (double) -src_x,
					    (double) -src_y,
					    zoom_level,
					    zoom_level,
					    interp_type,
					    255,
					    src_x, src_y,
					    viewer->priv->check_size,
					    viewer->priv->check_color1,
					    viewer->priv->check_color2);
	else
		gdk_pixbuf_scale (pixbuf,
				  viewer->priv->paint_pixbuf,
				  0, 0,
				  width, height,
				  (double) -src_x,
				  (double) -src_y,
				  zoom_level,
				  zoom_level,
				  interp_type);

	gdk_draw_rgb_image_dithalign (GTK_WIDGET (viewer)->window,
				      GTK_WIDGET (viewer)->style->black_gc,
				      dest_x, dest_y,
				      width, height,
				      GDK_RGB_DITHER_MAX,
				      gdk_pixbuf_get_pixels (viewer->priv->paint_pixbuf),
				      gdk_pixbuf_get_rowstride (viewer->priv->paint_pixbuf),
				      dest_x, dest_y);

#if 0
	gdk_draw_rectangle (GTK_WIDGET (viewer)->window,
			    GTK_WIDGET (viewer)->style->black_gc,
			    FALSE,
			    dest_x, dest_y,
			    width, height);
#endif
}


void
gth_image_viewer_crop_area (GthImageViewer *viewer,
			    GdkRectangle   *area)
{
	GtkWidget *widget = GTK_WIDGET (viewer);

	area->width = MIN (area->width, widget->allocation.width - viewer->priv->frame_border2);
	area->width = MIN (area->height, widget->allocation.height - viewer->priv->frame_border2);
}
