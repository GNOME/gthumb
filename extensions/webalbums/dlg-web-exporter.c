/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2003-2010 Free Software Foundation, Inc.
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
#include <gtk/gtk.h>
#include "dlg-web-exporter.h"

#define GET_WIDGET(name) _gtk_builder_get_widget (data->builder, (name))
#define STRING_IS_VOID(x) (((x) == NULL) || (*(x) == 0))
#define DEFAULT_ALBUM_THEME "Wiki"

static struct {
	int width;
	int height;
} resize_size[] = { { 320, 200 },
		    { 320, 320 },
		    { 640, 480 },
		    { 640, 640 },
		    { 800, 600 },
		    { 800, 800 },
		    { 1024, 768 },
		    { 1024, 1024 },
		    { 1280, 960 },
		    { 1280, 1280 } };

typedef struct {
	GthBrowser *browser;
	GtkBuilder *builder;
	GtkWidget  *dialog;
	GtkWidget  *thumbnail_caption_chooser;
	GtkWidget  *image_caption_chooser;
} DialogData;


static void
destroy_cb (GtkWidget  *widget,
	    DialogData *data)
{
	gth_browser_set_dialog (data->browser, "web_exporter", NULL);
	g_object_unref (data->builder);
	g_free (data);
}


static void
help_clicked_cb (GtkWidget  *widget,
		 DialogData *data)
{
	show_help_dialog (GTK_WINDOW (data->dialog), "webalbums");
}


static void
ok_clicked_cb (GtkWidget  *widget,
	       DialogData *data)
{
	GthTask *task;

	task = gth_web_exporter_new (data->browser, NULL);
	gth_browser_exec_task (data->browser, task, FALSE);

	gtk_widget_destroy (data->dialog);
}


static void
update_sensitivity (DialogData *data)
{
	gtk_widget_set_sensitive (GET_WIDGET ("resize_images_combobox"), gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("resize_images_checkbutton"))));
	gtk_widget_set_sensitive (GET_WIDGET ("resize_images_hbox"), gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("copy_images_checkbutton"))));
}


static void
footer_entry_icon_press_cb (GtkEntry            *entry,
			    GtkEntryIconPosition icon_pos,
			    GdkEvent            *event,
			    gpointer             user_data)
{
	DialogData *data = user_data;
	GtkWidget  *help_box;

	help_box = GET_WIDGET ("template_help_table");
	if (GTK_WIDGET_VISIBLE (help_box))
		gtk_widget_hide (help_box);
	else
		gtk_widget_show (help_box);
}


void
dlg_web_exporter (GthBrowser *browser)
{
	DialogData *data;
	int         i;
	int         active_index;
	GList      *scan;

	if (gth_browser_get_dialog (browser, "web_exporter") != NULL) {
		gtk_window_present (GTK_WINDOW (gth_browser_get_dialog (browser, "web_exporter")));
		return;
	}

	data = g_new0 (DialogData, 1);
	data->browser = browser;
	data->builder = _gtk_builder_new_from_file ("web-album-exporter.ui", "webalbums");

	data->dialog = _gtk_builder_get_widget (data->builder, "web_album_dialog");
	gth_browser_set_dialog (browser, "web_exporter", data->dialog);
	g_object_set_data (G_OBJECT (data->dialog), "dialog_data", data);

	data->thumbnail_caption_chooser = gth_metadata_chooser_new (GTH_METADATA_ALLOW_IN_PRINT);
	gtk_widget_show (data->thumbnail_caption_chooser);
	gtk_container_add (GTK_CONTAINER (GET_WIDGET ("thumbnail_caption_scrolledwindow")), data->thumbnail_caption_chooser);

	data->image_caption_chooser = gth_metadata_chooser_new (GTH_METADATA_ALLOW_IN_PRINT);
	gtk_widget_show (data->image_caption_chooser);
	gtk_container_add (GTK_CONTAINER (GET_WIDGET ("image_caption_scrolledwindow")), data->image_caption_chooser);

	/* Set widgets data. */

	active_index = 0;
	for (i = 0; i < G_N_ELEMENTS (resize_size); i++) {
		GtkTreeIter  iter;
		char        *name;

		gtk_list_store_append (GTK_LIST_STORE (GET_WIDGET ("size_liststore")), &iter);

		/* Translators: this is an image size, such as 1024 × 768 */
		name = g_strdup_printf (_("%d × %d"), resize_size[i].width, resize_size[i].height);
		gtk_list_store_set (GTK_LIST_STORE (GET_WIDGET ("size_liststore")), &iter,
				    0, name,
				    -1);

		g_free (name);
	}
	gtk_combo_box_set_active (GTK_COMBO_BOX (GET_WIDGET ("resize_images_combobox")), active_index);

	active_index = 0;
	for (i = 0, scan = gth_main_get_all_sort_types (); scan; scan = scan->next, i++) {
		GthFileDataSort *sort_type = scan->data;
		GtkTreeIter      iter;

		if (g_str_equal (sort_type->name, "file::mtime"))
			active_index = i;

		gtk_list_store_append (GTK_LIST_STORE (GET_WIDGET ("sort_liststore")), &iter);
		gtk_list_store_set (GTK_LIST_STORE (GET_WIDGET ("sort_liststore")), &iter,
				    0, sort_type,
				    1, sort_type->display_name,
				    -1);
	}
	gtk_combo_box_set_active (GTK_COMBO_BOX (GET_WIDGET ("sort_combobox")), active_index);

	gtk_entry_set_text (GTK_ENTRY (GET_WIDGET ("header_entry")),
			    g_file_info_get_edit_name (gth_browser_get_location_data (browser)->info));

	update_sensitivity (data);

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
				  data->dialog);
	g_signal_connect_swapped (GET_WIDGET ("copy_images_checkbutton"),
				  "clicked",
				  G_CALLBACK (update_sensitivity),
				  data);
	g_signal_connect_swapped (GET_WIDGET ("resize_images_checkbutton"),
				  "clicked",
				  G_CALLBACK (update_sensitivity),
				  data);
	g_signal_connect (GET_WIDGET ("footer_entry"),
			  "icon-press",
			  G_CALLBACK (footer_entry_icon_press_cb),
			  data);

	/* Run dialog. */

	gtk_window_set_transient_for (GTK_WINDOW (data->dialog), GTK_WINDOW (browser));
	gtk_window_set_modal (GTK_WINDOW (data->dialog), FALSE);
	gtk_widget_show (data->dialog);
}
