/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2005-2009 Free Software Foundation, Inc.
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

#ifndef GTH_BROWSER_ACTION_ENTRIES_H
#define GTH_BROWSER_ACTION_ENTRIES_H

#include <config.h>
#include <glib/gi18n.h>
#include "gth-stock.h"

static GtkActionEntry gth_browser_action_entries[] = {
	{ "FileMenu", NULL, N_("_File") },
	{ "EditMenu", NULL, N_("_Edit") },
	{ "ViewMenu", NULL, N_("_View") },
	{ "GoMenu", NULL, N_("_Go") },
	{ "BookmarksMenu", NULL, N_("_Bookmarks") },
	{ "HelpMenu", NULL, N_("_Help") },

	{ "File_NewWindow", "window-new",
	  N_("New _Window"), "<control>N",
	  N_("Open another window"),
	  G_CALLBACK (gth_browser_activate_action_file_new_window) },

	{ "File_OpenWith", GTK_STOCK_OPEN,
	  N_("_Open With..."), "",
	  N_("Open selected images with an application"),
	  G_CALLBACK (gth_browser_activate_action_file_open_with) },

	{ "File_Save", GTK_STOCK_SAVE,
	  NULL, "<control>s",
	  NULL,
	  G_CALLBACK (gth_browser_activate_action_file_save) },

	{ "File_SaveAs", GTK_STOCK_SAVE_AS,
	  NULL, NULL,
	  NULL,
	  G_CALLBACK (gth_browser_activate_action_file_save_as) },

	{ "File_Revert", GTK_STOCK_REVERT_TO_SAVED,
	  NULL, "F4",
	  NULL,
	  G_CALLBACK (gth_browser_activate_action_file_revert) },

	{ "Folder_Open", GTK_STOCK_OPEN,
	  N_("Open"), "",
	  NULL,
	  G_CALLBACK (gth_browser_activate_action_folder_open) },

	{ "Folder_OpenInNewWindow", NULL,
	  N_("Open in New Window"), "",
	  NULL,
	  G_CALLBACK (gth_browser_activate_action_folder_open_in_new_window) },

	{ "Edit_Preferences", GTK_STOCK_PREFERENCES,
	  NULL, NULL,
	  N_("Edit various preferences"),
	  G_CALLBACK (gth_browser_activate_action_edit_preferences) },

	{ "Edit_SelectAll", GTK_STOCK_SELECT_ALL,
	  NULL, NULL,
	  NULL,
	  G_CALLBACK (gth_browser_activate_action_edit_select_all) },

	{ "Edit_Metadata", GTK_STOCK_EDIT,
	  N_("Metadata"), "C",
	  N_("Edit file metadata"),
	  G_CALLBACK (gth_browser_activate_action_edit_metadata) },

	{ "View_Sort_By", NULL,
	  N_("_Sort By..."), NULL,
	  NULL,
	  G_CALLBACK (gth_browser_activate_action_view_sort_by) },

	{ "View_Filters", NULL,
	  N_("_Filter..."), NULL,
	  NULL,
	  G_CALLBACK (gth_browser_activate_action_view_filter) },

	{ "View_Stop", GTK_STOCK_STOP,
	  NULL, "Escape",
	  NULL,
	  G_CALLBACK (gth_browser_activate_action_view_stop) },

	{ "View_Reload", GTK_STOCK_REFRESH,
	  NULL, "<ctrl>R",
	  NULL,
	  G_CALLBACK (gth_browser_activate_action_view_reload) },

	{ "View_Prev", GTK_STOCK_GO_UP,
	  N_("Previous"), NULL,
	  N_("View previous image"),
	  G_CALLBACK (gth_browser_activate_action_view_prev) },

	{ "View_Next", GTK_STOCK_GO_DOWN,
	  N_("Next"), NULL,
	  N_("View next image"),
	  G_CALLBACK (gth_browser_activate_action_view_next) },

	{ "Go_Back", GTK_STOCK_GO_BACK,
	  NULL, "<alt>Left",
	  N_("Go to the previous visited location"),
	  G_CALLBACK (gth_browser_activate_action_go_back) },

	{ "Go_Forward", GTK_STOCK_GO_FORWARD,
	  NULL, "<alt>Right",
	  N_("Go to the next visited location"),
	  G_CALLBACK (gth_browser_activate_action_go_forward) },

	{ "Go_Up", GTK_STOCK_GO_UP,
	  NULL, "<alt>Up",
	  N_("Go up one level"),
	  G_CALLBACK (gth_browser_activate_action_go_up) },

	{ "Go_Home", NULL,
	  NULL, "h",
	  NULL,
	  G_CALLBACK (gth_browser_activate_action_go_home) },

	{ "Go_Clear_History", GTK_STOCK_CLEAR,
	  N_("_Delete History"), NULL,
	  N_("Delete the list of visited locations"),
	  G_CALLBACK (gth_browser_activate_action_go_clear_history) },

	{ "Bookmarks_Add", GTK_STOCK_ADD,
	  N_("_Add Bookmark"), "<control>D",
	  N_("Add current location to bookmarks"),
	  G_CALLBACK (gth_browser_activate_action_bookmarks_add) },

	{ "Bookmarks_Edit", NULL,
	  N_("_Edit Bookmarks..."), "<control>B",
	  N_("Edit bookmarks"),
	  G_CALLBACK (gth_browser_activate_action_bookmarks_edit) },

	{ "View_BrowserMode", GTH_STOCK_BROWSER_MODE,
	  N_("Browser"), NULL,
	  N_("View the folders"),
	  G_CALLBACK (gth_browser_activate_action_browser_mode) },

	{ "Help_About", GTK_STOCK_ABOUT,
	  NULL, NULL,
	  N_("Show information about gthumb"),
	  G_CALLBACK (gth_browser_activate_action_help_about) },

	{ "Help_Help", GTK_STOCK_HELP,
	  N_("Contents"), "F1",
	  N_("Display the gthumb Manual"),
	  G_CALLBACK (gth_browser_activate_action_help_help) },

	{ "Help_Shortcuts", NULL,
	  N_("_Keyboard Shortcuts"), NULL,
	  " ",
	  G_CALLBACK (gth_browser_activate_action_help_shortcuts) },
};
static guint gth_browser_action_entries_size = G_N_ELEMENTS (gth_browser_action_entries);


static GtkToggleActionEntry gth_browser_action_toggle_entries[] = {
	{ "View_Toolbar", NULL,
	  N_("_Toolbar"), NULL,
	  N_("View or hide the toolbar of this window"),
	  G_CALLBACK (gth_browser_activate_action_view_toolbar),
	  TRUE },
	{ "View_Statusbar", NULL,
	  N_("_Statusbar"), NULL,
	  N_("View or hide the statusbar of this window"),
	  G_CALLBACK (gth_browser_activate_action_view_statusbar),
	  TRUE },
	{ "View_Filterbar", NULL,
	  N_("_Filterbar"), NULL,
	  N_("View or hide the filterbar of this window"),
	  G_CALLBACK (gth_browser_activate_action_view_filterbar),
	  TRUE },
	{ "View_Thumbnails", NULL,
	  N_("_Thumbnails"), "<control>T",
	  N_("View thumbnails"),
	  G_CALLBACK (gth_browser_activate_action_view_thumbnails),
	  TRUE },
	{ "View_ShowHiddenFiles", NULL,
	  N_("Show _Hidden Files"), "<control>H",
	  N_("Show hidden files and folders"),
	  G_CALLBACK (gth_browser_activate_action_view_show_hidden_files),
	  FALSE },
	{ "Viewer_Properties", GTK_STOCK_PROPERTIES,
	  NULL, NULL,
	  N_("View file properties"),
	  G_CALLBACK (gth_browser_activate_action_viewer_properties),
	  FALSE },
	{ "Viewer_Tools", GTK_STOCK_EDIT,
	  NULL, NULL,
	  N_("View file tools"),
	  G_CALLBACK (gth_browser_activate_action_viewer_tools),
	  FALSE },
};
static guint gth_browser_action_toggle_entries_size = G_N_ELEMENTS (gth_browser_action_toggle_entries);

#endif /* GTH_BROWSER_ACTION_ENTRIES_H */
