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
	double gamma[5];
	double low_input[5];
	double high_input[5];
	double low_output[5];
	double high_output[5];
} Levels;


typedef struct {
	GtkWidget    *viewer_page;
	GthHistogram *hist;
	Levels       *levels;
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
adjust_levels_init (GthPixbufTask *pixop)
{
	EnhanceData *data = pixop->data;
	int          channel;

	copy_source_to_destination (pixop);
	data->hist = gth_histogram_new ();
	gth_histogram_calculate (data->hist, pixop->src);

	data->levels = g_new0 (Levels, 1);

	for (channel = 0; channel < GTH_HISTOGRAM_N_CHANNELS + 1; channel++) {
		data->levels->gamma[channel]       = 1.0;
		data->levels->low_input[channel]   = 0;
		data->levels->high_input[channel]  = 255;
		data->levels->low_output[channel]  = 0;
		data->levels->high_output[channel] = 255;
	}

	for (channel = 1; channel < GTH_HISTOGRAM_N_CHANNELS; channel++)
		levels_channel_auto (data->levels, data->hist, channel);
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


static void
adjust_levels_step (GthPixbufTask *pixop)
{
	EnhanceData *data = pixop->data;

	pixop->dest_pixel[RED_PIX]   = levels_func (pixop->src_pixel[RED_PIX], data->levels, RED_PIX);
	pixop->dest_pixel[GREEN_PIX] = levels_func (pixop->src_pixel[GREEN_PIX], data->levels, GREEN_PIX);
	pixop->dest_pixel[BLUE_PIX]  = levels_func (pixop->src_pixel[BLUE_PIX], data->levels, BLUE_PIX);

	if (pixop->has_alpha)
		pixop->dest_pixel[ALPHA_PIX] = pixop->src_pixel[ALPHA_PIX];
}


static void
adjust_levels_release (GthPixbufTask *pixop,
		       GError        *error)
{
	EnhanceData *data = pixop->data;

	if (error == NULL)
		gth_image_viewer_page_set_pixbuf (GTH_IMAGE_VIEWER_PAGE (data->viewer_page), pixop->dest, TRUE);

	g_object_unref (data->hist);
	data->hist = NULL;
	g_free (data->levels);
	data->levels = NULL;
}


static void
adjust_levels_destroy_data (gpointer user_data)
{
	EnhanceData *data = user_data;

	g_object_unref (data->viewer_page);
	g_free (data);
}


static void
gth_file_tool_enhance_activate (GthFileTool *base)
{
	GtkWidget   *window;
	GtkWidget   *viewer_page;
	GtkWidget   *viewer;
	GdkPixbuf   *src_pixbuf;
	EnhanceData *data;
	GthTask     *task;

	window = gth_file_tool_get_window (base);
	viewer_page = gth_browser_get_viewer_page (GTH_BROWSER (window));
	if (! GTH_IS_IMAGE_VIEWER_PAGE (viewer_page))
		return;

	viewer = gth_image_viewer_page_get_image_viewer (GTH_IMAGE_VIEWER_PAGE (viewer_page));
	src_pixbuf = gth_image_viewer_get_current_pixbuf (GTH_IMAGE_VIEWER (viewer));
	if (src_pixbuf == NULL)
		return;

	data = g_new0 (EnhanceData, 1);
	data->viewer_page = g_object_ref (viewer_page);
	task = gth_pixbuf_task_new (_("White balance correction"),
				    FALSE,
				    adjust_levels_init,
				    adjust_levels_step,
				    adjust_levels_release,
				    data,
				    adjust_levels_destroy_data);
	gth_pixbuf_task_set_source (GTH_PIXBUF_TASK (task), src_pixbuf);
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
