/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2008 Free Software Foundation, Inc.
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
#include "gth-file-data.h"
#include "glib-utils.h"
#include "gth-cell-renderer-thumbnail.h"


#define DEFAULT_THUMBNAIL_SIZE 112
#define MAX_THUMBNAIL_SIZE 320
#define THUMBNAIL_X_BORDER 8
#define THUMBNAIL_Y_BORDER 8


/* properties */
enum {
	PROP_0,
	PROP_SIZE,
	PROP_IS_ICON,
	PROP_THUMBNAIL,
	PROP_CHECKED,
	PROP_SELECTED,
	PROP_FILE
};


struct _GthCellRendererThumbnailPrivate
{
	int          size;
	gboolean     is_icon;
	GdkPixbuf   *thumbnail;
	GthFileData *file;
	gboolean     checked;
	gboolean     selected;
};


static gpointer parent_class = NULL;


static void
gth_cell_renderer_thumbnail_finalize (GObject *object)
{
	GthCellRendererThumbnail *cell_renderer;

	cell_renderer = GTH_CELL_RENDERER_THUMBNAIL (object);

	if (cell_renderer->priv != NULL) {
		_g_object_unref (cell_renderer->priv->thumbnail);
		_g_object_unref (cell_renderer->priv->file);
		g_free (cell_renderer->priv);
		cell_renderer->priv = NULL;
	}

	G_OBJECT_CLASS (parent_class)->finalize (object);
}


static void
gth_cell_renderer_thumbnail_get_property (GObject    *object,
					  guint       param_id,
					  GValue     *value,
					  GParamSpec *pspec)
{
	GthCellRendererThumbnail *self;

	self = GTH_CELL_RENDERER_THUMBNAIL (object);

	switch (param_id) {
	case PROP_SIZE:
		g_value_set_int (value, self->priv->size);
		break;
	case PROP_IS_ICON:
		g_value_set_boolean (value, self->priv->is_icon);
		break;
	case PROP_THUMBNAIL:
		g_value_set_object (value, self->priv->thumbnail);
		break;
	case PROP_FILE:
		g_value_set_object (value, self->priv->file);
		break;
	case PROP_CHECKED:
		g_value_set_boolean (value, self->priv->checked);
		break;
	case PROP_SELECTED:
		g_value_set_boolean (value, self->priv->selected);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
		break;
	}
}


static void
gth_cell_renderer_thumbnail_set_property (GObject      *object,
					  guint         param_id,
					  const GValue *value,
					  GParamSpec   *pspec)
{
	GthCellRendererThumbnail *self;

	self = GTH_CELL_RENDERER_THUMBNAIL (object);

	switch (param_id) {
	case PROP_SIZE:
		self->priv->size = g_value_get_int (value);
		break;
	case PROP_IS_ICON:
		self->priv->is_icon = g_value_get_boolean (value);
		break;
	case PROP_THUMBNAIL:
		self->priv->thumbnail = g_value_dup_object (value);
		break;
	case PROP_FILE:
		self->priv->file = g_value_dup_object (value);
		break;
	case PROP_CHECKED:
		self->priv->checked = g_value_get_boolean (value);
		break;
	case PROP_SELECTED:
		self->priv->selected = g_value_get_boolean (value);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
		break;
	}
}


static void
gth_cell_renderer_thumbnail_get_size (GtkCellRenderer *cell,
				      GtkWidget       *widget,
				      GdkRectangle    *cell_area,
				      int             *x_offset,
				      int             *y_offset,
				      int             *width,
				      int             *height)
{
	GthCellRendererThumbnail *self;
	int image_width;
	int image_height;
	int calc_width;
  	int calc_height;

  	self = GTH_CELL_RENDERER_THUMBNAIL (cell);

  	if (self->priv->thumbnail != NULL) {
  		image_width = gdk_pixbuf_get_width (self->priv->thumbnail);
  		image_height = gdk_pixbuf_get_height (self->priv->thumbnail);
  	}
  	else {
  		image_width = 0;
  		image_height = 0;
  	}

	if (self->priv->is_icon || (self->priv->thumbnail == NULL) || ((image_width < self->priv->size) && (image_height < self->priv->size))) {
		calc_width  = (int) (cell->xpad * 2) + (THUMBNAIL_X_BORDER * 2) + self->priv->size;
		calc_height = (int) (cell->ypad * 2) + (THUMBNAIL_Y_BORDER * 2) + self->priv->size;
	}
	else {
		calc_width  = (int) (cell->xpad * 2) + (THUMBNAIL_X_BORDER * 2) + image_width;
		calc_height = (int) (cell->ypad * 2) + (THUMBNAIL_Y_BORDER * 2) + image_height;
	}

	if (width != NULL)
		*width = calc_width;

	if (height != NULL)
		*height = calc_height;

	if (cell_area != NULL) {
		if (x_offset != NULL) {
			*x_offset = cell->xalign * (cell_area->width - calc_width);
			*x_offset = MAX (*x_offset, 0);
		}

		if (y_offset != NULL) {
      			*y_offset = cell->yalign * (cell_area->height - calc_height);
      			*y_offset = MAX (*y_offset, 0);
    		}
  	}
}


/* From gtkcellrendererpixbuf.c
 * Copyright (C) 2000  Red Hat, Inc.,  Jonathan Blandford <jrb@redhat.com>
 *
 * modified for gthumb */
static GdkPixbuf *
create_colorized_pixbuf (GdkPixbuf *src,
			 GdkColor  *new_color,
			 gdouble    alpha)
{
	gint i, j;
	gint width, height, has_alpha, src_row_stride, dst_row_stride;
	gint red_value, green_value, blue_value;
	guchar *target_pixels;
	guchar *original_pixels;
	guchar *pixsrc;
	guchar *pixdest;
	GdkPixbuf *dest;

	red_value = new_color->red / 255.0;
	green_value = new_color->green / 255.0;
	blue_value = new_color->blue / 255.0;

	dest = gdk_pixbuf_new (gdk_pixbuf_get_colorspace (src),
			       TRUE /*gdk_pixbuf_get_has_alpha (src)*/,
			       gdk_pixbuf_get_bits_per_sample (src),
			       gdk_pixbuf_get_width (src),
			       gdk_pixbuf_get_height (src));

	has_alpha = gdk_pixbuf_get_has_alpha (src);
	width = gdk_pixbuf_get_width (src);
	height = gdk_pixbuf_get_height (src);
	src_row_stride = gdk_pixbuf_get_rowstride (src);
	dst_row_stride = gdk_pixbuf_get_rowstride (dest);
	target_pixels = gdk_pixbuf_get_pixels (dest);
	original_pixels = gdk_pixbuf_get_pixels (src);

	for (i = 0; i < height; i++) {
		pixdest = target_pixels + i*dst_row_stride;
		pixsrc = original_pixels + i*src_row_stride;
		for (j = 0; j < width; j++) {
			*pixdest++ = (*pixsrc++ * red_value) >> 8;
			*pixdest++ = (*pixsrc++ * green_value) >> 8;
			*pixdest++ = (*pixsrc++ * blue_value) >> 8;
			if (has_alpha)
				*pixdest++ = (*pixsrc++ * alpha);
			else
				*pixdest++ = (255 * alpha);
		}
	}

	return dest;
}


static void
gth_cell_renderer_thumbnail_render (GtkCellRenderer      *cell,
				    GdkWindow            *window,
				    GtkWidget            *widget,
				    GdkRectangle         *background_area,
				    GdkRectangle         *cell_area,
				    GdkRectangle         *expose_area,
				    GtkCellRendererState  flags)
{
	GthCellRendererThumbnail *self;
	GtkStyle                 *style;
	GtkStateType              state;
	GdkRectangle              thumb_rect;
	GdkRectangle              draw_rect;
	GdkRectangle              image_rect;
	cairo_t                  *cr;
	cairo_path_t             *cr_path;
	GdkPixbuf                *pixbuf;
	GdkPixbuf                *colorized = NULL;
	int                       B;

	self = GTH_CELL_RENDERER_THUMBNAIL (cell);

 	gth_cell_renderer_thumbnail_get_size (cell, widget, cell_area,
 					      &thumb_rect.x,
 					      &thumb_rect.y,
 					      &thumb_rect.width,
 					      &thumb_rect.height);

	pixbuf = self->priv->thumbnail;
	if (pixbuf == NULL)
		return;

	thumb_rect.x += cell_area->x + cell->xpad;
  	thumb_rect.y += cell_area->y + cell->ypad;
  	thumb_rect.width  -= cell->xpad * 2;
  	thumb_rect.height -= cell->ypad * 2;

	if (! gdk_rectangle_intersect (cell_area, &thumb_rect, &draw_rect)
	    || ! gdk_rectangle_intersect (expose_area, &thumb_rect, &draw_rect))
	{
		return;
	}

	cr = gdk_cairo_create (window);

    	image_rect.width = gdk_pixbuf_get_width (pixbuf);
	image_rect.height = gdk_pixbuf_get_height (pixbuf);
	image_rect.x = thumb_rect.x + (thumb_rect.width - image_rect.width) * .5;
	image_rect.y = thumb_rect.y + (thumb_rect.height - image_rect.height) * .5;

	style = gtk_widget_get_style (widget);

  	if ((flags & GTK_CELL_RENDERER_SELECTED) == GTK_CELL_RENDERER_SELECTED)
  		state = GTK_WIDGET_HAS_FOCUS (widget) ? GTK_STATE_SELECTED : GTK_STATE_ACTIVE;
  	else
  		state = ((flags & GTK_CELL_RENDERER_FOCUSED) == GTK_CELL_RENDERER_FOCUSED) ? GTK_STATE_ACTIVE : GTK_STATE_NORMAL;

	if (self->priv->is_icon || (state != GTK_STATE_NORMAL) || ((image_rect.width < self->priv->size) && (image_rect.height < self->priv->size))) {
		int R = 7;

		if (state == GTK_STATE_NORMAL)
			gdk_cairo_set_source_color (cr, &widget->style->bg[state]);
		else
			gdk_cairo_set_source_color (cr, &widget->style->base[state]);

		cairo_move_to (cr, thumb_rect.x, thumb_rect.y + R);
		cairo_arc (cr, thumb_rect.x + R, thumb_rect.y + R, R, 1.0 * M_PI, 1.5 * M_PI);
		cairo_rel_line_to (cr, thumb_rect.width - (R * 2), 0);
		cairo_arc (cr, thumb_rect.x + thumb_rect.width - R, thumb_rect.y + R, R, 1.5 * M_PI, 2.0 * M_PI);
		cairo_rel_line_to (cr, 0, thumb_rect.height - (R * 2));
		cairo_arc (cr, thumb_rect.x + thumb_rect.width - R, thumb_rect.y + thumb_rect.height - R, R, 0.0 * M_PI, 0.5 * M_PI);
		cairo_rel_line_to (cr, - (thumb_rect.width - (R * 2)), 0);
		cairo_arc (cr, thumb_rect.x + R, thumb_rect.y + thumb_rect.height - R, R, 0.5 * M_PI, 1.0 * M_PI);
		cairo_close_path (cr);
		cr_path = cairo_copy_path (cr);
		cairo_fill (cr);
	}

	if (! self->priv->is_icon && ! ((image_rect.width < self->priv->size) && (image_rect.height < self->priv->size))) {
		GdkRectangle frame_rect;

		if (! _g_mime_type_is_image (gth_file_data_get_mime_type (self->priv->file))) {
			B = 1;
			frame_rect = image_rect;
		}
		else {
			B = 4;

			/*frame_rect = thumb_rect;
			frame_rect.width = self->priv->size;
			frame_rect.height = self->priv->size;
			frame_rect.x = thumb_rect.x + (thumb_rect.width - frame_rect.width) * .5;
			frame_rect.y = thumb_rect.y + (thumb_rect.height - frame_rect.height) * .5;*/

			frame_rect = image_rect;
		}

	  	gdk_draw_rectangle (GDK_DRAWABLE (window), style->dark_gc[state], FALSE,
  				    frame_rect.x - B,
  				    frame_rect.y - B,
  				    frame_rect.width + (B * 2) - 1,
				    frame_rect.height + (B * 2) - 1);

  		gdk_draw_line (GDK_DRAWABLE (window), style->dark_gc[state],
  			       frame_rect.x - (B / 2),
  			       frame_rect.y + frame_rect.height + B,
  			       frame_rect.x + frame_rect.width + B,
  			       frame_rect.y + frame_rect.height + B);
  		gdk_draw_line (GDK_DRAWABLE (window), style->dark_gc[state],
  			       frame_rect.x + frame_rect.width + B,
  			       frame_rect.y + frame_rect.height + B,
  			       frame_rect.x + frame_rect.width + B,
  			       frame_rect.y - (B / 2));

		gdk_draw_line (GDK_DRAWABLE (window), style->mid_gc[state],
  			       frame_rect.x - (B / 2) + 1,
  			       frame_rect.y + frame_rect.height + B + 1,
  			       frame_rect.x + frame_rect.width + B + 1,
  			       frame_rect.y + frame_rect.height + B + 1);
  		gdk_draw_line (GDK_DRAWABLE (window), style->mid_gc[state],
  			       frame_rect.x + frame_rect.width + B + 1,
  			       frame_rect.y + frame_rect.height + B + 1,
  			       frame_rect.x + frame_rect.width + B + 1,
  			       frame_rect.y - (B / 2) + 1);

  		/*gdk_draw_rectangle (GDK_DRAWABLE (window), style->light_gc[state], TRUE,
  				    frame_rect.x - (B - 1),
  				    frame_rect.y - (B - 1),
  				    frame_rect.width + ((B - 1) * 2),
  				    frame_rect.height + ((B - 1) * 2));*/
  		/*gdk_draw_rectangle (GDK_DRAWABLE (window), style->mid_gc[state], FALSE,
  				    image_rect.x - 1,
  				    image_rect.y - 1,
  				    image_rect.width + 1,
  				    image_rect.height + 1);*/
	}

  	if (! self->priv->checked || ((flags & (GTK_CELL_RENDERER_SELECTED | GTK_CELL_RENDERER_PRELIT)) != 0)) {
		colorized = create_colorized_pixbuf (pixbuf, &style->base[state], self->priv->checked ? 1.0 : 0.33);
		pixbuf = colorized;
	}

  	gdk_draw_pixbuf (GDK_DRAWABLE (window),
  			 NULL,
  			 pixbuf,
  			 0, 0,
  			 image_rect.x,
  			 image_rect.y,
  			 image_rect.width,
  			 image_rect.height,
  			 GDK_RGB_DITHER_NORMAL,
  			 0, 0);

  	/*if (! self->priv->checked) {
  		cairo_set_source_rgba (cr, 1.0, 0.0, 0.0, 0.5);
		cairo_set_line_width (cr, 6.0);

		cairo_move_to (cr, thumb_rect.x, thumb_rect.y);
		cairo_line_to (cr, thumb_rect.x + thumb_rect.width, thumb_rect.y + thumb_rect.height);
		cairo_stroke (cr);
		cairo_move_to (cr, thumb_rect.x + thumb_rect.width, thumb_rect.y);
		cairo_line_to (cr, thumb_rect.x, thumb_rect.y + thumb_rect.height);
		cairo_stroke (cr);
  	}*/

  	cairo_destroy (cr);

  	if (colorized != NULL)
		g_object_unref (colorized);
}


static void
gth_cell_renderer_thumbnail_class_init (GthCellRendererThumbnailClass *klass)
{
	GObjectClass         *object_class;
	GtkCellRendererClass *cell_renderer;

	parent_class = g_type_class_peek_parent (klass);
	object_class = G_OBJECT_CLASS (klass);
	cell_renderer = GTK_CELL_RENDERER_CLASS (klass);

	object_class->finalize = gth_cell_renderer_thumbnail_finalize;
	object_class->get_property = gth_cell_renderer_thumbnail_get_property;
  	object_class->set_property = gth_cell_renderer_thumbnail_set_property;
	cell_renderer->get_size = gth_cell_renderer_thumbnail_get_size;
	cell_renderer->render = gth_cell_renderer_thumbnail_render;

	/* properties */

	g_object_class_install_property (object_class,
					 PROP_SIZE,
					 g_param_spec_int ("size",
					 		   "Size",
							   "The thumbnail size",
							   0,
							   MAX_THUMBNAIL_SIZE,
							   DEFAULT_THUMBNAIL_SIZE,
							   G_PARAM_READWRITE));
	g_object_class_install_property (object_class,
					 PROP_IS_ICON,
					 g_param_spec_boolean ("is_icon",
					 		       "Is icon",
							       "Whether the image is a file icon",
							       TRUE,
							       G_PARAM_READWRITE));
	g_object_class_install_property (object_class,
					 PROP_THUMBNAIL,
					 g_param_spec_object ("thumbnail",
					 		      "Thumbnail",
							      "The image thumbnail",
							      GDK_TYPE_PIXBUF,
							      G_PARAM_READWRITE));
	g_object_class_install_property (object_class,
					 PROP_FILE,
					 g_param_spec_object ("file",
					 		      "File",
							      "The file data",
							      GTH_TYPE_FILE_DATA,
							      G_PARAM_READWRITE));
	g_object_class_install_property (object_class,
					 PROP_CHECKED,
					 g_param_spec_boolean ("checked",
					 		       "Checked",
							       "Whether the image has been checked by the user",
							       TRUE,
							       G_PARAM_READWRITE));
	g_object_class_install_property (object_class,
					 PROP_SELECTED,
					 g_param_spec_boolean ("selected",
					 		       "Selected",
							       "Whether the image has been selected by the user",
							       FALSE,
							       G_PARAM_READWRITE));
}


static void
gth_cell_renderer_thumbnail_init (GthCellRendererThumbnail *self)
{
	self->priv = g_new0 (GthCellRendererThumbnailPrivate, 1);
	self->priv->size = DEFAULT_THUMBNAIL_SIZE;
	self->priv->checked = TRUE;
}


GType
gth_cell_renderer_thumbnail_get_type (void)
{
	static GType type = 0;

	if (type == 0) {
		GTypeInfo type_info = {
			sizeof (GthCellRendererThumbnailClass),
			NULL,
			NULL,
			(GClassInitFunc) gth_cell_renderer_thumbnail_class_init,
			NULL,
			NULL,
			sizeof (GthCellRendererThumbnail),
			0,
			(GInstanceInitFunc) gth_cell_renderer_thumbnail_init,
			NULL
		};
		type = g_type_register_static (GTK_TYPE_CELL_RENDERER,
					       "GthCellRendererThumbnail",
					       &type_info,
					       0);
	}
	return type;
}


GtkCellRenderer *
gth_cell_renderer_thumbnail_new (void)
{
	return g_object_new (GTH_TYPE_CELL_RENDERER_THUMBNAIL, NULL);
}
