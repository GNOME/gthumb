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
#include <gthumb.h>
#include <extensions/image_viewer/gth-image-viewer-page.h>
#include "gth-file-tool-sharpen.h"
#include "gdk-pixbuf-blur.h"


#define GET_WIDGET(x) (_gtk_builder_get_widget (self->priv->builder, (x)))
#define APPLY_DELAY 150
#define DEFAULT_RADIUS 2.0
#define DEFAULT_AMOUNT 50.0
#define DEFAULT_THRESHOLD 1.0


static gpointer parent_class = NULL;


struct _GthFileToolSharpenPrivate {
	GdkPixbuf     *src_pixbuf;
	GdkPixbuf     *dest_pixbuf;
	GtkBuilder    *builder;
	GtkAdjustment *radius_adj;
	GtkAdjustment *amount_adj;
	GtkAdjustment *threshold_adj;
	GthTask       *pixbuf_task;
	guint          apply_event;
};


static void
gth_file_tool_sharpen_update_sensitivity (GthFileTool *base)
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
task_completed_cb (GthTask  *task,
		   GError   *error,
		   gpointer  user_data)
{
	GthFileTool *base = user_data;

	if (error == NULL) {
		GthPixbufTask *pixbuf_task;
		GtkWidget     *viewer_page;

		pixbuf_task = GTH_PIXBUF_TASK (task);
		viewer_page = gth_browser_get_viewer_page (GTH_BROWSER (gth_file_tool_get_window (base)));
		gth_image_viewer_page_set_pixbuf (GTH_IMAGE_VIEWER_PAGE (viewer_page), pixbuf_task->dest, TRUE);
	}
}


typedef struct {
	int    radius;
	double amount;
	int    threshold;
} SharpenData;


static void
sharpen_step (GthPixbufTask *pixbuf_task)
{
	SharpenData *sharpen_data = pixbuf_task->data;

	pixbuf_task->dest = _gdk_pixbuf_sharpen (pixbuf_task->src,
						 sharpen_data->radius,
						 sharpen_data->amount,
						 sharpen_data->threshold);
}


static void
ok_button_clicked_cb (GtkButton          *button,
		      GthFileToolSharpen *self)
{
	SharpenData *sharpen_data;
	GthTask     *task;

	sharpen_data = g_new (SharpenData, 1);
	sharpen_data->radius = gtk_adjustment_get_value (self->priv->radius_adj);
	sharpen_data->amount = - gtk_adjustment_get_value (self->priv->amount_adj) / 100.0;
	sharpen_data->threshold = gtk_adjustment_get_value (self->priv->threshold_adj);
	task = gth_pixbuf_task_new (_("Sharpening image"),
				    TRUE,
				    NULL,
				    sharpen_step,
				    NULL,
				    sharpen_data,
				    g_free);
	gth_pixbuf_task_set_source (GTH_PIXBUF_TASK (task), self->priv->src_pixbuf);
	g_signal_connect (task, "completed", G_CALLBACK (task_completed_cb), self);
	gth_browser_exec_task (GTH_BROWSER (gth_file_tool_get_window (GTH_FILE_TOOL (self))), task, FALSE);

	g_object_unref (task);

	gth_file_tool_hide_options (GTH_FILE_TOOL (self));
}


static void
cancel_button_clicked_cb (GtkButton          *button,
			  GthFileToolSharpen *self)
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
reset_button_clicked_cb (GtkButton          *button,
			 GthFileToolSharpen *self)
{
	gtk_adjustment_set_value (self->priv->radius_adj, DEFAULT_RADIUS);
	gtk_adjustment_set_value (self->priv->amount_adj, DEFAULT_AMOUNT);
	gtk_adjustment_set_value (self->priv->threshold_adj, DEFAULT_THRESHOLD);
}


static void
value_changed_cb (GtkAdjustment      *adj,
		  GthFileToolSharpen *self)
{
	if (self->priv->apply_event != 0) {
		g_source_remove (self->priv->apply_event);
		self->priv->apply_event = 0;
	}
	/* FIXME
	self->priv->apply_event = g_timeout_add (APPLY_DELAY, apply_cb, self);
	*/
}


static GtkWidget *
gth_file_tool_sharpen_get_options (GthFileTool *base)
{
	GthFileToolSharpen *self;
	GtkWidget          *window;
	GtkWidget          *viewer_page;
	GtkWidget          *viewer;
	GtkWidget          *options;

	self = (GthFileToolSharpen *) base;

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

	self->priv->builder = _gtk_builder_new_from_file ("sharpen-options.ui", "file_tools");
	options = _gtk_builder_get_widget (self->priv->builder, "options");
	gtk_widget_show (options);

	/* FIXME: add a preview here
	self->priv->histogram_view = gth_histogram_view_new (self->priv->histogram);
	gtk_widget_show (self->priv->histogram_view);
	gtk_box_pack_start (GTK_BOX (GET_WIDGET ("histogram_hbox")), self->priv->histogram_view, TRUE, TRUE, 0);
	*/

	self->priv->radius_adj = gimp_scale_entry_new (GET_WIDGET ("radius_hbox"),
						       GTK_LABEL (GET_WIDGET ("radius_label")),
						       DEFAULT_RADIUS, 0.0, 10.0, 0.1, 1.0, 1);
	self->priv->amount_adj = gimp_scale_entry_new (GET_WIDGET ("amount_hbox"),
						       GTK_LABEL (GET_WIDGET ("amount_label")),
						       DEFAULT_AMOUNT, 0.0, 200.0, 1.0, 10.0, 0);
	self->priv->threshold_adj = gimp_scale_entry_new (GET_WIDGET ("threshold_hbox"),
							  GTK_LABEL (GET_WIDGET ("threshold_label")),
							  DEFAULT_THRESHOLD, 1.0, 255.0, 1.0, 10.0, 0);

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
	g_signal_connect (G_OBJECT (self->priv->radius_adj),
			  "value-changed",
			  G_CALLBACK (value_changed_cb),
			  self);
	g_signal_connect (G_OBJECT (self->priv->amount_adj),
			  "value-changed",
			  G_CALLBACK (value_changed_cb),
			  self);
	g_signal_connect (G_OBJECT (self->priv->threshold_adj),
			  "value-changed",
			  G_CALLBACK (value_changed_cb),
			  self);

	return options;
}


static void
gth_file_tool_sharpen_destroy_options (GthFileTool *base)
{
	GthFileToolSharpen *self;

	self = (GthFileToolSharpen *) base;

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
gth_file_tool_sharpen_activate (GthFileTool *base)
{
	gth_file_tool_show_options (base);
}


static void
gth_file_tool_sharpen_instance_init (GthFileToolSharpen *self)
{
	self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, GTH_TYPE_FILE_TOOL_SHARPEN, GthFileToolSharpenPrivate);
	self->priv->src_pixbuf = NULL;
	self->priv->dest_pixbuf = NULL;
	self->priv->builder = NULL;

	gth_file_tool_construct (GTH_FILE_TOOL (self), "tool-sharpen", _("Enhance Focus..."), _("Enhance Focus"), FALSE);
}


static void
gth_file_tool_sharpen_finalize (GObject *object)
{
	GthFileToolSharpen *self;

	g_return_if_fail (object != NULL);
	g_return_if_fail (GTH_IS_FILE_TOOL_SHARPEN (object));

	self = (GthFileToolSharpen *) object;

	_g_object_unref (self->priv->src_pixbuf);
	_g_object_unref (self->priv->dest_pixbuf);
	_g_object_unref (self->priv->builder);

	/* Chain up */
	G_OBJECT_CLASS (parent_class)->finalize (object);
}


static void
gth_file_tool_sharpen_class_init (GthFileToolSharpenClass *klass)
{
	GObjectClass     *gobject_class;
	GthFileToolClass *file_tool_class;

	parent_class = g_type_class_peek_parent (klass);
	g_type_class_add_private (klass, sizeof (GthFileToolSharpenPrivate));

	gobject_class = (GObjectClass*) klass;
	gobject_class->finalize = gth_file_tool_sharpen_finalize;

	file_tool_class = (GthFileToolClass *) klass;
	file_tool_class->update_sensitivity = gth_file_tool_sharpen_update_sensitivity;
	file_tool_class->activate = gth_file_tool_sharpen_activate;
	file_tool_class->get_options = gth_file_tool_sharpen_get_options;
	file_tool_class->destroy_options = gth_file_tool_sharpen_destroy_options;
}


GType
gth_file_tool_sharpen_get_type (void) {
	static GType type_id = 0;
	if (type_id == 0) {
		static const GTypeInfo g_define_type_info = {
			sizeof (GthFileToolSharpenClass),
			(GBaseInitFunc) NULL,
			(GBaseFinalizeFunc) NULL,
			(GClassInitFunc) gth_file_tool_sharpen_class_init,
			(GClassFinalizeFunc) NULL,
			NULL,
			sizeof (GthFileToolSharpen),
			0,
			(GInstanceInitFunc) gth_file_tool_sharpen_instance_init,
			NULL
		};
		type_id = g_type_register_static (GTH_TYPE_FILE_TOOL, "GthFileToolSharpen", &g_define_type_info, 0);
	}
	return type_id;
}
