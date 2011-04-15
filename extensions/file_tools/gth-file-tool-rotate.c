/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2011 Free Software Foundation, Inc.
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
#include "gth-file-tool-rotate.h"
#include "gth-image-rotator.h"
#include "preferences.h"


#define GET_WIDGET(x) (_gtk_builder_get_widget (self->priv->builder, (x)))
#define HIGH_QUALITY_INTERPOLATION GDK_INTERP_HYPER
#define APPLY_DELAY 50
#define DEFAULT_GRID 15


static gpointer parent_class = NULL;


struct _GthFileToolRotatePrivate {
	GtkBuilder      *builder;
	GtkAdjustment   *angle_adj;
	GtkAdjustment   *small_angle_adj;
	GthImageRotator *rotator;
	int              pixbuf_width;
	int              pixbuf_height;
	int              new_width;
	int              new_height;
	GthUnit          unit;
	guint            apply_event;
	double           angle_step;
	gboolean         use_grid;
	GtkAdjustment   *grid_adj;
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
cancel_button_clicked_cb (GtkButton *button,
			  gpointer   user_data)
{
	GthFileToolRotate *self = user_data;

	if (self->priv->apply_event != 0) {
		g_source_remove (self->priv->apply_event);
		self->priv->apply_event = 0;
	}

	gth_file_tool_hide_options (GTH_FILE_TOOL (self));
}


static void
ok_button_clicked_cb (GtkButton *button,
		      gpointer   user_data)
{
	GthFileToolRotate *self = user_data;
	cairo_surface_t   *new_image;

	new_image = gth_image_rotator_get_result (self->priv->rotator);
	if (new_image != NULL) {
		GtkWidget *window;
		GtkWidget *viewer_page;

		window = gth_file_tool_get_window (GTH_FILE_TOOL (self));
		viewer_page = gth_browser_get_viewer_page (GTH_BROWSER (window));
		gth_image_viewer_page_set_image (GTH_IMAGE_VIEWER_PAGE (viewer_page), new_image, TRUE);
		gth_file_tool_hide_options (GTH_FILE_TOOL (self));

		cairo_surface_destroy (new_image);
	}
}


static void
rotator_center_changed_cb (GObject  *gobject,
			   gpointer  user_data)
{
	GthFileToolRotate *self = user_data;
	int                x, y;
	double             dx, dy;

	g_signal_handlers_block_by_data (GET_WIDGET ("center_x_spinbutton"), self);
	g_signal_handlers_block_by_data (GET_WIDGET ("center_y_spinbutton"), self);

	gth_image_rotator_get_center (self->priv->rotator, &x, &y);
	if (self->priv->unit == GTH_UNIT_PERCENTAGE) {
		dx = ((double) x / self->priv->pixbuf_width) * 100.0;
		dy = ((double) y / self->priv->pixbuf_height) * 100.0;
	}
	else {
		dx = x;
		dy = y;
	}
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (GET_WIDGET ("center_x_spinbutton")), dx);
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (GET_WIDGET ("center_y_spinbutton")), dy);

	g_signal_handlers_unblock_by_data (GET_WIDGET ("center_x_spinbutton"), self);
	g_signal_handlers_unblock_by_data (GET_WIDGET ("center_y_spinbutton"), self);
}


static void
center_position_changed_cb (GtkSpinButton *spinbutton,
			    gpointer       user_data)
{
	GthFileToolRotate *self = user_data;
	double             x, y;

	g_signal_handlers_block_by_data (self->priv->rotator, user_data);

	x = gtk_spin_button_get_value (GTK_SPIN_BUTTON (GET_WIDGET ("center_x_spinbutton")));
	y = gtk_spin_button_get_value (GTK_SPIN_BUTTON (GET_WIDGET ("center_y_spinbutton")));
	if (self->priv->unit == GTH_UNIT_PERCENTAGE) {
		x = round ((double) self->priv->pixbuf_width * (x / 100.0));
		y = round ((double) self->priv->pixbuf_height * (y / 100.0));
	}
	gth_image_rotator_set_center (self->priv->rotator, x, y);

	g_signal_handlers_unblock_by_data (self->priv->rotator, user_data);
}


static void
unit_combobox_changed_cb (GtkComboBox *combobox,
			  gpointer     user_data)
{
	GthFileToolRotate *self = user_data;

	g_signal_handlers_block_by_data (GET_WIDGET ("center_x_spinbutton"), self);
	g_signal_handlers_block_by_data (GET_WIDGET ("center_y_spinbutton"), self);

	self->priv->unit = gtk_combo_box_get_active (combobox);
	if (self->priv->unit == GTH_UNIT_PERCENTAGE) {
		double p;

		gtk_spin_button_set_digits (GTK_SPIN_BUTTON (GET_WIDGET ("center_x_spinbutton")), 2);
		gtk_spin_button_set_digits (GTK_SPIN_BUTTON (GET_WIDGET ("center_y_spinbutton")), 2);

		p = gtk_spin_button_get_value (GTK_SPIN_BUTTON (GET_WIDGET ("center_x_spinbutton"))) / self->priv->pixbuf_width * 100.0;
		gtk_spin_button_set_value (GTK_SPIN_BUTTON (GET_WIDGET ("center_x_spinbutton")), p);

		p = gtk_spin_button_get_value (GTK_SPIN_BUTTON (GET_WIDGET ("center_y_spinbutton"))) / self->priv->pixbuf_height * 100.0;
		gtk_spin_button_set_value (GTK_SPIN_BUTTON (GET_WIDGET ("center_y_spinbutton")), p);
	}
	else if (self->priv->unit == GTH_UNIT_PIXELS) {
		double p;

		gtk_spin_button_set_digits (GTK_SPIN_BUTTON (GET_WIDGET ("center_x_spinbutton")), 0);
		gtk_spin_button_set_digits (GTK_SPIN_BUTTON (GET_WIDGET ("center_y_spinbutton")), 0);

		p = round (self->priv->pixbuf_width * (gtk_spin_button_get_value (GTK_SPIN_BUTTON (GET_WIDGET ("center_x_spinbutton"))) / 100.0));
		gtk_spin_button_set_value (GTK_SPIN_BUTTON (GET_WIDGET ("center_x_spinbutton")), p);

		p = round (self->priv->pixbuf_height * (gtk_spin_button_get_value (GTK_SPIN_BUTTON (GET_WIDGET ("center_y_spinbutton"))) / 100.0));
		gtk_spin_button_set_value (GTK_SPIN_BUTTON (GET_WIDGET ("center_y_spinbutton")), p);
	}

	g_signal_handlers_unblock_by_data (GET_WIDGET ("center_x_spinbutton"), self);
	g_signal_handlers_unblock_by_data (GET_WIDGET ("center_y_spinbutton"), self);

	center_position_changed_cb (NULL, self);
}


static void
_gth_file_tool_rotate (GthFileToolRotate *self,
		       double             angle)
{
	char *s;

	gth_image_rotator_set_angle (self->priv->rotator, angle);
	s = g_strdup_printf ("%2.2f°", angle);
	gtk_label_set_text (GTK_LABEL (GET_WIDGET ("angle_label")), s);

	g_free (s);
}


static gboolean
apply_small_angle_cb (gpointer user_data)
{
	GthFileToolRotate *self = user_data;
	double             angle;

	self->priv->apply_event = 0;

	angle = gtk_adjustment_get_value (self->priv->angle_adj) + gtk_adjustment_get_value (self->priv->small_angle_adj);
	_gth_file_tool_rotate (self, angle);

	return FALSE;
}


static void
small_angle_value_changed_cb (GtkAdjustment *adj,
			      gpointer       user_data)
{
	GthFileToolRotate *self = user_data;

	if (self->priv->apply_event == 0)
		self->priv->apply_event = g_timeout_add (APPLY_DELAY, apply_small_angle_cb, self);
}


static void
angle_value_changed_cb (GtkAdjustment *adj,
			gpointer       user_data)
{
	GthFileToolRotate *self = user_data;
	double             angle;

	if (self->priv->apply_event != 0) {
		g_source_remove (self->priv->apply_event);
		self->priv->apply_event = 0;
	}

	if (self->priv->angle_step != 0.0) {
		double angle;
		double rounded_angle;

		angle = gtk_adjustment_get_value (self->priv->angle_adj) / self->priv->angle_step;
		rounded_angle = round (angle);
		if (angle != rounded_angle) {
			angle = rounded_angle * self->priv->angle_step;
			gtk_adjustment_set_value (self->priv->angle_adj, angle);
			return;
		}
	}

	g_signal_handlers_block_by_func (self->priv->small_angle_adj, small_angle_value_changed_cb, self);
	gtk_adjustment_set_value (self->priv->small_angle_adj, 0.0);
	g_signal_handlers_unblock_by_func (self->priv->small_angle_adj, small_angle_value_changed_cb, self);

	angle = gtk_adjustment_get_value (self->priv->angle_adj) + gtk_adjustment_get_value (self->priv->small_angle_adj);
	_gth_file_tool_rotate (self, angle);
}


static void
grid_value_changed_cb (GtkAdjustment *adj,
		       gpointer       user_data)
{
	GthFileToolRotate *self = user_data;

	if (self->priv->use_grid)
		gth_image_rotator_set_grid (self->priv->rotator, TRUE, (int) gtk_adjustment_get_value (self->priv->grid_adj));
}


static void
grid_checkbutton_toggled_cb (GtkToggleButton *button,
		     	     gpointer         user_data)
{
	GthFileToolRotate *self = user_data;

	self->priv->use_grid = gtk_toggle_button_get_active (button);
	if (self->priv->use_grid)
		gth_image_rotator_set_grid (self->priv->rotator, TRUE, (int) gtk_adjustment_get_value (self->priv->grid_adj));
	else
		gth_image_rotator_set_grid (self->priv->rotator, FALSE, 0);
}


static void
size_combobox_changed_cb (GtkComboBox *combo_box,
			  gpointer     user_data)
{
	GthFileToolRotate *self = user_data;

	gth_image_rotator_set_resize (self->priv->rotator, (GthTransformResize) gtk_combo_box_get_active (combo_box));
}


static void
background_colorbutton_notify_color_cb (GObject    *gobject,
					GParamSpec *pspec,
					gpointer    user_data)
{
	GthFileToolRotate *self = user_data;
	GdkColor           gdk_color;
	cairo_color_t      color;

	gtk_color_button_get_color (GTK_COLOR_BUTTON (gobject), &gdk_color);
	_gdk_color_to_cairo_color (&gdk_color, &color);
	color.a = (double) gtk_color_button_get_alpha (GTK_COLOR_BUTTON (gobject)) / 255.0;
	gth_image_rotator_set_background (self->priv->rotator, &color);
}


static void
center_button_clicked_cb (GtkButton *button,
			  gpointer    user_data)
{
	GthFileToolRotate *self = user_data;

	gth_image_rotator_set_center (self->priv->rotator,
				      self->priv->pixbuf_width * 0.5,
				      self->priv->pixbuf_height * 0.5);
}


static GtkWidget *
gth_file_tool_rotate_get_options (GthFileTool *base)
{
	GthFileToolRotate *self;
	GtkWidget         *window;
	GtkWidget         *viewer_page;
	GtkWidget         *viewer;
	cairo_surface_t   *image;
	GtkWidget         *options;

	self = (GthFileToolRotate *) base;

	window = gth_file_tool_get_window (base);
	viewer_page = gth_browser_get_viewer_page (GTH_BROWSER (window));
	if (! GTH_IS_IMAGE_VIEWER_PAGE (viewer_page))
		return NULL;

	viewer = gth_image_viewer_page_get_image_viewer (GTH_IMAGE_VIEWER_PAGE (viewer_page));
	image = gth_image_viewer_get_current_image (GTH_IMAGE_VIEWER (viewer));
	if (image == NULL)
		return NULL;

	self->priv->pixbuf_width = cairo_image_surface_get_width (image);
	self->priv->pixbuf_height = cairo_image_surface_get_height (image);
	self->priv->unit = eel_gconf_get_enum (PREF_ROTATE_UNIT, GTH_TYPE_UNIT, GTH_UNIT_PERCENTAGE);
	self->priv->builder = _gtk_builder_new_from_file ("rotate-options.ui", "file_tools");

	options = _gtk_builder_get_widget (self->priv->builder, "options");
	gtk_widget_show (options);

	if (self->priv->unit == GTH_UNIT_PIXELS) {
		gtk_spin_button_set_digits (GTK_SPIN_BUTTON (GET_WIDGET ("center_x_spinbutton")), 0);
		gtk_spin_button_set_digits (GTK_SPIN_BUTTON (GET_WIDGET ("center_y_spinbutton")), 0);
		gtk_spin_button_set_value (GTK_SPIN_BUTTON (GET_WIDGET ("center_x_spinbutton")), self->priv->pixbuf_width / 2);
		gtk_spin_button_set_value (GTK_SPIN_BUTTON (GET_WIDGET ("center_y_spinbutton")), self->priv->pixbuf_height / 2);
	}
	else if (self->priv->unit == GTH_UNIT_PERCENTAGE) {
		gtk_spin_button_set_digits (GTK_SPIN_BUTTON (GET_WIDGET ("center_x_spinbutton")), 2);
		gtk_spin_button_set_digits (GTK_SPIN_BUTTON (GET_WIDGET ("center_y_spinbutton")), 2);
		gtk_spin_button_set_value (GTK_SPIN_BUTTON (GET_WIDGET ("center_x_spinbutton")), 50.0);
		gtk_spin_button_set_value (GTK_SPIN_BUTTON (GET_WIDGET ("center_y_spinbutton")), 50.0);
	}
	gtk_combo_box_set_active (GTK_COMBO_BOX (GET_WIDGET ("unit_combobox")), self->priv->unit);

	gtk_combo_box_set_active (GTK_COMBO_BOX (GET_WIDGET ("size_combobox")), 0);

	self->priv->angle_adj = gimp_scale_entry_new (GET_WIDGET ("angle_box"),
						      NULL,
						      0.0, -180.0, 180.0, 1.0, 10.0, 2);
	self->priv->small_angle_adj = gimp_scale_entry_new (GET_WIDGET ("small_angle_box"),
							    NULL,
							    0.0, -5.0, 5.0, 0.01, 0.1, 2);
	self->priv->grid_adj = gimp_scale_entry_new (GET_WIDGET ("grid_box"),
						     NULL,
						     eel_gconf_get_integer (PREF_ROTATE_GRID_SIZE, DEFAULT_GRID), 2.0, 50.0, 1.0, 10.0, 0);

	g_signal_connect (GET_WIDGET ("ok_button"),
			  "clicked",
			  G_CALLBACK (ok_button_clicked_cb),
			  self);
	g_signal_connect (GET_WIDGET ("cancel_button"),
			  "clicked",
			  G_CALLBACK (cancel_button_clicked_cb),
			  self);
	g_signal_connect (GET_WIDGET ("center_x_spinbutton"),
			  "value-changed",
			  G_CALLBACK (center_position_changed_cb),
			  self);
	g_signal_connect (GET_WIDGET ("center_y_spinbutton"),
			  "value-changed",
			  G_CALLBACK (center_position_changed_cb),
			  self);
	g_signal_connect (GET_WIDGET ("unit_combobox"),
			  "changed",
			  G_CALLBACK (unit_combobox_changed_cb),
			  self);
	g_signal_connect (G_OBJECT (self->priv->angle_adj),
			  "value-changed",
			  G_CALLBACK (angle_value_changed_cb),
			  self);
	g_signal_connect (G_OBJECT (self->priv->small_angle_adj),
			  "value-changed",
			  G_CALLBACK (small_angle_value_changed_cb),
			  self);
	g_signal_connect (G_OBJECT (self->priv->grid_adj),
			  "value-changed",
			  G_CALLBACK (grid_value_changed_cb),
			  self);
	g_signal_connect (GET_WIDGET ("grid_checkbutton"),
			  "toggled",
			  G_CALLBACK (grid_checkbutton_toggled_cb),
			  self);
	g_signal_connect (GET_WIDGET ("size_combobox"),
			  "changed",
			  G_CALLBACK (size_combobox_changed_cb),
			  self);
	g_signal_connect (GET_WIDGET ("background_colorbutton"),
			  "notify::color",
			  G_CALLBACK (background_colorbutton_notify_color_cb),
			  self);
	g_signal_connect (GET_WIDGET ("center_button"),
			  "clicked",
			  G_CALLBACK (center_button_clicked_cb),
			  self);

	self->priv->rotator = (GthImageRotator *) gth_image_rotator_new (GTH_IMAGE_VIEWER (viewer));
	gth_image_rotator_set_grid (self->priv->rotator, FALSE, eel_gconf_get_integer (PREF_ROTATE_GRID_SIZE, DEFAULT_GRID));
	_gth_file_tool_rotate (self, 0.0);
	gth_image_rotator_set_center (self->priv->rotator,
				      self->priv->pixbuf_width * 0.5,
				      self->priv->pixbuf_height * 0.5);

	g_signal_connect (self->priv->rotator,
			  "center-changed",
			  G_CALLBACK (rotator_center_changed_cb),
			  self);

	gth_image_viewer_set_tool (GTH_IMAGE_VIEWER (viewer), (GthImageViewerTool *) self->priv->rotator);

	return options;
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

	if (self->priv->builder != NULL) {
		int unit;

		/* save the dialog options */

		unit = gtk_combo_box_get_active (GTK_COMBO_BOX (GET_WIDGET ("unit_combobox")));
		eel_gconf_set_enum (PREF_ROTATE_UNIT, GTH_TYPE_UNIT, unit);
		eel_gconf_set_integer (PREF_ROTATE_GRID_SIZE, (int) gtk_adjustment_get_value (self->priv->grid_adj));

		/* destroy the options data */

		_g_clear_object (&self->priv->builder);
	}

	window = gth_file_tool_get_window (GTH_FILE_TOOL (self));
	viewer_page = gth_browser_get_viewer_page (GTH_BROWSER (window));
	viewer = gth_image_viewer_page_get_image_viewer (GTH_IMAGE_VIEWER_PAGE (viewer_page));
	gth_image_viewer_set_tool (GTH_IMAGE_VIEWER (viewer), NULL);
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
	self->priv->angle_step = 0.0;
	self->priv->use_grid = FALSE;
	gth_file_tool_construct (GTH_FILE_TOOL (self), "tool-rotate", _("Rotate..."), _("Rotate"), TRUE);
}


static void
gth_file_tool_rotate_finalize (GObject *object)
{
	GthFileToolRotate *self;

	g_return_if_fail (object != NULL);
	g_return_if_fail (GTH_IS_FILE_TOOL_ROTATE (object));

	self = (GthFileToolRotate *) object;
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
