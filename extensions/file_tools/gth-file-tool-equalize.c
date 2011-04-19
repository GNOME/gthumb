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
#include <math.h>
#include <gthumb.h>
#include <extensions/image_viewer/gth-image-viewer-page.h>
#include "gth-file-tool-equalize.h"


typedef struct {
	GtkWidget        *viewer_page;
	cairo_surface_t  *source;
	cairo_surface_t  *destination;
	GthHistogram     *histogram;
	int             **part;
} EqualizeData;


static void
equalize_histogram_setup (GthHistogram  *hist,
			  int          **part)
{
	int  i, k, j;
	int  pixels_per_value;
	int  desired;
	int  sum, dif;

	pixels_per_value = gth_histogram_get_count (hist, 0, 255) / 256.0;

	for (k = 0; k < gth_histogram_get_nchannels (hist); k++) {
		/* First and last points in partition */
		part[k][0]   = 0;
		part[k][256] = 256;

		/* Find intermediate points */
		j   = 0;
		sum = (gth_histogram_get_value (hist, k + 1, 0) +
		       gth_histogram_get_value (hist, k + 1, 1));

		for (i = 1; i < 256; i++) {
			desired = i * pixels_per_value;

			while (sum <= desired) {
				j++;
				sum += gth_histogram_get_value (hist, k + 1, j + 1);
			}

			/* Nearest sum */
			dif = sum - gth_histogram_get_value (hist, k + 1, j);

			if ((sum - desired) > (dif / 2.0))
				part[k][i] = j;
			else
				part[k][i] = j + 1;
		}
	}
}


static void
equalize_before (GthAsyncTask *task,
	         gpointer      user_data)
{
	gth_task_progress (GTH_TASK (task), _("Equalizing image histogram"), NULL, TRUE, 0.0);
}


static guchar
equalize_func (guchar   u_value,
	       int    **part,
	       int      channel)
{
	guchar i = 0;
	while (part[channel][i + 1] <= u_value)
		i++;
	return i;
}


static gpointer
equalize_exec (GthAsyncTask *task,
	       gpointer      user_data)
{
	EqualizeData    *equalize_data = user_data;
	int              i;
	cairo_format_t   format;
	int              width;
	int              height;
	int              source_stride;
	int              destination_stride;
	unsigned char   *p_source_line;
	unsigned char   *p_destination_line;
	unsigned char   *p_source;
	unsigned char   *p_destination;
	gboolean         cancelled;
	double           progress;
	gboolean         terminated;
	int              x, y;
	unsigned char    red, green, blue, alpha;

	/* initialize the extra data */

	equalize_data->histogram = gth_histogram_new ();
	gth_histogram_calculate_for_image (equalize_data->histogram, equalize_data->source);

	equalize_data->part = g_new0 (int *, GTH_HISTOGRAM_N_CHANNELS);
	for (i = 0; i < GTH_HISTOGRAM_N_CHANNELS; i++)
		equalize_data->part[i] = g_new0 (int, 257);
	equalize_histogram_setup (equalize_data->histogram, equalize_data->part);

	/* convert the image */

	format = cairo_image_surface_get_format (equalize_data->source);
	width = cairo_image_surface_get_width (equalize_data->source);
	height = cairo_image_surface_get_height (equalize_data->source);
	source_stride = cairo_image_surface_get_stride (equalize_data->source);

	equalize_data->destination = cairo_image_surface_create (format, width, height);
	destination_stride = cairo_image_surface_get_stride (equalize_data->destination);
	p_source_line = cairo_image_surface_get_data (equalize_data->source);
	p_destination_line = cairo_image_surface_get_data (equalize_data->destination);
	for (y = 0; y < height; y++) {
		gth_async_task_get_data (task, NULL, &cancelled, NULL);
		if (cancelled)
			return NULL;

		progress = (double) y / height;
		gth_async_task_set_data (task, NULL, NULL, &progress);

		p_source = p_source_line;
		p_destination = p_destination_line;
		for (x = 0; x < width; x++) {
			CAIRO_GET_RGBA (p_source, red, green, blue, alpha);
			red   = equalize_func (red, equalize_data->part, RED_PIX);
			green = equalize_func (green, equalize_data->part, GREEN_PIX);
			blue  = equalize_func (blue, equalize_data->part, BLUE_PIX);
			if (alpha != 0xff)
				alpha = equalize_func (alpha, equalize_data->part, ALPHA_PIX);
			CAIRO_SET_RGBA (p_destination, red, green, blue, alpha);

			p_source += 4;
			p_destination += 4;
		}
		p_source_line += source_stride;
		p_destination_line += destination_stride;
	}

	terminated = TRUE;
	gth_async_task_set_data (task, &terminated, NULL, NULL);

	return NULL;
}


static void
equalize_after (GthAsyncTask *task,
	        GError       *error,
	        gpointer      user_data)
{
	EqualizeData *equalize_data = user_data;
	int           i;

	if (error == NULL)
		gth_image_viewer_page_set_image (GTH_IMAGE_VIEWER_PAGE (equalize_data->viewer_page), equalize_data->destination, TRUE);

	for (i = 0; i < GTH_HISTOGRAM_N_CHANNELS; i++)
		g_free (equalize_data->part[i]);
	g_free (equalize_data->part);
	equalize_data->part = NULL;

	g_object_unref (equalize_data->histogram);
	equalize_data->histogram = NULL;
}


static void
equalize_destroy_data (gpointer user_data)
{
	EqualizeData *equalize_data = user_data;

	g_object_unref (equalize_data->viewer_page);
	cairo_surface_destroy (equalize_data->destination);
	cairo_surface_destroy (equalize_data->source);
	g_free (equalize_data);
}


static void
gth_file_tool_equalize_activate (GthFileTool *base)
{
	GtkWidget       *window;
	GtkWidget       *viewer_page;
	GtkWidget       *viewer;
	cairo_surface_t *image;
	EqualizeData    *equalize_data;
	GthTask         *task;

	window = gth_file_tool_get_window (base);
	viewer_page = gth_browser_get_viewer_page (GTH_BROWSER (window));
	if (! GTH_IS_IMAGE_VIEWER_PAGE (viewer_page))
		return;

	viewer = gth_image_viewer_page_get_image_viewer (GTH_IMAGE_VIEWER_PAGE (viewer_page));
	image = gth_image_viewer_get_current_image (GTH_IMAGE_VIEWER (viewer));
	if (image == NULL)
		return;

	equalize_data = g_new0 (EqualizeData, 1);
	equalize_data->viewer_page = g_object_ref (viewer_page);
	equalize_data->source = cairo_surface_reference (image);
	task = gth_async_task_new (equalize_before,
				   equalize_exec,
				   equalize_after,
				   equalize_data,
				   equalize_destroy_data);
	gth_browser_exec_task (GTH_BROWSER (window), task, FALSE);

	g_object_unref (task);
}


static void
gth_file_tool_equalize_update_sensitivity (GthFileTool *base)
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
gth_file_tool_equalize_instance_init (GthFileToolEqualize *self)
{
	gth_file_tool_construct (GTH_FILE_TOOL (self), "histogram", _("Equalize"), NULL, FALSE);
	gtk_widget_set_tooltip_text (GTK_WIDGET (self), _("Equalize image histogram"));
}


static void
gth_file_tool_equalize_class_init (GthFileToolClass *klass)
{
	klass->update_sensitivity = gth_file_tool_equalize_update_sensitivity;
	klass->activate = gth_file_tool_equalize_activate;
}


GType
gth_file_tool_equalize_get_type (void) {
	static GType type_id = 0;
	if (type_id == 0) {
		static const GTypeInfo g_define_type_info = {
			sizeof (GthFileToolEqualizeClass),
			(GBaseInitFunc) NULL,
			(GBaseFinalizeFunc) NULL,
			(GClassInitFunc) gth_file_tool_equalize_class_init,
			(GClassFinalizeFunc) NULL,
			NULL,
			sizeof (GthFileToolEqualize),
			0,
			(GInstanceInitFunc) gth_file_tool_equalize_instance_init,
			NULL
		};
		type_id = g_type_register_static (GTH_TYPE_FILE_TOOL, "GthFileToolEqualize", &g_define_type_info, 0);
	}
	return type_id;
}
