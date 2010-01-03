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
#define APPLY_DELAY 150
#define SQR(x) ((x) * (x))


static gpointer parent_class = NULL;


struct _GthFileToolAdjustColorsPrivate {
	GdkPixbuf     *src_pixbuf;
	GdkPixbuf     *dest_pixbuf;
	GtkBuilder    *builder;
	GtkAdjustment *gamma_adj;
	GtkAdjustment *brightness_adj;
	GtkAdjustment *contrast_adj;
	GtkAdjustment *saturation_adj;
	GtkAdjustment *cyan_red_adj;
	GtkAdjustment *magenta_green_adj;
	GtkAdjustment *yellow_blue_adj;
	GtkWidget     *histogram_view;
	GthHistogram  *histogram;
	GthTask       *pixbuf_task;
	guint          apply_event;
};


typedef struct {
	GtkWidget   *viewer_page;
	double       gamma;
	double       brightness;
	double       contrast;
	double       saturation;
	double       color_level[3];
	PixbufCache *cache;
	double       midtone_distance[256];
} AdjustData;


static void
adjust_colors_init (GthPixbufTask *pixop)
{
	AdjustData *data = pixop->data;
	int         i;

	copy_source_to_destination (pixop);
	data->cache = pixbuf_cache_new ();
	for (i = 0; i < 256; i++)
		data->midtone_distance[i] = 0.667 * (1 - SQR (((double) i - 127.0) / 127.0));
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
	int         channel;

	if (pixop->has_alpha)
		pixop->dest_pixel[ALPHA_PIX] = pixop->src_pixel[ALPHA_PIX];

	/* gamma correction / brightness / contrast */

	for (channel = RED_PIX; channel <= BLUE_PIX; channel++) {
		guchar v;

		v = pixop->src_pixel[channel];
		if (! pixbuf_cache_get (data->cache, channel + 1, &v)) {
			int i;

			v = gamma_correction (v, data->gamma);

			if (data->brightness > 0)
				v = interpolate_value (v, 0, data->brightness);
			else
				v = interpolate_value (v, 255, - data->brightness);

			if (data->contrast < 0)
				v = interpolate_value (v, 127, tan (data->contrast * G_PI_2) /*data->contrast*/);
			else
				v = interpolate_value (v, 127, data->contrast);

			i = v + data->color_level[channel] * data->midtone_distance[v];
			v = CLAMP(i, 0, 255);

			pixbuf_cache_set (data->cache, channel + 1, pixop->src_pixel[channel], v);
		}

		pixop->dest_pixel[channel] = v;
	}

	/* saturation */

	if (data->saturation != 0.0) {
		guchar min, max, lightness;
		double saturation;

		max = MAX (pixop->dest_pixel[RED_PIX], pixop->dest_pixel[GREEN_PIX]);
		max = MAX (max, pixop->dest_pixel[BLUE_PIX]);
		min = MIN (pixop->dest_pixel[RED_PIX], pixop->dest_pixel[GREEN_PIX]);
		min = MIN (min, pixop->dest_pixel[BLUE_PIX]);
		lightness = (max + min) / 2;

		if (data->saturation < 0)
			saturation = tan (data->saturation * G_PI_2);
		else
			saturation = data->saturation;

		pixop->dest_pixel[RED_PIX] = interpolate_value (pixop->dest_pixel[RED_PIX], lightness, saturation);
		pixop->dest_pixel[GREEN_PIX] = interpolate_value (pixop->dest_pixel[GREEN_PIX], lightness, saturation);
		pixop->dest_pixel[BLUE_PIX] = interpolate_value (pixop->dest_pixel[BLUE_PIX], lightness, saturation);
	}
}


static void
adjust_colors_release (GthPixbufTask *pixop,
		       GError        *error)
{
	AdjustData *data = pixop->data;

	pixbuf_cache_free (data->cache);
}


static void
adjust_colors_destroy_data (gpointer user_data)
{
	AdjustData *data = user_data;

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
	GtkWidget *window;
	GtkWidget *viewer_page;

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
reset_button_clicked_cb (GtkButton               *button,
			 GthFileToolAdjustColors *self)
{
	gtk_adjustment_set_value (self->priv->gamma_adj, 0.0);
	gtk_adjustment_set_value (self->priv->brightness_adj, 0.0);
	gtk_adjustment_set_value (self->priv->contrast_adj, 0.0);
	gtk_adjustment_set_value (self->priv->saturation_adj, 0.0);
	gtk_adjustment_set_value (self->priv->cyan_red_adj, 0.0);
	gtk_adjustment_set_value (self->priv->magenta_green_adj, 0.0);
	gtk_adjustment_set_value (self->priv->yellow_blue_adj, 0.0);
}


static void
task_completed_cb (GthTask                 *task,
		   GError                  *error,
		   GthFileToolAdjustColors *self)
{
	if (self->priv->pixbuf_task == task)
		self->priv->pixbuf_task = NULL;

	if (error == NULL) {
		GtkWidget *window;
		GtkWidget *viewer_page;

		_g_object_unref (self->priv->dest_pixbuf);
		self->priv->dest_pixbuf = g_object_ref (GTH_PIXBUF_TASK (task)->dest);

		window = gth_file_tool_get_window (GTH_FILE_TOOL (self));
		viewer_page = gth_browser_get_viewer_page (GTH_BROWSER (window));
		gth_image_viewer_page_set_pixbuf (GTH_IMAGE_VIEWER_PAGE (viewer_page), self->priv->dest_pixbuf, FALSE);
		gth_histogram_calculate (self->priv->histogram, self->priv->dest_pixbuf);
	}

	g_object_unref (task);
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

	data->gamma = pow (10, - (gtk_adjustment_get_value (self->priv->gamma_adj) / 100.0));
	data->brightness = gtk_adjustment_get_value (self->priv->brightness_adj) / 100.0 * -1.0;
	data->contrast = gtk_adjustment_get_value (self->priv->contrast_adj) / 100.0 * -1.0;
	data->saturation = gtk_adjustment_get_value (self->priv->saturation_adj) / 100.0 * -1.0;
	data->color_level[0] = gtk_adjustment_get_value (self->priv->cyan_red_adj);
	data->color_level[1] = gtk_adjustment_get_value (self->priv->magenta_green_adj);
	data->color_level[2] = gtk_adjustment_get_value (self->priv->yellow_blue_adj);

	self->priv->pixbuf_task = gth_pixbuf_task_new (_("Applying changes"),
						       FALSE,
						       adjust_colors_init,
						       adjust_colors_step,
						       adjust_colors_release,
						       data,
						       adjust_colors_destroy_data);
	gth_pixbuf_task_set_source (GTH_PIXBUF_TASK (self->priv->pixbuf_task),
				    self->priv->src_pixbuf);
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
	self->priv->dest_pixbuf = NULL;

	self->priv->builder = _gtk_builder_new_from_file ("adjust-colors-options.ui", "file_tools");
	options = _gtk_builder_get_widget (self->priv->builder, "options");
	gtk_widget_show (options);

	self->priv->histogram_view = gth_histogram_view_new (self->priv->histogram);
	gtk_widget_show (self->priv->histogram_view);
	gtk_box_pack_start (GTK_BOX (GET_WIDGET ("histogram_hbox")), self->priv->histogram_view, TRUE, TRUE, 0);

	self->priv->brightness_adj = gimp_scale_entry_new (GET_WIDGET ("brightness_hbox"), 0.0, -100.0, 100.0, 1.0, 10.0, 0);
	self->priv->contrast_adj = gimp_scale_entry_new (GET_WIDGET ("contrast_hbox"), 0.0, -100.0, 100.0, 1.0, 10.0, 0);
	self->priv->gamma_adj = gimp_scale_entry_new (GET_WIDGET ("gamma_hbox"), 0.0, -100.0, 100.0, 1.0, 10.0, 0);
	self->priv->saturation_adj = gimp_scale_entry_new (GET_WIDGET ("saturation_hbox"), 0.0, -100.0, 100.0, 1.0, 10.0, 0);
	self->priv->cyan_red_adj = gimp_scale_entry_new (GET_WIDGET ("cyan_red_hbox"), 0.0, -100.0, 100.0, 1.0, 10.0, 0);
	self->priv->magenta_green_adj = gimp_scale_entry_new (GET_WIDGET ("magenta_green_hbox"), 0.0, -100.0, 100.0, 1.0, 10.0, 0);
	self->priv->yellow_blue_adj = gimp_scale_entry_new (GET_WIDGET ("yellow_blue_hbox"), 0.0, -100.0, 100.0, 1.0, 10.0, 0);

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

	gth_histogram_calculate (self->priv->histogram, self->priv->src_pixbuf);

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
	self->priv->histogram = gth_histogram_new ();

	gth_file_tool_construct (GTH_FILE_TOOL (self), GTK_STOCK_EDIT, _("Adjust Colors"), _("Adjust Colors"), FALSE);
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
