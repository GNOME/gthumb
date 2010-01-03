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
#include <gthumb.h>
#include <extensions/image_viewer/gth-image-viewer-page.h>
#include "gth-file-tool-crop.h"


#define GET_WIDGET(x) (_gtk_builder_get_widget (self->priv->builder, (x)))


typedef enum {
	GTH_CROP_RATIO_NONE = 0,
	GTH_CROP_RATIO_SQUARE,
	GTH_CROP_RATIO_IMAGE,
	GTH_CROP_RATIO_DISPLAY,
	GTH_CROP_RATIO_4_3,
	GTH_CROP_RATIO_4_6,
	GTH_CROP_RATIO_5_7,
	GTH_CROP_RATIO_8_10,
	GTH_CROP_RATIO_CUSTOM
} GthCropRatio;


static gpointer parent_class = NULL;


struct _GthFileToolCropPrivate {
	GdkPixbuf        *src_pixbuf;
	GtkBuilder       *builder;
	int               pixbuf_width;
	int               pixbuf_height;
	int               screen_width;
	int               screen_height;
	GthImageSelector *selector;
	GtkWidget        *ratio_combobox;
	GtkWidget        *crop_x_spinbutton;
	GtkWidget        *crop_y_spinbutton;
	GtkWidget        *crop_width_spinbutton;
	GtkWidget        *crop_height_spinbutton;
};


static void
gth_file_tool_crop_update_sensitivity (GthFileTool *base)
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
cancel_button_clicked_cb (GtkButton       *button,
			  GthFileToolCrop *self)
{
	gth_file_tool_hide_options (GTH_FILE_TOOL (self));
}


static void
crop_button_clicked_cb (GtkButton       *button,
			GthFileToolCrop *self)
{
	GdkRectangle  selection;
	GdkPixbuf    *new_pixbuf;

	gth_image_selector_get_selection (self->priv->selector, &selection);
	if ((selection.width == 0) || (selection.height == 0))
		return;

	new_pixbuf = gdk_pixbuf_new_subpixbuf (self->priv->src_pixbuf,
					       selection.x,
					       selection.y,
					       selection.width,
					       selection.height);
	if (new_pixbuf != NULL) {
		GtkWidget *window;
		GtkWidget *viewer_page;

		window = gth_file_tool_get_window (GTH_FILE_TOOL (self));
		viewer_page = gth_browser_get_viewer_page (GTH_BROWSER (window));
		gth_image_viewer_page_set_pixbuf (GTH_IMAGE_VIEWER_PAGE (viewer_page), new_pixbuf, TRUE);
		gth_file_tool_hide_options (GTH_FILE_TOOL (self));

		g_object_unref (new_pixbuf);
	}
}


static void
selection_x_value_changed_cb (GtkSpinButton    *spin,
			      GthFileToolCrop *self)
{
	gth_image_selector_set_selection_x (self->priv->selector, gtk_spin_button_get_value_as_int (spin));
}


static void
selection_y_value_changed_cb (GtkSpinButton    *spin,
			      GthFileToolCrop *self)
{
	gth_image_selector_set_selection_y (self->priv->selector, gtk_spin_button_get_value_as_int (spin));
}


static void
selection_width_value_changed_cb (GtkSpinButton    *spin,
				  GthFileToolCrop *self)
{
	gth_image_selector_set_selection_width (self->priv->selector, gtk_spin_button_get_value_as_int (spin));
}


static void
selection_height_value_changed_cb (GtkSpinButton    *spin,
				   GthFileToolCrop *self)
{
	gth_image_selector_set_selection_height (self->priv->selector, gtk_spin_button_get_value_as_int (spin));
}


static void
set_spin_range_value (GthFileToolCrop *self,
		      GtkWidget        *spin,
		      int               min,
		      int               max,
		      int               x)
{
	g_signal_handlers_block_by_data (G_OBJECT (spin), self);
	gtk_spin_button_set_range (GTK_SPIN_BUTTON (spin), min, max);
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (spin), x);
	g_signal_handlers_unblock_by_data (G_OBJECT (spin), self);
}


static void
selector_selection_changed_cb (GthImageSelector *selector,
			       GthFileToolCrop *self)
{
	GdkRectangle selection;
	int          min, max;

	gth_image_selector_get_selection (selector, &selection);

	min = 0;
	max = self->priv->pixbuf_width - selection.width;
	set_spin_range_value (self, self->priv->crop_x_spinbutton, min, max, selection.x);

	min = 0;
	max = self->priv->pixbuf_height - selection.height;
	set_spin_range_value (self, self->priv->crop_y_spinbutton, min, max, selection.y);

	min = 0;
	max = self->priv->pixbuf_width - selection.x;
	set_spin_range_value (self, self->priv->crop_width_spinbutton, min, max, selection.width);

	min = 0;
	max = self->priv->pixbuf_height - selection.y;
	set_spin_range_value (self, self->priv->crop_height_spinbutton, min, max, selection.height);

	gth_image_selector_set_mask_visible (selector, (selection.width != 0 || selection.height != 0));
}


static void
set_spin_value (GthFileToolCrop *self,
		GtkWidget        *spin,
		int               x)
{
	g_signal_handlers_block_by_data (G_OBJECT (spin), self);
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (spin), x);
	g_signal_handlers_unblock_by_data (G_OBJECT (spin), self);
}


static void
ratio_combobox_changed_cb (GtkComboBox      *combobox,
			   GthFileToolCrop *self)
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
	idx = gtk_combo_box_get_active (combobox);

	switch (idx) {
	case GTH_CROP_RATIO_NONE:
		use_ratio = FALSE;
		break;
	case GTH_CROP_RATIO_SQUARE:
		w = h = 1;
		break;
	case GTH_CROP_RATIO_IMAGE:
		w = self->priv->pixbuf_width;
		h = self->priv->pixbuf_height;
		break;
	case GTH_CROP_RATIO_DISPLAY:
		w = self->priv->screen_width;
		h = self->priv->screen_height;
		break;
	case GTH_CROP_RATIO_4_3:
		w = 4;
		h = 3;
		break;
	case GTH_CROP_RATIO_4_6:
		w = 4;
		h = 6;
		break;
	case GTH_CROP_RATIO_5_7:
		w = 5;
		h = 7;
		break;
	case GTH_CROP_RATIO_8_10:
		w = 8;
		h = 10;
		break;
	case GTH_CROP_RATIO_CUSTOM:
	default:
		w = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (ratio_w_spinbutton));
		h = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (ratio_h_spinbutton));
		break;
	}

	gtk_widget_set_sensitive (GET_WIDGET ("custom_ratio_box"), idx == GTH_CROP_RATIO_CUSTOM);
	gtk_widget_set_sensitive (GET_WIDGET ("invert_ratio_checkbutton"), use_ratio);
	set_spin_value (self, ratio_w_spinbutton, w);
	set_spin_value (self, ratio_h_spinbutton, h);
	gth_image_selector_set_ratio (GTH_IMAGE_SELECTOR (self->priv->selector),
				      use_ratio,
				      (double) w / h,
				      FALSE);
}


static void
update_ratio (GtkSpinButton    *spin,
	      GthFileToolCrop *self,
	      gboolean          swap_x_and_y_to_start)
{
	gboolean use_ratio;
	int      w, h;
	double   ratio;

	use_ratio = gtk_combo_box_get_active (GTK_COMBO_BOX (self->priv->ratio_combobox)) != GTH_CROP_RATIO_NONE;
	w = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (GET_WIDGET ("ratio_w_spinbutton")));
	h = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (GET_WIDGET ("ratio_h_spinbutton")));

	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("invert_ratio_checkbutton"))))
		ratio = (double) h / w;
	else
		ratio = (double) w / h;
	gth_image_selector_set_ratio (self->priv->selector,
				      use_ratio,
				      ratio,
				      swap_x_and_y_to_start);
}


static void
ratio_value_changed_cb (GtkSpinButton    *spin,
			GthFileToolCrop *self)
{
	update_ratio (spin, self, FALSE);
}


static void
invert_ratio_changed_cb (GtkSpinButton    *spin,
			 GthFileToolCrop *self)
{
	update_ratio (spin, self, TRUE);
}


static GtkWidget *
gth_file_tool_crop_get_options (GthFileTool *base)
{
	GthFileToolCrop *self;
	GtkWidget       *window;
	GtkWidget       *viewer_page;
	GtkWidget       *viewer;
	GtkWidget       *options;
	char            *text;

	self = (GthFileToolCrop *) base;

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

	self->priv->builder = _gtk_builder_new_from_file ("crop-options.ui", "file_tools");

	options = _gtk_builder_get_widget (self->priv->builder, "options");
	gtk_widget_show (options);
	self->priv->crop_x_spinbutton = _gtk_builder_get_widget (self->priv->builder, "crop_x_spinbutton");
	self->priv->crop_y_spinbutton = _gtk_builder_get_widget (self->priv->builder, "crop_y_spinbutton");
	self->priv->crop_width_spinbutton = _gtk_builder_get_widget (self->priv->builder, "crop_width_spinbutton");
	self->priv->crop_height_spinbutton = _gtk_builder_get_widget (self->priv->builder, "crop_height_spinbutton");

	self->priv->ratio_combobox = _gtk_combo_box_new_with_texts (_("None"), _("Square"), NULL);
	text = g_strdup_printf (_("%d x %d (Image)"), self->priv->pixbuf_width, self->priv->pixbuf_height);
	gtk_combo_box_append_text (GTK_COMBO_BOX (self->priv->ratio_combobox), text);
	g_free (text);
	text = g_strdup_printf (_("%d x %d (Screen)"), self->priv->screen_width, self->priv->screen_height);
	gtk_combo_box_append_text (GTK_COMBO_BOX (self->priv->ratio_combobox), text);
	g_free (text);
	_gtk_combo_box_append_texts (GTK_COMBO_BOX (self->priv->ratio_combobox),
				     _("4 x 3 (Book, DVD)"),
				     _("4 x 6 (Postcard)"),
				     _("5 x 7"),
				     _("8 x 10"),
				     _("Custom"),
				     NULL);
	gtk_widget_show (self->priv->ratio_combobox);
	gtk_box_pack_start (GTK_BOX (GET_WIDGET ("ratio_combobox_box")), self->priv->ratio_combobox, FALSE, FALSE, 0);

	g_signal_connect (GET_WIDGET ("crop_button"),
			  "clicked",
			  G_CALLBACK (crop_button_clicked_cb),
			  self);
	g_signal_connect (GET_WIDGET ("cancel_button"),
			  "clicked",
			  G_CALLBACK (cancel_button_clicked_cb),
			  self);
	g_signal_connect (G_OBJECT (self->priv->crop_x_spinbutton),
			  "value-changed",
			  G_CALLBACK (selection_x_value_changed_cb),
			  self);
	g_signal_connect (G_OBJECT (self->priv->crop_y_spinbutton),
			  "value-changed",
			  G_CALLBACK (selection_y_value_changed_cb),
			  self);
	g_signal_connect (G_OBJECT (self->priv->crop_width_spinbutton),
			  "value-changed",
			  G_CALLBACK (selection_width_value_changed_cb),
			  self);
	g_signal_connect (G_OBJECT (self->priv->crop_height_spinbutton),
			  "value-changed",
			  G_CALLBACK (selection_height_value_changed_cb),
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

	self->priv->selector = (GthImageSelector *) gth_image_selector_new (GTH_IMAGE_VIEWER (viewer), GTH_SELECTOR_TYPE_REGION);
	g_signal_connect (self->priv->selector,
			  "selection-changed",
			  G_CALLBACK (selector_selection_changed_cb),
			  self);
	/*g_signal_connect (self->priv->selector,
			  "mask_visibility_changed",
			  G_CALLBACK (selector_mask_visibility_changed_cb),
			  self);*/

	gtk_combo_box_set_active (GTK_COMBO_BOX (self->priv->ratio_combobox), 0);
	gth_image_viewer_set_tool (GTH_IMAGE_VIEWER (viewer), (GthImageViewerTool *) self->priv->selector);

	return options;
}


static void
gth_file_tool_crop_destroy_options (GthFileTool *base)
{
	GthFileToolCrop *self;
	GtkWidget       *window;
	GtkWidget       *viewer_page;
	GtkWidget       *viewer;

	self = (GthFileToolCrop *) base;

	window = gth_file_tool_get_window (GTH_FILE_TOOL (self));
	viewer_page = gth_browser_get_viewer_page (GTH_BROWSER (window));
	viewer = gth_image_viewer_page_get_image_viewer (GTH_IMAGE_VIEWER_PAGE (viewer_page));
	gth_image_viewer_set_tool (GTH_IMAGE_VIEWER (viewer), NULL);

	_g_object_unref (self->priv->src_pixbuf);
	_g_object_unref (self->priv->builder);
	_g_object_unref (self->priv->selector);

	self->priv->src_pixbuf = NULL;
	self->priv->builder = NULL;
	self->priv->selector = NULL;
}


static void
gth_file_tool_crop_activate (GthFileTool *base)
{
	gth_file_tool_show_options (base);
}


static void
gth_file_tool_crop_instance_init (GthFileToolCrop *self)
{
	self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, GTH_TYPE_FILE_TOOL_CROP, GthFileToolCropPrivate);
	gth_file_tool_construct (GTH_FILE_TOOL (self), GTK_STOCK_EDIT, _("Crop"), _("Crop"), TRUE);
}


static void
gth_file_tool_crop_finalize (GObject *object)
{
	GthFileToolCrop *self;

	g_return_if_fail (object != NULL);
	g_return_if_fail (GTH_IS_FILE_TOOL_CROP (object));

	self = (GthFileToolCrop *) object;

	_g_object_unref (self->priv->src_pixbuf);
	_g_object_unref (self->priv->selector);
	_g_object_unref (self->priv->builder);

	/* Chain up */
	G_OBJECT_CLASS (parent_class)->finalize (object);
}


static void
gth_file_tool_crop_class_init (GthFileToolCropClass *class)
{
	GObjectClass     *gobject_class;
	GthFileToolClass *file_tool_class;

	parent_class = g_type_class_peek_parent (class);
	g_type_class_add_private (class, sizeof (GthFileToolCropPrivate));

	gobject_class = (GObjectClass*) class;
	gobject_class->finalize = gth_file_tool_crop_finalize;

	file_tool_class = (GthFileToolClass *) class;
	file_tool_class->update_sensitivity = gth_file_tool_crop_update_sensitivity;
	file_tool_class->activate = gth_file_tool_crop_activate;
	file_tool_class->get_options = gth_file_tool_crop_get_options;
	file_tool_class->destroy_options = gth_file_tool_crop_destroy_options;
}


GType
gth_file_tool_crop_get_type (void) {
	static GType type_id = 0;
	if (type_id == 0) {
		static const GTypeInfo g_define_type_info = {
			sizeof (GthFileToolCropClass),
			(GBaseInitFunc) NULL,
			(GBaseFinalizeFunc) NULL,
			(GClassInitFunc) gth_file_tool_crop_class_init,
			(GClassFinalizeFunc) NULL,
			NULL,
			sizeof (GthFileToolCrop),
			0,
			(GInstanceInitFunc) gth_file_tool_crop_instance_init,
			NULL
		};
		type_id = g_type_register_static (GTH_TYPE_FILE_TOOL, "GthFileToolCrop", &g_define_type_info, 0);
	}
	return type_id;
}
