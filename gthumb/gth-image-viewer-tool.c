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
#include "gth-image-viewer-tool.h"


GType
gth_image_viewer_tool_get_type (void)
{
	static GType type_id = 0;
	if (type_id == 0) {
		static const GTypeInfo g_define_type_info = {
			sizeof (GthImageViewerToolIface),
			(GBaseInitFunc) NULL,
			(GBaseFinalizeFunc) NULL,
			(GClassInitFunc) NULL,
			(GClassFinalizeFunc) NULL,
			NULL,
			0,
			0,
			(GInstanceInitFunc) NULL,
			NULL
		};
		type_id = g_type_register_static (G_TYPE_INTERFACE,
						  "GthImageViewerTool",
						  &g_define_type_info,
						  0);
	}
	return type_id;
}


void
gth_image_viewer_tool_realize (GthImageViewerTool *self)
{
	GTH_IMAGE_VIEWER_TOOL_GET_INTERFACE (self)->realize (self);
}


void
gth_image_viewer_tool_unrealize (GthImageViewerTool *self)
{
	GTH_IMAGE_VIEWER_TOOL_GET_INTERFACE (self)->unrealize (self);
}


void
gth_image_viewer_tool_size_allocate (GthImageViewerTool *self,
			      GtkAllocation      *allocation)
{
	GTH_IMAGE_VIEWER_TOOL_GET_INTERFACE (self)->size_allocate (self, allocation);
}


void
gth_image_viewer_tool_expose (GthImageViewerTool *self,
		       GdkRectangle       *paint_area)
{
	GTH_IMAGE_VIEWER_TOOL_GET_INTERFACE (self)->expose (self, paint_area);
}


gboolean
gth_image_viewer_tool_button_press (GthImageViewerTool *self,
			     GdkEventButton     *event)
{
	return GTH_IMAGE_VIEWER_TOOL_GET_INTERFACE (self)->button_press (self, event);
}


gboolean
gth_image_viewer_tool_button_release (GthImageViewerTool *self,
			       GdkEventButton     *event)
{
	return GTH_IMAGE_VIEWER_TOOL_GET_INTERFACE (self)->button_release (self, event);
}


gboolean
gth_image_viewer_tool_motion_notify (GthImageViewerTool *self,
			      GdkEventMotion     *event)
{
	return GTH_IMAGE_VIEWER_TOOL_GET_INTERFACE (self)->motion_notify (self, event);
}


void
gth_image_viewer_tool_image_changed (GthImageViewerTool *self)
{
	GTH_IMAGE_VIEWER_TOOL_GET_INTERFACE (self)->image_changed (self);
}


void
gth_image_viewer_tool_zoom_changed (GthImageViewerTool *self)
{
	GTH_IMAGE_VIEWER_TOOL_GET_INTERFACE (self)->zoom_changed (self);
}
