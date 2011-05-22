/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2008 Free Software Foundation, Inc.
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
#include <gth-catalog.h>
#include "dlg-add-to-catalog.h"
#include "dlg-catalog-properties.h"
#include "gth-file-source-catalogs.h"


void
gth_browser_activate_action_edit_add_to_catalog (GtkAction  *action,
						 GthBrowser *browser)
{
	GList *items;
	GList *file_list = NULL;
	GList *files;

	items = gth_file_selection_get_selected (GTH_FILE_SELECTION (gth_browser_get_file_list_view (browser)));
	file_list = gth_file_list_get_files (GTH_FILE_LIST (gth_browser_get_file_list (browser)), items);
	files = gth_file_data_list_to_file_list (file_list);
	dlg_add_to_catalog (browser, files);

	_g_object_list_unref (files);
	_g_object_list_unref (file_list);
	_gtk_tree_path_list_free (items);
}


void
gth_browser_activate_action_edit_remove_from_catalog (GtkAction  *action,
						      GthBrowser *browser)
{
	GList *items;
	GList *file_data_list;

	items = gth_file_selection_get_selected (GTH_FILE_SELECTION (gth_browser_get_file_list_view (browser)));
	file_data_list = gth_file_list_get_files (GTH_FILE_LIST (gth_browser_get_file_list (browser)), items);

	gth_catalog_manager_remove_files (GTK_WINDOW (browser),
					  gth_browser_get_location_data (browser),
					  file_data_list);

	_g_object_list_unref (file_data_list);
	_gtk_tree_path_list_free (items);
}


void
gth_browser_activate_action_catalog_new (GtkAction  *action,
					 GthBrowser *browser)
{
	char          *name;
	GthFileData   *selected_parent;
	GFile         *parent;
	GthFileSource *file_source;
	GFile         *gio_parent;
	GError        *error = NULL;
	GFile         *gio_file;

	name = _gtk_request_dialog_run (GTK_WINDOW (browser),
				        GTK_DIALOG_MODAL,
				        _("Enter the catalog name: "),
				        "",
				        1024,
				        GTK_STOCK_CANCEL,
				        _("C_reate"));
	if (name == NULL)
		return;

	selected_parent = gth_browser_get_folder_popup_file_data (browser);
	if (selected_parent != NULL) {
		GthFileSource *file_source;
		GFileInfo     *info;

		file_source = gth_main_get_file_source (selected_parent->file);
		info = gth_file_source_get_file_info (file_source, selected_parent->file, GFILE_BASIC_ATTRIBUTES);
		if (g_file_info_get_attribute_boolean (info, "gthumb::no-child"))
			parent = g_file_get_parent (selected_parent->file);
		else
			parent = g_file_dup (selected_parent->file);

		g_object_unref (info);
		g_object_unref (file_source);
	}
	else
		parent = g_file_new_for_uri ("catalog:///");

	file_source = gth_main_get_file_source (parent);
	gio_parent = gth_file_source_to_gio_file (file_source, parent);
	gio_file = _g_file_create_unique (gio_parent, name, ".catalog", &error);
	if (gio_file != NULL) {
		GFile *file;
		GList *list;

		file = gth_catalog_file_from_gio_file (gio_file, NULL);
		list = g_list_prepend (NULL, file);
		gth_monitor_folder_changed (gth_main_get_default_monitor (),
					    parent,
					    list,
					    GTH_MONITOR_EVENT_CREATED);

		g_list_free (list);
		g_object_unref (file);
	}
	else
		_gtk_error_dialog_from_gerror_show (GTK_WINDOW (browser), _("Could not create the catalog"), &error);

	g_object_unref (gio_file);
	g_object_unref (gio_parent);
	g_object_unref (file_source);
}


void
gth_browser_activate_action_catalog_new_library (GtkAction  *action,
						 GthBrowser *browser)
{
	char          *name;
	GthFileData   *selected_parent;
	GFile         *parent;
	GthFileSource *file_source;
	GFile         *gio_parent;
	GError        *error = NULL;
	GFile         *gio_file;

	name = _gtk_request_dialog_run (GTK_WINDOW (browser),
				        GTK_DIALOG_MODAL,
				        _("Enter the library name: "),
				        "",
				        1024,
				        GTK_STOCK_CANCEL,
				        _("C_reate"));
	if (name == NULL)
		return;

	selected_parent = gth_browser_get_folder_popup_file_data (browser);
	if (selected_parent != NULL) {
		GthFileSource *file_source;
		GFileInfo     *info;

		file_source = gth_main_get_file_source (selected_parent->file);
		info = gth_file_source_get_file_info (file_source, selected_parent->file, GFILE_BASIC_ATTRIBUTES);
		if (g_file_info_get_attribute_boolean (info, "gthumb::no-child"))
			parent = g_file_get_parent (selected_parent->file);
		else
			parent = g_file_dup (selected_parent->file);

		g_object_unref (info);
		g_object_unref (file_source);
	}
	else
		parent = g_file_new_for_uri ("catalog:///");

	file_source = gth_main_get_file_source (parent);
	gio_parent = gth_file_source_to_gio_file (file_source, parent);
	gio_file = _g_directory_create_unique (gio_parent, name, "", &error);
	if (gio_file != NULL) {
		GFile *file;
		GList *list;

		file = gth_catalog_file_from_gio_file (gio_file, NULL);
		list = g_list_prepend (NULL, file);
		gth_monitor_folder_changed (gth_main_get_default_monitor (),
					    parent,
					    list,
					    GTH_MONITOR_EVENT_CREATED);

		g_list_free (list);
		g_object_unref (file);
	}
	else
		_gtk_error_dialog_from_gerror_show (GTK_WINDOW (browser), _("Could not create the library"), &error);

	g_object_unref (gio_file);
	g_object_unref (gio_parent);
	g_object_unref (file_source);
}


static void
remove_catalog (GtkWindow   *window,
		GthFileData *file_data)
{
	GFile  *gio_file;
	GError *error = NULL;

	gio_file = gth_main_get_gio_file (file_data->file);
	if (g_file_delete (gio_file, NULL, &error)) {
		GFile *parent;
		GList *files;

		parent = g_file_get_parent (file_data->file);
		files = g_list_prepend (NULL, g_object_ref (file_data->file));
		gth_monitor_folder_changed (gth_main_get_default_monitor (),
					    parent,
					    files,
					    GTH_MONITOR_EVENT_DELETED);

		_g_object_list_unref (files);
		_g_object_unref (parent);
	}
	else
		_gtk_error_dialog_from_gerror_show (window,
						    _("Could not remove the catalog"),
						    &error);

	g_object_unref (gio_file);
}


static void
remove_catalog_response_cb (GtkDialog *dialog,
			    int        response_id,
			    gpointer   user_data)
{
	GthFileData *file_data = user_data;

	if (response_id == GTK_RESPONSE_YES)
		remove_catalog (gtk_window_get_transient_for (GTK_WINDOW (dialog)), file_data);

	gtk_widget_destroy (GTK_WIDGET (dialog));
	g_object_unref (file_data);
}


void
gth_browser_activate_action_catalog_remove (GtkAction  *action,
					    GthBrowser *browser)
{
	GthFileData *file_data;

	file_data = gth_browser_get_folder_popup_file_data (browser);

	if (eel_gconf_get_boolean (PREF_MSG_CONFIRM_DELETION, DEFAULT_MSG_CONFIRM_DELETION)) {
		char      *prompt;
		GtkWidget *d;

		prompt = g_strdup_printf (_("Are you sure you want to remove \"%s\"?"), g_file_info_get_display_name (file_data->info));
		d = _gtk_message_dialog_new (GTK_WINDOW (browser),
					     GTK_DIALOG_MODAL,
					     GTK_STOCK_DIALOG_QUESTION,
					     prompt,
					     NULL,
					     GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
					     GTK_STOCK_REMOVE, GTK_RESPONSE_YES,
					     NULL);
		g_signal_connect (d, "response", G_CALLBACK (remove_catalog_response_cb), file_data);
		gtk_widget_show (d);

		g_free (prompt);
	}
	else {
		remove_catalog (GTK_WINDOW (browser), file_data);
		g_object_unref (file_data);
	}
}


void
gth_browser_activate_action_catalog_rename (GtkAction  *action,
					    GthBrowser *browser)
{
	GthFileData *file_data;

	file_data = gth_browser_get_folder_popup_file_data (browser);
	gth_folder_tree_start_editing (GTH_FOLDER_TREE (gth_browser_get_folder_tree (browser)), file_data->file);

	g_object_unref (file_data);
}


void
gth_browser_activate_action_catalog_properties (GtkAction  *action,
						GthBrowser *browser)
{
	GthFileData *file_data;

	file_data = gth_browser_get_folder_popup_file_data (browser);
	dlg_catalog_properties (browser, file_data);

	g_object_unref (file_data);
}


void
gth_browser_activate_action_go_to_container (GtkAction  *action,
					     GthBrowser *browser)
{
	GList *items;
	GList *file_list = NULL;

	items = gth_file_selection_get_selected (GTH_FILE_SELECTION (gth_browser_get_file_list_view (browser)));
	file_list = gth_file_list_get_files (GTH_FILE_LIST (gth_browser_get_file_list (browser)), items);

	if (file_list != NULL) {
		GthFileData *first_file = file_list->data;
		GFile       *parent;

		parent = g_file_get_parent (first_file->file);
		gth_browser_go_to (browser, parent, first_file->file);

		g_object_unref (parent);
	}

	_g_object_list_unref (file_list);
	_gtk_tree_path_list_free (items);
}


void
gth_browser_add_to_catalog (GthBrowser *browser,
  			    GFile      *catalog)
{
	GList *items;
	GList *file_list;
	GList *files;

	items = gth_file_selection_get_selected (GTH_FILE_SELECTION (gth_browser_get_file_list_view (browser)));
	file_list = gth_file_list_get_files (GTH_FILE_LIST (gth_browser_get_file_list (browser)), items);
	files = gth_file_data_list_to_file_list (file_list);
	if (files != NULL)
		add_to_catalog (browser, catalog, files);

	_g_object_list_unref (files);
	_g_object_list_unref (file_list);
	_gtk_tree_path_list_free (items);
}
