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
	{ "go-to-location", gth_browser_activate_go_to_location, "s", NULL, NULL },
	{ "go-up", gth_browser_activate_go_up },
	{ "reload", gth_browser_activate_reload },
	{ "open-location", gth_browser_activate_open_location },
	{ "revert-to-saved", gth_browser_activate_revert_to_saved },
	{ "save", gth_browser_activate_save },
	{ "save-as", gth_browser_activate_save_as },
	{ "viewer-edit-file", toggle_action_activated, NULL, "false", gth_browser_activate_viewer_edit_file },
	{ "viewer-properties", toggle_action_activated, NULL, "false", gth_browser_activate_viewer_properties },
	{ "unfullscreen", gth_browser_activate_unfullscreen },
	{ "open-folder-in-new-window", gth_browser_activate_open_folder_in_new_window },

	{ "show-hidden-files", toggle_action_activated, NULL, "false", gth_browser_activate_show_hidden_files },
	{ "sort-by", gth_browser_activate_sort_by },

	{ "show-statusbar", toggle_action_activated, NULL, "false", gth_browser_activate_show_statusbar },
	{ "show-sidebar", toggle_action_activated, NULL, "false", gth_browser_activate_show_sidebar },
	{ "show-thumbnail-list", toggle_action_activated, NULL, "false", gth_browser_activate_show_thumbnail_list },

	{ "show-previous-image", gth_browser_activate_show_previous_image },
	{ "show-next-image", gth_browser_activate_show_next_image },
};


static const GthAccelerator gth_browser_accelerators[] = {
	{ "browser-mode", "Escape" },
	{ "browser-properties", "<Control>i" },
	{ "close", "<Control>w" },
	{ "fullscreen", "F11" },
	{ "revert-to-saved", "F4" },
	{ "show-sidebar", "F9" },
	{ "show-thumbnail-list", "F8" },
	{ "go-back", "<Alt>Left" },
	{ "go-forward", "<Alt>Right" },
	{ "go-up", "<Alt>Up" },
	{ "go-home", "<Alt>Home" },
	{ "reload", "<Control>r" }
};


#endif /* GTH_BROWSER_ACTION_ENTRIES_H */
