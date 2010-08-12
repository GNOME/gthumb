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
#include "gth-file-tool-flip.h"


static void
gth_file_tool_flip_activate (GthFileTool *base)
{
	GtkWidget *window;
	GtkWidget *viewer_page;
	GtkWidget *viewer;
	GdkPixbuf *src_pixbuf;
	GdkPixbuf *dest_pixbuf;

	window = gth_file_tool_get_window (base);
	viewer_page = gth_browser_get_viewer_page (GTH_BROWSER (window));
	if (! GTH_IS_IMAGE_VIEWER_PAGE (viewer_page))
		return;

	viewer = gth_image_viewer_page_get_image_viewer (GTH_IMAGE_VIEWER_PAGE (viewer_page));
	src_pixbuf = gth_image_viewer_get_current_pixbuf (GTH_IMAGE_VIEWER (viewer));
	if (src_pixbuf == NULL)
		return;

	dest_pixbuf = _gdk_pixbuf_transform (src_pixbuf, GTH_TRANSFORM_FLIP_V);
	gth_image_viewer_page_set_pixbuf (GTH_IMAGE_VIEWER_PAGE (viewer_page), dest_pixbuf, TRUE);

	g_object_unref (dest_pixbuf);
}


static void
gth_file_tool_flip_update_sensitivity (GthFileTool *base)
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
gth_file_tool_flip_instance_init (GthFileToolFlip *self)
{
	gth_file_tool_construct (GTH_FILE_TOOL (self), "tool-flip", _("Flip"), NULL, FALSE);
	/*gtk_widget_set_tooltip_text (GTK_WIDGET (self), _("Automatic white balance correction"));*/
}


static void
gth_file_tool_flip_class_init (GthFileToolClass *klass)
{
	klass->update_sensitivity = gth_file_tool_flip_update_sensitivity;
	klass->activate = gth_file_tool_flip_activate;
}


GType
gth_file_tool_flip_get_type (void) {
	static GType type_id = 0;
	if (type_id == 0) {
		static const GTypeInfo g_define_type_info = {
			sizeof (GthFileToolFlipClass),
			(GBaseInitFunc) NULL,
			(GBaseFinalizeFunc) NULL,
			(GClassInitFunc) gth_file_tool_flip_class_init,
			(GClassFinalizeFunc) NULL,
			NULL,
			sizeof (GthFileToolFlip),
			0,
			(GInstanceInitFunc) gth_file_tool_flip_instance_init,
			NULL
		};
		type_id = g_type_register_static (GTH_TYPE_FILE_TOOL, "GthFileToolFlip", &g_define_type_info, 0);
	}
	return type_id;
}
