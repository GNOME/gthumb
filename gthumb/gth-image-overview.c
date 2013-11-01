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
#include "gth-image-utils.h"
#include "gth-image-viewer.h"


#define MAX_ALLOCATION_SIZE	180
#define IMAGE_BORDER		3.0
#define VISIBLE_AREA_BORDER	2.0
#define MAX_IMAGE_SIZE		(MAX_ALLOCATION_SIZE - (IMAGE_BORDER * 2))


/* Properties */
enum {
        PROP_0,
        PROP_VIEWER
};


struct _GthImageOverviewPrivate {
	GthImageViewer		*viewer;
	cairo_surface_t		*preview;
	int                      preview_width;
	double			 preview_frame_border;
	int			 preview_image_width;
	int			 preview_image_height;
	int                      preview_height;
	cairo_rectangle_int_t	 visible_area;
	double			 zoom_factor;
	double                   quality_zoom;
	gulong			 zoom_changed_id;
	gulong			 image_changed_id;
	gulong			 vadj_vchanged_id;
	gulong			 hadj_vchanged_id;
	gulong			 vadj_changed_id;
	gulong			 hadj_changed_id;
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

	cairo_surface_destroy (self->priv->preview);

	G_OBJECT_CLASS (gth_image_overview_parent_class)->finalize (object);
}


static void
_gth_image_overview_update_preview (GthImageOverview *self)
{
	cairo_surface_t  *image;
	int               width, height;
	int               frame_border;
	double            zoom_factor;

	cairo_surface_destroy (self->priv->preview);
	self->priv->preview = NULL;

	image = gth_image_viewer_get_current_image (self->priv->viewer);
	if (image == NULL) {
		self->priv->preview_width = 0;
		self->priv->preview_height = 0;
		self->priv->visible_area.width = 0;
		self->priv->visible_area.height = 0;
		gtk_widget_queue_draw (GTK_WIDGET (self));

		return;
	}

	gth_image_viewer_get_original_size (self->priv->viewer, &width, &height);
	frame_border = gth_image_viewer_get_frame_border (self->priv->viewer);
	width += (frame_border * 2);
	height += (frame_border * 2);

	self->priv->preview_width = width;
	self->priv->preview_height = height;
	scale_keeping_ratio (&self->priv->preview_width,
			     &self->priv->preview_height,
			     MAX_IMAGE_SIZE,
			     MAX_IMAGE_SIZE,
			     FALSE);

	zoom_factor = MIN ((double) (self->priv->preview_width) / width,
			   (double) (self->priv->preview_height) / height);
	self->priv->preview_frame_border = frame_border * zoom_factor;
	self->priv->preview_image_width = self->priv->preview_width - (self->priv->preview_frame_border * 2);
	self->priv->preview_image_height = self->priv->preview_height - (self->priv->preview_frame_border * 2);

	self->priv->preview = _cairo_image_surface_scale_bilinear (image,
								   self->priv->preview_image_width,
								   self->priv->preview_image_height);
}


static void
_gth_image_overview_update_zoom_info (GthImageOverview *self)
{
	cairo_surface_t *image;
	int              width, height;
	double           zoom;
	int              frame_border;

	if (self->priv->preview == NULL)
		return;

	image = gth_image_viewer_get_current_image (self->priv->viewer);
	if (image == NULL)
		return;

	gth_image_viewer_get_original_size (self->priv->viewer, &width, &height);
	self->priv->quality_zoom = (double) width / cairo_image_surface_get_width (image);

	zoom = gth_image_viewer_get_zoom (self->priv->viewer);
	frame_border = gth_image_viewer_get_frame_border (self->priv->viewer);
	width = width * zoom + (frame_border * 2);
	height = height * zoom + (frame_border * 2);

	self->priv->zoom_factor = MIN ((double) (self->priv->preview_width) / width,
				       (double) (self->priv->preview_height) / height);
}


static void
_gth_image_overview_update_visible_area (GthImageOverview *self)
{
	/* visible area size */

	self->priv->visible_area.width = self->priv->viewer->visible_area.width * self->priv->zoom_factor;
	self->priv->visible_area.width = MIN (self->priv->visible_area.width, self->priv->preview_width);
	self->priv->visible_area.height = self->priv->viewer->visible_area.height * self->priv->zoom_factor;
	self->priv->visible_area.height = MIN (self->priv->visible_area.height, self->priv->preview_height);

	/* visible area position */

	self->priv->visible_area.x = self->priv->viewer->visible_area.x * self->priv->zoom_factor;
	self->priv->visible_area.y = self->priv->viewer->visible_area.y * self->priv->zoom_factor;

	gtk_widget_queue_draw (GTK_WIDGET (self));
}


static void
viewer_zoom_changed_cb (GthImageViewer *viewer,
			gpointer        user_data)
{
	GthImageOverview *self = GTH_IMAGE_OVERVIEW (user_data);

	_gth_image_overview_update_zoom_info (self);
	_gth_image_overview_update_visible_area (self);
}


static void
viewer_image_changed_cb (GthImageViewer *viewer,
			 gpointer        user_data)
{
	GthImageOverview *self = GTH_IMAGE_OVERVIEW (user_data);

	_gth_image_overview_update_preview (self);
	_gth_image_overview_update_zoom_info (self);
	_gth_image_overview_update_visible_area (self);

	gtk_widget_queue_resize (GTK_WIDGET (user_data));
}


static void
viewer_adj_value_changed_cb (GtkAdjustment *adjustment,
			     gpointer       user_data)
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
		if (self->priv->viewer != NULL)
			g_signal_handler_disconnect (self->priv->viewer, self->priv->zoom_changed_id);
		self->priv->zoom_changed_id = 0;
	}
	if (self->priv->image_changed_id > 0) {
		if (self->priv->viewer != NULL)
			g_signal_handler_disconnect (self->priv->viewer, self->priv->image_changed_id);
		self->priv->image_changed_id = 0;
	}
	if (self->priv->vadj_vchanged_id > 0) {
		if (self->priv->viewer != NULL)
			g_signal_handler_disconnect (self->priv->viewer->vadj, self->priv->vadj_vchanged_id);
		self->priv->vadj_vchanged_id = 0;
	}
	if (self->priv->hadj_vchanged_id > 0) {
		if (self->priv->viewer != NULL)
			g_signal_handler_disconnect (self->priv->viewer->hadj, self->priv->hadj_vchanged_id);
		self->priv->hadj_vchanged_id = 0;
	}
	if (self->priv->vadj_changed_id > 0) {
		if (self->priv->viewer != NULL)
			g_signal_handler_disconnect (self->priv->viewer->vadj, self->priv->vadj_changed_id);
		self->priv->vadj_changed_id = 0;
	}
	if (self->priv->hadj_changed_id > 0) {
		if (self->priv->viewer != NULL)
			g_signal_handler_disconnect (self->priv->viewer->hadj, self->priv->hadj_changed_id);
		self->priv->hadj_changed_id = 0;
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
	self->priv->image_changed_id = g_signal_connect (self->priv->viewer,
							 "image-changed",
							 G_CALLBACK (viewer_image_changed_cb),
							 self);
	self->priv->vadj_vchanged_id = g_signal_connect (self->priv->viewer->vadj,
						         "value-changed",
						         G_CALLBACK (viewer_adj_value_changed_cb),
						         self);
	self->priv->hadj_vchanged_id = g_signal_connect (self->priv->viewer->hadj,
						         "value-changed",
						         G_CALLBACK (viewer_adj_value_changed_cb),
						         self);
	self->priv->vadj_changed_id = g_signal_connect (self->priv->viewer->vadj,
						        "changed",
						        G_CALLBACK (viewer_adj_value_changed_cb),
						        self);
	self->priv->hadj_changed_id = g_signal_connect (self->priv->viewer->hadj,
						        "changed",
						        G_CALLBACK (viewer_adj_value_changed_cb),
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
				  | GDK_BUTTON_MOTION_MASK
				  | GDK_SCROLL_MASK);
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
	GtkAllocation     allocation;

	self = GTH_IMAGE_OVERVIEW (widget);

	if (self->priv->preview == NULL)
		return FALSE;

	gtk_widget_get_allocation (widget, &allocation);

  	cairo_save (cr);
	cairo_rectangle (cr, 0.5, 0.5, allocation.width - 1, allocation.height - 1);
	cairo_set_source_rgb (cr, 0.0, 0.0, 0.0);
	cairo_stroke_preserve (cr);
	cairo_set_source_rgb (cr, 1.0, 1.0, 1.0);
	cairo_fill (cr);
	cairo_restore (cr);

	cairo_save (cr);
	cairo_set_antialias (cr, CAIRO_ANTIALIAS_NONE);
	cairo_set_source_surface (cr, self->priv->preview,
			          IMAGE_BORDER + self->priv->preview_frame_border,
			          IMAGE_BORDER + self->priv->preview_frame_border);
	cairo_pattern_set_filter (cairo_get_source (cr), CAIRO_FILTER_FAST);
	cairo_rectangle (cr,
			 IMAGE_BORDER + self->priv->preview_frame_border,
			 IMAGE_BORDER + self->priv->preview_frame_border,
			 self->priv->preview_image_width,
			 self->priv->preview_image_height);
  	cairo_fill_preserve (cr);
  	cairo_set_line_width (cr, 1.0);
	cairo_set_source_rgb (cr, 0.0, 0.0, 0.0);
	cairo_stroke (cr);
  	cairo_restore (cr);

	if ((self->priv->visible_area.width < self->priv->preview_width)
	    || (self->priv->visible_area.height < self->priv->preview_height))
	{
		cairo_save (cr);
		cairo_set_line_width (cr, VISIBLE_AREA_BORDER);
		cairo_set_source_rgb (cr, 1.0, 0.0, 0.0);
		cairo_rectangle (cr,
				 self->priv->visible_area.x + IMAGE_BORDER,
				 self->priv->visible_area.y + IMAGE_BORDER,
				 self->priv->visible_area.width,
				 self->priv->visible_area.height);
		cairo_stroke (cr);
		cairo_restore (cr);
	}

	return TRUE;
}


static void
get_visible_area_origin_as_double (GthImageOverview *self,
				   int               mx,
				   int               my,
				   double           *x,
				   double           *y)
{
	*x = MIN (mx, self->priv->preview_width);
	*y = MIN (my, self->priv->preview_height);

	if (*x - self->priv->visible_area.width / 2.0 < 0.0)
		*x = self->priv->visible_area.width / 2.0;

	if (*y - self->priv->visible_area.height / 2.0 < 0.0)
		*y = self->priv->visible_area.height / 2.0;

	if (*x + self->priv->visible_area.width / 2.0 > self->priv->preview_width - 0)
		*x = self->priv->preview_width - 0 - self->priv->visible_area.width / 2.0;

	if (*y + self->priv->visible_area.height / 2.0 > self->priv->preview_height - 0)
		*y = self->priv->preview_height - 0 - self->priv->visible_area.height / 2.0;

	*x = *x - self->priv->visible_area.width / 2.0;
	*y = *y - self->priv->visible_area.height / 2.0;
}


static void
update_offset (GthImageOverview *self,
	       GdkEvent         *event)
{
	int    mx, my;
	double x, y;

	gdk_window_get_device_position (gtk_widget_get_window (GTK_WIDGET (self)),
					gdk_event_get_device (event),
					&mx,
					&my,
					NULL);

	get_visible_area_origin_as_double (self, mx, my, &x, &y);

	mx = (int) (x / (self->priv->quality_zoom * self->priv->zoom_factor));
	my = (int) (y / (self->priv->quality_zoom * self->priv->zoom_factor));
	gth_image_viewer_set_scroll_offset (self->priv->viewer, mx, my);
}


static gboolean
gth_image_overview_button_press_event (GtkWidget       *widget,
				       GdkEventButton  *event)
{
	if (event->window != gtk_widget_get_window (widget))
		return FALSE;

	gth_image_overview_activate_scrolling (GTH_IMAGE_OVERVIEW (widget), TRUE, event);
	update_offset (GTH_IMAGE_OVERVIEW (widget), (GdkEvent*) event);

        return TRUE;
}


static gboolean
gth_image_overview_button_release_event (GtkWidget      *widget,
					 GdkEventButton *event)
{
	gth_image_overview_activate_scrolling (GTH_IMAGE_OVERVIEW (widget), FALSE, event);
	return TRUE;
}


static gboolean
gth_image_overview_motion_notify_event (GtkWidget      *widget,
					GdkEventMotion *event)
{
	GthImageOverview *self;

	self = GTH_IMAGE_OVERVIEW (widget);

	if (self->priv->scrolling_active) {
		update_offset (self, (GdkEvent*) event);
		return TRUE;
	}

	return FALSE;
}


static gboolean
gth_image_overview_scroll_event (GtkWidget      *widget,
				 GdkEventScroll *event)
{
	GthImageOverview *self;

	self = GTH_IMAGE_OVERVIEW (widget);

	switch (event->direction) {
	case GDK_SCROLL_UP:
	case GDK_SCROLL_DOWN:
		if (event->direction == GDK_SCROLL_UP)
			gth_image_viewer_zoom_in (self->priv->viewer);
		else
			gth_image_viewer_zoom_out (self->priv->viewer);
		return TRUE;
		break;

	default:
		break;
	}

	return FALSE;
}


static GtkSizeRequestMode
gth_image_overview_get_request_mode (GtkWidget *widget)
{
	return GTK_SIZE_REQUEST_CONSTANT_SIZE;
}


static void
gth_image_overview_get_preferred_width (GtkWidget *widget,
				        int       *minimum_width,
				        int       *natural_width)
{
	GthImageOverview *self = GTH_IMAGE_OVERVIEW (widget);
	int		  size;

	if (self->priv->viewer != NULL)
		size = self->priv->preview_width + (IMAGE_BORDER * 2);
	else
		size = MAX_ALLOCATION_SIZE;
	*minimum_width = *natural_width = size;
}


static void
gth_image_overview_get_preferred_height (GtkWidget *widget,
				        int       *minimum_height,
				        int       *natural_height)
{
	GthImageOverview *self = GTH_IMAGE_OVERVIEW (widget);
	int		  size;

	if (self->priv->viewer != NULL)
		size = self->priv->preview_height + (IMAGE_BORDER * 2);
	else
		size = MAX_ALLOCATION_SIZE;

	*minimum_height = *natural_height = size;
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
	widget_class->scroll_event = gth_image_overview_scroll_event;
	widget_class->get_request_mode = gth_image_overview_get_request_mode;
	widget_class->get_preferred_width = gth_image_overview_get_preferred_width;
	widget_class->get_preferred_height = gth_image_overview_get_preferred_height;

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
	self->priv->preview = NULL;
	self->priv->visible_area.width = 0;
	self->priv->visible_area.height = 0;
	self->priv->zoom_factor = 1.0;
	self->priv->zoom_changed_id = 0;
	self->priv->image_changed_id = 0;
	self->priv->vadj_vchanged_id = 0;
	self->priv->hadj_vchanged_id = 0;
	self->priv->vadj_changed_id = 0;
	self->priv->hadj_changed_id = 0;
	self->priv->scrolling_active = FALSE;

	/* do not use the rgba visual on the drawing area */
	{
	        GdkVisual *visual;
	        visual = gdk_screen_get_system_visual (gtk_widget_get_screen (GTK_WIDGET (self)));
        	if (visual != NULL)
                	gtk_widget_set_visual (GTK_WIDGET (self), visual);
	}
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
	if (width != NULL) *width = self->priv->preview_width;
	if (height != NULL) *height = self->priv->preview_height;
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
		gtk_grab_remove (GTK_WIDGET (self));
	}

	self->priv->scrolling_active = active;
}


gboolean
gth_image_overview_get_scrolling_is_active (GthImageOverview	*self)
{
	return self->priv->scrolling_active;
}
