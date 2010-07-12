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
#include <glib/gi18n.h>
#include <glib-object.h>
#include <gdk/gdkkeysyms.h>
#include <gthumb.h>
#include "gth-file-tool-crop.h"
#include "gth-file-tool-enhance.h"
#include "gth-file-tool-flip.h"
#include "gth-file-tool-mirror.h"
#include "gth-file-tool-resize.h"
#include "gth-file-tool-rotate-left.h"
#include "gth-file-tool-rotate-right.h"


gpointer
file_tools__gth_browser_file_list_key_press_cb (GthBrowser  *browser,
						GdkEventKey *event)
{
	gpointer     result = NULL;
	GtkWidget   *sidebar;
	GtkWidget   *toolbox;
	GthFileTool *tool = NULL;

	if ((event->state & GDK_CONTROL_MASK) || (event->state & GDK_MOD1_MASK))
		return NULL;

	if (gth_window_get_current_page (GTH_WINDOW (browser)) != GTH_BROWSER_PAGE_VIEWER)
		return NULL;

	sidebar = gth_browser_get_viewer_sidebar (browser);
	toolbox = gth_sidebar_get_toolbox (GTH_SIDEBAR (sidebar));

	if (gth_toolbox_tool_is_active (GTH_TOOLBOX (toolbox)))
		return NULL;

	switch (event->keyval) {
	case GDK_h:
		tool = (GthFileTool *) gth_toolbox_get_tool (GTH_TOOLBOX (toolbox), GTH_TYPE_FILE_TOOL_ENHANCE);
		break;
	case GDK_l:
		tool = (GthFileTool *) gth_toolbox_get_tool (GTH_TOOLBOX (toolbox), GTH_TYPE_FILE_TOOL_FLIP);
		break;
	case GDK_m:
		tool = (GthFileTool *) gth_toolbox_get_tool (GTH_TOOLBOX (toolbox), GTH_TYPE_FILE_TOOL_MIRROR);
		break;
	case GDK_r:
		tool = (GthFileTool *) gth_toolbox_get_tool (GTH_TOOLBOX (toolbox), GTH_TYPE_FILE_TOOL_ROTATE_RIGHT);
		break;
	case GDK_R:
		tool = (GthFileTool *) gth_toolbox_get_tool (GTH_TOOLBOX (toolbox), GTH_TYPE_FILE_TOOL_ROTATE_LEFT);
		break;
	case GDK_C:
		gth_browser_show_viewer_tools (browser, TRUE);
		tool = (GthFileTool *) gth_toolbox_get_tool (GTH_TOOLBOX (toolbox), GTH_TYPE_FILE_TOOL_CROP);
		break;
	case GDK_S:
		gth_browser_show_viewer_tools (browser, TRUE);
		tool = (GthFileTool *) gth_toolbox_get_tool (GTH_TOOLBOX (toolbox), GTH_TYPE_FILE_TOOL_RESIZE);
		break;
	}

	if (tool != NULL) {
		gth_file_tool_activate (tool);
		result = GINT_TO_POINTER (1);
	}

	return result;
}
