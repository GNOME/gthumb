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


#define BROWSER_DATA_KEY "edit-metadata-data"


static const char *fixed_ui_info =
"<ui>"
"  <menubar name='MenuBar'>"
"    <menu name='Edit' action='EditMenu'>"
"      <placeholder name='Edit_Actions'>"
"        <menuitem action='Edit_Metadata'/>"
"      </placeholder>"
"    </menu>"
"  </menubar>"
"  <toolbar name='ViewerToolBar'>"
"    <placeholder name='Edit_Actions'>"
"      <toolitem action='Edit_Metadata'/>"
"    </placeholder>"
"  </toolbar>"
"  <toolbar name='Fullscreen_ToolBar'>"
"    <placeholder name='Edit_Actions'>"
"      <toolitem action='Edit_Metadata'/>"
"    </placeholder>"
"  </toolbar>"
"  <popup name='FileListPopup'>"
"    <placeholder name='File_LastActions'>"
"      <menuitem action='Edit_Metadata'/>"
"    </placeholder>"
"  </popup>"
"  <popup name='FilePopup'>"
"    <placeholder name='File_LastActions'>"
"      <menuitem action='Edit_Metadata'/>"
"    </placeholder>"
"  </popup>"
"</ui>";


static const char *browser_ui_info =
"<ui>"
"  <toolbar name='ToolBar'>"
"    <placeholder name='Edit_Actions'>"
"      <toolitem action='Edit_Metadata'/>"
"    </placeholder>"
"  </toolbar>"
"</ui>";


static const char *viewer_ui_info =
"<ui>"
"  <menubar name='MenuBar'>"
"    <menu name='Edit' action='EditMenu'>"
"      <placeholder name='Edit_Actions'>"
"        <menuitem action='Edit_Metadata'/>"
"      </placeholder>"
"    </menu>"
"  </menubar>"
"</ui>";


static GtkActionEntry edit_metadata_action_entries[] = {
	{ "Edit_Metadata", GTK_STOCK_EDIT,
	  N_("Comment"), "<control>M",
	  N_("Edit the comment an other information of the selected files"),
	  G_CALLBACK (gth_browser_activate_action_edit_metadata) }
};


typedef struct {
	GthBrowser     *browser;
	GtkActionGroup *actions;
	guint           browser_ui_merge_id;
	guint           viewer_ui_merge_id;
} BrowserData;


static void
browser_data_free (BrowserData *data)
{
	g_free (data);
}


void
edit_metadata__gth_browser_construct_cb (GthBrowser *browser)
{
	BrowserData *data;
	GError      *error = NULL;

	g_return_if_fail (GTH_IS_BROWSER (browser));

	data = g_new0 (BrowserData, 1);
	data->browser = browser;

	data->actions = gtk_action_group_new ("Edit Metadata Actions");
	gtk_action_group_set_translation_domain (data->actions, NULL);
	gtk_action_group_add_actions (data->actions,
				      edit_metadata_action_entries,
				      G_N_ELEMENTS (edit_metadata_action_entries),
				      browser);
	gtk_ui_manager_insert_action_group (gth_browser_get_ui_manager (browser), data->actions, 0);

	if (! gtk_ui_manager_add_ui_from_string (gth_browser_get_ui_manager (browser), fixed_ui_info, -1, &error)) {
		g_message ("building menus failed: %s", error->message);
		g_error_free (error);
	}

	gtk_tool_item_set_is_important (GTK_TOOL_ITEM (gtk_ui_manager_get_widget (gth_browser_get_ui_manager (browser), "/Fullscreen_ToolBar/Edit_Actions/Edit_Metadata")), TRUE);

	g_object_set_data_full (G_OBJECT (browser), BROWSER_DATA_KEY, data, (GDestroyNotify) browser_data_free);
}


void
edit_metadata__gth_browser_set_current_page_cb (GthBrowser *browser)
{
	BrowserData *data;
	GError      *error = NULL;

	data = g_object_get_data (G_OBJECT (browser), BROWSER_DATA_KEY);
	g_return_if_fail (data != NULL);

	switch (gth_window_get_current_page (GTH_WINDOW (browser))) {
	case GTH_BROWSER_PAGE_BROWSER:
		if (data->viewer_ui_merge_id != 0) {
			gtk_ui_manager_remove_ui (gth_browser_get_ui_manager (browser), data->viewer_ui_merge_id);
			data->viewer_ui_merge_id = 0;
		}
		if (data->browser_ui_merge_id != 0)
			return;
		data->browser_ui_merge_id = gtk_ui_manager_add_ui_from_string (gth_browser_get_ui_manager (browser), browser_ui_info, -1, &error);
		if (data->browser_ui_merge_id == 0) {
			g_warning ("ui building failed: %s", error->message);
			g_clear_error (&error);
		}
		gtk_tool_item_set_is_important (GTK_TOOL_ITEM (gtk_ui_manager_get_widget (gth_browser_get_ui_manager (browser), "/ToolBar/Edit_Actions/Edit_Metadata")), TRUE);
		break;

	case GTH_BROWSER_PAGE_VIEWER:
		if (data->browser_ui_merge_id != 0) {
			gtk_ui_manager_remove_ui (gth_browser_get_ui_manager (browser), data->browser_ui_merge_id);
			data->browser_ui_merge_id = 0;
		}
		if (data->viewer_ui_merge_id != 0)
			return;
		data->viewer_ui_merge_id = gtk_ui_manager_add_ui_from_string (gth_browser_get_ui_manager (browser), viewer_ui_info, -1, &error);
		if (data->viewer_ui_merge_id == 0) {
			g_warning ("ui building failed: %s", error->message);
			g_clear_error (&error);
		}
		gtk_tool_item_set_is_important (GTK_TOOL_ITEM (gtk_ui_manager_get_widget (gth_browser_get_ui_manager (browser), "/ViewerToolBar/Edit_Actions/Edit_Metadata")), TRUE);

		break;

	default:
		break;
	}
}


void
edit_metadata__gth_browser_update_sensitivity_cb (GthBrowser *browser)
{
	BrowserData *data;
	GtkAction   *action;
	int          n_selected;
	gboolean     sensitive;

	data = g_object_get_data (G_OBJECT (browser), BROWSER_DATA_KEY);
	g_return_if_fail (data != NULL);

	n_selected = gth_file_selection_get_n_selected (GTH_FILE_SELECTION (gth_browser_get_file_list_view (browser)));

	action = gtk_action_group_get_action (data->actions, "Edit_Metadata");
	sensitive = (n_selected > 0);
	g_object_set (action, "sensitive", sensitive, NULL);
}
