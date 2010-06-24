/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2010 Free Software Foundation, Inc.
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
#include "gth-file-tool-resize.h"
#include "preferences.h"


#define GET_WIDGET(x) (_gtk_builder_get_widget (self->priv->builder, (x)))
#define HIGH_QUALITY_INTERPOLATION GDK_INTERP_HYPER


static gpointer parent_class = NULL;


struct _GthFileToolResizePrivate {
	GdkPixbuf     *src_pixbuf;
	GdkPixbuf     *new_pixbuf;
	GtkBuilder    *builder;
	GtkWidget     *ratio_combobox;
	int            pixbuf_width;
	int            pixbuf_height;
	int            screen_width;
	int            screen_height;
	gboolean       fixed_aspect_ratio;
	double         aspect_ratio;
	int            new_width;
	int            new_height;
	GdkInterpType  interpolation;
	GthUnit        unit;
};


static void
gth_file_tool_resize_update_sensitivity (GthFileTool *base)
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
cancel_button_clicked_cb (GtkButton         *button,
			  GthFileToolResize *self)
{
	GtkWidget *window;
	GtkWidget *viewer_page;

	window = gth_file_tool_get_window (GTH_FILE_TOOL (self));
	viewer_page = gth_browser_get_viewer_page (GTH_BROWSER (window));
	gth_image_viewer_page_reset (GTH_IMAGE_VIEWER_PAGE (viewer_page));
	gth_file_tool_hide_options (GTH_FILE_TOOL (self));
}


static void
resize_button_clicked_cb (GtkButton       *button,
			GthFileToolResize *self)
{
	GtkWidget *window;
	GtkWidget *viewer_page;

	if (self->priv->new_pixbuf == NULL)
		return;

	window = gth_file_tool_get_window (GTH_FILE_TOOL (self));
	viewer_page = gth_browser_get_viewer_page (GTH_BROWSER (window));
	gth_image_viewer_page_set_pixbuf (GTH_IMAGE_VIEWER_PAGE (viewer_page), self->priv->new_pixbuf, TRUE);
	gth_file_tool_hide_options (GTH_FILE_TOOL (self));
}


static void
update_dimensione_info_label (GthFileToolResize *self,
			      const char        *id,
			      double             x,
			      double             y,
			      gboolean           as_int)
{
	char *s;

	if (as_int)
		s = g_strdup_printf ("%d×%d", (int) x, (int) y);
	else
		s = g_strdup_printf ("%.2f×%.2f", x, y);
	gtk_label_set_text (GTK_LABEL (GET_WIDGET (id)), s);

	g_free (s);
}


static void
update_pixbuf_size (GthFileToolResize *self)
{
	GtkWidget *window;
	GtkWidget *viewer_page;

	_g_object_unref (self->priv->new_pixbuf);
	self->priv->new_pixbuf = gdk_pixbuf_scale_simple (self->priv->src_pixbuf,
							  self->priv->new_width,
							  self->priv->new_height,
							  self->priv->interpolation);
	window = gth_file_tool_get_window (GTH_FILE_TOOL (self));
	viewer_page = gth_browser_get_viewer_page (GTH_BROWSER (window));
	gth_image_viewer_page_set_pixbuf (GTH_IMAGE_VIEWER_PAGE (viewer_page), self->priv->new_pixbuf, FALSE);

	update_dimensione_info_label (self,
				      "new_dimensions_label",
				      self->priv->new_width,
				      self->priv->new_height,
				      TRUE);
	update_dimensione_info_label (self,
				      "scale_factor_label",
				      (double) self->priv->new_width / self->priv->pixbuf_width,
				      (double) self->priv->new_height / self->priv->pixbuf_height,
				      FALSE);
}


static void
selection_width_value_changed_cb (GtkSpinButton     *spin,
				  GthFileToolResize *self)
{
	if (self->priv->unit == GTH_UNIT_PIXELS)
		self->priv->new_width = MAX (gtk_spin_button_get_value_as_int (spin), 1);
	else if (self->priv->unit == GTH_UNIT_PERCENTAGE)
		self->priv->new_width = MAX ((int) round ((gtk_spin_button_get_value (spin) / 100.0) * self->priv->pixbuf_width), 1);

	if (self->priv->fixed_aspect_ratio) {
		g_signal_handlers_block_by_data (GET_WIDGET ("resize_height_spinbutton"), self);
		self->priv->new_height = MAX ((int) round ((double) self->priv->new_width / self->priv->aspect_ratio), 1);
		if (self->priv->unit == GTH_UNIT_PIXELS)
			gtk_spin_button_set_value (GTK_SPIN_BUTTON (GET_WIDGET ("resize_height_spinbutton")), self->priv->new_height);
		else if (self->priv->unit == GTH_UNIT_PERCENTAGE)
			gtk_spin_button_set_value (GTK_SPIN_BUTTON (GET_WIDGET ("resize_height_spinbutton")), ((double) self->priv->new_height) / self->priv->pixbuf_height * 100.0);
		g_signal_handlers_unblock_by_data (GET_WIDGET ("resize_height_spinbutton"), self);
	}

	update_pixbuf_size (self);
}


static void
selection_height_value_changed_cb (GtkSpinButton     *spin,
				   GthFileToolResize *self)
{
	if (self->priv->unit == GTH_UNIT_PIXELS)
		self->priv->new_height = MAX (gtk_spin_button_get_value_as_int (spin), 1);
	else if (self->priv->unit == GTH_UNIT_PERCENTAGE)
		self->priv->new_height = MAX ((int) round ((gtk_spin_button_get_value (spin) / 100.0) * self->priv->pixbuf_height), 1);

	if (self->priv->fixed_aspect_ratio) {
		g_signal_handlers_block_by_data (GET_WIDGET ("resize_width_spinbutton"), self);
		self->priv->new_width = MAX ((int) round ((double) self->priv->new_height * self->priv->aspect_ratio), 1);
		if (self->priv->unit == GTH_UNIT_PIXELS)
			gtk_spin_button_set_value (GTK_SPIN_BUTTON (GET_WIDGET ("resize_width_spinbutton")), self->priv->new_width);
		else if (self->priv->unit == GTH_UNIT_PERCENTAGE)
			gtk_spin_button_set_value (GTK_SPIN_BUTTON (GET_WIDGET ("resize_width_spinbutton")), ((double) self->priv->new_width) / self->priv->pixbuf_width * 100.0);
		g_signal_handlers_unblock_by_data (GET_WIDGET ("resize_width_spinbutton"), self);
	}

	update_pixbuf_size (self);
}


static void
high_quality_checkbutton_toggled_cb (GtkToggleButton   *button,
				     GthFileToolResize *self)
{
	self->priv->interpolation = gtk_toggle_button_get_active (button) ? HIGH_QUALITY_INTERPOLATION : GDK_INTERP_NEAREST;
	update_pixbuf_size (self);
}


static void
unit_combobox_changed_cb (GtkComboBox       *combobox,
			  GthFileToolResize *self)
{
	g_signal_handlers_block_by_data (GET_WIDGET ("resize_width_spinbutton"), self);
	g_signal_handlers_block_by_data (GET_WIDGET ("resize_height_spinbutton"), self);

	self->priv->unit = gtk_combo_box_get_active (combobox);
	if (self->priv->unit == GTH_UNIT_PERCENTAGE) {
		double p;

		gtk_spin_button_set_digits (GTK_SPIN_BUTTON (GET_WIDGET ("resize_width_spinbutton")), 2);
		gtk_spin_button_set_digits (GTK_SPIN_BUTTON (GET_WIDGET ("resize_height_spinbutton")), 2);

		p = ((double) self->priv->new_width) / self->priv->pixbuf_width * 100.0;
		gtk_spin_button_set_value (GTK_SPIN_BUTTON (GET_WIDGET ("resize_width_spinbutton")), p);
		p = ((double) self->priv->new_height) / self->priv->pixbuf_height * 100.0;
		gtk_spin_button_set_value (GTK_SPIN_BUTTON (GET_WIDGET ("resize_height_spinbutton")), p);
	}
	else if (self->priv->unit == GTH_UNIT_PIXELS) {
		gtk_spin_button_set_digits (GTK_SPIN_BUTTON (GET_WIDGET ("resize_width_spinbutton")), 0);
		gtk_spin_button_set_digits (GTK_SPIN_BUTTON (GET_WIDGET ("resize_height_spinbutton")), 0);
		gtk_spin_button_set_value (GTK_SPIN_BUTTON (GET_WIDGET ("resize_width_spinbutton")), self->priv->new_width);
		gtk_spin_button_set_value (GTK_SPIN_BUTTON (GET_WIDGET ("resize_height_spinbutton")), self->priv->new_height);
	}

	g_signal_handlers_unblock_by_data (GET_WIDGET ("resize_width_spinbutton"), self);
	g_signal_handlers_unblock_by_data (GET_WIDGET ("resize_height_spinbutton"), self);

	selection_width_value_changed_cb (GTK_SPIN_BUTTON (GET_WIDGET ("resize_width_spinbutton")), self);
}


static void
set_spin_value (GthFileToolResize *self,
		GtkWidget         *spin,
		int                x)
{
	g_signal_handlers_block_by_data (G_OBJECT (spin), self);
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (spin), x);
	g_signal_handlers_unblock_by_data (G_OBJECT (spin), self);
}


static void
ratio_combobox_changed_cb (GtkComboBox       *combobox,
			   GthFileToolResize *self)
{
	GtkWidget *ratio_w_spinbutton;
	GtkWidget *ratio_h_spinbutton;
	int        idx;
	int        w, h;
	gboolean   use_ratio;

	ratio_w_spinbutton = GET_WIDGET ("ratio_w_spinbutton");
	ratio_h_spinbutton = GET_WIDGET ("ratio_h_spinbutton");
	w = h = 1;
	use_ratio = TRUE;
	idx = gtk_combo_box_get_active (GTK_COMBO_BOX (self->priv->ratio_combobox));

	switch (idx) {
	case GTH_ASPECT_RATIO_NONE:
		use_ratio = FALSE;
		break;
	case GTH_ASPECT_RATIO_SQUARE:
		w = h = 1;
		break;
	case GTH_ASPECT_RATIO_IMAGE:
		w = self->priv->pixbuf_width;
		h = self->priv->pixbuf_height;
		break;
	case GTH_ASPECT_RATIO_DISPLAY:
		w = self->priv->screen_width;
		h = self->priv->screen_height;
		break;
	case GTH_ASPECT_RATIO_5x4:
		w = 5;
		h = 4;
		break;
	case GTH_ASPECT_RATIO_4x3:
		w = 4;
		h = 3;
		break;
	case GTH_ASPECT_RATIO_7x5:
		w = 7;
		h = 5;
		break;
	case GTH_ASPECT_RATIO_3x2:
		w = 3;
		h = 2;
		break;
	case GTH_ASPECT_RATIO_16x10:
		w = 16;
		h = 10;
		break;
	case GTH_ASPECT_RATIO_16x9:
		w = 16;
		h = 9;
		break;
	case GTH_ASPECT_RATIO_185x100:
		w = 185;
		h = 100;
		break;
	case GTH_ASPECT_RATIO_239x100:
		w = 239;
		h = 100;
		break;
	case GTH_ASPECT_RATIO_CUSTOM:
	default:
		w = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (ratio_w_spinbutton));
		h = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (ratio_h_spinbutton));
		break;
	}

	gtk_widget_set_sensitive (GET_WIDGET ("custom_ratio_box"), idx == GTH_ASPECT_RATIO_CUSTOM);
	gtk_widget_set_sensitive (GET_WIDGET ("invert_ratio_checkbutton"), use_ratio);
	set_spin_value (self, ratio_w_spinbutton, w);
	set_spin_value (self, ratio_h_spinbutton, h);

	self->priv->fixed_aspect_ratio = use_ratio;
	self->priv->aspect_ratio = (double) w / h;
	selection_width_value_changed_cb (GTK_SPIN_BUTTON (GET_WIDGET ("resize_width_spinbutton")), self);
}


static void
update_ratio (GtkSpinButton     *spin,
	      GthFileToolResize *self,
	      gboolean           swap_x_and_y_to_start)
{
	int w, h;

	self->priv->fixed_aspect_ratio = gtk_combo_box_get_active (GTK_COMBO_BOX (self->priv->ratio_combobox)) != GTH_ASPECT_RATIO_NONE;
	w = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (GET_WIDGET ("ratio_w_spinbutton")));
	h = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (GET_WIDGET ("ratio_h_spinbutton")));

	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("invert_ratio_checkbutton"))))
		self->priv->aspect_ratio = (double) h / w;
	else
		self->priv->aspect_ratio = (double) w / h;
	selection_width_value_changed_cb (GTK_SPIN_BUTTON (GET_WIDGET ("resize_width_spinbutton")), self);
}


static void
ratio_value_changed_cb (GtkSpinButton     *spin,
			GthFileToolResize *self)
{
	update_ratio (spin, self, FALSE);
}


static void
invert_ratio_changed_cb (GtkSpinButton     *spin,
			 GthFileToolResize *self)
{
	update_ratio (spin, self, TRUE);
}


static GtkWidget *
gth_file_tool_resize_get_options (GthFileTool *base)
{
	GthFileToolResize *self;
	GtkWidget         *window;
	GtkWidget         *viewer_page;
	GtkWidget         *viewer;
	GtkWidget         *options;
	char              *text;

	self = (GthFileToolResize *) base;

	window = gth_file_tool_get_window (base);
	viewer_page = gth_browser_get_viewer_page (GTH_BROWSER (window));
	if (! GTH_IS_IMAGE_VIEWER_PAGE (viewer_page))
		return NULL;

	_g_object_unref (self->priv->src_pixbuf);

	viewer = gth_image_viewer_page_get_image_viewer (GTH_IMAGE_VIEWER_PAGE (viewer_page));
	self->priv->src_pixbuf = gth_image_viewer_get_current_pixbuf (GTH_IMAGE_VIEWER (viewer));
	if (self->priv->src_pixbuf == NULL)
		return NULL;

	g_object_ref (self->priv->src_pixbuf);

	self->priv->pixbuf_width = gdk_pixbuf_get_width (self->priv->src_pixbuf);
	self->priv->pixbuf_height = gdk_pixbuf_get_height (self->priv->src_pixbuf);
	_gtk_widget_get_screen_size (window, &self->priv->screen_width, &self->priv->screen_height);
	self->priv->new_pixbuf = NULL;
	self->priv->new_width = self->priv->pixbuf_width;
	self->priv->new_height = self->priv->pixbuf_height;
	self->priv->interpolation = eel_gconf_get_boolean (PREF_RESIZE_HIGH_QUALITY, TRUE) ? HIGH_QUALITY_INTERPOLATION : GDK_INTERP_NEAREST;
	self->priv->unit = eel_gconf_get_enum (PREF_RESIZE_UNIT, GTH_TYPE_UNIT, GTH_UNIT_PERCENTAGE);
	self->priv->builder = _gtk_builder_new_from_file ("resize-options.ui", "file_tools");

	update_dimensione_info_label (self,
				      "original_dimensions_label",
				      self->priv->pixbuf_width,
				      self->priv->pixbuf_height,
				      TRUE);

	options = _gtk_builder_get_widget (self->priv->builder, "options");
	gtk_widget_show (options);

	if (self->priv->unit == GTH_UNIT_PIXELS) {
		gtk_spin_button_set_digits (GTK_SPIN_BUTTON (GET_WIDGET ("resize_width_spinbutton")), 0);
		gtk_spin_button_set_digits (GTK_SPIN_BUTTON (GET_WIDGET ("resize_height_spinbutton")), 0);
		gtk_spin_button_set_value (GTK_SPIN_BUTTON (GET_WIDGET ("resize_width_spinbutton")), self->priv->pixbuf_width);
		gtk_spin_button_set_value (GTK_SPIN_BUTTON (GET_WIDGET ("resize_height_spinbutton")), self->priv->pixbuf_height);
	}
	else if (self->priv->unit == GTH_UNIT_PERCENTAGE) {
		gtk_spin_button_set_digits (GTK_SPIN_BUTTON (GET_WIDGET ("resize_width_spinbutton")), 2);
		gtk_spin_button_set_digits (GTK_SPIN_BUTTON (GET_WIDGET ("resize_height_spinbutton")), 2);
		gtk_spin_button_set_value (GTK_SPIN_BUTTON (GET_WIDGET ("resize_width_spinbutton")), 100.0);
		gtk_spin_button_set_value (GTK_SPIN_BUTTON (GET_WIDGET ("resize_height_spinbutton")), 100.0);
	}
	gtk_combo_box_set_active (GTK_COMBO_BOX (GET_WIDGET ("unit_combobox")), self->priv->unit);

	self->priv->ratio_combobox = _gtk_combo_box_new_with_texts (_("None"), _("Square"), NULL);
	text = g_strdup_printf (_("%d x %d (Image)"), self->priv->pixbuf_width, self->priv->pixbuf_height);
	gtk_combo_box_append_text (GTK_COMBO_BOX (self->priv->ratio_combobox), text);
	g_free (text);
	text = g_strdup_printf (_("%d x %d (Screen)"), self->priv->screen_width, self->priv->screen_height);
	gtk_combo_box_append_text (GTK_COMBO_BOX (self->priv->ratio_combobox), text);
	g_free (text);
	_gtk_combo_box_append_texts (GTK_COMBO_BOX (self->priv->ratio_combobox),
				     _("5:4"),
				     _("4:3 (DVD, Book)"),
				     _("7:5"),
				     _("3:2 (Postcard)"),
				     _("16:10"),
				     _("16:9 (DVD)"),
				     _("1.85:1"),
				     _("2.39:1"),
				     _("Custom"),
				     NULL);
	gtk_widget_show (self->priv->ratio_combobox);
	gtk_box_pack_start (GTK_BOX (GET_WIDGET ("ratio_combobox_box")), self->priv->ratio_combobox, FALSE, FALSE, 0);

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("high_quality_checkbutton")), self->priv->interpolation != GDK_INTERP_NEAREST);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("invert_ratio_checkbutton")), eel_gconf_get_boolean (PREF_RESIZE_ASPECT_RATIO_INVERT, FALSE));

	gtk_spin_button_set_value (GTK_SPIN_BUTTON (GET_WIDGET ("ratio_w_spinbutton")), MAX (eel_gconf_get_integer (PREF_RESIZE_ASPECT_RATIO_WIDTH, 1), 1));
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (GET_WIDGET ("ratio_h_spinbutton")), MAX (eel_gconf_get_integer (PREF_RESIZE_ASPECT_RATIO_HEIGHT, 1), 1));

	g_signal_connect (GET_WIDGET ("resize_button"),
			  "clicked",
			  G_CALLBACK (resize_button_clicked_cb),
			  self);
	g_signal_connect (GET_WIDGET ("cancel_button"),
			  "clicked",
			  G_CALLBACK (cancel_button_clicked_cb),
			  self);
	g_signal_connect (GET_WIDGET ("resize_width_spinbutton"),
			  "value-changed",
			  G_CALLBACK (selection_width_value_changed_cb),
			  self);
	g_signal_connect (GET_WIDGET ("resize_height_spinbutton"),
			  "value-changed",
			  G_CALLBACK (selection_height_value_changed_cb),
			  self);
	g_signal_connect (GET_WIDGET ("high_quality_checkbutton"),
			  "toggled",
			  G_CALLBACK (high_quality_checkbutton_toggled_cb),
			  self);
	g_signal_connect (GET_WIDGET ("unit_combobox"),
			  "changed",
			  G_CALLBACK (unit_combobox_changed_cb),
			  self);
	g_signal_connect (self->priv->ratio_combobox,
			  "changed",
			  G_CALLBACK (ratio_combobox_changed_cb),
			  self);
	g_signal_connect (GET_WIDGET ("ratio_w_spinbutton"),
			  "value_changed",
			  G_CALLBACK (ratio_value_changed_cb),
			  self);
	g_signal_connect (GET_WIDGET ("ratio_h_spinbutton"),
			  "value_changed",
			  G_CALLBACK (ratio_value_changed_cb),
			  self);
	g_signal_connect (GET_WIDGET ("invert_ratio_checkbutton"),
			  "toggled",
			  G_CALLBACK (invert_ratio_changed_cb),
			  self);

	gtk_combo_box_set_active (GTK_COMBO_BOX (self->priv->ratio_combobox), eel_gconf_get_enum (PREF_RESIZE_ASPECT_RATIO, GTH_TYPE_ASPECT_RATIO, GTH_ASPECT_RATIO_IMAGE));

	return options;
}


static void
gth_file_tool_resize_destroy_options (GthFileTool *base)
{
	GthFileToolResize *self;
	GtkWidget         *window;
	GtkWidget         *viewer_page;
	GtkWidget         *viewer;

	self = (GthFileToolResize *) base;

	if (self->priv->builder != NULL) {
		/* save the dialog options */

		eel_gconf_set_enum (PREF_RESIZE_UNIT, GTH_TYPE_UNIT, gtk_combo_box_get_active (GTK_COMBO_BOX (GET_WIDGET ("unit_combobox"))));
		eel_gconf_set_integer (PREF_RESIZE_ASPECT_RATIO_WIDTH, gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (GET_WIDGET ("ratio_w_spinbutton"))));
		eel_gconf_set_integer (PREF_RESIZE_ASPECT_RATIO_HEIGHT, gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (GET_WIDGET ("ratio_h_spinbutton"))));
		eel_gconf_set_enum (PREF_RESIZE_ASPECT_RATIO, GTH_TYPE_ASPECT_RATIO, gtk_combo_box_get_active (GTK_COMBO_BOX (self->priv->ratio_combobox)));
		eel_gconf_set_boolean (PREF_RESIZE_ASPECT_RATIO_INVERT, gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("invert_ratio_checkbutton"))));
		eel_gconf_set_boolean (PREF_RESIZE_HIGH_QUALITY, gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("high_quality_checkbutton"))));

		/* destroy the options data */

		_g_object_unref (self->priv->new_pixbuf);
		_g_object_unref (self->priv->src_pixbuf);
		_g_object_unref (self->priv->builder);
		self->priv->new_pixbuf = NULL;
		self->priv->src_pixbuf = NULL;
		self->priv->builder = NULL;
	}

	window = gth_file_tool_get_window (GTH_FILE_TOOL (self));
	viewer_page = gth_browser_get_viewer_page (GTH_BROWSER (window));
	viewer = gth_image_viewer_page_get_image_viewer (GTH_IMAGE_VIEWER_PAGE (viewer_page));
	gth_image_viewer_set_tool (GTH_IMAGE_VIEWER (viewer), NULL);
}


static void
gth_file_tool_resize_activate (GthFileTool *base)
{
	gth_file_tool_show_options (base);
}


static void
gth_file_tool_resize_instance_init (GthFileToolResize *self)
{
	self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, GTH_TYPE_FILE_TOOL_RESIZE, GthFileToolResizePrivate);
	gth_file_tool_construct (GTH_FILE_TOOL (self), "tool-resize", _("Resize..."), _("Resize"), TRUE);
}


static void
gth_file_tool_resize_finalize (GObject *object)
{
	GthFileToolResize *self;

	g_return_if_fail (object != NULL);
	g_return_if_fail (GTH_IS_FILE_TOOL_RESIZE (object));

	self = (GthFileToolResize *) object;

	_g_object_unref (self->priv->new_pixbuf);
	_g_object_unref (self->priv->src_pixbuf);
	_g_object_unref (self->priv->builder);

	/* Chain up */
	G_OBJECT_CLASS (parent_class)->finalize (object);
}


static void
gth_file_tool_resize_class_init (GthFileToolResizeClass *class)
{
	GObjectClass     *gobject_class;
	GthFileToolClass *file_tool_class;

	parent_class = g_type_class_peek_parent (class);
	g_type_class_add_private (class, sizeof (GthFileToolResizePrivate));

	gobject_class = (GObjectClass*) class;
	gobject_class->finalize = gth_file_tool_resize_finalize;

	file_tool_class = (GthFileToolClass *) class;
	file_tool_class->update_sensitivity = gth_file_tool_resize_update_sensitivity;
	file_tool_class->activate = gth_file_tool_resize_activate;
	file_tool_class->get_options = gth_file_tool_resize_get_options;
	file_tool_class->destroy_options = gth_file_tool_resize_destroy_options;
}


GType
gth_file_tool_resize_get_type (void) {
	static GType type_id = 0;
	if (type_id == 0) {
		static const GTypeInfo g_define_type_info = {
			sizeof (GthFileToolResizeClass),
			(GBaseInitFunc) NULL,
			(GBaseFinalizeFunc) NULL,
			(GClassInitFunc) gth_file_tool_resize_class_init,
			(GClassFinalizeFunc) NULL,
			NULL,
			sizeof (GthFileToolResize),
			0,
			(GInstanceInitFunc) gth_file_tool_resize_instance_init,
			NULL
		};
		type_id = g_type_register_static (GTH_TYPE_FILE_TOOL, "GthFileToolResize", &g_define_type_info, 0);
	}
	return type_id;
}
