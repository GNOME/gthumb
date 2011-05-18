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
#include "cairo-rotate.h"
#include "enum-types.h"
#include "gth-file-tool-rotate.h"
#include "gth-image-line-tool.h"
#include "gth-image-rotator.h"
#include "preferences.h"


#define GET_WIDGET(x) (_gtk_builder_get_widget (self->priv->builder, (x)))


static gpointer parent_class = NULL;


struct _GthFileToolRotatePrivate {
	cairo_surface_t    *image;
	gboolean            has_alpha;
	GtkBuilder         *builder;
	GtkWidget          *crop_grid;
	GtkAdjustment      *rotation_angle_adj;
	GtkAdjustment      *crop_p1_adj;
	GtkAdjustment      *crop_p2_adj;
	gboolean            crop_enabled;
	double              crop_p1_plus_p2;
	GdkRectangle        crop_region;
	GthImageViewerTool *alignment;
	GthImageViewerTool *rotator;
};


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
update_crop_parameters (GthFileToolRotate *self)
{
	GthTransformResize resize;
	double             rotation_angle;
	double             crop_p1;
	double             crop_p_min;

	resize = gtk_combo_box_get_active (GTK_COMBO_BOX (GET_WIDGET ("resize_combobox")));
	self->priv->crop_enabled = (resize == GTH_TRANSFORM_RESIZE_CROP);

	if (self->priv->crop_enabled) {
		gtk_widget_set_sensitive (GET_WIDGET ("crop_options_table"), TRUE);

		rotation_angle = gtk_adjustment_get_value (self->priv->rotation_angle_adj);

		if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("keep_aspect_ratio")))) {
			gtk_widget_set_sensitive (GET_WIDGET ("crop_p2_label"), FALSE);
			gtk_widget_set_sensitive (GET_WIDGET ("crop_p2_hbox"), FALSE);

			_cairo_image_surface_rotate_get_cropping_parameters (self->priv->image,
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
	}
	else
		gtk_widget_set_sensitive (GET_WIDGET ("crop_options_table"), FALSE);

	gth_image_rotator_set_resize (GTH_IMAGE_ROTATOR (self->priv->rotator), resize);
}


static void
update_crop_region (GthFileToolRotate *self)
{
	if (self->priv->crop_enabled) {
		double rotation_angle;
		double crop_p1;
		double crop_p2;

		rotation_angle = gtk_adjustment_get_value (self->priv->rotation_angle_adj);
		crop_p1 = gtk_adjustment_get_value (self->priv->crop_p1_adj);
		crop_p2 = gtk_adjustment_get_value (self->priv->crop_p2_adj);
		_cairo_image_surface_rotate_get_cropping_region (self->priv->image,
								 rotation_angle,
								 crop_p1,
								 crop_p2,
								 &self->priv->crop_region);
		gth_image_rotator_set_crop_region (GTH_IMAGE_ROTATOR (self->priv->rotator), &self->priv->crop_region);
	}
	else
		gth_image_rotator_set_crop_region (GTH_IMAGE_ROTATOR (self->priv->rotator), NULL);
}


static void
update_crop_grid (GthFileToolRotate *self)
{
	gth_image_rotator_set_grid_type (GTH_IMAGE_ROTATOR (self->priv->rotator), gtk_combo_box_get_active (GTK_COMBO_BOX (self->priv->crop_grid)));
}


static void
alignment_changed_cb (GthImageLineTool  *line_tool,
		      GthFileToolRotate *self)
{
	GtkWidget *window;
	GtkWidget *viewer_page;
	GtkWidget *viewer;
	GdkPoint   p1;
	GdkPoint   p2;
	double     angle;

	window = gth_file_tool_get_window (GTH_FILE_TOOL (self));
	viewer_page = gth_browser_get_viewer_page (GTH_BROWSER (window));
	viewer = gth_image_viewer_page_get_image_viewer (GTH_IMAGE_VIEWER_PAGE (viewer_page));
	gtk_notebook_set_current_page (GTK_NOTEBOOK (GET_WIDGET ("options_notebook")), 0);
	gth_image_viewer_set_tool (GTH_IMAGE_VIEWER (viewer), self->priv->rotator);

	gth_image_line_tool_get_points (line_tool, &p1, &p2);
	angle = _cairo_image_surface_rotate_get_align_angle (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("alignment_parallel_radiobutton"))), &p1, &p2);
	gtk_adjustment_set_value (self->priv->rotation_angle_adj, angle);
}


static void
apply_changes (GthFileToolRotate *self)
{
	gth_image_rotator_set_angle (GTH_IMAGE_ROTATOR (self->priv->rotator), gtk_adjustment_get_value (self->priv->rotation_angle_adj));
	update_crop_parameters (self);
	update_crop_region (self);
}


static void
alignment_cancel_button_clicked_cb (GtkButton         *button,
				    GthFileToolRotate *self)
{
	GtkWidget *window;
	GtkWidget *viewer_page;
	GtkWidget *viewer;

	window = gth_file_tool_get_window (GTH_FILE_TOOL (self));
	viewer_page = gth_browser_get_viewer_page (GTH_BROWSER (window));
	viewer = gth_image_viewer_page_get_image_viewer (GTH_IMAGE_VIEWER_PAGE (viewer_page));
	gtk_notebook_set_current_page (GTK_NOTEBOOK (GET_WIDGET ("options_notebook")), 0);
	gth_image_viewer_set_tool (GTH_IMAGE_VIEWER (viewer), self->priv->rotator);
}


static void
align_button_clicked_cb (GtkButton         *button,
			 GthFileToolRotate *self)
{
	GtkWidget *window;
	GtkWidget *viewer_page;
	GtkWidget *viewer;

	window = gth_file_tool_get_window (GTH_FILE_TOOL (self));
	viewer_page = gth_browser_get_viewer_page (GTH_BROWSER (window));
	viewer = gth_image_viewer_page_get_image_viewer (GTH_IMAGE_VIEWER_PAGE (viewer_page));

	gtk_notebook_set_current_page (GTK_NOTEBOOK (GET_WIDGET ("options_notebook")), 1);
	gth_image_viewer_set_tool (GTH_IMAGE_VIEWER (viewer), self->priv->alignment);
}


static void
reset_button_clicked_cb (GtkButton         *button,
			 GthFileToolRotate *self)
{
	gth_image_rotator_set_center (GTH_IMAGE_ROTATOR (self->priv->rotator),
				      cairo_image_surface_get_width (self->priv->image) / 2,
				      cairo_image_surface_get_height (self->priv->image) / 2);
	gtk_adjustment_set_value (self->priv->rotation_angle_adj, 0.0);
}


static void
apply_button_clicked_cb (GtkButton         *button,
			 GthFileToolRotate *self)
{
	cairo_surface_t *image;
	GtkWidget       *window;
	GtkWidget       *viewer_page;

	image = gth_image_rotator_get_result (GTH_IMAGE_ROTATOR (self->priv->rotator), TRUE);

	window = gth_file_tool_get_window (GTH_FILE_TOOL (self));
	viewer_page = gth_browser_get_viewer_page (GTH_BROWSER (window));
	gth_image_viewer_page_set_image (GTH_IMAGE_VIEWER_PAGE (viewer_page), image, TRUE);
	gth_file_tool_hide_options (GTH_FILE_TOOL (self));

	cairo_surface_destroy (image);
}


static void
cancel_button_clicked_cb (GtkButton         *button,
			  GthFileToolRotate *self)
{
	GtkWidget *window;
	GtkWidget *viewer_page;

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
	if ((adj == self->priv->crop_p1_adj) && gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("keep_aspect_ratio"))))
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
rotation_angle_value_changed_cb (GtkAdjustment     *adj,
				 GthFileToolRotate *self)
{
	apply_changes (self);
}


static void
background_colorbutton_color_set_cb (GtkColorButton    *color_button,
		             	     GthFileToolRotate *self)
{
	GdkColor      color;
	cairo_color_t background_color;

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("background_transparent_checkbutton")), FALSE);

	gtk_color_button_get_color (color_button, &color);
	background_color.r = (double) color.red / 65535;
	background_color.g = (double) color.green / 65535;
	background_color.b = (double) color.blue / 65535;
	background_color.a = 1.0;
	gth_image_rotator_set_background (GTH_IMAGE_ROTATOR (self->priv->rotator), &background_color);

	apply_changes (self);
}


static void
background_transparent_toggled_cb (GtkToggleButton   *toggle_button,
		             	   GthFileToolRotate *self)
{
	if (gtk_toggle_button_get_active (toggle_button)) {
		cairo_color_t background_color;

		background_color.r = 0.0;
		background_color.g = 0.0;
		background_color.b = 0.0;
		background_color.a = 0.0;
		gth_image_rotator_set_background (GTH_IMAGE_ROTATOR (self->priv->rotator), &background_color);
	}
	else
		background_colorbutton_color_set_cb (GTK_COLOR_BUTTON (GET_WIDGET ("background_colorbutton")), self);
}


static void
resize_combobox_changed_cb (GtkComboBox       *combo_box,
			    GthFileToolRotate *self)
{
	update_crop_parameters (self);
	update_crop_region (self);
}


static void
rotator_angle_changed_cb (GthImageRotator   *rotator,
			  double             angle,
			  GthFileToolRotate *self)
{
	gtk_adjustment_set_value (self->priv->rotation_angle_adj, angle);
}


static void
rotator_center_changed_cb (GthImageRotator   *rotator,
		  	   int                x,
		  	   int                y,
		  	   GthFileToolRotate *self)
{
	gth_image_rotator_set_center (rotator, x, y);
	update_crop_parameters (self);
	update_crop_region (self);
}


static GtkWidget *
gth_file_tool_rotate_get_options (GthFileTool *base)
{
	GthFileToolRotate *self;
	GtkWidget         *window;
	GtkWidget         *viewer_page;
	GtkWidget         *viewer;
	char              *color_spec;
	GdkColor           color;
	cairo_color_t      background_color;

	self = (GthFileToolRotate *) base;

	window = gth_file_tool_get_window (base);
	viewer_page = gth_browser_get_viewer_page (GTH_BROWSER (window));
	if (! GTH_IS_IMAGE_VIEWER_PAGE (viewer_page))
		return NULL;

	cairo_surface_destroy (self->priv->image);

	viewer = gth_image_viewer_page_get_image_viewer (GTH_IMAGE_VIEWER_PAGE (viewer_page));
	self->priv->image = gth_image_viewer_get_current_image (GTH_IMAGE_VIEWER (viewer));
	if (self->priv->image == NULL)
		return NULL;

	cairo_surface_reference (self->priv->image);

	self->priv->builder = _gtk_builder_new_from_file ("rotate-options.ui", "file_tools");

	self->priv->rotation_angle_adj = gimp_scale_entry_new (GET_WIDGET ("rotation_angle_hbox"),
							       GTK_LABEL (GET_WIDGET ("rotation_angle_label")),
							       0.0, -90.0, 90.0, 0.1, 1.0, 1);

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
	gtk_combo_box_set_active (GTK_COMBO_BOX (self->priv->crop_grid), eel_gconf_get_enum (PREF_ROTATE_GRID_TYPE, GTH_TYPE_GRID_TYPE, GTH_GRID_THIRDS));
	gtk_widget_show (self->priv->crop_grid);
	gtk_box_pack_start (GTK_BOX (GET_WIDGET ("crop_grid_hbox")), self->priv->crop_grid, FALSE, FALSE, 0);
	gtk_label_set_mnemonic_widget (GTK_LABEL (GET_WIDGET ("crop_grid_label")), self->priv->crop_grid);

	gtk_combo_box_set_active (GTK_COMBO_BOX (GET_WIDGET ("resize_combobox")), eel_gconf_get_enum (PREF_ROTATE_RESIZE, GTH_TYPE_TRANSFORM_RESIZE, GTH_TRANSFORM_RESIZE_CROP));

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("keep_aspect_ratio")), eel_gconf_get_boolean (PREF_ROTATE_KEEP_ASPECT_RATIO, TRUE));

	self->priv->alignment = gth_image_line_tool_new ();

	self->priv->rotator = gth_image_rotator_new ();
	gth_image_rotator_set_center (GTH_IMAGE_ROTATOR (self->priv->rotator),
				      cairo_image_surface_get_width (self->priv->image) / 2,
				      cairo_image_surface_get_height (self->priv->image) / 2);

	self->priv->has_alpha = _cairo_image_surface_get_has_alpha (self->priv->image);
	if (self->priv->has_alpha) {
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("background_transparent_checkbutton")), TRUE);
	}
	else {
		gtk_widget_set_sensitive (GET_WIDGET ("background_transparent_checkbutton"), FALSE);
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("background_transparent_checkbutton")), FALSE);
	}

	color_spec = eel_gconf_get_string (PREF_ROTATE_BACKGROUND_COLOR, "#000");
	if (! self->priv->has_alpha && gdk_color_parse (color_spec, &color)) {
		_gdk_color_to_cairo_color (&color, &background_color);
	}
	else {
		background_color.r = 0.0;
		background_color.g = 0.0;
		background_color.b = 0.0;
		background_color.a = self->priv->has_alpha ? 0.0 : 1.0;
	}
	gth_image_rotator_set_background (GTH_IMAGE_ROTATOR (self->priv->rotator), &background_color);

	gth_image_viewer_set_tool (GTH_IMAGE_VIEWER (viewer), self->priv->rotator);
	gth_viewer_page_update_sensitivity (GTH_VIEWER_PAGE (viewer_page));

	self->priv->crop_enabled = TRUE;
	self->priv->crop_region.x = 0;
	self->priv->crop_region.y = 0;
	self->priv->crop_region.width = cairo_image_surface_get_width (self->priv->image);
	self->priv->crop_region.height = cairo_image_surface_get_height (self->priv->image);

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
	g_signal_connect (GET_WIDGET ("align_button"),
			  "clicked",
			  G_CALLBACK (align_button_clicked_cb),
			  self);
	g_signal_connect (G_OBJECT (self->priv->rotation_angle_adj),
			  "value-changed",
			  G_CALLBACK (rotation_angle_value_changed_cb),
			  self);
	g_signal_connect (GET_WIDGET ("background_colorbutton"),
			  "color-set",
			  G_CALLBACK (background_colorbutton_color_set_cb),
			  self);
	g_signal_connect (GET_WIDGET ("background_transparent_checkbutton"),
			  "toggled",
			  G_CALLBACK (background_transparent_toggled_cb),
			  self);
	g_signal_connect (G_OBJECT (self->priv->crop_p1_adj),
			  "value-changed",
			  G_CALLBACK (crop_parameters_changed_cb),
			  self);
	g_signal_connect (G_OBJECT (self->priv->crop_p2_adj),
			  "value-changed",
			  G_CALLBACK (crop_parameters_changed_cb),
			  self);
	g_signal_connect (G_OBJECT (GET_WIDGET ("keep_aspect_ratio")),
			  "toggled",
			  G_CALLBACK (crop_settings_changed_cb),
			  self);
	g_signal_connect (G_OBJECT (self->priv->crop_grid),
			  "changed",
			  G_CALLBACK (crop_grid_changed_cb),
			  self);
	g_signal_connect (self->priv->alignment,
			  "changed",
			  G_CALLBACK (alignment_changed_cb),
			  self);
	g_signal_connect (GET_WIDGET ("alignment_cancel_button"),
			  "clicked",
			  G_CALLBACK (alignment_cancel_button_clicked_cb),
			  self);
	g_signal_connect (GET_WIDGET ("resize_combobox"),
			  "changed",
			  G_CALLBACK (resize_combobox_changed_cb),
			  self);
	g_signal_connect (self->priv->rotator,
			  "angle-changed",
			  G_CALLBACK (rotator_angle_changed_cb),
			  self);
	g_signal_connect (self->priv->rotator,
			  "center-changed",
			  G_CALLBACK (rotator_center_changed_cb),
			  self);

	update_crop_parameters (self);
	update_crop_region (self);
	update_crop_grid (self);

	return GET_WIDGET ("options_notebook");
}


static void
gth_file_tool_rotate_destroy_options (GthFileTool *base)
{
	GthFileToolRotate *self;
	GtkWidget         *window;
	GtkWidget         *viewer_page;
	GtkWidget         *viewer;

	self = (GthFileToolRotate *) base;

	if (self->priv->builder != NULL) {
		cairo_color_t  background_color;
		GdkColor       color;
		char          *color_spec;

		/* save the dialog options */

		eel_gconf_set_enum (PREF_ROTATE_RESIZE, GTH_TYPE_TRANSFORM_RESIZE, gth_image_rotator_get_resize (GTH_IMAGE_ROTATOR (self->priv->rotator)));
		eel_gconf_set_boolean (PREF_ROTATE_KEEP_ASPECT_RATIO, gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("keep_aspect_ratio"))));
		eel_gconf_set_enum (PREF_ROTATE_GRID_TYPE, GTH_TYPE_GRID_TYPE, gth_image_rotator_get_grid_type (GTH_IMAGE_ROTATOR (self->priv->rotator)));

		gth_image_rotator_get_background (GTH_IMAGE_ROTATOR (self->priv->rotator), &background_color);
		color.red = background_color.r * 255.0;
		color.green = background_color.g * 255.0;
		color.blue = background_color.b * 255.0;
		color_spec = gdk_color_to_string (&color);
		eel_gconf_set_string (PREF_ROTATE_BACKGROUND_COLOR, color_spec);
		g_free (color_spec);
	}

	window = gth_file_tool_get_window (GTH_FILE_TOOL (self));
	viewer_page = gth_browser_get_viewer_page (GTH_BROWSER (window));
	viewer = gth_image_viewer_page_get_image_viewer (GTH_IMAGE_VIEWER_PAGE (viewer_page));
	gth_image_viewer_set_tool (GTH_IMAGE_VIEWER (viewer), NULL);
	gth_image_viewer_set_zoom_enabled (GTH_IMAGE_VIEWER (viewer), TRUE);
	gth_viewer_page_update_sensitivity (GTH_VIEWER_PAGE (viewer_page));

	cairo_surface_destroy (self->priv->image);
	self->priv->image = NULL;
	_g_clear_object (&self->priv->builder);
	_g_clear_object (&self->priv->rotator);
	_g_clear_object (&self->priv->alignment);
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

	cairo_surface_destroy (self->priv->image);
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
