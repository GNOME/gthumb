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
#include "gth-file-tool-save-as.h"


static void
gth_file_tool_save_as_update_sensitivity (GthFileTool *base)
{
	GtkWidget *window;

	window = gth_file_tool_get_window (base);
	gtk_widget_set_sensitive (GTK_WIDGET (base), gth_browser_get_current_file (GTH_BROWSER (window)) != NULL);
}


static void
gth_file_tool_save_as_activate (GthFileTool *tool)
{
	GtkWidget *window;
	GtkWidget *viewer_page;

	window = gth_file_tool_get_window (tool);
	viewer_page = gth_browser_get_viewer_page (GTH_BROWSER (window));
	gth_viewer_page_save_as (GTH_VIEWER_PAGE (viewer_page), NULL, NULL);
}


static void
gth_file_tool_save_as_instance_init (GthFileToolSaveAs *self)
{
	gth_file_tool_construct (GTH_FILE_TOOL (self), GTK_STOCK_SAVE_AS, _("Save As"), NULL, FALSE);
}


static void
gth_file_tool_save_as_class_init (GthFileToolClass *klass)
{
	klass->update_sensitivity = gth_file_tool_save_as_update_sensitivity;
	klass->activate = gth_file_tool_save_as_activate;
}


GType
gth_file_tool_save_as_get_type (void) {
	static GType type_id = 0;
	if (type_id == 0) {
		static const GTypeInfo g_define_type_info = {
			sizeof (GthFileToolSaveAsClass),
			(GBaseInitFunc) NULL,
			(GBaseFinalizeFunc) NULL,
			(GClassInitFunc) gth_file_tool_save_as_class_init,
			(GClassFinalizeFunc) NULL,
			NULL,
			sizeof (GthFileToolSaveAs),
			0,
			(GInstanceInitFunc) gth_file_tool_save_as_instance_init,
			NULL
		};
		type_id = g_type_register_static (GTH_TYPE_FILE_TOOL, "GthFileToolSaveAs", &g_define_type_info, 0);
	}
	return type_id;
}
