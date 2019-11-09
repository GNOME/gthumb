/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2019 Free Software Foundation, Inc.
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
#include "actions.h"
#include "gth-file-tool-adjust-contrast.h"
#include "gth-file-tool-crop.h"
#include "gth-file-tool-flip.h"
#include "gth-file-tool-mirror.h"
#include "gth-file-tool-resize.h"
#include "gth-file-tool-rotate-left.h"
#include "gth-file-tool-rotate-right.h"


void
gth_browser_activate_file_tool (GSimpleAction *action,
				GVariant      *parameter,
				gpointer       user_data)
{
	GthBrowser    *browser = user_data;
	GtkWidget     *sidebar;
	GtkWidget     *toolbox;
	GthViewerPage *page;
	const char    *tool_name;
	GthFileTool   *tool;

	sidebar = gth_browser_get_viewer_sidebar (browser);
	toolbox = gth_sidebar_get_toolbox (GTH_SIDEBAR (sidebar));
	if (gth_toolbox_tool_is_active (GTH_TOOLBOX (toolbox)))
		return;

	page = gth_browser_get_viewer_page (browser);
	if (! GTH_IS_IMAGE_VIEWER_PAGE (page))
		return;

	tool_name = g_variant_get_string (parameter, NULL);

	tool = NULL;
	if (g_strcmp0 (tool_name, "adjust-contrast") == 0)
		tool = (GthFileTool *) gth_toolbox_get_tool (GTH_TOOLBOX (toolbox), GTH_TYPE_FILE_TOOL_ADJUST_CONTRAST);
	if (g_strcmp0 (tool_name, "flip") == 0)
		tool = (GthFileTool *) gth_toolbox_get_tool (GTH_TOOLBOX (toolbox), GTH_TYPE_FILE_TOOL_FLIP);
	if (g_strcmp0 (tool_name, "mirror") == 0)
		tool = (GthFileTool *) gth_toolbox_get_tool (GTH_TOOLBOX (toolbox), GTH_TYPE_FILE_TOOL_MIRROR);
	if (g_strcmp0 (tool_name, "rotate-right") == 0)
		tool = (GthFileTool *) gth_toolbox_get_tool (GTH_TOOLBOX (toolbox), GTH_TYPE_FILE_TOOL_ROTATE_RIGHT);
	if (g_strcmp0 (tool_name, "rotate-left") == 0)
		tool = (GthFileTool *) gth_toolbox_get_tool (GTH_TOOLBOX (toolbox), GTH_TYPE_FILE_TOOL_ROTATE_LEFT);
	if (g_strcmp0 (tool_name, "crop") == 0)
		tool = (GthFileTool *) gth_toolbox_get_tool (GTH_TOOLBOX (toolbox), GTH_TYPE_FILE_TOOL_CROP);
	if (g_strcmp0 (tool_name, "resize") == 0)
		tool = (GthFileTool *) gth_toolbox_get_tool (GTH_TOOLBOX (toolbox), GTH_TYPE_FILE_TOOL_RESIZE);

	if (tool != NULL) {
		if (gth_window_get_current_page (GTH_WINDOW (browser)) == GTH_BROWSER_PAGE_BROWSER)
			gth_window_set_current_page (GTH_WINDOW (browser), GTH_BROWSER_PAGE_VIEWER);
		gth_file_tool_activate (tool);
	}
}
