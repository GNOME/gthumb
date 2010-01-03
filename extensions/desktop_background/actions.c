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
#include <gthumb.h>


void
gth_browser_activate_action_tool_desktop_background (GtkAction  *action,
					             GthBrowser *browser)
{
	GList       *items;
	GList       *file_list;
	GthFileData *file_data;
	char        *path;
	GError      *error = NULL;

	items = gth_file_selection_get_selected (GTH_FILE_SELECTION (gth_browser_get_file_list_view (browser)));
	file_list = gth_file_list_get_files (GTH_FILE_LIST (gth_browser_get_file_list (browser)), items);

	if (file_list == NULL)
		return;

	file_data = file_list->data;
	if (file_data == NULL)
		return;

	path = g_file_get_path (file_data->file);
	if (path != NULL)
		eel_gconf_set_string ("/desktop/gnome/background/picture_filename", path);

	if (! g_spawn_command_line_async ("gnome-appearance-properties --show-page=background", &error))
		_gtk_error_dialog_from_gerror_run (GTK_WINDOW (browser), _("Could not show the desktop background properties"), &error);

	g_free (path);
	_g_object_list_unref (file_list);
	_gtk_tree_path_list_free (items);
}
