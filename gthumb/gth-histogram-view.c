/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2001-2009 The Free Software Foundation, Inc.
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
#include <math.h>
#include <cairo/cairo.h>
#include "glib-utils.h"
#include "gth-histogram-view.h"


/* Properties */
enum {
        PROP_0,
        PROP_HISTOGRAM
};


static gpointer gth_histogram_view_parent_class = NULL;


struct _GthHistogramViewPrivate {
	GthHistogram      *histogram;
	gulong             histogram_changed_event;
	GthHistogramMode   display_mode;
	GthHistogramScale  scale_type;
	int                current_channel;
	guchar             selection_start;
	guchar             selection_end;
};


static void
gth_histogram_set_property (GObject      *object,
			    guint         property_id,
			    const GValue *value,
			    GParamSpec   *pspec)
{
	GthHistogramView *self;

        self = GTH_HISTOGRAM_VIEW (object);

	switch (property_id) {
	case PROP_HISTOGRAM:
		gth_histogram_view_set_histogram (self, g_value_get_object (value));
		break;
	default:
		break;
	}
}


static void
gth_histogram_get_property (GObject    *object,
		            guint       property_id,
		            GValue     *value,
		            GParamSpec *pspec)
{
	GthHistogramView *self;

        self = GTH_HISTOGRAM_VIEW (object);

	switch (property_id) {
	case PROP_HISTOGRAM:
		g_value_set_object (value, self->priv->histogram);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
		break;
	}
}


static void
gth_histogram_view_finalize (GObject *obj)
{
	GthHistogramView *self;

	self = GTH_HISTOGRAM_VIEW (obj);

	gth_histogram_view_set_histogram (self, NULL);

	G_OBJECT_CLASS (gth_histogram_view_parent_class)->finalize (obj);
}


static double
convert_to_scale (GthHistogramScale scale_type,
		  double            value)
{
	switch (scale_type) {
	case GTH_HISTOGRAM_SCALE_LINEAR:
		return value;
	case GTH_HISTOGRAM_SCALE_LOGARITHMIC:
		return log (value);
	}

	return 0.0;
}


static void
gth_histogram_paint_channel (GthHistogramView *self,
			     cairo_t          *cr,
			     int               channel,
			     gboolean          black_mask)
{
	GtkWidget     *widget = GTK_WIDGET (self);
	GtkAllocation  allocation;
	double         max;
	double         step;
	int            i;

	if (channel > 3)
		return;
	if ((self->priv->display_mode == GTH_HISTOGRAM_MODE_ALL_CHANNELS) && (channel == 0))
		return;

	gtk_widget_get_allocation (widget, &allocation);

	switch (channel) {
	case 0:
	default:
		cairo_set_source_rgba (cr, 0.0, 0.0, 0.0, 1.0);
		break;
	case 1:
		cairo_set_source_rgba (cr, 1.0, 0.0, 0.0, 1.0);
		break;
	case 2:
		cairo_set_source_rgba (cr, 0.0, 1.0, 0.0, 1.0);
		break;
	case 3:
		cairo_set_source_rgba (cr, 0.0, 0.0, 1.0, 1.0);
		break;
	case 4:
		cairo_set_source_rgba (cr, 0.5, 0.5, 0.5, 1.0);
		break;
	}

	if (black_mask) {
		cairo_set_source_rgba (cr, 0.0, 0.0, 0.0, 1.0);
		cairo_set_operator (cr, CAIRO_OPERATOR_OVER);
	}
	else if (self->priv->display_mode == GTH_HISTOGRAM_MODE_ALL_CHANNELS)
		cairo_set_operator (cr, CAIRO_OPERATOR_ADD);
	else
		cairo_set_operator (cr, CAIRO_OPERATOR_OVER);

	max = gth_histogram_get_channel_max (self->priv->histogram, channel);
	if (max > 0.0)
		max = convert_to_scale (self->priv->scale_type, max);
	else
		max = 1.0;

	step = allocation.width / 256.0;
	cairo_set_line_width (cr, 0.5);
	for (i = 0; i < 256; i++) {
		double value;
		int    y;

		value = gth_histogram_get_value (self->priv->histogram, channel, i);
		y = (int) (allocation.height * convert_to_scale (self->priv->scale_type, value)) / max;

		/*cairo_new_path (cr);
		cairo_move_to (cr, i * step + (step / 2) + 0.5, h - y);
		cairo_line_to (cr, i * step + (step / 2) + 0.5, h);
		cairo_close_path (cr);
		cairo_stroke (cr);*/

		cairo_rectangle (cr, (i * step) + 0.5, allocation.height - y, 1 + step, allocation.height);
	}
	cairo_fill (cr);
}


static void
gth_histogram_paint_grid (GthHistogramView *self,
			  cairo_t          *cr)
{
	GtkWidget     *widget = GTK_WIDGET (self);
	GtkAllocation  allocation;
	GtkStyle      *style;
	int            i;

	cairo_set_operator (cr, CAIRO_OPERATOR_OVER);

	style = gtk_widget_get_style (widget);
	gdk_cairo_set_source_color (cr, &style->dark[gtk_widget_get_state (widget)]);

	gtk_widget_get_allocation (widget, &allocation);
	cairo_rectangle (cr, 0, 0, allocation.width, allocation.height);
	cairo_stroke (cr);

	cairo_set_line_width (cr, 0.5);
	for (i = 1; i <= 4; i++) {
		int x;

		x = (i * 64) * ((float) allocation.width / 256);

		/*cairo_new_path (cr);*/
		cairo_move_to (cr, x + 0.5, 0);
		cairo_line_to (cr, x + 0.5, allocation.height);
		/*cairo_close_path (cr);*/
	}
	cairo_stroke (cr);
}


static gboolean
gth_histogram_view_expose_event (GtkWidget      *widget,
				 GdkEventExpose *event)
{
	GthHistogramView *self;
	GtkAllocation     allocation;
	GtkStyle         *style;
	cairo_t          *cr;

	self = GTH_HISTOGRAM_VIEW (widget);

	gtk_widget_get_allocation (widget, &allocation);
	style = gtk_widget_get_style (widget);

	cr = gdk_cairo_create (gtk_widget_get_window (widget));
	gdk_cairo_set_source_color (cr, &style->base[gtk_widget_get_state (widget)]);
	cairo_rectangle (cr, 0, 0, allocation.width, allocation.height);
	cairo_fill (cr);

	cairo_set_line_width (cr, 2.0);

	if ((self->priv->histogram == NULL)
	    || (self->priv->current_channel > gth_histogram_get_nchannels (self->priv->histogram)))
	{
		/* draw an x if no histogram is set */
		cairo_new_path (cr);
		cairo_move_to (cr, 0, 0);
		cairo_line_to (cr, allocation.width, allocation.height);
		cairo_close_path (cr);
		cairo_stroke (cr);

		cairo_new_path (cr);
		cairo_move_to (cr, allocation.width, 0);
		cairo_line_to (cr, 0, allocation.height);
		cairo_close_path (cr);
		cairo_stroke (cr);
	}
	else {
		gth_histogram_paint_grid (self, cr);
		if (self->priv->display_mode == GTH_HISTOGRAM_MODE_ALL_CHANNELS) {
			int i;

			for (i = 0; i <= gth_histogram_get_nchannels (self->priv->histogram); i++)
				gth_histogram_paint_channel (self, cr, i, TRUE);

			for (i = 0; i <= gth_histogram_get_nchannels (self->priv->histogram); i++)
				if (i != self->priv->current_channel)
					gth_histogram_paint_channel (self, cr, i, FALSE);
			gth_histogram_paint_channel (self, cr, self->priv->current_channel, FALSE);
		}
		else
			gth_histogram_paint_channel (self, cr, self->priv->current_channel, FALSE);
	}

	cairo_destroy (cr);

	if (GTK_WIDGET_CLASS (gth_histogram_view_parent_class)->expose_event != NULL)
		GTK_WIDGET_CLASS (gth_histogram_view_parent_class)->expose_event (widget, event);

	return FALSE;
}


static void
gth_histogram_view_map (GtkWidget *widget)
{
	GdkWindow *window;

	if (GTK_WIDGET_CLASS (gth_histogram_view_parent_class)->map != NULL)
		GTK_WIDGET_CLASS (gth_histogram_view_parent_class)->map (widget);

	window = gtk_widget_get_window (widget);
	gdk_window_set_events (window, GDK_BUTTON_PRESS_MASK | gdk_window_get_events (window));
}


static gboolean
gth_histogram_view_scroll_event (GtkWidget      *widget,
				 GdkEventScroll *event)
{
	GthHistogramView *self;

	self = GTH_HISTOGRAM_VIEW (widget);

	if (self->priv->histogram == NULL)
		return FALSE;

	if (event->direction == GDK_SCROLL_UP)
		self->priv->current_channel--;
	else if (event->direction == GDK_SCROLL_DOWN)
		self->priv->current_channel++;
	self->priv->current_channel = CLAMP (self->priv->current_channel, 0, 3);
	gtk_widget_queue_draw (widget);

	return TRUE;
}


static void
gth_histogram_view_class_init (GthHistogramViewClass *klass)
{
	GObjectClass   *object_class;
	GtkWidgetClass *widget_class;

	gth_histogram_view_parent_class = g_type_class_peek_parent (klass);
	g_type_class_add_private (klass, sizeof (GthHistogramViewPrivate));

	object_class = (GObjectClass*) klass;
	object_class->set_property = gth_histogram_set_property;
	object_class->get_property = gth_histogram_get_property;
	object_class->finalize = gth_histogram_view_finalize;

	widget_class = (GtkWidgetClass*) klass;
	widget_class->expose_event = gth_histogram_view_expose_event;
	widget_class->map = gth_histogram_view_map;
	widget_class->scroll_event = gth_histogram_view_scroll_event;

	/* properties */

	g_object_class_install_property (object_class,
					 PROP_HISTOGRAM,
					 g_param_spec_object ("histogram",
							      "Histogram",
							      "The histogram to display",
							      GTH_TYPE_HISTOGRAM,
							      G_PARAM_READWRITE));
}


static void
gth_histogram_view_instance_init (GthHistogramView *self)
{
	self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, GTH_TYPE_HISTOGRAM_VIEW, GthHistogramViewPrivate);
	self->priv->histogram = NULL;
	self->priv->current_channel = 0;
	self->priv->display_mode = GTH_HISTOGRAM_MODE_ONE_CHANNEL /*GTH_HISTOGRAM_MODE_ALL_CHANNELS*/;
	self->priv->scale_type = GTH_HISTOGRAM_SCALE_LINEAR;
}


GType
gth_histogram_view_get_type (void) {
	static GType gth_histogram_view_type_id = 0;
	if (gth_histogram_view_type_id == 0) {
		static const GTypeInfo g_define_type_info = {
			sizeof (GthHistogramViewClass),
			(GBaseInitFunc) NULL,
			(GBaseFinalizeFunc) NULL,
			(GClassInitFunc) gth_histogram_view_class_init,
			(GClassFinalizeFunc) NULL,
			NULL,
			sizeof (GthHistogramView),
			0,
			(GInstanceInitFunc) gth_histogram_view_instance_init,
			NULL
		};
		gth_histogram_view_type_id = g_type_register_static (GTK_TYPE_DRAWING_AREA, "GthHistogramView", &g_define_type_info, 0);
	}
	return gth_histogram_view_type_id;
}


GtkWidget *
gth_histogram_view_new (GthHistogram *histogram)
{
	return (GtkWidget *) g_object_new (GTH_TYPE_HISTOGRAM_VIEW, "histogram", histogram, NULL);
}


static void
histogram_changed_cb (GthHistogram *histogram,
		      gpointer      user_data)
{
	GthHistogramView *self = user_data;

	gtk_widget_queue_draw (GTK_WIDGET (self));
}


void
gth_histogram_view_set_histogram (GthHistogramView *self,
				  GthHistogram     *histogram)
{
	g_return_if_fail (GTH_IS_HISTOGRAM_VIEW (self));

	if (self->priv->histogram == histogram)
		return;

	if (self->priv->histogram != NULL) {
		g_signal_handler_disconnect (self->priv->histogram, self->priv->histogram_changed_event);
		_g_object_unref (self->priv->histogram);
		self->priv->histogram_changed_event = 0;
		self->priv->histogram = NULL;
	}

	if (histogram == NULL)
		return;

	self->priv->histogram = g_object_ref (histogram);
	self->priv->histogram_changed_event = g_signal_connect (self->priv->histogram, "changed", G_CALLBACK (histogram_changed_cb), self);
}


GthHistogram *
gth_histogram_view_get_histogram (GthHistogramView *self)
{
	g_return_val_if_fail (GTH_IS_HISTOGRAM_VIEW (self), NULL);
	return self->priv->histogram;
}


void
gth_histogram_view_set_current_channel (GthHistogramView *self,
					int               n_channel)
{
	g_return_if_fail (GTH_IS_HISTOGRAM_VIEW (self));
	self->priv->current_channel = n_channel;
	gtk_widget_queue_draw (GTK_WIDGET (self));
}


gint
gth_histogram_view_get_current_channel (GthHistogramView *self)
{
	g_return_val_if_fail (GTH_IS_HISTOGRAM_VIEW (self), 0);
	return self->priv->current_channel;
}


void
gth_histogram_view_set_display_mode (GthHistogramView *self,
				     GthHistogramMode  mode)
{
	g_return_if_fail (GTH_IS_HISTOGRAM_VIEW (self));
	self->priv->display_mode = mode;
	gtk_widget_queue_draw (GTK_WIDGET (self));
}


GthHistogramMode
gth_histogram_view_get_display_mode (GthHistogramView *self)
{
	g_return_val_if_fail (GTH_IS_HISTOGRAM_VIEW (self), 0);
	return self->priv->display_mode;
}


void
gth_histogram_view_set_scale_type (GthHistogramView  *self,
				   GthHistogramScale  scale_type)
{
	g_return_if_fail (GTH_IS_HISTOGRAM_VIEW (self));
	self->priv->scale_type = scale_type;
	gtk_widget_queue_draw (GTK_WIDGET (self));
}


GthHistogramScale
gth_histogram_view_get_scale_type (GthHistogramView *self)
{
	g_return_val_if_fail (GTH_IS_HISTOGRAM_VIEW (self), 0);
	return self->priv->scale_type;
}


void
gth_histogram_view_set_selection (GthHistogramView *self,
				  guchar            start,
				  guchar            end)
{
	g_return_if_fail (GTH_IS_HISTOGRAM_VIEW (self));

	self->priv->selection_start = start;
	self->priv->selection_end = end;
}
