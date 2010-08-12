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
#include <gthumb.h>
#include "gth-file-tool-save.h"


static void
gth_file_tool_save_update_sensitivity (GthFileTool *base)
{
	GtkWidget *window;

	window = gth_file_tool_get_window (base);
	gtk_widget_set_sensitive (GTK_WIDGET (base), gth_browser_get_file_modified (GTH_BROWSER (window)));
}


static void
gth_file_tool_save_activate (GthFileTool *tool)
{
	GtkWidget *window;
	GtkWidget *viewer_page;

	window = gth_file_tool_get_window (tool);
	viewer_page = gth_browser_get_viewer_page (GTH_BROWSER (window));
	gth_viewer_page_save (GTH_VIEWER_PAGE (viewer_page), NULL, NULL, NULL);
}


static void
gth_file_tool_save_instance_init (GthFileToolSave *self)
{
	gth_file_tool_construct (GTH_FILE_TOOL (self), GTK_STOCK_SAVE, _("Save"), _("Save"), FALSE);
}


static void
gth_file_tool_save_class_init (GthFileToolClass *klass)
{
	klass->update_sensitivity = gth_file_tool_save_update_sensitivity;
	klass->activate = gth_file_tool_save_activate;
}


GType
gth_file_tool_save_get_type (void) {
	static GType type_id = 0;
	if (type_id == 0) {
		static const GTypeInfo g_define_type_info = {
			sizeof (GthFileToolSaveClass),
			(GBaseInitFunc) NULL,
			(GBaseFinalizeFunc) NULL,
			(GClassInitFunc) gth_file_tool_save_class_init,
			(GClassFinalizeFunc) NULL,
			NULL,
			sizeof (GthFileToolSave),
			0,
			(GInstanceInitFunc) gth_file_tool_save_instance_init,
			NULL
		};
		type_id = g_type_register_static (GTH_TYPE_FILE_TOOL, "GthFileToolSave", &g_define_type_info, 0);
	}
	return type_id;
}
