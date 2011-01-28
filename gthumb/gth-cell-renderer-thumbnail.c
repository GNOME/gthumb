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
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <config.h>
#include "cairo-utils.h"
#include "gth-file-data.h"
#include "glib-utils.h"
#include "gth-cell-renderer-thumbnail.h"
#include "pixbuf-utils.h"

#define DEFAULT_THUMBNAIL_SIZE 128
#define MAX_THUMBNAIL_SIZE 320
#define THUMBNAIL_BORDER 8
#define THUMBNAIL_BORDER 8


/* properties */
enum {
	PROP_0,
	PROP_SIZE,
	PROP_IS_ICON,
	PROP_THUMBNAIL,
	PROP_CHECKED,
	PROP_SELECTED,
	PROP_FILE,
	PROP_FIXED_SIZE
};


struct _GthCellRendererThumbnailPrivate
{
	int          size;
	gboolean     is_icon;
	GdkPixbuf   *thumbnail;
	GthFileData *file;
	gboolean     checked;
	gboolean     selected;
	gboolean     fixed_size;
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

	self = (GthCellRendererThumbnail *) object;

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
	case PROP_FIXED_SIZE:
		g_value_set_boolean (value, self->priv->fixed_size);
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

	self = (GthCellRendererThumbnail *) object;

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
	case PROP_FIXED_SIZE:
		self->priv->fixed_size = g_value_get_boolean (value);
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
	int   image_width;
	int   image_height;
	int   calc_width;
	int   calc_height;
	int   xpad;
	int   ypad;

	self = (GthCellRendererThumbnail *) cell;

	if (self->priv->thumbnail != NULL) {
		image_width = gdk_pixbuf_get_width (self->priv->thumbnail);
		image_height = gdk_pixbuf_get_height (self->priv->thumbnail);
	}
	else {
		image_width = 0;
		image_height = 0;
	}

	gtk_cell_renderer_get_padding (cell, &xpad, &ypad);

	if (self->priv->is_icon
	    || self->priv->fixed_size
	    || (self->priv->thumbnail == NULL)
	    || ((image_width < self->priv->size) && (image_height < self->priv->size)))
	{
		calc_width  = (int) (xpad * 2) + (THUMBNAIL_BORDER * 2) + self->priv->size;
		calc_height = (int) (ypad * 2) + (THUMBNAIL_BORDER * 2) + self->priv->size;
	}
	else {
		calc_width  = (int) (xpad * 2) + (THUMBNAIL_BORDER * 2) + self->priv->size;
		calc_height = (int) (ypad * 2) + (THUMBNAIL_BORDER * 2) + self->priv->size;
	}

	if (width != NULL)
		*width = calc_width;

	if (height != NULL)
		*height = calc_height;

	if (cell_area != NULL) {
		float xalign;
		float yalign;

		gtk_cell_renderer_get_alignment (cell, &xalign, &yalign);

		if (x_offset != NULL) {
			*x_offset = xalign * (cell_area->width - calc_width);
			*x_offset = MAX (*x_offset, 0);
		}

		if (y_offset != NULL) {
			 *y_offset = yalign * (cell_area->height - calc_height);
			 *y_offset = MAX (*y_offset, 0);
		}
	}
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
	GdkPixbuf                *pixbuf;
	int                       xpad;
	int                       ypad;
	GdkPixbuf                *colorized = NULL;
	int                       border;

	self = (GthCellRendererThumbnail *) cell;

	gth_cell_renderer_thumbnail_get_size (cell, widget, cell_area,
					      &thumb_rect.x,
					      &thumb_rect.y,
					      &thumb_rect.width,
					      &thumb_rect.height);

	pixbuf = self->priv->thumbnail;
	if (pixbuf == NULL)
		return;

	gtk_cell_renderer_get_padding (cell, &xpad, &ypad);

	thumb_rect.x += cell_area->x + xpad;
	thumb_rect.y += cell_area->y + ypad;
	thumb_rect.width  -= xpad * 2;
	thumb_rect.height -= ypad * 2;

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
		state = gtk_widget_has_focus (widget) ? GTK_STATE_SELECTED : GTK_STATE_ACTIVE;
	else
		state = ((flags & GTK_CELL_RENDERER_FOCUSED) == GTK_CELL_RENDERER_FOCUSED) ? GTK_STATE_ACTIVE : GTK_STATE_NORMAL;

	if (self->priv->is_icon || ((image_rect.width < self->priv->size) && (image_rect.height < self->priv->size))) {
		GtkStyle *style;

		/* use a gray rounded box for icons or when the original size
		 * is smaller than the thumbnail size... */

		style = gtk_widget_get_style (widget);
		if (state == GTK_STATE_NORMAL)
			gdk_cairo_set_source_color (cr, &style->bg[state]);
		else
			gdk_cairo_set_source_color (cr, &style->base[state]);

		_cairo_draw_rounded_box (cr,
					 thumb_rect.x,
					 thumb_rect.y,
					 thumb_rect.width,
					 thumb_rect.height,
					 2);
		cairo_fill (cr);
	}
	else {

		/* ...else draw a frame with a drop-shadow effect */

		GdkRectangle frame_rect;

		if (state == GTK_STATE_ACTIVE)
			state = GTK_STATE_SELECTED;

		if (self->priv->fixed_size && _g_mime_type_is_image (gth_file_data_get_mime_type (self->priv->file))) {
			frame_rect.width = self->priv->size; /*image_rect.width*/
			frame_rect.height = self->priv->size; /*image_rect.height*/
			frame_rect.x = cell_area->x + xpad + THUMBNAIL_BORDER - 1;
			frame_rect.y = cell_area->y + ypad + THUMBNAIL_BORDER - 1;

			border = 6;
		}
		else {
			frame_rect = image_rect;
			border = 1;
			frame_rect.width -= 1;
			frame_rect.height -= 1;
		}

		cairo_translate (cr, 0.5, 0.5);
		cairo_set_line_width (cr, 0.5);
		cairo_set_antialias (cr, CAIRO_ANTIALIAS_NONE);

		if (border > 1) {
			/* the drop shadow */

			gdk_cairo_set_source_color (cr, &style->dark[state]);
			_cairo_draw_rounded_box (cr,
						 frame_rect.x - border + 2,
						 frame_rect.y - border + 2,
						 frame_rect.width + (border * 2),
						 frame_rect.height + (border * 2),
						 1);
			cairo_fill (cr);

			/* the outer frame */

			if (state == GTK_STATE_NORMAL)
				gdk_cairo_set_source_color (cr, &style->bg[state]);
			else
				gdk_cairo_set_source_color (cr, &style->base[state]);
			_cairo_draw_rounded_box (cr,
						 frame_rect.x - border,
						 frame_rect.y - border,
						 frame_rect.width + (border * 2),
						 frame_rect.height + (border * 2),
						 1);
			cairo_fill_preserve (cr);

			gdk_cairo_set_source_color (cr, &style->mid[state]);
			cairo_stroke (cr);

			/* the inner frame */

			gdk_cairo_set_source_color (cr, &style->mid[state]);
			cairo_rectangle (cr,
					 image_rect.x - 1,
					 image_rect.y - 1,
					 image_rect.width,
					 image_rect.height);
			cairo_stroke (cr);
		}
		else {
			/* the drop shadow */

			gdk_cairo_set_source_color (cr, &style->dark[state]);
			cairo_rectangle (cr,
					 frame_rect.x - border + 2,
					 frame_rect.y - border + 2,
					 frame_rect.width + (border * 2),
					 frame_rect.height + (border * 2));
			cairo_fill (cr);

			gdk_cairo_set_source_color (cr, &style->dark[state]);
			cairo_rectangle (cr,
					 frame_rect.x - border,
					 frame_rect.y - border,
					 frame_rect.width + (border * 2),
					 frame_rect.height + (border * 2));
			cairo_stroke (cr);
		}

		cairo_identity_matrix (cr);
	}

	if (! self->priv->checked || ((flags & (GTK_CELL_RENDERER_SELECTED | GTK_CELL_RENDERER_PRELIT)) != 0)) {
		colorized = _gdk_pixbuf_colorize (pixbuf, &style->base[state], self->priv->checked ? 1.0 : 0.33);
		pixbuf = colorized;
	}

	gdk_cairo_set_source_pixbuf (cr, pixbuf, image_rect.x, image_rect.y);
	cairo_rectangle (cr, image_rect.x, image_rect.y, image_rect.width, image_rect.height);
	cairo_fill (cr);

	_g_object_unref (colorized);
	cairo_destroy (cr);
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
	g_object_class_install_property (object_class,
					 PROP_FIXED_SIZE,
					 g_param_spec_boolean ("fixed-size",
							       "Fixed size",
							       "Whether to always use the maximum size for width and height",
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
