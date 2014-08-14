/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2014 The Free Software Foundation, Inc.
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
#include "dlg-media-viewer-preferences.h"
#include "preferences.h"


typedef struct {
	GtkBuilder *builder;
	GSettings  *settings;
	GtkWidget  *dialog;
} DialogData;


static void
update_settings (DialogData *data)
{
	char *uri;

	uri = gtk_file_chooser_get_uri (GTK_FILE_CHOOSER (gtk_builder_get_object (data->builder, "screenshots_filechooserbutton")));
	if (uri == NULL)
		return;

	_g_settings_set_uri (data->settings,
			     PREF_GSTREAMER_TOOLS_SCREESHOT_LOCATION,
			     uri);

	g_free (uri);
}


static void
destroy_cb (GtkWidget  *widget,
	    DialogData *data)
{
	update_settings (data);
	g_object_unref (data->builder);
	g_object_unref (data->settings);
	g_free (data);
}


void
dlg_media_viewer_preferences (GtkWindow *parent)
{
	DialogData *data;
	char       *uri;

	data = g_new0 (DialogData, 1);
	data->builder = _gtk_builder_new_from_file ("media-viewer-preferences.ui", "gstreamer_tools");
	data->settings = g_settings_new (GTHUMB_GSTREAMER_TOOLS_SCHEMA);

	/* Get the widgets. */

	data->dialog = _gtk_builder_get_widget (data->builder, "preferences_dialog");

	/* Set widgets data. */

	uri = _g_settings_get_uri_or_special_dir (data->settings, PREF_GSTREAMER_TOOLS_SCREESHOT_LOCATION, G_USER_DIRECTORY_PICTURES);
	gtk_file_chooser_set_uri (GTK_FILE_CHOOSER (gtk_builder_get_object (data->builder, "screenshots_filechooserbutton")), uri);
	g_free (uri);

	/* Set the signals handlers. */

	g_signal_connect (G_OBJECT (data->dialog),
			  "destroy",
			  G_CALLBACK (destroy_cb),
			  data);
	g_signal_connect_swapped (gtk_builder_get_object (data->builder, "close_button"),
				  "clicked",
				  G_CALLBACK (gtk_widget_destroy),
				  G_OBJECT (data->dialog));

	/* run dialog. */

	gtk_window_set_transient_for (GTK_WINDOW (data->dialog), parent);
	gtk_window_set_modal (GTK_WINDOW (data->dialog), TRUE);
	gtk_widget_show (data->dialog);
}
