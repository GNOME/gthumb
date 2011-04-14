/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2001-2011 Free Software Foundation, Inc.
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
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <config.h>
#include <stdlib.h>
#include <math.h>
#include <glib/gi18n.h>
#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>
#include "cairo-utils.h"
#include "gth-enum-types.h"
#include "gth-image-dragger.h"
#include "gth-image-viewer.h"
#include "gth-marshal.h"
#include "glib-utils.h"
#include "pixbuf-utils.h"

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


enum {
	CLICKED,
	ZOOM_IN,
	ZOOM_OUT,
	SET_ZOOM,
	SET_FIT_MODE,
	ZOOM_CHANGED,
	SIZE_CHANGED,
	SCROLL,
	LAST_SIGNAL
};


struct _GthImageViewerPrivate {
	cairo_surface_t        *surface;
	GdkPixbufAnimation     *animation;
	int                     original_width;
	int                     original_height;

	GdkPixbufAnimationIter *iter;
	GTimeVal                time;               /* Timer used to get the current frame. */
	guint                   anim_id;
	cairo_surface_t        *iter_surface;
	GdkPixbufAnimationIter *last_iter;

	gboolean                is_animation;
	gboolean                play_animation;
	gboolean                cursor_visible;

	gboolean                frame_visible;
	int                     frame_border;
	int                     frame_border2;

	GthTranspType           transp_type;
	GthCheckType            check_type;
	int                     check_size;
	guint32                 check_color1;
	guint32                 check_color2;

	GthImageViewerTool     *tool;

	GdkCursor              *cursor;
	GdkCursor              *cursor_void;

	gboolean                enable_zoom_with_keys;
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

	gboolean                black_bg;

	gboolean                skip_zoom_change;
	gboolean                skip_size_change;

	gboolean                reset_scrollbars;
	guint                   update_adjustment_values;

	GList                  *painters;
};


static gpointer parent_class = NULL;
static guint gth_image_viewer_signals[LAST_SIGNAL] = { 0 };


typedef struct {
	GthImageViewerPaintFunc func;
	gpointer                user_data;
} PainterData;


static void
painter_data_free (PainterData *painter_data)
{
	g_free (painter_data);
}


static void
gth_image_viewer_finalize (GObject *object)
{
	GthImageViewer *self;

	g_return_if_fail (GTH_IS_IMAGE_VIEWER (object));

	self = GTH_IMAGE_VIEWER (object);

	if (self->priv->anim_id != 0)
		g_source_remove (self->priv->anim_id);

	if (self->priv->cursor != NULL)
		gdk_cursor_unref (self->priv->cursor);

	if (self->priv->cursor_void != NULL)
		gdk_cursor_unref (self->priv->cursor_void);

	if (self->hadj != NULL) {
		g_signal_handlers_disconnect_by_data (G_OBJECT (self->hadj), self);
		g_object_unref (self->hadj);
	}

	if (self->vadj != NULL) {
		g_signal_handlers_disconnect_by_data (G_OBJECT (self->vadj), self);
		g_object_unref (self->vadj);
	}

	g_list_foreach (self->priv->painters, (GFunc) painter_data_free, NULL);

	_g_clear_object (&self->priv->animation);
	_g_clear_object (&self->priv->iter);
	_cairo_clear_surface (&self->priv->iter_surface);
	_cairo_clear_surface (&self->priv->surface);

	G_OBJECT_CLASS (parent_class)->finalize (object);
}


static double zooms[] = {                  0.05, 0.07, 0.10,
			 0.15, 0.20, 0.30, 0.50, 0.75, 1.0,
			 1.5 , 2.0 , 3.0 , 5.0 , 7.5,  10.0,
			 15.0, 20.0, 30.0, 50.0, 75.0, 100.0};

static const int nzooms = sizeof (zooms) / sizeof (gdouble);


static double
get_next_zoom (gdouble zoom)
{
	int i;

	i = 0;
	while ((i < nzooms) && (zooms[i] <= zoom))
		i++;
	i = CLAMP (i, 0, nzooms - 1);

	return zooms[i];
}


static double
get_prev_zoom (double zoom)
{
	int i;

	i = nzooms - 1;
	while ((i >= 0) && (zooms[i] >= zoom))
		i--;
	i = CLAMP (i, 0, nzooms - 1);

	return zooms[i];
}


static void
_gth_image_viewer_get_zoomed_size (GthImageViewer *self,
		 	 	   int            *width,
		 	 	   int            *height)
{
	if (gth_image_viewer_get_current_image (self) == NULL) {
		*width = 0;
		*height = 0;
	}
	else {
		*width  = (int) floor ((double) self->priv->original_width * self->priv->zoom_level);
		*height = (int) floor ((double) self->priv->original_height * self->priv->zoom_level);
	}
}


static void
_gth_image_viewer_update_image_area (GthImageViewer *self)
{
	GtkWidget     *widget;
	int            zoomed_width;
	int            zoomed_height;
	GtkAllocation  allocation;
	int            gdk_width;
	int            gdk_height;

	widget = GTK_WIDGET (self);

	gtk_widget_get_allocation (widget, &allocation);
	gdk_width = allocation.width - self->priv->frame_border2;
	gdk_height = allocation.height - self->priv->frame_border2;

	_gth_image_viewer_get_zoomed_size (self, &zoomed_width, &zoomed_height);

	self->image_area.x = MAX (self->priv->frame_border, (gdk_width - zoomed_width) / 2);
	self->image_area.y = MAX (self->priv->frame_border, (gdk_height - zoomed_height) / 2);
	self->image_area.width  = MIN (zoomed_width, gdk_width);
	self->image_area.height = MIN (zoomed_height, gdk_height);
}


static void
set_zoom (GthImageViewer *self,
	  gdouble         zoom_level,
	  int             center_x,
	  int             center_y)
{
	GtkWidget     *widget = (GtkWidget*) self;
	GtkAllocation  allocation;
	gdouble        zoom_ratio;
	int            gdk_width, gdk_height;

	g_return_if_fail (self != NULL);

	gtk_widget_get_allocation (widget, &allocation);
	gdk_width = allocation.width - self->priv->frame_border2;
	gdk_height = allocation.height - self->priv->frame_border2;

	/* try to keep the center of the view visible. */

	zoom_ratio = zoom_level / self->priv->zoom_level;
	self->x_offset = ((self->x_offset + center_x) * zoom_ratio - gdk_width / 2);
	self->y_offset = ((self->y_offset + center_y) * zoom_ratio - gdk_height / 2);

 	/* reset zoom_fit unless we are performing a zoom to fit. */

	if (! self->priv->doing_zoom_fit)
		self->priv->fit = GTH_FIT_NONE;

	self->priv->zoom_level = zoom_level;

	_gth_image_viewer_update_image_area (self);
	gth_image_viewer_tool_zoom_changed (self->priv->tool);

	if (! self->priv->doing_zoom_fit) {
		gtk_widget_queue_resize (GTK_WIDGET (self));
		gtk_widget_queue_draw (GTK_WIDGET (self));
	}

	if (! self->priv->skip_zoom_change)
		g_signal_emit (G_OBJECT (self),
			       gth_image_viewer_signals[ZOOM_CHANGED],
			       0);
	else
		self->priv->skip_zoom_change = FALSE;
}


static int
to_255 (int v)
{
	return v * 255 / 65535;
}


static void
gth_image_viewer_realize (GtkWidget *widget)
{
	GthImageViewer *self;
	GtkAllocation   allocation;
	GdkWindowAttr   attributes;
	int             attributes_mask;
	GdkWindow      *window;
	GtkStyle       *style;

	g_return_if_fail (GTH_IS_IMAGE_VIEWER (widget));

	self = GTH_IMAGE_VIEWER (widget);
	gtk_widget_set_realized (widget, TRUE);
	gtk_widget_get_allocation (widget, &allocation);

	attributes.window_type = GDK_WINDOW_CHILD;
	attributes.x           = allocation.x;
	attributes.y           = allocation.y;
	attributes.width       = allocation.width;
	attributes.height      = allocation.height;
	attributes.wclass      = GDK_INPUT_OUTPUT;
	attributes.visual      = gtk_widget_get_visual (widget);
	attributes.colormap    = gtk_widget_get_colormap (widget);
	attributes.event_mask  = (gtk_widget_get_events (widget)
				  | GDK_EXPOSURE_MASK
				  | GDK_BUTTON_PRESS_MASK
				  | GDK_BUTTON_RELEASE_MASK
				  | GDK_POINTER_MOTION_MASK
				  | GDK_POINTER_MOTION_HINT_MASK
				  | GDK_BUTTON_MOTION_MASK
				  | GDK_STRUCTURE_MASK);

	attributes_mask        = (GDK_WA_X
				  | GDK_WA_Y
				  | GDK_WA_VISUAL
				  | GDK_WA_COLORMAP);

	window = gdk_window_new (gtk_widget_get_parent_window (widget),
				 &attributes,
				 attributes_mask);
	gtk_widget_set_window (widget, window);
	gdk_window_set_user_data (window, self);

	style = gtk_widget_get_style (widget);
	style = gtk_style_attach (style, window);
	gtk_style_set_background (style, window, GTK_STATE_NORMAL);
	gtk_widget_set_style (widget, style);

	self->priv->cursor = gdk_cursor_new (GDK_LEFT_PTR);
	self->priv->cursor_void = gdk_cursor_new_for_display (gtk_widget_get_display (widget), GDK_BLANK_CURSOR);
	if (self->priv->cursor_visible)
		gdk_window_set_cursor (window, self->priv->cursor);
	else
		gdk_window_set_cursor (window, self->priv->cursor_void);

	if (self->priv->transp_type == GTH_TRANSP_TYPE_NONE) {
		GdkColor color;
		guint    base_color;

		color = style->bg[GTK_STATE_NORMAL];
		base_color = (0xFF000000
			      | (to_255 (color.red) << 16)
			      | (to_255 (color.green) << 8)
			      | (to_255 (color.blue) << 0));

		self->priv->check_color1 = base_color;
		self->priv->check_color2 = base_color;
	}

	gth_image_viewer_tool_realize (self->priv->tool);
}


static void
gth_image_viewer_unrealize (GtkWidget *widget)
{
	GthImageViewer *self;

	g_return_if_fail (GTH_IS_IMAGE_VIEWER (widget));

	self = GTH_IMAGE_VIEWER (widget);

	if (self->priv->cursor) {
		gdk_cursor_unref (self->priv->cursor);
		self->priv->cursor = NULL;
	}
	if (self->priv->cursor_void) {
		gdk_cursor_unref (self->priv->cursor_void);
		self->priv->cursor_void = NULL;
	}

	if (self->priv->update_adjustment_values != 0) {
		g_source_remove (self->priv->update_adjustment_values);
		self->priv->update_adjustment_values = 0;
	}

	gth_image_viewer_tool_unrealize (self->priv->tool);

	GTK_WIDGET_CLASS (parent_class)->unrealize (widget);
}


static void
gth_image_viewer_map (GtkWidget *widget)
{
	GthImageViewer *self;

	g_return_if_fail (GTH_IS_IMAGE_VIEWER (widget));

	GTK_WIDGET_CLASS (parent_class)->map (widget);

	self = GTH_IMAGE_VIEWER (widget);
	gth_image_viewer_tool_map (self->priv->tool);
}


static void
gth_image_viewer_unmap (GtkWidget *widget)
{
	GthImageViewer *self;

	g_return_if_fail (GTH_IS_IMAGE_VIEWER (widget));

	self = GTH_IMAGE_VIEWER (widget);
	gth_image_viewer_tool_unmap (self->priv->tool);

	GTK_WIDGET_CLASS (parent_class)->unmap (widget);
}


static void
zoom_to_fit (GthImageViewer *self)
{
	GtkAllocation allocation;
	int           gdk_width;
	int           gdk_height;
	int           original_width;
	int           original_height;
	double        x_level;
	double        y_level;
	double        new_zoom_level;

	gtk_widget_get_allocation (GTK_WIDGET (self), &allocation);
	gdk_width = allocation.width - self->priv->frame_border2;
	gdk_height = allocation.height - self->priv->frame_border2;

	gth_image_viewer_get_original_size (self, &original_width, &original_height);

	x_level = (double) gdk_width / original_width;
	y_level = (double) gdk_height / original_height;

	new_zoom_level = (x_level < y_level) ? x_level : y_level;
	if (new_zoom_level > 0.0) {
		self->priv->doing_zoom_fit = TRUE;
		gth_image_viewer_set_zoom (self, new_zoom_level);
		self->priv->doing_zoom_fit = FALSE;
	}
}


static void
zoom_to_fit_width (GthImageViewer *self)
{
	GtkAllocation allocation;
	int           gdk_width;
	int           original_width;
	double        new_zoom_level;

	gtk_widget_get_allocation (GTK_WIDGET (self), &allocation);
	gdk_width = allocation.width - self->priv->frame_border2;

	gth_image_viewer_get_original_size (self, &original_width, NULL);

	new_zoom_level = (double) gdk_width / original_width;

	if (new_zoom_level > 0.0) {
		self->priv->doing_zoom_fit = TRUE;
		gth_image_viewer_set_zoom (self, new_zoom_level);
		self->priv->doing_zoom_fit = FALSE;
	}
}


static void
gth_image_viewer_size_allocate (GtkWidget     *widget,
				GtkAllocation *allocation)
{
	GthImageViewer  *self;
	int              gdk_width;
	int              gdk_height;
	int              original_width;
	int              original_height;
	cairo_surface_t *current_image;

	self = GTH_IMAGE_VIEWER (widget);

	gtk_widget_set_allocation (widget, allocation);

	gdk_width = allocation->width - self->priv->frame_border2;
	gdk_height = allocation->height - self->priv->frame_border2;

	if ((gdk_width < 0) || (gdk_height < 0))
		return;

	current_image = gth_image_viewer_get_current_image (self);

	gth_image_viewer_get_original_size (self, &original_width, &original_height);

	/* If a fit type is active update the zoom level. */

	if (! self->priv->is_void && (current_image != NULL)) {
		switch (self->priv->fit) {
		case GTH_FIT_SIZE:
			zoom_to_fit (self);
			break;

		case GTH_FIT_SIZE_IF_LARGER:
			if ((gdk_width < original_width) || (gdk_height < original_height)) {
				zoom_to_fit (self);
		    	}
		    	else {
				self->priv->doing_zoom_fit = TRUE;
				gth_image_viewer_set_zoom (self, 1.0);
				self->priv->doing_zoom_fit = FALSE;
			}
			break;

		case GTH_FIT_WIDTH:
			zoom_to_fit_width (self);
			break;

		case GTH_FIT_WIDTH_IF_LARGER:
			if (gdk_width < original_width) {
				zoom_to_fit_width (self);
			}
			else {
				self->priv->doing_zoom_fit = TRUE;
				gth_image_viewer_set_zoom (self, 1.0);
				self->priv->doing_zoom_fit = FALSE;
			}
			break;

		default:
			break;
		}
	}

	/* Check whether the offset is still valid. */

	if (current_image != NULL) {
		int width;
		int height;

		_gth_image_viewer_get_zoomed_size (self, &width, &height);

		if (width > gdk_width)
			self->x_offset = CLAMP (self->x_offset,
						0,
						width - gdk_width);
		else
			self->x_offset = 0;

		if (height > gdk_height)
			self->y_offset = CLAMP (self->y_offset,
						0,
						height - gdk_height);
		else
			self->y_offset = 0;

		if ((width != gtk_adjustment_get_upper (self->hadj)) || (height != gtk_adjustment_get_upper (self->vadj)))
			g_signal_emit (G_OBJECT (self),
				       gth_image_viewer_signals[SIZE_CHANGED],
				       0);

		/* Change adjustment values. */

		gtk_adjustment_configure (self->hadj,
					  self->x_offset,
					  0.0,
					  width,
					  STEP_INCREMENT,
					  gdk_width / 2,
					  gdk_width);
		gtk_adjustment_configure (self->vadj,
					  self->y_offset,
					  0.0,
					  height,
					  STEP_INCREMENT,
					  gdk_height / 2,
					  gdk_height);
	}
	else {
		gtk_adjustment_configure (self->hadj,
					  0.0,
					  0.0,
					  1.0,
					  0.1,
					  1.0,
					  1.0);
		gtk_adjustment_configure (self->vadj,
					  0.0,
					  0.0,
					  1.0,
					  0.1,
					  1.0,
					  1.0);
	}

	_gth_image_viewer_update_image_area (self);

	/* FIXME
	g_signal_handlers_block_by_data (G_OBJECT (self->hadj), self);
	g_signal_handlers_block_by_data (G_OBJECT (self->vadj), self);
	gtk_adjustment_changed (self->hadj);
	gtk_adjustment_changed (self->vadj);
	g_signal_handlers_unblock_by_data (G_OBJECT (self->hadj), self);
	g_signal_handlers_unblock_by_data (G_OBJECT (self->vadj), self);
	*/

	/**/

	if (gtk_widget_get_realized (widget))
		gdk_window_move_resize (gtk_widget_get_window (widget),
					allocation->x,
					allocation->y,
					allocation->width,
					allocation->height);

	gth_image_viewer_tool_size_allocate (self->priv->tool, allocation);

	if (! self->priv->skip_size_change)
		g_signal_emit (G_OBJECT (self),
			       gth_image_viewer_signals[SIZE_CHANGED],
			       0);
	else
		self->priv->skip_size_change = FALSE;
}


static gboolean
gth_image_viewer_focus_in (GtkWidget     *widget,
			   GdkEventFocus *event)
{
	gtk_widget_queue_draw (widget);
	return TRUE;
}


static gboolean
gth_image_viewer_focus_out (GtkWidget     *widget,
			    GdkEventFocus *event)
{
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


static gboolean
change_animation_frame (gpointer data)
{
	GthImageViewer *self = data;

	if (self->priv->anim_id != 0) {
		g_source_remove (self->priv->anim_id);
		self->priv->anim_id = 0;
	}

	if (self->priv->is_void
	    || ! self->priv->is_animation
	    || (self->priv->iter == NULL))
	{
		return FALSE;
	}

	g_time_val_add (&self->priv->time, (glong) gdk_pixbuf_animation_iter_get_delay_time (self->priv->iter) * 1000);
	gdk_pixbuf_animation_iter_advance (self->priv->iter, &self->priv->time);

	self->priv->skip_zoom_change = TRUE;
	self->priv->skip_size_change = TRUE;
	gth_image_viewer_update_view (self);

	return FALSE;
}


static void
queue_animation_frame_change (GthImageViewer *self)
{
	if (! self->priv->is_void
	    && self->priv->is_animation
	    && self->priv->play_animation
	    && (self->priv->anim_id == 0)
	    && (self->priv->iter != NULL))
	{
		self->priv->anim_id = g_timeout_add (MAX (gdk_pixbuf_animation_iter_get_delay_time (self->priv->iter), MINIMUM_DELAY),
						     change_animation_frame,
						     self);
	}
}


static int
gth_image_viewer_expose (GtkWidget      *widget,
			 GdkEventExpose *event)
{
	GthImageViewer *self;
	cairo_t        *cr;

	self = GTH_IMAGE_VIEWER (widget);

	cr = gdk_cairo_create (gtk_widget_get_window (widget));
	/*cairo_set_line_width (cr, 0.5);
	cairo_set_antialias (cr, CAIRO_ANTIALIAS_NONE);*/

	gdk_cairo_region (cr, event->region);
	cairo_clip (cr);

	gth_image_viewer_tool_expose (self->priv->tool, event, cr);

#if 0
	/* Draw the focus. */

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

	queue_animation_frame_change (self);

	cairo_destroy (cr);

	return FALSE;
}


static gboolean
gth_image_viewer_button_press (GtkWidget      *widget,
			       GdkEventButton *event)
{
	GthImageViewer *self = GTH_IMAGE_VIEWER (widget);
	int             retval;

	if (! gtk_widget_has_focus (widget)) {
		gtk_widget_grab_focus (widget);
		self->priv->just_focused = TRUE;
	}

	if (self->dragging)
		return FALSE;

	if ((event->type == GDK_2BUTTON_PRESS)
	    || (event->type == GDK_3BUTTON_PRESS))
	{
		self->priv->double_click = TRUE;
		/*return FALSE;*/
	}
	else
		self->priv->double_click = FALSE;

	retval = gth_image_viewer_tool_button_press (self->priv->tool, event);

	if (self->pressed && ! self->priv->double_click) {
		self->event_x_start = self->event_x_prev = event->x;
		self->event_y_start = self->event_y_prev = event->y;
		self->drag_x = self->drag_x_start = self->drag_x_prev = event->x + self->x_offset;
		self->drag_y = self->drag_y_start = self->drag_y_prev = event->y + self->y_offset;
	}

	return retval;
}


static gboolean
gth_image_viewer_button_release (GtkWidget      *widget,
				 GdkEventButton *event)
{
	GthImageViewer *self = GTH_IMAGE_VIEWER (widget);

	if ((event->button == 1)
	    && ! self->dragging
	    && ! self->priv->double_click
	    && ! self->priv->just_focused)
	{
		g_signal_emit (G_OBJECT (self),
			       gth_image_viewer_signals[CLICKED],
			       0);
	}

	gth_image_viewer_tool_button_release (self->priv->tool, event);

	self->priv->just_focused = FALSE;
	self->pressed = FALSE;
	self->dragging = FALSE;

	return FALSE;
}


static void
scroll_to (GthImageViewer *self,
	   int            *x_offset,
	   int            *y_offset)
{
	GdkDrawable   *drawable;
	GtkAllocation  allocation;
	int            width, height;
	int            delta_x, delta_y;
	int            gdk_width, gdk_height;

	g_return_if_fail (self != NULL);

	if (gth_image_viewer_get_current_image (self) == NULL)
		return;

	_gth_image_viewer_get_zoomed_size (self, &width, &height);

	drawable = gtk_widget_get_window (GTK_WIDGET (self));
	gtk_widget_get_allocation (GTK_WIDGET (self), &allocation);
	gdk_width = allocation.width - self->priv->frame_border2;
	gdk_height = allocation.height - self->priv->frame_border2;

	if (width > gdk_width)
		*x_offset = CLAMP (*x_offset, 0, width - gdk_width);
	else
		*x_offset = self->x_offset;

	if (height > gdk_height)
		*y_offset = CLAMP (*y_offset, 0, height - gdk_height);
	else
		*y_offset = self->y_offset;

	if ((*x_offset == self->x_offset) && (*y_offset == self->y_offset))
		return;

	delta_x = *x_offset - self->x_offset;
	delta_y = *y_offset - self->y_offset;

	self->x_offset = *x_offset;
	self->y_offset = *y_offset;

	if (self->priv->painters != NULL) {
		GdkRectangle  area;

		area.x = 0;
		area.y = 0;
		area.width = allocation.width;
		area.height = allocation.height;
		gdk_window_invalidate_rect (drawable, &area, FALSE);
		gdk_window_process_updates (drawable, FALSE);

		return;
	}

	/* move without invalidating the frame */

	{
		GdkRectangle  area;
		GdkRegion    *region;

		area.x = (delta_x < 0) ? self->priv->frame_border : self->priv->frame_border + delta_x;
		area.y = (delta_y < 0) ? self->priv->frame_border : self->priv->frame_border + delta_y;
		area.width = gdk_width - abs (delta_x);
		area.height = gdk_height - abs (delta_y);
		region = gdk_region_rectangle (&area);
		gdk_window_move_region (drawable, region, -delta_x, -delta_y);

		gdk_region_destroy (region);
	}

	/* invalidate the exposed areas */

	{
		GdkRegion    *region;
		GdkRectangle  area;

		region = gdk_region_new ();

		area.x = self->priv->frame_border;
		area.y = (delta_y < 0) ? self->priv->frame_border : self->priv->frame_border + gdk_height - delta_y;
		area.width = gdk_width;
		area.height = abs (delta_y);
		gdk_region_union_with_rect (region, &area);

		area.x = (delta_x < 0) ? self->priv->frame_border : self->priv->frame_border + gdk_width - delta_x;
		area.y = self->priv->frame_border;
		area.width = abs (delta_x);
		area.height = gdk_height;
		gdk_region_union_with_rect (region, &area);

		gdk_window_invalidate_region (drawable, region, FALSE);

		gdk_region_destroy (region);
	}

	gdk_window_process_updates (drawable, TRUE);
}


static gboolean
gth_image_viewer_motion_notify (GtkWidget      *widget,
				GdkEventMotion *event)
{
	GthImageViewer *self = GTH_IMAGE_VIEWER (widget);

	if (self->pressed) {
		self->drag_x = event->x + self->x_offset;
		self->drag_y = event->y + self->y_offset;
	}

	gth_image_viewer_tool_motion_notify (self->priv->tool, event);

	if (self->pressed) {
		self->event_x_prev = event->x;
		self->event_y_prev = event->y;
		self->drag_x_prev = self->drag_x;
		self->drag_y_prev = self->drag_y;
	}

	return FALSE;
}


static gboolean
gth_image_viewer_scroll_event (GtkWidget      *widget,
			       GdkEventScroll *event)
{
	GthImageViewer *self = GTH_IMAGE_VIEWER (widget);
	GtkAdjustment  *adj;
	gdouble         new_value = 0.0;

	g_return_val_if_fail (GTH_IS_IMAGE_VIEWER (widget), FALSE);
	g_return_val_if_fail (event != NULL, FALSE);

	if (event->state & GDK_CONTROL_MASK) {
		if (event->direction == GDK_SCROLL_UP) {
			set_zoom (self,
				  get_next_zoom (self->priv->zoom_level),
				  (int) event->x,
				  (int) event->y);
			return TRUE;
		}
		if (event->direction == GDK_SCROLL_DOWN) {
			set_zoom (self,
				  get_prev_zoom (self->priv->zoom_level),
				  (int) event->x,
				  (int) event->y);
			return TRUE;
		}
	}

	if (event->direction == GDK_SCROLL_UP || event->direction == GDK_SCROLL_DOWN)
		return FALSE;

	adj = self->hadj;
	if (event->direction == GDK_SCROLL_LEFT)
		new_value = gtk_adjustment_get_value (adj) - gtk_adjustment_get_page_increment (adj) / 2;
	else if (event->direction == GDK_SCROLL_RIGHT)
		new_value = gtk_adjustment_get_value (adj) + gtk_adjustment_get_page_increment (adj) / 2;
	new_value = CLAMP (new_value, gtk_adjustment_get_lower (adj), gtk_adjustment_get_upper (adj) - gtk_adjustment_get_page_size (adj));
	gtk_adjustment_set_value (adj, new_value);

	return TRUE;
}


static void
gth_image_viewer_style_set (GtkWidget *widget,
			    GtkStyle  *previous_style)
{
	GthImageViewer *self = GTH_IMAGE_VIEWER (widget);

	GTK_WIDGET_CLASS (parent_class)->style_set (widget, previous_style);

	if (self->priv->transp_type == GTH_TRANSP_TYPE_NONE) {
		GtkStyle *style;
		GdkColor  color;
		guint     base_color;

		style = gtk_widget_get_style (GTK_WIDGET (self));
		color = style->bg[GTK_STATE_NORMAL];
		base_color = (0xFF000000
			      | (to_255 (color.red) << 16)
			      | (to_255 (color.green) << 8)
			      | (to_255 (color.blue) << 0));

		self->priv->check_color1 = base_color;
		self->priv->check_color2 = base_color;
	}
}


static void
scroll_relative (GthImageViewer *self,
		 int             delta_x,
		 int             delta_y)
{
	gth_image_viewer_scroll_to (self,
				    self->x_offset + delta_x,
				    self->y_offset + delta_y);
}


static void
scroll_signal (GtkWidget     *widget,
	       GtkScrollType  xscroll_type,
	       GtkScrollType  yscroll_type)
{
	GthImageViewer *self = GTH_IMAGE_VIEWER (widget);
	int             xstep, ystep;

	switch (xscroll_type) {
	case GTK_SCROLL_STEP_LEFT:
		xstep = - gtk_adjustment_get_step_increment (self->hadj);
		break;
	case GTK_SCROLL_STEP_RIGHT:
		xstep = gtk_adjustment_get_step_increment (self->hadj);
		break;
	case GTK_SCROLL_PAGE_LEFT:
		xstep = - gtk_adjustment_get_page_increment (self->hadj);
		break;
	case GTK_SCROLL_PAGE_RIGHT:
		xstep = gtk_adjustment_get_page_increment (self->hadj);
		break;
	default:
		xstep = 0;
		break;
	}

	switch (yscroll_type) {
	case GTK_SCROLL_STEP_UP:
		ystep = - gtk_adjustment_get_step_increment (self->vadj);
		break;
	case GTK_SCROLL_STEP_DOWN:
		ystep = gtk_adjustment_get_step_increment (self->vadj);
		break;
	case GTK_SCROLL_PAGE_UP:
		ystep = - gtk_adjustment_get_page_increment (self->vadj);
		break;
	case GTK_SCROLL_PAGE_DOWN:
		ystep = gtk_adjustment_get_page_increment (self->vadj);
		break;
	default:
		ystep = 0;
		break;
	}

	scroll_relative (self, xstep, ystep);
}


static gboolean
hadj_value_changed (GtkAdjustment  *adj,
		    GthImageViewer *self)
{
	int x_ofs, y_ofs;

	x_ofs = (int) gtk_adjustment_get_value (adj);
	y_ofs = self->y_offset;
	scroll_to (self, &x_ofs, &y_ofs);

	return FALSE;
}


static gboolean
vadj_value_changed (GtkAdjustment  *adj,
		    GthImageViewer *self)
{
	int x_ofs, y_ofs;

	x_ofs = self->x_offset;
	y_ofs = (int) gtk_adjustment_get_value (adj);
	scroll_to (self, &x_ofs, &y_ofs);

	return FALSE;
}


static void
set_scroll_adjustments (GtkWidget     *widget,
			GtkAdjustment *hadj,
			GtkAdjustment *vadj)
{
	GthImageViewer *self;

	g_return_if_fail (widget != NULL);
	g_return_if_fail (GTH_IS_IMAGE_VIEWER (widget));

	self = GTH_IMAGE_VIEWER (widget);

	if (hadj != NULL)
		g_return_if_fail (GTK_IS_ADJUSTMENT (hadj));
	else
		hadj = GTK_ADJUSTMENT (gtk_adjustment_new (0.0, 0.0, 0.0,
							   0.0, 0.0, 0.0));

	if (vadj != NULL)
		g_return_if_fail (GTK_IS_ADJUSTMENT (vadj));
	else
		vadj = GTK_ADJUSTMENT (gtk_adjustment_new (0.0, 0.0, 0.0,
							   0.0, 0.0, 0.0));

	if ((self->hadj != NULL) && (self->hadj != hadj)) {
		g_signal_handlers_disconnect_by_data (G_OBJECT (self->hadj),
						      self);
		g_object_unref (self->hadj);

		self->hadj = NULL;
	}

	if ((self->vadj != NULL) && (self->vadj != vadj)) {
		g_signal_handlers_disconnect_by_data (G_OBJECT (self->vadj), self);
		g_object_unref (self->vadj);
		self->vadj = NULL;
	}

	if (self->hadj != hadj) {
		self->hadj = hadj;
		g_object_ref (self->hadj);
		g_object_ref_sink (self->hadj);

		g_signal_connect (G_OBJECT (self->hadj),
				  "value_changed",
				  G_CALLBACK (hadj_value_changed),
				  self);
	}

	if (self->vadj != vadj) {
		self->vadj = vadj;
		g_object_ref (self->vadj);
		g_object_ref_sink (self->vadj);

		g_signal_connect (G_OBJECT (self->vadj),
				  "value_changed",
				  G_CALLBACK (vadj_value_changed),
				  self);
	}
}


static void
gth_image_viewer_zoom_in__key_binding (GthImageViewer *self)
{
	if (self->priv->enable_zoom_with_keys)
		gth_image_viewer_zoom_in (self);
}


static void
gth_image_viewer_zoom_out__key_binding (GthImageViewer *self)
{
	if (self->priv->enable_zoom_with_keys)
		gth_image_viewer_zoom_out (self);
}

static void
gth_image_viewer_set_fit_mode__key_binding (GthImageViewer *self,
					    GthFit          fit_mode)
{
	if (self->priv->enable_zoom_with_keys)
		gth_image_viewer_set_fit_mode (self, fit_mode);
}


static void
gth_image_viewer_set_zoom__key_binding (GthImageViewer *self,
					gdouble         zoom_level)
{
	if (self->priv->enable_zoom_with_keys)
		gth_image_viewer_set_zoom (self, zoom_level);
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
	class->zoom_changed = NULL;
	class->scroll       = scroll_signal;
	class->zoom_in      = gth_image_viewer_zoom_in__key_binding;
	class->zoom_out     = gth_image_viewer_zoom_out__key_binding;
	class->set_zoom     = gth_image_viewer_set_zoom__key_binding;
	class->set_fit_mode = gth_image_viewer_set_fit_mode__key_binding;

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
halt_animation (GthImageViewer *self)
{
	g_return_if_fail (self != NULL);

	if (self->priv->anim_id != 0) {
		g_source_remove (self->priv->anim_id);
		self->priv->anim_id = 0;
	}
}


static void
gth_image_viewer_instance_init (GthImageViewer *self)
{
	gtk_widget_set_can_focus (GTK_WIDGET (self), TRUE);
	gtk_widget_set_double_buffered (GTK_WIDGET (self), TRUE);

	/* Initialize data. */

	self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, GTH_TYPE_IMAGE_VIEWER, GthImageViewerPrivate);

	self->priv->check_type = GTH_CHECK_TYPE_MIDTONE;
	self->priv->check_size = GTH_CHECK_SIZE_LARGE;
	self->priv->check_color1 = COLOR_GRAY_66;
	self->priv->check_color2 = COLOR_GRAY_99;

	self->priv->is_animation = FALSE;
	self->priv->play_animation = TRUE;
	self->priv->cursor_visible = TRUE;

	self->priv->frame_visible = TRUE;
	self->priv->frame_border = GTH_IMAGE_VIEWER_FRAME_BORDER;
	self->priv->frame_border2 = GTH_IMAGE_VIEWER_FRAME_BORDER2;

	self->priv->anim_id = 0;
	self->priv->iter = NULL;
	self->priv->last_iter = NULL;
	self->priv->iter_surface = NULL;

	self->priv->enable_zoom_with_keys = TRUE;
	self->priv->zoom_level = 1.0;
	self->priv->zoom_quality = GTH_ZOOM_QUALITY_HIGH;
	self->priv->zoom_change = GTH_ZOOM_CHANGE_KEEP_PREV;
	self->priv->fit = GTH_FIT_SIZE_IF_LARGER;
	self->priv->doing_zoom_fit = FALSE;

	self->priv->skip_zoom_change = FALSE;
	self->priv->skip_size_change = FALSE;

	self->priv->is_void = TRUE;
	self->x_offset = 0;
	self->y_offset = 0;
	self->dragging = FALSE;
	self->priv->double_click = FALSE;
	self->priv->just_focused = FALSE;

	self->priv->black_bg = FALSE;

	self->priv->cursor = NULL;
	self->priv->cursor_void = NULL;

	self->priv->reset_scrollbars = TRUE;
	self->priv->update_adjustment_values = 0;

	self->priv->tool = gth_image_dragger_new (self);

	/* Create the widget. */

	self->hadj = GTK_ADJUSTMENT (gtk_adjustment_new (0.0, 1.0, 0.0, 0.1, 1.0, 1.0));
	self->vadj = GTK_ADJUSTMENT (gtk_adjustment_new (0.0, 1.0, 0.0, 0.1, 1.0, 1.0));

	g_object_ref (self->hadj);
	g_object_ref_sink (self->hadj);
	g_object_ref (self->vadj);
	g_object_ref_sink (self->vadj);

	g_signal_connect (G_OBJECT (self->hadj),
			  "value_changed",
			  G_CALLBACK (hadj_value_changed),
			  self);
	g_signal_connect (G_OBJECT (self->vadj),
			  "value_changed",
			  G_CALLBACK (vadj_value_changed),
			  self);
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


static void
_gth_image_viewer_set_original_size (GthImageViewer *self,
				     int             original_width,
				     int             original_height)
{
	cairo_surface_t *image;

	image = gth_image_viewer_get_current_image (self);

	if (original_width > 0)
		self->priv->original_width = original_width;
	else
		self->priv->original_width = (image != NULL) ? cairo_image_surface_get_width (image) : 0;

	if (original_height > 0)
		self->priv->original_height = original_height;
	else
		self->priv->original_height = (image != NULL) ? cairo_image_surface_get_height (image) : 0;
}


static void
_gth_image_viewer_content_changed (GthImageViewer *self,
				   gboolean        better_quality)
{
	halt_animation (self);

	if (! better_quality && self->priv->reset_scrollbars) {
		self->x_offset = 0;
		self->y_offset = 0;
	}

	gth_image_viewer_tool_image_changed (self->priv->tool);

	if (better_quality)
		return;

	switch (self->priv->zoom_change) {
	case GTH_ZOOM_CHANGE_ACTUAL_SIZE:
		gth_image_viewer_set_zoom (self, 1.0);
		queue_animation_frame_change (self);
		break;

	case GTH_ZOOM_CHANGE_KEEP_PREV:
		gth_image_viewer_update_view (self);
		break;

	case GTH_ZOOM_CHANGE_FIT_SIZE:
		gth_image_viewer_set_fit_mode (self, GTH_FIT_SIZE);
		queue_animation_frame_change (self);
		break;

	case GTH_ZOOM_CHANGE_FIT_SIZE_IF_LARGER:
		gth_image_viewer_set_fit_mode (self, GTH_FIT_SIZE_IF_LARGER);
		queue_animation_frame_change (self);
		break;

	case GTH_ZOOM_CHANGE_FIT_WIDTH:
		gth_image_viewer_set_fit_mode (self, GTH_FIT_WIDTH);
		queue_animation_frame_change (self);
		break;

	case GTH_ZOOM_CHANGE_FIT_WIDTH_IF_LARGER:
		gth_image_viewer_set_fit_mode (self, GTH_FIT_WIDTH_IF_LARGER);
		queue_animation_frame_change (self);
		break;
	}
}


static void
_set_animation (GthImageViewer     *self,
		GdkPixbufAnimation *animation,
		int                 original_width,
		int                 original_height,
		gboolean            better_quality)
{
	g_return_if_fail (self != NULL);

	_cairo_clear_surface (&self->priv->surface);

	_g_object_unref (self->priv->animation);
	self->priv->animation = _g_object_ref (animation);
	self->priv->is_void = (self->priv->animation == NULL);
	self->priv->is_animation = (self->priv->animation != NULL) ? ! gdk_pixbuf_animation_is_static_image (self->priv->animation) : FALSE;
	if (self->priv->animation != NULL) {
		g_get_current_time (&self->priv->time);
		_g_object_unref (self->priv->iter);
		self->priv->iter = gdk_pixbuf_animation_get_iter (self->priv->animation, &self->priv->time);
	}
	_gth_image_viewer_set_original_size (self, original_width, original_height);

	_gth_image_viewer_content_changed (self, better_quality);
}


void
gth_image_viewer_set_animation (GthImageViewer     *self,
				GdkPixbufAnimation *animation,
				int                 original_width,
				int                 original_height)
{
	_set_animation (self, animation, original_width, original_height, FALSE);
}


void
gth_image_viewer_set_better_quality (GthImageViewer     *self,
				     GdkPixbufAnimation *animation,
				     int                 original_width,
				     int                 original_height)
{
	_set_animation (self, animation, original_width, original_height, TRUE);
}


void
gth_image_viewer_set_pixbuf (GthImageViewer *self,
			     GdkPixbuf      *pixbuf,
			     int             original_width,
			     int             original_height)
{
	cairo_surface_t *surface;

	g_return_if_fail (self != NULL);

	surface = _cairo_image_surface_create_from_pixbuf (pixbuf);
	gth_image_viewer_set_surface (self, surface, original_width, original_height);

	cairo_surface_destroy (surface);
}


void
gth_image_viewer_set_surface (GthImageViewer  *self,
			      cairo_surface_t *surface,
			      int              original_width,
			      int              original_height)
{
	g_return_if_fail (self != NULL);

	_g_clear_object (&self->priv->animation);
	_g_clear_object (&self->priv->iter);

	cairo_surface_destroy (self->priv->surface);
	self->priv->surface = cairo_surface_reference (surface);
	self->priv->is_void = (self->priv->surface == NULL);
	self->priv->is_animation = FALSE;
	_gth_image_viewer_set_original_size (self, original_width, original_height);

	_gth_image_viewer_content_changed (self, FALSE);
}


void
gth_image_viewer_set_void (GthImageViewer *self)
{
	g_return_if_fail (self != NULL);

	_cairo_clear_surface (&self->priv->surface);
	_g_clear_object (&self->priv->animation);
	_g_clear_object (&self->priv->iter);

	self->priv->is_void = TRUE;
	self->priv->is_animation = FALSE;

	_gth_image_viewer_content_changed (self, FALSE);
}


gboolean
gth_image_viewer_is_void (GthImageViewer *self)
{
	return self->priv->is_void;
}


void
gth_image_viewer_update_view (GthImageViewer *self)
{
	g_return_if_fail (self != NULL);

	if (self->priv->fit == GTH_FIT_NONE)
		gth_image_viewer_set_zoom (self, self->priv->zoom_level);
	else
		gth_image_viewer_set_fit_mode (self, self->priv->fit);
}


void
gth_image_viewer_add_painter (GthImageViewer          *self,
			      GthImageViewerPaintFunc  func,
			      gpointer                 user_data)
{
	GList       *link;
	GList       *scan;
	PainterData *painter_data;

	g_return_if_fail (self != NULL);

	link = NULL;
	for (scan = self->priv->painters; scan; scan = scan->next) {
		PainterData *painter_data = scan->data;
		if ((painter_data->func == func) && (painter_data->user_data == user_data)) {
			link = scan;
			break;
		}
	}

	if (link != NULL)
		return;

	painter_data = g_new0 (PainterData, 1);
	painter_data->func = func;
	painter_data->user_data = user_data;
	self->priv->painters = g_list_append (self->priv->painters, painter_data);
}


void
gth_image_viewer_remove_painter (GthImageViewer          *self,
				 GthImageViewerPaintFunc  func,
				 gpointer                 user_data)
{
	GList *link;
	GList *scan;

	link = NULL;
	for (scan = self->priv->painters; scan; scan = scan->next) {
		PainterData *painter_data = scan->data;
		if ((painter_data->func == func) && (painter_data->user_data == user_data)) {
			link = scan;
			break;
		}
	}

	if (link == NULL)
		return;

	self->priv->painters = g_list_remove_link (self->priv->painters, link);
	painter_data_free (link->data);
	g_list_free (link);
}


int
gth_image_viewer_get_image_width (GthImageViewer *self)
{
	cairo_surface_t *image;

	g_return_val_if_fail (self != NULL, 0);

	image = gth_image_viewer_get_current_image (self);
	if (image != NULL)
		return cairo_image_surface_get_width (image);

	return 0;
}


int
gth_image_viewer_get_image_height (GthImageViewer *self)
{
	cairo_surface_t *image;

	g_return_val_if_fail (self != NULL, 0);

	image = gth_image_viewer_get_current_image (self);
	if (image != NULL)
		return cairo_image_surface_get_height (image);

	return 0;
}


void
gth_image_viewer_get_original_size (GthImageViewer *self,
				    int            *width,
				    int            *height)
{
	g_return_if_fail (self != NULL);

	if (width != NULL)
		*width = self->priv->original_width;
	if (height != NULL)
		*height = self->priv->original_height;
}


gboolean
gth_image_viewer_get_has_alpha (GthImageViewer *self)
{
	GdkPixbuf *pixbuf;

	g_return_val_if_fail (self != NULL, FALSE);

	pixbuf = gth_image_viewer_get_current_pixbuf (self);
	if (pixbuf != NULL)
		return gdk_pixbuf_get_has_alpha (pixbuf);

	return FALSE;
}


GdkPixbuf *
gth_image_viewer_get_current_pixbuf (GthImageViewer *self)
{
	g_return_val_if_fail (self != NULL, NULL);

	if (self->priv->is_void)
		return NULL;

	if (self->priv->surface != NULL)
		return _gdk_pixbuf_new_from_cairo_surface (self->priv->surface);

	if (self->priv->iter != NULL)
		return gdk_pixbuf_animation_iter_get_pixbuf (self->priv->iter);

	return NULL;
}


cairo_surface_t *
gth_image_viewer_get_current_image (GthImageViewer *self)
{
	g_return_val_if_fail (self != NULL, NULL);

	if (self->priv->is_void)
		return NULL;

	if (self->priv->surface != NULL)
		return self->priv->surface;

	if (self->priv->iter != NULL) {
		if ((self->priv->iter_surface == NULL) || (self->priv->iter != self->priv->last_iter)) {
			self->priv->last_iter = self->priv->iter;
			_cairo_clear_surface (&self->priv->iter_surface);
			self->priv->iter_surface = _cairo_image_surface_create_from_pixbuf (gdk_pixbuf_animation_iter_get_pixbuf (self->priv->iter));
		}
		return self->priv->iter_surface;
	}

	return NULL;
}


void
gth_image_viewer_start_animation (GthImageViewer *self)
{
	g_return_if_fail (self != NULL);

	self->priv->play_animation = TRUE;
	gth_image_viewer_update_view (self);
}


void
gth_image_viewer_stop_animation (GthImageViewer *self)
{
	g_return_if_fail (self != NULL);

	self->priv->play_animation = FALSE;
	halt_animation (self);
}


void
gth_image_viewer_step_animation (GthImageViewer *self)
{
	g_return_if_fail (self != NULL);

	if (! self->priv->is_animation)
		return;
	if (self->priv->play_animation)
		return;

	change_animation_frame (self);
}


gboolean
gth_image_viewer_is_animation (GthImageViewer *self)
{
	g_return_val_if_fail (self != NULL, FALSE);

	return self->priv->is_animation;
}


gboolean
gth_image_viewer_is_playing_animation (GthImageViewer *self)
{
	g_return_val_if_fail (self != NULL, FALSE);

	return self->priv->is_animation && self->priv->play_animation;
}


void
gth_image_viewer_set_zoom (GthImageViewer *self,
			   gdouble         zoom_level)
{
	GtkAllocation allocation;

	gtk_widget_get_allocation (GTK_WIDGET (self), &allocation);
	set_zoom (self,
		  zoom_level,
		  (allocation.width - self->priv->frame_border2) / 2,
		  (allocation.height - self->priv->frame_border2) / 2);
}


gdouble
gth_image_viewer_get_zoom (GthImageViewer *self)
{
	return self->priv->zoom_level;
}


void
gth_image_viewer_set_zoom_quality (GthImageViewer *self,
				   GthZoomQuality  quality)
{
	self->priv->zoom_quality = quality;
}


GthZoomQuality
gth_image_viewer_get_zoom_quality (GthImageViewer *self)
{
	return self->priv->zoom_quality;
}


void
gth_image_viewer_set_zoom_change (GthImageViewer *self,
				  GthZoomChange   zoom_change)
{
	self->priv->zoom_change = zoom_change;
}


GthZoomChange
gth_image_viewer_get_zoom_change (GthImageViewer *self)
{
	return self->priv->zoom_change;
}


void
gth_image_viewer_zoom_in (GthImageViewer *self)
{
	if (gth_image_viewer_get_current_image (self) == NULL)
		return;
	gth_image_viewer_set_zoom (self, get_next_zoom (self->priv->zoom_level));
}


void
gth_image_viewer_zoom_out (GthImageViewer *self)
{
	if (gth_image_viewer_get_current_image (self) == NULL)
		return;
	gth_image_viewer_set_zoom (self, get_prev_zoom (self->priv->zoom_level));
}


void
gth_image_viewer_set_fit_mode (GthImageViewer *self,
			       GthFit          fit_mode)
{
	self->priv->fit = fit_mode;
	if (self->priv->is_void)
		return;
	gtk_widget_queue_resize (GTK_WIDGET (self));
}


GthFit
gth_image_viewer_get_fit_mode (GthImageViewer *self)
{
	return self->priv->fit;
}


void
gth_image_viewer_enable_zoom_with_keys (GthImageViewer *self,
					gboolean        value)
{
	self->priv->enable_zoom_with_keys = value;
}


void
gth_image_viewer_set_transp_type (GthImageViewer *self,
				  GthTranspType   transp_type)
{
	GtkStyle *style;
	GdkColor  color;
	guint     base_color;

	g_return_if_fail (self != NULL);

	self->priv->transp_type = transp_type;

	style = gtk_widget_get_style (GTK_WIDGET (self));
	color = style->bg[GTK_STATE_NORMAL];
	base_color = (0xFF000000
		      | (to_255 (color.red) << 16)
		      | (to_255 (color.green) << 8)
		      | (to_255 (color.blue) << 0));

	switch (transp_type) {
	case GTH_TRANSP_TYPE_BLACK:
		self->priv->check_color1 = COLOR_GRAY_00;
		self->priv->check_color2 = COLOR_GRAY_00;
		break;

	case GTH_TRANSP_TYPE_NONE:
		self->priv->check_color1 = base_color;
		self->priv->check_color2 = base_color;
		break;

	case GTH_TRANSP_TYPE_WHITE:
		self->priv->check_color1 = COLOR_GRAY_FF;
		self->priv->check_color2 = COLOR_GRAY_FF;
		break;

	case GTH_TRANSP_TYPE_CHECKED:
		switch (self->priv->check_type) {
		case GTH_CHECK_TYPE_DARK:
			self->priv->check_color1 = COLOR_GRAY_00;
			self->priv->check_color2 = COLOR_GRAY_33;
			break;

		case GTH_CHECK_TYPE_MIDTONE:
			self->priv->check_color1 = COLOR_GRAY_66;
			self->priv->check_color2 = COLOR_GRAY_99;
			break;

		case GTH_CHECK_TYPE_LIGHT:
			self->priv->check_color1 = COLOR_GRAY_CC;
			self->priv->check_color2 = COLOR_GRAY_FF;
			break;
		}
		break;
	}
}


GthTranspType
gth_image_viewer_get_transp_type (GthImageViewer *self)
{
	return self->priv->transp_type;
}


void
gth_image_viewer_set_check_type (GthImageViewer *self,
				 GthCheckType    check_type)
{
	self->priv->check_type = check_type;
}


GthCheckType
gth_image_viewer_get_check_type (GthImageViewer *self)
{
	return self->priv->check_type;
}


void
gth_image_viewer_set_check_size (GthImageViewer *self,
				 GthCheckSize    check_size)
{
	self->priv->check_size = check_size;
}


GthCheckSize
gth_image_viewer_get_check_size (GthImageViewer *self)
{
	return self->priv->check_size;
}


void
gth_image_viewer_clicked (GthImageViewer *self)
{
	g_signal_emit (G_OBJECT (self), gth_image_viewer_signals[CLICKED], 0);
}


void
gth_image_viewer_set_size_request (GthImageViewer *self,
				   int             width,
				   int             height)
{
	GtkRequisition requisition;

	requisition.width = width;
	requisition.height = height;
	gtk_widget_size_request (GTK_WIDGET (self), &requisition);

	gtk_widget_queue_resize (GTK_WIDGET (self));
}


void
gth_image_viewer_set_black_background (GthImageViewer *self,
				       gboolean        set_black)
{
	self->priv->black_bg = set_black;
	if (set_black)
		gth_image_viewer_hide_frame (self);
	else
		gth_image_viewer_show_frame (self);
	gtk_widget_queue_draw (GTK_WIDGET (self));
}


gboolean
gth_image_viewer_is_black_background (GthImageViewer *self)
{
	return self->priv->black_bg;
}


void
gth_image_viewer_get_adjustments (GthImageViewer  *self,
				  GtkAdjustment  **hadj,
				  GtkAdjustment  **vadj)
{
	if (hadj != NULL)
		*hadj = self->hadj;
	if (vadj != NULL)
		*vadj = self->vadj;
}


void
gth_image_viewer_set_tool (GthImageViewer     *self,
			   GthImageViewerTool *tool)
{
	_g_object_unref (self->priv->tool);
	if (tool == NULL)
		self->priv->tool = gth_image_dragger_new (self);
	else
		self->priv->tool = g_object_ref (tool);
	if (gtk_widget_get_realized (GTK_WIDGET (self)))
		gth_image_viewer_tool_realize (self->priv->tool);
	gth_image_viewer_tool_image_changed (self->priv->tool);
	gtk_widget_queue_resize (GTK_WIDGET (self));
	gtk_widget_queue_draw (GTK_WIDGET (self));
}


static gboolean
update_adjustment_values_cb (gpointer user_data)
{
	GthImageViewer *self = user_data;

	g_source_remove (self->priv->update_adjustment_values);
	self->priv->update_adjustment_values = 0;

	g_signal_handlers_block_by_data (G_OBJECT (self->hadj), self);
	g_signal_handlers_block_by_data (G_OBJECT (self->vadj), self);
	gtk_adjustment_set_value (self->hadj, self->x_offset);
	gtk_adjustment_set_value (self->vadj, self->y_offset);
	g_signal_handlers_unblock_by_data (G_OBJECT (self->hadj), self);
	g_signal_handlers_unblock_by_data (G_OBJECT (self->vadj), self);

	return FALSE;
}


void
gth_image_viewer_scroll_to (GthImageViewer *self,
			    int             x_offset,
			    int             y_offset)
{
	g_return_if_fail (self != NULL);

	if (gth_image_viewer_get_current_image (self) == NULL)
		return;

	scroll_to (self, &x_offset, &y_offset);

	if (self->priv->update_adjustment_values != 0)
		g_source_remove (self->priv->update_adjustment_values);
	self->priv->update_adjustment_values = g_timeout_add (10, update_adjustment_values_cb, self);
}


void
gth_image_viewer_scroll_step_x (GthImageViewer *self,
				gboolean        increment)
{
	scroll_relative (self,
			 (increment ? 1 : -1) * gtk_adjustment_get_step_increment (self->hadj),
			 0);
}


void
gth_image_viewer_scroll_step_y (GthImageViewer *self,
				gboolean        increment)
{
	scroll_relative (self,
			 0,
			 (increment ? 1 : -1) * gtk_adjustment_get_step_increment (self->vadj));
}


void
gth_image_viewer_scroll_page_x (GthImageViewer *self,
				gboolean        increment)
{
	scroll_relative (self,
			 (increment ? 1 : -1) * gtk_adjustment_get_page_increment (self->hadj),
			 0);
}


void
gth_image_viewer_scroll_page_y (GthImageViewer *self,
				gboolean        increment)
{
	scroll_relative (self,
			 0,
			 (increment ? 1 : -1) * gtk_adjustment_get_page_increment (self->vadj));
}


void
gth_image_viewer_get_scroll_offset (GthImageViewer *self,
				    int            *x,
				    int            *y)
{
	*x = self->x_offset;
	*y = self->y_offset;
}


void
gth_image_viewer_set_reset_scrollbars (GthImageViewer *self,
				       gboolean        reset)
{
	self->priv->reset_scrollbars = reset;
}


gboolean
gth_image_viewer_get_reset_scrollbars (GthImageViewer *self)
{
	return self->priv->reset_scrollbars;
}


void
gth_image_viewer_show_cursor (GthImageViewer *self)
{
	if (self->priv->cursor_visible)
		return;

	self->priv->cursor_visible = TRUE;
	if (self->priv->cursor != NULL)
		gdk_window_set_cursor (gtk_widget_get_window (GTK_WIDGET (self)), self->priv->cursor);
}


void
gth_image_viewer_hide_cursor (GthImageViewer *self)
{
	if (! self->priv->cursor_visible)
		return;

	self->priv->cursor_visible = FALSE;
	if (self->priv->cursor_void != NULL)
		gdk_window_set_cursor (gtk_widget_get_window (GTK_WIDGET (self)), self->priv->cursor_void);
}


void
gth_image_viewer_set_cursor (GthImageViewer *self,
			     GdkCursor      *cursor)
{
	if (cursor != NULL)
		gdk_cursor_ref (cursor);

	if (self->priv->cursor != NULL) {
		gdk_cursor_unref (self->priv->cursor);
		self->priv->cursor = NULL;
	}
	if (cursor != NULL)
		self->priv->cursor = cursor;
	else
		self->priv->cursor = gdk_cursor_ref (self->priv->cursor_void);

	if (! gtk_widget_get_realized (GTK_WIDGET (self)))
		return;

	if (self->priv->cursor_visible)
		gdk_window_set_cursor (gtk_widget_get_window (GTK_WIDGET (self)), self->priv->cursor);
}


gboolean
gth_image_viewer_is_cursor_visible (GthImageViewer *self)
{
	return self->priv->cursor_visible;
}


void
gth_image_viewer_show_frame (GthImageViewer *self)
{
	self->priv->frame_visible = TRUE;
	self->priv->frame_border = GTH_IMAGE_VIEWER_FRAME_BORDER;
	self->priv->frame_border2 = GTH_IMAGE_VIEWER_FRAME_BORDER2;

	gtk_widget_queue_resize (GTK_WIDGET (self));
}


void
gth_image_viewer_hide_frame (GthImageViewer *self)
{
	self->priv->frame_visible = FALSE;
	self->priv->frame_border = 0;
	self->priv->frame_border2 = 0;

	gtk_widget_queue_resize (GTK_WIDGET (self));
}


gboolean
gth_image_viewer_is_frame_visible (GthImageViewer *self)
{
	return self->priv->frame_visible;
}


void
gth_image_viewer_paint (GthImageViewer  *self,
			cairo_t         *cr,
			cairo_surface_t *surface,
			int              src_x,
			int              src_y,
			int              dest_x,
			int              dest_y,
			int              width,
			int              height,
			cairo_filter_t   filter)
{
	int    original_width;
	double zoom_level;
	double src_dx;
	double src_dy;
	double dest_dx;
	double dest_dy;
	double dwidth;
	double dheight;

	cairo_save (cr);

	gth_image_viewer_get_original_size (self, &original_width, NULL);
	zoom_level = self->priv->zoom_level * ((double) original_width / cairo_image_surface_get_width (surface));
	src_dx = (double) src_x / zoom_level;
	src_dy = (double) src_y / zoom_level;
	dest_dx = (double) dest_x / zoom_level;
	dest_dy = (double) dest_y / zoom_level;
	dwidth = (double) width / zoom_level;
	dheight = (double) height / zoom_level;

	cairo_scale (cr, zoom_level, zoom_level);

	/* FIXME: paint the background for images with a transparency */

	/* g_print ("src: (%d, %d) -- dest: (%d, %d) [%d, %d]\n", src_x, src_y, dest_x, dest_y, width, height); */

	cairo_set_source_surface (cr, surface, dest_dx - src_dx, dest_dy - src_dy);
	cairo_pattern_set_filter (cairo_get_source (cr), filter);
	cairo_rectangle (cr, dest_dx, dest_dy, dwidth, dheight);
  	cairo_clip_preserve (cr);
  	cairo_fill (cr);

  	cairo_restore (cr);
}


void
gth_image_viewer_paint_region (GthImageViewer  *self,
			       cairo_t         *cr,
			       cairo_surface_t *surface,
			       int              src_x,
			       int              src_y,
			       GdkRectangle    *pixbuf_area,
			       GdkRegion       *region,
			       cairo_filter_t   filter)
{
	GdkRectangle *rects;
	int           n_rects;
	int           i;

	cairo_save (cr);

	gdk_cairo_rectangle (cr, pixbuf_area);
	cairo_clip (cr);

	gdk_region_get_rectangles (region, &rects, &n_rects);
	for (i = 0; i < n_rects; i++) {
		GdkRectangle paint_area;

		if (gdk_rectangle_intersect (pixbuf_area, &rects[i], &paint_area))
			gth_image_viewer_paint (self,
						cr,
						surface,
						src_x + paint_area.x,
						src_y + paint_area.y,
						paint_area.x,
						paint_area.y,
						paint_area.width,
						paint_area.height,
						filter);
	}

	cairo_restore (cr);

	g_free (rects);
}


void
gth_image_viewer_paint_background (GthImageViewer *self,
				   cairo_t        *cr)
{
	GtkAllocation   allocation;
	int             gdk_width;
	int             gdk_height;
	GtkStyle       *style;

	gtk_widget_get_allocation (GTK_WIDGET (self), &allocation);
	gdk_width = allocation.width - self->priv->frame_border2;
	gdk_height = allocation.height - self->priv->frame_border2;

	style = gtk_widget_get_style (GTK_WIDGET (self));

	if ((self->image_area.x > self->priv->frame_border)
	    || (self->image_area.y > self->priv->frame_border)
	    || (self->image_area.width < gdk_width)
	    || (self->image_area.height < gdk_height))
	{
		int rx, ry, rw, rh;

		if (self->priv->black_bg)
			cairo_set_source_rgb (cr, 0.0, 0.0, 0.0);
		else
			gdk_cairo_set_source_color (cr, &style->bg[GTK_STATE_NORMAL]);

		if (gth_image_viewer_get_current_image (self) == NULL) {
			cairo_rectangle (cr,
					 0,
					 0,
					 allocation.width,
					 allocation.height);
		}
		else {
			/* If an image is present draw in four phases to avoid
			 * flickering. */

			/* Top rectangle. */

			rx = 0;
			ry = 0;
			rw = allocation.width;
			rh = self->image_area.y;
			if ((rw > 0) && (rh > 0))
				cairo_rectangle (cr, rx, ry, rw, rh);

			/* Bottom rectangle. */

			rx = 0;
			ry = self->image_area.y + self->image_area.height;
			rw = allocation.width;
			rh = allocation.height - self->image_area.y - self->image_area.height;
			if ((rw > 0) && (rh > 0))
				cairo_rectangle (cr, rx, ry, rw, rh);

			/* Left rectangle. */

			rx = 0;
			ry = self->image_area.y - 1;
			rw = self->image_area.x;
			rh = self->image_area.height + 2;
			if ((rw > 0) && (rh > 0))
				cairo_rectangle (cr, rx, ry, rw, rh);

			/* Right rectangle. */

			rx = self->image_area.x + self->image_area.width;
			ry = self->image_area.y - 1;
			rw = allocation.width - self->image_area.x - self->image_area.width;
			rh = self->image_area.height + 2;
			if ((rw > 0) && (rh > 0))
				cairo_rectangle (cr, rx, ry, rw, rh);
		}

		cairo_fill (cr);
	}

	/* Draw the frame. */

	if ((self->priv->frame_border > 0)
	    && (gth_image_viewer_get_current_image (self) != NULL))
	{

		/* bottom and right side */

		if (self->priv->black_bg)
			cairo_set_source_rgb (cr, 0.0, 0.0, 0.0);
		else
			gdk_cairo_set_source_color (cr, &style->light[GTK_STATE_NORMAL]);

		cairo_move_to (cr,
			       self->image_area.x + self->image_area.width + 0.5,
			       self->image_area.y - 1 + 0.5);
		cairo_line_to (cr,
			       self->image_area.x + self->image_area.width + 0.5,
			       self->image_area.y + self->image_area.height + 0.5);
		cairo_line_to (cr,
			       self->image_area.x - 1 + 0.5,
			       self->image_area.y + self->image_area.height + 0.5);
		cairo_stroke (cr);

		/* top and left side */

		if (! self->priv->black_bg)
			gdk_cairo_set_source_color (cr, &style->dark[GTK_STATE_NORMAL]);

		cairo_move_to (cr,
			       self->image_area.x - 1 + 0.5,
			       self->image_area.y + self->image_area.height + 0.5);
		cairo_line_to (cr,
			       self->image_area.x - 1 + 0.5,
			       self->image_area.y - 1 + 0.5);
		cairo_line_to (cr,
			       self->image_area.x + self->image_area.width + 0.5,
			       self->image_area.y - 1 + 0.5);
		cairo_stroke (cr);
	}
}


void
gth_image_viewer_apply_painters (GthImageViewer *self,
				 GdkEventExpose *event,
				 cairo_t        *cr)
{
	GList *scan;

	for (scan = self->priv->painters; scan; scan = scan->next) {
		PainterData *painter_data = scan->data;

		cairo_save (cr);
		painter_data->func (self, event, cr, painter_data->user_data);
		cairo_restore (cr);
	}
}


void
gth_image_viewer_crop_area (GthImageViewer *self,
			    GdkRectangle   *area)
{
	GtkWidget     *widget = GTK_WIDGET (self);
	GtkAllocation  allocation;

	gtk_widget_get_allocation (widget, &allocation);
	area->width = MIN (area->width, allocation.width - self->priv->frame_border2);
	area->width = MIN (area->height, allocation.height - self->priv->frame_border2);
}
