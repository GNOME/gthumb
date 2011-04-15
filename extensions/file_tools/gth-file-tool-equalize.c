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
	GtkWidget     *viewer_page;
	GthHistogram  *histogram;
	int          **part;
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
equalize_init (GthPixbufTask *pixop)
{
	EqualizeData *data = pixop->data;
	int           i;

	copy_source_to_destination (pixop);
	data->histogram = gth_histogram_new ();
	gth_histogram_calculate_for_pixbuf (data->histogram, pixop->src);

	data->part = g_new0 (int *, GTH_HISTOGRAM_N_CHANNELS);
	for (i = 0; i < GTH_HISTOGRAM_N_CHANNELS; i++)
		data->part[i] = g_new0 (int, 257);
	equalize_histogram_setup (data->histogram, data->part);
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


static void
equalize_step (GthPixbufTask *pixop)
{
	EqualizeData *data = pixop->data;

	pixop->dest_pixel[RED_PIX]   = equalize_func (pixop->src_pixel[RED_PIX], data->part, 0);
	pixop->dest_pixel[GREEN_PIX] = equalize_func (pixop->src_pixel[GREEN_PIX], data->part, 1);
	pixop->dest_pixel[BLUE_PIX]  = equalize_func (pixop->src_pixel[BLUE_PIX], data->part, 2);
	if (pixop->has_alpha)
		pixop->dest_pixel[ALPHA_PIX] = equalize_func (pixop->src_pixel[ALPHA_PIX], data->part, 3);
}


static void
equalize_release (GthPixbufTask *pixop,
		  GError        *error)
{
	EqualizeData *data = pixop->data;
	int           i;

	if (error == NULL)
		gth_image_viewer_page_set_pixbuf (GTH_IMAGE_VIEWER_PAGE (data->viewer_page), pixop->dest, TRUE);

	for (i = 0; i < GTH_HISTOGRAM_N_CHANNELS; i++)
		g_free (data->part[i]);
	g_free (data->part);
	g_object_unref (data->histogram);
}


static void
equalize_destroy_data (gpointer user_data)
{
	EqualizeData *data = user_data;

	g_object_unref (data->viewer_page);
	g_free (data);
}


static void
gth_file_tool_equalize_activate (GthFileTool *base)
{
	GtkWidget    *window;
	GtkWidget    *viewer_page;
	GtkWidget    *viewer;
	GdkPixbuf    *src_pixbuf;
	EqualizeData *data;
	GthTask      *task;

	window = gth_file_tool_get_window (base);
	viewer_page = gth_browser_get_viewer_page (GTH_BROWSER (window));
	if (! GTH_IS_IMAGE_VIEWER_PAGE (viewer_page))
		return;

	viewer = gth_image_viewer_page_get_image_viewer (GTH_IMAGE_VIEWER_PAGE (viewer_page));
	src_pixbuf = gth_image_viewer_get_current_pixbuf (GTH_IMAGE_VIEWER (viewer));
	if (src_pixbuf == NULL)
		return;

	data = g_new0 (EqualizeData, 1);
	data->viewer_page = g_object_ref (viewer_page);
	task = gth_pixbuf_task_new (_("Equalizing image histogram"),
				    FALSE,
				    equalize_init,
				    equalize_step,
				    equalize_release,
				    data,
				    equalize_destroy_data);
	gth_pixbuf_task_set_source (GTH_PIXBUF_TASK (task), src_pixbuf);
	gth_browser_exec_task (GTH_BROWSER (window), task, FALSE);

	g_object_unref (task);
	g_object_unref (src_pixbuf);
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
