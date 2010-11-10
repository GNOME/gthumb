/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2010 The Free Software Foundation, Inc.
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
#include <gtk/gtk.h>
#include <gthumb.h>
#include "preferences.h"


typedef struct {
	GtkBuilder *builder;
	GtkWidget  *dialog;
} DialogData;


static void
destroy_cb (GtkWidget  *widget,
	    DialogData *data)
{
	g_object_unref (data->builder);
	g_free (data);
}


static void
ask_checkbutton_clicked_cb (GtkWidget  *widget,
			     DialogData *data)
{
	eel_gconf_set_boolean (PREF_COPY_MOVE_TO_FOLDER_SHOW_DIALOG, gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (gtk_builder_get_object (data->builder, "ask_button"))));
}


void
dlg_copy_move_to_folder_preferences (GtkWindow *parent)
{
	DialogData *data;

	data = g_new0 (DialogData, 1);
	data->builder = _gtk_builder_new_from_file ("copy-move-to-folder-preferences.ui", "copy-move-to-folder");

	/* Get the widgets. */

	data->dialog = _gtk_builder_get_widget (data->builder, "preferences_dialog");

	/* Set widgets data. */

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (gtk_builder_get_object (data->builder, "ask_button")), eel_gconf_get_boolean (PREF_COPY_MOVE_TO_FOLDER_SHOW_DIALOG, TRUE));

	/* Set the signals handlers. */

	g_signal_connect (G_OBJECT (data->dialog),
			  "destroy",
			  G_CALLBACK (destroy_cb),
			  data);
	g_signal_connect_swapped (gtk_builder_get_object (data->builder, "close_button"),
				  "clicked",
				  G_CALLBACK (gtk_widget_destroy),
				  G_OBJECT (data->dialog));
	g_signal_connect (gtk_builder_get_object (data->builder, "ask_button"),
			  "clicked",
			  G_CALLBACK (ask_checkbutton_clicked_cb),
			  data);

	/* run dialog. */

	gtk_window_set_transient_for (GTK_WINDOW (data->dialog), parent);
	gtk_window_set_modal (GTK_WINDOW (data->dialog), TRUE);
	gtk_widget_show (data->dialog);
}
