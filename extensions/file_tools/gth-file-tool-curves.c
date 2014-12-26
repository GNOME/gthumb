/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2014 Free Software Foundation, Inc.
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
#include "gth-curve-editor.h"
#include "gth-file-tool-curves.h"
#include "gth-curve.h"
#include "gth-points.h"
#include "gth-preview-tool.h"


#define GET_WIDGET(x) (_gtk_builder_get_widget (self->priv->builder, (x)))
#define APPLY_DELAY 150
#define PREVIEW_SIZE 0.9


G_DEFINE_TYPE (GthFileToolCurves, gth_file_tool_curves, GTH_TYPE_IMAGE_VIEWER_PAGE_TOOL)


struct _GthFileToolCurvesPrivate {
	cairo_surface_t    *destination;
	cairo_surface_t    *preview;
	GtkBuilder         *builder;
	GthTask            *image_task;
	guint               apply_event;
	GthImageViewerTool *preview_tool;
	GthHistogram       *histogram;
	gboolean            view_original;
	gboolean            apply_to_original;
	gboolean            closing;
	GtkWidget          *curve_editor;
};


/* -- apply_changes -- */


typedef struct {
	long     *value_map[GTH_HISTOGRAM_N_CHANNELS];
	GthCurve *curve[GTH_HISTOGRAM_N_CHANNELS];
} TaskData;


static void
curves_setup (TaskData        *task_data,
	      cairo_surface_t *source)
{
	long **value_map;
	int    c, v;

	for (c = GTH_HISTOGRAM_CHANNEL_VALUE; c <= GTH_HISTOGRAM_CHANNEL_BLUE; c++) {
		task_data->value_map[c] = g_new (long, 256);
		for (v = 0; v <= 255; v++) {
			double u = gth_curve_eval (task_data->curve[c], v);
			if (c > GTH_HISTOGRAM_CHANNEL_VALUE)
				u = task_data->value_map[GTH_HISTOGRAM_CHANNEL_VALUE][(int)u];
			task_data->value_map[c][v] = u;
		}
	}
}


static inline guchar
value_func (TaskData *task_data,
	    int       n_channel,
	    guchar    value)
{
	return (guchar) task_data->value_map[n_channel][value];
}


static gpointer
curves_exec (GthAsyncTask *task,
	     gpointer      user_data)
{
	TaskData        *task_data = user_data;
	cairo_surface_t *source;
	cairo_format_t   format;
	int              width;
	int              height;
	int              source_stride;
	cairo_surface_t *destination;
	int              destination_stride;
	unsigned char   *p_source_line;
	unsigned char   *p_destination_line;
	unsigned char   *p_source;
	unsigned char   *p_destination;
	gboolean         cancelled;
	double           progress;
	int              x, y;
	unsigned char    red, green, blue, alpha;

	/* initialize the extra data */

	source = gth_image_task_get_source_surface (GTH_IMAGE_TASK (task));
	curves_setup (task_data, source);

	/* convert the image */

	format = cairo_image_surface_get_format (source);
	width = cairo_image_surface_get_width (source);
	height = cairo_image_surface_get_height (source);
	source_stride = cairo_image_surface_get_stride (source);

	destination = cairo_image_surface_create (format, width, height);
	destination_stride = cairo_image_surface_get_stride (destination);
	p_source_line = _cairo_image_surface_flush_and_get_data (source);
	p_destination_line = _cairo_image_surface_flush_and_get_data (destination);
	for (y = 0; y < height; y++) {
		gth_async_task_get_data (task, NULL, &cancelled, NULL);
		if (cancelled) {
			cairo_surface_destroy (destination);
			cairo_surface_destroy (source);
			return NULL;
		}

		progress = (double) y / height;
		gth_async_task_set_data (task, NULL, NULL, &progress);

		p_source = p_source_line;
		p_destination = p_destination_line;
		for (x = 0; x < width; x++) {
			CAIRO_GET_RGBA (p_source, red, green, blue, alpha);
			red   = value_func (task_data, GTH_HISTOGRAM_CHANNEL_RED, red);
			green = value_func (task_data, GTH_HISTOGRAM_CHANNEL_GREEN, green);
			blue  = value_func (task_data, GTH_HISTOGRAM_CHANNEL_BLUE, blue);
			CAIRO_SET_RGBA (p_destination, red, green, blue, alpha);

			p_source += 4;
			p_destination += 4;
		}
		p_source_line += source_stride;
		p_destination_line += destination_stride;
	}

	cairo_surface_mark_dirty (destination);
	gth_image_task_set_destination_surface (GTH_IMAGE_TASK (task), destination);

	cairo_surface_destroy (destination);
	cairo_surface_destroy (source);

	return NULL;
}


static void apply_changes (GthFileToolCurves *self);


static void
image_task_completed_cb (GthTask  *task,
			 GError   *error,
			 gpointer  user_data)
{
	GthFileToolCurves *self = user_data;
	GthImage	  *destination_image;

	self->priv->image_task = NULL;

	if (self->priv->closing) {
		g_object_unref (task);
		gth_image_viewer_page_tool_reset_image (GTH_IMAGE_VIEWER_PAGE_TOOL (self));
		return;
	}

	if (error != NULL) {
		if (g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
			apply_changes (self);
		g_object_unref (task);
		return;
	}

	destination_image = gth_image_task_get_destination (GTH_IMAGE_TASK (task));
	if (destination_image == NULL) {
		g_object_unref (task);
		return;
	}

	cairo_surface_destroy (self->priv->destination);
	self->priv->destination = gth_image_get_cairo_surface (destination_image);

	if (self->priv->apply_to_original) {
		if (self->priv->destination != NULL) {
			GtkWidget *window;
			GtkWidget *viewer_page;

			window = gth_file_tool_get_window (GTH_FILE_TOOL (self));
			viewer_page = gth_browser_get_viewer_page (GTH_BROWSER (window));
			gth_image_viewer_page_set_image (GTH_IMAGE_VIEWER_PAGE (viewer_page), self->priv->destination, TRUE);
		}

		gth_file_tool_hide_options (GTH_FILE_TOOL (self));
	}
	else {
		if (! self->priv->view_original)
			gth_preview_tool_set_image (GTH_PREVIEW_TOOL (self->priv->preview_tool), self->priv->destination);
	}

	g_object_unref (task);
}


static TaskData *
task_data_new (GthPoints *points)
{
	TaskData *task_data;
	int       c, n;

	task_data = g_new (TaskData, 1);
	for (c = 0; c < GTH_HISTOGRAM_N_CHANNELS; c++) {
		task_data->value_map[c] = NULL;
		task_data->curve[c] = gth_bezier_new (&points[c]);
	}

	return task_data;
}


static void
task_data_destroy (gpointer user_data)
{
	TaskData *task_data = user_data;
	int       c;

	if (task_data == NULL)
		return;

	for (c = 0; c < GTH_HISTOGRAM_N_CHANNELS; c++)
		g_object_unref (task_data->curve[c]);
	for (c = 0; c < GTH_HISTOGRAM_N_CHANNELS; c++)
		g_free (task_data->value_map[c]);
	g_free (task_data);
}


static gboolean
apply_cb (gpointer user_data)
{
	GthFileToolCurves *self = user_data;
	GtkWidget         *window;
	GthPoints          points[GTH_HISTOGRAM_N_CHANNELS];
	int                c;
	TaskData          *task_data;

	if (self->priv->apply_event != 0) {
		g_source_remove (self->priv->apply_event);
		self->priv->apply_event = 0;
	}

	if (self->priv->image_task != NULL) {
		gth_task_cancel (self->priv->image_task);
		return FALSE;
	}

	window = gth_file_tool_get_window (GTH_FILE_TOOL (self));

	for (c = 0; c <= GTH_HISTOGRAM_N_CHANNELS; c++)
		gth_points_init (points + c, 0);
	gth_curve_editor_get_points (GTH_CURVE_EDITOR (self->priv->curve_editor), points);
	task_data = task_data_new (points);
	self->priv->image_task =  gth_image_task_new (_("Applying changes"),
						      NULL,
						      curves_exec,
						      NULL,
						      task_data,
						      task_data_destroy);
	if (self->priv->apply_to_original)
		gth_image_task_set_source_surface (GTH_IMAGE_TASK (self->priv->image_task), gth_image_viewer_page_tool_get_source (GTH_IMAGE_VIEWER_PAGE_TOOL (self)));
	else
		gth_image_task_set_source_surface (GTH_IMAGE_TASK (self->priv->image_task), self->priv->preview);

	g_signal_connect (self->priv->image_task,
			  "completed",
			  G_CALLBACK (image_task_completed_cb),
			  self);
	gth_browser_exec_task (GTH_BROWSER (window), self->priv->image_task, FALSE);

	return FALSE;
}


static void
apply_changes (GthFileToolCurves *self)
{
	if (self->priv->apply_event != 0) {
		g_source_remove (self->priv->apply_event);
		self->priv->apply_event = 0;
	}
	self->priv->apply_event = g_timeout_add (APPLY_DELAY, apply_cb, self);
}


static void
reset_button_clicked_cb (GtkButton *button,
		  	 gpointer   user_data)
{
	GthFileToolCurves *self = user_data;
	gth_curve_editor_reset (GTH_CURVE_EDITOR (self->priv->curve_editor));
}


static void
curve_editor_changed_cb (GthCurveEditor *curve_editor,
			 gpointer        user_data)
{
	GthFileToolCurves *self = user_data;
	apply_changes (self);
}


static void
preview_checkbutton_toggled_cb (GtkToggleButton *togglebutton,
				gpointer         user_data)
{
	GthFileToolCurves *self = user_data;

	self->priv->view_original = ! gtk_toggle_button_get_active (togglebutton);
	if (self->priv->view_original)
		gth_preview_tool_set_image (GTH_PREVIEW_TOOL (self->priv->preview_tool), self->priv->preview);
	else
		gth_preview_tool_set_image (GTH_PREVIEW_TOOL (self->priv->preview_tool), self->priv->destination);
}


static GtkWidget *
gth_file_tool_curves_get_options (GthFileTool *base)
{
	GthFileToolCurves *self;
	GtkWidget         *viewer_page;
	GtkWidget         *viewer;
	cairo_surface_t   *source;
	GtkWidget         *options;
	int                width, height;
	GtkAllocation      allocation;

	self = (GthFileToolCurves *) base;

	viewer_page = gth_image_viewer_page_tool_get_page (GTH_IMAGE_VIEWER_PAGE_TOOL (self));
	if (viewer_page == NULL)
		return NULL;

	_cairo_clear_surface (&self->priv->destination);
	_cairo_clear_surface (&self->priv->preview);

	source = gth_image_viewer_page_tool_get_source (GTH_IMAGE_VIEWER_PAGE_TOOL (self));
	if (source == NULL)
		return NULL;

	width = cairo_image_surface_get_width (source);
	height = cairo_image_surface_get_height (source);
	viewer = gth_image_viewer_page_get_image_viewer (GTH_IMAGE_VIEWER_PAGE (viewer_page));
	gtk_widget_get_allocation (GTK_WIDGET (viewer), &allocation);
	if (scale_keeping_ratio (&width, &height, PREVIEW_SIZE * allocation.width, PREVIEW_SIZE * allocation.height, FALSE))
		self->priv->preview = _cairo_image_surface_scale_bilinear (source, width, height);
	else
		self->priv->preview = cairo_surface_reference (source);

	self->priv->destination = cairo_surface_reference (self->priv->preview);
	self->priv->apply_to_original = FALSE;
	self->priv->view_original = FALSE;
	self->priv->closing = FALSE;

	self->priv->builder = _gtk_builder_new_from_file ("curves-options.ui", "file_tools");
	options = _gtk_builder_get_widget (self->priv->builder, "options");
	gtk_widget_show (options);

	self->priv->curve_editor = gth_curve_editor_new (self->priv->histogram);
	gtk_widget_show (self->priv->curve_editor);
	gtk_box_pack_start (GTK_BOX (GET_WIDGET ("curves_box")), self->priv->curve_editor, TRUE, TRUE, 0);

	g_signal_connect (self->priv->curve_editor,
			  "changed",
			  G_CALLBACK (curve_editor_changed_cb),
			  self);

	g_signal_connect (GET_WIDGET ("preview_checkbutton"),
			  "toggled",
			  G_CALLBACK (preview_checkbutton_toggled_cb),
			  self);

	self->priv->preview_tool = gth_preview_tool_new ();
	gth_preview_tool_set_image (GTH_PREVIEW_TOOL (self->priv->preview_tool), self->priv->preview);
	gth_image_viewer_set_tool (GTH_IMAGE_VIEWER (viewer), self->priv->preview_tool);
	gth_histogram_calculate_for_image (self->priv->histogram, self->priv->preview);
	apply_changes (self);

	return options;
}


static void
gth_file_tool_curves_destroy_options (GthFileTool *base)
{
	GthFileToolCurves *self;
	GtkWidget         *viewer_page;

	self = (GthFileToolCurves *) base;

	if (self->priv->apply_event != 0) {
		g_source_remove (self->priv->apply_event);
		self->priv->apply_event = 0;
	}

	viewer_page = gth_image_viewer_page_tool_get_page (GTH_IMAGE_VIEWER_PAGE_TOOL (self));
	gth_image_viewer_page_reset_viewer_tool (GTH_IMAGE_VIEWER_PAGE (viewer_page));
	gth_viewer_page_update_sensitivity (GTH_VIEWER_PAGE (viewer_page));

	_cairo_clear_surface (&self->priv->preview);
	_cairo_clear_surface (&self->priv->destination);
	_g_clear_object (&self->priv->builder);
}


static void
gth_file_tool_curves_apply_options (GthFileTool *base)
{
	GthFileToolCurves *self;

	self = (GthFileToolCurves *) base;
	self->priv->apply_to_original = TRUE;
	apply_changes (self);
}


static void
gth_file_tool_curves_populate_headerbar (GthFileTool *base,
					 GthBrowser  *browser)
{
	GthFileToolCurves *self;
	gboolean           rtl;
	GtkWidget         *button;

	self = (GthFileToolCurves *) base;

	/* reset button */

	rtl = gtk_widget_get_direction (GTK_WIDGET (base)) == GTK_TEXT_DIR_RTL;
	button = gth_browser_add_header_bar_button (browser,
						    GTH_BROWSER_HEADER_SECTION_EDITOR_COMMANDS,
						    rtl ? "edit-undo-rtl-symbolic" : "edit-undo-symbolic",
						    _("Reset"),
						    NULL,
						    NULL);
	g_signal_connect (button,
			  "clicked",
			  G_CALLBACK (reset_button_clicked_cb),
			  self);
}


static void
gth_file_tool_sharpen_reset_image (GthImageViewerPageTool *base)
{
	GthFileToolCurves *self = (GthFileToolCurves *) base;

	if (self->priv->image_task != NULL) {
		self->priv->closing = TRUE;
		gth_task_cancel (self->priv->image_task);
		return;
	}

	if (self->priv->apply_event != 0) {
		g_source_remove (self->priv->apply_event);
		self->priv->apply_event = 0;
	}

	gth_image_viewer_page_reset (GTH_IMAGE_VIEWER_PAGE (gth_image_viewer_page_tool_get_page (GTH_IMAGE_VIEWER_PAGE_TOOL (self))));
	gth_file_tool_hide_options (GTH_FILE_TOOL (self));
}


static void
gth_file_tool_curves_init (GthFileToolCurves *self)
{
	int c;

	self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, GTH_TYPE_FILE_TOOL_CURVES, GthFileToolCurvesPrivate);
	self->priv->preview = NULL;
	self->priv->destination = NULL;
	self->priv->builder = NULL;
	self->priv->image_task = NULL;
	self->priv->view_original = FALSE;
	self->priv->histogram = gth_histogram_new ();

	gth_file_tool_construct (GTH_FILE_TOOL (self), "curves-symbolic", _("Curves"), GTH_TOOLBOX_SECTION_COLORS);
	gtk_widget_set_tooltip_text (GTK_WIDGET (self), _("Adjust color curves"));
}


static void
gth_file_tool_curves_finalize (GObject *object)
{
	GthFileToolCurves *self;

	g_return_if_fail (object != NULL);
	g_return_if_fail (GTH_IS_FILE_TOOL_CURVES (object));

	self = (GthFileToolCurves *) object;

	cairo_surface_destroy (self->priv->preview);
	cairo_surface_destroy (self->priv->destination);
	_g_object_unref (self->priv->builder);
	_g_object_unref (self->priv->histogram);

	G_OBJECT_CLASS (gth_file_tool_curves_parent_class)->finalize (object);
}


static void
gth_file_tool_curves_class_init (GthFileToolCurvesClass *klass)
{
	GObjectClass                *gobject_class;
	GthFileToolClass            *file_tool_class;
	GthImageViewerPageToolClass *image_viewer_page_tool_class;

	g_type_class_add_private (klass, sizeof (GthFileToolCurvesPrivate));

	gobject_class = (GObjectClass*) klass;
	gobject_class->finalize = gth_file_tool_curves_finalize;

	file_tool_class = (GthFileToolClass *) klass;
	file_tool_class->get_options = gth_file_tool_curves_get_options;
	file_tool_class->destroy_options = gth_file_tool_curves_destroy_options;
	file_tool_class->apply_options = gth_file_tool_curves_apply_options;
	file_tool_class->populate_headerbar = gth_file_tool_curves_populate_headerbar;

	image_viewer_page_tool_class = (GthImageViewerPageToolClass *) klass;
	image_viewer_page_tool_class->reset_image = gth_file_tool_sharpen_reset_image;
}
