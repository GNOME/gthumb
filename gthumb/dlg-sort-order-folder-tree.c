/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2023 The Free Software Foundation, Inc.
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
#include "dlg-sort-order-folder-tree.h"
#include "gth-browser.h"
#include "gth-folder-tree.h"
#include "gth-main.h"
#include "gtk-utils.h"


#define GET_WIDGET(name) _gtk_builder_get_widget (data->builder, (name))


typedef struct {
	GthBrowser        *browser;
	GtkBuilder        *builder;
	GtkWidget         *dialog;
	GthFolderTreeSort  sort_type;
} DialogData;


static void
destroy_cb (GtkWidget  *widget,
	    DialogData *data)
{
	gth_browser_set_dialog (data->browser, "sort-order-folder-tree", NULL);
	g_object_unref (data->builder);
	g_free (data);
}


static void
apply_sort_order (GtkWidget  *widget,
		  gpointer    user_data)
{
	DialogData *data = user_data;

	gth_folder_tree_set_sort_type (
		GTH_FOLDER_TREE (gth_browser_get_folder_tree (data->browser)),
		(gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("time_radiobutton"))) ? GTH_FOLDER_TREE_SORT_MODIFICATION_TIME : GTH_FOLDER_TREE_SORT_NAME),
		gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("inverse_checkbutton"))));
}


void
dlg_sort_order_folder_tree (GthBrowser *browser)
{
	DialogData        *data;
	GthFolderTreeSort  sort_type;
	gboolean           sort_inverse;

	if (gth_browser_get_dialog (browser, "sort-order-folder-tree") != NULL) {
		gtk_window_present (GTK_WINDOW (gth_browser_get_dialog (browser, "sort-order-folder-tree")));
		return;
	}

	data = g_new0 (DialogData, 1);
	data->browser = browser;
	data->builder = _gtk_builder_new_from_file ("sort-order-folder-tree.ui", NULL);

	/* Get the widgets. */

	data->dialog = g_object_new (GTK_TYPE_DIALOG,
				     "title", _("Sort By"),
				     "transient-for", GTK_WINDOW (browser),
				     "modal", FALSE,
				     "destroy-with-parent", FALSE,
				     "use-header-bar", _gtk_settings_get_dialogs_use_header (),
				     NULL);
	gtk_container_add (GTK_CONTAINER (gtk_dialog_get_content_area (GTK_DIALOG (data->dialog))),
			   _gtk_builder_get_widget (data->builder, "dialog_content"));
	gtk_dialog_add_buttons (GTK_DIALOG (data->dialog),
				_GTK_LABEL_OK, GTK_RESPONSE_CLOSE,
				NULL);

	gth_browser_set_dialog (browser, "sort-order-folder-tree", data->dialog);
	g_object_set_data (G_OBJECT (data->dialog), "dialog_data", data);

	/* Set widgets data. */

	gth_folder_tree_get_sort_type (GTH_FOLDER_TREE (gth_browser_get_folder_tree (data->browser)), &sort_type, &sort_inverse);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("time_radiobutton")), (sort_type == GTH_FOLDER_TREE_SORT_MODIFICATION_TIME));
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("filename_radiobutton")), (sort_type != GTH_FOLDER_TREE_SORT_MODIFICATION_TIME));
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("inverse_checkbutton")), sort_inverse);

	/* Set the signals handlers. */

	g_signal_connect (G_OBJECT (data->dialog),
			  "destroy",
			  G_CALLBACK (destroy_cb),
			  data);
	g_signal_connect_swapped (gtk_dialog_get_widget_for_response (GTK_DIALOG (data->dialog), GTK_RESPONSE_CLOSE),
				  "clicked",
				  G_CALLBACK (gtk_widget_destroy),
				  G_OBJECT (data->dialog));
	g_signal_connect (GET_WIDGET ("filename_radiobutton"),
			  "toggled",
			  G_CALLBACK (apply_sort_order),
			  data);
	g_signal_connect (GET_WIDGET ("time_radiobutton"),
			  "toggled",
			  G_CALLBACK (apply_sort_order),
			  data);
	g_signal_connect (GET_WIDGET ("inverse_checkbutton"),
			  "toggled",
			  G_CALLBACK (apply_sort_order),
			  data);

	/* run dialog. */

	gtk_window_set_transient_for (GTK_WINDOW (data->dialog),
				      GTK_WINDOW (browser));
	gtk_window_set_modal (GTK_WINDOW (data->dialog), FALSE);
	gtk_widget_show (data->dialog);
}
