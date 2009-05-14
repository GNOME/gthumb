/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2005 Free Software Foundation, Inc.
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

#ifndef GTH_VIEWER_ACTION_ENTRIES_H
#define GTH_VIEWER_ACTION_ENTRIES_H


#include <config.h>
#include <glib/gi18n.h>
#include "typedefs.h"
#include "gthumb-stock.h"
#include "gth-viewer-actions-callbacks.h"


static GtkActionEntry gth_viewer_action_entries[] = {
	{ "FileMenu", NULL, N_("_File") },
	{ "EditMenu", NULL, N_("_Edit") },
	{ "ViewMenu", NULL, N_("_View") },
	{ "ViewShowHideMenu", NULL, N_("Show/_Hide") },
	{ "ViewZoomMenu", NULL, N_("_Zoom") },
	{ "ImageMenu", NULL, N_("_Image") },
	{ "ToolsMenu", NULL, N_("_Tools") },
	{ "ToolsWallpaperMenu", NULL, N_("Set Image as _Wallpaper") },
	{ "ScriptMenu", NULL, N_("Scri_pts") },
	{ "HelpMenu", NULL, N_("_Help") },

	{ "File_NewWindow", GTK_STOCK_NEW,
	  N_("New _Window"), NULL,
	  N_("Create a new window"),
	  G_CALLBACK (gth_viewer_activate_action_file_new_window) },

	{ "File_OpenFolder", GTK_STOCK_JUMP_TO,
	  N_("_Go to the Image Folder"), "<alt>End",
	  N_("Go to the folder that contains the selected image"),
	  G_CALLBACK (gth_viewer_activate_action_file_open_folder) },

	{ "Edit_AddToCatalog", GTHUMB_STOCK_ADD_TO_CATALOG,
	  N_("_Add to Catalog..."), NULL,
	  N_("Add selected images to a catalog"),
	  G_CALLBACK (gth_viewer_activate_action_edit_add_to_catalog) },

	{ "Go_Refresh", GTK_STOCK_REFRESH,
	  N_("_Reload"), "<control>R",
	  N_("Reload the current location"),
	  G_CALLBACK (gth_viewer_activate_action_go_refresh) },

        { "External_Scripts", GTK_STOCK_EDIT,
          N_("_Edit Scripts"), NULL,
          N_("Edit external scripts"),
          G_CALLBACK (gth_viewer_activate_action_scripts) },
	  
};
static guint gth_viewer_action_entries_size = G_N_ELEMENTS (gth_viewer_action_entries);


static GtkToggleActionEntry gth_viewer_action_toggle_entries[] = {
	{ "View_Toolbar", NULL,
	  N_("_Toolbar"), NULL,
	  N_("View or hide the toolbar of this window"),
	  G_CALLBACK (gth_viewer_activate_action_view_toolbar),
	  TRUE },
	{ "View_Statusbar", NULL,
	  N_("_Statusbar"), NULL,
	  N_("View or hide the statusbar of this window"),
	  G_CALLBACK (gth_viewer_activate_action_view_statusbar),
	  TRUE },
	{ "View_ShowMetadata", GTK_STOCK_PROPERTIES,
	  NULL, NULL,
	  N_("View image properties"),
	  G_CALLBACK (gth_viewer_activate_action_view_show_info),
	  TRUE },
	{ "View_SingleWindow", NULL,
	  N_("_Single Window"), NULL,
	  N_("Reuse this window to view other images"),
	  G_CALLBACK (gth_viewer_activate_action_single_window),
	  FALSE }
};
static guint gth_viewer_action_toggle_entries_size = G_N_ELEMENTS (gth_viewer_action_toggle_entries);


#endif /* GTH_VIEWER_ACTION_ENTRIES_H */
