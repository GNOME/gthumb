/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2001-2014 The Free Software Foundation, Inc.
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
#include <cairo/cairo.h>
#include <glib/gi18n.h>
#include "gth-curve.h"
#include "gth-curve-editor.h"
#include "gth-enum-types.h"
#include "gth-points.h"


/* Properties */
enum {
        PROP_0,
        PROP_HISTOGRAM,
        PROP_CURRENT_CHANNEL,
        PROP_SCALE_TYPE
};

enum {
	CHANNEL_COLUMN_NAME,
	CHANNEL_COLUMN_SENSITIVE
};


struct _GthCurveEditorPrivate {
	GthHistogram        *histogram;
	gulong               histogram_changed_event;
	GthHistogramScale    scale_type;
	GthHistogramChannel  current_channel;
	GtkWidget           *view;
	GtkWidget           *linear_histogram_button;
	GtkWidget           *logarithmic_histogram_button;
	GtkWidget           *channel_combo_box;
	GthPoints            points[GTH_HISTOGRAM_N_CHANNELS];
	GthCurve            *curve[GTH_HISTOGRAM_N_CHANNELS];
};


G_DEFINE_TYPE (GthCurveEditor, gth_curve_editor, GTK_TYPE_BOX)


static void
gth_curve_editor_set_property (GObject      *object,
			       guint         property_id,
			       const GValue *value,
			       GParamSpec   *pspec)
{
	GthCurveEditor *self;

        self = GTH_CURVE_EDITOR (object);

	switch (property_id) {
	case PROP_HISTOGRAM:
		gth_curve_editor_set_histogram (self, g_value_get_object (value));
		break;
	case PROP_CURRENT_CHANNEL:
		gth_curve_editor_set_current_channel (self, g_value_get_enum (value));
		break;
	case PROP_SCALE_TYPE:
		gth_curve_editor_set_scale_type (self, g_value_get_enum (value));
		break;
	default:
		break;
	}
}


static void
gth_curve_editor_get_property (GObject    *object,
			       guint       property_id,
			       GValue     *value,
			       GParamSpec *pspec)
{
	GthCurveEditor *self;

        self = GTH_CURVE_EDITOR (object);

	switch (property_id) {
	case PROP_HISTOGRAM:
		g_value_set_object (value, self->priv->histogram);
		break;
	case PROP_CURRENT_CHANNEL:
		g_value_set_int (value, self->priv->current_channel);
		break;
	case PROP_SCALE_TYPE:
		g_value_set_enum (value, self->priv->scale_type);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
		break;
	}
}


static void
gth_curve_editor_finalize (GObject *obj)
{
	GthCurveEditor *self;
	int             c;

	self = GTH_CURVE_EDITOR (obj);

	if (self->priv->histogram_changed_event != 0)
		g_signal_handler_disconnect (self->priv->histogram, self->priv->histogram_changed_event);
	_g_object_unref (self->priv->histogram);

	for (c = 0; c < GTH_HISTOGRAM_N_CHANNELS; c++)
		_g_object_unref (self->priv->curve[c]);

	G_OBJECT_CLASS (gth_curve_editor_parent_class)->finalize (obj);
}


static void
gth_curve_editor_class_init (GthCurveEditorClass *klass)
{
	GObjectClass   *object_class;

	g_type_class_add_private (klass, sizeof (GthCurveEditorPrivate));

	object_class = (GObjectClass*) klass;
	object_class->set_property = gth_curve_editor_set_property;
	object_class->get_property = gth_curve_editor_get_property;
	object_class->finalize = gth_curve_editor_finalize;

	/* properties */

	g_object_class_install_property (object_class,
					 PROP_HISTOGRAM,
					 g_param_spec_object ("histogram",
							      "Histogram",
							      "The histogram to display",
							      GTH_TYPE_HISTOGRAM,
							      G_PARAM_READWRITE));
	g_object_class_install_property (object_class,
					 PROP_CURRENT_CHANNEL,
					 g_param_spec_enum ("current-channel",
							    "Channel",
							    "The channel to display",
							    GTH_TYPE_HISTOGRAM_CHANNEL,
							    GTH_HISTOGRAM_CHANNEL_VALUE,
							    G_PARAM_READWRITE));
	g_object_class_install_property (object_class,
					 PROP_SCALE_TYPE,
					 g_param_spec_enum ("scale-type",
							    "Scale",
							    "The scale type",
							    GTH_TYPE_HISTOGRAM_SCALE,
							    GTH_HISTOGRAM_SCALE_LOGARITHMIC,
							    G_PARAM_READWRITE));
}


#define convert_to_scale(scale_type, value) (((scale_type) == GTH_HISTOGRAM_SCALE_LOGARITHMIC) ? log (value) : (value))
#define HISTOGRAM_TRANSPARENCY 0.25


static void
_cairo_set_source_color_from_channel (cairo_t *cr,
				      int      channel)
{
	switch (channel) {
	case GTH_HISTOGRAM_CHANNEL_VALUE:
	default:
		cairo_set_source_rgba (cr, 0.8, 0.8, 0.8, HISTOGRAM_TRANSPARENCY);
		break;
	case GTH_HISTOGRAM_CHANNEL_RED:
		cairo_set_source_rgba (cr, 0.68, 0.18, 0.19, HISTOGRAM_TRANSPARENCY); /* #af2e31 */
		break;
	case GTH_HISTOGRAM_CHANNEL_GREEN:
		cairo_set_source_rgba (cr, 0.33, 0.78, 0.30, HISTOGRAM_TRANSPARENCY); /* #55c74d */
		break;
	case GTH_HISTOGRAM_CHANNEL_BLUE:
		cairo_set_source_rgba (cr, 0.13, 0.54, 0.8, HISTOGRAM_TRANSPARENCY); /* #238acc */
		break;
	case GTH_HISTOGRAM_CHANNEL_ALPHA:
		cairo_set_source_rgba (cr, 0.5, 0.5, 0.5, HISTOGRAM_TRANSPARENCY);
		break;
	}
}


static void
gth_histogram_paint_channel (GthCurveEditor   *self,
			     GtkStyleContext  *style_context,
			     cairo_t          *cr,
			     int               channel,
			     GtkAllocation    *allocation)
{
	double max;
	double step;
	int    i;

	if (channel > gth_histogram_get_nchannels (self->priv->histogram))
		return;

	_cairo_set_source_color_from_channel (cr, channel);

	cairo_save (cr);
	cairo_set_operator (cr, CAIRO_OPERATOR_OVER);

	max = gth_histogram_get_channel_max (self->priv->histogram, channel);
	if (max > 0.0)
		max = convert_to_scale (self->priv->scale_type, max);
	else
		max = 1.0;

	step = (double) allocation->width / 256.0;
	cairo_set_line_width (cr, 0.5);
	for (i = 0; i <= 255; i++) {
		double value;
		int    y;

		value = gth_histogram_get_value (self->priv->histogram, channel, i);
		y = CLAMP ((int) (allocation->height * convert_to_scale (self->priv->scale_type, value)) / max, 0, allocation->height);

		cairo_rectangle (cr,
				 allocation->x + (i * step) + 0.5,
				 allocation->y + allocation->height - y + 0.5,
				 step,
				 y);
	}
	cairo_fill (cr);
	cairo_restore (cr);
}


static void
gth_histogram_paint_grid (GthCurveEditor *self,
			  GtkStyleContext  *style_context,
			  cairo_t          *cr,
			  GtkAllocation    *allocation)
{
	GdkRGBA color;
	double  grid_step;
	int     i;

	cairo_save (cr);
	gtk_style_context_get_border_color (style_context,
					    gtk_widget_get_state_flags (GTK_WIDGET (self)),
					    &color);
	cairo_set_line_width (cr, 0.5);

	grid_step = (double) allocation->width / 8.0;
	for (i = 0; i <= 8; i++) {
		int ofs = round (grid_step * i);

		cairo_set_source_rgba (cr, color.red, color.green, color.blue, (i == 4) ? 1.0 : 0.5);
		cairo_move_to (cr, allocation->x + ofs + 0.5, allocation->y);
		cairo_line_to (cr, allocation->x + ofs + 0.5, allocation->y + allocation->height);
		cairo_stroke (cr);
	}

	grid_step = (double) allocation->height / 8.0;
	for (i = 0; i <= 8; i++) {
		int ofs = round (grid_step * i);

		cairo_set_source_rgba (cr, color.red, color.green, color.blue, (i == 4) ? 1.0 : 0.5);
		cairo_move_to (cr, allocation->x + 0.5, allocation->y + ofs + 0.5);
		cairo_line_to (cr, allocation->x + allocation->width + 0.5, allocation->y + ofs + 0.5);
		cairo_stroke (cr);
	}

	/* diagonal */
	cairo_set_line_width (cr, 1.0);
	cairo_set_source_rgba (cr, color.red, color.green, color.blue, 0.5);
	cairo_move_to (cr, allocation->x + 0.5, allocation->y + allocation->height + 0.5);
	cairo_line_to (cr, allocation->x + allocation->width+ 0.5, allocation->y + 0.5);
	cairo_stroke (cr);

	cairo_restore (cr);
}


static void
gth_histogram_paint_points (GthCurveEditor  *self,
			    GtkStyleContext *style_context,
			    cairo_t         *cr,
			    GthPoints       *points,
			    GtkAllocation   *allocation)
{
	double x_scale, y_scale;
	int    i;

	cairo_save (cr);
	cairo_set_line_width (cr, 1.0);
	cairo_set_source_rgba (cr, 1.0, 1.0, 1.0, 1.0);

	x_scale = (double) allocation->width / 255;
	y_scale = (double) allocation->height / 255;
	for (i = 0; i < points->n; i++) {
		double x = points->p[i].x;
		double y = points->p[i].y;

		cairo_arc (cr,
			   round (allocation->x + (x * x_scale)),
			   round (allocation->y + allocation->height - (y * y_scale)),
			   3.5,
			   0.0,
			   2 * M_PI);
		cairo_stroke (cr);
	}

	cairo_restore (cr);
}


static void
gth_histogram_paint_curve (GthCurveEditor  *self,
			   GtkStyleContext *style_context,
			   cairo_t         *cr,
			   GthCurve        *spline,
			   GtkAllocation   *allocation)
{
	double x_scale, y_scale;
	int    i;

	cairo_save (cr);
	cairo_set_line_width (cr, 1.0);
	cairo_set_source_rgba (cr, 1.0, 1.0, 1.0, 1.0);

	x_scale = (double) allocation->width / 255;
	y_scale = (double) allocation->height / 255;
	for (i = 0; i < 255; i++) {
		int    j;
		double x, y;

		j = gth_curve_eval (spline, i);
		x = round (allocation->x + (i * x_scale));
		y = round (allocation->y + allocation->height - (j * y_scale));

		if (x == 0)
			cairo_move_to (cr, x, y);
		else
			cairo_line_to (cr, x, y);

	}
	cairo_stroke (cr);
	cairo_restore (cr);
}


static gboolean
curve_editor_draw_cb (GtkWidget *widget,
		      cairo_t   *cr,
		      gpointer   user_data)
{
	GthCurveEditor *self = user_data;
	GtkAllocation     allocation;
	GtkStyleContext  *style_context;

	style_context = gtk_widget_get_style_context (widget);
	gtk_style_context_save (style_context);
	gtk_style_context_add_class (style_context, GTK_STYLE_CLASS_VIEW);
	gtk_style_context_add_class (style_context, "histogram");

	gtk_widget_get_allocation (widget, &allocation);
	gtk_render_background (style_context, cr, 0, 0, allocation.width, allocation.height);

	if ((self->priv->histogram != NULL)
	    && ((int) self->priv->current_channel <= gth_histogram_get_nchannels (self->priv->histogram)))
	{
		GtkBorder     padding;
		GtkAllocation inner_allocation;

		cairo_set_antialias (cr, CAIRO_ANTIALIAS_NONE);

		gtk_style_context_get_padding (style_context, gtk_widget_get_state_flags (widget), &padding);

		padding.left = 5;
		padding.right = 5;
		padding.top = 5;
		padding.bottom = 5;

		inner_allocation.x = padding.left;
		inner_allocation.y = padding.top;
		inner_allocation.width = allocation.width - (padding.right + padding.left) - 1;
		inner_allocation.height = allocation.height - (padding.top + padding.bottom) - 1;

		gth_histogram_paint_channel (self, style_context, cr, self->priv->current_channel, &inner_allocation);
		gth_histogram_paint_grid (self, style_context, cr, &inner_allocation);
		gth_histogram_paint_points (self, style_context, cr, &self->priv->points[self->priv->current_channel], &inner_allocation);
		gth_histogram_paint_curve (self, style_context, cr, self->priv->curve[self->priv->current_channel], &inner_allocation);
	}

	gtk_style_context_restore (style_context);

	return TRUE;
}


static gboolean
curve_editor_scroll_event_cb (GtkWidget      *widget,
				GdkEventScroll *event,
				gpointer        user_data)
{
	GthCurveEditor *self = user_data;
	int               channel = 0;

	if (self->priv->histogram == NULL)
		return FALSE;

	if (event->direction == GDK_SCROLL_UP)
		channel = self->priv->current_channel - 1;
	else if (event->direction == GDK_SCROLL_DOWN)
		channel = self->priv->current_channel + 1;

	if (channel <= gth_histogram_get_nchannels (self->priv->histogram))
		gth_curve_editor_set_current_channel (self, CLAMP (channel, 0, GTH_HISTOGRAM_N_CHANNELS - 1));

	return TRUE;
}


static gboolean
curve_editor_button_press_event_cb (GtkWidget      *widget,
				    GdkEventButton *event,
				    gpointer        user_data)
{
	GthCurveEditor *self = user_data;
	GtkAllocation   allocation;
	int             value;

	gtk_widget_get_allocation (self->priv->view, &allocation);
	value = CLAMP (event->x / allocation.width * 256 + .5, 0, 255);
	/* FIXME */

	return TRUE;
}


static gboolean
curve_editor_button_release_event_cb (GtkWidget      *widget,
					GdkEventButton *event,
					gpointer        user_data)
{
	/* FIXME */

	return TRUE;
}


static gboolean
curve_editor_motion_notify_event_cb (GtkWidget      *widget,
                		       GdkEventMotion *event,
                		       gpointer        user_data)
{
	/* FIXME */

	return TRUE;
}


static void
linear_histogram_button_toggled_cb (GtkToggleButton *button,
				    gpointer   user_data)
{
	GthCurveEditor *self = user_data;

	if (gtk_toggle_button_get_active (button))
		gth_curve_editor_set_scale_type (GTH_CURVE_EDITOR (self), GTH_HISTOGRAM_SCALE_LINEAR);
}


static void
logarithmic_histogram_button_toggled_cb (GtkToggleButton *button,
					 gpointer         user_data)
{
	GthCurveEditor *self = user_data;

	if (gtk_toggle_button_get_active (button))
		gth_curve_editor_set_scale_type (GTH_CURVE_EDITOR (self), GTH_HISTOGRAM_SCALE_LOGARITHMIC);
}


static void
channel_combo_box_changed_cb (GtkComboBox *combo_box,
			      gpointer     user_data)
{
	GthCurveEditor *self = user_data;
	int             n_channel;

	n_channel = gtk_combo_box_get_active (combo_box);
	if (n_channel < GTH_HISTOGRAM_N_CHANNELS)
		gth_curve_editor_set_current_channel (GTH_CURVE_EDITOR (self), n_channel);
}


static void
self_notify_current_channel_cb (GObject    *gobject,
				GParamSpec *pspec,
				gpointer    user_data)
{
	GthCurveEditor *self = user_data;

	gtk_combo_box_set_active (GTK_COMBO_BOX (self->priv->channel_combo_box), self->priv->current_channel);
}


static void
self_notify_scale_type_cb (GObject    *gobject,
			   GParamSpec *pspec,
			   gpointer    user_data)
{
	GthCurveEditor *self = user_data;

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (self->priv->linear_histogram_button), self->priv->scale_type == GTH_HISTOGRAM_SCALE_LINEAR);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (self->priv->logarithmic_histogram_button), self->priv->scale_type == GTH_HISTOGRAM_SCALE_LOGARITHMIC);
}


static void
gth_curve_editor_init (GthCurveEditor *self)
{
	GtkWidget       *topbar_box;
	GtkWidget       *sub_box;
	PangoAttrList   *attr_list;
	GtkWidget       *label;
	GtkWidget       *view_container;
	GtkListStore    *channel_model;
	GtkCellRenderer *renderer;
	GtkTreeIter      iter;
	int              c;

	self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, GTH_TYPE_CURVE_EDITOR, GthCurveEditorPrivate);
	self->priv->histogram = NULL;
	self->priv->current_channel = GTH_HISTOGRAM_CHANNEL_VALUE;
	self->priv->scale_type = GTH_HISTOGRAM_SCALE_LINEAR;
	for (c = 0; c < GTH_HISTOGRAM_N_CHANNELS; c++) {
		gth_points_init (&self->priv->points[c], 0);
		self->priv->curve[c] = NULL;
	}

	gtk_orientable_set_orientation (GTK_ORIENTABLE (self), GTK_ORIENTATION_VERTICAL);
	gtk_box_set_spacing (GTK_BOX (self), 6);
	gtk_widget_set_vexpand (GTK_WIDGET (self), FALSE);

	/* topbar */

	topbar_box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
	gtk_widget_show (topbar_box);

	/* linear / logarithmic buttons */

	sub_box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_widget_show (sub_box);
	gtk_box_pack_end (GTK_BOX (topbar_box), sub_box, FALSE, FALSE, 0);

	self->priv->linear_histogram_button = gtk_toggle_button_new ();
	gtk_widget_set_tooltip_text (self->priv->linear_histogram_button, _("Linear scale"));
	gtk_button_set_relief (GTK_BUTTON (self->priv->linear_histogram_button), GTK_RELIEF_NONE);
	gtk_container_add (GTK_CONTAINER (self->priv->linear_histogram_button), gtk_image_new_from_icon_name ("format-linear-symbolic", GTK_ICON_SIZE_MENU));
	gtk_widget_show_all (self->priv->linear_histogram_button);
	gtk_box_pack_start (GTK_BOX (sub_box), self->priv->linear_histogram_button, FALSE, FALSE, 0);

	g_signal_connect (self->priv->linear_histogram_button,
			  "toggled",
			  G_CALLBACK (linear_histogram_button_toggled_cb),
			  self);

	self->priv->logarithmic_histogram_button = gtk_toggle_button_new ();
	gtk_widget_set_tooltip_text (self->priv->logarithmic_histogram_button, _("Logarithmic scale"));
	gtk_button_set_relief (GTK_BUTTON (self->priv->logarithmic_histogram_button), GTK_RELIEF_NONE);
	gtk_container_add (GTK_CONTAINER (self->priv->logarithmic_histogram_button), gtk_image_new_from_icon_name ("format-logarithmic-symbolic", GTK_ICON_SIZE_MENU));
	gtk_widget_show_all (self->priv->logarithmic_histogram_button);
	gtk_box_pack_start (GTK_BOX (sub_box), self->priv->logarithmic_histogram_button, FALSE, FALSE, 0);

	g_signal_connect (self->priv->logarithmic_histogram_button,
			  "toggled",
			  G_CALLBACK (logarithmic_histogram_button_toggled_cb),
			  self);

	/* channel selector */

	sub_box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
	gtk_widget_show (sub_box);
	gtk_box_pack_start (GTK_BOX (topbar_box), sub_box, FALSE, FALSE, 0);

	attr_list = pango_attr_list_new ();
	pango_attr_list_insert (attr_list, pango_attr_size_new (PANGO_SCALE * 8));

	label = gtk_label_new (_("Channel:"));
	gtk_label_set_attributes (GTK_LABEL (label), attr_list);
	gtk_widget_show (label);
	gtk_box_pack_start (GTK_BOX (sub_box), label, FALSE, FALSE, 0);
	channel_model = gtk_list_store_new (2, G_TYPE_STRING, G_TYPE_BOOLEAN);
	self->priv->channel_combo_box = gtk_combo_box_new_with_model (GTK_TREE_MODEL (channel_model));
	g_object_unref (channel_model);

	renderer = gtk_cell_renderer_text_new ();
	g_object_set (renderer, "attributes", attr_list, NULL);
	gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (self->priv->channel_combo_box),
				    renderer,
				    TRUE);
	gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (self->priv->channel_combo_box),
			    	    	renderer,
			    	    	"text", CHANNEL_COLUMN_NAME,
			    	    	"sensitive", CHANNEL_COLUMN_SENSITIVE,
			    	    	NULL);

	gtk_list_store_append (channel_model, &iter);
	gtk_list_store_set (channel_model, &iter,
			    CHANNEL_COLUMN_NAME, _("Value"),
			    CHANNEL_COLUMN_SENSITIVE, TRUE,
			    -1);
	gtk_list_store_append (channel_model, &iter);
	gtk_list_store_set (channel_model, &iter,
			    CHANNEL_COLUMN_NAME, _("Red"),
			    CHANNEL_COLUMN_SENSITIVE, TRUE,
			    -1);
	gtk_list_store_append (channel_model, &iter);
	gtk_list_store_set (channel_model, &iter,
			    CHANNEL_COLUMN_NAME, _("Green"),
			    CHANNEL_COLUMN_SENSITIVE, TRUE,
			    -1);
	gtk_list_store_append (channel_model, &iter);
	gtk_list_store_set (channel_model, &iter,
			    CHANNEL_COLUMN_NAME, _("Blue"),
			    CHANNEL_COLUMN_SENSITIVE, TRUE,
			    -1);
	gtk_list_store_append (channel_model, &iter);
	gtk_list_store_set (channel_model, &iter,
			    CHANNEL_COLUMN_NAME, _("Alpha"),
			    CHANNEL_COLUMN_SENSITIVE, FALSE,
			    -1);

	gtk_combo_box_set_active (GTK_COMBO_BOX (self->priv->channel_combo_box), self->priv->current_channel);
	gtk_widget_show (self->priv->channel_combo_box);
	gtk_box_pack_start (GTK_BOX (sub_box), self->priv->channel_combo_box, FALSE, FALSE, 0);

	g_signal_connect (self->priv->channel_combo_box,
			  "changed",
			  G_CALLBACK (channel_combo_box_changed_cb),
			  self);

	pango_attr_list_unref (attr_list);

	/* histogram view */

	view_container = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (view_container), GTK_SHADOW_IN);
	gtk_widget_set_vexpand (view_container, TRUE);
	gtk_widget_show (view_container);

	self->priv->view = gtk_drawing_area_new ();
	gtk_widget_add_events (self->priv->view, GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK | GDK_POINTER_MOTION_MASK | GDK_STRUCTURE_MASK);
	gtk_widget_show (self->priv->view);
	gtk_container_add (GTK_CONTAINER (view_container), self->priv->view);

	g_signal_connect (self->priv->view,
			  "draw",
			  G_CALLBACK (curve_editor_draw_cb),
			  self);
	g_signal_connect (self->priv->view,
			  "scroll-event",
			  G_CALLBACK (curve_editor_scroll_event_cb),
			  self);
	g_signal_connect (self->priv->view,
			  "button-press-event",
			  G_CALLBACK (curve_editor_button_press_event_cb),
			  self);
	g_signal_connect (self->priv->view,
			  "button-release-event",
			  G_CALLBACK (curve_editor_button_release_event_cb),
			  self);
	g_signal_connect (self->priv->view,
			  "motion-notify-event",
			  G_CALLBACK (curve_editor_motion_notify_event_cb),
			  self);

	/* pack the widget */

	gtk_box_pack_start (GTK_BOX (self), topbar_box, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (self), view_container, TRUE, TRUE, 0);

	/* update widgets when a property changes */

	g_signal_connect (self,
			  "notify::current-channel",
			  G_CALLBACK (self_notify_current_channel_cb),
			  self);
	g_signal_connect (self,
			  "notify::scale-type",
			  G_CALLBACK (self_notify_scale_type_cb),
			  self);

	/* default values */

	gth_curve_editor_set_scale_type (self, GTH_HISTOGRAM_SCALE_LINEAR);
	gth_curve_editor_set_current_channel (self, GTH_HISTOGRAM_CHANNEL_VALUE);
}


GtkWidget *
gth_curve_editor_new (GthHistogram *histogram)
{
	return (GtkWidget *) g_object_new (GTH_TYPE_CURVE_EDITOR, "histogram", histogram, NULL);
}


static void
update_sensitivity (GthCurveEditor *self)
{
	gboolean     has_alpha;
	GtkTreePath *path;
	GtkTreeIter  iter;

	/* view */

	if ((self->priv->histogram == NULL)
	    || ((int) self->priv->current_channel > gth_histogram_get_nchannels (self->priv->histogram)))
	{
		gtk_widget_set_sensitive (self->priv->view, FALSE);
	}
	else
		gtk_widget_set_sensitive (self->priv->view, TRUE);

	/* channel combobox */

	has_alpha = (self->priv->histogram != NULL) && (gth_histogram_get_nchannels (self->priv->histogram) > 3);
	path = gtk_tree_path_new_from_indices (GTH_HISTOGRAM_CHANNEL_ALPHA, -1);
	if (gtk_tree_model_get_iter (GTK_TREE_MODEL (gtk_combo_box_get_model (GTK_COMBO_BOX (self->priv->channel_combo_box))),
				     &iter,
				     path))
	{
		gtk_list_store_set (GTK_LIST_STORE (gtk_combo_box_get_model (GTK_COMBO_BOX (self->priv->channel_combo_box))),
				    &iter,
				    CHANNEL_COLUMN_SENSITIVE, has_alpha,
				    -1);
	}

	gtk_tree_path_free (path);
}


static void
histogram_changed_cb (GthHistogram *histogram,
		      gpointer      user_data)
{
	GthCurveEditor *self = user_data;

	update_sensitivity (self);
	gtk_widget_queue_draw (GTK_WIDGET (self));
}


void
gth_curve_editor_set_histogram (GthCurveEditor *self,
				  GthHistogram     *histogram)
{
	g_return_if_fail (GTH_IS_CURVE_EDITOR (self));

	if (self->priv->histogram == histogram)
		return;

	if (self->priv->histogram != NULL) {
		g_signal_handler_disconnect (self->priv->histogram, self->priv->histogram_changed_event);
		_g_object_unref (self->priv->histogram);
		self->priv->histogram_changed_event = 0;
		self->priv->histogram = NULL;
	}

	if (histogram != NULL) {
		self->priv->histogram = g_object_ref (histogram);
		self->priv->histogram_changed_event = g_signal_connect (self->priv->histogram, "changed", G_CALLBACK (histogram_changed_cb), self);
	}

	g_object_notify (G_OBJECT (self), "histogram");

	update_sensitivity (self);
}


GthHistogram *
gth_curve_editor_get_histogram (GthCurveEditor *self)
{
	g_return_val_if_fail (GTH_IS_CURVE_EDITOR (self), NULL);
	return self->priv->histogram;
}


void
gth_curve_editor_set_current_channel (GthCurveEditor *self,
					int               n_channel)
{
	g_return_if_fail (GTH_IS_CURVE_EDITOR (self));

	if (n_channel == self->priv->current_channel)
		return;

	self->priv->current_channel = CLAMP (n_channel, 0, GTH_HISTOGRAM_N_CHANNELS);
	g_object_notify (G_OBJECT (self), "current-channel");

	gtk_widget_queue_draw (GTK_WIDGET (self));
}


gint
gth_curve_editor_get_current_channel (GthCurveEditor *self)
{
	g_return_val_if_fail (GTH_IS_CURVE_EDITOR (self), 0);
	return self->priv->current_channel;
}


void
gth_curve_editor_set_scale_type (GthCurveEditor  *self,
				   GthHistogramScale  scale_type)
{
	g_return_if_fail (GTH_IS_CURVE_EDITOR (self));

	self->priv->scale_type = scale_type;
	g_object_notify (G_OBJECT (self), "scale-type");

	gtk_widget_queue_draw (GTK_WIDGET (self));
}


GthHistogramScale
gth_curve_editor_get_scale_type (GthCurveEditor *self)
{
	g_return_val_if_fail (GTH_IS_CURVE_EDITOR (self), 0);
	return self->priv->scale_type;
}


void
gth_curve_editor_set_points (GthCurveEditor	 *self,
			     GthPoints		 *points)
{
	int c;

	for (c = 0; c < GTH_HISTOGRAM_N_CHANNELS; c++) {
		gth_points_dispose (&self->priv->points[c]);
		gth_points_copy (&points[c], &self->priv->points[c]);

		_g_object_unref (self->priv->curve[c]);
		self->priv->curve[c] = gth_cspline_new (&self->priv->points[c]);
	}

	gtk_widget_queue_draw (GTK_WIDGET (self));
}


void
gth_curve_editor_get_points (GthCurveEditor  *self,
			     GthPoints	     *points)
{
	int c;

	for (c = 0; c < GTH_HISTOGRAM_N_CHANNELS; c++) {
		gth_points_dispose (&points[c]);
		gth_points_copy (&self->priv->points[c], &points[c]);
	}
}
