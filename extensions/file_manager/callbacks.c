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
#include <gthumb.h>
#include "actions.h"


#define BROWSER_DATA_KEY "file-manager-browser-data"


static const char *vfs_ui_info =
"<ui>"
"  <menubar name='MenuBar'>"
"    <menu name='Edit' action='EditMenu'>"
"      <placeholder name='Folder_Actions'>"
"        <menuitem action='Edit_Duplicate'/>"
"        <menuitem action='Edit_Rename'/>"
"        <separator/>"
"        <menuitem action='Edit_Trash'/>"
"        <menuitem action='Edit_Delete'/>"
"      </placeholder>"
"    </menu>"
"  </menubar>"
"</ui>";


static const char *browser_ui_info =
"<ui>"
"  <menubar name='MenuBar'>"
"    <menu name='Edit' action='EditMenu'>"
"    <placeholder name='File_Actions_1'>"
"      <menuitem action='Edit_CutFiles'/>"
"      <menuitem action='Edit_CopyFiles'/>"
"      <menuitem action='Edit_PasteInFolder'/>"
"    </placeholder>"
"    </menu>"
"  </menubar>"
"  <popup name='FileListPopup'>"
"    <placeholder name='File_Actions'>"
"      <menuitem action='Edit_CutFiles'/>"
"      <menuitem action='Edit_CopyFiles'/>"
"      <menuitem action='Edit_PasteInFolder'/>"
"    </placeholder>"
"  </popup>"
"</ui>";


static const char *browser_vfs_ui_info =
"<ui>"
"  <popup name='FileListPopup'>"
"    <placeholder name='Folder_Actions'>"
"      <menuitem action='Edit_Trash'/>"
"      <menuitem action='Edit_Delete'/>"
"    </placeholder>"
"  </popup>"
"</ui>";


static const char *folder_popup_ui_info =
"<ui>"
"  <popup name='FolderListPopup'>"
"    <placeholder name='SourceCommands'>"
"      <menuitem action='File_NewFolder'/>"
"      <menuitem action='File_RenameFolder'/>"
"    </placeholder>"
"  </popup>"
"</ui>";


static GtkActionEntry action_entries[] = {
	{ "File_NewFolder", "folder-new",
	  N_("Create _Folder"), "<control><shift>N",
	  N_("Create a new empty folder inside this folder"),
	  G_CALLBACK (gth_browser_action_new_folder) },
	{ "File_RenameFolder", NULL,
	  N_("_Rename"), NULL,
	  N_("Rename this folder"),
	  G_CALLBACK (gth_browser_action_rename_folder) },
        { "Edit_CutFiles", GTK_STOCK_CUT,
	  NULL, NULL,
	  NULL,
	  G_CALLBACK (gth_browser_activate_action_edit_cut_files) },
	{ "Edit_CopyFiles", GTK_STOCK_COPY,
	  NULL, NULL,
	  NULL,
	  G_CALLBACK (gth_browser_activate_action_edit_copy_files) },
	{ "Edit_PasteInFolder", GTK_STOCK_PASTE,
	  NULL, NULL,
	  NULL,
	  G_CALLBACK (gth_browser_activate_action_edit_paste_in_folder) },
	{ "Edit_Duplicate", NULL,
	  N_("D_uplicate"), NULL,
	  N_("Duplicate the selected files"),
	  G_CALLBACK (gth_browser_activate_action_edit_duplicate) },
	{ "Edit_Rename", NULL,
	  N_("_Rename"), "F2",
	  N_("Rename the selected files"),
	  G_CALLBACK (gth_browser_activate_action_edit_rename) },
	{ "Edit_Trash", "user-trash",
	  N_("Mo_ve to Trash"), "Delete",
	  N_("Move the selected files to the Trash"),
	  G_CALLBACK (gth_browser_activate_action_edit_trash) },
	{ "Edit_Delete", "edit-delete",
	  N_("_Delete"), "<shift>Delete",
	  N_("Delete the selected files"),
	  G_CALLBACK (gth_browser_activate_action_edit_delete) },
};


typedef struct {
	GtkActionGroup *action_group;
	guint           vfs_merge_id;
	guint           browser_merge_id;
	guint           browser_vfs_merge_id;
	guint           folder_popup_merge_id;
} BrowserData;


static void
browser_data_free (BrowserData *data)
{
	g_free (data);
}


static void
set_action_sensitive (BrowserData *data,
		      const char  *action_name,
		      gboolean     sensitive)
{
	GtkAction *action;

	action = gtk_action_group_get_action (data->action_group, action_name);
	g_object_set (action, "sensitive", sensitive, NULL);
}


void
fm__gth_browser_construct_cb (GthBrowser *browser)
{
	BrowserData *data;

	g_return_if_fail (GTH_IS_BROWSER (browser));

	data = g_new0 (BrowserData, 1);

	data->action_group = gtk_action_group_new ("File Manager Actions");
	gtk_action_group_set_translation_domain (data->action_group, NULL);
	gtk_action_group_add_actions (data->action_group,
				      action_entries,
				      G_N_ELEMENTS (action_entries),
				      browser);
	gtk_ui_manager_insert_action_group (gth_browser_get_ui_manager (browser), data->action_group, 0);
	set_action_sensitive (data, "Edit_PasteInFolder", FALSE);

	g_object_set_data_full (G_OBJECT (browser), BROWSER_DATA_KEY, data, (GDestroyNotify) browser_data_free);
}


static void
file_manager_update_ui (BrowserData *data,
			GthBrowser  *browser)
{
	if (GTH_IS_FILE_SOURCE_VFS (gth_browser_get_file_source (browser))) {
		if (data->vfs_merge_id == 0) {
			GError *local_error = NULL;

			data->vfs_merge_id = gtk_ui_manager_add_ui_from_string (gth_browser_get_ui_manager (browser), vfs_ui_info, -1, &local_error);
			if (data->vfs_merge_id == 0) {
				g_warning ("building ui failed: %s", local_error->message);
				g_error_free (local_error);
			}
		}
	}
	else if (data->vfs_merge_id != 0) {
			gtk_ui_manager_remove_ui (gth_browser_get_ui_manager (browser), data->vfs_merge_id);
			data->vfs_merge_id = 0;
	}

	if (gth_window_get_current_page (GTH_WINDOW (browser)) == GTH_BROWSER_PAGE_BROWSER) {
		if (data->browser_merge_id == 0) {
			GError *local_error = NULL;

			data->browser_merge_id = gtk_ui_manager_add_ui_from_string (gth_browser_get_ui_manager (browser), browser_ui_info, -1, &local_error);
			if (data->browser_merge_id == 0) {
				g_warning ("building ui failed: %s", local_error->message);
				g_error_free (local_error);
			}
		}
	}
	else if (data->browser_merge_id != 0) {
		gtk_ui_manager_remove_ui (gth_browser_get_ui_manager (browser), data->browser_merge_id);
		data->browser_merge_id = 0;
	}

	if (GTH_IS_FILE_SOURCE_VFS (gth_browser_get_file_source (browser))
	    && (gth_window_get_current_page (GTH_WINDOW (browser)) == GTH_BROWSER_PAGE_BROWSER))
	{
		if (data->browser_vfs_merge_id == 0) {
			GError *local_error = NULL;

			data->browser_vfs_merge_id = gtk_ui_manager_add_ui_from_string (gth_browser_get_ui_manager (browser), browser_vfs_ui_info, -1, &local_error);
			if (data->browser_vfs_merge_id == 0) {
				g_warning ("building ui failed: %s", local_error->message);
				g_error_free (local_error);
			}
		}
	}
	else if (data->browser_vfs_merge_id != 0) {
		gtk_ui_manager_remove_ui (gth_browser_get_ui_manager (browser), data->browser_vfs_merge_id);
		data->browser_vfs_merge_id = 0;
	}
}


void
fm__gth_browser_set_current_page_cb (GthBrowser *browser)
{
	BrowserData *data;

	data = g_object_get_data (G_OBJECT (browser), BROWSER_DATA_KEY);
	file_manager_update_ui (data, browser);
}


void
fm__gth_browser_load_location_after_cb (GthBrowser   *browser,
					GFile        *location,
					const GError *error)
{
	BrowserData *data;

	if (location == NULL)
		return;

	data = g_object_get_data (G_OBJECT (browser), BROWSER_DATA_KEY);
	file_manager_update_ui (data, browser);
}


void
fm__gth_browser_folder_tree_popup_before_cb (GthBrowser    *browser,
					     GthFileSource *file_source,
					     GFile         *folder)
{
	BrowserData *data;

	data = g_object_get_data (G_OBJECT (browser), BROWSER_DATA_KEY);
	g_return_if_fail (data != NULL);

	if (GTH_IS_FILE_SOURCE_VFS (file_source)) {
		GtkAction *action;
		gboolean   sensitive;

		if (data->folder_popup_merge_id == 0) {
			GError *error = NULL;

			data->folder_popup_merge_id = gtk_ui_manager_add_ui_from_string (gth_browser_get_ui_manager (browser), folder_popup_ui_info, -1, &error);
			if (data->folder_popup_merge_id == 0) {
				g_message ("building menus failed: %s", error->message);
				g_error_free (error);
			}
		}

		action = gtk_action_group_get_action (data->action_group, "File_RenameFolder");
		sensitive = folder != NULL;
		g_object_set (action, "sensitive", sensitive, NULL);
	}
	else {
		if (data->folder_popup_merge_id != 0) {
			gtk_ui_manager_remove_ui (gth_browser_get_ui_manager (browser), data->folder_popup_merge_id);
			data->folder_popup_merge_id = 0;
		}
	}
}


static void
clipboard_targets_received_cb (GtkClipboard *clipboard,
			       GdkAtom      *atoms,
                               int           n_atoms,
                               gpointer      user_data)
{
	GthBrowser  *browser = user_data;
	BrowserData *data;
	gboolean     can_paste;
	int          i;

	data = g_object_get_data (G_OBJECT (browser), BROWSER_DATA_KEY);

	can_paste = FALSE;
	for (i = 0; ! can_paste && (i < n_atoms); i++)
		if (atoms[i] == GNOME_COPIED_FILES)
			can_paste = TRUE;

	set_action_sensitive (data, "Edit_PasteInFolder", can_paste);

	g_object_unref (browser);
}


static void
_gth_browser_update_paste_command_sensitivity (GthBrowser   *browser,
                                               GtkClipboard *clipboard)
{
	BrowserData *data;

	data = g_object_get_data (G_OBJECT (browser), BROWSER_DATA_KEY);
	g_return_if_fail (data != NULL);

        set_action_sensitive (data, "Edit_PasteInFolder", FALSE);

	if (clipboard == NULL)
		clipboard = gtk_widget_get_clipboard (GTK_WIDGET (browser), GDK_SELECTION_CLIPBOARD);
	gtk_clipboard_request_targets (clipboard,
				       clipboard_targets_received_cb,
				       g_object_ref (browser));
}


void
fm__gth_browser_update_sensitivity_cb (GthBrowser *browser)
{
	BrowserData *data;
	int          n_selected;
	gboolean     sensitive;

	data = g_object_get_data (G_OBJECT (browser), BROWSER_DATA_KEY);
	g_return_if_fail (data != NULL);

	n_selected = gth_file_selection_get_n_selected (GTH_FILE_SELECTION (gth_browser_get_file_list_view (browser)));

	sensitive = n_selected > 0;
	set_action_sensitive (data, "Edit_CutFiles", sensitive);
	set_action_sensitive (data, "Edit_CopyFiles", sensitive);
	set_action_sensitive (data, "Edit_Trash", sensitive);
	set_action_sensitive (data, "Edit_Delete", sensitive);
	set_action_sensitive (data, "Edit_Duplicate", sensitive);
	set_action_sensitive (data, "Edit_Rename", sensitive);

	_gth_browser_update_paste_command_sensitivity (browser, NULL);
}


static void
clipboard_owner_change_cb (GtkClipboard *clipboard,
                           GdkEvent     *event,
                           gpointer      user_data)
{
	_gth_browser_update_paste_command_sensitivity ((GthBrowser *) user_data, clipboard);
}


void
fm__gth_browser_realize_cb (GthBrowser *browser)
{
	GtkClipboard *clipboard;

	clipboard = gtk_clipboard_get (GDK_SELECTION_CLIPBOARD);
	g_signal_connect (clipboard,
	                  "owner_change",
	                  G_CALLBACK (clipboard_owner_change_cb),
	                  browser);
}


void
fm__gth_browser_unrealize_cb (GthBrowser *browser)
{
	GtkClipboard *clipboard;

	clipboard = gtk_widget_get_clipboard (GTK_WIDGET (browser), GDK_SELECTION_CLIPBOARD);
	g_signal_handlers_disconnect_by_func (clipboard,
	                                      G_CALLBACK (clipboard_owner_change_cb),
	                                      browser);
}

