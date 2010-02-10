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
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Street #330, Boston, MA 02111-1307, USA.
 */

#include <config.h>
#include <gthumb.h>
#include <extensions/image_viewer/gth-image-viewer-page.h>
#include "gth-file-tool-negative.h"


static void
gth_file_tool_negative_update_sensitivity (GthFileTool *base)
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
negative_step (GthPixbufTask *pixop)
{
	pixop->dest_pixel[RED_PIX]   = 255 - pixop->dest_pixel[RED_PIX];
	pixop->dest_pixel[GREEN_PIX] = 255 - pixop->dest_pixel[GREEN_PIX];
	pixop->dest_pixel[BLUE_PIX]  = 255 - pixop->dest_pixel[BLUE_PIX];

	if (pixop->has_alpha)
		pixop->dest_pixel[ALPHA_PIX] = pixop->src_pixel[ALPHA_PIX];
}


static void
negative_release (GthPixbufTask *pixop,
		    GError        *error)
{
	if (error == NULL)
		gth_image_viewer_page_set_pixbuf (GTH_IMAGE_VIEWER_PAGE (pixop->data), pixop->dest, TRUE);
}


static void
gth_file_tool_negative_activate (GthFileTool *base)
{
	GtkWidget *window;
	GtkWidget *viewer_page;
	GtkWidget *viewer;
	GdkPixbuf *src_pixbuf;
	GthTask   *task;

	window = gth_file_tool_get_window (base);
	viewer_page = gth_browser_get_viewer_page (GTH_BROWSER (window));
	if (! GTH_IS_IMAGE_VIEWER_PAGE (viewer_page))
		return;

	viewer = gth_image_viewer_page_get_image_viewer (GTH_IMAGE_VIEWER_PAGE (viewer_page));
	src_pixbuf = gth_image_viewer_get_current_pixbuf (GTH_IMAGE_VIEWER (viewer));
	if (src_pixbuf == NULL)
		return;

	task = gth_pixbuf_task_new (_("Applying changes"),
				    FALSE,
				    copy_source_to_destination,
				    negative_step,
				    negative_release,
				    viewer_page,
				    NULL);
	gth_pixbuf_task_set_source (GTH_PIXBUF_TASK (task), src_pixbuf);
	gth_browser_exec_task (GTH_BROWSER (window), task, FALSE);

	g_object_unref (task);
}


static void
gth_file_tool_negative_instance_init (GthFileToolNegative *self)
{
	gth_file_tool_construct (GTH_FILE_TOOL (self), "tool-invert", _("Negative"), _("Negative"), FALSE);
}


static void
gth_file_tool_negative_class_init (GthFileToolClass *klass)
{
	klass->update_sensitivity = gth_file_tool_negative_update_sensitivity;
	klass->activate = gth_file_tool_negative_activate;
}


GType
gth_file_tool_negative_get_type (void) {
	static GType type_id = 0;
	if (type_id == 0) {
		static const GTypeInfo g_define_type_info = {
			sizeof (GthFileToolNegativeClass),
			(GBaseInitFunc) NULL,
			(GBaseFinalizeFunc) NULL,
			(GClassInitFunc) gth_file_tool_negative_class_init,
			(GClassFinalizeFunc) NULL,
			NULL,
			sizeof (GthFileToolNegative),
			0,
			(GInstanceInitFunc) gth_file_tool_negative_instance_init,
			NULL
		};
		type_id = g_type_register_static (GTH_TYPE_FILE_TOOL, "GthFileToolNegative", &g_define_type_info, 0);
	}
	return type_id;
}
