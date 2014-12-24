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
#include "gth-file-tool-curves.h"
#include "gth-preview-tool.h"


#define GET_WIDGET(x) (_gtk_builder_get_widget (self->priv->builder, (x)))
#define APPLY_DELAY 150
#define PREVIEW_SIZE 0.9


G_DEFINE_TYPE (GthFileToolCurves, gth_file_tool_curves, GTH_TYPE_IMAGE_VIEWER_PAGE_TOOL)


typedef struct {
	double x;
	double y;
} Point;


typedef struct {
	Point *p;
	int    n;
} Points;


struct _GthFileToolCurvesPrivate {
	cairo_surface_t    *destination;
	cairo_surface_t    *preview;
	GtkBuilder         *builder;
	GthTask            *image_task;
	guint               apply_event;
	GthImageViewerTool *preview_tool;
	gboolean            view_original;
	gboolean            apply_to_original;
	gboolean            closing;
	Points              points[GTH_HISTOGRAM_N_CHANNELS];
};


/* -- Gauss-Jordan linear equation solver (for splines) -- */


typedef struct {
	double **v;
	int      r;
	int      c;
} Matrix;


static Matrix *
GJ_matrix_new (int r, int c)
{
	Matrix *m;
	int     i, j;

	m = g_new (Matrix, 1);
	m->r = r;
	m->c = c;
	m->v = g_new (double *, r);
	for (i = 0; i < r; i++) {
		m->v[i] = g_new (double, c);
		for (j = 0; j < c; j++)
			m->v[i][j] = 0.0;
	}

	return m;
}


static void
GJ_matrix_free (Matrix *m)
{
	int i;
	for (i = 0; i < m->r; i++)
		g_free (m->v[i]);
	g_free (m->v);
	g_free (m);
}


static void
GJ_swap_rows (double **m, int k, int l)
{
	double *t = m[k];
	m[k] = m[l];
	m[l] = t;
}


static gboolean
GJ_matrix_solve (Matrix *m, double *x)
{
	double **A = m->v;
	int      r = m->r;
	int      k, i, j;

	for (k = 0; k < r; k++) { // column
		// pivot for column
		int    i_max = 0;
		double vali = 0;

		for (i = k; i < r; i++) {
			if ((i == k) || (A[i][k] > vali)) {
				i_max = i;
				vali = A[i][k];
			}
		}
		GJ_swap_rows (A, k, i_max);

		if (A[i_max][i] == 0) {
			g_print ("matrix is singular!\n");
			return TRUE;
		}

		// for all rows below pivot
		for (i = k + 1; i < r; i++) {
			for (j = k + 1; j < r + 1; j++)
				A[i][j] = A[i][j] - A[k][j] * (A[i][k] / A[k][k]);
			A[i][k] = 0.0;
		}
	}

	for (i = r - 1; i >= 0; i--) { // rows = columns
		double v = A[i][r] / A[i][i];

		x[i] = v;
		for (j = i - 1; j >= 0; j--) { // rows
			A[j][r] -= A[j][i] * v;
			A[j][i] = 0.0;
		}
	}

	return FALSE;
}


/* -- points -- */


static void
points_init (Points *p, int n)
{
	p->n = n;
	p->p = g_new (Point, p->n);
}


static void
points_dispose (Points *p)
{
	if (p->p != NULL)
		g_free (p->p);
	points_init (p, 0);
}


static void
points_copy (Points *source, Points *dest)
{
	int i;

	points_init (dest, source->n);
	for (i = 0; i < source->n; i++) {
		dest->p[i].x = source->p[i].x;
		dest->p[i].y = source->p[i].y;
	}
}


/* -- spline function -- */


typedef struct {
	Points    points;
	double   *k;
	gboolean  is_singular;
} Spline;


static Spline *
spline_new (Points *points)
{
	Spline *spline;
	int     i;

	spline = g_new (Spline, 1);
	points_copy (points, &spline->points);
	spline->k = g_new (double, points->n + 1);
	for (i = 0; i < points->n + 1; i++)
		spline->k[i] = 1.0;
	spline->is_singular = FALSE;

	return spline;
}


static void
spline_free (Spline *spline)
{
	g_return_if_fail (spline != NULL);

	points_dispose (&spline->points);
	g_free (spline->k);
	g_free (spline);
}


static void
spline_set_natural_ks (Spline *spline)
{
	int      n = spline->points.n;
	Point   *p = spline->points.p;
	Matrix  *m;
	double **A;
	int      i;

	m = GJ_matrix_new (n+1, n+2);
	A = m->v;
	for (i = 1; i < n; i++) {
		A[i][i-1] = 1.0 / (p[i].x - p[i-1].x);
		A[i][i  ] = 2.0 * (1.0 / (p[i].x - p[i-1].x) + 1.0 / (p[i+1].x - p[i].x));
		A[i][i+1] = 1.0 / (p[i+1].x - p[i].x);
		A[i][n+1] = 3.0 * ( (p[i].y - p[i-1].y) / ((p[i].x - p[i-1].x) * (p[i].x - p[i-1].x)) + (p[i+1].y - p[i].y) / ((p[i+1].x - p[i].x) * (p[i+1].x - p[i].x)) );
	}

        A[0][0  ] = 2.0 / (p[1].x - p[0].x);
        A[0][1  ] = 1.0 / (p[1].x - p[0].x);
        A[0][n+1] = 3.0 * (p[1].y - p[0].y) / ((p[1].x - p[0].x) * (p[1].x - p[0].x));

        A[n][n-1] = 1.0 / (p[n].x - p[n-1].x);
        A[n][n  ] = 2.0 / (p[n].x - p[n-1].x);
        A[n][n+1] = 3.0 * (p[n].y - p[n-1].y) / ((p[n].x - p[n-1].x) * (p[n].x - p[n-1].x));

        spline->is_singular = GJ_matrix_solve (m, spline->k);

	GJ_matrix_free (m);
}


static int
spline_eval (Spline *spline, double x)
{
	Point  *p = spline->points.p;
	double *k = spline->k;
	int     i;

	for (i = 1; p[i].x < x; i++)
		/* void */;
	double t = (x - p[i-1].x) / (p[i].x - p[i-1].x);
	double a = k[i-1] * (p[i].x - p[i-1].x) - (p[i].y - p[i-1].y);
	double b = - k[i] * (p[i].x - p[i-1].x) + (p[i].y - p[i-1].y);
	double y = round ( ((1-t) * p[i-1].y) + (t * p[i].y) + (t * (1-t) * (a * (1-t) + b * t)) );

	return CLAMP (y, 0, 255);
}


/* -- apply_changes -- */


typedef struct {
	long   *value_map[GTH_HISTOGRAM_N_CHANNELS];
	Spline *spline[GTH_HISTOGRAM_N_CHANNELS];
} TaskData;


static void
curves_setup (TaskData        *task_data,
	      cairo_surface_t *source)
{
	long **value_map;
	int    c, v;

	for (c = 0; c < GTH_HISTOGRAM_N_CHANNELS; c++)
		spline_set_natural_ks (task_data->spline[c]);

	for (c = 0; c < GTH_HISTOGRAM_N_CHANNELS; c++) {
		task_data->value_map[c] = g_new (long, 256);
		if (task_data->spline[c]->is_singular) {
			for (v = 0; v <= 255; v++)
				task_data->value_map[c][v] = v;
		}
		else {
			for (v = 0; v <= 255; v++) {
				task_data->value_map[c][v] = spline_eval (task_data->spline[c], v);
				/*if (c == 0) FIXME
					g_print ("%d -> %ld\n", v, task_data->value_map[c][v]);*/
			}
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
task_data_new (Points *points)
{
	TaskData *task_data;
	int       c, n;

	task_data = g_new (TaskData, 1);
	for (c = 0; c < GTH_HISTOGRAM_N_CHANNELS; c++) {
		task_data->value_map[c] = NULL;
		task_data->spline[c] = spline_new (&points[c]);
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
		spline_free (task_data->spline[c]);
	for (c = 0; c < GTH_HISTOGRAM_N_CHANNELS; c++)
		g_free (task_data->value_map[c]);
	g_free (task_data);
}


static gboolean
apply_cb (gpointer user_data)
{
	GthFileToolCurves *self = user_data;
	GtkWidget         *window;
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

	task_data = task_data_new (self->priv->points);
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
	int                c;

	for (c = 0; c < GTH_HISTOGRAM_N_CHANNELS; c++) {
		points_init (&self->priv->points[c], 3);

		self->priv->points[c].p[0].x = 0;
		self->priv->points[c].p[0].y = 0;

		self->priv->points[c].p[1].x = 128;
		self->priv->points[c].p[1].y = 128;

		self->priv->points[c].p[2].x = 255;
		self->priv->points[c].p[2].y = 255;
	}

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

	g_signal_connect (GET_WIDGET ("preview_checkbutton"),
			  "toggled",
			  G_CALLBACK (preview_checkbutton_toggled_cb),
			  self);

	self->priv->preview_tool = gth_preview_tool_new ();
	gth_preview_tool_set_image (GTH_PREVIEW_TOOL (self->priv->preview_tool), self->priv->preview);
	gth_image_viewer_set_tool (GTH_IMAGE_VIEWER (viewer), self->priv->preview_tool);
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

	for (c = 0; c < GTH_HISTOGRAM_N_CHANNELS; c++) {
		/* points_init (&self->priv->points[c], 0); FIXME */
		points_init (&self->priv->points[c], 4);

		self->priv->points[c].p[0].x = 0;
		self->priv->points[c].p[0].y = 0;

		self->priv->points[c].p[1].x = 100;
		self->priv->points[c].p[1].y = 54;

		self->priv->points[c].p[2].x = 161;
		self->priv->points[c].p[2].y = 190;

		self->priv->points[c].p[3].x = 255;
		self->priv->points[c].p[3].y = 255;
	}

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
