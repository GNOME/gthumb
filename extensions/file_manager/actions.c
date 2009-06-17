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
#include "gth-duplicate-task.h"


void
gth_browser_action_new_folder (GtkAction  *action,
			       GthBrowser *browser)
{
}


void
gth_browser_action_rename_folder (GtkAction  *action,
				  GthBrowser *browser)
{
}


void
gth_browser_activate_action_edit_cut_files (GtkAction  *action,
					    GthBrowser *browser)
{
	gth_browser_clipboard_cut (browser);
}


void
gth_browser_activate_action_edit_copy_files (GtkAction  *action,
					     GthBrowser *browser)
{
	gth_browser_clipboard_copy (browser);
}


void
gth_browser_activate_action_edit_paste_in_folder (GtkAction  *action,
						  GthBrowser *browser)
{
	gth_browser_clipboard_paste (browser);
}


void
gth_browser_activate_action_edit_duplicate (GtkAction  *action,
					    GthBrowser *browser)
{
	GList   *items;
	GList   *file_list;
	GthTask *task;

	items = gth_file_selection_get_selected (GTH_FILE_SELECTION (gth_browser_get_file_list_view (browser)));
	file_list = gth_file_list_get_files (GTH_FILE_LIST (gth_browser_get_file_list (browser)), items);
	task = gth_duplicate_task_new (file_list);
	gth_browser_exec_task (browser, task);

	g_object_unref (task);
	_g_object_list_unref (file_list);
	_gtk_tree_path_list_free (items);
}


void
gth_browser_activate_action_edit_rename (GtkAction  *action,
					 GthBrowser *browser)
{
}


static void
delete_file_permanently (GtkWindow *window,
			 GList     *file_list)
{
	GList  *files;
	GError *error = NULL;

	files = gth_file_data_list_to_file_list (file_list);
	if (! _g_delete_files (files, TRUE, &error))
		_gtk_error_dialog_from_gerror_show (window, _("Could not delete the files"), &error);

	_g_object_list_unref (files);
}


static void
delete_permanently_response_cb (GtkDialog *dialog,
				int        response_id,
				gpointer   user_data)
{
	GList  *file_list = user_data;

	if (response_id == GTK_RESPONSE_YES)
		delete_file_permanently (gtk_window_get_transient_for (GTK_WINDOW (dialog)), file_list);
	gtk_widget_destroy (GTK_WIDGET (dialog));
	_g_object_list_unref (file_list);
}


void
gth_browser_activate_action_edit_trash (GtkAction  *action,
					GthBrowser *browser)
{
	GList  *items;
	GList  *file_list = NULL;
	GList  *scan;
	GError *error = NULL;

	items = gth_file_selection_get_selected (GTH_FILE_SELECTION (gth_browser_get_file_list_view (browser)));
	file_list = gth_file_list_get_files (GTH_FILE_LIST (gth_browser_get_file_list (browser)), items);

	for (scan = file_list; scan; scan = scan->next) {
		GthFileData *file_data = scan->data;

		if (! g_file_trash (file_data->file, NULL, &error)) {
			if (g_error_matches (error, G_IO_ERROR,  G_IO_ERROR_NOT_SUPPORTED)) {
				GtkWidget *d;

				g_clear_error (&error);

				d = _gtk_yesno_dialog_new (GTK_WINDOW (browser),
							   GTK_DIALOG_MODAL,
							   _("The files cannot be moved to the Trash. Do you want to delete them permanently?"),
							   GTK_STOCK_CANCEL,
							   GTK_STOCK_DELETE);
				g_signal_connect (d, "response", G_CALLBACK (delete_permanently_response_cb), file_list);
				gtk_widget_show (d);

				file_list = NULL;

				break;
			}
			_gtk_error_dialog_from_gerror_show (GTK_WINDOW (browser), _("Could not move the files to the Trash"), &error);
			break;
		}
	}

	_g_object_list_unref (file_list);
	_gtk_tree_path_list_free (items);
}


void
gth_browser_activate_action_edit_delete (GtkAction  *action,
					 GthBrowser *browser)
{
	GList     *items;
	GList     *file_list = NULL;
	int        file_count;
	char      *prompt;
	GtkWidget *d;

	items = gth_file_selection_get_selected (GTH_FILE_SELECTION (gth_browser_get_file_list_view (browser)));
	file_list = gth_file_list_get_files (GTH_FILE_LIST (gth_browser_get_file_list (browser)), items);

	file_count = g_list_length (file_list);
	if (file_count == 1) {
		GthFileData *file_data = file_list->data;

		prompt = g_strdup_printf (_("Are you sure you want to permanently delete \"%s\"?"), g_file_info_get_display_name (file_data->info));
	}
	else
		prompt = g_strdup_printf (ngettext("Are you sure you want to permanently delete "
						   "the %'d selected file?",
						   "Are you sure you want to permanently delete "
					  	   "the %'d selected files?", file_count),
					  file_count);

	d = _gtk_message_dialog_new (GTK_WINDOW (browser),
				     GTK_DIALOG_MODAL,
				     GTK_STOCK_DIALOG_QUESTION,
				     prompt,
				     _("If you delete a file, it will be permanently lost."),
				     GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
				     GTK_STOCK_DELETE, GTK_RESPONSE_YES,
				     NULL);
	g_signal_connect (d, "response", G_CALLBACK (delete_permanently_response_cb), file_list);
	gtk_widget_show (d);

	g_free (prompt);
	_gtk_tree_path_list_free (items);
}
