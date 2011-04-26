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
#include "gth-file-tool-enhance.h"


typedef struct {
	double gamma[GTH_HISTOGRAM_N_CHANNELS];
	double low_input[GTH_HISTOGRAM_N_CHANNELS];
	double high_input[GTH_HISTOGRAM_N_CHANNELS];
	double low_output[GTH_HISTOGRAM_N_CHANNELS];
	double high_output[GTH_HISTOGRAM_N_CHANNELS];
} Levels;


typedef struct {
	GtkWidget       *viewer_page;
	cairo_surface_t *source;
	cairo_surface_t *destination;
	GthHistogram    *histogram;
	Levels          *levels;
} EnhanceData;


static void
levels_channel_auto (Levels       *levels,
		     GthHistogram *hist,
		     int           channel)
{
	int    i;
	double count, new_count, percentage, next_percentage;

	g_return_if_fail (levels != NULL);
	g_return_if_fail (hist != NULL);

	levels->gamma[channel]       = 1.0;
	levels->low_output[channel]  = 0;
	levels->high_output[channel] = 255;

	count = gth_histogram_get_count (hist, 0, 255);

	if (count == 0.0) {
		levels->low_input[channel]  = 0;
		levels->high_input[channel] = 0;
	}
	else {
		/*  Set the low input  */

		new_count = 0.0;
		for (i = 0; i < 255; i++) {
			double value;
			double next_value;

			value = gth_histogram_get_value (hist, channel, i);
			next_value = gth_histogram_get_value (hist, channel, i + 1);

			new_count += value;
			percentage = new_count / count;
			next_percentage = (new_count + next_value) / count;

			if (fabs (percentage - 0.006) < fabs (next_percentage - 0.006)) {
				levels->low_input[channel] = i + 1;
				break;
			}
		}

		/*  Set the high input  */

		new_count = 0.0;
		for (i = 255; i > 0; i--) {
			double value;
			double next_value;

			value = gth_histogram_get_value (hist, channel, i);
			next_value = gth_histogram_get_value (hist, channel, i - 1);

			new_count += value;
			percentage = new_count / count;
			next_percentage = (new_count + next_value) / count;

			if (fabs (percentage - 0.006) < fabs (next_percentage - 0.006))	{
				levels->high_input[channel] = i - 1;
				break;
			}
		}
	}
}


static void
enhance_before (GthAsyncTask *task,
	        gpointer      user_data)
{
	gth_task_progress (GTH_TASK (task), _("White balance correction"), NULL, TRUE, 0.0);
}


static guchar
levels_func (guchar  value,
	     Levels *levels,
	     int     channel)
{
	double inten;
	int    j;

	inten = value;

	/* For color  images this runs through the loop with j = channel + 1
	 * the first time and j = 0 the second time
	 *
	 * For bw images this runs through the loop with j = 0 the first and
	 *  only time
	 */
	for (j = channel + 1; j >= 0; j -= (channel + 1)) {
		inten /= 255.0;

		/*  determine input intensity  */

		if (levels->high_input[j] != levels->low_input[j])
			inten = (255.0 * inten - levels->low_input[j]) /
				(levels->high_input[j] - levels->low_input[j]);
		else
			inten = 255.0 * inten - levels->low_input[j];

		if (levels->gamma[j] != 0.0) {
			if (inten >= 0.0)
				inten =  pow ( inten, (1.0 / levels->gamma[j]));
			else
				inten = -pow (-inten, (1.0 / levels->gamma[j]));
		}

		/*  determine the output intensity  */

		if (levels->high_output[j] >= levels->low_output[j])
			inten = inten * (levels->high_output[j] - levels->low_output[j]) +
				levels->low_output[j];
		else if (levels->high_output[j] < levels->low_output[j])
			inten = levels->low_output[j] - inten *
				(levels->low_output[j] - levels->high_output[j]);
	}

	if (inten < 0.0)
		inten = 0.0;
	else if (inten > 255.0)
		inten = 255.0;

	return (guchar) inten;
}


static gpointer
enhance_exec (GthAsyncTask *task,
	      gpointer      user_data)
{
	EnhanceData     *enhance_data = user_data;
	int              channel;
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

	/* initialize some extra data */

	enhance_data->histogram = gth_histogram_new ();
	gth_histogram_calculate_for_image (enhance_data->histogram, enhance_data->source);

	enhance_data->levels = g_new0 (Levels, 1);
	for (channel = 0; channel < GTH_HISTOGRAM_N_CHANNELS; channel++) {
		enhance_data->levels->gamma[channel]       = 1.0;
		enhance_data->levels->low_input[channel]   = 0;
		enhance_data->levels->high_input[channel]  = 255;
		enhance_data->levels->low_output[channel]  = 0;
		enhance_data->levels->high_output[channel] = 255;
	}

	for (channel = 1; channel < GTH_HISTOGRAM_N_CHANNELS - 1; channel++)
		levels_channel_auto (enhance_data->levels, enhance_data->histogram, channel);

	/* convert the image */

	format = cairo_image_surface_get_format (enhance_data->source);
	width = cairo_image_surface_get_width (enhance_data->source);
	height = cairo_image_surface_get_height (enhance_data->source);
	source_stride = cairo_image_surface_get_stride (enhance_data->source);

	enhance_data->destination = cairo_image_surface_create (format, width, height);
	cairo_surface_flush (enhance_data->destination);
	destination_stride = cairo_image_surface_get_stride (enhance_data->destination);
	p_source_line = cairo_image_surface_get_data (enhance_data->source);
	p_destination_line = cairo_image_surface_get_data (enhance_data->destination);
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
			red   = levels_func (red, enhance_data->levels, RED_PIX);
			green = levels_func (green, enhance_data->levels, GREEN_PIX);
			blue  = levels_func (blue, enhance_data->levels, BLUE_PIX);
			CAIRO_SET_RGBA (p_destination, red, green, blue, alpha);

			p_source += 4;
			p_destination += 4;
		}
		p_source_line += source_stride;
		p_destination_line += destination_stride;
	}

	cairo_surface_mark_dirty (enhance_data->destination);
	terminated = TRUE;
	gth_async_task_set_data (task, &terminated, NULL, NULL);

	return NULL;
}


static void
enhance_after (GthAsyncTask *task,
	       GError       *error,
	       gpointer      user_data)
{
	EnhanceData *enhance_data = user_data;

	if (error == NULL)
		gth_image_viewer_page_set_image (GTH_IMAGE_VIEWER_PAGE (enhance_data->viewer_page), enhance_data->destination, TRUE);

	g_object_unref (enhance_data->histogram);
	enhance_data->histogram = NULL;

	g_free (enhance_data->levels);
	enhance_data->levels = NULL;
}


static void
enhance_data_free (gpointer user_data)
{
	EnhanceData *enhance_data = user_data;

	g_object_unref (enhance_data->viewer_page);
	cairo_surface_destroy (enhance_data->destination);
	cairo_surface_destroy (enhance_data->source);
	g_free (enhance_data);
}


static void
gth_file_tool_enhance_activate (GthFileTool *base)
{
	GtkWidget       *window;
	GtkWidget       *viewer_page;
	GtkWidget       *viewer;
	cairo_surface_t *image;
	EnhanceData     *enhance_data;
	GthTask         *task;

	window = gth_file_tool_get_window (base);
	viewer_page = gth_browser_get_viewer_page (GTH_BROWSER (window));
	if (! GTH_IS_IMAGE_VIEWER_PAGE (viewer_page))
		return;

	viewer = gth_image_viewer_page_get_image_viewer (GTH_IMAGE_VIEWER_PAGE (viewer_page));
	image = gth_image_viewer_get_current_image (GTH_IMAGE_VIEWER (viewer));
	if (image == NULL)
		return;

	enhance_data = g_new0 (EnhanceData, 1);
	enhance_data->viewer_page = g_object_ref (viewer_page);
	enhance_data->source = cairo_surface_reference (image);
	enhance_data->histogram = NULL;
	enhance_data->levels = NULL;
	task = gth_async_task_new (enhance_before,
				   enhance_exec,
				   enhance_after,
				   enhance_data,
				   enhance_data_free);
	gth_browser_exec_task (GTH_BROWSER (window), task, FALSE);

	g_object_unref (task);
}


static void
gth_file_tool_enhance_update_sensitivity (GthFileTool *base)
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
gth_file_tool_enhance_instance_init (GthFileToolEnhance *self)
{
	gth_file_tool_construct (GTH_FILE_TOOL (self), "tool-enhance", _("Enhance Colors"), NULL, TRUE);
	gtk_widget_set_tooltip_text (GTK_WIDGET (self), _("Automatic white balance correction"));
}


static void
gth_file_tool_enhance_class_init (GthFileToolClass *klass)
{
	klass->update_sensitivity = gth_file_tool_enhance_update_sensitivity;
	klass->activate = gth_file_tool_enhance_activate;
}


GType
gth_file_tool_enhance_get_type (void) {
	static GType type_id = 0;
	if (type_id == 0) {
		static const GTypeInfo g_define_type_info = {
			sizeof (GthFileToolEnhanceClass),
			(GBaseInitFunc) NULL,
			(GBaseFinalizeFunc) NULL,
			(GClassInitFunc) gth_file_tool_enhance_class_init,
			(GClassFinalizeFunc) NULL,
			NULL,
			sizeof (GthFileToolEnhance),
			0,
			(GInstanceInitFunc) gth_file_tool_enhance_instance_init,
			NULL
		};
		type_id = g_type_register_static (GTH_TYPE_FILE_TOOL, "GthFileToolEnhance", &g_define_type_info, 0);
	}
	return type_id;
}
