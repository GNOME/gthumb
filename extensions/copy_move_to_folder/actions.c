/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2010 Free Software Foundation, Inc.
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
#include <glib/gi18n.h>
#include <gthumb.h>
#include <extensions/file_manager/gth-copy-task.h>
#include "preferences.h"


typedef struct {
	GthBrowser *browser;
	gboolean    move;
	GFile      *destination;
	gboolean    view_destination;
} CopyToFolderData;


static void
copy_to_folder_data_free (CopyToFolderData *data)
{
	g_object_unref (data->destination);
	g_object_unref (data->browser);
	g_free (data);
}


void
copy_complete_cb (GthTask  *task,
		  GError   *error,
		  gpointer  user_data)
{
	CopyToFolderData *data = user_data;

	if ((error == NULL) && (data->view_destination))
		gth_browser_load_location (data->browser, data->destination);

	g_object_unref (task);
	copy_to_folder_data_free (data);
}


static void
copy_files_to_folder (GthBrowser *browser,
		      gboolean    move,
		      char       *destination_uri,
		      gboolean    view_destination)
{
	GthFileData      *destination_data;
	GthFileSource    *file_source;
	GList            *items;
	GList            *file_list;
	GList            *files;
	CopyToFolderData *data;
	GthTask          *task;

	destination_data = gth_file_data_new_for_uri (destination_uri, NULL);
	file_source = gth_main_get_file_source (destination_data->file);

	items = gth_file_selection_get_selected (GTH_FILE_SELECTION (gth_browser_get_file_list_view (browser)));
	file_list = gth_file_list_get_files (GTH_FILE_LIST (gth_browser_get_file_list (browser)), items);
	files = gth_file_data_list_to_file_list (file_list);

	data = g_new0 (CopyToFolderData, 1);
	data->browser = g_object_ref (browser);
	data->move = move;
	data->destination = g_file_dup (destination_data->file);
	data->view_destination = view_destination;

	task = gth_copy_task_new (file_source, destination_data, move, files);
	g_signal_connect (task,
			  "completed",
			  G_CALLBACK (copy_complete_cb),
			  data);
	gth_browser_exec_task (browser, task, FALSE);

	_g_object_list_unref (files);
	_g_object_list_unref (file_list);
	_gtk_tree_path_list_free (items);
	g_object_unref (file_source);
}


static void
copy_to_folder_dialog (GthBrowser *browser,
		       gboolean    move)
{
	GtkWidget *dialog;
	char      *start_uri;
	GtkWidget *box;
	GtkWidget *view_destination_button;

	dialog = gtk_file_chooser_dialog_new (move ? _("Move To") : _("Copy To"),
					      NULL,
					      GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER,
					      GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
					      (move ? _("Move") : _("Copy")), GTK_RESPONSE_ACCEPT,
					      NULL);

	start_uri = eel_gconf_get_string (PREF_FILE_MANAGER_COPY_LAST_FOLDER, get_home_uri ());
	gtk_file_chooser_set_current_folder_uri (GTK_FILE_CHOOSER (dialog), start_uri);
	g_free(start_uri);

	box = gtk_vbox_new (FALSE, 6);
	gtk_container_set_border_width (GTK_CONTAINER (box), 6);
	gtk_widget_show (box);

	view_destination_button = gtk_check_button_new_with_mnemonic (_("_View the destination"));
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (view_destination_button),
				      eel_gconf_get_boolean (PREF_FILE_MANAGER_COPY_VIEW_DESTINATION, FALSE));
	gtk_widget_show (view_destination_button);
	gtk_box_pack_start (GTK_BOX (box), view_destination_button, FALSE, FALSE, 0);

	gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))), box, FALSE, FALSE, 0);

	if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT) {
		char *destination_uri;

		destination_uri = gtk_file_chooser_get_uri (GTK_FILE_CHOOSER (dialog));
		if (destination_uri != NULL) {
			gboolean view_destination;

			/* save the options */

			view_destination = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (view_destination_button));
			eel_gconf_set_boolean (PREF_FILE_MANAGER_COPY_VIEW_DESTINATION, view_destination);
			eel_gconf_set_string (PREF_FILE_MANAGER_COPY_LAST_FOLDER, destination_uri);

			/* copy / move the files */

			copy_files_to_folder (browser, move, destination_uri, view_destination);
		}

		g_free (destination_uri);
	}

	gtk_widget_destroy (dialog);
}


void
gth_browser_activate_action_tool_copy_to_folder (GtkAction  *action,
						 GthBrowser *browser)
{
	copy_to_folder_dialog (browser, FALSE);
}


void
gth_browser_activate_action_tool_move_to_folder (GtkAction  *action,
						 GthBrowser *browser)
{
	copy_to_folder_dialog (browser, TRUE);
}
