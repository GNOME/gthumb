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

#include <config.h>
#include <math.h>
#include <gthumb.h>
#include <extensions/image_viewer/gth-image-viewer-page.h>
#include "gth-file-tool-adjust-colors.h"


#define GET_WIDGET(x) (_gtk_builder_get_widget (self->priv->builder, (x)))
#define APPLY_DELAY 250


static gpointer parent_class = NULL;


struct _GthFileToolAdjustColorsPrivate {
	GdkPixbuf     *src_pixbuf;
	GdkPixbuf     *dest_pixbuf;
	GtkBuilder    *builder;
	GtkAdjustment *brightness_adj;
	GtkAdjustment *contrast_adj;
	GtkAdjustment *saturation_adj;
	GtkAdjustment *gamma_adj;
	GthTask       *pixbuf_task;
	guint          apply_event;
};


typedef struct {
	double gamma[5];
	int    low_input[5];
	int    high_input[5];
	int    low_output[5];
	int    high_output[5];
} Levels;



typedef struct {
	GtkWidget *viewer_page;
	Levels     levels;
	double     gamma;
	double     brightness;
	double     contrast;
	double     saturation;
} AdjustData;


static void
levels_init (Levels *levels,
	     double  brightness,
	     double  contrast,
	     double  gamma)
{
	int    channel;
	double slant;
	double value;

	for (channel = 0; channel < MAX_N_CHANNELS + 1; channel++) {
		levels->gamma[channel] = 1.0;
		levels->low_input[channel] = 0;
		levels->high_input[channel] = 255;
		levels->low_output[channel] = 0;
		levels->high_output[channel] = 255;
	}

	/*  */

	levels->gamma[0] = pow (10, - (gamma / 100.0));

	brightness = brightness / 127.0 / 2.0;
	contrast = contrast / 127.0;

	slant = tan ((contrast + 1) * G_PI_4);
	if (brightness >= 0) {
		value = -0.5 * slant + brightness * slant + 0.5;
		if (value < 0.0) {
			value = 0.0;

			/* this slightly convoluted math follows by inverting the
			 * calculation of the brightness/contrast LUT in base/lut-funcs.h */

			levels->low_input[0] = 255.0 * (- brightness * slant + 0.5 * slant - 0.5) / (slant - brightness * slant);
		}

		levels->low_output[0] = 255.0 * value;

		value = 0.5 * slant + 0.5;
		if (value > 1.0) {
			value = 1.0;
			levels->high_input[0] = 255.0 * (- brightness * slant + 0.5 * slant + 0.5) / (slant - brightness * slant);
	        }

		levels->high_output[0] = 255.0 * value;
	}
	else {
		value = 0.5 - 0.5 * slant;
		if (value < 0.0) {
			value = 0.0;
			levels->low_input[0] = 255.0 * (0.5 * slant - 0.5) / (slant + brightness * slant);
		}

		levels->low_output[0] = 255.0 * value;

		value = slant * brightness + slant * 0.5 + 0.5;
		if (value > 1.0) {
			value = 1.0;
			levels->high_input[0] = 255.0 * (0.5 * slant + 0.5) / (slant + brightness * slant);
		}

		levels->high_output[0] = 255.0 * value;
	}
}


static void
adjust_colors_init (GthPixbufTask *pixop)
{
	/* AdjustData *data = pixop->data; FIXME: delte if not used */
}


static guchar
adjust_levels_func (guchar  value,
		    Levels *levels,
		    guchar  lightness,
		    double  saturation,
		    int     channel)
{
	double inten;
	int    j;

	inten = value;

	/* For color  images this runs through the loop with j = channel + 1
	 * the first time and j = 0 the second time
	 *
	 * For bw images this runs through the loop with j = 0 the first and
	 *  only time
	 */
	for (j = channel + 1; j >= 0; j -= (channel + 1)) {
		inten /= 255.0;

		/*  determine input intensity  */

		if (levels->high_input[j] != levels->low_input[j])
			inten = (255.0 * inten - levels->low_input[j]) /
				(levels->high_input[j] - levels->low_input[j]);
		else
			inten = 255.0 * inten - levels->low_input[j];

		if (levels->gamma[j] != 0.0) {
			if (inten >= 0.0)
				inten =  pow ( inten, (1.0 / levels->gamma[j]));
			else
				inten = -pow (-inten, (1.0 / levels->gamma[j]));
		}

		inten = (saturation * lightness) + ((1.0 - saturation) * inten);

		/*  determine the output intensity  */

		if (levels->high_output[j] >= levels->low_output[j])
			inten = inten * (levels->high_output[j] - levels->low_output[j]) +
				levels->low_output[j];
		else if (levels->high_output[j] < levels->low_output[j])
			inten = levels->low_output[j] - inten *
				(levels->low_output[j] - levels->high_output[j]);
	}

	if (inten < 0.0)
		inten = 0.0;
	else if (inten > 255.0)
		inten = 255.0;

	return (guchar) inten;
}


static guchar
interpolate_value (guchar original,
		   guchar reference,
		   double distance)
{
	return CLAMP((distance * reference) + ((1.0 - distance) * original), 0, 255);
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


static void
adjust_colors_step (GthPixbufTask *pixop)
{
	AdjustData *data = pixop->data;
	guchar      min, max, lightness;

	pixop->dest_pixel[RED_PIX] = pixop->src_pixel[RED_PIX];
	pixop->dest_pixel[GREEN_PIX] = pixop->src_pixel[GREEN_PIX];
	pixop->dest_pixel[BLUE_PIX] = pixop->src_pixel[BLUE_PIX];
	if (pixop->has_alpha)
		pixop->dest_pixel[ALPHA_PIX] = pixop->src_pixel[ALPHA_PIX];

	/* intensitiy / gamma correction */

	pixop->dest_pixel[RED_PIX] = gamma_correction (pixop->dest_pixel[RED_PIX], data->gamma);
	pixop->dest_pixel[GREEN_PIX] = gamma_correction (pixop->dest_pixel[GREEN_PIX], data->gamma);
	pixop->dest_pixel[BLUE_PIX] = gamma_correction (pixop->dest_pixel[BLUE_PIX], data->gamma);

	/* brightness */

	pixop->dest_pixel[RED_PIX] = interpolate_value (pixop->dest_pixel[RED_PIX], 0, data->brightness);
	pixop->dest_pixel[GREEN_PIX] = interpolate_value (pixop->dest_pixel[GREEN_PIX], 0, data->brightness);
	pixop->dest_pixel[BLUE_PIX] = interpolate_value (pixop->dest_pixel[BLUE_PIX], 0, data->brightness);

	/* contrast */

	pixop->dest_pixel[RED_PIX] = interpolate_value (pixop->dest_pixel[RED_PIX], 127, data->contrast);
	pixop->dest_pixel[GREEN_PIX] = interpolate_value (pixop->dest_pixel[GREEN_PIX], 127, data->contrast);
	pixop->dest_pixel[BLUE_PIX] = interpolate_value (pixop->dest_pixel[BLUE_PIX], 127, data->contrast);

	/* saturation */

	max = MAX (pixop->dest_pixel[RED_PIX], pixop->dest_pixel[GREEN_PIX]);
	max = MAX (max, pixop->dest_pixel[BLUE_PIX]);
	min = MIN (pixop->dest_pixel[RED_PIX], pixop->dest_pixel[GREEN_PIX]);
	min = MIN (min, pixop->dest_pixel[BLUE_PIX]);
	lightness = (max + min) / 2;

	pixop->dest_pixel[RED_PIX] = interpolate_value (pixop->dest_pixel[RED_PIX], lightness, data->saturation);
	pixop->dest_pixel[GREEN_PIX] = interpolate_value (pixop->dest_pixel[GREEN_PIX], lightness, data->saturation);
	pixop->dest_pixel[BLUE_PIX] = interpolate_value (pixop->dest_pixel[BLUE_PIX], lightness, data->saturation);

	/*pixop->dest_pixel[RED_PIX]   = adjust_levels_func (pixop->src_pixel[RED_PIX], &data->levels, lightness, data->saturation, RED_PIX);
	pixop->dest_pixel[GREEN_PIX] = adjust_levels_func (pixop->src_pixel[GREEN_PIX], &data->levels, lightness, data->saturation, GREEN_PIX);
	pixop->dest_pixel[BLUE_PIX]  = adjust_levels_func (pixop->src_pixel[BLUE_PIX], &data->levels, lightness, data->saturation, BLUE_PIX);
	if (pixop->has_alpha)
		pixop->dest_pixel[ALPHA_PIX] = pixop->src_pixel[ALPHA_PIX];*/
}


static void
adjust_colors_release (GthPixbufTask *pixop,
		       GError        *error)
{
	AdjustData *data = pixop->data;

	if (error == NULL)
		gth_image_viewer_page_set_pixbuf (GTH_IMAGE_VIEWER_PAGE (data->viewer_page), pixop->dest, TRUE);

	g_object_unref (data->viewer_page);
	g_free (data);
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
ok_button_clicked_cb (GtkButton               *button,
		      GthFileToolAdjustColors *self)
{
	GtkWidget *window;
	GtkWidget *viewer_page;

	window = gth_file_tool_get_window (GTH_FILE_TOOL (self));
	viewer_page = gth_browser_get_viewer_page (GTH_BROWSER (window));
	gth_image_viewer_page_set_pixbuf (GTH_IMAGE_VIEWER_PAGE (viewer_page), self->priv->dest_pixbuf, TRUE);
	gth_file_tool_hide_options (GTH_FILE_TOOL (self));
}


static void
cancel_button_clicked_cb (GtkButton               *button,
			  GthFileToolAdjustColors *self)
{
	GtkWidget   *window;
	GthFileData *current_file;

	if (self->priv->apply_event != 0) {
		g_source_remove (self->priv->apply_event);
		self->priv->apply_event = 0;
	}

	window = gth_file_tool_get_window (GTH_FILE_TOOL (self));
	current_file = gth_browser_get_current_file (GTH_BROWSER (window));
	if (current_file != NULL) {
		g_file_info_set_attribute_boolean (current_file->info, "gth::file::is-modified", FALSE);
		gth_monitor_metadata_changed (gth_main_get_default_monitor (), current_file);
	}

	gth_file_tool_hide_options (GTH_FILE_TOOL (self));
}


static void
task_completed_cb (GthTask                 *task,
		   GError                  *error,
		   GthFileToolAdjustColors *self)
{
	if (self->priv->pixbuf_task == task)
		self->priv->pixbuf_task = NULL;
	g_object_unref (task);

	if (error == NULL) {
		GtkWidget *window;
		GtkWidget *viewer_page;

		window = gth_file_tool_get_window (GTH_FILE_TOOL (self));
		viewer_page = gth_browser_get_viewer_page (GTH_BROWSER (window));
		gth_image_viewer_page_set_pixbuf (GTH_IMAGE_VIEWER_PAGE (viewer_page), self->priv->dest_pixbuf, FALSE);
	}
}


static gboolean
apply_cb (gpointer user_data)
{
	GthFileToolAdjustColors *self = user_data;
	GtkWidget               *window;
	AdjustData              *data;

	if (self->priv->apply_event != 0) {
		g_source_remove (self->priv->apply_event);
		self->priv->apply_event = 0;
	}

	if (self->priv->pixbuf_task != NULL)
		gth_task_cancel (self->priv->pixbuf_task);

	window = gth_file_tool_get_window (GTH_FILE_TOOL (self));

	data = g_new0 (AdjustData, 1);
	data->viewer_page = g_object_ref (gth_browser_get_viewer_page (GTH_BROWSER (window)));

	levels_init (&data->levels,
		     gtk_adjustment_get_value (self->priv->brightness_adj),
		     gtk_adjustment_get_value (self->priv->contrast_adj),
		     gtk_adjustment_get_value (self->priv->gamma_adj));

	data->gamma = pow (10, - (gtk_adjustment_get_value (self->priv->gamma_adj) / 100.0));
	data->brightness = gtk_adjustment_get_value (self->priv->brightness_adj) / 100.0;
	data->contrast = gtk_adjustment_get_value (self->priv->contrast_adj) / 100.0;
	data->saturation = gtk_adjustment_get_value (self->priv->saturation_adj) / 100.0;

	self->priv->pixbuf_task = gth_pixbuf_task_new (_("Applying changes"),
						       self->priv->src_pixbuf,
						       self->priv->dest_pixbuf,
						       adjust_colors_init,
						       adjust_colors_step,
						       adjust_colors_release,
						       data);
	g_signal_connect (self->priv->pixbuf_task,
			  "completed",
			  G_CALLBACK (task_completed_cb),
			  self);

	gth_browser_exec_task (GTH_BROWSER (window), self->priv->pixbuf_task, FALSE);

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


static GtkAdjustment *
gimp_scale_entry_new (GtkWidget *parent_box,
		      float      value,
		      float      lower,
		      float      upper,
		      float      step_increment,
		      float      page_increment,
		      int        digits)
{
	GtkWidget *hbox;
	GtkWidget *scale;
	GtkWidget *spinbutton;
	GtkObject *adj;

	adj = gtk_adjustment_new (value, lower, upper,
				  step_increment, page_increment,
				  0.0);

	spinbutton = gtk_spin_button_new  (GTK_ADJUSTMENT (adj), 1.0, 0);
	gtk_spin_button_set_digits (GTK_SPIN_BUTTON (spinbutton), digits);
	gtk_entry_set_width_chars (GTK_ENTRY (spinbutton), 4);

	scale = gtk_hscale_new (GTK_ADJUSTMENT (adj));
	gtk_scale_set_draw_value (GTK_SCALE (scale), FALSE);
	gtk_scale_set_digits (GTK_SCALE (scale), digits);

	hbox = gtk_hbox_new (FALSE, 5);
	gtk_box_pack_start (GTK_BOX (hbox), scale, TRUE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX (hbox), spinbutton, FALSE, FALSE, 0);

	gtk_box_pack_start (GTK_BOX (parent_box), hbox, TRUE, TRUE, 0);
	gtk_widget_show_all (hbox);

	return (GtkAdjustment *) adj;
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

	_g_object_unref (self->priv->src_pixbuf);
	_g_object_unref (self->priv->dest_pixbuf);

	viewer = gth_image_viewer_page_get_image_viewer (GTH_IMAGE_VIEWER_PAGE (viewer_page));
	self->priv->src_pixbuf = gth_image_viewer_get_current_pixbuf (GTH_IMAGE_VIEWER (viewer));
	if (self->priv->src_pixbuf == NULL)
		return NULL;

	self->priv->src_pixbuf = g_object_ref (self->priv->src_pixbuf);
	self->priv->dest_pixbuf = gdk_pixbuf_copy (self->priv->src_pixbuf);

	self->priv->builder = _gtk_builder_new_from_file ("adjust-colors-options.ui", "file_tools");
	options = _gtk_builder_get_widget (self->priv->builder, "options");
	gtk_widget_show (options);

	self->priv->brightness_adj = gimp_scale_entry_new (GET_WIDGET ("brightness_hbox"), 0.0, -100.0, 100.0, 1.0, 10.0, 0);
	self->priv->contrast_adj = gimp_scale_entry_new (GET_WIDGET ("contrast_hbox"), 0.0, -100.0, 100.0, 1.0, 10.0, 0);
	self->priv->gamma_adj = gimp_scale_entry_new (GET_WIDGET ("gamma_hbox"), 0.0, -100.0, 100.0, 1.0, 10.0, 0);
	self->priv->saturation_adj = gimp_scale_entry_new (GET_WIDGET ("saturation_hbox"), 0.0, -100.0, 100.0, 1.0, 10.0, 0);

	g_signal_connect (GET_WIDGET ("ok_button"),
			  "clicked",
			  G_CALLBACK (ok_button_clicked_cb),
			  self);
	g_signal_connect (GET_WIDGET ("cancel_button"),
			  "clicked",
			  G_CALLBACK (cancel_button_clicked_cb),
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

	_g_object_unref (self->priv->src_pixbuf);
	_g_object_unref (self->priv->dest_pixbuf);
	_g_object_unref (self->priv->builder);
	self->priv->src_pixbuf = NULL;
	self->priv->dest_pixbuf = NULL;
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
	gth_file_tool_construct (GTH_FILE_TOOL (self), GTK_STOCK_EDIT, _("Adjust Colors"), _("Adjust Colors"), TRUE);
	gtk_widget_set_tooltip_text (GTK_WIDGET (self), _("Change brightness, contrast, saturation and gamma level of the image"));
}


static void
gth_file_tool_adjust_colors_finalize (GObject *object)
{
	GthFileToolAdjustColors *self;

	g_return_if_fail (object != NULL);
	g_return_if_fail (GTH_IS_FILE_TOOL_ADJUST_COLORS (object));

	self = (GthFileToolAdjustColors *) object;

	_g_object_unref (self->priv->src_pixbuf);
	_g_object_unref (self->priv->dest_pixbuf);
	_g_object_unref (self->priv->builder);

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
