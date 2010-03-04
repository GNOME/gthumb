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
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Street #330, Boston, MA 02111-1307, USA.
 */

#include <config.h>
#include <math.h>
#include "glib-utils.h"
#include "gth-cursors.h"
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
_gth_image_dragger_update_cursor (GthImageDragger *self)
{
	GdkCursor *cursor;

	if (self->priv->draggable)
		cursor = gth_cursor_get (gtk_widget_get_window (GTK_WIDGET (self->priv->viewer)), GTH_CURSOR_HAND_OPEN);
	else
		cursor = gdk_cursor_new (GDK_LEFT_PTR);
	gth_image_viewer_set_cursor (self->priv->viewer, cursor);

	gdk_cursor_unref (cursor);
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
gth_image_dragger_size_allocate (GthImageViewerTool  *base,
				 GtkAllocation       *allocation)
{
	GthImageDragger *self;
	GthImageViewer  *viewer;

	self = (GthImageDragger *) base;
	viewer = (GthImageViewer *) self->priv->viewer;

	self->priv->draggable = (viewer->hadj->upper > viewer->hadj->page_size) || (viewer->vadj->upper > viewer->vadj->page_size);
	if (GTK_WIDGET_REALIZED (viewer))
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
			  GdkRectangle       *event_area)
{
	GthImageDragger *dragger;
	GthImageViewer  *viewer;
	GdkInterpType    interp_type;
	GdkRectangle     paint_area;

	dragger = (GthImageDragger *) self;
	viewer = dragger->priv->viewer;

	if (gth_image_viewer_get_current_pixbuf (viewer) == NULL)
		return;

	if (gth_image_viewer_get_zoom_quality (viewer) == GTH_ZOOM_QUALITY_LOW)
		interp_type = GDK_INTERP_TILES;
	else
		interp_type = GDK_INTERP_BILINEAR;

	if (gth_image_viewer_get_zoom (viewer) == 1.0)
		interp_type = GDK_INTERP_NEAREST;

	if (gdk_rectangle_intersect (&viewer->image_area, event_area, &paint_area))
		gth_image_viewer_paint (viewer,
					gth_image_viewer_get_current_pixbuf (viewer),
					viewer->x_offset + paint_area.x - viewer->image_area.x,
					viewer->y_offset + paint_area.y - viewer->image_area.y,
					paint_area.x,
					paint_area.y,
					paint_area.width,
					paint_area.height,
					interp_type);
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

		cursor = gth_cursor_get (widget->window, GTH_CURSOR_HAND_CLOSED);
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
