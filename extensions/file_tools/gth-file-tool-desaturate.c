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
#include "gth-file-tool-desaturate.h"


typedef struct {
	GtkWidget       *viewer_page;
	cairo_surface_t *source;
	cairo_surface_t *destination;
} DesaturateData;


static void
gth_file_tool_desaturate_update_sensitivity (GthFileTool *base)
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
desaturate_data_free (gpointer user_data)
{
	DesaturateData *desaturate_data = user_data;

	cairo_surface_destroy (desaturate_data->destination);
	cairo_surface_destroy (desaturate_data->source);
	g_free (desaturate_data);
}


static void
desaturate_init (GthAsyncTask *task,
	         gpointer      user_data)
{
	gth_task_progress (GTH_TASK (task), _("Applying changes"), NULL, TRUE, 0.0);
}


static gpointer
desaturate_exec (GthAsyncTask *task,
	         gpointer      user_data)
{
	DesaturateData    *desaturate_data = user_data;
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
	unsigned char    min, max, lightness;

	format = cairo_image_surface_get_format (desaturate_data->source);
	width = cairo_image_surface_get_width (desaturate_data->source);
	height = cairo_image_surface_get_height (desaturate_data->source);
	source_stride = cairo_image_surface_get_stride (desaturate_data->source);

	desaturate_data->destination = cairo_image_surface_create (format, width, height);
	destination_stride = cairo_image_surface_get_stride (desaturate_data->destination);
	p_source_line = cairo_image_surface_get_data (desaturate_data->source);
	p_destination_line = cairo_image_surface_get_data (desaturate_data->destination);
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

			max = MAX (MAX (red, green), blue);
			min = MIN (MIN (red, green), blue);
			lightness = (max + min) / 2;

			CAIRO_SET_RGBA (p_destination,
					lightness,
					lightness,
					lightness,
					alpha);

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
desaturate_after (GthAsyncTask *task,
		  GError       *error,
		  gpointer      user_data)
{
	DesaturateData *desaturate_data = user_data;

	if (error == NULL)
		gth_image_viewer_page_set_image (GTH_IMAGE_VIEWER_PAGE (desaturate_data->viewer_page),
					         desaturate_data->destination,
					         TRUE);
}


static void
gth_file_tool_desaturate_activate (GthFileTool *base)
{
	GtkWidget       *window;
	GtkWidget       *viewer_page;
	GtkWidget       *viewer;
	cairo_surface_t *image;
	DesaturateData  *desaturate_data;
	GthTask         *task;

	window = gth_file_tool_get_window (base);
	viewer_page = gth_browser_get_viewer_page (GTH_BROWSER (window));
	if (! GTH_IS_IMAGE_VIEWER_PAGE (viewer_page))
		return;

	viewer = gth_image_viewer_page_get_image_viewer (GTH_IMAGE_VIEWER_PAGE (viewer_page));
	image = gth_image_viewer_get_current_image (GTH_IMAGE_VIEWER (viewer));
	if (image == NULL)
		return;

	desaturate_data = g_new0 (DesaturateData, 1);
	desaturate_data->viewer_page = viewer_page;
	desaturate_data->source = cairo_surface_reference (image);
	task = gth_async_task_new (desaturate_init,
				   desaturate_exec,
				   desaturate_after,
				   desaturate_data,
				   desaturate_data_free);
	gth_browser_exec_task (GTH_BROWSER (window), task, FALSE);

	g_object_unref (task);
}


static void
gth_file_tool_desaturate_instance_init (GthFileToolDesaturate *self)
{
	gth_file_tool_construct (GTH_FILE_TOOL (self), "tool-desaturate", _("Desaturate"), _("Desaturate"), FALSE);
}


static void
gth_file_tool_desaturate_class_init (GthFileToolClass *klass)
{
	klass->update_sensitivity = gth_file_tool_desaturate_update_sensitivity;
	klass->activate = gth_file_tool_desaturate_activate;
}


GType
gth_file_tool_desaturate_get_type (void) {
	static GType type_id = 0;
	if (type_id == 0) {
		static const GTypeInfo g_define_type_info = {
			sizeof (GthFileToolDesaturateClass),
			(GBaseInitFunc) NULL,
			(GBaseFinalizeFunc) NULL,
			(GClassInitFunc) gth_file_tool_desaturate_class_init,
			(GClassFinalizeFunc) NULL,
			NULL,
			sizeof (GthFileToolDesaturate),
			0,
			(GInstanceInitFunc) gth_file_tool_desaturate_instance_init,
			NULL
		};
		type_id = g_type_register_static (GTH_TYPE_FILE_TOOL, "GthFileToolDesaturate", &g_define_type_info, 0);
	}
	return type_id;
}
