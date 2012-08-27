/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2012 Free Software Foundation, Inc.
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
#include "gth-color-scale.h"
#include "gth-enum-types.h"


#define _GTK_STYLE_CLASS_COLOR "color"


enum {
        PROP_0,
        PROP_SCALE_TYPE
};


struct _GthColorScalePrivate {
	GthColorScaleType  scale_type;
	cairo_surface_t   *surface;
	int                width;
	int                height;
};


G_DEFINE_TYPE (GthColorScale, gth_color_scale, GTK_TYPE_SCALE)


static void
gth_color_scale_finalize (GObject *object)
{
	GthColorScale *self;

	g_return_if_fail (GTH_IS_COLOR_SCALE (object));

	self = GTH_COLOR_SCALE (object);
	cairo_surface_destroy (self->priv->surface);

	G_OBJECT_CLASS (gth_color_scale_parent_class)->finalize (object);
}


static void
gth_color_scale_set_property (GObject      *object,
			      guint         property_id,
			      const GValue *value,
			      GParamSpec   *pspec)
{
	GthColorScale *self;

	self = GTH_COLOR_SCALE (object);

	switch (property_id) {
	case PROP_SCALE_TYPE:
		self->priv->scale_type = g_value_get_enum (value);
		if (self->priv->scale_type != GTH_COLOR_SCALE_DEFAULT)
			gtk_style_context_add_class (gtk_widget_get_style_context (GTK_WIDGET (self)), _GTK_STYLE_CLASS_COLOR);
		break;
	default:
		break;
	}
}


static void
gth_color_scale_get_property (GObject    *object,
			      guint       property_id,
			      GValue     *value,
			      GParamSpec *pspec)
{
	GthColorScale *self;

	self = GTH_COLOR_SCALE (object);

	switch (property_id) {
	case PROP_SCALE_TYPE:
		g_value_set_enum (value, self->priv->scale_type);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
		break;
	}
}


static void
_gth_color_scale_get_surface_size (GthColorScale *self,
				   int           *x_out,
				   int           *y_out,
				   int           *width_out,
				   int           *height_out)
{
	GtkWidget             *widget = GTK_WIDGET (self);
	int                    focus_line_width;
	int                    focus_padding;
	int                    slider_width;
	int                    slider_height;
	cairo_rectangle_int_t  bounding_box;
	cairo_rectangle_int_t  trough_rect;

	gtk_widget_style_get (widget,
			      "focus-line-width", &focus_line_width,
			      "focus-padding", &focus_padding,
			      "slider-width", &slider_width,
			      "slider-length", &slider_height,
			      NULL);

	bounding_box.width = gtk_widget_get_allocated_width (widget) - 2 * (focus_line_width + focus_padding);
	bounding_box.height = gtk_widget_get_allocated_height (widget) - 2 * (focus_line_width + focus_padding);
	bounding_box.x = focus_line_width + focus_padding;
	bounding_box.y = focus_line_width + focus_padding;

	if (gtk_orientable_get_orientation (GTK_ORIENTABLE (widget)) == GTK_ORIENTATION_HORIZONTAL) {
		trough_rect.x = bounding_box.x + 1;
		trough_rect.width = bounding_box.width - 2;
		trough_rect.height = 3;
		trough_rect.y = bounding_box.y + ((bounding_box.height - trough_rect.height) / 2);
	}
	else {
		trough_rect.y = bounding_box.y + 1;
		trough_rect.height = bounding_box.height - 2;
		trough_rect.width = 3;
		trough_rect.x = bounding_box.x + ((bounding_box.width - trough_rect.width) / 2);
	}

	if (x_out) *x_out = trough_rect.x;
	if (y_out) *y_out = trough_rect.y;
	if (width_out) *width_out = trough_rect.width;
	if (height_out) *height_out = trough_rect.height;
}


static void
_gth_color_scale_update_surface (GthColorScale *self)
{
	int              width;
	int              height;
	cairo_pattern_t *pattern;
	cairo_t         *cr;

	if (! gtk_widget_get_realized (GTK_WIDGET (self)))
		return;

	if (self->priv->scale_type == GTH_COLOR_SCALE_DEFAULT)
		return;

	_gth_color_scale_get_surface_size (self, NULL, NULL, &width, &height);

	if ((self->priv->surface != NULL)
	    && (self->priv->width == width)
	    && (self->priv->height == height))
	{
		return;
	}

	cairo_surface_destroy (self->priv->surface);
	self->priv->surface = NULL;

	pattern = cairo_pattern_create_linear (0.0, 0.0, width, 0.0);

	switch (self->priv->scale_type) {
	case GTH_COLOR_SCALE_DEFAULT:
		g_assert_not_reached ();
		break;

	case GTH_COLOR_SCALE_WHITE_BLACK:
		cairo_pattern_add_color_stop_rgb (pattern, 0.0, 1.0, 1.0, 1.0);
		cairo_pattern_add_color_stop_rgb (pattern, 1.0, 0.0, 0.0, 0.0);
		break;

	case GTH_COLOR_SCALE_BLACK_WHITE:
		cairo_pattern_add_color_stop_rgb (pattern, 0.0, 0.0, 0.0, 0.0);
		cairo_pattern_add_color_stop_rgb (pattern, 1.0, 1.0, 1.0, 1.0);
		break;

	case GTH_COLOR_SCALE_GRAY_BLACK:
		cairo_pattern_add_color_stop_rgb (pattern, 0.0, 0.5, 0.5, 0.5);
		cairo_pattern_add_color_stop_rgb (pattern, 1.0, 0.0, 0.0, 0.0);
		break;

	case GTH_COLOR_SCALE_GRAY_WHITE:
		cairo_pattern_add_color_stop_rgb (pattern, 0.0, 0.5, 0.5, 0.5);
		cairo_pattern_add_color_stop_rgb (pattern, 1.0, 1.0, 1.0, 1.0);
		break;

	case GTH_COLOR_SCALE_CYAN_RED:
		cairo_pattern_add_color_stop_rgb (pattern, 0.0, 0.0, 1.0, 1.0);
		cairo_pattern_add_color_stop_rgb (pattern, 1.0, 1.0, 0.0, 0.0);
		break;

	case GTH_COLOR_SCALE_MAGENTA_GREEN:
		cairo_pattern_add_color_stop_rgb (pattern, 0.0, 1.0, 0.0, 1.0);
		cairo_pattern_add_color_stop_rgb (pattern, 1.0, 0.0, 1.0, 0.0);
		break;

	case GTH_COLOR_SCALE_YELLOW_BLUE:
		cairo_pattern_add_color_stop_rgb (pattern, 0.0, 1.0, 1.0, 0.0);
		cairo_pattern_add_color_stop_rgb (pattern, 1.0, 0.0, 0.0, 1.0);
		break;
	}

	self->priv->surface = cairo_image_surface_create (CAIRO_FORMAT_RGB24, width, height);
	cr = cairo_create (self->priv->surface);
	cairo_set_source (cr, pattern);
	cairo_rectangle (cr, 0, 0, width, height);
	cairo_paint (cr);

	cairo_pattern_destroy (pattern);
	cairo_destroy (cr);
}


static gboolean
gth_color_scale_draw (GtkWidget *widget,
		      cairo_t   *cr)
{
	GthColorScale         *self;
	cairo_rectangle_int_t  surface_rect;
	cairo_pattern_t       *pattern;

	self = GTH_COLOR_SCALE (widget);

	if (self->priv->scale_type == GTH_COLOR_SCALE_DEFAULT) {
		GTK_WIDGET_CLASS (gth_color_scale_parent_class)->draw (widget, cr);
		return FALSE;
	}

	_gth_color_scale_update_surface (self);
	_gth_color_scale_get_surface_size (self,
					   &surface_rect.x,
					   &surface_rect.y,
					   &surface_rect.width,
					   &surface_rect.height);

	cairo_save (cr);
	cairo_translate (cr, surface_rect.x, surface_rect.y);
	cairo_rectangle (cr, 0, 0, surface_rect.width, surface_rect.height);
	pattern = cairo_pattern_create_for_surface (self->priv->surface);
	if ((gtk_orientable_get_orientation (GTK_ORIENTABLE (widget)) == GTK_ORIENTATION_HORIZONTAL)
	    && (gtk_widget_get_direction (widget) == GTK_TEXT_DIR_RTL))
	{
		cairo_matrix_t matrix;

		cairo_matrix_init_scale (&matrix, -1, 1);
		cairo_matrix_translate (&matrix, -surface_rect.width, 0);
		cairo_pattern_set_matrix (pattern, &matrix);
	}
	cairo_set_source (cr, pattern);
	cairo_fill (cr);
	cairo_restore (cr);

	cairo_pattern_destroy (pattern);

	GTK_WIDGET_CLASS (gth_color_scale_parent_class)->draw (widget, cr);

	return FALSE;
}


static void
gth_color_scale_class_init (GthColorScaleClass *class)
{
	GObjectClass   *object_class;
	GtkWidgetClass *widget_class;

	g_type_class_add_private (class, sizeof (GthColorScalePrivate));

	object_class = G_OBJECT_CLASS (class);
	object_class->set_property = gth_color_scale_set_property;
	object_class->get_property = gth_color_scale_get_property;
	object_class->finalize = gth_color_scale_finalize;

	widget_class = GTK_WIDGET_CLASS (class);
	widget_class->draw = gth_color_scale_draw;

	g_object_class_install_property (object_class,
					 PROP_SCALE_TYPE,
					 g_param_spec_enum ("scale-type",
							    "Scale Type",
							    "The type of scale",
							    GTH_TYPE_COLOR_SCALE_TYPE,
							    GTH_COLOR_SCALE_DEFAULT,
							    G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
}


static void
gth_color_scale_init (GthColorScale *self)
{
	self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, GTH_TYPE_COLOR_SCALE, GthColorScalePrivate);
	self->priv->surface = NULL;
	self->priv->width = -1;
	self->priv->height = -1;
}


GtkWidget *
gth_color_scale_new (GtkAdjustment      *adjustment,
		     GthColorScaleType   scale_type)
{
	return g_object_new (GTH_TYPE_COLOR_SCALE,
			     "adjustment", adjustment,
			     "scale-type", scale_type,
			     NULL);
}


/* -- gth_color_scale_label_new -- */


typedef struct {
	GtkWidget *value_label;
	char      *format;
} ScaleData;


static void
scale_data_free (gpointer user_data)
{
	ScaleData *scale_data = user_data;

	g_free (scale_data->format);
	g_free (scale_data);
}


static void
scale_value_changed (GtkAdjustment *adjustment,
		     gpointer       user_data)
{
	ScaleData *scale_data = user_data;
	double     num;
	char      *value;
	char      *markup;

	num = gtk_adjustment_get_value (adjustment);
	value = g_strdup_printf (scale_data->format, num);
	if ((num == 0.0) && ((value[0] == '-') || (value[0] == '+')))
		value[0] = ' ';
	markup = g_strdup_printf ("<small>%s</small>", value);
	gtk_label_set_markup (GTK_LABEL (scale_data->value_label), markup);

	g_free (markup);
	g_free (value);
}


GtkAdjustment *
gth_color_scale_label_new (GtkWidget         *parent_box,
			   GtkLabel          *related_label,
			   GthColorScaleType  scale_type,
			   float              value,
			   float              lower,
			   float              upper,
			   float              step_increment,
			   float              page_increment,
			   const char        *format)
{
	ScaleData     *scale_data;
	GtkAdjustment *adj;
	GtkWidget     *scale;
	GtkWidget     *hbox;

	adj = gtk_adjustment_new (value, lower, upper,
				  step_increment, page_increment,
				  0.0);

	scale_data = g_new (ScaleData, 1);
	scale_data->format = g_strdup (format);
	scale_data->value_label = gtk_label_new ("0");
	g_object_set_data_full (G_OBJECT (adj), "gth-scale-data", scale_data, scale_data_free);

	gtk_label_set_width_chars (GTK_LABEL (scale_data->value_label), 3);
	gtk_misc_set_alignment (GTK_MISC (scale_data->value_label), 1.0, 0.5);
	gtk_widget_show (scale_data->value_label);
	g_signal_connect (adj,
			  "value-changed",
			  G_CALLBACK (scale_value_changed),
			  scale_data);

	scale = gth_color_scale_new (adj, scale_type);
	gtk_scale_set_value_pos (GTK_SCALE (scale), GTK_POS_RIGHT);
	gtk_scale_set_draw_value (GTK_SCALE (scale), FALSE);
	gtk_widget_show (scale);

	hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 5);
	gtk_widget_show (hbox);
	gtk_box_pack_start (GTK_BOX (hbox), scale, TRUE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX (hbox), scale_data->value_label, FALSE, FALSE, 0);

	gtk_box_pack_start (GTK_BOX (parent_box), hbox, TRUE, TRUE, 0);

	if (related_label != NULL)
		gtk_label_set_mnemonic_widget (related_label, scale);

	scale_value_changed (adj, scale_data);

	return adj;
}
