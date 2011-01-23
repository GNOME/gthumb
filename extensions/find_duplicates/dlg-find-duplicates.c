/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2005-2009 Free Software Foundation, Inc.
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
#include "dlg-find-duplicates.h"


#define GET_WIDGET(name) _gtk_builder_get_widget (data->builder, (name))


typedef struct {
	GthBrowser *browser;
	GtkBuilder *builder;
	GtkWidget  *dialog;
	GtkWidget  *location_chooser;
} DialogData;


static void
destroy_cb (GtkWidget  *widget,
	    DialogData *data)
{
	gth_browser_set_dialog (data->browser, "find_duplicates", NULL);

	g_object_unref (data->builder);
	g_free (data);
}


static void
help_clicked_cb (GtkWidget  *widget,
		 DialogData *data)
{
	show_help_dialog (GTK_WINDOW (data->dialog), "gthumb-find-duplicates");
}


static void
ok_clicked_cb (GtkWidget  *widget,
	       DialogData *data)
{
	/* FIXME */
	gtk_widget_destroy (data->dialog);
}


void
dlg_find_duplicates (GthBrowser *browser)
{
	DialogData *data;

	if (gth_browser_get_dialog (browser, "find_duplicates") != NULL) {
		gtk_window_present (GTK_WINDOW (gth_browser_get_dialog (browser, "find_duplicates")));
		return;
	}

	data = g_new0 (DialogData, 1);
	data->browser = browser;
	data->builder = _gtk_builder_new_from_file ("find-duplicates.ui", "find_duplicates");

	/* Get the widgets. */

	data->dialog = _gtk_builder_get_widget (data->builder, "find_duplicates_dialog");
	gth_browser_set_dialog (browser, "find_duplicates", data->dialog);
	g_object_set_data (G_OBJECT (data->dialog), "dialog_data", data);

	data->location_chooser = gth_location_chooser_new ();
	gtk_widget_show (data->location_chooser);
  	gtk_box_pack_start (GTK_BOX (GET_WIDGET ("location_box")), data->location_chooser, TRUE, TRUE, 0);
  	gtk_label_set_mnemonic_widget (GTK_LABEL (GET_WIDGET ("start_at_label")), data->location_chooser);

	/* Set widgets data. */

	gth_location_chooser_set_current (GTH_LOCATION_CHOOSER (data->location_chooser), gth_browser_get_location (browser));

	/* Set the signals handlers. */

	g_signal_connect (G_OBJECT (data->dialog),
			  "destroy",
			  G_CALLBACK (destroy_cb),
			  data);
	g_signal_connect (GET_WIDGET ("ok_button"),
			  "clicked",
			  G_CALLBACK (ok_clicked_cb),
			  data);
        g_signal_connect (GET_WIDGET ("help_button"),
                          "clicked",
                          G_CALLBACK (help_clicked_cb),
                          data);
	g_signal_connect_swapped (GET_WIDGET ("cancel_button"),
				  "clicked",
				  G_CALLBACK (gtk_widget_destroy),
				  G_OBJECT (data->dialog));

	/* Run dialog. */


	gtk_window_set_transient_for (GTK_WINDOW (data->dialog), GTK_WINDOW (browser));
	gtk_window_set_modal (GTK_WINDOW (data->dialog), FALSE);
	gtk_widget_show (data->dialog);
}
