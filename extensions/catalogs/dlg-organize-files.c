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
#include "dlg-organize-files.h"
#include "gth-organize-task.h"


#define GET_WIDGET(name) _gtk_builder_get_widget (data->builder, (name))


typedef struct {
	GthBrowser *browser;
	GtkBuilder *builder;
	GtkWidget  *dialog;
	GFile      *folder;
} DialogData;


static void
destroy_cb (GtkWidget  *widget,
	    DialogData *data)
{
	g_object_ref (data->folder);
	g_object_unref (data->builder);
	g_free (data);
}


static void
start_button_clicked_cb (GtkWidget  *widget,
			 DialogData *data)
{
	GthTask *task;

	task = gth_organize_task_new (data->browser, data->folder, gtk_combo_box_get_active (GTK_COMBO_BOX (GET_WIDGET ("group_by_combobox"))));
	gth_organize_task_set_recursive (GTH_ORGANIZE_TASK (task), gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("include_subfolders_checkbutton"))));
	gth_organize_task_set_create_singletons (GTH_ORGANIZE_TASK (task), ! gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("ignore_singletons_checkbutton"))));
	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("use_singletons_catalog_checkbutton"))))
		gth_organize_task_set_singletons_catalog (GTH_ORGANIZE_TASK (task), gtk_entry_get_text (GTK_ENTRY (GET_WIDGET ("single_catalog_entry"))));
	gth_browser_exec_task (data->browser, task, FALSE);

	gtk_widget_destroy (data->dialog);
	g_object_unref (task);
}


static void
help_button_clicked_cb (GtkWidget  *widget,
			DialogData *data)
{
	show_help_dialog (GTK_WINDOW (data->dialog), "organize-files");
}


static void
ignore_singletons_checkbutton_clicked_cb (GtkToggleButton *button,
					  DialogData      *data)
{
	if (gtk_toggle_button_get_active (button)) {
		gtk_widget_set_sensitive (GET_WIDGET ("single_catalog_box"), TRUE);
		gtk_toggle_button_set_inconsistent (GTK_TOGGLE_BUTTON (GET_WIDGET ("use_singletons_catalog_checkbutton")), FALSE);
	}
	else {
		gtk_toggle_button_set_inconsistent (GTK_TOGGLE_BUTTON (GET_WIDGET ("use_singletons_catalog_checkbutton")), TRUE);
		gtk_widget_set_sensitive (GET_WIDGET ("single_catalog_box"), FALSE);
	}
}


static void
use_singletons_catalog_checkbutton_clicked_cb (GtkToggleButton *button,
					       DialogData      *data)
{
	gtk_widget_set_sensitive (GET_WIDGET ("single_catalog_entry"), gtk_toggle_button_get_active (button));
}


void
dlg_organize_files (GthBrowser *browser,
		    GFile      *folder)
{
	DialogData *data;

	g_return_if_fail (folder != NULL);

	data = g_new0 (DialogData, 1);
	data->browser = browser;
	data->folder = g_file_dup (folder);
	data->builder = _gtk_builder_new_from_file ("organize-files.ui", "catalogs");
	data->dialog = GET_WIDGET ("organize_files_dialog");

	/* Set the signals handlers. */

	g_signal_connect (G_OBJECT (data->dialog),
			  "destroy",
			  G_CALLBACK (destroy_cb),
			  data);
	g_signal_connect_swapped (G_OBJECT (GET_WIDGET ("cancel_button")),
				  "clicked",
				  G_CALLBACK (gtk_widget_destroy),
				  data->dialog);
	g_signal_connect (G_OBJECT (GET_WIDGET ("help_button")),
			  "clicked",
			  G_CALLBACK (help_button_clicked_cb),
			  data);
	g_signal_connect (G_OBJECT (GET_WIDGET ("start_button")),
			  "clicked",
			  G_CALLBACK (start_button_clicked_cb),
			  data);
	g_signal_connect (G_OBJECT (GET_WIDGET ("ignore_singletons_checkbutton")),
			  "clicked",
			  G_CALLBACK (ignore_singletons_checkbutton_clicked_cb),
			  data);
	g_signal_connect (G_OBJECT (GET_WIDGET ("use_singletons_catalog_checkbutton")),
			  "clicked",
			  G_CALLBACK (use_singletons_catalog_checkbutton_clicked_cb),
			  data);

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("include_subfolders_checkbutton")), TRUE);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("ignore_singletons_checkbutton")), FALSE);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("use_singletons_catalog_checkbutton")), FALSE);

	/* run dialog. */

	gtk_window_set_transient_for (GTK_WINDOW (data->dialog), GTK_WINDOW (browser));
	gtk_window_set_modal (GTK_WINDOW (data->dialog), TRUE);
	gtk_widget_show (data->dialog);
}
