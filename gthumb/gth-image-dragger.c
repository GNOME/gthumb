/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2009 Free Software Foundation, Inc.
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
#include "glib-utils.h"
#include "gth-image-dragger.h"


struct _GthImageDraggerPrivate {
	GthImageViewer *viewer;
	gboolean        draggable;
};


static gpointer parent_class = NULL;


static void
gth_image_dragger_finalize (GObject *object)
{
	g_return_if_fail (object != NULL);
	g_return_if_fail (GTH_IS_IMAGE_DRAGGER (object));

	/* Chain up */
	G_OBJECT_CLASS (parent_class)->finalize (object);
}


static void
gth_image_dragger_class_init (GthImageDraggerClass *class)
{
	GObjectClass *gobject_class;

	parent_class = g_type_class_peek_parent (class);
	g_type_class_add_private (class, sizeof (GthImageDraggerPrivate));

	gobject_class = (GObjectClass*) class;
	gobject_class->finalize = gth_image_dragger_finalize;
}


static void
gth_image_dragger_instance_init (GthImageDragger *dragger)
{
	dragger->priv = G_TYPE_INSTANCE_GET_PRIVATE (dragger, GTH_TYPE_IMAGE_DRAGGER, GthImageDraggerPrivate);
}


static void
gth_image_dragger_realize (GthImageViewerTool *base)
{
	/* void */
}


static void
gth_image_dragger_unrealize (GthImageViewerTool *base)
{
	/* void */
}


static void
_gth_image_dragger_update_cursor (GthImageDragger *self)
{
	GdkCursor *cursor;

	cursor = gdk_cursor_new (GDK_LEFT_PTR);
	gth_image_viewer_set_cursor (self->priv->viewer, cursor);

	gdk_cursor_unref (cursor);
}


static void
gth_image_dragger_size_allocate (GthImageViewerTool *base,
				 GtkAllocation      *allocation)
{
	GthImageDragger *self;
	GthImageViewer  *viewer;
	double           h_page_size;
	double           v_page_size;
	double           h_upper;
	double           v_upper;

	self = (GthImageDragger *) base;
	viewer = (GthImageViewer *) self->priv->viewer;

	h_page_size = gtk_adjustment_get_page_size (viewer->hadj);
	v_page_size = gtk_adjustment_get_page_size (viewer->vadj);
	h_upper = gtk_adjustment_get_upper (viewer->hadj);
	v_upper = gtk_adjustment_get_upper (viewer->vadj);

	self->priv->draggable = (h_page_size > 0) && (v_page_size > 0) && ((h_upper > h_page_size) || (v_upper > v_page_size));
	if (gtk_widget_get_realized (GTK_WIDGET (viewer)))
		_gth_image_dragger_update_cursor (self);
}


static void
gth_image_dragger_map (GthImageViewerTool *base)
{
	/* void */
}


static void
gth_image_dragger_unmap (GthImageViewerTool *base)
{
	/* void */
}


static void
gth_image_dragger_expose (GthImageViewerTool *self,
			  GdkEventExpose     *event,
		          cairo_t            *cr)
{
	GthImageDragger *dragger;
	GthImageViewer  *viewer;
	cairo_filter_t   filter;

	dragger = (GthImageDragger *) self;
	viewer = dragger->priv->viewer;

	gth_image_viewer_paint_background (viewer, cr);

	if (gth_image_viewer_get_current_image (viewer) == NULL)
		return;

	if (gth_image_viewer_get_zoom_quality (viewer) == GTH_ZOOM_QUALITY_LOW)
		filter = CAIRO_FILTER_FAST;
	else
		filter = CAIRO_FILTER_GOOD;

	if (gth_image_viewer_get_zoom (viewer) == 1.0)
		filter = CAIRO_FILTER_FAST;

	gth_image_viewer_paint_region (viewer,
				       cr,
				       gth_image_viewer_get_current_image (viewer),
				       viewer->x_offset - viewer->image_area.x,
				       viewer->y_offset - viewer->image_area.y,
				       &viewer->image_area,
				       event->region,
				       filter);

	gth_image_viewer_apply_painters (viewer, event, cr);
}


static gboolean
gth_image_dragger_button_press (GthImageViewerTool *self,
				GdkEventButton     *event)
{
	GthImageDragger *dragger;
	GthImageViewer  *viewer;
	GtkWidget       *widget;

	dragger = (GthImageDragger *) self;
	viewer = dragger->priv->viewer;
	widget = GTK_WIDGET (viewer);

	if (! dragger->priv->draggable)
		return FALSE;

	if ((event->button == 1) && ! viewer->dragging) {
		GdkCursor *cursor;
		int        retval;

		cursor = gdk_cursor_new_from_name (gtk_widget_get_display (widget), "grabbing");
		retval = gdk_pointer_grab (gtk_widget_get_window (widget),
					   FALSE,
					   (GDK_POINTER_MOTION_MASK
					    | GDK_POINTER_MOTION_HINT_MASK
					    | GDK_BUTTON_RELEASE_MASK),
					   NULL,
					   cursor,
					   event->time);

		if (cursor != NULL)
			gdk_cursor_unref (cursor);

		if (retval != 0)
			return FALSE;

		viewer->pressed = TRUE;
		viewer->dragging = TRUE;

		return TRUE;
	}

	return FALSE;
}


static gboolean
gth_image_dragger_button_release (GthImageViewerTool *self,
				  GdkEventButton     *event)
{
	GthImageDragger *dragger;
	GthImageViewer  *viewer;

	if (event->button != 1)
		return FALSE;

	dragger = (GthImageDragger *) self;
	viewer = dragger->priv->viewer;

	if (viewer->dragging)
		gdk_pointer_ungrab (event->time);

	return TRUE;
}


static gboolean
gth_image_dragger_motion_notify (GthImageViewerTool *self,
				 GdkEventMotion     *event)
{
	GthImageDragger *dragger;
	GthImageViewer  *viewer;

	dragger = (GthImageDragger *) self;
	viewer = dragger->priv->viewer;

	if (! viewer->pressed)
		return FALSE;

	viewer->dragging = TRUE;

	if (! event->is_hint)
		return FALSE;

	gth_image_viewer_scroll_to (viewer,
				    viewer->x_offset + viewer->event_x_prev - event->x,
				    viewer->y_offset + viewer->event_y_prev - event->y);

	return TRUE;
}


static void
gth_image_dragger_image_changed (GthImageViewerTool *self)
{
	/* void */
}


static void
gth_image_dragger_zoom_changed (GthImageViewerTool *self)
{
	/* void */
}


static void
gth_image_dragger_gth_image_tool_interface_init (GthImageViewerToolIface *iface)
{
	iface->realize = gth_image_dragger_realize;
	iface->unrealize = gth_image_dragger_unrealize;
	iface->size_allocate = gth_image_dragger_size_allocate;
	iface->map = gth_image_dragger_map;
	iface->unmap = gth_image_dragger_unmap;
	iface->expose = gth_image_dragger_expose;
	iface->button_press = gth_image_dragger_button_press;
	iface->button_release = gth_image_dragger_button_release;
	iface->motion_notify = gth_image_dragger_motion_notify;
	iface->image_changed = gth_image_dragger_image_changed;
	iface->zoom_changed = gth_image_dragger_zoom_changed;
}


GType
gth_image_dragger_get_type (void)
{
	static GType type = 0;

	if (! type) {
		GTypeInfo type_info = {
			sizeof (GthImageDraggerClass),
			NULL,
			NULL,
			(GClassInitFunc) gth_image_dragger_class_init,
			NULL,
			NULL,
			sizeof (GthImageDragger),
			0,
			(GInstanceInitFunc) gth_image_dragger_instance_init
		};
		static const GInterfaceInfo gth_image_tool_info = {
			(GInterfaceInitFunc) gth_image_dragger_gth_image_tool_interface_init,
			(GInterfaceFinalizeFunc) NULL,
			NULL
		};

		type = g_type_register_static (G_TYPE_OBJECT,
					       "GthImageDragger",
					       &type_info,
					       0);
		g_type_add_interface_static (type, GTH_TYPE_IMAGE_VIEWER_TOOL, &gth_image_tool_info);
	}

	return type;
}


GthImageViewerTool *
gth_image_dragger_new (GthImageViewer *viewer)
{
	GthImageDragger *dragger;

	dragger = g_object_new (GTH_TYPE_IMAGE_DRAGGER, NULL);
	dragger->priv->viewer = viewer;

	return GTH_IMAGE_VIEWER_TOOL (dragger);
}
