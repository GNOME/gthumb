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
	GdkPixbuf        *dest_pixbuf;
	GtkBuilder       *builder;
	int               pixbuf_width;
	int               pixbuf_height;
	int               screen_width;
	int               screen_height;
	GtkWidget        *rotation_angle;
	GtkWidget        *auto_crop;
	guint             apply_event;
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
apply_button_clicked_cb (GtkButton         *button,
			 GthFileToolRotate *self)
{
	if (self->priv->dest_pixbuf != NULL) {
		GtkWidget *window;
		GtkWidget *viewer_page;

		window = gth_file_tool_get_window (GTH_FILE_TOOL (self));
		viewer_page = gth_browser_get_viewer_page (GTH_BROWSER (window));
		gth_image_viewer_page_set_pixbuf (GTH_IMAGE_VIEWER_PAGE (viewer_page), self->priv->dest_pixbuf, TRUE);
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


static gboolean
apply_cb (gpointer user_data)
{
	GthFileToolRotate *self = user_data;
	GtkWidget         *window;
	GtkWidget         *viewer_page;
	double             rotation_angle;
	gint               auto_crop;

	if (self->priv->apply_event != 0) {
		g_source_remove (self->priv->apply_event);
		self->priv->apply_event = 0;
	}

	window = gth_file_tool_get_window (GTH_FILE_TOOL (self));
	viewer_page = gth_browser_get_viewer_page (GTH_BROWSER (window));

	rotation_angle = gtk_range_get_value (GTK_RANGE (self->priv->rotation_angle));
	auto_crop = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (self->priv->auto_crop));
	
	_g_object_unref (self->priv->dest_pixbuf);
	self->priv->dest_pixbuf = _gdk_pixbuf_rotate (self->priv->src_pixbuf, rotation_angle, auto_crop);

	gth_image_viewer_page_set_pixbuf (GTH_IMAGE_VIEWER_PAGE (viewer_page), self->priv->dest_pixbuf, FALSE);
	
	return FALSE;
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


static GtkWidget *
gth_file_tool_rotate_get_options (GthFileTool *base)
{
	GthFileToolRotate *self;
	GtkWidget         *window;
	GtkWidget         *viewer_page;
	GtkWidget         *viewer;
	GtkWidget         *options;

	self = (GthFileToolRotate *) base;

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
	
	self->priv->pixbuf_width = gdk_pixbuf_get_width (self->priv->src_pixbuf);
	self->priv->pixbuf_height = gdk_pixbuf_get_height (self->priv->src_pixbuf);
	_gtk_widget_get_screen_size (window, &self->priv->screen_width, &self->priv->screen_height);

	self->priv->builder = _gtk_builder_new_from_file ("rotate-options.ui", "file_tools");

	options = _gtk_builder_get_widget (self->priv->builder, "options");
	gtk_widget_show (options);
	self->priv->rotation_angle = _gtk_builder_get_widget (self->priv->builder, "rotation_angle");
	self->priv->auto_crop = _gtk_builder_get_widget (self->priv->builder, "auto_crop");

	g_signal_connect (GET_WIDGET ("apply_button"),
			  "clicked",
			  G_CALLBACK (apply_button_clicked_cb),
			  self);
	g_signal_connect (GET_WIDGET ("cancel_button"),
			  "clicked",
			  G_CALLBACK (cancel_button_clicked_cb),
			  self);
	g_signal_connect (G_OBJECT (self->priv->rotation_angle),
			  "value-changed",
			  G_CALLBACK (value_changed_cb),
			  self);
	g_signal_connect (G_OBJECT (self->priv->auto_crop),
			  "toggled",
			  G_CALLBACK (value_changed_cb),
			  self);

	return options;
}


static void
gth_file_tool_rotate_destroy_options (GthFileTool *base)
{
	GthFileToolRotate *self;

	self = (GthFileToolRotate *) base;

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
gth_file_tool_rotate_activate (GthFileTool *base)
{
	gth_file_tool_show_options (base);
}


static void
gth_file_tool_rotate_instance_init (GthFileToolRotate *self)
{
	self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, GTH_TYPE_FILE_TOOL_ROTATE, GthFileToolRotatePrivate);

	gth_file_tool_construct (GTH_FILE_TOOL (self), "tool-rotate", _("Rotate..."), _("Rotate"), FALSE);
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
	_g_object_unref (self->priv->dest_pixbuf);
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
