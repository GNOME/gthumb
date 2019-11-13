/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2005-2019 Free Software Foundation, Inc.
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
#include "gtk-utils.h"
#include "typedefs.h"


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
	{ "go-to-location", gth_browser_activate_go_to_location, "s", NULL, NULL },
	{ "go-up", gth_browser_activate_go_up },
	{ "reload", gth_browser_activate_reload },
	{ "open-location", gth_browser_activate_open_location },
	{ "revert-to-saved", gth_browser_activate_revert_to_saved },
	{ "save", gth_browser_activate_save },
	{ "save-as", gth_browser_activate_save_as },
	{ "toggle-edit-file", gth_browser_activate_toggle_edit_file },
	{ "toggle-file-properties", gth_browser_activate_toggle_file_properties },
	{ "viewer-edit-file", toggle_action_activated, NULL, "false", gth_browser_activate_viewer_edit_file },
	{ "viewer-properties", toggle_action_activated, NULL, "false", gth_browser_activate_viewer_properties },
	{ "unfullscreen", gth_browser_activate_unfullscreen },
	{ "open-folder-in-new-window", gth_browser_activate_open_folder_in_new_window },

	{ "show-hidden-files", toggle_action_activated, NULL, "false", gth_browser_activate_show_hidden_files },
	{ "sort-by", gth_browser_activate_sort_by },

	{ "show-statusbar", toggle_action_activated, NULL, "false", gth_browser_activate_show_statusbar },
	{ "show-sidebar", toggle_action_activated, NULL, "false", gth_browser_activate_show_sidebar },
	{ "show-thumbnail-list", toggle_action_activated, NULL, "false", gth_browser_activate_show_thumbnail_list },

	{ "show-first-image", gth_browser_activate_show_first_image },
	{ "show-last-image", gth_browser_activate_show_last_image },
	{ "show-previous-image", gth_browser_activate_show_previous_image },
	{ "show-next-image", gth_browser_activate_show_next_image },

	{ "apply-editor-changes", gth_browser_activate_apply_editor_changes },
};


static const GthAccelerator gth_browser_accelerators[] = {
	{ "browser-mode", "Escape" },
};


static const GthShortcut gth_browser_shortcuts[] = {
	{ "browser-mode", N_("Show browser"), GTH_SHORTCUT_CONTEXT_INTERNAL | GTH_SHORTCUT_CONTEXT_VIEWER, GTH_SHORTCUT_CATEGORY_GENERAL, "Escape" },
	{ "close", N_("Close window"), GTH_SHORTCUT_CONTEXT_INTERNAL | GTH_SHORTCUT_CONTEXT_BROWSER_VIEWER, GTH_SHORTCUT_CATEGORY_GENERAL, "<Primary>w" },
	{ "quit", N_("Quit"), GTH_SHORTCUT_CONTEXT_INTERNAL | GTH_SHORTCUT_CONTEXT_BROWSER_VIEWER, GTH_SHORTCUT_CATEGORY_GENERAL, "<Primary>q" },
	{ "new-window", N_("New Window"), GTH_SHORTCUT_CONTEXT_INTERNAL | GTH_SHORTCUT_CONTEXT_BROWSER_VIEWER, GTH_SHORTCUT_CATEGORY_GENERAL, "<Primary>n" },

	{ "open-location", N_("Open location"), GTH_SHORTCUT_CONTEXT_BROWSER, GTH_SHORTCUT_CATEGORY_NAVIGATION, "o" },
	{ "fullscreen", N_("Fullscreen"), GTH_SHORTCUT_CONTEXT_BROWSER_VIEWER, GTH_SHORTCUT_CATEGORY_VIEWER, "f" },
	{ "revert-to-saved", N_("Revert image to saved"), GTH_SHORTCUT_CONTEXT_BROWSER, GTH_SHORTCUT_CATEGORY_FILE_MANAGER, "F4" },

	{ "go-back", N_("Load previuos location"), GTH_SHORTCUT_CONTEXT_BROWSER, GTH_SHORTCUT_CATEGORY_NAVIGATION, "<Alt>Left" },
	{ "go-forward", N_("Load next location"), GTH_SHORTCUT_CONTEXT_BROWSER, GTH_SHORTCUT_CATEGORY_NAVIGATION, "<Alt>Right" },
	{ "go-up", N_("Load parent folder"), GTH_SHORTCUT_CONTEXT_BROWSER, GTH_SHORTCUT_CATEGORY_NAVIGATION, "<Alt>Up" },
	{ "go-home", N_("Load home"), GTH_SHORTCUT_CONTEXT_BROWSER, GTH_SHORTCUT_CATEGORY_NAVIGATION, "<Alt>Home" },
	{ "reload", N_("Reload location"), GTH_SHORTCUT_CONTEXT_BROWSER, GTH_SHORTCUT_CATEGORY_NAVIGATION, "r" },
	{ "show-hidden-files", N_("Show/Hide hidden files"), GTH_SHORTCUT_CONTEXT_BROWSER, GTH_SHORTCUT_CATEGORY_NAVIGATION, "h" },
	{ "sort-by", N_("Change sorting order"), GTH_SHORTCUT_CONTEXT_BROWSER, GTH_SHORTCUT_CATEGORY_NAVIGATION, "s" },

	{ "show-previous-image", N_("Show previous file"), GTH_SHORTCUT_CONTEXT_VIEWER, GTH_SHORTCUT_CATEGORY_VIEWER, "BackSpace" },
	{ "show-next-image", N_("Show next file"), GTH_SHORTCUT_CONTEXT_VIEWER, GTH_SHORTCUT_CATEGORY_VIEWER, "space" },
	{ "show-first-image", N_("Show first file"), GTH_SHORTCUT_CONTEXT_VIEWER, GTH_SHORTCUT_CATEGORY_VIEWER, "Home" },
	{ "show-last-image", N_("Show last file"), GTH_SHORTCUT_CONTEXT_VIEWER, GTH_SHORTCUT_CATEGORY_VIEWER, "End" },

	{ "toggle-edit-file", N_("Image tools"), GTH_SHORTCUT_CONTEXT_BROWSER_VIEWER, GTH_SHORTCUT_CATEGORY_UI, "e" },
	{ "toggle-file-properties", N_("File properties"), GTH_SHORTCUT_CONTEXT_BROWSER_VIEWER, GTH_SHORTCUT_CATEGORY_UI, "i" },
};


static const GthMenuEntry gears_app_action_entries[] = {
	{ N_("_Preferences"), "app.preferences" },
	{ N_("Keyboard Shortcuts"), "app.shortcuts" },
	{ N_("_Help"), "app.help" },
	{ N_("_About gThumb"), "app.about" }
};


#endif /* GTH_BROWSER_ACTION_ENTRIES_H */
