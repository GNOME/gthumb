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
#include "cairo-blur.h"


#define GET_WIDGET(x) (_gtk_builder_get_widget (self->priv->builder, (x)))
#define APPLY_DELAY 150
#define DEFAULT_RADIUS 2.0
#define DEFAULT_AMOUNT 50.0
#define DEFAULT_THRESHOLD 0.0


static gpointer parent_class = NULL;


struct _GthFileToolSharpenPrivate {
	cairo_surface_t *source;
	cairo_surface_t *destination;
	GtkBuilder      *builder;
	GtkAdjustment   *radius_adj;
	GtkAdjustment   *amount_adj;
	GtkAdjustment   *threshold_adj;
	GtkWidget       *preview;
	GthTask         *pixbuf_task;
	guint            apply_event;
	gboolean         show_preview;
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


typedef struct {
	GthFileToolSharpen *self;
	cairo_surface_t    *source;
	cairo_surface_t    *destination;
	GtkWidget          *viewer_page;
	int                 radius;
	double              amount;
	int                 threshold;
} SharpenData;


static SharpenData *
sharpen_data_new (GthFileToolSharpen *self)
{
	SharpenData *sharpen_data;

	sharpen_data = g_new0 (SharpenData, 1);
	sharpen_data->source = NULL;
	sharpen_data->destination = NULL;
	sharpen_data->viewer_page = NULL;
	sharpen_data->radius = gtk_adjustment_get_value (self->priv->radius_adj);
	sharpen_data->amount = - gtk_adjustment_get_value (self->priv->amount_adj) / 100.0;
	sharpen_data->threshold = gtk_adjustment_get_value (self->priv->threshold_adj);

	return sharpen_data;
}


static void
sharpen_before (GthAsyncTask *task,
	        gpointer      user_data)
{
	gth_task_progress (GTH_TASK (task), _("Sharpening image"), NULL, TRUE, 0.0);
}


static gpointer
sharpen_exec (GthAsyncTask *task,
	      gpointer      user_data)
{
	SharpenData *sharpen_data = user_data;

	sharpen_data->destination = _cairo_image_surface_copy (sharpen_data->source);

	/* FIXME: set progress info and allow cancellation */

	_cairo_image_surface_sharpen (sharpen_data->destination,
				      sharpen_data->radius,
				      sharpen_data->amount,
				      sharpen_data->threshold);

	return NULL;
}


static void
sharpen_after (GthAsyncTask *task,
	       GError       *error,
	       gpointer      user_data)
{
	SharpenData *sharpen_data = user_data;

	if (error == NULL)
		gth_image_viewer_page_set_image (GTH_IMAGE_VIEWER_PAGE (sharpen_data->viewer_page), sharpen_data->destination, TRUE);
}


static void
sharpen_data_free (gpointer user_data)
{
	SharpenData *sharpen_data = user_data;

	_g_object_unref (sharpen_data->viewer_page);
	cairo_surface_destroy (sharpen_data->destination);
	cairo_surface_destroy (sharpen_data->source);
	g_free (sharpen_data);
}


static void
ok_button_clicked_cb (GtkButton          *button,
		      GthFileToolSharpen *self)
{
	GtkWidget   *window;
	GtkWidget   *viewer_page;
	SharpenData *sharpen_data;
	GthTask     *task;

	if (self->priv->apply_event != 0) {
		g_source_remove (self->priv->apply_event);
		self->priv->apply_event = 0;
	}

	window = gth_file_tool_get_window (GTH_FILE_TOOL (self));
	viewer_page = gth_browser_get_viewer_page (GTH_BROWSER (window));
	if (! GTH_IS_IMAGE_VIEWER_PAGE (viewer_page))
		return;

	sharpen_data = sharpen_data_new (self);
	sharpen_data->viewer_page = g_object_ref (viewer_page);
	sharpen_data->source = cairo_surface_reference (self->priv->source);
	task = gth_async_task_new (sharpen_before,
				   sharpen_exec,
				   sharpen_after,
				   sharpen_data,
				   sharpen_data_free);
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


static gboolean
apply_cb (gpointer user_data)
{
	GthFileToolSharpen *self = user_data;
	GthImageViewer     *preview;

	if (self->priv->apply_event != 0) {
		g_source_remove (self->priv->apply_event);
		self->priv->apply_event = 0;
	}

	preview = GTH_IMAGE_VIEWER (self->priv->preview);
	if (self->priv->show_preview) {
		SharpenData     *sharpen_data;
		int              x, y, w ,h;
		cairo_surface_t *preview_surface;
		cairo_t         *cr;

		sharpen_data = sharpen_data_new (self);
		x = gtk_adjustment_get_value (preview->hadj);
		y = gtk_adjustment_get_value (preview->vadj);
		w = gtk_adjustment_get_page_size (preview->hadj);
		h = gtk_adjustment_get_page_size (preview->vadj);

		cairo_surface_destroy (self->priv->destination);
		self->priv->destination = _cairo_image_surface_copy (self->priv->source);

		/* FIXME: use a cairo sub-surface when cairo 1.10 will be requiered */

		preview_surface = _cairo_image_surface_copy_subsurface (self->priv->destination, x, y, w, h);
		_cairo_image_surface_sharpen (preview_surface,
					      sharpen_data->radius,
					      sharpen_data->amount,
					      sharpen_data->threshold);

		cr = cairo_create (self->priv->destination);
		cairo_set_source_surface (cr, preview_surface, x, y);
		cairo_rectangle (cr, x, y, w, h);
		cairo_fill (cr);
		cairo_destroy (cr);

		gth_image_viewer_set_image (preview, self->priv->destination, -1, -1);

		cairo_surface_destroy (preview_surface);
		sharpen_data_free (sharpen_data);
	}
	else
		gth_image_viewer_set_image (preview, self->priv->source, -1, -1);

	return FALSE;
}


static void
value_changed_cb (GtkAdjustment      *adj,
		  GthFileToolSharpen *self)
{
	if (self->priv->apply_event != 0) {
		g_source_remove (self->priv->apply_event);
		self->priv->apply_event = 0;
	}
	self->priv->apply_event = g_timeout_add (APPLY_DELAY, apply_cb, self);
}


static void
preview_checkbutton_toggled_cb (GtkToggleButton    *toggle_button,
				GthFileToolSharpen *self)
{
	self->priv->show_preview = gtk_toggle_button_get_active (toggle_button);
	if (self->priv->apply_event != 0) {
		g_source_remove (self->priv->apply_event);
		self->priv->apply_event = 0;
	}
	apply_cb (self);
}


static GtkWidget *
gth_file_tool_sharpen_get_options (GthFileTool *base)
{
	GthFileToolSharpen *self;
	GtkWidget          *window;
	GtkWidget          *viewer_page;
	GtkWidget          *viewer;
	GtkWidget          *options;
	GtkWidget          *image_navigator;

	self = (GthFileToolSharpen *) base;

	window = gth_file_tool_get_window (base);
	viewer_page = gth_browser_get_viewer_page (GTH_BROWSER (window));
	if (! GTH_IS_IMAGE_VIEWER_PAGE (viewer_page))
		return NULL;

	cairo_surface_destroy (self->priv->source);
	cairo_surface_destroy (self->priv->destination);

	viewer = gth_image_viewer_page_get_image_viewer (GTH_IMAGE_VIEWER_PAGE (viewer_page));
	self->priv->source = cairo_surface_reference (gth_image_viewer_get_current_image (GTH_IMAGE_VIEWER (viewer)));
	if (self->priv->source == NULL)
		return NULL;

	self->priv->destination = NULL;

	self->priv->builder = _gtk_builder_new_from_file ("sharpen-options.ui", "file_tools");
	options = _gtk_builder_get_widget (self->priv->builder, "options");
	gtk_widget_show (options);

	self->priv->preview = gth_image_viewer_new ();
	gth_image_viewer_set_reset_scrollbars (GTH_IMAGE_VIEWER (self->priv->preview), FALSE);
	gth_image_viewer_set_fit_mode (GTH_IMAGE_VIEWER (self->priv->preview), GTH_FIT_NONE);
	gth_image_viewer_set_zoom_change (GTH_IMAGE_VIEWER (self->priv->preview), GTH_ZOOM_CHANGE_KEEP_PREV);
	gth_image_viewer_set_zoom (GTH_IMAGE_VIEWER (self->priv->preview), 1.0);
	gth_image_viewer_enable_zoom_with_keys (GTH_IMAGE_VIEWER (self->priv->preview), FALSE);
	gth_image_viewer_set_image (GTH_IMAGE_VIEWER (self->priv->preview), self->priv->source, -1, -1);
	image_navigator = gth_image_navigator_new (GTH_IMAGE_VIEWER (self->priv->preview));
	gtk_widget_show_all (image_navigator);
	gtk_box_pack_start (GTK_BOX (GET_WIDGET ("preview_hbox")), image_navigator, TRUE, TRUE, 0);

	self->priv->amount_adj = gimp_scale_entry_new (GET_WIDGET ("amount_hbox"),
						       GTK_LABEL (GET_WIDGET ("amount_label")),
						       DEFAULT_AMOUNT, 0.0, 500.0, 1.0, 10.0, 0);
	self->priv->radius_adj = gimp_scale_entry_new (GET_WIDGET ("radius_hbox"),
						       GTK_LABEL (GET_WIDGET ("radius_label")),
						       DEFAULT_RADIUS, 0.0, 10.0, 1.0, 1.0, 0);
	self->priv->threshold_adj = gimp_scale_entry_new (GET_WIDGET ("threshold_hbox"),
							  GTK_LABEL (GET_WIDGET ("threshold_label")),
							  DEFAULT_THRESHOLD, 0.0, 255.0, 1.0, 10.0, 0);

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
	g_signal_connect (GTH_IMAGE_VIEWER (self->priv->preview)->hadj,
			  "value-changed",
			  G_CALLBACK (value_changed_cb),
			  self);
	g_signal_connect (GTH_IMAGE_VIEWER (self->priv->preview)->vadj,
			  "value-changed",
			  G_CALLBACK (value_changed_cb),
			  self);
	g_signal_connect (GET_WIDGET ("preview_checkbutton"),
			  "clicked",
			  G_CALLBACK (preview_checkbutton_toggled_cb),
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

	cairo_surface_destroy (self->priv->source);
	cairo_surface_destroy (self->priv->destination);
	_g_object_unref (self->priv->builder);
	self->priv->source = NULL;
	self->priv->destination = NULL;
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
	self->priv->source = NULL;
	self->priv->destination = NULL;
	self->priv->builder = NULL;
	self->priv->show_preview = TRUE;

	gth_file_tool_construct (GTH_FILE_TOOL (self), "tool-sharpen", _("Enhance Focus..."), _("Enhance Focus"), FALSE);
}


static void
gth_file_tool_sharpen_finalize (GObject *object)
{
	GthFileToolSharpen *self;

	g_return_if_fail (object != NULL);
	g_return_if_fail (GTH_IS_FILE_TOOL_SHARPEN (object));

	self = (GthFileToolSharpen *) object;

	cairo_surface_destroy (self->priv->source);
	cairo_surface_destroy (self->priv->destination);
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
