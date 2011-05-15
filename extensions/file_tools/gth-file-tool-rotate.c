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

#include <math.h>
#include <config.h>
#include <gthumb.h>
#include <extensions/image_viewer/gth-image-viewer-page.h>
#include "enum-types.h"
#include "gdk-pixbuf-rotate.h"
#include "gth-file-tool-rotate.h"


#define GET_WIDGET(x) (_gtk_builder_get_widget (self->priv->builder, (x)))
#define APPLY_DELAY 150


static gpointer parent_class = NULL;


struct _GthFileToolRotatePrivate {
	GdkPixbuf        *src_pixbuf;
	GdkPixbuf        *rotate_pixbuf;
	gboolean          has_alpha;
	GtkBuilder       *builder;
	GtkWidget        *options;
	GtkAdjustment    *rotation_angle_adj;
	GtkWidget        *high_quality;
	GtkWidget        *background_colorbutton;
	GtkWidget        *background_transparent;
	GtkWidget        *enable_guided_crop;
	gboolean          crop_enabled;
	GtkWidget        *show_grid;
	GtkWidget        *keep_aspect_ratio;
	GtkAdjustment    *crop_p1_adj;
	GtkAdjustment    *crop_p2_adj;
	double            crop_p1_plus_p2;
	GtkWidget        *crop_grid;
	GdkRectangle      crop_region;
	GthImageSelector *selector_crop;
	GthImageSelector *selector_align;
	guint             selector_align_direction;
	guint             selector_align_point;
	GdkPoint          align_points[2];
	guint             apply_event;
};


static void
update_crop_parameters (GthFileToolRotate *self)
{
	GtkWidget *window;
	GtkWidget *viewer_page;
	GtkWidget *viewer;
	double    rotation_angle;
	gboolean  keep_aspect_ratio;
	double    crop_p1;
	double    crop_p_min;

	window = gth_file_tool_get_window (GTH_FILE_TOOL (self));
	viewer_page = gth_browser_get_viewer_page (GTH_BROWSER (window));
	viewer = gth_image_viewer_page_get_image_viewer (GTH_IMAGE_VIEWER_PAGE (viewer_page));

	gtk_widget_set_sensitive (GTK_WIDGET (self->priv->enable_guided_crop), TRUE);

	self->priv->crop_enabled = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (self->priv->enable_guided_crop));

	if (self->priv->crop_enabled) {
		gtk_widget_set_sensitive (GET_WIDGET ("crop_options_table"), TRUE);

		rotation_angle = gtk_adjustment_get_value (self->priv->rotation_angle_adj);
		keep_aspect_ratio = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (self->priv->keep_aspect_ratio));

		if (keep_aspect_ratio) {
			gtk_widget_set_sensitive (GET_WIDGET ("crop_p2_label"), FALSE);
			gtk_widget_set_sensitive (GET_WIDGET ("crop_p2_hbox"), FALSE);

			_gdk_pixbuf_rotate_get_cropping_parameters (self->priv->src_pixbuf,
								    rotation_angle,
								    &self->priv->crop_p1_plus_p2,
								    &crop_p_min);

			/* This centers the cropping region in the middle of the rotated image */

			crop_p1 = self->priv->crop_p1_plus_p2 / 2.0;

			gtk_adjustment_set_lower (self->priv->crop_p1_adj, MAX (crop_p_min, 0.0));
			gtk_adjustment_set_lower (self->priv->crop_p2_adj, MAX (crop_p_min, 0.0));

			gtk_adjustment_set_upper (self->priv->crop_p1_adj, MIN (self->priv->crop_p1_plus_p2 - crop_p_min, 1.0));
			gtk_adjustment_set_upper (self->priv->crop_p2_adj, MIN (self->priv->crop_p1_plus_p2 - crop_p_min, 1.0));

			gtk_adjustment_set_value (self->priv->crop_p1_adj, crop_p1);
		}
		else {
			self->priv->crop_p1_plus_p2 = 0;

			gtk_widget_set_sensitive (GET_WIDGET ("crop_p2_label"), TRUE);
			gtk_widget_set_sensitive (GET_WIDGET ("crop_p2_hbox"), TRUE);

			gtk_adjustment_set_lower (self->priv->crop_p1_adj, 0.0);
			gtk_adjustment_set_lower (self->priv->crop_p2_adj, 0.0);

			gtk_adjustment_set_upper (self->priv->crop_p1_adj, 1.0);
			gtk_adjustment_set_upper (self->priv->crop_p2_adj, 1.0);
		}

		gth_image_viewer_set_tool (GTH_IMAGE_VIEWER (viewer), (GthImageViewerTool *) self->priv->selector_crop);
	}
	else {
		gth_image_viewer_set_tool (GTH_IMAGE_VIEWER (viewer), NULL);
		gtk_widget_set_sensitive (GET_WIDGET ("crop_options_table"), FALSE);
	}
}


static void
update_crop_region (GthFileToolRotate *self)
{
	GtkWidget *window;
	GtkWidget *viewer_page;
	GtkWidget *viewer;
	double     rotation_angle;
	double     crop_p1;
	double     crop_p2;

	window = gth_file_tool_get_window (GTH_FILE_TOOL (self));
	viewer_page = gth_browser_get_viewer_page (GTH_BROWSER (window));
	viewer = gth_image_viewer_page_get_image_viewer (GTH_IMAGE_VIEWER_PAGE (viewer_page));

	rotation_angle = gtk_adjustment_get_value (self->priv->rotation_angle_adj);

	crop_p1 = gtk_adjustment_get_value (self->priv->crop_p1_adj);
	crop_p2 = gtk_adjustment_get_value (self->priv->crop_p2_adj);

	_gdk_pixbuf_rotate_get_cropping_region (self->priv->src_pixbuf,
						rotation_angle,
						crop_p1,
						crop_p2,
						&self->priv->crop_region);

	gth_image_selector_set_selection (self->priv->selector_crop, self->priv->crop_region);
}


static void
update_crop_grid (GthFileToolRotate *self)
{
	gth_image_selector_set_grid_type (self->priv->selector_crop, gtk_combo_box_get_active (GTK_COMBO_BOX (self->priv->crop_grid)));
}


static void
align_begin (GthFileToolRotate *self)
{
	GtkWidget *window;
	GtkWidget *viewer_page;
	GtkWidget *viewer;

	window = gth_file_tool_get_window (GTH_FILE_TOOL (self));
	viewer_page = gth_browser_get_viewer_page (GTH_BROWSER (window));
	viewer = gth_image_viewer_page_get_image_viewer (GTH_IMAGE_VIEWER_PAGE (viewer_page));

	self->priv->selector_align_point = 0;

	gtk_widget_set_sensitive (self->priv->options, FALSE);

	gth_image_viewer_page_set_pixbuf (GTH_IMAGE_VIEWER_PAGE (viewer_page), self->priv->src_pixbuf, FALSE);

	gth_image_viewer_set_tool (GTH_IMAGE_VIEWER (viewer), (GthImageViewerTool *) self->priv->selector_align);
}


static void
align_end (GthFileToolRotate *self)
{
	GtkWidget *window;
	GtkWidget *viewer_page;
	GtkWidget *viewer;
	double     angle;

	window = gth_file_tool_get_window (GTH_FILE_TOOL (self));
	viewer_page = gth_browser_get_viewer_page (GTH_BROWSER (window));
	viewer = gth_image_viewer_page_get_image_viewer (GTH_IMAGE_VIEWER_PAGE (viewer_page));

	angle = _gdk_pixbuf_rotate_get_align_angle (self->priv->selector_align_direction == 1,
						    self->priv->align_points[0],
						    self->priv->align_points[1]),

	self->priv->selector_align_direction = 0;
	self->priv->selector_align_point = 0;

	gtk_widget_set_sensitive (self->priv->options, TRUE);

	if (angle == gtk_adjustment_get_value (self->priv->rotation_angle_adj)) {

		/* We already have the pixmap ready */

		gth_image_viewer_page_set_pixbuf (GTH_IMAGE_VIEWER_PAGE (viewer_page), self->priv->rotate_pixbuf, FALSE);

		update_crop_parameters (self);
		update_crop_region (self);
	}
	else {
		/* This triggers the rotation and the update of crop parameters */

		gtk_adjustment_set_value (self->priv->rotation_angle_adj, angle);
	}
}


static gboolean
apply_cb (gpointer user_data)
{
	GthFileToolRotate *self = user_data;
	GtkWidget         *window;
	GtkWidget         *viewer_page;
	double             rotation_angle;
	gboolean           high_quality;
	GdkColor           background_color;
	guchar             r0, g0, b0, a0;

	if (self->priv->apply_event != 0) {
		g_source_remove (self->priv->apply_event);
		self->priv->apply_event = 0;
	}

	window = gth_file_tool_get_window (GTH_FILE_TOOL (self));
	viewer_page = gth_browser_get_viewer_page (GTH_BROWSER (window));

	rotation_angle = gtk_adjustment_get_value (self->priv->rotation_angle_adj);
	high_quality = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (self->priv->high_quality));

	if (! gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (self->priv->background_transparent))) {

		gtk_color_button_get_color (GTK_COLOR_BUTTON (self->priv->background_colorbutton), &background_color);

		r0 = background_color.red >> 8;
		g0 = background_color.green >> 8;
		b0 = background_color.blue >> 8;

		if (self->priv->has_alpha)
			a0 = gtk_color_button_get_alpha (GTK_COLOR_BUTTON (self->priv->background_colorbutton)) >> 8;
		else
			a0 = 0;
	}
	else {
		r0 = 0;
		g0 = 0;
		b0 = 0;
		a0 = 0;
	}

	_g_object_unref (self->priv->rotate_pixbuf);
	self->priv->rotate_pixbuf = _gdk_pixbuf_rotate (self->priv->src_pixbuf, rotation_angle, high_quality, r0, g0, b0, a0);

	gth_image_viewer_page_set_pixbuf (GTH_IMAGE_VIEWER_PAGE (viewer_page), self->priv->rotate_pixbuf, FALSE);

	update_crop_parameters (self);
	update_crop_region (self);

	return FALSE;
}


static void
gth_file_tool_rotate_update_sensitivity (GthFileTool *base)
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
reset_button_clicked_cb (GtkButton         *button,
			 GthFileToolRotate *self)
{
	gtk_adjustment_set_value (self->priv->rotation_angle_adj, 0.0);
}


static void
align_h_button_clicked_cb (GtkButton         *button,
			   GthFileToolRotate *self)
{
	self->priv->selector_align_direction = 0;

	align_begin (self);
}


static void
align_v_button_clicked_cb (GtkButton         *button,
			   GthFileToolRotate *self)
{
	self->priv->selector_align_direction = 1;

	align_begin (self);
}


static void
apply_button_clicked_cb (GtkButton         *button,
			 GthFileToolRotate *self)
{
	if (self->priv->rotate_pixbuf != self->priv->src_pixbuf) {
		GtkWidget *window;
		GtkWidget *viewer_page;
		GdkPixbuf *result;

		window = gth_file_tool_get_window (GTH_FILE_TOOL (self));
		viewer_page = gth_browser_get_viewer_page (GTH_BROWSER (window));

		if (self->priv->crop_enabled
		    && ((self->priv->crop_region.x != 0)
		        || (self->priv->crop_region.y != 0)
		        || (self->priv->crop_region.width != gdk_pixbuf_get_width (self->priv->rotate_pixbuf))
		        || (self->priv->crop_region.height != gdk_pixbuf_get_height (self->priv->rotate_pixbuf))))
		{
			result = gdk_pixbuf_new_subpixbuf (self->priv->rotate_pixbuf,
							   self->priv->crop_region.x,
							   self->priv->crop_region.y,
							   self->priv->crop_region.width,
							   self->priv->crop_region.height);
		}
		else
			result = g_object_ref (self->priv->rotate_pixbuf);

		gth_image_viewer_page_set_pixbuf (GTH_IMAGE_VIEWER_PAGE (viewer_page), result, TRUE);

		g_object_unref (result);
	}

	gth_file_tool_hide_options (GTH_FILE_TOOL (self));
}


static void
cancel_button_clicked_cb (GtkButton         *button,
			  GthFileToolRotate *self)
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
crop_settings_changed_cb (GtkAdjustment     *adj,
		          GthFileToolRotate *self)
{
	update_crop_parameters (self);
	update_crop_region (self);
}


static void
crop_parameters_changed_cb (GtkAdjustment     *adj,
		            GthFileToolRotate *self)
{
	if ((adj == self->priv->crop_p1_adj) && gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (self->priv->keep_aspect_ratio)))
		gtk_adjustment_set_value (self->priv->crop_p2_adj,
					  self->priv->crop_p1_plus_p2 - gtk_adjustment_get_value (adj));
	else
		update_crop_region (self);
}


static void
crop_grid_changed_cb (GtkAdjustment     *adj,
		      GthFileToolRotate *self)
{
	update_crop_grid (self);
}


static void
value_changed_cb (GtkAdjustment     *adj,
		  GthFileToolRotate *self)
{
	if (self->priv->apply_event != 0) {
		g_source_remove (self->priv->apply_event);
		self->priv->apply_event = 0;
	}

	self->priv->apply_event = g_timeout_add (APPLY_DELAY, apply_cb, self);
}


static void
background_color_changed_cb (GtkAdjustment     *adj,
		             GthFileToolRotate *self)
{
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (self->priv->background_transparent), FALSE);

	value_changed_cb (adj, self);
}


static void
selector_selected_cb (GthImageSelector  *selector,
		      int                x,
		      int                y,
		      GthFileToolRotate *self)
{
	guint i;

	i = self->priv->selector_align_point++;

	self->priv->align_points[i].x = x;
	self->priv->align_points[i].y = y;

	if (self->priv->selector_align_point == 2)
		align_end (self);
}


static GtkWidget *
gth_file_tool_rotate_get_options (GthFileTool *base)
{
	GthFileToolRotate *self;
	GtkWidget         *window;
	GtkWidget         *viewer_page;
	GtkWidget         *viewer;

	self = (GthFileToolRotate *) base;

	window = gth_file_tool_get_window (base);
	viewer_page = gth_browser_get_viewer_page (GTH_BROWSER (window));
	if (! GTH_IS_IMAGE_VIEWER_PAGE (viewer_page))
		return NULL;

	_g_clear_object (&self->priv->src_pixbuf);
	_g_clear_object (&self->priv->rotate_pixbuf);

	viewer = gth_image_viewer_page_get_image_viewer (GTH_IMAGE_VIEWER_PAGE (viewer_page));
	self->priv->src_pixbuf = gth_image_viewer_get_current_pixbuf (GTH_IMAGE_VIEWER (viewer));
	if (self->priv->src_pixbuf == NULL)
		return NULL;

	self->priv->rotate_pixbuf = g_object_ref (self->priv->src_pixbuf);

	self->priv->builder = _gtk_builder_new_from_file ("rotate-options.ui", "file_tools");

	self->priv->options = _gtk_builder_get_widget (self->priv->builder, "options");
	gtk_widget_show (self->priv->options);

	self->priv->rotation_angle_adj = gimp_scale_entry_new (GET_WIDGET ("rotation_angle_hbox"),
							       GTK_LABEL (GET_WIDGET ("rotation_angle_label")),
							       0.0, -90.0, 90.0, 0.1, 1.0, 1);

	self->priv->high_quality = _gtk_builder_get_widget (self->priv->builder, "high_quality");

	self->priv->background_colorbutton = _gtk_builder_get_widget (self->priv->builder, "background_colorbutton");
	self->priv->background_transparent = _gtk_builder_get_widget (self->priv->builder, "background_transparent_checkbutton");

	self->priv->has_alpha = gdk_pixbuf_get_n_channels (self->priv->src_pixbuf) == 4;

	if (self->priv->has_alpha) {
		gtk_color_button_set_use_alpha (GTK_COLOR_BUTTON (self->priv->background_colorbutton), TRUE);
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (self->priv->background_transparent), TRUE);
	}
	else {
		gtk_widget_set_sensitive (self->priv->background_transparent, FALSE);
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (self->priv->background_transparent), FALSE);
	}

	self->priv->show_grid = _gtk_builder_get_widget (self->priv->builder, "show_grid");
	self->priv->enable_guided_crop = _gtk_builder_get_widget (self->priv->builder, "enable_guided_crop");
	self->priv->keep_aspect_ratio = _gtk_builder_get_widget (self->priv->builder, "keep_aspect_ratio");

	self->priv->crop_p1_adj = gimp_scale_entry_new (GET_WIDGET ("crop_p1_hbox"),
							GTK_LABEL (GET_WIDGET ("crop_p1_label")),
							1.0, 0.0, 1.0, 0.001, 0.01, 3);

	self->priv->crop_p2_adj = gimp_scale_entry_new (GET_WIDGET ("crop_p2_hbox"),
							GTK_LABEL (GET_WIDGET ("crop_p2_label")),
							1.0, 0.0, 1.0, 0.001, 0.01, 3);

	self->priv->crop_grid = _gtk_combo_box_new_with_texts (_("None"),
							       _("Rule of Thirds"),
							       _("Golden Sections"),
							       _("Center Lines"),
							       _("Uniform"),
							       NULL);

	gtk_combo_box_set_active (GTK_COMBO_BOX (self->priv->crop_grid), GTH_GRID_UNIFORM);
	gtk_widget_show (self->priv->crop_grid);
	gtk_box_pack_start (GTK_BOX (GET_WIDGET ("crop_grid_hbox")), self->priv->crop_grid, FALSE, FALSE, 0);
	gtk_label_set_mnemonic_widget (GTK_LABEL (GET_WIDGET ("crop_grid_label")), self->priv->crop_grid);

	self->priv->selector_crop = (GthImageSelector *) gth_image_selector_new (GTH_IMAGE_VIEWER (viewer), GTH_SELECTOR_TYPE_REGION);
	self->priv->selector_align = (GthImageSelector *) gth_image_selector_new (GTH_IMAGE_VIEWER (viewer), GTH_SELECTOR_TYPE_POINT);

	gth_image_selector_set_mask_visible (self->priv->selector_crop, TRUE);
	gth_image_selector_set_mask_visible (self->priv->selector_align, FALSE);

	self->priv->selector_align_direction = 0;
	self->priv->selector_align_point = 0;

	self->priv->crop_enabled = TRUE;
	self->priv->crop_region.x = 0;
	self->priv->crop_region.y = 0;
	self->priv->crop_region.width = gdk_pixbuf_get_width (self->priv->src_pixbuf);
	self->priv->crop_region.height = gdk_pixbuf_get_height (self->priv->src_pixbuf);

	g_signal_connect (GET_WIDGET ("apply_button"),
			  "clicked",
			  G_CALLBACK (apply_button_clicked_cb),
			  self);
	g_signal_connect (GET_WIDGET ("cancel_button"),
			  "clicked",
			  G_CALLBACK (cancel_button_clicked_cb),
			  self);
	g_signal_connect (GET_WIDGET ("reset_button"),
			  "clicked",
			  G_CALLBACK (reset_button_clicked_cb),
			  self);
	g_signal_connect (GET_WIDGET ("align_h_button"),
			  "clicked",
			  G_CALLBACK (align_h_button_clicked_cb),
			  self);
	g_signal_connect (GET_WIDGET ("align_v_button"),
			  "clicked",
			  G_CALLBACK (align_v_button_clicked_cb),
			  self);
	g_signal_connect (G_OBJECT (self->priv->rotation_angle_adj),
			  "value-changed",
			  G_CALLBACK (value_changed_cb),
			  self);
	g_signal_connect (G_OBJECT (self->priv->background_colorbutton),
			  "color-set",
			  G_CALLBACK (background_color_changed_cb),
			  self);
	g_signal_connect (G_OBJECT (self->priv->background_transparent),
			  "toggled",
			  G_CALLBACK (value_changed_cb),
			  self);
	g_signal_connect (G_OBJECT (self->priv->crop_p1_adj),
			  "value-changed",
			  G_CALLBACK (crop_parameters_changed_cb),
			  self);
	g_signal_connect (G_OBJECT (self->priv->crop_p2_adj),
			  "value-changed",
			  G_CALLBACK (crop_parameters_changed_cb),
			  self);
	g_signal_connect (G_OBJECT (self->priv->high_quality),
			  "toggled",
			  G_CALLBACK (value_changed_cb),
			  self);
	g_signal_connect (G_OBJECT (self->priv->enable_guided_crop),
			  "toggled",
			  G_CALLBACK (crop_settings_changed_cb),
			  self);
	g_signal_connect (G_OBJECT (self->priv->keep_aspect_ratio),
			  "toggled",
			  G_CALLBACK (crop_settings_changed_cb),
			  self);
	g_signal_connect (G_OBJECT (self->priv->crop_grid),
			  "changed",
			  G_CALLBACK (crop_grid_changed_cb),
			  self);
	g_signal_connect (self->priv->selector_align,
			  "selected",
			  G_CALLBACK (selector_selected_cb),
			  self);

	update_crop_parameters (self);
	update_crop_region (self);
	update_crop_grid (self);

	return self->priv->options;
}


static void
gth_file_tool_rotate_destroy_options (GthFileTool *base)
{
	GthFileToolRotate *self;
	GtkWidget         *window;
	GtkWidget         *viewer_page;
	GtkWidget         *viewer;

	self = (GthFileToolRotate *) base;

	if (self->priv->apply_event != 0) {
		g_source_remove (self->priv->apply_event);
		self->priv->apply_event = 0;
	}

	window = gth_file_tool_get_window (GTH_FILE_TOOL (self));
	viewer_page = gth_browser_get_viewer_page (GTH_BROWSER (window));
	viewer = gth_image_viewer_page_get_image_viewer (GTH_IMAGE_VIEWER_PAGE (viewer_page));
	gth_image_viewer_set_tool (GTH_IMAGE_VIEWER (viewer), NULL);

	_g_clear_object (&self->priv->src_pixbuf);
	_g_clear_object (&self->priv->rotate_pixbuf);
	_g_clear_object (&self->priv->builder);
	_g_clear_object (&self->priv->selector_crop);
	_g_clear_object (&self->priv->selector_align);
	self->priv->selector_align_direction = 0;
	self->priv->selector_align_point = 0;
}


static void
gth_file_tool_rotate_activate (GthFileTool *base)
{
	gth_file_tool_show_options (base);
}


static void
gth_file_tool_rotate_instance_init (GthFileToolRotate *self)
{
	self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, GTH_TYPE_FILE_TOOL_ROTATE, GthFileToolRotatePrivate);

	gth_file_tool_construct (GTH_FILE_TOOL (self), "tool-rotate", _("Rotate..."), _("Rotate"), TRUE);
	gtk_widget_set_tooltip_text (GTK_WIDGET (self), _("Freely rotate the image"));
}


static void
gth_file_tool_rotate_finalize (GObject *object)
{
	GthFileToolRotate *self;

	g_return_if_fail (object != NULL);
	g_return_if_fail (GTH_IS_FILE_TOOL_ROTATE (object));

	self = (GthFileToolRotate *) object;

	_g_object_unref (self->priv->src_pixbuf);
	_g_object_unref (self->priv->rotate_pixbuf);
	_g_object_unref (self->priv->builder);

	/* Chain up */
	G_OBJECT_CLASS (parent_class)->finalize (object);
}


static void
gth_file_tool_rotate_class_init (GthFileToolRotateClass *class)
{
	GObjectClass     *gobject_class;
	GthFileToolClass *file_tool_class;

	parent_class = g_type_class_peek_parent (class);
	g_type_class_add_private (class, sizeof (GthFileToolRotatePrivate));

	gobject_class = (GObjectClass*) class;
	gobject_class->finalize = gth_file_tool_rotate_finalize;

	file_tool_class = (GthFileToolClass *) class;
	file_tool_class->update_sensitivity = gth_file_tool_rotate_update_sensitivity;
	file_tool_class->activate = gth_file_tool_rotate_activate;
	file_tool_class->get_options = gth_file_tool_rotate_get_options;
	file_tool_class->destroy_options = gth_file_tool_rotate_destroy_options;
}


GType
gth_file_tool_rotate_get_type (void) {
	static GType type_id = 0;
	if (type_id == 0) {
		static const GTypeInfo g_define_type_info = {
			sizeof (GthFileToolRotateClass),
			(GBaseInitFunc) NULL,
			(GBaseFinalizeFunc) NULL,
			(GClassInitFunc) gth_file_tool_rotate_class_init,
			(GClassFinalizeFunc) NULL,
			NULL,
			sizeof (GthFileToolRotate),
			0,
			(GInstanceInitFunc) gth_file_tool_rotate_instance_init,
			NULL
		};
		type_id = g_type_register_static (GTH_TYPE_FILE_TOOL, "GthFileToolRotate", &g_define_type_info, 0);
	}
	return type_id;
}
