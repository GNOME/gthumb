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
#include "gth-file-tool-desaturate.h"


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
desaturate_step (GthPixbufTask *pixop)
{
	guchar min, max, lightness;

	max = MAX (pixop->src_pixel[RED_PIX], pixop->src_pixel[GREEN_PIX]);
	max = MAX (max, pixop->src_pixel[BLUE_PIX]);
	min = MIN (pixop->src_pixel[RED_PIX], pixop->src_pixel[GREEN_PIX]);
	min = MIN (min, pixop->src_pixel[BLUE_PIX]);
	lightness = (max + min) / 2;

	pixop->dest_pixel[RED_PIX]   = lightness;
	pixop->dest_pixel[GREEN_PIX] = lightness;
	pixop->dest_pixel[BLUE_PIX]  = lightness;

	if (pixop->has_alpha)
		pixop->dest_pixel[ALPHA_PIX] = pixop->src_pixel[ALPHA_PIX];
}


static void
desaturate_release (GthPixbufTask *pixop,
		    GError        *error)
{
	if (error == NULL)
		gth_image_viewer_page_set_pixbuf (GTH_IMAGE_VIEWER_PAGE (pixop->data), pixop->dest, TRUE);
}


static void
gth_file_tool_desaturate_activate (GthFileTool *base)
{
	GtkWidget *window;
	GtkWidget *viewer_page;
	GtkWidget *viewer;
	GdkPixbuf *src_pixbuf;
	GdkPixbuf *dest_pixbuf;
	GthTask   *task;

	window = gth_file_tool_get_window (base);
	viewer_page = gth_browser_get_viewer_page (GTH_BROWSER (window));
	if (! GTH_IS_IMAGE_VIEWER_PAGE (viewer_page))
		return;

	viewer = gth_image_viewer_page_get_image_viewer (GTH_IMAGE_VIEWER_PAGE (viewer_page));
	src_pixbuf = gth_image_viewer_get_current_pixbuf (GTH_IMAGE_VIEWER (viewer));
	if (src_pixbuf == NULL)
		return;

	dest_pixbuf = gdk_pixbuf_copy (src_pixbuf);
	task = gth_pixbuf_task_new (_("Desaturating image"),
				    src_pixbuf,
				    dest_pixbuf,
				    NULL,
				    desaturate_step,
				    desaturate_release,
				    viewer_page);
	gth_browser_exec_task (GTH_BROWSER (window), task, FALSE);

	g_object_unref (task);
	g_object_unref (dest_pixbuf);
}


static void
gth_file_tool_desaturate_instance_init (GthFileToolDesaturate *self)
{
	gth_file_tool_construct (GTH_FILE_TOOL (self), GTK_STOCK_EDIT, _("Desaturate"), _("Desaturate"), FALSE);
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
