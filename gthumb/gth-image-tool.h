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

#ifndef GTH_IMAGE_TOOL_H
#define GTH_IMAGE_TOOL_H

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define GTH_TYPE_IMAGE_TOOL               (gth_image_tool_get_type ())
#define GTH_IMAGE_TOOL(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTH_TYPE_IMAGE_TOOL, GthImageTool))
#define GTH_IS_IMAGE_TOOL(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTH_TYPE_IMAGE_TOOL))
#define GTH_IMAGE_TOOL_GET_INTERFACE(obj) (G_TYPE_INSTANCE_GET_INTERFACE ((obj), GTH_TYPE_IMAGE_TOOL, GthImageToolIface))

typedef struct _GthImageTool GthImageTool;
typedef struct _GthImageToolIface GthImageToolIface;

struct _GthImageToolIface {
	GTypeInterface parent_iface;

	void      (*realize)        (GthImageTool   *self);
	void      (*unrealize)      (GthImageTool   *self);
	void      (*size_allocate)  (GthImageTool   *self,
				     GtkAllocation  *allocation);
	void      (*expose)         (GthImageTool   *self,
				     GdkRectangle   *paint_area);
	gboolean  (*button_press)   (GthImageTool   *self,
				     GdkEventButton *event);
	gboolean  (*button_release) (GthImageTool   *self,
				     GdkEventButton *event);
	gboolean  (*motion_notify)  (GthImageTool   *self,
				     GdkEventMotion *event);
	void      (*image_changed)  (GthImageTool   *self);
	void      (*zoom_changed)   (GthImageTool   *self);
};

GType      gth_image_tool_get_type         (void);
void       gth_image_tool_realize          (GthImageTool   *self);
void       gth_image_tool_unrealize        (GthImageTool   *self);
void       gth_image_tool_size_allocate    (GthImageTool   *self,
					    GtkAllocation  *allocation);
void       gth_image_tool_expose           (GthImageTool   *self,
					    GdkRectangle   *paint_area);
gboolean   gth_image_tool_button_press     (GthImageTool   *self,
					    GdkEventButton *event);
gboolean   gth_image_tool_button_release   (GthImageTool   *self,
					    GdkEventButton *event);
gboolean   gth_image_tool_motion_notify    (GthImageTool   *self,
					    GdkEventMotion *event);
void       gth_image_tool_image_changed    (GthImageTool   *self);
void       gth_image_tool_zoom_changed     (GthImageTool   *self);

G_END_DECLS

#endif /* GTH_IMAGE_TOOL_H */
