/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2006-2010 Free Software Foundation, Inc.
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
#include "gth-image-navigator.h"
#include "gth-image-overview.h"
#include "gth-image-viewer.h"
#include "gtk-utils.h"
#include "pixbuf-utils.h"


#define POPUP_BORDER   2
#define POPUP_BORDER_2 (POPUP_BORDER*2)


/* Properties */
enum {
        PROP_0,
        PROP_VIEWER
};


struct _GthImageNavigatorPrivate {
	GtkWidget *viewer;
	GtkWidget *vscrollbar;
	GtkWidget *hscrollbar;
	GtkWidget *navigator_event_area;
	gboolean   automatic_scrollbars;
	gboolean   hscrollbar_visible;
	gboolean   vscrollbar_visible;
};


static void
_gth_image_navigator_set_viewer (GthImageNavigator *self,
			         GtkWidget         *viewer)
{
	if (self->priv->viewer == viewer)
		return;

	if (self->priv->viewer != NULL)
		gtk_container_remove (GTK_CONTAINER (self), self->priv->viewer);

	if (viewer == NULL)
		return;

	gtk_container_add (GTK_CONTAINER (self), viewer);
	gtk_range_set_adjustment (GTK_RANGE (self->priv->hscrollbar), gtk_scrollable_get_hadjustment (GTK_SCROLLABLE (viewer)));
	gtk_range_set_adjustment (GTK_RANGE (self->priv->vscrollbar), gtk_scrollable_get_vadjustment (GTK_SCROLLABLE (viewer)));

	g_object_notify (G_OBJECT (self), "viewer");
}


static void
gth_image_navigator_set_property (GObject      *object,
				  guint         property_id,
				  const GValue *value,
				  GParamSpec   *pspec)
{
	GthImageNavigator *self = GTH_IMAGE_NAVIGATOR (object);

	switch (property_id) {
	case PROP_VIEWER:
		_gth_image_navigator_set_viewer (self, g_value_get_object (value));
		break;

	default:
		break;
	}
}


static void
gth_image_navigator_get_property (GObject    *object,
				  guint       property_id,
				  GValue     *value,
				  GParamSpec *pspec)
{
	GthImageNavigator *self = GTH_IMAGE_NAVIGATOR (object);

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
gth_image_navigator_size_allocate (GtkWidget     *widget,
				   GtkAllocation *allocation)
{
	GthImageNavigator *self = (GthImageNavigator *) widget;
	gboolean           hscrollbar_visible;
	gboolean           vscrollbar_visible;
	GtkAllocation      viewer_allocation;

	gtk_widget_set_allocation (widget, allocation);

	if (self->priv->automatic_scrollbars) {
		gth_image_viewer_needs_scrollbars (GTH_IMAGE_VIEWER (self->priv->viewer),
						   allocation,
						   self->priv->hscrollbar,
						   self->priv->vscrollbar,
						   &hscrollbar_visible,
						   &vscrollbar_visible);

		if (self->priv->vscrollbar_visible != vscrollbar_visible)
			self->priv->vscrollbar_visible = vscrollbar_visible;

		if (self->priv->hscrollbar_visible != hscrollbar_visible)
			self->priv->hscrollbar_visible = hscrollbar_visible;
	}
	else {
		self->priv->vscrollbar_visible = FALSE;
		self->priv->hscrollbar_visible = FALSE;
	}

	gtk_widget_set_child_visible (self->priv->vscrollbar, self->priv->vscrollbar_visible);
	gtk_widget_set_child_visible (self->priv->hscrollbar, self->priv->hscrollbar_visible);
	gtk_widget_set_child_visible (self->priv->navigator_event_area, self->priv->hscrollbar_visible || self->priv->vscrollbar_visible);

	viewer_allocation = *allocation;
	if (self->priv->hscrollbar_visible || self->priv->vscrollbar_visible) {
		GtkRequisition vscrollbar_requisition;
		GtkRequisition hscrollbar_requisition;
		GtkAllocation  child_allocation;

		gtk_widget_get_preferred_size (self->priv->vscrollbar, &vscrollbar_requisition, NULL);
		gtk_widget_get_preferred_size (self->priv->hscrollbar, &hscrollbar_requisition, NULL);

		if (self->priv->vscrollbar_visible) {
			viewer_allocation.width -= vscrollbar_requisition.width;

			/* vertical scrollbar */

			child_allocation.x = allocation->x + allocation->width - vscrollbar_requisition.width;
			child_allocation.y = allocation->y;
			child_allocation.width = vscrollbar_requisition.width;
			child_allocation.height = allocation->height - hscrollbar_requisition.height;
			gtk_widget_size_allocate (self->priv->vscrollbar, &child_allocation);
		}

		if (self->priv->hscrollbar_visible) {
			viewer_allocation.height -= hscrollbar_requisition.height;

			/* horizontal scrollbar */

			child_allocation.x = allocation->x;
			child_allocation.y = allocation->y + allocation->height - hscrollbar_requisition.height;
			child_allocation.width = allocation->width - vscrollbar_requisition.width;
			child_allocation.height = hscrollbar_requisition.height;
			gtk_widget_size_allocate (self->priv->hscrollbar, &child_allocation);
		}

		/* event area */

		child_allocation.x = allocation->x + allocation->width - vscrollbar_requisition.width;
		child_allocation.y = allocation->y + allocation->height - hscrollbar_requisition.height;
		child_allocation.width = vscrollbar_requisition.width;
		child_allocation.height = hscrollbar_requisition.height;
		gtk_widget_size_allocate (self->priv->navigator_event_area, &child_allocation);
	}

	gtk_widget_size_allocate (self->priv->viewer, &viewer_allocation);
}


typedef struct {
	GtkWidget *container;
	cairo_t   *cr;
} DrawData;


static void
gth_image_navigator_draw_child (GtkWidget *child,
                                gpointer   user_data)
{
	DrawData *data = user_data;

	if (gtk_widget_get_child_visible (child))
		gtk_container_propagate_draw (GTK_CONTAINER (data->container),
					      child,
					      data->cr);
}


static gboolean
gth_image_navigator_draw (GtkWidget *widget,
			  cairo_t   *cr)
{
	DrawData data;

	data.container = widget;
	data.cr = cr;
	gtk_container_forall (GTK_CONTAINER (widget),
			      gth_image_navigator_draw_child,
			      &data);

	return FALSE;
}


static void
gth_image_navigator_add (GtkContainer *container,
			 GtkWidget    *widget)
{
	GthImageNavigator *self = GTH_IMAGE_NAVIGATOR (container);

	if (self->priv->viewer != NULL) {
		g_warning ("Attempt to add a second widget to a GthImageNavigator");
		return;
	}

	gtk_widget_set_parent (widget, GTK_WIDGET (container));
	self->priv->viewer = widget;
}


static void
gth_image_navigator_remove (GtkContainer *container,
			    GtkWidget    *widget)
{
	GthImageNavigator *self = GTH_IMAGE_NAVIGATOR (container);
	gboolean           widget_was_visible;

	g_return_if_fail (self->priv->viewer == widget);

	widget_was_visible = gtk_widget_get_visible (widget);
	gtk_widget_unparent (widget);
	self->priv->viewer = NULL;

	if (widget_was_visible && gtk_widget_get_visible (GTK_WIDGET (container)))
		gtk_widget_queue_resize (GTK_WIDGET (container));
}


static void
gth_image_navigator_forall (GtkContainer *container,
			    gboolean      include_internals,
			    GtkCallback   callback,
			    gpointer      callback_data)
{
	GthImageNavigator *self = GTH_IMAGE_NAVIGATOR (container);

	if (self->priv->viewer != NULL)
		(* callback) (GTK_WIDGET (self->priv->viewer), callback_data);
	if (include_internals) {
		(* callback) (self->priv->hscrollbar, callback_data);
		(* callback) (self->priv->vscrollbar, callback_data);
		(* callback) (self->priv->navigator_event_area, callback_data);
	}
}


static GType
gth_image_navigator_child_type (GtkContainer *container)
{
	return GTK_TYPE_WIDGET;
}


static void
gth_image_navigator_class_init (GthImageNavigatorClass *klass)
{
	GObjectClass      *object_class;
	GtkWidgetClass    *widget_class;
	GtkContainerClass *container_class;

	g_type_class_add_private (klass, sizeof (GthImageNavigatorPrivate));

	object_class = (GObjectClass *) klass;
	object_class->set_property = gth_image_navigator_set_property;
	object_class->get_property = gth_image_navigator_get_property;

	widget_class = (GtkWidgetClass *) klass;
	widget_class->size_allocate = gth_image_navigator_size_allocate;
	widget_class->draw = gth_image_navigator_draw;

	container_class = (GtkContainerClass *) klass;
	container_class->add = gth_image_navigator_add;
	container_class->remove = gth_image_navigator_remove;
	container_class->forall = gth_image_navigator_forall;
	container_class->child_type = gth_image_navigator_child_type;
	gtk_container_class_handle_border_width (container_class);

	/* properties */

	g_object_class_install_property (object_class,
					 PROP_VIEWER,
					 g_param_spec_object ("viewer",
                                                              "Viewer",
                                                              "The image viewer to use",
                                                              GTH_TYPE_IMAGE_VIEWER,
                                                              G_PARAM_READWRITE));
}


/* -- NavigatorPopup -- */


typedef struct {
	GthImageViewer	*viewer;
	GtkWidget	*popup_win;
	GtkWidget	*overview;
} NavigatorPopup;


static void
navigator_popup_close (NavigatorPopup *nav_popup)
{
	gtk_widget_destroy (nav_popup->popup_win);
	g_free (nav_popup);
}


static int
popup_window_event_cb (GtkWidget *widget,
		       GdkEvent  *event,
		       gpointer   data)
{
	NavigatorPopup *nav_popup = data;

	switch (event->type) {
	case GDK_BUTTON_RELEASE:
		navigator_popup_close (nav_popup);
		return TRUE;

	case GDK_MOTION_NOTIFY:
		gtk_widget_event (nav_popup->overview, event);
		gtk_widget_queue_draw (nav_popup->overview);
		return TRUE;

	case GDK_KEY_PRESS:
		switch (event->key.keyval) {
		case GDK_KEY_plus:
		case GDK_KEY_minus:
		case GDK_KEY_1:
			switch (event->key.keyval) {
			case GDK_KEY_plus:
				gth_image_viewer_zoom_in (nav_popup->viewer);
				break;
			case GDK_KEY_minus:
				gth_image_viewer_zoom_out (nav_popup->viewer);
				break;
			case GDK_KEY_1:
				gth_image_viewer_set_zoom (nav_popup->viewer, 1.0);
				break;
			}
			break;

		case GDK_KEY_Escape:
			navigator_popup_close (nav_popup);
			break;

		default:
			break;
		}
		return TRUE;

	default:
		break;
	}

	return FALSE;
}


static void
navigator_event_area_button_press_event_cb (GtkWidget      *widget,
					    GdkEventButton *event,
					    gpointer        user_data)
{
	GthImageNavigator	*self = user_data;
	NavigatorPopup		*nav_popup;
	GtkWidget		*out_frame;
	GtkWidget		*in_frame;
	int			 popup_x, popup_y;
	int			 popup_width, popup_height;
	cairo_rectangle_int_t	 visible_area;

	if ((self->priv->viewer == NULL) || gth_image_viewer_is_void (GTH_IMAGE_VIEWER (self->priv->viewer)))
		return;

	/* create the popup and its content */

	nav_popup = g_new0 (NavigatorPopup, 1);
	nav_popup->viewer = GTH_IMAGE_VIEWER (self->priv->viewer);
	nav_popup->popup_win = gtk_window_new (GTK_WINDOW_POPUP);
	g_signal_connect (G_OBJECT (nav_popup->popup_win),
			  "event",
			  G_CALLBACK (popup_window_event_cb),
			  nav_popup);
	gtk_window_set_wmclass (GTK_WINDOW (nav_popup->popup_win), "", "gthumb_navigator");

	out_frame = gtk_frame_new (NULL);
	gtk_frame_set_shadow_type (GTK_FRAME (out_frame), GTK_SHADOW_OUT);
	gtk_container_add (GTK_CONTAINER (nav_popup->popup_win), out_frame);

	in_frame = gtk_frame_new (NULL);
	gtk_frame_set_shadow_type (GTK_FRAME (in_frame), GTK_SHADOW_IN);
	gtk_container_add (GTK_CONTAINER (out_frame), in_frame);

	nav_popup->overview = gth_image_overview_new (GTH_IMAGE_VIEWER (self->priv->viewer));
	gtk_container_add (GTK_CONTAINER (in_frame), nav_popup->overview);

	/* show the popup */

	gth_image_overview_get_size (GTH_IMAGE_OVERVIEW (nav_popup->overview),
				     &popup_width,
				     &popup_height);
	gth_image_overview_get_visible_area (GTH_IMAGE_OVERVIEW (nav_popup->overview),
					     &visible_area.x,
					     &visible_area.y,
					     &visible_area.width,
					     &visible_area.height);
	popup_x = MIN (event->x_root - visible_area.x - POPUP_BORDER - visible_area.width / 2,
		       gdk_screen_width () - popup_width - POPUP_BORDER_2);
	popup_y = MIN (event->y_root - visible_area.y - POPUP_BORDER - visible_area.height / 2,
		       gdk_screen_height () - popup_height - POPUP_BORDER_2);
	gtk_window_move (GTK_WINDOW (nav_popup->popup_win),
			 popup_x,
			 popup_y);
  	gtk_window_set_default_size (GTK_WINDOW (nav_popup->popup_win),
				     popup_width + POPUP_BORDER_2,
				     popup_height + POPUP_BORDER_2);
	gtk_widget_show_all (nav_popup->popup_win);

	gth_image_overview_activate_scrolling (GTH_IMAGE_OVERVIEW (nav_popup->overview), TRUE, event);
}


static void
gth_image_navigator_init (GthImageNavigator *self)
{
	GtkWidget *navigator_icon;

	self->priv = G_TYPE_INSTANCE_GET_PRIVATE ((self), GTH_TYPE_IMAGE_NAVIGATOR, GthImageNavigatorPrivate);

	gtk_widget_set_has_window (GTK_WIDGET (self), FALSE);
	gtk_container_set_reallocate_redraws (GTK_CONTAINER (self), TRUE);

	self->priv->automatic_scrollbars = TRUE;
	self->priv->hscrollbar_visible = FALSE;
	self->priv->vscrollbar_visible = FALSE;

	/* horizonal scrollbar */

	self->priv->hscrollbar = gtk_scrollbar_new (GTK_ORIENTATION_HORIZONTAL, NULL);
	gtk_widget_set_parent (self->priv->hscrollbar, GTK_WIDGET (self));

	/* vertical scrollbar */

	self->priv->vscrollbar = gtk_scrollbar_new (GTK_ORIENTATION_VERTICAL, NULL);
	gtk_widget_set_parent (self->priv->vscrollbar, GTK_WIDGET (self));

	/* navigator event area */

	navigator_icon = gtk_image_new_from_icon_name ("image-navigator-symbolic", GTK_ICON_SIZE_MENU);
	gtk_image_set_pixel_size (GTK_IMAGE (navigator_icon), 12);

	self->priv->navigator_event_area = gtk_event_box_new ();
	gtk_widget_set_parent (GTK_WIDGET (self->priv->navigator_event_area), GTK_WIDGET (self));
	gtk_container_add (GTK_CONTAINER (self->priv->navigator_event_area), navigator_icon);
	g_signal_connect (G_OBJECT (self->priv->navigator_event_area),
			  "button_press_event",
			  G_CALLBACK (navigator_event_area_button_press_event_cb),
			  self);

	gtk_widget_show (self->priv->hscrollbar);
	gtk_widget_show (self->priv->vscrollbar);
	gtk_widget_show_all (self->priv->navigator_event_area);
}


G_DEFINE_TYPE (GthImageNavigator, gth_image_navigator, GTK_TYPE_CONTAINER)


GtkWidget *
gth_image_navigator_new (GthImageViewer *viewer)
{
	g_return_val_if_fail (viewer != NULL, NULL);
	return (GtkWidget *) g_object_new (GTH_TYPE_IMAGE_NAVIGATOR, "viewer", viewer, NULL);
}


void
gth_image_navigator_set_automatic_scrollbars (GthImageNavigator *self,
				              gboolean           automatic)
{
	self->priv->automatic_scrollbars = automatic;
	gtk_widget_queue_resize (self->priv->viewer);
}
