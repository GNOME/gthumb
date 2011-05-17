/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2011 Free Software Foundation, Inc.
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
#include <gthumb.h>
#include "gth-image-line-tool.h"


#define MIN4(a,b,c,d) MIN(MIN((a),(b)),MIN((c),(d)))
#define MAX4(a,b,c,d) MAX(MAX((a),(b)),MAX((c),(d)))


enum {
	CHANGED,
	LAST_SIGNAL
};


static guint signals[LAST_SIGNAL] = { 0 };
static gpointer parent_class = NULL;


struct _GthImageLineToolPrivate {
	GthImageViewer     *viewer;

	/* options */

	GdkPoint            p1;
	GdkPoint            p2;

	/* utility variables */

	int                 original_width;
	int                 original_height;
	double              preview_zoom;
	cairo_surface_t    *preview_image;
	GdkRectangle        preview_image_area;
	GdkPoint            preview_center;
	GdkRectangle        clip_area;
	cairo_matrix_t      matrix;
	gboolean            first_point_set;
	GthFit              original_fit_mode;
	gboolean            original_zoom_enabled;
};


static void
gth_image_line_tool_set_viewer (GthImageViewerTool *base,
			        GthImageViewer     *viewer)
{
	GthImageLineTool *self = GTH_IMAGE_LINE_TOOL (base);

	self->priv->viewer = viewer;
	self->priv->original_fit_mode = gth_image_viewer_get_fit_mode (GTH_IMAGE_VIEWER (viewer));
	self->priv->original_zoom_enabled = gth_image_viewer_get_zoom_enabled (GTH_IMAGE_VIEWER (viewer));
	gth_image_viewer_set_fit_mode (GTH_IMAGE_VIEWER (viewer), GTH_FIT_SIZE_IF_LARGER);
	gth_image_viewer_set_zoom_enabled (GTH_IMAGE_VIEWER (viewer), FALSE);
	self->priv->first_point_set = FALSE;
}


static void
gth_image_line_tool_unset_viewer (GthImageViewerTool *base,
			          GthImageViewer     *viewer)
{
	GthImageLineTool *self = GTH_IMAGE_LINE_TOOL (base);

	gth_image_viewer_set_fit_mode (GTH_IMAGE_VIEWER (viewer), self->priv->original_fit_mode);
	gth_image_viewer_set_zoom_enabled (GTH_IMAGE_VIEWER (viewer), self->priv->original_zoom_enabled);
	self->priv->viewer = NULL;
	self->priv->first_point_set = FALSE;
}


static void
gth_image_line_tool_realize (GthImageViewerTool *base)
{
	/* void */
}


static void
gth_image_line_tool_unrealize (GthImageViewerTool *base)
{
	/* void */
}


static void
update_image_surface (GthImageLineTool *self)
{
	GtkAllocation    allocation;
	cairo_surface_t *image;
	int              max_size;
	int              width;
	int              height;
	cairo_surface_t *preview_image;

	if (self->priv->preview_image != NULL) {
		cairo_surface_destroy (self->priv->preview_image);
		self->priv->preview_image = NULL;
	}

	image = gth_image_viewer_get_current_image (GTH_IMAGE_VIEWER (self->priv->viewer));
	if (image == NULL)
		return;

	self->priv->original_width = cairo_image_surface_get_width (image);
	self->priv->original_height = cairo_image_surface_get_height (image);
	width = self->priv->original_width;
	height = self->priv->original_height;
	gtk_widget_get_allocation (GTK_WIDGET (self->priv->viewer), &allocation);
	max_size = MAX (allocation.width, allocation.height) / G_SQRT2 + 2;
	if (scale_keeping_ratio (&width, &height, max_size, max_size, FALSE))
		preview_image = _cairo_image_surface_scale_to (image, width, height, CAIRO_FILTER_BILINEAR);
	else
		preview_image = cairo_surface_reference (image);

	self->priv->preview_zoom = (double) width / self->priv->original_width;
	self->priv->preview_image = preview_image;
	self->priv->preview_image_area.width = width;
	self->priv->preview_image_area.height = height;
	self->priv->preview_image_area.x = MAX ((allocation.width - self->priv->preview_image_area.width) / 2 - 0.5, 0);
	self->priv->preview_image_area.y = MAX ((allocation.height - self->priv->preview_image_area.height) / 2 - 0.5, 0);
}


static void
gth_image_line_tool_size_allocate (GthImageViewerTool *base,
				   GtkAllocation      *allocation)
{
	update_image_surface (GTH_IMAGE_LINE_TOOL (base));
}


static void
gth_image_line_tool_map (GthImageViewerTool *base)
{
	/* void */
}


static void
gth_image_line_tool_unmap (GthImageViewerTool *base)
{
	/* void */
}


static void
paint_image (GthImageLineTool *self,
	     cairo_t          *cr)
{
	cairo_save (cr);

	cairo_set_antialias (cr, CAIRO_ANTIALIAS_NONE);
	cairo_set_source_surface (cr, self->priv->preview_image,
				  self->priv->preview_image_area.x,
				  self->priv->preview_image_area.y);
	cairo_pattern_set_filter (cairo_get_source (cr), CAIRO_FILTER_FAST);
  	cairo_rectangle (cr,
  			 self->priv->preview_image_area.x,
  			 self->priv->preview_image_area.y,
  			 self->priv->preview_image_area.width,
  			 self->priv->preview_image_area.height);
  	cairo_fill (cr);

  	cairo_restore (cr);
}


static void
gth_image_line_tool_expose (GthImageViewerTool *base,
			    GdkEventExpose     *event,
			    cairo_t            *cr)
{
	GthImageLineTool *self = GTH_IMAGE_LINE_TOOL (base);

	if (self->priv->preview_image == NULL)
		return;

	cairo_save (cr);

  	cairo_rectangle (cr,
  			 event->area.x,
  			 event->area.y,
  			 event->area.width,
  			 event->area.height);
  	cairo_clip (cr);

	paint_image (self, cr);

	if (self->priv->first_point_set) {
#if CAIRO_VERSION >= CAIRO_VERSION_ENCODE(1, 9, 2)
		cairo_set_operator (cr, CAIRO_OPERATOR_DIFFERENCE);
#endif
		cairo_set_line_width (cr, 5.0);
		cairo_set_antialias (cr, CAIRO_ANTIALIAS_DEFAULT);
		cairo_set_source_rgb (cr, 1.0, 1.0, 1.0);
		cairo_translate (cr, self->priv->preview_image_area.x, self->priv->preview_image_area.y);
		cairo_scale (cr, self->priv->preview_zoom, self->priv->preview_zoom);
		cairo_move_to (cr, self->priv->p1.x, self->priv->p1.y);
		cairo_line_to (cr, self->priv->p2.x, self->priv->p2.y);
		cairo_stroke (cr);
	}

	cairo_restore (cr);
}


static gboolean
gth_image_line_tool_button_release (GthImageViewerTool *base,
				    GdkEventButton     *event)
{
	GthImageLineTool *self = GTH_IMAGE_LINE_TOOL (base);

	g_signal_emit (self, signals[CHANGED], 0);
	return FALSE;
}


static gboolean
gth_image_line_tool_button_press (GthImageViewerTool *base,
				  GdkEventButton     *event)
{
	GthImageLineTool *self = GTH_IMAGE_LINE_TOOL (base);

	if (event->type == GDK_BUTTON_PRESS) {
		self->priv->p1.x = self->priv->p2.x = (event->x - self->priv->preview_image_area.x) / self->priv->preview_zoom;
		self->priv->p1.y = self->priv->p2.y = (event->y - self->priv->preview_image_area.y) / self->priv->preview_zoom;
		self->priv->first_point_set = TRUE;
	}

	return FALSE;
}


static gboolean
gth_image_line_tool_motion_notify (GthImageViewerTool *base,
				   GdkEventMotion     *event)
{
	GthImageLineTool *self = GTH_IMAGE_LINE_TOOL (base);

	if (! self->priv->first_point_set)
		return FALSE;

	self->priv->p2.x = (event->x - self->priv->preview_image_area.x) / self->priv->preview_zoom;
	self->priv->p2.y = (event->y - self->priv->preview_image_area.y) / self->priv->preview_zoom;
	gtk_widget_queue_draw (GTK_WIDGET (self->priv->viewer));

	return FALSE;
}


static void
gth_image_line_tool_image_changed (GthImageViewerTool *base)
{
	update_image_surface (GTH_IMAGE_LINE_TOOL (base));
}


static void
gth_image_line_tool_zoom_changed (GthImageViewerTool *base)
{
	update_image_surface (GTH_IMAGE_LINE_TOOL (base));
}


static void
gth_image_line_tool_instance_init (GthImageLineTool *self)
{
	self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, GTH_TYPE_IMAGE_LINE_TOOL, GthImageLineToolPrivate);
	self->priv->preview_image = NULL;
	self->priv->first_point_set = FALSE;
}


static void
gth_image_line_tool_finalize (GObject *object)
{
	GthImageLineTool *self;

	g_return_if_fail (object != NULL);
	g_return_if_fail (GTH_IS_IMAGE_LINE_TOOL (object));

	self = (GthImageLineTool *) object;
	if (self->priv->preview_image != NULL)
		cairo_surface_destroy (self->priv->preview_image);

	/* Chain up */
	G_OBJECT_CLASS (parent_class)->finalize (object);
}


static void
gth_image_line_tool_class_init (GthImageLineToolClass *class)
{
	GObjectClass *gobject_class;

	parent_class = g_type_class_peek_parent (class);
	g_type_class_add_private (class, sizeof (GthImageLineToolPrivate));

	gobject_class = (GObjectClass*) class;
	gobject_class->finalize = gth_image_line_tool_finalize;

	signals[CHANGED] = g_signal_new ("changed",
					 G_TYPE_FROM_CLASS (class),
					 G_SIGNAL_RUN_LAST,
					 G_STRUCT_OFFSET (GthImageLineToolClass, changed),
					 NULL, NULL,
					 g_cclosure_marshal_VOID__VOID,
					 G_TYPE_NONE,
					 0);
}


static void
gth_image_line_tool_gth_image_tool_interface_init (GthImageViewerToolIface *iface)
{
	iface->set_viewer = gth_image_line_tool_set_viewer;
	iface->unset_viewer = gth_image_line_tool_unset_viewer;
	iface->realize = gth_image_line_tool_realize;
	iface->unrealize = gth_image_line_tool_unrealize;
	iface->size_allocate = gth_image_line_tool_size_allocate;
	iface->map = gth_image_line_tool_map;
	iface->unmap = gth_image_line_tool_unmap;
	iface->expose = gth_image_line_tool_expose;
	iface->button_press = gth_image_line_tool_button_press;
	iface->button_release = gth_image_line_tool_button_release;
	iface->motion_notify = gth_image_line_tool_motion_notify;
	iface->image_changed = gth_image_line_tool_image_changed;
	iface->zoom_changed = gth_image_line_tool_zoom_changed;
}


GType
gth_image_line_tool_get_type (void)
{
	static GType type = 0;

	if (! type) {
		GTypeInfo type_info = {
			sizeof (GthImageLineToolClass),
			NULL,
			NULL,
			(GClassInitFunc) gth_image_line_tool_class_init,
			NULL,
			NULL,
			sizeof (GthImageLineTool),
			0,
			(GInstanceInitFunc) gth_image_line_tool_instance_init
		};
		static const GInterfaceInfo gth_image_tool_info = {
			(GInterfaceInitFunc) gth_image_line_tool_gth_image_tool_interface_init,
			(GInterfaceFinalizeFunc) NULL,
			NULL
		};

		type = g_type_register_static (G_TYPE_OBJECT,
					       "GthImageLineTool",
					       &type_info,
					       0);
		g_type_add_interface_static (type, GTH_TYPE_IMAGE_VIEWER_TOOL, &gth_image_tool_info);
	}

	return type;
}


GthImageViewerTool *
gth_image_line_tool_new (void)
{
	return (GthImageViewerTool *) g_object_new (GTH_TYPE_IMAGE_LINE_TOOL, NULL);
}


void
gth_image_line_tool_get_points (GthImageLineTool *self,
		       	        GdkPoint         *p1,
		       	        GdkPoint         *p2)
{
	*p1 = self->priv->p1;
	*p2 = self->priv->p2;
}
