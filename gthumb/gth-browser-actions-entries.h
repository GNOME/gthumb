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
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef GTH_BROWSER_ACTION_ENTRIES_H
#define GTH_BROWSER_ACTION_ENTRIES_H

#include <config.h>
#include <glib/gi18n.h>
#include "gth-stock.h"
#include "gtk-utils.h"

static const GActionEntry gth_browser_actions[] = {
	{ "browser-mode", gth_browser_activate_browser_mode },
	{ "browser-edit-file", gth_browser_activate_browser_edit_file },
	{ "browser-properties", toggle_action_activated, NULL, "false", gth_browser_activate_browser_properties },
	{ "clear-history", gth_browser_activate_clear_history },
	{ "close", gth_browser_activate_close },
	{ "fullscreen", gth_browser_activate_fullscreen },
	{ "go-back", gth_browser_activate_go_back },
	{ "go-forward", gth_browser_activate_go_forward },
	{ "go-home", gth_browser_activate_go_home },
	{ "go-to-history-position", gth_browser_activate_go_to_history_pos, "s", "''", NULL },
	{ "go-to-location", gth_browser_activate_go_to_location, "s", "''", NULL },
	{ "go-up", gth_browser_activate_go_up },
	{ "open-location", gth_browser_activate_open_location },
	{ "revert-to-saved", gth_browser_activate_revert_to_saved },
	{ "save", gth_browser_activate_save },
	{ "save-as", gth_browser_activate_save_as },
	{ "viewer-edit-file", toggle_action_activated, NULL, "false", gth_browser_activate_viewer_edit_file },
	{ "viewer-properties", toggle_action_activated, NULL, "false", gth_browser_activate_viewer_properties },
	{ "unfullscreen", gth_browser_activate_unfullscreen },
};


static const GthAccelerator gth_browser_accelerators[] = {
	{ "browser-mode", "Escape" },
	{ "fullscreen", "F11" },
	{ "browser-properties", "<Ctrl>i" },
};


static GthActionEntryExt gth_browser_action_entries[] = {
	{ "EditMenu", NULL, N_("_Edit") },
	{ "ViewMenu", NULL, N_("_View") },
	{ "OpenWithMenu", NULL, N_("Open _With") },

	{ "Folder_Open", GTK_STOCK_OPEN,
	  N_("Open"), "",
	  NULL,
	  GTH_ACTION_FLAG_NONE,
	  G_CALLBACK (gth_browser_activate_action_folder_open) },

	{ "Folder_OpenInNewWindow", NULL,
	  N_("Open in New Window"), "",
	  NULL,
	  GTH_ACTION_FLAG_NONE,
	  G_CALLBACK (gth_browser_activate_action_folder_open_in_new_window) },

	{ "Edit_SelectAll", GTK_STOCK_SELECT_ALL,
	  NULL, "<control>A",
	  NULL,
	  GTH_ACTION_FLAG_NONE,
	  G_CALLBACK (gth_browser_activate_action_edit_select_all) },

	{ "View_Sort_By", NULL,
	  N_("_Sort By..."), NULL,
	  NULL,
	  GTH_ACTION_FLAG_NONE,
	  G_CALLBACK (gth_browser_activate_action_view_sort_by) },

	{ "View_Filters", NULL,
	  N_("_Filter..."), NULL,
	  NULL,
	  GTH_ACTION_FLAG_NONE,
	  G_CALLBACK (gth_browser_activate_action_view_filter) },

	{ "View_Stop", GTK_STOCK_STOP,
	  NULL, NULL,
	  N_("Stop loading the current location"),
	  GTH_ACTION_FLAG_NONE,
	  G_CALLBACK (gth_browser_activate_action_view_stop) },

	{ "View_Reload", GTK_STOCK_REFRESH,
	  NULL, "<control>R",
	  N_("Reload the current location"),
	  GTH_ACTION_FLAG_NONE,
	  G_CALLBACK (gth_browser_activate_action_view_reload) },
};


static GtkToggleActionEntry gth_browser_action_toggle_entries[] = {
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
	{ "View_Sidebar", NULL,
	  N_("_Sidebar"), "F9",
	  N_("View or hide the sidebar of this window"),
	  G_CALLBACK (gth_browser_activate_action_view_sidebar),
	  TRUE },
	{ "View_Thumbnail_List", NULL,
	  N_("_Thumbnail Pane"), "F8",
	  N_("View or hide the thumbnail pane in viewer mode"),
	  G_CALLBACK (gth_browser_activate_action_view_thumbnail_list),
	  TRUE },
	{ "View_Thumbnails", NULL,
	  N_("_Thumbnails"), "<control>T",
	  N_("View thumbnails"),
	  G_CALLBACK (gth_browser_activate_action_view_thumbnails),
	  TRUE },
	{ "View_ShowHiddenFiles", NULL,
	  N_("_Hidden Files"), "<control>H",
	  N_("Show hidden files and folders"),
	  G_CALLBACK (gth_browser_activate_action_view_show_hidden_files),
	  FALSE },
	{ "View_ShrinkWrap", NULL,
	  N_("_Fit Window to Image"), "<control>e",
	  N_("Resize the window to the size of the image"),
	  G_CALLBACK (gth_browser_activate_action_view_shrink_wrap),
	  FALSE },
};

#endif /* GTH_BROWSER_ACTION_ENTRIES_H */
