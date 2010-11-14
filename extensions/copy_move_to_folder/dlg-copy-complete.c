/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2001-2010 The Free Software Foundation, Inc.
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

#define GET_WIDGET(x) (_gtk_builder_get_widget (data->builder, (x)))
#define IS_ACTIVE(x) (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (x)))


typedef struct {
	GthBrowser *browser;
	GtkBuilder *builder;
	GtkWidget  *dialog;
	GFile      *location;
} DialogData;


static void
dialog_destroy_cb (GtkWidget  *widget,
		   DialogData *data)
{
	g_object_unref (data->builder);
	g_object_unref (data->location);
	g_object_unref (data->browser);
	g_free (data);
}

static void
close_button_clicked (GtkWidget  *button,
		   DialogData *data)
{
	if (gtk_toggle_button_get_active((GtkToggleButton *)GET_WIDGET ("always_button")))
	{
		eel_gconf_set_boolean (PREF_COPY_MOVE_TO_FOLDER_SHOW_DIALOG, FALSE);
		eel_gconf_set_boolean (PREF_COPY_MOVE_TO_FOLDER_ALWAYS_OPEN, FALSE);
	}
	gtk_widget_destroy (data->dialog);
}

static void
open_button_clicked (GtkWidget  *button,
		   DialogData *data)
{
	gth_browser_load_location(data->browser, data->location);

	if (gtk_toggle_button_get_active((GtkToggleButton *)GET_WIDGET ("always_button")))
	{
		eel_gconf_set_boolean (PREF_COPY_MOVE_TO_FOLDER_SHOW_DIALOG, FALSE);
		eel_gconf_set_boolean (PREF_COPY_MOVE_TO_FOLDER_ALWAYS_OPEN, TRUE);
	}

	gtk_widget_destroy (data->dialog);
}


void
dlg_copy_complete (GthBrowser *browser,
			gboolean move,
			GFile *location)
{
	DialogData  *data;

	if(eel_gconf_get_boolean (PREF_COPY_MOVE_TO_FOLDER_SHOW_DIALOG, TRUE))
	{
		data = g_new0 (DialogData, 1);
		data->location = g_file_dup(location);
		data->browser = browser;
		g_object_ref(browser);
		data->builder = _gtk_builder_new_from_file ("copy-move-to-folder-copy-complete.ui", "copy_move_to_folder");
		data->dialog = GET_WIDGET ("completed_messagedialog");

		g_signal_connect (G_OBJECT (data->dialog),
				  "destroy",
				  G_CALLBACK (dialog_destroy_cb),
				  data);

		g_signal_connect (GET_WIDGET ("close_button"),
				  "clicked",
				  G_CALLBACK (close_button_clicked),
				  data);

		g_signal_connect (GET_WIDGET ("open_button"),
				  "clicked",
				  G_CALLBACK (open_button_clicked),
				  data);

		gtk_window_set_transient_for (GTK_WINDOW (data->dialog),
					      GTK_WINDOW (browser));
		gtk_window_set_modal (GTK_WINDOW (data->dialog), FALSE);
		gtk_widget_show (data->dialog);
	}
	else
	{
		if(eel_gconf_get_boolean (PREF_COPY_MOVE_TO_FOLDER_ALWAYS_OPEN, FALSE))
		{
			gth_browser_load_location(browser, location);
		}
	}
}
