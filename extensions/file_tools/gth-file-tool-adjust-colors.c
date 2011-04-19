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
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <config.h>
#include <math.h>
#include <gthumb.h>
#include <extensions/image_viewer/gth-image-viewer-page.h>
#include "gth-file-tool-adjust-colors.h"


#define GET_WIDGET(x) (_gtk_builder_get_widget (self->priv->builder, (x)))
#define APPLY_DELAY 150


static gpointer parent_class = NULL;


struct _GthFileToolAdjustColorsPrivate {
	cairo_surface_t *source;
	cairo_surface_t *destination;
	GtkBuilder      *builder;
	GtkAdjustment   *gamma_adj;
	GtkAdjustment   *brightness_adj;
	GtkAdjustment   *contrast_adj;
	GtkAdjustment   *saturation_adj;
	GtkAdjustment   *cyan_red_adj;
	GtkAdjustment   *magenta_green_adj;
	GtkAdjustment   *yellow_blue_adj;
	GtkWidget       *histogram_view;
	GthHistogram    *histogram;
	GthTask         *image_task;
	guint            apply_event;
};


typedef struct {
	GthFileToolAdjustColors *self;
	cairo_surface_t         *source;
	cairo_surface_t         *destination;
	GtkWidget               *viewer_page;
	double                   gamma;
	double                   brightness;
	double                   contrast;
	double                   saturation;
	double                   color_level[3];
	PixbufCache             *cache;
	double                   midtone_distance[256];
} AdjustData;


static void
adjust_colors_before (GthAsyncTask *task,
		      gpointer      user_data)
{
	AdjustData *adjust_data = user_data;
	int         i;

	adjust_data->cache = pixbuf_cache_new ();
	for (i = 0; i < 256; i++)
		adjust_data->midtone_distance[i] = 0.667 * (1 - SQR (((double) i - 127.0) / 127.0));
}


static guchar
gamma_correction (guchar original,
		  double gamma)
{
	double inten;

	inten = (double) original / 255.0;

	if (gamma != 0.0) {
		if (inten >= 0.0)
			inten =  pow ( inten, (1.0 / gamma));
		else
			inten = -pow (-inten, (1.0 / gamma));
	}

	return CLAMP (inten * 255.0, 0, 255);
}


static gpointer
adjust_colors_exec (GthAsyncTask *task,
		    gpointer      user_data)
{
	AdjustData      *adjust_data = user_data;
	cairo_format_t   format;
	int              width;
	int              height;
	int              source_stride;
	int              destination_stride;
	unsigned char   *p_source_line;
	unsigned char   *p_destination_line;
	unsigned char   *p_source;
	unsigned char   *p_destination;
	gboolean         cancelled;
	double           progress;
	gboolean         terminated;
	int              x, y;
	unsigned char    values[4];
	int              channel;
	unsigned char    value;

	format = cairo_image_surface_get_format (adjust_data->source);
	width = cairo_image_surface_get_width (adjust_data->source);
	height = cairo_image_surface_get_height (adjust_data->source);
	source_stride = cairo_image_surface_get_stride (adjust_data->source);

	adjust_data->destination = cairo_image_surface_create (format, width, height);
	destination_stride = cairo_image_surface_get_stride (adjust_data->destination);
	p_source_line = cairo_image_surface_get_data (adjust_data->source);
	p_destination_line = cairo_image_surface_get_data (adjust_data->destination);
	for (y = 0; y < height; y++) {
		gth_async_task_get_data (task, NULL, &cancelled, NULL);
		if (cancelled)
			return NULL;

		progress = (double) y / height;
		gth_async_task_set_data (task, NULL, NULL, &progress);

		p_source = p_source_line;
		p_destination = p_destination_line;
		for (x = 0; x < width; x++) {
			CAIRO_GET_RGBA (p_source, values[0], values[1], values[2], values[3]);

			/* gamma correction / brightness / contrast */

			for (channel = 0; channel < 3; channel++) {
				value = values[channel];

				if (! pixbuf_cache_get (adjust_data->cache, channel + 1, &value)) {
					int i;

					value = gamma_correction (value, adjust_data->gamma);

					if (adjust_data->brightness > 0)
						value = interpolate_value (value, 0, adjust_data->brightness);
					else
						value = interpolate_value (value, 255, - adjust_data->brightness);

					if (adjust_data->contrast < 0)
						value = interpolate_value (value, 127, tan (adjust_data->contrast * G_PI_2));
					else
						value = interpolate_value (value, 127, adjust_data->contrast);

					i = value + adjust_data->color_level[channel] * adjust_data->midtone_distance[value];
					value = CLAMP (i, 0, 255);

					pixbuf_cache_set (adjust_data->cache, channel + 1, values[channel], value);
				}

				values[channel] = value;
			}

			/* saturation */

			if (adjust_data->saturation != 0.0) {
				guchar min, max, lightness;
				double saturation;

				max = MAX (MAX (values[0], values[1]), values[2]);
				min = MIN (MIN (values[0], values[1]), values[2]);
				lightness = (max + min) / 2;

				if (adjust_data->saturation < 0)
					saturation = tan (adjust_data->saturation * G_PI_2);
				else
					saturation = adjust_data->saturation;

				values[0] = interpolate_value (values[0], lightness, saturation);
				values[1] = interpolate_value (values[1], lightness, saturation);
				values[2] = interpolate_value (values[2], lightness, saturation);
			}

			CAIRO_SET_RGBA (p_destination, values[0], values[1], values[2], values[3]);

			p_source += 4;
			p_destination += 4;
		}
		p_source_line += source_stride;
		p_destination_line += destination_stride;
	}

	terminated = TRUE;
	gth_async_task_set_data (task, &terminated, NULL, NULL);

	return NULL;
}


static void
adjust_colors_after (GthAsyncTask *task,
		     GError       *error,
		     gpointer      user_data)
{
	AdjustData *adjust_data = user_data;

	if (error == NULL) {
		GthFileToolAdjustColors *self = adjust_data->self;

		cairo_surface_destroy (self->priv->destination);
		self->priv->destination = cairo_surface_reference (adjust_data->destination);

		gth_image_viewer_page_set_image (GTH_IMAGE_VIEWER_PAGE (adjust_data->viewer_page), self->priv->destination, FALSE);
		gth_histogram_calculate_for_image (self->priv->histogram, self->priv->destination);
	}

	pixbuf_cache_free (adjust_data->cache);
}


static void
adjust_data_free (gpointer user_data)
{
	AdjustData *adjust_data = user_data;

	cairo_surface_destroy (adjust_data->destination);
	cairo_surface_destroy (adjust_data->source);
	g_object_unref (adjust_data->viewer_page);
	g_free (adjust_data);
}


static void
gth_file_tool_adjust_colors_update_sensitivity (GthFileTool *base)
{
	GtkWidget *window;
	GtkWidget *viewer_page;

	window = gth_file_tool_get_window (base);
	viewer_page = gth_browser_get_viewer_page (GTH_BROWSER (window));
	if (! GTH_IS_IMAGE_VIEWER_PAGE (viewer_page))
		gtk_widget_set_sensitive (GTK_WIDGET (base), FALSE);
	else
		gtk_widget_set_sensitive (GTK_WIDGET (base), TRUE);
}


static void
ok_button_clicked_cb (GtkButton *button,
		      gpointer   user_data)
{
	GthFileToolAdjustColors *self = user_data;

	if (self->priv->destination != NULL) {
		GtkWidget *window;
		GtkWidget *viewer_page;

		window = gth_file_tool_get_window (GTH_FILE_TOOL (self));
		viewer_page = gth_browser_get_viewer_page (GTH_BROWSER (window));
		gth_image_viewer_page_set_image (GTH_IMAGE_VIEWER_PAGE (viewer_page), self->priv->destination, TRUE);
	}

	gth_file_tool_hide_options (GTH_FILE_TOOL (self));
}


static void
cancel_button_clicked_cb (GtkButton *button,
			  gpointer   user_data)
{
	GthFileToolAdjustColors *self = user_data;
	GtkWidget               *window;
	GtkWidget               *viewer_page;

	if (self->priv->apply_event != 0) {
		g_source_remove (self->priv->apply_event);
		self->priv->apply_event = 0;
	}

	window = gth_file_tool_get_window (GTH_FILE_TOOL (self));
	viewer_page = gth_browser_get_viewer_page (GTH_BROWSER (window));
	gth_image_viewer_page_reset (GTH_IMAGE_VIEWER_PAGE (viewer_page));

	gth_file_tool_hide_options (GTH_FILE_TOOL (self));
}


static void
reset_button_clicked_cb (GtkButton *button,
		  	 gpointer   user_data)
{
	GthFileToolAdjustColors *self = user_data;

	gtk_adjustment_set_value (self->priv->gamma_adj, 0.0);
	gtk_adjustment_set_value (self->priv->brightness_adj, 0.0);
	gtk_adjustment_set_value (self->priv->contrast_adj, 0.0);
	gtk_adjustment_set_value (self->priv->saturation_adj, 0.0);
	gtk_adjustment_set_value (self->priv->cyan_red_adj, 0.0);
	gtk_adjustment_set_value (self->priv->magenta_green_adj, 0.0);
	gtk_adjustment_set_value (self->priv->yellow_blue_adj, 0.0);
}


static gboolean
apply_cb (gpointer user_data)
{
	GthFileToolAdjustColors *self = user_data;
	GtkWidget               *window;
	AdjustData              *adjust_data;

	if (self->priv->apply_event != 0) {
		g_source_remove (self->priv->apply_event);
		self->priv->apply_event = 0;
	}

	if (self->priv->image_task != NULL)
		gth_task_cancel (self->priv->image_task);

	window = gth_file_tool_get_window (GTH_FILE_TOOL (self));

	adjust_data = g_new0 (AdjustData, 1);
	adjust_data->self = self;
	adjust_data->viewer_page = g_object_ref (gth_browser_get_viewer_page (GTH_BROWSER (window)));
	adjust_data->source = cairo_surface_reference (self->priv->source);
	adjust_data->gamma = pow (10, - (gtk_adjustment_get_value (self->priv->gamma_adj) / 100.0));
	adjust_data->brightness = gtk_adjustment_get_value (self->priv->brightness_adj) / 100.0 * -1.0;
	adjust_data->contrast = gtk_adjustment_get_value (self->priv->contrast_adj) / 100.0 * -1.0;
	adjust_data->saturation = gtk_adjustment_get_value (self->priv->saturation_adj) / 100.0 * -1.0;
	adjust_data->color_level[0] = gtk_adjustment_get_value (self->priv->cyan_red_adj);
	adjust_data->color_level[1] = gtk_adjustment_get_value (self->priv->magenta_green_adj);
	adjust_data->color_level[2] = gtk_adjustment_get_value (self->priv->yellow_blue_adj);

	self->priv->image_task = gth_async_task_new (adjust_colors_before,
						      adjust_colors_exec,
						      adjust_colors_after,
						      adjust_data,
						      adjust_data_free);
	gth_browser_exec_task (GTH_BROWSER (window), self->priv->image_task, FALSE);

	return FALSE;
}


static void
value_changed_cb (GtkAdjustment           *adj,
		  GthFileToolAdjustColors *self)
{
	if (self->priv->apply_event != 0) {
		g_source_remove (self->priv->apply_event);
		self->priv->apply_event = 0;
	}
	self->priv->apply_event = g_timeout_add (APPLY_DELAY, apply_cb, self);
}


static GtkWidget *
gth_file_tool_adjust_colors_get_options (GthFileTool *base)
{
	GthFileToolAdjustColors *self;
	GtkWidget               *window;
	GtkWidget               *viewer_page;
	GtkWidget               *viewer;
	GtkWidget               *options;

	self = (GthFileToolAdjustColors *) base;

	window = gth_file_tool_get_window (base);
	viewer_page = gth_browser_get_viewer_page (GTH_BROWSER (window));
	if (! GTH_IS_IMAGE_VIEWER_PAGE (viewer_page))
		return NULL;

	cairo_surface_destroy (self->priv->source);
	cairo_surface_destroy (self->priv->destination);

	viewer = gth_image_viewer_page_get_image_viewer (GTH_IMAGE_VIEWER_PAGE (viewer_page));
	self->priv->source = cairo_surface_reference (gth_image_viewer_get_current_image (GTH_IMAGE_VIEWER (viewer)));
	if (self->priv->source == NULL)
		return NULL;

	self->priv->destination = NULL;

	self->priv->builder = _gtk_builder_new_from_file ("adjust-colors-options.ui", "file_tools");
	options = _gtk_builder_get_widget (self->priv->builder, "options");
	gtk_widget_show (options);

	self->priv->histogram_view = gth_histogram_view_new (self->priv->histogram);
	gtk_widget_show (self->priv->histogram_view);
	gtk_box_pack_start (GTK_BOX (GET_WIDGET ("histogram_hbox")), self->priv->histogram_view, TRUE, TRUE, 0);

	self->priv->brightness_adj    = gimp_scale_entry_new (GET_WIDGET ("brightness_hbox"),
							      GTK_LABEL (GET_WIDGET ("brightness_label")),
							      0.0, -100.0, 100.0, 1.0, 10.0, 0);
	self->priv->contrast_adj      = gimp_scale_entry_new (GET_WIDGET ("contrast_hbox"),
							      GTK_LABEL (GET_WIDGET ("contrast_label")),
							      0.0, -100.0, 100.0, 1.0, 10.0, 0);
	self->priv->gamma_adj         = gimp_scale_entry_new (GET_WIDGET ("gamma_hbox"),
							      GTK_LABEL (GET_WIDGET ("gamma_label")),
							      0.0, -100.0, 100.0, 1.0, 10.0, 0);
	self->priv->saturation_adj    = gimp_scale_entry_new (GET_WIDGET ("saturation_hbox"),
							      GTK_LABEL (GET_WIDGET ("saturation_label")),
							      0.0, -100.0, 100.0, 1.0, 10.0, 0);
	self->priv->cyan_red_adj      = gimp_scale_entry_new (GET_WIDGET ("cyan_red_hbox"),
							      GTK_LABEL (GET_WIDGET ("cyan_red_label")),
							      0.0, -100.0, 100.0, 1.0, 10.0, 0);
	self->priv->magenta_green_adj = gimp_scale_entry_new (GET_WIDGET ("magenta_green_hbox"),
							      GTK_LABEL (GET_WIDGET ("magenta_green_label")),
							      0.0, -100.0, 100.0, 1.0, 10.0, 0);
	self->priv->yellow_blue_adj   = gimp_scale_entry_new (GET_WIDGET ("yellow_blue_hbox"),
							      GTK_LABEL (GET_WIDGET ("yellow_blue_label")),
							      0.0, -100.0, 100.0, 1.0, 10.0, 0);

	g_signal_connect (GET_WIDGET ("ok_button"),
			  "clicked",
			  G_CALLBACK (ok_button_clicked_cb),
			  self);
	g_signal_connect (GET_WIDGET ("cancel_button"),
			  "clicked",
			  G_CALLBACK (cancel_button_clicked_cb),
			  self);
	g_signal_connect (GET_WIDGET ("reset_button"),
			  "clicked",
			  G_CALLBACK (reset_button_clicked_cb),
			  self);
	g_signal_connect (G_OBJECT (self->priv->brightness_adj),
			  "value-changed",
			  G_CALLBACK (value_changed_cb),
			  self);
	g_signal_connect (G_OBJECT (self->priv->contrast_adj),
			  "value-changed",
			  G_CALLBACK (value_changed_cb),
			  self);
	g_signal_connect (G_OBJECT (self->priv->gamma_adj),
			  "value-changed",
			  G_CALLBACK (value_changed_cb),
			  self);
	g_signal_connect (G_OBJECT (self->priv->saturation_adj),
			  "value-changed",
			  G_CALLBACK (value_changed_cb),
			  self);
	g_signal_connect (G_OBJECT (self->priv->cyan_red_adj),
			  "value-changed",
			  G_CALLBACK (value_changed_cb),
			  self);
	g_signal_connect (G_OBJECT (self->priv->magenta_green_adj),
			  "value-changed",
			  G_CALLBACK (value_changed_cb),
			  self);
	g_signal_connect (G_OBJECT (self->priv->yellow_blue_adj),
			  "value-changed",
			  G_CALLBACK (value_changed_cb),
			  self);

	gth_histogram_calculate_for_image (self->priv->histogram, self->priv->source);

	return options;
}


static void
gth_file_tool_adjust_colors_destroy_options (GthFileTool *base)
{
	GthFileToolAdjustColors *self;

	self = (GthFileToolAdjustColors *) base;

	if (self->priv->apply_event != 0) {
		g_source_remove (self->priv->apply_event);
		self->priv->apply_event = 0;
	}

	cairo_surface_destroy (self->priv->source);
	cairo_surface_destroy (self->priv->destination);
	_g_object_unref (self->priv->builder);
	self->priv->source = NULL;
	self->priv->destination = NULL;
	self->priv->builder = NULL;
}


static void
gth_file_tool_adjust_colors_activate (GthFileTool *base)
{
	gth_file_tool_show_options (base);
}


static void
gth_file_tool_adjust_colors_instance_init (GthFileToolAdjustColors *self)
{
	self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, GTH_TYPE_FILE_TOOL_ADJUST_COLORS, GthFileToolAdjustColorsPrivate);
	self->priv->histogram = gth_histogram_new ();

	gth_file_tool_construct (GTH_FILE_TOOL (self), "tool-adjust-colors", _("Adjust Colors..."), _("Adjust Colors"), FALSE);
	gtk_widget_set_tooltip_text (GTK_WIDGET (self), _("Change brightness, contrast, saturation and gamma level of the image"));
}


static void
gth_file_tool_adjust_colors_finalize (GObject *object)
{
	GthFileToolAdjustColors *self;

	g_return_if_fail (object != NULL);
	g_return_if_fail (GTH_IS_FILE_TOOL_ADJUST_COLORS (object));

	self = (GthFileToolAdjustColors *) object;

	cairo_surface_destroy (self->priv->source);
	cairo_surface_destroy (self->priv->destination);
	_g_object_unref (self->priv->builder);
	_g_object_unref (self->priv->histogram);

	/* Chain up */
	G_OBJECT_CLASS (parent_class)->finalize (object);
}


static void
gth_file_tool_adjust_colors_class_init (GthFileToolAdjustColorsClass *class)
{
	GObjectClass     *gobject_class;
	GthFileToolClass *file_tool_class;

	parent_class = g_type_class_peek_parent (class);
	g_type_class_add_private (class, sizeof (GthFileToolAdjustColorsPrivate));

	gobject_class = (GObjectClass*) class;
	gobject_class->finalize = gth_file_tool_adjust_colors_finalize;

	file_tool_class = (GthFileToolClass *) class;
	file_tool_class->update_sensitivity = gth_file_tool_adjust_colors_update_sensitivity;
	file_tool_class->activate = gth_file_tool_adjust_colors_activate;
	file_tool_class->get_options = gth_file_tool_adjust_colors_get_options;
	file_tool_class->destroy_options = gth_file_tool_adjust_colors_destroy_options;
}


GType
gth_file_tool_adjust_colors_get_type (void) {
	static GType type_id = 0;
	if (type_id == 0) {
		static const GTypeInfo g_define_type_info = {
			sizeof (GthFileToolAdjustColorsClass),
			(GBaseInitFunc) NULL,
			(GBaseFinalizeFunc) NULL,
			(GClassInitFunc) gth_file_tool_adjust_colors_class_init,
			(GClassFinalizeFunc) NULL,
			NULL,
			sizeof (GthFileToolAdjustColors),
			0,
			(GInstanceInitFunc) gth_file_tool_adjust_colors_instance_init,
			NULL
		};
		type_id = g_type_register_static (GTH_TYPE_FILE_TOOL, "GthFileToolAdjustColors", &g_define_type_info, 0);
	}
	return type_id;
}
