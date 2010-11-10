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
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */


#include <config.h>
#include <glib/gi18n.h>
#include <gthumb.h>
#include "preferences.h"
#include <extensions/file_manager/gth-copy-task.h>

struct _CopyMoveToFolderPrivate{
	GthFileData   *destination;
	GthBrowser   *browser;
};

typedef struct _CopyMoveToFolderPrivate CopyMoveToFolderPrivate;

/* get the previous dir stored in gconf
 default to the home dir if no previous */
static char*
copy_move_to_folder_get_start_uri(gboolean move) {
	GFile		*home_dir_gfile;
	char		*default_uri;
	char		*start_uri;

	home_dir_gfile = g_file_new_for_path(g_get_home_dir());
	default_uri = g_file_get_uri(home_dir_gfile);
	if(move)
		start_uri = eel_gconf_get_string (PREF_COPY_MOVE_TO_FOLDER_MOVE_URI, default_uri);
	else
		start_uri = eel_gconf_get_string (PREF_COPY_MOVE_TO_FOLDER_COPY_URI, default_uri);
	g_object_unref(home_dir_gfile);
	g_free(default_uri);
	return start_uri;
}

/* ask the user if they wish to move to the destination folder */
void
copy_complete_cb(GthTask    *task,
				GError     *error,
				gpointer   data)
{
	/* TODO */
	/* what should we do on error here? */

	GthBrowser *browser;
	GthFileData *destination;
	CopyMoveToFolderPrivate *priv;

	priv = data;
	browser = priv->browser;
	destination = priv->destination;

	gth_browser_load_location(browser, destination->file);

	g_object_unref(browser);
	g_object_unref(destination);
	g_free(priv);

}

/* get the list of files and create and execute the task */
static void
copy_move_to_folder_copy_files(GthBrowser *browser,
					gboolean move,
					char *selected_folder_uri)
{
	GList		*items;
	GList		*file_list;
	GthTask 	*task;
	GthFileData	*destination_path_fd;
	GthFileSource	*file_source;
	GList		*files;
	GList 		*scan;
	GthFileData 	*fd;

	//get the selected folder as a GFile
	GFile *destination_path;
	destination_path = g_file_new_for_uri(selected_folder_uri);

	//create a file data object for the destination GFile
	destination_path_fd = gth_file_data_new(destination_path, NULL);
	g_object_unref(destination_path);
	file_source = gth_main_get_file_source (destination_path_fd->file);

	//create the GList of GFiles to copied
	items = gth_file_selection_get_selected (GTH_FILE_SELECTION (gth_browser_get_file_list_view (browser)));
	file_list = gth_file_list_get_files (GTH_FILE_LIST (gth_browser_get_file_list (browser)), items);

	files = NULL;

	for (scan = file_list; scan; scan = scan->next)
	{
		fd = (GthFileData*) scan->data;
		files = g_list_prepend(files, g_file_dup(fd->file));
	}

	// create and execute the task
	task = gth_copy_task_new (file_source, destination_path_fd, move, files);

	// setup copy completed signal
	CopyMoveToFolderPrivate *data;
	data = g_new0(CopyMoveToFolderPrivate, 1);
	data->destination = destination_path_fd;
	data->browser = browser;
	g_object_ref(browser);
	g_object_ref(destination_path_fd);
	g_signal_connect (task,
			  "completed",
			  G_CALLBACK (copy_complete_cb),
			  (gpointer)data);
	gth_browser_exec_task (browser, task, FALSE);

	//free data
	g_object_unref(destination_path_fd);
	g_object_unref(file_source);
	_g_object_list_unref (file_list);
	_gtk_tree_path_list_free (items);
	_g_object_list_unref (files);

}

/* show the dialog and execute the copy/move */
static void
copy_move_to_folder(GthBrowser *browser,
				gboolean move)
{

	char		*start_uri;
	GtkWidget 	*dialog;
	char 		*selected_folder_uri;

	// create the select folder dialog
	start_uri = copy_move_to_folder_get_start_uri(move);

	dialog = gtk_file_chooser_dialog_new (N_("Select Folder"),
		NULL,
		GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER,
		GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
		GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT,
		NULL);
	gtk_file_chooser_set_current_folder_uri (GTK_FILE_CHOOSER (dialog), start_uri);
	g_free(start_uri);

	if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT)
	{
		selected_folder_uri = gtk_file_chooser_get_uri (GTK_FILE_CHOOSER (dialog));
		copy_move_to_folder_copy_files(browser, move, selected_folder_uri);

		//store the current uri in gconf
		if(move)
			eel_gconf_set_string (PREF_COPY_MOVE_TO_FOLDER_MOVE_URI, selected_folder_uri);
		else
			eel_gconf_set_string (PREF_COPY_MOVE_TO_FOLDER_COPY_URI, selected_folder_uri);

		g_free (selected_folder_uri);
	}
	gtk_widget_destroy (dialog);
}

/* copy to folder action */
void
gth_browser_activate_action_tool_copy_to_folder (GtkAction  *action,
						GthBrowser *browser)
{
	copy_move_to_folder(browser, FALSE);

}

/* move to folder action */
void
gth_browser_activate_action_tool_move_to_folder (GtkAction  *action,
						GthBrowser *browser)
{
	copy_move_to_folder(browser, TRUE);
}
