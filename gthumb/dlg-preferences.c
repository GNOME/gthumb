/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2001-2008 The Free Software Foundation, Inc.
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
#include "dlg-preferences.h"
#include "gth-browser.h"
#include "gth-enum-types.h"
#include "gth-main.h"
#include "gth-preferences.h"
#include "gtk-utils.h"
#include "glib-utils.h"
#include "typedefs.h"


#define GET_WIDGET(name) _gtk_builder_get_widget (data->builder, (name))


typedef struct {
	GthBrowser *browser;
	GtkBuilder *builder;
	GtkWidget  *dialog;
} DialogData;


static void
apply_changes (DialogData *data)
{
	gth_hook_invoke ("dlg-preferences-apply", data->dialog, data->browser, data->builder);
}


static void
destroy_cb (GtkWidget *widget,
	    DialogData *data)
{
	apply_changes (data);
	gth_browser_set_dialog (data->browser, "preferences", NULL);

	g_object_unref (data->builder);
	g_free (data);
}


static void
close_button_clicked_cb (GtkWidget  *widget,
			 DialogData *data)
{
	gtk_widget_destroy (data->dialog);
}


void
dlg_preferences (GthBrowser *browser)
{
	DialogData       *data;

	if (gth_browser_get_dialog (browser, "preferences") != NULL) {
		gtk_window_present (GTK_WINDOW (gth_browser_get_dialog (browser, "preferences")));
		return;
	}

	data = g_new0 (DialogData, 1);
	data->browser = browser;
	data->builder = _gtk_builder_new_from_file ("preferences.ui", NULL);
	data->dialog = GET_WIDGET ("preferences_dialog");

	gth_browser_set_dialog (browser, "preferences", data->dialog);
	gth_hook_invoke ("dlg-preferences-construct", data->dialog, data->browser, data->builder);

	/* Set the signals handlers. */

	g_signal_connect (G_OBJECT (data->dialog),
			  "destroy",
			  G_CALLBACK (destroy_cb),
			  data);
	g_signal_connect (G_OBJECT (GET_WIDGET ("close_button")),
			  "clicked",
			  G_CALLBACK (close_button_clicked_cb),
			  data);

	/* run dialog. */

	gtk_window_set_transient_for (GTK_WINDOW (data->dialog), GTK_WINDOW (browser));
	gtk_window_set_modal (GTK_WINDOW (data->dialog), FALSE);
	gtk_widget_show (data->dialog);
}
