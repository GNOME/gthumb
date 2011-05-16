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
#include "cairo-rotate.h"
#include "gth-image-rotator.h"


#define MIN4(a,b,c,d) MIN(MIN((a),(b)),MIN((c),(d)))
#define MAX4(a,b,c,d) MAX(MAX((a),(b)),MAX((c),(d)))


enum {
	CHANGED,
	CENTER_CHANGED,
	LAST_SIGNAL
};


static guint signals[LAST_SIGNAL] = { 0 };
static gpointer parent_class = NULL;


struct _GthImageRotatorPrivate {
	GthImageViewer     *viewer;

	/* options */

	GdkPoint            center;
	double              angle;
	cairo_color_t       background_color;
	gboolean            enable_crop;
	GdkRectangle        crop_region;
	GthGridType         grid_type;
	GthTransformResize  resize;

	/* utility variables */

	int                 original_width;
	int                 original_height;
	double              preview_zoom;
	cairo_surface_t    *preview_image;
	GdkRectangle        preview_image_area;
	GdkPoint            preview_center;
	GdkRectangle        clip_area;
	cairo_matrix_t      matrix;
};


static void
gth_image_rotator_realize (GthImageViewerTool *base)
{
	/* void */
}


static void
gth_image_rotator_unrealize (GthImageViewerTool *base)
{
	/* void */
}


static void
_cairo_matrix_transform_point (cairo_matrix_t *matrix,
			       double          x,
			       double          y,
			       double         *tx,
			       double         *ty)
{
	*tx = x;
	*ty = y;
	cairo_matrix_transform_point (matrix, tx, ty);
}


static void
gth_transform_resize (cairo_matrix_t     *matrix,
		      GthTransformResize  resize,
		      GdkRectangle       *original,
		      GdkRectangle       *boundary)
{
	int x1, y1, x2, y2;

	x1 = original->x;
	y1 = original->y;
	x2 = original->x + original->width;
	y2 = original->y + original->height;

	switch (resize) {
	case GTH_TRANSFORM_RESIZE_CLIP:
		/* keep the original size */
		break;

	case GTH_TRANSFORM_RESIZE_BOUNDING_BOX:
	case GTH_TRANSFORM_RESIZE_CROP:
		{
			double dx1, dx2, dx3, dx4;
			double dy1, dy2, dy3, dy4;

			_cairo_matrix_transform_point (matrix, x1, y1, &dx1, &dy1);
			_cairo_matrix_transform_point (matrix, x2, y1, &dx2, &dy2);
			_cairo_matrix_transform_point (matrix, x1, y2, &dx3, &dy3);
			_cairo_matrix_transform_point (matrix, x2, y2, &dx4, &dy4);

			x1 = (int) floor (MIN4 (dx1, dx2, dx3, dx4));
			y1 = (int) floor (MIN4 (dy1, dy2, dy3, dy4));
			x2 = (int) ceil  (MAX4 (dx1, dx2, dx3, dx4));
			y2 = (int) ceil  (MAX4 (dy1, dy2, dy3, dy4));
			break;
		}
	}

	boundary->x = x1;
	boundary->y = y1;
	boundary->width = x2 - x1;
	boundary->height = y2 - y1;
}


static void
_gth_image_rotator_update_tranformation_matrix (GthImageRotator *self)
{
	int tx, ty;

	self->priv->preview_center.x = self->priv->center.x * self->priv->preview_zoom;
	self->priv->preview_center.y = self->priv->center.y * self->priv->preview_zoom;

	tx = self->priv->preview_image_area.x + self->priv->preview_center.x;
	ty = self->priv->preview_image_area.y + self->priv->preview_center.y;

	cairo_matrix_init_identity (&self->priv->matrix);
	cairo_matrix_translate (&self->priv->matrix, tx, ty);
	cairo_matrix_rotate (&self->priv->matrix, self->priv->angle);
	cairo_matrix_translate (&self->priv->matrix, -tx, -ty);

	gth_transform_resize (&self->priv->matrix,
			      self->priv->resize,
			      &self->priv->preview_image_area,
			      &self->priv->clip_area);
}


static void
update_image_surface (GthImageRotator *self)
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

	_gth_image_rotator_update_tranformation_matrix (self);
}


static void
gth_image_rotator_size_allocate (GthImageViewerTool *base,
				 GtkAllocation      *allocation)
{
	update_image_surface (GTH_IMAGE_ROTATOR (base));
}


static void
gth_image_rotator_map (GthImageViewerTool *base)
{
	/* void */
}


static void
gth_image_rotator_unmap (GthImageViewerTool *base)
{
	/* void */
}


static void
paint_image (GthImageRotator *self,
	     cairo_t         *cr)
{
	cairo_save (cr);

	cairo_set_matrix (cr, &self->priv->matrix);
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
paint_darker_background (GthImageRotator *self,
			 GdkEventExpose  *event,
			 cairo_t         *cr)
{
	GdkRectangle crop_region;

	cairo_save (cr);
	cairo_set_source_rgba (cr, 0.0, 0.0, 0.0, 0.5);

	/* the crop_region is not zoomed the clip_area is already zoomed */

	crop_region = self->priv->crop_region;
	crop_region.x = crop_region.x * self->priv->preview_zoom;
	crop_region.y = crop_region.y * self->priv->preview_zoom;
	crop_region.width = crop_region.width * self->priv->preview_zoom;
	crop_region.height = crop_region.height * self->priv->preview_zoom;

	/* left side */

	cairo_rectangle (cr,
			 self->priv->clip_area.x,
			 self->priv->clip_area.y,
			 crop_region.x,
			 self->priv->clip_area.height);

	/* right side */

	cairo_rectangle (cr,
			 self->priv->clip_area.x + crop_region.x + crop_region.width,
			 self->priv->clip_area.y,
			 self->priv->clip_area.width - crop_region.x - crop_region.width,
			 self->priv->clip_area.height);

	/* top */

	cairo_rectangle (cr,
			 self->priv->clip_area.x,
			 self->priv->clip_area.y,
			 self->priv->clip_area.width,
			 crop_region.y);

	/* bottom */

	cairo_rectangle (cr,
			 self->priv->clip_area.x,
			 self->priv->clip_area.y + crop_region.y + crop_region.height,
			 self->priv->clip_area.width,
			 self->priv->clip_area.height - crop_region.y - crop_region.height);

	cairo_fill (cr);
	cairo_restore (cr);
}


static void
paint_grid (GthImageRotator *self,
	    GdkEventExpose  *event,
	    cairo_t         *cr)
{
	GdkRectangle grid;

	cairo_save (cr);

	cairo_scale (cr, self->priv->preview_zoom, self->priv->preview_zoom);

	grid = self->priv->crop_region;
	grid.x += self->priv->clip_area.x / self->priv->preview_zoom;
	grid.y += self->priv->clip_area.y / self->priv->preview_zoom;
	_cairo_paint_grid (cr, &grid, self->priv->grid_type);

	cairo_restore (cr);
}


static void
gth_image_rotator_expose (GthImageViewerTool *base,
			  GdkEventExpose     *event,
			  cairo_t            *cr)
{
	GthImageRotator *self = GTH_IMAGE_ROTATOR (base);
	GtkStyle        *style;
	GtkAllocation    allocation;

	cairo_save (cr);

  	cairo_rectangle (cr,
  			 event->area.x,
  			 event->area.y,
  			 event->area.width,
  			 event->area.height);
  	cairo_clip (cr);

  	/* background */

	style = gtk_widget_get_style (GTK_WIDGET (self->priv->viewer));
	gtk_widget_get_allocation (GTK_WIDGET (self->priv->viewer), &allocation);
	gdk_cairo_set_source_color (cr, &style->bg[GTK_STATE_NORMAL]);
	cairo_rectangle (cr, 0, 0, allocation.width, allocation.height);
	cairo_fill (cr);

	if (self->priv->preview_image == NULL)
		return;

	/* clip box */

  	cairo_rectangle (cr,
  			 self->priv->clip_area.x,
  			 self->priv->clip_area.y,
  			 self->priv->clip_area.width,
  			 self->priv->clip_area.height);
  	cairo_clip_preserve (cr);
  	cairo_set_source_rgba (cr,
  			       self->priv->background_color.r,
  			       self->priv->background_color.g,
  			       self->priv->background_color.b,
  			       self->priv->background_color.a);
  	cairo_fill (cr);

	paint_image (self, cr);
	if (self->priv->enable_crop) {
		paint_darker_background (self, event, cr);
		paint_grid (self, event, cr);
	}

	cairo_restore (cr);
}


static gboolean
gth_image_rotator_button_release (GthImageViewerTool *base,
				  GdkEventButton     *event)
{
	/* FIXME */

	return FALSE;
}


static gboolean
gth_image_rotator_button_press (GthImageViewerTool *base,
				GdkEventButton     *event)
{
	GthImageRotator *self = GTH_IMAGE_ROTATOR (base);

	if (event->type == GDK_2BUTTON_PRESS) {
		double x, y;

		x = (event->x - self->priv->preview_image_area.x) / self->priv->preview_zoom;
		y = (event->y - self->priv->preview_image_area.y) / self->priv->preview_zoom;
		gth_image_rotator_set_center (self, (int) x, (int) y);
	}

	return FALSE;
}


static gboolean
gth_image_rotator_motion_notify (GthImageViewerTool *base,
				 GdkEventMotion     *event)
{
	/* FIXME */

	return FALSE;
}


static void
gth_image_rotator_image_changed (GthImageViewerTool *base)
{
	update_image_surface (GTH_IMAGE_ROTATOR (base));
}


static void
gth_image_rotator_zoom_changed (GthImageViewerTool *base)
{
	update_image_surface (GTH_IMAGE_ROTATOR (base));
}


static void
gth_image_rotator_instance_init (GthImageRotator *self)
{
	self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, GTH_TYPE_IMAGE_ROTATOR, GthImageRotatorPrivate);
	self->priv->preview_image = NULL;
	self->priv->grid_type = GTH_GRID_NONE;
	self->priv->resize = GTH_TRANSFORM_RESIZE_BOUNDING_BOX;
	self->priv->background_color.r = 0.0;
	self->priv->background_color.g = 0.0;
	self->priv->background_color.b = 0.0;
	self->priv->background_color.a = 1.0;
	self->priv->enable_crop = FALSE;
	self->priv->crop_region.x = 0;
	self->priv->crop_region.y = 0;
	self->priv->crop_region.width = 0;
	self->priv->crop_region.height = 0;
}


static void
gth_image_rotator_finalize (GObject *object)
{
	GthImageRotator *self;

	g_return_if_fail (object != NULL);
	g_return_if_fail (GTH_IS_IMAGE_ROTATOR (object));

	self = (GthImageRotator *) object;
	if (self->priv->preview_image != NULL)
		cairo_surface_destroy (self->priv->preview_image);

	/* Chain up */
	G_OBJECT_CLASS (parent_class)->finalize (object);
}


static void
gth_image_rotator_class_init (GthImageRotatorClass *class)
{
	GObjectClass *gobject_class;

	parent_class = g_type_class_peek_parent (class);
	g_type_class_add_private (class, sizeof (GthImageRotatorPrivate));

	gobject_class = (GObjectClass*) class;
	gobject_class->finalize = gth_image_rotator_finalize;

	signals[CHANGED] = g_signal_new ("changed",
					 G_TYPE_FROM_CLASS (class),
					 G_SIGNAL_RUN_LAST,
					 G_STRUCT_OFFSET (GthImageRotatorClass, changed),
					 NULL, NULL,
					 g_cclosure_marshal_VOID__VOID,
					 G_TYPE_NONE,
					 0);
	signals[CENTER_CHANGED] = g_signal_new ("center-changed",
					 	G_TYPE_FROM_CLASS (class),
					 	G_SIGNAL_RUN_LAST,
					 	G_STRUCT_OFFSET (GthImageRotatorClass, center_changed),
					 	NULL, NULL,
					 	g_cclosure_marshal_VOID__VOID,
					 	G_TYPE_NONE,
					 	0);
}


static void
gth_image_rotator_gth_image_tool_interface_init (GthImageViewerToolIface *iface)
{
	iface->realize = gth_image_rotator_realize;
	iface->unrealize = gth_image_rotator_unrealize;
	iface->size_allocate = gth_image_rotator_size_allocate;
	iface->map = gth_image_rotator_map;
	iface->unmap = gth_image_rotator_unmap;
	iface->expose = gth_image_rotator_expose;
	iface->button_press = gth_image_rotator_button_press;
	iface->button_release = gth_image_rotator_button_release;
	iface->motion_notify = gth_image_rotator_motion_notify;
	iface->image_changed = gth_image_rotator_image_changed;
	iface->zoom_changed = gth_image_rotator_zoom_changed;
}


GType
gth_image_rotator_get_type (void)
{
	static GType type = 0;

	if (! type) {
		GTypeInfo type_info = {
			sizeof (GthImageRotatorClass),
			NULL,
			NULL,
			(GClassInitFunc) gth_image_rotator_class_init,
			NULL,
			NULL,
			sizeof (GthImageRotator),
			0,
			(GInstanceInitFunc) gth_image_rotator_instance_init
		};
		static const GInterfaceInfo gth_image_tool_info = {
			(GInterfaceInitFunc) gth_image_rotator_gth_image_tool_interface_init,
			(GInterfaceFinalizeFunc) NULL,
			NULL
		};

		type = g_type_register_static (G_TYPE_OBJECT,
					       "GthImageRotator",
					       &type_info,
					       0);
		g_type_add_interface_static (type, GTH_TYPE_IMAGE_VIEWER_TOOL, &gth_image_tool_info);
	}

	return type;
}


GthImageViewerTool *
gth_image_rotator_new (GthImageViewer *viewer)
{
	GthImageRotator *rotator;

	rotator = g_object_new (GTH_TYPE_IMAGE_ROTATOR, NULL);
	rotator->priv->viewer = viewer;
	rotator->priv->angle = 0.0;

	return GTH_IMAGE_VIEWER_TOOL (rotator);
}


void
gth_image_rotator_set_grid_type (GthImageRotator *self,
                                 GthGridType      grid_type)
{
        if (grid_type == self->priv->grid_type)
                return;

        self->priv->grid_type = grid_type;
        gtk_widget_queue_draw (GTK_WIDGET (self->priv->viewer));
        /*g_signal_emit (G_OBJECT (self),
                       signals[GRID_VISIBILITY_CHANGED],
                       0);*/
}


GthGridType
gth_image_rotator_get_grid_type (GthImageRotator *self)
{
        return self->priv->grid_type;
}


void
gth_image_rotator_set_center (GthImageRotator *self,
			      int              x,
			      int              y)
{
	self->priv->center.x = x;
	self->priv->center.y = y;
	_gth_image_rotator_update_tranformation_matrix (self);
	gtk_widget_queue_draw (GTK_WIDGET (self->priv->viewer));

	g_signal_emit (self, signals[CENTER_CHANGED], 0);
}


void
gth_image_rotator_get_center (GthImageRotator *self,
			      int             *x,
			      int             *y)
{
	*x = self->priv->center.x;
	*y = self->priv->center.y;
}


void
gth_image_rotator_set_angle (GthImageRotator *self,
			     double           angle)
{
	double radiants;

	radiants = angle * M_PI / 180.0;
	if (radiants == self->priv->angle)
		return;
	self->priv->angle = radiants;
	_gth_image_rotator_update_tranformation_matrix (self);
	gtk_widget_queue_draw (GTK_WIDGET (self->priv->viewer));

	g_signal_emit (self, signals[CHANGED], 0);
}


double
gth_image_rotator_get_angle (GthImageRotator *self)
{
	return self->priv->angle;
}


void
gth_image_rotator_set_resize (GthImageRotator    *self,
			      GthTransformResize  resize)
{
	self->priv->resize = resize;
	_gth_image_rotator_update_tranformation_matrix (self);
	gtk_widget_queue_draw (GTK_WIDGET (self->priv->viewer));

	g_signal_emit (self, signals[CHANGED], 0);
}


GthTransformResize
gth_image_rotator_get_resize (GthImageRotator *self)
{
	return self->priv->resize;
}


void
gth_image_rotator_set_crop_region (GthImageRotator *self,
				   GdkRectangle    *region)
{
	self->priv->enable_crop = (region != NULL);
	if (region != NULL)
		self->priv->crop_region = *region;

	gtk_widget_queue_draw (GTK_WIDGET (self->priv->viewer));

	g_signal_emit (self, signals[CHANGED], 0);
}


void
gth_image_rotator_set_background (GthImageRotator *self,
			          cairo_color_t   *color)
{
	self->priv->background_color = *color;
	gtk_widget_queue_draw (GTK_WIDGET (self->priv->viewer));

	g_signal_emit (self, signals[CHANGED], 0);
}


void
gth_image_rotator_get_background (GthImageRotator *self,
			          cairo_color_t   *color)
{
	*color = self->priv->background_color;
}


static cairo_surface_t *
gth_image_rotator_get_result_fast (GthImageRotator *self)
{
	double           tx, ty;
	cairo_matrix_t   matrix;
	GdkRectangle     image_area;
	GdkRectangle     clip_area;
	cairo_surface_t *output;
	cairo_t         *cr;

	/* compute the transformation matrix and the clip area */

	tx = self->priv->center.x;
	ty = self->priv->center.y;
	cairo_matrix_init_identity (&matrix);
	cairo_matrix_translate (&matrix, tx, ty);
	cairo_matrix_rotate (&matrix, self->priv->angle);
	cairo_matrix_translate (&matrix, -tx, -ty);

	image_area.x = 0.0;
	image_area.y = 0.0;
	image_area.width = self->priv->original_width;
	image_area.height = self->priv->original_height;

	gth_transform_resize (&matrix,
			      self->priv->resize,
			      &image_area,
			      &clip_area);

	if (! self->priv->enable_crop) {
		self->priv->crop_region.x = 0;
		self->priv->crop_region.y = 0;
		self->priv->crop_region.width = clip_area.width;
		self->priv->crop_region.height = clip_area.height;
	}

	output = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, self->priv->crop_region.width, self->priv->crop_region.height);

	/* set the device offset to make the clip area start from the top left
	 * corner of the output image */

	cairo_surface_set_device_offset (output,
					 - clip_area.x - self->priv->crop_region.x,
					 - clip_area.y - self->priv->crop_region.y);
	cr = cairo_create (output);

	/* paint the background */

  	cairo_rectangle (cr, clip_area.x, clip_area.y, clip_area.width, clip_area.height);
  	cairo_clip_preserve (cr);
  	cairo_set_source_rgba (cr,
  			       self->priv->background_color.r,
  			       self->priv->background_color.g,
  			       self->priv->background_color.b,
  			       self->priv->background_color.a);
  	cairo_fill (cr);

  	/* paint the rotated image */

	cairo_set_matrix (cr, &matrix);
	cairo_set_source_surface (cr, gth_image_viewer_get_current_image (GTH_IMAGE_VIEWER (self->priv->viewer)), image_area.x, image_area.y);
  	cairo_rectangle (cr, image_area.x, image_area.y, image_area.width, image_area.height);
  	cairo_fill (cr);
  	cairo_surface_flush (output);
  	cairo_surface_set_device_offset (output, 0.0, 0.0);

	cairo_destroy (cr);

	return output;
}


static cairo_surface_t *
gth_image_rotator_get_result_high_quality (GthImageRotator *self)
{
	cairo_surface_t *rotated;
	cairo_surface_t *result;

	rotated = _cairo_image_surface_rotate (gth_image_viewer_get_current_image (GTH_IMAGE_VIEWER (self->priv->viewer)),
					       self->priv->angle / G_PI * 180.0,
					       TRUE,
					       &self->priv->background_color);

	switch (self->priv->resize) {
	case GTH_TRANSFORM_RESIZE_BOUNDING_BOX:
		self->priv->crop_region.x = 0;
		self->priv->crop_region.y = 0;
		self->priv->crop_region.width = cairo_image_surface_get_width (rotated);
		self->priv->crop_region.height = cairo_image_surface_get_height (rotated);
		break;

	case GTH_TRANSFORM_RESIZE_CLIP:
		self->priv->crop_region.x = (cairo_image_surface_get_width (rotated) - self->priv->original_width) / 2;
		self->priv->crop_region.y = (cairo_image_surface_get_height (rotated) - self->priv->original_height) / 2;
		self->priv->crop_region.width = self->priv->original_width;
		self->priv->crop_region.height = self->priv->original_height;
		break;

	case GTH_TRANSFORM_RESIZE_CROP:
		/* set by the user */
		break;
	}

	result = _cairo_image_surface_copy_subsurface (rotated,
						       self->priv->crop_region.x,
						       self->priv->crop_region.y,
						       self->priv->crop_region.width,
						       self->priv->crop_region.height);

	cairo_surface_destroy (rotated);

	return result;
}


cairo_surface_t *
gth_image_rotator_get_result (GthImageRotator *self,
			      gboolean         high_quality)
{
	if (high_quality)
		return gth_image_rotator_get_result_high_quality (self);
	else
		return gth_image_rotator_get_result_fast (self);
}
