/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2013 Free Software Foundation, Inc.
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
#include <math.h>
#include <gdk/gdkkeysyms.h>
#include "cairo-scale.h"
#include "gth-image-overview.h"
#include "gth-image-viewer.h"


#define VISIBLE_AREA_BORDER 2.0
#define MAX_SIZE 180


/* Properties */
enum {
        PROP_0,
        PROP_VIEWER
};


struct _GthImageOverviewPrivate {
	GthImageViewer		*viewer;
	cairo_surface_t		*image;
	int                      original_width;
	int                      original_height;
	int                      max_width;
	int                      max_height;
	int                      overview_width;
	int                      overview_height;
	cairo_rectangle_int_t	 visible_area;
	double			 zoom_factor;
	double                   quality_zoom;
	gulong			 zoom_changed_id;
	gboolean                 scrolling_active;
};


G_DEFINE_TYPE_WITH_CODE (GthImageOverview, gth_image_overview, GTK_TYPE_WIDGET,
			 G_ADD_PRIVATE (GthImageOverview))


static void
gth_image_overview_finalize (GObject *object)
{
	GthImageOverview *self;

	g_return_if_fail (GTH_IS_IMAGE_OVERVIEW (object));

	self = GTH_IMAGE_OVERVIEW (object);

	if (self->priv->zoom_changed_id > 0) {
		g_signal_handler_disconnect (self->priv->viewer, self->priv->zoom_changed_id);
		self->priv->zoom_changed_id = 0;
	}
	cairo_surface_destroy (self->priv->image);

	G_OBJECT_CLASS (gth_image_overview_parent_class)->finalize (object);
}


static void
_gth_image_overview_update_visible_area (GthImageOverview *self)
{
	cairo_surface_t  *image;
	int               zoomed_width;
	int               zoomed_height;
	GtkAllocation     allocation;
	int               scroll_offset_x;
	int               scroll_offset_y;

	image = gth_image_viewer_get_current_image (self->priv->viewer);
	if (image == NULL) {
		cairo_surface_destroy (self->priv->image);
		self->priv->image = NULL;
		self->priv->max_width = 0;
		self->priv->max_height = 0;
		self->priv->overview_width = 0;
		self->priv->overview_height = 0;
		self->priv->visible_area.width = 0;
		self->priv->visible_area.height = 0;
		gtk_widget_queue_draw (GTK_WIDGET (self));

		return;
	}

	gth_image_viewer_get_original_size (self->priv->viewer, &self->priv->original_width, &self->priv->original_height);
	zoomed_width = self->priv->original_width * gth_image_viewer_get_zoom (self->priv->viewer);
	zoomed_height = self->priv->original_height * gth_image_viewer_get_zoom (self->priv->viewer);
	self->priv->max_width = MIN (zoomed_width, MAX_SIZE);
	self->priv->max_height = MIN (zoomed_width, MAX_SIZE);
	self->priv->zoom_factor = MIN ((double) (self->priv->max_width) / zoomed_width,
				       (double) (self->priv->max_height) / zoomed_height);
	self->priv->quality_zoom = (double) self->priv->original_width / cairo_image_surface_get_width (image);
	self->priv->overview_width = MAX ((int) floor (self->priv->zoom_factor * zoomed_width + 0.5), 1);
	self->priv->overview_height = MAX ((int) floor (self->priv->zoom_factor * zoomed_height + 0.5), 1);

	cairo_surface_destroy (self->priv->image);
	self->priv->image = _cairo_image_surface_scale_bilinear (image,
								 self->priv->overview_width,
								 self->priv->overview_height);

	/* visible area size */

	gtk_widget_get_allocation (GTK_WIDGET (self->priv->viewer), &allocation);
	self->priv->visible_area.width = (allocation.width - (gth_image_viewer_get_frame_border (self->priv->viewer) * 2)) * self->priv->zoom_factor;
	self->priv->visible_area.width = MIN (self->priv->visible_area.width, self->priv->max_width);
	self->priv->visible_area.height = (allocation.height - (gth_image_viewer_get_frame_border (self->priv->viewer) * 2)) * self->priv->zoom_factor;
	self->priv->visible_area.height = MIN (self->priv->visible_area.height, self->priv->max_height);

	/* visible area position */

	gth_image_viewer_get_scroll_offset (self->priv->viewer, &scroll_offset_x, &scroll_offset_y);
	self->priv->visible_area.x = scroll_offset_x * (self->priv->zoom_factor * self->priv->quality_zoom);
	self->priv->visible_area.y = scroll_offset_y * (self->priv->zoom_factor * self->priv->quality_zoom);

	gtk_widget_queue_draw (GTK_WIDGET (self));
}


static void
viewer_zoom_changed_cb (GthImageViewer *viewer,
			gpointer        user_data)
{
	_gth_image_overview_update_visible_area (GTH_IMAGE_OVERVIEW (user_data));
}


static void
_gth_image_overview_set_viewer (GthImageOverview *self,
			        GtkWidget         *viewer)
{
	if (self->priv->viewer == (GthImageViewer *) viewer)
		return;

	if (self->priv->zoom_changed_id > 0) {
		g_signal_handler_disconnect (self->priv->viewer, self->priv->zoom_changed_id);
		self->priv->zoom_changed_id = 0;
	}
	self->priv->viewer = NULL;

	if (viewer == NULL)
		return;

	self->priv->viewer = (GthImageViewer *) viewer;
	_gth_image_overview_update_visible_area (self);
	self->priv->zoom_changed_id = g_signal_connect (self->priv->viewer,
							"zoom-changed",
							G_CALLBACK (viewer_zoom_changed_cb),
							self);

	g_object_notify (G_OBJECT (self), "viewer");
}


static void
gth_image_overview_set_property (GObject      *object,
				 guint         property_id,
				 const GValue *value,
				 GParamSpec   *pspec)
{
	GthImageOverview *self = GTH_IMAGE_OVERVIEW (object);

	switch (property_id) {
	case PROP_VIEWER:
		_gth_image_overview_set_viewer (self, g_value_get_object (value));
		break;

	default:
		break;
	}
}


static void
gth_image_overview_get_property (GObject    *object,
				 guint       property_id,
				 GValue     *value,
				 GParamSpec *pspec)
{
	GthImageOverview *self = GTH_IMAGE_OVERVIEW (object);

	switch (property_id) {
	case PROP_VIEWER:
		g_value_set_object (value, self->priv->viewer);
		break;

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
		break;
	}
}


static void
gth_image_overview_realize (GtkWidget *widget)
{
	GtkAllocation  allocation;
	GdkWindowAttr  attributes;
	int            attributes_mask;
	GdkWindow     *window;

	g_return_if_fail (GTH_IS_IMAGE_OVERVIEW (widget));

	gtk_widget_set_realized (widget, TRUE);
	gtk_widget_get_allocation (widget, &allocation);

	attributes.window_type = GDK_WINDOW_CHILD;
	attributes.x           = allocation.x;
	attributes.y           = allocation.y;
	attributes.width       = allocation.width;
	attributes.height      = allocation.height;
	attributes.wclass      = GDK_INPUT_OUTPUT;
	attributes.visual      = gtk_widget_get_visual (widget);
	attributes.event_mask  = (gtk_widget_get_events (widget)
				  | GDK_EXPOSURE_MASK
				  | GDK_BUTTON_PRESS_MASK
				  | GDK_BUTTON_RELEASE_MASK
				  | GDK_POINTER_MOTION_MASK
				  | GDK_POINTER_MOTION_HINT_MASK
				  | GDK_BUTTON_MOTION_MASK);
	attributes_mask = GDK_WA_X | GDK_WA_Y | GDK_WA_VISUAL;
	window = gdk_window_new (gtk_widget_get_parent_window (widget), &attributes, attributes_mask);
	gtk_widget_register_window (widget, window);
	gtk_widget_set_window (widget, window);
	gtk_style_context_set_background (gtk_widget_get_style_context (widget), window);
}


static void
gth_image_overview_size_allocate (GtkWidget     *widget,
				  GtkAllocation *allocation)
{
	gtk_widget_set_allocation (widget, allocation);
	if (gtk_widget_get_realized (widget))
		gdk_window_move_resize (gtk_widget_get_window (widget),
					allocation->x,
					allocation->y,
					allocation->width,
					allocation->height);
	_gth_image_overview_update_visible_area (GTH_IMAGE_OVERVIEW (widget));
}


static gboolean
gth_image_overview_draw (GtkWidget *widget,
			 cairo_t   *cr)
{
	GthImageOverview *self;

	self = GTH_IMAGE_OVERVIEW (widget);

	if (self->priv->image == NULL)
		return FALSE;

	cairo_save (cr);
	cairo_set_antialias (cr, CAIRO_ANTIALIAS_NONE);
	cairo_set_source_surface (cr, self->priv->image, 0, 0);
	cairo_pattern_set_filter (cairo_get_source (cr), CAIRO_FILTER_FAST);
	cairo_rectangle (cr, 0, 0, self->priv->overview_width, self->priv->overview_height);
  	cairo_fill (cr);
  	cairo_restore (cr);

  	cairo_save (cr);
	cairo_set_source_rgba (cr, 0.0, 0.0, 0.0, 0.5);
	cairo_rectangle (cr, 0, 0, self->priv->overview_width, self->priv->overview_height);
	cairo_fill (cr);
	cairo_restore (cr);

	if ((self->priv->visible_area.width < self->priv->overview_width)
	    || (self->priv->visible_area.height < self->priv->overview_height))
	{
		cairo_save (cr);
		cairo_rectangle (cr,
				 self->priv->visible_area.x,
				 self->priv->visible_area.y,
				 self->priv->visible_area.width,
				 self->priv->visible_area.height);
		cairo_clip (cr);
		cairo_set_source_surface (cr, self->priv->image, 0, 0);
		cairo_pattern_set_filter (cairo_get_source (cr), CAIRO_FILTER_FAST);
		cairo_rectangle (cr, 0, 0, self->priv->overview_width, self->priv->overview_height);
	  	cairo_fill (cr);
	  	cairo_restore (cr);

		cairo_save (cr);
		cairo_set_line_width (cr, VISIBLE_AREA_BORDER);
		cairo_set_source_rgb (cr, 1.0, 1.0, 1.0);
		cairo_rectangle (cr,
				 self->priv->visible_area.x + 1.0,
				 self->priv->visible_area.y + 1.0,
				 self->priv->visible_area.width - VISIBLE_AREA_BORDER,
				 self->priv->visible_area.height - VISIBLE_AREA_BORDER);
		cairo_stroke (cr);
		cairo_restore (cr);
	}

	return TRUE;
}


static gboolean
gth_image_overview_button_press_event (GtkWidget       *widget,
				       GdkEventButton  *event)
{
	gth_image_overview_activate_scrolling (GTH_IMAGE_OVERVIEW (widget), TRUE, event);
        return FALSE;
}


static gboolean
gth_image_overview_button_release_event (GtkWidget      *widget,
					 GdkEventButton *event)
{
	gth_image_overview_activate_scrolling (GTH_IMAGE_OVERVIEW (widget), FALSE, event);
	return FALSE;
}


/* -- gth_image_overview_motion_notify_event -- */


static void
get_visible_area_origin_as_double (GthImageOverview *self,
				   int               mx,
				   int               my,
				   double           *x,
				   double           *y)
{
	*x = MIN (mx, self->priv->max_width);
	*y = MIN (my, self->priv->max_height);

	if (*x - self->priv->visible_area.width / 2.0 < 0.0)
		*x = self->priv->visible_area.width / 2.0;

	if (*y - self->priv->visible_area.height / 2.0 < 0.0)
		*y = self->priv->visible_area.height / 2.0;

	if (*x + self->priv->visible_area.width / 2.0 > self->priv->overview_width - 0)
		*x = self->priv->overview_width - 0 - self->priv->visible_area.width / 2.0;

	if (*y + self->priv->visible_area.height / 2.0 > self->priv->overview_height - 0)
		*y = self->priv->overview_height - 0 - self->priv->visible_area.height / 2.0;

	*x = *x - self->priv->visible_area.width / 2.0;
	*y = *y - self->priv->visible_area.height / 2.0;
}


static gboolean
gth_image_overview_motion_notify_event (GtkWidget      *widget,
					GdkEventMotion *event)
{
	GthImageOverview *self;
	int               mx, my;
	double            x, y;

	self = GTH_IMAGE_OVERVIEW (widget);

	if (! self->priv->scrolling_active)
		return FALSE;

	gdk_window_get_device_position (gtk_widget_get_window (widget),
					gdk_event_get_device ((GdkEvent *) event),
					&mx,
					&my,
					NULL);

	get_visible_area_origin_as_double (self, mx, my, &x, &y);
	self->priv->visible_area.x = (int) x;
	self->priv->visible_area.y = (int) y;

	mx = (int) (x / (self->priv->quality_zoom * self->priv->zoom_factor));
	my = (int) (y / (self->priv->quality_zoom * self->priv->zoom_factor));
	gth_image_viewer_set_scroll_offset (self->priv->viewer, mx, my);

	return FALSE;
}


static void
gth_image_overview_class_init (GthImageOverviewClass *klass)
{
	GObjectClass   *object_class;
	GtkWidgetClass *widget_class;

	object_class = (GObjectClass *) klass;
	object_class->finalize = gth_image_overview_finalize;
	object_class->set_property = gth_image_overview_set_property;
	object_class->get_property = gth_image_overview_get_property;

	widget_class = (GtkWidgetClass *) klass;
	widget_class->realize = gth_image_overview_realize;
	widget_class->size_allocate = gth_image_overview_size_allocate;
	widget_class->draw = gth_image_overview_draw;
	widget_class->button_press_event = gth_image_overview_button_press_event;
	widget_class->button_release_event = gth_image_overview_button_release_event;
	widget_class->motion_notify_event = gth_image_overview_motion_notify_event;

	/* properties */

	g_object_class_install_property (object_class,
					 PROP_VIEWER,
					 g_param_spec_object ("viewer",
                                                              "Viewer",
                                                              "The image viewer to use",
                                                              GTH_TYPE_IMAGE_VIEWER,
                                                              G_PARAM_READWRITE));
}


static void
gth_image_overview_init (GthImageOverview *self)
{
	self->priv = gth_image_overview_get_instance_private (self);
	self->priv->viewer = NULL;
	self->priv->image = NULL;
	self->priv->visible_area.width = 0;
	self->priv->visible_area.height = 0;
	self->priv->zoom_factor = 1.0;
	self->priv->zoom_changed_id = 0;
	self->priv->scrolling_active = FALSE;
}


GtkWidget *
gth_image_overview_new (GthImageViewer *viewer)
{
	g_return_val_if_fail (viewer != NULL, NULL);
	return (GtkWidget *) g_object_new (GTH_TYPE_IMAGE_OVERVIEW, "viewer", viewer, NULL);
}


void
gth_image_overview_get_size (GthImageOverview	*self,
			     int		*width,
			     int		*height)
{
	if (width != NULL) *width = self->priv->overview_width;
	if (height != NULL) *height = self->priv->overview_height;
}


void
gth_image_overview_get_visible_area (GthImageOverview	*self,
				     int		*x,
				     int		*y,
				     int		*width,
				     int		*height)
{
	if (x != NULL) *x = self->priv->visible_area.x;
	if (y != NULL) *y = self->priv->visible_area.y;
	if (width != NULL) *width = self->priv->visible_area.width;
	if (height != NULL) *height = self->priv->visible_area.height;
}


void
gth_image_overview_activate_scrolling (GthImageOverview	*self,
				       gboolean          active,
				       GdkEventButton   *event)
{
	g_return_if_fail (event != NULL);

	if (active && ! self->priv->scrolling_active) {
		GdkCursor *cursor;

		gtk_widget_realize (GTK_WIDGET (self));
		gtk_grab_add (GTK_WIDGET (self));

		/* capture mouse events */

		cursor = gdk_cursor_new_for_display (gtk_widget_get_display (GTK_WIDGET (self->priv->viewer)), GDK_FLEUR);
		gdk_device_grab (gdk_event_get_device ((GdkEvent *) event),
				 gtk_widget_get_window (GTK_WIDGET (self)),
				 GDK_OWNERSHIP_WINDOW,
				 FALSE,
				 GDK_ALL_EVENTS_MASK,
				 cursor,
				 event->time);
		g_object_unref (cursor);

		/* capture keyboard events. */

		gdk_keyboard_grab (gtk_widget_get_window (GTK_WIDGET (self)), FALSE, event->time);
	        gtk_widget_grab_focus (GTK_WIDGET (self));
	}
	else if (! active && self->priv->scrolling_active) {
		gdk_device_ungrab (gdk_event_get_device ((GdkEvent *) event), event->time);
		gdk_keyboard_ungrab (event->time);
	}

	self->priv->scrolling_active = active;
}
