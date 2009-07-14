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
#include <gth-catalog.h>
#include "gth-file-source-catalogs.h"
#include "actions.h"

#define BROWSER_DATA_KEY "catalogs-browser-data"


static const char *fixed_ui_info =
"<ui>"
"  <popup name='FileListPopup'>"
"    <placeholder name='Folder_Actions2'>"
"      <menuitem action='Edit_AddToCatalog'/>"
"    </placeholder>"
"  </popup>"
"  <popup name='FilePopup'>"
"    <placeholder name='Folder_Actions2'>"
"      <menuitem action='Edit_AddToCatalog'/>"
"    </placeholder>"
"  </popup>"
"</ui>";


static const char *vfs_ui_info =
"<ui>"
"  <popup name='FileListPopup'>"
"    <placeholder name='Folder_Actions2'>"
"      <menuitem action='Edit_RemoveFromCatalog'/>"
"    </placeholder>"
"  </popup>"
"  <popup name='FilePopup'>"
"    <placeholder name='Folder_Actions2'>"
"      <menuitem action='Edit_RemoveFromCatalog'/>"
"    </placeholder>"
"  </popup>"
"</ui>";



static const gchar *folder_popup_ui_info =
"<ui>"
"  <popup name='FolderListPopup'>"
"    <placeholder name='SourceCommands'>"
"      <menuitem action='Catalog_New'/>"
"      <menuitem action='Catalog_New_Library'/>"
"      <separator/>"
"      <menuitem action='Catalog_Remove'/>"
"      <menuitem action='Catalog_Rename'/>"
"    </placeholder>"
"  </popup>"
"</ui>";


static GtkActionEntry catalog_action_entries[] = {
	{ "Edit_AddToCatalog", GTK_STOCK_ADD,
	  N_("_Add to Catalog..."), NULL,
	  N_("Add selected images to a catalog"),
	  G_CALLBACK (gth_browser_activate_action_edit_add_to_catalog) },

	{ "Edit_RemoveFromCatalog", GTK_STOCK_REMOVE,
	  N_("Remo_ve from Catalog"), NULL,
	  N_("Remove selected images from the catalog"),
	  G_CALLBACK (gth_browser_activate_action_edit_remove_from_catalog) },

	{ "Catalog_New", NULL,
	  N_("Create _Catalog"), NULL,
	  NULL,
	  G_CALLBACK (gth_browser_activate_action_catalog_new) },

	{ "Catalog_New_Library", NULL,
	  N_("Create _Library"), NULL,
	  NULL,
	  G_CALLBACK (gth_browser_activate_action_catalog_new_library) },

	{ "Catalog_Remove", GTK_STOCK_REMOVE,
	  NULL, NULL,
	  NULL,
	  G_CALLBACK (gth_browser_activate_action_catalog_remove) },

	{ "Catalog_Rename", NULL,
	  N_("Rena_me"), NULL,
	  NULL,
	  G_CALLBACK (gth_browser_activate_action_catalog_rename) }
};
static guint catalog_action_entries_size = G_N_ELEMENTS (catalog_action_entries);


typedef struct {
	GtkActionGroup *actions;
	guint           folder_popup_merge_id;
	guint           vfs_merge_id;
} BrowserData;


static void
browser_data_free (BrowserData *data)
{
	g_free (data);
}


void
catalogs__gth_browser_construct_cb (GthBrowser *browser)
{
	BrowserData *data;
	GError      *error = NULL;

	g_return_if_fail (GTH_IS_BROWSER (browser));

	data = g_new0 (BrowserData, 1);

	data->actions = gtk_action_group_new ("Catalog Actions");
	gtk_action_group_set_translation_domain (data->actions, NULL);
	gtk_action_group_add_actions (data->actions,
				      catalog_action_entries,
				      catalog_action_entries_size,
				      browser);
	gtk_ui_manager_insert_action_group (gth_browser_get_ui_manager (browser), data->actions, 0);

	if (! gtk_ui_manager_add_ui_from_string (gth_browser_get_ui_manager (browser), fixed_ui_info, -1, &error)) {
		g_message ("building menus failed: %s", error->message);
		g_error_free (error);
	}

	g_object_set_data_full (G_OBJECT (browser), BROWSER_DATA_KEY, data, (GDestroyNotify) browser_data_free);
}


void
catalogs__gth_browser_update_sensitivity_cb (GthBrowser *browser)
{
	BrowserData *data;
	GtkAction   *action;
	int          n_selected;
	gboolean     sensitive;

	data = g_object_get_data (G_OBJECT (browser), BROWSER_DATA_KEY);
	g_return_if_fail (data != NULL);

	n_selected = gth_file_selection_get_n_selected (GTH_FILE_SELECTION (gth_browser_get_file_list_view (browser)));

	action = gtk_action_group_get_action (data->actions, "Edit_AddToCatalog");
	sensitive = n_selected > 0;
	g_object_set (action, "sensitive", sensitive, NULL);

	action = gtk_action_group_get_action (data->actions, "Edit_RemoveFromCatalog");
	sensitive = (n_selected > 0) && GTH_IS_FILE_SOURCE_CATALOGS (gth_browser_get_location_source (browser));
	g_object_set (action, "sensitive", sensitive, NULL);
}


void
catalogs__gth_browser_folder_tree_popup_before_cb (GthBrowser    *browser,
						   GthFileSource *file_source,
					           GthFileData   *folder)
{
	BrowserData *data;

	data = g_object_get_data (G_OBJECT (browser), BROWSER_DATA_KEY);
	g_return_if_fail (data != NULL);

	if (GTH_IS_FILE_SOURCE_CATALOGS (file_source)) {
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

		action = gtk_action_group_get_action (data->actions, "Catalog_Remove");
		sensitive = (folder != NULL) && g_file_info_get_attribute_boolean (folder->info, G_FILE_ATTRIBUTE_ACCESS_CAN_DELETE);
		g_object_set (action, "sensitive", sensitive, NULL);

		action = gtk_action_group_get_action (data->actions, "Catalog_Rename");
		sensitive = (folder != NULL) && g_file_info_get_attribute_boolean (folder->info, G_FILE_ATTRIBUTE_ACCESS_CAN_RENAME);
		g_object_set (action, "sensitive", sensitive, NULL);
	}
	else {
		if (data->folder_popup_merge_id != 0) {
			gtk_ui_manager_remove_ui (gth_browser_get_ui_manager (browser), data->folder_popup_merge_id);
			data->folder_popup_merge_id = 0;
		}
	}
}


GthCatalog *
catalogs__gth_catalog_load_from_data_cb (const void *buffer)
{
	if ((buffer == NULL)
	    || (strcmp (buffer, "") == 0)
	    || (strncmp (buffer, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<catalog ", 48) == 0))
	{
		return gth_catalog_new ();
	}
	else
		return NULL;
}


void
catalogs__gth_browser_load_location_after_cb (GthBrowser   *browser,
					      GFile        *location,
					      const GError *error)
{
	BrowserData *data;

	if (location == NULL)
		return;

	data = g_object_get_data (G_OBJECT (browser), BROWSER_DATA_KEY);

	if (GTH_IS_FILE_SOURCE_CATALOGS (gth_browser_get_location_source (browser))) {
		if (data->vfs_merge_id == 0) {
			GError *error = NULL;

			data->vfs_merge_id = gtk_ui_manager_add_ui_from_string (gth_browser_get_ui_manager (browser), vfs_ui_info, -1, &error);
			if (data->vfs_merge_id == 0) {
				g_message ("building menus failed: %s", error->message);
				g_error_free (error);
			}
		}
	}
	else {
		if (data->vfs_merge_id != 0) {
			gtk_ui_manager_remove_ui (gth_browser_get_ui_manager (browser), data->vfs_merge_id);
			data->vfs_merge_id = 0;
		}
	}
}
