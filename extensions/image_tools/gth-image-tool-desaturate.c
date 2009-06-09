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
#include "gth-image-tool-desaturate.h"


static void
gth_image_tool_desaturate_real_update_sensitivity (GthFileTool *base,
						   GtkWidget   *window)
{
	GthImageToolDesaturate *self;

	self = (GthImageToolDesaturate*) base;
}


static void
gth_file_tool_interface_init (GthFileToolIface *iface)
{
	iface->update_sensitivity = gth_image_tool_desaturate_real_update_sensitivity;
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
desaturate_release (GthPixbufTask *pixop)
{
	gth_image_viewer_page_set_pixbuf (GTH_IMAGE_VIEWER_PAGE (pixop->data), pixop->dest);
}


static void
button_clicked_cb (GtkButton *button,
		   gpointer   data)
{
	GtkWidget *window;
	GtkWidget *viewer_page;
	GtkWidget *viewer;
	GdkPixbuf *src_pixbuf;
	GdkPixbuf *dest_pixbuf;
	GthTask   *task;

	window = gtk_widget_get_toplevel (GTK_WIDGET (button));
	viewer_page = gth_browser_get_viewer_page (GTH_BROWSER (window));
	if (! GTH_IS_IMAGE_VIEWER_PAGE (viewer_page))
		return;

	viewer = gth_image_viewer_page_get_image_viewer (GTH_IMAGE_VIEWER_PAGE (viewer_page));
	src_pixbuf = gth_image_viewer_get_current_pixbuf (GTH_IMAGE_VIEWER (viewer));
	if (src_pixbuf == NULL)
		return;

	dest_pixbuf = gdk_pixbuf_copy (src_pixbuf);
	task = gth_pixbuf_task_new (src_pixbuf,
				    dest_pixbuf,
				    NULL,
				    desaturate_step,
				    desaturate_release,
				    viewer_page);
	gth_browser_exec_task (GTH_BROWSER (window), task);

	g_object_unref (task);
	g_object_unref (dest_pixbuf);
}


static void
gth_image_tool_desaturate_instance_init (GthImageToolDesaturate *self)
{
	GtkWidget *hbox;
	GtkWidget *icon;
	GtkWidget *label;

	gtk_button_set_relief (GTK_BUTTON (self), GTK_RELIEF_NONE);

	hbox = gtk_hbox_new (FALSE, 6);

	icon = gtk_image_new_from_stock (GTK_STOCK_EDIT, GTK_ICON_SIZE_MENU);
	gtk_widget_show (icon);
	gtk_box_pack_start (GTK_BOX (hbox), icon, FALSE, FALSE, 0);

	label = gtk_label_new (_("Desaturate"));
	gtk_widget_show (label);
	gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);

	gtk_widget_show (hbox);
	gtk_container_add (GTK_CONTAINER (self), hbox);

	g_signal_connect (self, "clicked", G_CALLBACK (button_clicked_cb), self);
}


GType
gth_image_tool_desaturate_get_type (void) {
	static GType type_id = 0;
	if (type_id == 0) {
		static const GTypeInfo g_define_type_info = {
			sizeof (GthImageToolDesaturateClass),
			(GBaseInitFunc) NULL,
			(GBaseFinalizeFunc) NULL,
			(GClassInitFunc) NULL,
			(GClassFinalizeFunc) NULL,
			NULL,
			sizeof (GthImageToolDesaturate),
			0,
			(GInstanceInitFunc) gth_image_tool_desaturate_instance_init,
			NULL
		};
		static const GInterfaceInfo gth_file_tool_info = {
			(GInterfaceInitFunc) gth_file_tool_interface_init,
			(GInterfaceFinalizeFunc) NULL,
			NULL
		};
		type_id = g_type_register_static (GTK_TYPE_BUTTON, "GthImageToolDesaturate", &g_define_type_info, 0);
		g_type_add_interface_static (type_id, GTH_TYPE_FILE_TOOL, &gth_file_tool_info);
	}
	return type_id;
}
