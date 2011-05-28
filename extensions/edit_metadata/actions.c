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
#include <gthumb.h>
#include "dlg-edit-metadata.h"
#include "gth-tag-chooser-dialog.h"
#include "gth-tag-task.h"


void
gth_browser_activate_action_edit_metadata (GtkAction  *action,
				 	   GthBrowser *browser)
{
	GList *items;
	GList *file_data_list;
	GList *file_list;

	items = gth_file_selection_get_selected (GTH_FILE_SELECTION (gth_browser_get_file_list_view (browser)));
	file_data_list = gth_file_list_get_files (GTH_FILE_LIST (gth_browser_get_file_list (browser)), items);
	file_list = gth_file_data_list_to_file_list (file_data_list);
	dlg_edit_metadata (browser, file_list);

	_g_object_list_unref (file_list);
	_g_object_list_unref (file_data_list);
	_gtk_tree_path_list_free (items);
}


static void
tag_chooser_dialog_response_cb (GtkDialog *dialog,
				int        response_id,
				gpointer   user_data)
{
	GthBrowser *browser = user_data;

	switch (response_id) {
	case GTK_RESPONSE_HELP:
		show_help_dialog (GTK_WINDOW (browser), "assign-tags");
		break;

	case GTK_RESPONSE_OK:
		{
			GList    *items;
			GList    *file_data_list;
			GList    *file_list;
			char    **tags;
			GthTask  *task;

			items = gth_file_selection_get_selected (GTH_FILE_SELECTION (gth_browser_get_file_list_view (browser)));
			file_data_list = gth_file_list_get_files (GTH_FILE_LIST (gth_browser_get_file_list (browser)), items);
			file_list = gth_file_data_list_to_file_list (file_data_list);
			tags = gth_tag_chooser_dialog_get_tags (GTH_TAG_CHOOSER_DIALOG (dialog));
			task = gth_tag_task_new (file_list, tags);
			gth_browser_exec_task (browser, task, FALSE);
			gtk_widget_destroy (GTK_WIDGET (dialog));

			g_object_unref (task);
			g_strfreev (tags);
			_g_object_list_unref (file_list);
			_g_object_list_unref (file_data_list);
			_gtk_tree_path_list_free (items);
		}
		break;

	case GTK_RESPONSE_DELETE_EVENT:
	case GTK_RESPONSE_CANCEL:
		gtk_widget_destroy (GTK_WIDGET (dialog));
		break;
	}
}


void
gth_browser_activate_action_edit_tag_files (GtkAction  *action,
					    GthBrowser *browser)
{
	GtkWidget *dialog;

	dialog = gth_tag_chooser_dialog_new ();
	g_signal_connect (dialog,
			  "delete-event",
			  G_CALLBACK (gtk_true),
			  NULL);
	g_signal_connect (dialog,
			  "response",
			  G_CALLBACK (tag_chooser_dialog_response_cb),
			  browser);
	gtk_window_set_transient_for (GTK_WINDOW (dialog), GTK_WINDOW (browser));
	gtk_window_present (GTK_WINDOW (dialog));
}


void
gth_browser_activate_action_tool_delete_metadata (GtkAction  *action,
						  GthBrowser *browser)
{
}
