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


/* -- gimpcolorspace -- */


static void
gimp_rgb_to_hls_int (gint *red,
		     gint *green,
		     gint *blue)
{
  gint    r, g, b;
  gdouble h, l, s;
  gint    min, max;
  gint    delta;

  r = *red;
  g = *green;
  b = *blue;

  if (r > g)
    {
      max = MAX (r, b);
      min = MIN (g, b);
    }
  else
    {
      max = MAX (g, b);
      min = MIN (r, b);
    }

  l = (max + min) / 2.0;

  if (max == min)
    {
      s = 0.0;
      h = 0.0;
    }
  else
    {
      delta = (max - min);

      if (l < 128)
	s = 255 * (gdouble) delta / (gdouble) (max + min);
      else
	s = 255 * (gdouble) delta / (gdouble) (511 - max - min);

      if (r == max)
	h = (g - b) / (gdouble) delta;
      else if (g == max)
	h = 2 + (b - r) / (gdouble) delta;
      else
	h = 4 + (r - g) / (gdouble) delta;

      h = h * 42.5;

      if (h < 0)
	h += 255;
      else if (h > 255)
	h -= 255;
    }

  *red   = h;
  *green = l;
  *blue  = s;
}


static gint
gimp_hls_value (gdouble n1,
		gdouble n2,
		gdouble hue)
{
  gdouble value;

  if (hue > 255)
    hue -= 255;
  else if (hue < 0)
    hue += 255;
  if (hue < 42.5)
    value = n1 + (n2 - n1) * (hue / 42.5);
  else if (hue < 127.5)
    value = n2;
  else if (hue < 170)
    value = n1 + (n2 - n1) * ((170 - hue) / 42.5);
  else
    value = n1;

  return (gint) (value * 255);
}


static void
gimp_hls_to_rgb_int (gint *hue,
		     gint *lightness,
		     gint *saturation)
{
  gdouble h, l, s;
  gdouble m1, m2;

  h = *hue;
  l = *lightness;
  s = *saturation;

  if (s == 0)
    {
      /*  achromatic case  */
      *hue        = l;
      *lightness  = l;
      *saturation = l;
    }
  else
    {
      if (l < 128)
	m2 = (l * (255 + s)) / 65025.0;
      else
	m2 = (l + s - (l * s) / 255.0) / 255.0;

      m1 = (l / 127.5) - m2;

      /*  chromatic case  */
      *hue        = gimp_hls_value (m1, m2, h + 85);
      *lightness  = gimp_hls_value (m1, m2, h);
      *saturation = gimp_hls_value (m1, m2, h - 85);
    }
}


/* -- hue, lightness, saturation -- */


typedef enum {
	GIMP_ALL_HUES,
	GIMP_RED_HUES,
	GIMP_YELLOW_HUES,
	GIMP_GREEN_HUES,
	GIMP_CYAN_HUES,
	GIMP_BLUE_HUES,
	GIMP_MAGENTA_HUES
} GimpHueRange;


typedef struct {
	double hue[7];
	double lightness[7];
	double saturation[7];
	int    hue_transfer[6][256];
	int    lightness_transfer[6][256];
	int    saturation_transfer[6][256];
} HueSaturationData;


static void
hue_saturation_data_init (HueSaturationData *hs)
{
	GimpHueRange partition;

	g_return_if_fail (hs != NULL);

	for (partition = GIMP_ALL_HUES; partition <= GIMP_MAGENTA_HUES; partition++) {
		hs->hue[partition]        = 0.0;
		hs->lightness[partition]  = 0.0;
		hs->saturation[partition] = 0.0;
	}
}


static void
hue_saturation_calculate_transfers (HueSaturationData *hs)
{
	int value;
	int hue;
	int i;

	g_return_if_fail (hs != NULL);

	/*  Calculate transfers  */
	for (hue = 0; hue < 6; hue++)
		for (i = 0; i < 256; i++) {
			/* Hue */
			value = (hs->hue[0] + hs->hue[hue + 1]) * 255.0 / 360.0;
			if ((i + value) < 0)
				hs->hue_transfer[hue][i] = 255 + (i + value);
			else if ((i + value) > 255)
				hs->hue_transfer[hue][i] = i + value - 255;
			else
				hs->hue_transfer[hue][i] = i + value;

			/*  Lightness  */
			value = (hs->lightness[0] + hs->lightness[hue + 1]) * 127.0 / 100.0;
			value = CLAMP (value, -255, 255);
			if (value < 0)
				hs->lightness_transfer[hue][i] = (unsigned char) ((i * (255 + value)) / 255);
			else
				hs->lightness_transfer[hue][i] = (unsigned char) (i + ((255 - i) * value) / 255);

			/*  Saturation  */
			value = (hs->saturation[0] + hs->saturation[hue + 1]) * 255.0 / 100.0;
			value = CLAMP (value, -255, 255);
			hs->saturation_transfer[hue][i] = CLAMP ((i * (255 + value)) / 255, 0, 255);
		}
}


/* --- */


struct _GthFileToolAdjustColorsPrivate {
	GdkPixbuf     *src_pixbuf;
	GdkPixbuf     *dest_pixbuf;
	GtkBuilder    *builder;
	GtkAdjustment *gamma_adj;
	GtkAdjustment *brightness_adj;
	GtkAdjustment *contrast_adj;
	GtkAdjustment *saturation_adj;
	GtkAdjustment *hue_adj;
	GtkAdjustment *lightness_adj;
	GthTask       *pixbuf_task;
	guint          apply_event;
};


typedef struct {
	GtkWidget         *viewer_page;
	double             gamma;
	double             brightness;
	double             contrast;
	double             saturation;
	double             hue;
	double             lightness;
	PixbufCache       *cache;
	HueSaturationData *hs;
} AdjustData;


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
adjust_colors_init (GthPixbufTask *pixop)
{
	AdjustData *data = pixop->data;

	data->hs = g_new (HueSaturationData, 1);
	hue_saturation_data_init (data->hs);
	data->hs->hue[GIMP_ALL_HUES] = data->hue /* -180.0 ==> 180.0 */;
	data->hs->lightness[GIMP_ALL_HUES] = data->lightness /* -100.0 ==> 100.0 */;
	data->hs->saturation[GIMP_ALL_HUES] = data->saturation * (- 100.0)  /* -100.0 ==> 100.0 */;
	hue_saturation_calculate_transfers (data->hs);

	data->cache = pixbuf_cache_new ();
}


static void
hue_saturation_step (GthPixbufTask *pixop)
{
	AdjustData        *data = pixop->data;
	HueSaturationData *hs = data->hs;
	int                r, g, b, hue_idx;

	r = pixop->dest_pixel[RED_PIX];
	g = pixop->dest_pixel[GREEN_PIX];
	b = pixop->dest_pixel[BLUE_PIX];

	gimp_rgb_to_hls_int (&r, &g, &b);

	if (r < 43)
		hue_idx = 0;
	else if (r < 85)
		hue_idx = 1;
	else if (r < 128)
		hue_idx = 2;
	else if (r < 171)
		hue_idx = 3;
	else if (r < 213)
		hue_idx = 4;
	else
		hue_idx = 5;

	r = hs->hue_transfer[hue_idx][r];
	g = hs->lightness_transfer[hue_idx][g];
	b = hs->saturation_transfer[hue_idx][b];

	gimp_hls_to_rgb_int (&r, &g, &b);

	pixop->dest_pixel[RED_PIX] = r;
	pixop->dest_pixel[GREEN_PIX] = g;
	pixop->dest_pixel[BLUE_PIX] = b;
}


static void
adjust_colors_step (GthPixbufTask *pixop)
{
	AdjustData *data = pixop->data;
	int         channel;

	if (pixop->has_alpha)
		pixop->dest_pixel[ALPHA_PIX] = pixop->src_pixel[ALPHA_PIX];

	for (channel = RED_PIX; channel <= BLUE_PIX; channel++) {
		pixop->dest_pixel[channel] = pixop->src_pixel[channel];
		if (! pixbuf_cache_get (data->cache, channel + 1, &pixop->dest_pixel[channel])) {
			pixop->dest_pixel[channel] = gamma_correction (pixop->dest_pixel[channel], data->gamma);
			pixop->dest_pixel[channel] = interpolate_value (pixop->dest_pixel[channel], 0, data->brightness);
			pixop->dest_pixel[channel] = interpolate_value (pixop->dest_pixel[channel], 127, data->contrast);
			pixbuf_cache_set (data->cache, channel + 1, pixop->src_pixel[channel], pixop->dest_pixel[channel]);
		}
	}

#if 1
	if ((data->saturation != 0.0) || (data->hue != 0.0) || (data->lightness != 0))
		hue_saturation_step (pixop);
#endif

#if 0
	/* saturation */

	if (data->saturation != 0.0) {
		guchar min, max, lightness;

		max = MAX (pixop->dest_pixel[RED_PIX], pixop->dest_pixel[GREEN_PIX]);
		max = MAX (max, pixop->dest_pixel[BLUE_PIX]);
		min = MIN (pixop->dest_pixel[RED_PIX], pixop->dest_pixel[GREEN_PIX]);
		min = MIN (min, pixop->dest_pixel[BLUE_PIX]);
		lightness = (max + min) / 2;

		pixop->dest_pixel[RED_PIX] = interpolate_value (pixop->dest_pixel[RED_PIX], lightness, data->saturation);
		pixop->dest_pixel[GREEN_PIX] = interpolate_value (pixop->dest_pixel[GREEN_PIX], lightness, data->saturation);
		pixop->dest_pixel[BLUE_PIX] = interpolate_value (pixop->dest_pixel[BLUE_PIX], lightness, data->saturation);
	}
#endif
}


static void
adjust_colors_release (GthPixbufTask *pixop,
		       GError        *error)
{
	AdjustData *data = pixop->data;

	g_object_unref (data->viewer_page);
	pixbuf_cache_free (data->cache);
	g_free (data->hs);
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

	data->gamma = pow (10, - (gtk_adjustment_get_value (self->priv->gamma_adj) / 100.0));
	data->brightness = gtk_adjustment_get_value (self->priv->brightness_adj) / 100.0 * -1.0;
	data->contrast = gtk_adjustment_get_value (self->priv->contrast_adj) / 100.0 * -1.0;
	data->saturation = gtk_adjustment_get_value (self->priv->saturation_adj) / 100.0 * -1.0;
	data->hue = gtk_adjustment_get_value (self->priv->hue_adj);
	data->lightness = gtk_adjustment_get_value (self->priv->lightness_adj);

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
	self->priv->hue_adj = gimp_scale_entry_new (GET_WIDGET ("hue_hbox"), 0.0, -180.0, 180.0, 1.0, 10.0, 0);
	self->priv->lightness_adj = gimp_scale_entry_new (GET_WIDGET ("lightness_hbox"), 0.0, -180.0, 180.0, 1.0, 10.0, 0);

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
	g_signal_connect (G_OBJECT (self->priv->hue_adj),
			  "value-changed",
			  G_CALLBACK (value_changed_cb),
			  self);
	g_signal_connect (G_OBJECT (self->priv->lightness_adj),
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
