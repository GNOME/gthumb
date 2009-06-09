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
#include "dlg-edit-metadata.h"
#include "glib-utils.h"
#include "gth-edit-metadata-dialog.h"
#include "gth-main.h"
#include "gth-metadata-provider.h"
#include "gtk-utils.h"


typedef struct {
	GthBrowser  *browser;
	GtkWidget   *dialog;
	GthFileData *file_data;
} DialogData;


static void
destroy_cb (GtkWidget  *widget,
	    DialogData *data)
{
	g_object_unref (data->file_data);
	g_free (data);
}


static void
write_metadata_ready_cb (GError   *error,
			 gpointer  user_data)
{
	DialogData *data = user_data;
	GthMonitor *monitor;
	GFile      *parent;
	GList      *files;

	if (error != NULL) {
		_gtk_error_dialog_from_gerror_show (GTK_WINDOW (data->browser), _("Could not save file properties"), &error);
		return;
	}

	monitor = gth_main_get_default_monitor ();
	parent = g_file_get_parent (data->file_data->file);
	files = g_list_prepend (NULL, g_object_ref (data->file_data->file));
	gth_monitor_folder_changed (monitor, parent, files, GTH_MONITOR_EVENT_CHANGED);

	gtk_widget_destroy (GTK_WIDGET (data->dialog));

	_g_object_list_unref (files);
	g_object_unref (parent);
}


static void
edit_metadata_dialog__response_cb (GtkDialog *dialog,
				   int        response,
				   gpointer   user_data)
{
	DialogData *data = user_data;
	GList      *files;

	if (response != GTK_RESPONSE_OK) {
		gtk_widget_destroy (GTK_WIDGET (data->dialog));
		return;
	}

	gth_edit_metadata_dialog_update_info (GTH_EDIT_METADATA_DIALOG (data->dialog), data->file_data->info);

	files = g_list_prepend (NULL, g_object_ref (data->file_data));
	_g_write_metadata_async (files, "*", NULL, write_metadata_ready_cb, data);

	_g_object_list_unref (files);
}


void
dlg_edit_metadata (GthBrowser *browser)
{
	DialogData *data;

	if (gth_browser_get_current_file (browser) == NULL)
		return;

	data = g_new0 (DialogData, 1);
	data->browser = browser;
	data->file_data = g_object_ref (gth_browser_get_current_file (browser));
	data->dialog = gth_edit_metadata_dialog_new ();

	g_signal_connect (G_OBJECT (data->dialog),
			  "destroy",
			  G_CALLBACK (destroy_cb),
			  data);
	g_signal_connect (data->dialog,
			  "response",
			  G_CALLBACK (edit_metadata_dialog__response_cb),
			  data);

	gtk_window_set_transient_for (GTK_WINDOW (data->dialog), GTK_WINDOW (browser));
	gtk_window_set_modal (GTK_WINDOW (data->dialog), TRUE);
	gtk_window_present (GTK_WINDOW (data->dialog));

	gth_edit_metadata_dialog_set_file (GTH_EDIT_METADATA_DIALOG (data->dialog), data->file_data);
}
