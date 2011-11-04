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
#include "gtk-utils.h"
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


G_DEFINE_TYPE (GthCellRendererThumbnail, gth_cell_renderer_thumbnail, GTK_TYPE_CELL_RENDERER)


static void
gth_cell_renderer_thumbnail_finalize (GObject *object)
{
	GthCellRendererThumbnail *cell_renderer;

	cell_renderer = GTH_CELL_RENDERER_THUMBNAIL (object);

	_g_object_unref (cell_renderer->priv->thumbnail);
	_g_object_unref (cell_renderer->priv->file);

	G_OBJECT_CLASS (gth_cell_renderer_thumbnail_parent_class)->finalize (object);
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
		if (self->priv->thumbnail != NULL)
			g_object_unref (self->priv->thumbnail);
		self->priv->thumbnail = g_value_dup_object (value);
		break;
	case PROP_FILE:
		if (self->priv->file != NULL)
			g_object_unref (self->priv->file);
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


static GtkSizeRequestMode
gth_cell_renderer_thumbnail_get_request_mode (GtkCellRenderer *cell)
{
	return GTK_SIZE_REQUEST_CONSTANT_SIZE;
}


static void
gth_cell_renderer_thumbnail_get_size (GtkCellRenderer    *cell,
				      GtkWidget          *widget,
				      const GdkRectangle *cell_area,
				      int                *x_offset,
				      int                *y_offset,
				      int                *width,
				      int                *height)
{
	GthCellRendererThumbnail *self;
	int                       xpad;
	int                       ypad;
	int                       calc_width;
	int                       calc_height;

	self = (GthCellRendererThumbnail *) cell;

	gtk_cell_renderer_get_padding (cell, &xpad, &ypad);
	calc_width  = (int) (xpad * 2) + (THUMBNAIL_BORDER * 2) + self->priv->size;
	calc_height = (int) (ypad * 2) + (THUMBNAIL_BORDER * 2) + self->priv->size;
	if (width != NULL) *width = calc_width;
	if (height != NULL) *height = calc_height;

	if (cell_area != NULL) {
		float xalign;
		float yalign;

		gtk_cell_renderer_get_alignment (cell, &xalign, &yalign);

		if (x_offset != NULL) *x_offset = MAX (xalign * (cell_area->width - calc_width), 0);
		if (y_offset != NULL) *y_offset = MAX (yalign * (cell_area->height - calc_height), 0);
	}
	else {
		if (x_offset) *x_offset = 0;
		if (y_offset) *y_offset = 0;
	}
}


static void
gth_cell_renderer_thumbnail_render (GtkCellRenderer      *cell,
				    cairo_t              *cr,
				    GtkWidget            *widget,
				    const GdkRectangle   *background_area,
				    const GdkRectangle   *cell_area,
				    GtkCellRendererState  cell_state)
{
	GthCellRendererThumbnail *self;
	GtkStyleContext          *style_context;
	GtkStateFlags             state;
	cairo_rectangle_int_t     thumb_rect;
	cairo_rectangle_int_t     draw_rect;
	cairo_rectangle_int_t     image_rect;
	GdkPixbuf                *pixbuf;
	int                       xpad;
	int                       ypad;
	int                       border;

	self = (GthCellRendererThumbnail *) cell;

	gth_cell_renderer_thumbnail_get_size (cell, widget, cell_area,
					      &thumb_rect.x,
					      &thumb_rect.y,
					      &thumb_rect.width,
					      &thumb_rect.height);

	gtk_cell_renderer_get_padding (cell, &xpad, &ypad);

	thumb_rect.x += cell_area->x + xpad;
	thumb_rect.y += cell_area->y + ypad;
	thumb_rect.width  -= xpad * 2;
	thumb_rect.height -= ypad * 2;

	if (! gdk_rectangle_intersect (cell_area, &thumb_rect, &draw_rect))
		return;

	pixbuf = self->priv->thumbnail;
	if (pixbuf == NULL)
		return;

	g_object_ref (pixbuf);

	image_rect.width = gdk_pixbuf_get_width (pixbuf);
	image_rect.height = gdk_pixbuf_get_height (pixbuf);
	image_rect.x = thumb_rect.x + (thumb_rect.width - image_rect.width) * .5;
	image_rect.y = thumb_rect.y + (thumb_rect.height - image_rect.height) * .5;

	style_context = gtk_widget_get_style_context (widget);
	gtk_style_context_save (style_context);
	gtk_style_context_remove_class (style_context, GTK_STYLE_CLASS_VIEW);
	gtk_style_context_add_class (style_context, GTK_STYLE_CLASS_CELL);
	state = cell_state;

	if (self->priv->is_icon || ((image_rect.width < self->priv->size) && (image_rect.height < self->priv->size))) {
		GdkRGBA background_color;

		/* use a gray rounded box for icons or when the original size
		 * is smaller than the thumbnail size... */

		gtk_style_context_get_background_color (style_context, state, &background_color);
		gdk_cairo_set_source_rgba (cr, &background_color);

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

		cairo_rectangle_int_t frame_rect;
		GdkRGBA               background_color;
		GdkRGBA               lighter_color;
		GdkRGBA               darker_color;

		gdk_rgba_parse (&background_color, "#edeceb");
		gtk_style_context_get_background_color (style_context, state, &background_color);
		_gdk_rgba_darker (&background_color, &lighter_color);
		_gdk_rgba_darker (&lighter_color, &darker_color);

		if (self->priv->fixed_size && (self->priv->file != NULL) && _g_mime_type_is_image (gth_file_data_get_mime_type (self->priv->file))) {
			frame_rect.width = self->priv->size; /*image_rect.width*/
			frame_rect.height = self->priv->size; /*image_rect.height*/
			frame_rect.x = thumb_rect.x + (thumb_rect.width - frame_rect.width) * .5; /*cell_area->x + xpad + THUMBNAIL_BORDER - 1;*/
			frame_rect.y = thumb_rect.y + (thumb_rect.height - frame_rect.height) * .5; /*cell_area->y + ypad + THUMBNAIL_BORDER - 1;*/

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

			gdk_cairo_set_source_rgba (cr, &darker_color);
			_cairo_draw_rounded_box (cr,
						 frame_rect.x - border + 2,
						 frame_rect.y - border + 2,
						 frame_rect.width + (border * 2),
						 frame_rect.height + (border * 2),
						 1);
			cairo_fill (cr);

			/* the outer frame */

			gdk_cairo_set_source_rgba (cr, &background_color);
			_cairo_draw_rounded_box (cr,
						 frame_rect.x - border,
						 frame_rect.y - border,
						 frame_rect.width + (border * 2),
						 frame_rect.height + (border * 2),
						 1);
			cairo_fill_preserve (cr);

			if (state == GTK_STATE_FLAG_SELECTED)
				gdk_cairo_set_source_rgba (cr, &darker_color);
			else
				gdk_cairo_set_source_rgba (cr, &lighter_color);

			cairo_stroke (cr);

			/* the inner frame */

			cairo_rectangle (cr,
					 image_rect.x - 1,
					 image_rect.y - 1,
					 image_rect.width,
					 image_rect.height);
			cairo_stroke (cr);
		}
		else {
			/* the drop shadow */

			gdk_cairo_set_source_rgba (cr, &darker_color);
			cairo_rectangle (cr,
					 frame_rect.x - border + 2,
					 frame_rect.y - border + 2,
					 frame_rect.width + (border * 2),
					 frame_rect.height + (border * 2));
			cairo_fill (cr);

			gdk_cairo_set_source_rgba (cr, &darker_color);
			cairo_rectangle (cr,
					 frame_rect.x - border,
					 frame_rect.y - border,
					 frame_rect.width + (border * 2),
					 frame_rect.height + (border * 2));
			cairo_stroke (cr);
		}

		cairo_identity_matrix (cr);
	}

	gdk_cairo_set_source_pixbuf (cr, pixbuf, image_rect.x, image_rect.y);
	cairo_rectangle (cr, image_rect.x, image_rect.y, image_rect.width, image_rect.height);
	cairo_fill (cr);

	if (! self->priv->checked || ((cell_state & GTK_CELL_RENDERER_SELECTED) != 0)) {
		GdkRGBA color;

		state = gtk_cell_renderer_get_state (cell, widget, cell_state);
		gtk_style_context_get_background_color (style_context, state, &color);
		cairo_set_source_rgba (cr, color.red, color.green, color.blue, 0.33);
		cairo_rectangle (cr, thumb_rect.x, thumb_rect.y, thumb_rect.width, thumb_rect.height);
		cairo_fill (cr);
	}

	gtk_style_context_restore (style_context);
	g_object_unref (pixbuf);
}


static void
gth_cell_renderer_thumbnail_class_init (GthCellRendererThumbnailClass *klass)
{
	GObjectClass         *object_class;
	GtkCellRendererClass *cell_renderer;

	g_type_class_add_private (klass, sizeof (GthCellRendererThumbnailPrivate));

	object_class = G_OBJECT_CLASS (klass);
	object_class->finalize = gth_cell_renderer_thumbnail_finalize;
	object_class->get_property = gth_cell_renderer_thumbnail_get_property;
	object_class->set_property = gth_cell_renderer_thumbnail_set_property;

	cell_renderer = GTK_CELL_RENDERER_CLASS (klass);
	cell_renderer->get_request_mode = gth_cell_renderer_thumbnail_get_request_mode;
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
	self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, GTH_TYPE_CELL_RENDERER_THUMBNAIL, GthCellRendererThumbnailPrivate);
	self->priv->size = DEFAULT_THUMBNAIL_SIZE;
	self->priv->is_icon = FALSE;
	self->priv->thumbnail = NULL;
	self->priv->file = NULL;
	self->priv->checked = TRUE;
	self->priv->selected = FALSE;
	self->priv->fixed_size = TRUE;
}


GtkCellRenderer *
gth_cell_renderer_thumbnail_new (void)
{
	return g_object_new (GTH_TYPE_CELL_RENDERER_THUMBNAIL, NULL);
}
