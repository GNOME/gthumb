/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2009 The Free Software Foundation, Inc.
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
#include <gtk/gtk.h>
#include <gthumb.h>
#include "dlg-edit-metadata.h"
#include "gth-edit-metadata-dialog.h"


typedef struct {
	GthBrowser *browser;
	GtkWidget  *dialog;
	GList      *files; /* GFile list */
	GList      *file_list; /* GthFileData list */
} DialogData;


static void
destroy_cb (GtkWidget  *widget,
	    DialogData *data)
{
	_g_object_list_unref (data->file_list);
	_g_object_list_unref (data->files);
	g_free (data);
}


static void
write_metadata_ready_cb (GError   *error,
			 gpointer  user_data)
{
	DialogData *data = user_data;
	GthMonitor *monitor;
	GList      *scan;

	if (error != NULL) {
		_gtk_error_dialog_from_gerror_show (GTK_WINDOW (data->browser), _("Could not save the file metadata"), &error);
		return;
	}

	monitor = gth_main_get_default_monitor ();
	for (scan = data->file_list; scan; scan = scan->next) {
		GthFileData *file_data = scan->data;
		GFile       *parent;
		GList       *files;

		parent = g_file_get_parent (file_data->file);
		files = g_list_prepend (NULL, g_object_ref (file_data->file));
		gth_monitor_folder_changed (monitor, parent, files, GTH_MONITOR_EVENT_CHANGED);
		gth_monitor_metadata_changed (monitor, file_data);

		_g_object_list_unref (files);
		g_object_unref (parent);
	}

	gtk_widget_destroy (GTK_WIDGET (data->dialog));
}


static void
edit_metadata_dialog__response_cb (GtkDialog *dialog,
				   int        response,
				   gpointer   user_data)
{
	DialogData *data = user_data;

	if (response != GTK_RESPONSE_OK) {
		gtk_widget_destroy (GTK_WIDGET (data->dialog));
		return;
	}

	gth_edit_metadata_dialog_update_info (GTH_EDIT_METADATA_DIALOG (data->dialog), data->file_list);
	_g_write_metadata_async (data->file_list,
				 GTH_METADATA_WRITE_DEFAULT,
				 "*",
				 NULL,
				 write_metadata_ready_cb,
				 data);
}


static void
info_ready_cb (GList    *files,
	       GError   *error,
	       gpointer  user_data)
{
	DialogData *data = user_data;

	if (error != NULL) {
		_gtk_error_dialog_from_gerror_show (GTK_WINDOW (data->browser), _("Cannot read file information"), &error);
		gtk_widget_destroy (GTK_WIDGET (data->dialog));
		return;
	}

	data->file_list = _g_object_list_ref (files);
	gth_edit_metadata_dialog_set_file_list (GTH_EDIT_METADATA_DIALOG (data->dialog), data->file_list);

	gtk_window_set_transient_for (GTK_WINDOW (data->dialog), GTK_WINDOW (data->browser));
	gtk_window_set_modal (GTK_WINDOW (data->dialog), TRUE);
	gtk_window_present (GTK_WINDOW (data->dialog));
}


void
dlg_edit_metadata (GthBrowser *browser,
		   GList      *files)
{
	DialogData *data;

	data = g_new0 (DialogData, 1);
	data->browser = browser;
	data->files = _g_object_list_ref (files);
	data->dialog = gth_edit_metadata_dialog_new ();

	g_signal_connect (G_OBJECT (data->dialog),
			  "destroy",
			  G_CALLBACK (destroy_cb),
			  data);
	g_signal_connect (data->dialog,
			  "response",
			  G_CALLBACK (edit_metadata_dialog__response_cb),
			  data);

	/* FIXME: progress dialog ? */

	_g_query_all_metadata_async (data->files,
				     GTH_LIST_DEFAULT,
				     "*",
				     NULL,
				     info_ready_cb,
				     data);
}
