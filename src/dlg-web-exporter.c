/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2003 Free Software Foundation, Inc.
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
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <gtk/gtk.h>
#include <libgnomeui/gnome-file-entry.h>
#include <libgnomeui/gnome-dialog-util.h>
#include <libgnomeui/gnome-color-picker.h>
#include <libgnomeui/gnome-font-picker.h>
#include <libgnomevfs/gnome-vfs-directory.h>
#include <libgnomevfs/gnome-vfs-ops.h>
#include <glade/glade.h>
#include "catalog-web-exporter.h"
#include "dlg-file-utils.h"
#include "file-utils.h"
#include "gtk-utils.h"
#include "gth-file-view.h"
#include "main.h"
#include "pixbuf-utils.h"
#include "gconf-utils.h"
#include "gthumb-window.h"
#include "glib-utils.h"

static int           sort_method_to_idx[] = { -1, 0, 1, 2, 3 };
static GthSortMethod idx_to_sort_method[] = { GTH_SORT_METHOD_BY_NAME, 
					      GTH_SORT_METHOD_BY_PATH, 
					      GTH_SORT_METHOD_BY_SIZE, 
					      GTH_SORT_METHOD_BY_TIME };
static int           idx_to_resize_width[] = { 320, 640, 800, 1024, 1280 };
static int           idx_to_resize_height[] = { 200, 480, 600, 768, 960 };


#define str_void(x) (((x) == NULL) || (*(x) == 0))
#define GLADE_EXPORTER_FILE "gthumb_web_exporter.glade"


typedef struct {
	GThumbWindow       *window;

	GladeXML           *gui;
	GtkWidget          *dialog;

	GtkWidget          *progress_dialog;
	GtkWidget          *progress_progressbar;
	GtkWidget          *progress_info;
	GtkWidget          *progress_cancel;

	GtkWidget          *btn_ok;

	GtkWidget          *wa_select_destination_button;
	GtkWidget          *wa_destination_combo_entry;
	GtkWidget          *wa_destination_combo;
	GtkWidget          *wa_index_file_entry;
	GtkWidget          *wa_copy_images_checkbutton;
	GtkWidget          *wa_resize_images_checkbutton;
	GtkWidget          *wa_resize_images_optionmenu;
	GtkWidget          *wa_resize_images_hbox;
	GtkWidget          *wa_resize_images_options_hbox;

	GtkWidget          *wa_rows_spinbutton;
	GtkWidget          *wa_cols_spinbutton;
	GtkWidget          *wa_sort_images_optionmenu;
	GtkWidget          *wa_reverse_order_checkbutton;

	GtkWidget          *wa_header_entry;
	GtkWidget          *wa_footer_entry;
	GtkWidget          *wa_theme_combo_entry;
	GtkWidget          *wa_theme_combo;
	GtkWidget          *wa_select_theme_button;

	/**/

	CatalogWebExporter *exporter;
	GSList *destinations;
} DialogData;


/* called when the main dialog is closed. */
static void
destroy_cb (GtkWidget  *widget, 
	    DialogData *data)
{
	g_object_unref (data->gui);
	if (data->exporter != NULL)
		g_object_unref (data->exporter);
	g_free (data);
}


static void
export (GtkWidget  *widget,
	DialogData *data)
{
	CatalogWebExporter *exporter = data->exporter;
	char               *location;
	char               *path;
	char               *theme, *index_file;
	const char         *header;
	const char         *footer;

	/* Save options. */

	path = _gtk_entry_get_filename_text (GTK_ENTRY (data->wa_destination_combo_entry));
	location = remove_ending_separator (path);
	g_free (path);
	eel_gconf_set_path (PREF_WEB_ALBUM_DESTINATION, location);

	index_file = _gtk_entry_get_filename_text (GTK_ENTRY (data->wa_index_file_entry));
	eel_gconf_set_string (PREF_WEB_ALBUM_INDEX_FILE, index_file);

	eel_gconf_set_boolean (PREF_WEB_ALBUM_COPY_IMAGES, gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (data->wa_copy_images_checkbutton)));

	eel_gconf_set_boolean (PREF_WEB_ALBUM_RESIZE_IMAGES, gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (data->wa_resize_images_checkbutton)));

	eel_gconf_set_integer (PREF_WEB_ALBUM_RESIZE_WIDTH, idx_to_resize_width[gtk_option_menu_get_history (GTK_OPTION_MENU (data->wa_resize_images_optionmenu))]);

	eel_gconf_set_integer (PREF_WEB_ALBUM_RESIZE_HEIGHT, idx_to_resize_height[gtk_option_menu_get_history (GTK_OPTION_MENU (data->wa_resize_images_optionmenu))]);

	eel_gconf_set_integer (PREF_WEB_ALBUM_ROWS, gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (data->wa_rows_spinbutton)));

	eel_gconf_set_integer (PREF_WEB_ALBUM_COLUMNS, gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (data->wa_cols_spinbutton)));

	pref_set_web_album_sort_order (idx_to_sort_method [gtk_option_menu_get_history (GTK_OPTION_MENU (data->wa_sort_images_optionmenu))]);

	eel_gconf_set_boolean (PREF_WEB_ALBUM_REVERSE, gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (data->wa_reverse_order_checkbutton)));

	header = gtk_entry_get_text (GTK_ENTRY (data->wa_header_entry));
	eel_gconf_set_string (PREF_WEB_ALBUM_HEADER, header);

	footer = gtk_entry_get_text (GTK_ENTRY (data->wa_footer_entry));
	eel_gconf_set_string (PREF_WEB_ALBUM_FOOTER, footer);

	theme = _gtk_entry_get_filename_text (GTK_ENTRY (data->wa_theme_combo_entry));
	eel_gconf_set_string (PREF_WEB_ALBUM_THEME, theme);

	/**/

	if (! dlg_check_folder (data->window, location)) {
		g_free (location);
		return;
	}

	gtk_widget_hide (data->dialog);

	/* Set options. */

	catalog_web_exporter_set_location (exporter, location);
	catalog_web_exporter_set_index_file (exporter, index_file);

	catalog_web_exporter_set_copy_images (exporter, eel_gconf_get_boolean (PREF_WEB_ALBUM_COPY_IMAGES, FALSE));

	catalog_web_exporter_set_resize_images (exporter, 
						eel_gconf_get_boolean (PREF_WEB_ALBUM_RESIZE_IMAGES, FALSE),
						idx_to_resize_width[gtk_option_menu_get_history (GTK_OPTION_MENU (data->wa_resize_images_optionmenu))],
						idx_to_resize_height[gtk_option_menu_get_history (GTK_OPTION_MENU (data->wa_resize_images_optionmenu))]);

	catalog_web_exporter_set_row_col (exporter, eel_gconf_get_integer (PREF_WEB_ALBUM_ROWS, 4), eel_gconf_get_integer (PREF_WEB_ALBUM_COLUMNS, 4));
	
	catalog_web_exporter_set_sorted (exporter, pref_get_web_album_sort_order (), eel_gconf_get_boolean (PREF_WEB_ALBUM_REVERSE, FALSE));
	catalog_web_exporter_set_header (exporter, header);
	catalog_web_exporter_set_footer (exporter, footer);
	catalog_web_exporter_set_style (exporter, theme);

	g_free (location);
	g_free (theme);
	g_free (index_file);
	
	/* Export. */

	gtk_window_set_transient_for (GTK_WINDOW (data->progress_dialog),
				      GTK_WINDOW (data->window->app));
	gtk_window_set_modal (GTK_WINDOW (data->progress_dialog), TRUE);
	gtk_widget_show_all (data->progress_dialog);

	catalog_web_exporter_export (exporter);
}


static void
export_done (GtkObject  *object,
	     DialogData *data)
{
	gtk_widget_destroy (data->progress_dialog);
	gtk_widget_destroy (data->dialog);
}


static void
export_progress (GtkObject  *object,
		 float       percent,
		 DialogData *data)
{
	gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (data->progress_progressbar), percent);
}


static void
export_info (GtkObject  *object,
	     const char *info,
	     DialogData *data)
{
	gtk_label_set_text (GTK_LABEL (data->progress_info), info);
}


static void
export_start_copying (GtkObject  *object,
		      DialogData *data)
{
	gtk_widget_hide (data->progress_dialog);
}


static void show_album_theme_cb (GtkWidget *widget, DialogData *data);


static int
get_idx_from_resize_width (int width)
{
	if (width == 320)
		return 0;
	else if (width == 640)
		return 1;
	else if (width == 800)
		return 2;
	else if (width == 1024)
		return 3;
	else if (width == 1280)
		return 4;
	else
		return 1;
}


static void
copy_image_toggled_cb (GtkToggleButton *button,
		       DialogData      *data)
{
	gtk_widget_set_sensitive (data->wa_resize_images_hbox, gtk_toggle_button_get_active (button));
}


static void
resize_image_toggled_cb (GtkToggleButton *button,
			 DialogData      *data)
{
	gtk_widget_set_sensitive (data->wa_resize_images_options_hbox, gtk_toggle_button_get_active (button));
}


static void
update_destinations_list (DialogData *data)
{
	GList  *list = NULL;
	GSList *scan;
	
	for (scan = data->destinations; scan; scan = scan->next)
		list = g_list_prepend (list, scan->data);
	list = g_list_reverse (list);

	if (list == NULL)
		list = g_list_append (NULL, "");

	gtk_combo_set_popdown_strings (GTK_COMBO (data->wa_destination_combo), list);

	g_list_free (list);
}


static void show_destination_dialog_cb (GtkWidget  *widget, DialogData *data);


/* create the main dialog. */
void
dlg_web_exporter (GThumbWindow *window)
{
	DialogData   *data;
	GtkWidget    *btn_cancel;
	GtkWidget    *btn_help;
	GList        *list;
	char         *svalue;

	data = g_new (DialogData, 1);

	data->window = window;

	list = gth_file_view_get_file_list_selection (window->file_list->view);
	if (list == NULL) {
		g_warning ("No file selected.");
		g_free (data);
		return;
	}

	data->exporter = catalog_web_exporter_new (window, list);
	g_list_foreach (list, (GFunc) g_free, NULL);
	g_list_free (list);

	data->gui = glade_xml_new (GTHUMB_GLADEDIR "/" GLADE_EXPORTER_FILE, NULL, NULL);
        if (!data->gui) {
		g_object_unref (data->exporter);
		g_free (data);
                g_warning ("Could not find " GLADE_EXPORTER_FILE "\n");
                return;
        }

	/* Get the widgets. */

	data->dialog = glade_xml_get_widget (data->gui, "web_album_dialog");
	data->wa_select_destination_button = glade_xml_get_widget (data->gui, "wa_select_destination_button");
	data->wa_destination_combo_entry = glade_xml_get_widget (data->gui, "wa_destination_combo_entry");
	data->wa_destination_combo = glade_xml_get_widget (data->gui, "wa_destination_combo");
	data->wa_index_file_entry = glade_xml_get_widget (data->gui, "wa_index_file_entry");
	data->wa_copy_images_checkbutton = glade_xml_get_widget (data->gui, "wa_copy_images_checkbutton");

	data->wa_resize_images_checkbutton = glade_xml_get_widget (data->gui, "wa_resize_images_checkbutton");
	data->wa_resize_images_optionmenu = glade_xml_get_widget (data->gui, "wa_resize_images_optionmenu");
	data->wa_resize_images_hbox = glade_xml_get_widget (data->gui, "wa_resize_images_hbox");
	data->wa_resize_images_options_hbox = glade_xml_get_widget (data->gui, "wa_resize_images_options_hbox");

	data->wa_rows_spinbutton = glade_xml_get_widget (data->gui, "wa_rows_spinbutton");
	data->wa_cols_spinbutton = glade_xml_get_widget (data->gui, "wa_cols_spinbutton");
	data->wa_sort_images_optionmenu = glade_xml_get_widget (data->gui, "wa_sort_images_optionmenu");
	data->wa_reverse_order_checkbutton = glade_xml_get_widget (data->gui, "wa_reverse_order_checkbutton");

	data->wa_header_entry = glade_xml_get_widget (data->gui, "wa_header_entry");
	data->wa_footer_entry = glade_xml_get_widget (data->gui, "wa_footer_entry");
	data->wa_theme_combo_entry = glade_xml_get_widget (data->gui, "wa_theme_combo_entry");
	data->wa_select_theme_button = glade_xml_get_widget (data->gui, "wa_select_theme_button");

	/**/


	/**/

	data->progress_dialog = glade_xml_get_widget (data->gui, "progress_dialog");
	data->progress_progressbar = glade_xml_get_widget (data->gui, "progress_progressbar");
	data->progress_info = glade_xml_get_widget (data->gui, "progress_info");
	data->progress_cancel = glade_xml_get_widget (data->gui, "progress_cancel");

	btn_cancel = glade_xml_get_widget (data->gui, "wa_cancel_button");
	data->btn_ok = glade_xml_get_widget (data->gui, "wa_ok_button");
	btn_help = glade_xml_get_widget (data->gui, "wa_help_button");

	/* Set widgets data. */

	svalue = eel_gconf_get_string (PREF_WEB_ALBUM_INDEX_FILE, "index.html");
	_gtk_entry_set_filename_text (GTK_ENTRY (data->wa_index_file_entry), svalue);
	g_free (svalue);
	
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (data->wa_copy_images_checkbutton), eel_gconf_get_boolean (PREF_WEB_ALBUM_COPY_IMAGES, FALSE));

	gtk_widget_set_sensitive (data->wa_resize_images_hbox, eel_gconf_get_boolean (PREF_WEB_ALBUM_COPY_IMAGES, FALSE));

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (data->wa_resize_images_checkbutton), eel_gconf_get_boolean (PREF_WEB_ALBUM_RESIZE_IMAGES, FALSE));

	gtk_widget_set_sensitive (data->wa_resize_images_options_hbox, eel_gconf_get_boolean (PREF_WEB_ALBUM_RESIZE_IMAGES, FALSE));

	gtk_option_menu_set_history (GTK_OPTION_MENU (data->wa_resize_images_optionmenu), get_idx_from_resize_width (eel_gconf_get_integer (PREF_WEB_ALBUM_RESIZE_WIDTH, 640)));

	gtk_spin_button_set_value (GTK_SPIN_BUTTON (data->wa_rows_spinbutton), eel_gconf_get_integer (PREF_WEB_ALBUM_ROWS, 4));

	gtk_spin_button_set_value (GTK_SPIN_BUTTON (data->wa_cols_spinbutton), eel_gconf_get_integer (PREF_WEB_ALBUM_COLUMNS, 4));

	gtk_option_menu_set_history (GTK_OPTION_MENU (data->wa_sort_images_optionmenu), sort_method_to_idx [pref_get_web_album_sort_order ()]);

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (data->wa_reverse_order_checkbutton), eel_gconf_get_boolean (PREF_WEB_ALBUM_REVERSE, FALSE));

	svalue = eel_gconf_get_string (PREF_WEB_ALBUM_HEADER, "");
	gtk_entry_set_text (GTK_ENTRY (data->wa_header_entry), svalue);
	g_free (svalue);

	svalue = eel_gconf_get_string (PREF_WEB_ALBUM_FOOTER, "");
	gtk_entry_set_text (GTK_ENTRY (data->wa_footer_entry), svalue);
	g_free (svalue);

	svalue = eel_gconf_get_string (PREF_WEB_ALBUM_THEME, "Clean");
	_gtk_entry_set_filename_text (GTK_ENTRY (data->wa_theme_combo_entry), svalue);
	g_free (svalue);

	catalog_web_exporter_set_index_caption (data->exporter, eel_gconf_get_integer (PREF_WEB_ALBUM_INDEX_CAPTION, 0));
	catalog_web_exporter_set_image_caption (data->exporter, eel_gconf_get_integer (PREF_WEB_ALBUM_IMAGE_CAPTION, 0));

	/**/

	data->destinations = eel_gconf_get_path_list (PREF_WEB_ALBUM_DESTINATIONS);
	update_destinations_list (data);

	/**/

	svalue = eel_gconf_get_path (PREF_WEB_ALBUM_DESTINATION, NULL);
	_gtk_entry_set_filename_text (GTK_ENTRY (data->wa_destination_combo_entry),
				      (((svalue == NULL) || (*svalue == 0)) ? g_get_home_dir() : svalue));
	g_free (svalue);

	/* Signals. */

	g_signal_connect (G_OBJECT (data->dialog), 
			  "destroy",
			  G_CALLBACK (destroy_cb),
			  data);
	g_signal_connect_swapped (G_OBJECT (btn_cancel), 
				  "clicked",
				  G_CALLBACK (gtk_widget_destroy),
				  G_OBJECT (data->dialog));
	g_signal_connect (G_OBJECT (data->btn_ok), 
			  "clicked",
			  G_CALLBACK (export),
			  data);

	g_signal_connect (G_OBJECT (data->wa_select_theme_button), 
			  "clicked",
			  G_CALLBACK (show_album_theme_cb),
			  data);

	g_signal_connect (G_OBJECT (data->wa_copy_images_checkbutton), 
			  "toggled",
			  G_CALLBACK (copy_image_toggled_cb),
			  data);
	g_signal_connect (G_OBJECT (data->wa_resize_images_checkbutton), 
			  "toggled",
			  G_CALLBACK (resize_image_toggled_cb),
			  data);

	g_signal_connect (G_OBJECT (data->exporter), 
			  "web_exporter_done",
			  G_CALLBACK (export_done),
			  data);
	g_signal_connect (G_OBJECT (data->exporter), 
			  "web_exporter_progress",
			  G_CALLBACK (export_progress),
			  data);
	g_signal_connect (G_OBJECT (data->exporter), 
			  "web_exporter_info",
			  G_CALLBACK (export_info),
			  data);
	g_signal_connect (G_OBJECT (data->exporter), 
			  "web_exporter_start_copying",
			  G_CALLBACK (export_start_copying),
			  data);

	g_signal_connect_swapped (G_OBJECT (data->progress_dialog), 
				  "delete_event",
				  G_CALLBACK (catalog_web_exporter_interrupt),
				  data->exporter);
	g_signal_connect_swapped (G_OBJECT (data->progress_cancel), 
				  "clicked",
				  G_CALLBACK (catalog_web_exporter_interrupt),
				  data->exporter);

	g_signal_connect (G_OBJECT (data->wa_select_destination_button),
			  "clicked",
			  G_CALLBACK (show_destination_dialog_cb),
			  data);

	/* Run dialog. */

	gtk_widget_grab_focus (data->wa_destination_combo_entry);

	gtk_window_set_transient_for (GTK_WINDOW (data->dialog), GTK_WINDOW (window->app));
	gtk_window_set_modal (GTK_WINDOW (data->dialog), FALSE);
	gtk_widget_show_all (data->dialog);
}





enum {
	DESTINATION_NAME_COLUMN,
	DESTINATION_ENTRY_COLUMN,
};


typedef struct {
	DialogData         *data;
	GThumbWindow       *window;

	GladeXML           *gui;
	GtkWidget          *dialog;

	GtkWidget          *wad_destinations_treeview;
	GtkWidget          *wad_new_button;
	GtkWidget          *wad_edit_button;
	GtkWidget          *wad_remove_button;
	GtkWidget          *wad_cancelbutton;
	GtkWidget          *wad_okbutton;

	GtkListStore       *list_store;
	char               *new_value;
} DestinationsDialogData;


/* called when the main dialog is closed. */
static void
destination_dialog_destroy_cb (GtkWidget             *widget, 
			      DestinationsDialogData *ddata)
{
	eel_gconf_set_path_list (PREF_WEB_ALBUM_DESTINATIONS,
				 ddata->data->destinations);
	update_destinations_list (ddata->data);
	if (ddata->new_value != NULL)
		gtk_entry_set_text (GTK_ENTRY (ddata->data->wa_destination_combo_entry), ddata->new_value);
	g_free (ddata->new_value);
	g_object_unref (ddata->gui);
	g_free (ddata);
}


static void
destination_dialog__ok_clicked (GtkWidget              *widget, 
				DestinationsDialogData *ddata)
{
	GtkTreeSelection *selection;
	gboolean          destination_selected;
	GtkTreeIter       iter;

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (ddata->wad_destinations_treeview));
	destination_selected = gtk_tree_selection_get_selected (selection, NULL, &iter);

	if (destination_selected) 
		gtk_tree_model_get (GTK_TREE_MODEL (ddata->list_store),
				    &iter,
				    DESTINATION_NAME_COLUMN, &ddata->new_value,
				    -1);

	gtk_widget_destroy (ddata->dialog);
}


static void
destination_dialog__row_activated_cb (GtkTreeView            *tree_view,
				      GtkTreePath            *path,
				      GtkTreeViewColumn      *column,
				      DestinationsDialogData *ddata)
{
	destination_dialog__ok_clicked (NULL, ddata);
}


static void show_edit_destination_dialog (DestinationsDialogData *ddata,
					  GSList                 *destination_entry);


static void
destination_dialog__new_clicked (GtkWidget              *widget, 
				 DestinationsDialogData *ddata)
{
	GtkTreeSelection *selection;
	gboolean          destination_selected;
	GtkTreeIter       iter;

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (ddata->wad_destinations_treeview));
	destination_selected = gtk_tree_selection_get_selected (selection, NULL, &iter);

	if (destination_selected) {
		GSList *entry;
		gtk_tree_model_get (GTK_TREE_MODEL (ddata->list_store),
				    &iter,
				    DESTINATION_ENTRY_COLUMN, &entry,
				    -1);
		show_edit_destination_dialog (ddata, entry);

	} else
		show_edit_destination_dialog (ddata, NULL);
}


static void
destination_dialog__edit_clicked (GtkWidget              *widget, 
				  DestinationsDialogData *ddata)
{
	GtkTreeSelection *selection;
	gboolean          destination_selected;
	GtkTreeIter       iter;

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (ddata->wad_destinations_treeview));
	destination_selected = gtk_tree_selection_get_selected (selection, NULL, &iter);

	if (destination_selected) {
		GSList *entry;
		gtk_tree_model_get (GTK_TREE_MODEL (ddata->list_store),
				    &iter,
				    DESTINATION_ENTRY_COLUMN, &entry,
				    -1);
		show_edit_destination_dialog (ddata, entry);
	} 
}


static void
destination_dialog__remove_clicked (GtkWidget              *widget, 
				    DestinationsDialogData *ddata)
{
	GtkTreeSelection *selection;
	gboolean          destination_selected;
	GtkTreeIter       iter;

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (ddata->wad_destinations_treeview));
	destination_selected = gtk_tree_selection_get_selected (selection, NULL, &iter);

	if (destination_selected) {
		GSList *entry;
		gtk_tree_model_get (GTK_TREE_MODEL (ddata->list_store),
				    &iter,
				    DESTINATION_ENTRY_COLUMN, &entry,
				    -1);
		ddata->data->destinations = g_slist_remove_link (ddata->data->destinations, entry);
		g_free (entry->data);
		g_slist_free (entry);

		gtk_list_store_remove (ddata->list_store, &iter);
	}
}


static void
destionation_dialog_update_destination_list (DestinationsDialogData *ddata)
{
	GSList *scan;

	gtk_list_store_clear (ddata->list_store);

	for (scan = ddata->data->destinations; scan; scan = scan->next) {
		const char  *destination = scan->data;
		char        *utf8_name;
		GtkTreeIter  iter;
			
		utf8_name = g_filename_to_utf8 (destination, -1, NULL, NULL, NULL);
			
		gtk_list_store_append (ddata->list_store, &iter);
		gtk_list_store_set (ddata->list_store, &iter,
				    DESTINATION_NAME_COLUMN, utf8_name,
				    DESTINATION_ENTRY_COLUMN, scan,
				    -1);
		
		g_free (utf8_name);
	}
}


static void
show_destination_dialog_cb (GtkWidget  *widget,
			    DialogData *data)
{
	DestinationsDialogData   *ddata;
	GtkCellRenderer          *renderer;
	GtkTreeViewColumn        *column;

	ddata = g_new (DestinationsDialogData, 1);

	ddata->data = data;
	ddata->window = data->window;
	ddata->new_value = NULL;

	ddata->gui = glade_xml_new (GTHUMB_GLADEDIR "/" GLADE_EXPORTER_FILE, NULL, NULL);
        if (!ddata->gui) {
		g_free (ddata);
                g_warning ("Could not find " GLADE_EXPORTER_FILE "\n");
                return;
        }

	/* Get the widgets. */

	ddata->dialog = glade_xml_get_widget (ddata->gui, "location_dialog");
	ddata->wad_destinations_treeview = glade_xml_get_widget (ddata->gui, "wad_destinations_treeview");
	ddata->wad_new_button = glade_xml_get_widget (ddata->gui, "wad_new_button");
	ddata->wad_edit_button = glade_xml_get_widget (ddata->gui, "wad_edit_button");
	ddata->wad_remove_button = glade_xml_get_widget (ddata->gui, "wad_remove_button");
	ddata->wad_cancelbutton = glade_xml_get_widget (ddata->gui, "wad_cancelbutton");
	ddata->wad_okbutton = glade_xml_get_widget (ddata->gui, "wad_okbutton");

	/* Signals. */

	g_signal_connect (G_OBJECT (ddata->dialog), 
			  "destroy",
			  G_CALLBACK (destination_dialog_destroy_cb),
			  ddata);
	g_signal_connect_swapped (G_OBJECT (ddata->wad_cancelbutton), 
				  "clicked",
				  G_CALLBACK (gtk_widget_destroy),
				  G_OBJECT (ddata->dialog));
	g_signal_connect (G_OBJECT (ddata->wad_okbutton), 
			  "clicked",
			  G_CALLBACK (destination_dialog__ok_clicked),
			  ddata);
	g_signal_connect (G_OBJECT (ddata->wad_destinations_treeview),
			  "row_activated",
			  G_CALLBACK (destination_dialog__row_activated_cb),
			  ddata);

	g_signal_connect (G_OBJECT (ddata->wad_new_button),
			  "clicked",
			  G_CALLBACK (destination_dialog__new_clicked),
			  ddata);
	g_signal_connect (G_OBJECT (ddata->wad_edit_button),
			  "clicked",
			  G_CALLBACK (destination_dialog__edit_clicked),
			  ddata);
	g_signal_connect (G_OBJECT (ddata->wad_remove_button),
			  "clicked",
			  G_CALLBACK (destination_dialog__remove_clicked),
			  ddata);

	/* Set widgets data. */

	ddata->list_store = gtk_list_store_new (2, G_TYPE_STRING, G_TYPE_POINTER);
	gtk_tree_view_set_model (GTK_TREE_VIEW (ddata->wad_destinations_treeview), GTK_TREE_MODEL (ddata->list_store));
	g_object_unref (ddata->list_store);

	gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (ddata->wad_destinations_treeview), FALSE);
	gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (ddata->wad_destinations_treeview), FALSE);

	column = gtk_tree_view_column_new ();
	renderer = gtk_cell_renderer_text_new ();
        gtk_tree_view_column_pack_start (column, renderer, TRUE);
        gtk_tree_view_column_set_attributes (column, renderer,
                                             "text", DESTINATION_NAME_COLUMN,
                                             NULL);

        gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);        
	gtk_tree_view_column_set_sort_column_id (column, DESTINATION_NAME_COLUMN);
        gtk_tree_view_append_column (GTK_TREE_VIEW (ddata->wad_destinations_treeview), column);
	gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (ddata->list_store), DESTINATION_NAME_COLUMN, GTK_SORT_ASCENDING);

	destionation_dialog_update_destination_list (ddata);

	/* Run dialog. */

	gtk_widget_grab_focus (ddata->wad_destinations_treeview);

	gtk_window_set_transient_for (GTK_WINDOW (ddata->dialog), GTK_WINDOW (data->dialog));
	gtk_window_set_modal (GTK_WINDOW (ddata->dialog), TRUE);
	gtk_widget_show_all (ddata->dialog);
}




typedef struct {
	DestinationsDialogData *ddata;
	GThumbWindow       *window;

	GladeXML           *gui;
	GtkWidget          *dialog;

	GtkWidget          *new_location_dialog;
	GtkWidget          *wadd_optionmenu;
	GtkWidget          *wadd_site_entry;
	GtkWidget          *wadd_port_entry;
	GtkWidget          *wadd_path_combo_entry;
	GtkWidget          *wadd_user_entry;
	GtkWidget          *wadd_password_entry;
	GtkWidget          *wadd_cancelbutton;
	GtkWidget          *wadd_okbutton;

	GSList *destination_entry;
} EditDestinationDialogData;


/* called when the main dialog is closed. */
static void
edit_destination_dialog_destroy_cb (GtkWidget                 *widget, 
				    EditDestinationDialogData *eddata)
{
	g_object_unref (eddata->gui);
	g_free (eddata);
}


static void
edit_destination_dialog__ok_clicked (GtkWidget                 *widget, 
				     EditDestinationDialogData *eddata)
{
	const char  *protocol[3] = {"", "ssh://", "ftp://"};
	int          protocol_idx;
	char        *site, *port, *path, *user, *password, *location;
	GnomeVFSURI *uri;
	GnomeVFSURIHideOptions hide_opt = GNOME_VFS_URI_HIDE_NONE;

	protocol_idx = gtk_option_menu_get_history (GTK_OPTION_MENU (eddata->wadd_optionmenu));

	site = _gtk_entry_get_filename_text (GTK_ENTRY (eddata->wadd_site_entry));
	port = _gtk_entry_get_filename_text (GTK_ENTRY (eddata->wadd_port_entry));
	path = _gtk_entry_get_filename_text (GTK_ENTRY (eddata->wadd_path_combo_entry));
	user = _gtk_entry_get_locale_text (GTK_ENTRY (eddata->wadd_user_entry));
	password = _gtk_entry_get_locale_text (GTK_ENTRY (eddata->wadd_password_entry));

	location = g_strdup_printf ("%s%s", protocol[protocol_idx], path);
	uri = gnome_vfs_uri_new (location);
	g_free (location);

	if (protocol_idx == 0)
		hide_opt |= GNOME_VFS_URI_HIDE_TOPLEVEL_METHOD;
	if (! str_void (site))
		gnome_vfs_uri_set_host_name (uri, site);
	else
		hide_opt |= GNOME_VFS_URI_HIDE_HOST_NAME;
	if (! str_void (port))
		gnome_vfs_uri_set_host_port (uri, atoi (port));
	else
		hide_opt |= GNOME_VFS_URI_HIDE_HOST_PORT;
	if (! str_void (user))
		gnome_vfs_uri_set_user_name (uri, user);
	else
		hide_opt |= GNOME_VFS_URI_HIDE_USER_NAME;
	if (! str_void (password))
		gnome_vfs_uri_set_password (uri, password);
	else
		hide_opt |= GNOME_VFS_URI_HIDE_PASSWORD;

	g_free (site);
	g_free (port);
	g_free (path);
	g_free (user);
	g_free (password);

	location = gnome_vfs_uri_to_string (uri, hide_opt);
	gnome_vfs_uri_unref (uri);

	if (eddata->destination_entry == NULL)
		eddata->ddata->data->destinations = g_slist_append (eddata->ddata->data->destinations, location);
	else {
		g_free (eddata->destination_entry->data);
		eddata->destination_entry->data = location;
	}

	destionation_dialog_update_destination_list (eddata->ddata);
	gtk_widget_destroy (eddata->dialog);
}


static void
edit_destination_dialog_update_sensitivity (EditDestinationDialogData *eddata)
{
	gboolean local_host;

	local_host = gtk_option_menu_get_history (GTK_OPTION_MENU (eddata->wadd_optionmenu)) == 0;

	gtk_widget_set_sensitive (eddata->wadd_site_entry, !local_host);
	gtk_widget_set_sensitive (eddata->wadd_port_entry, !local_host);
	gtk_widget_set_sensitive (eddata->wadd_user_entry, !local_host);
	gtk_widget_set_sensitive (eddata->wadd_password_entry, !local_host);
}


static void
protocol_changed_cb (GtkOptionMenu             *opt_menu,
		     EditDestinationDialogData *eddata)
{
	edit_destination_dialog_update_sensitivity (eddata);
}


static void
show_edit_destination_dialog (DestinationsDialogData *ddata,
			      GSList                 *destination_entry)
{
	EditDestinationDialogData   *eddata;

	eddata = g_new (EditDestinationDialogData, 1);

	eddata->ddata = ddata;
	eddata->window = ddata->window;
	eddata->destination_entry = destination_entry;

	eddata->gui = glade_xml_new (GTHUMB_GLADEDIR "/" GLADE_EXPORTER_FILE, NULL, NULL);
        if (!eddata->gui) {
		g_free (eddata);
                g_warning ("Could not find " GLADE_EXPORTER_FILE "\n");
                return;
        }

	/* Get the widgets. */

	eddata->dialog = glade_xml_get_widget (eddata->gui, "new_location_dialog");
	eddata->wadd_optionmenu = glade_xml_get_widget (eddata->gui, "wadd_optionmenu");
	eddata->wadd_site_entry = glade_xml_get_widget (eddata->gui, "wadd_site_entry");
	eddata->wadd_port_entry = glade_xml_get_widget (eddata->gui, "wadd_port_entry");
	eddata->wadd_path_combo_entry = glade_xml_get_widget (eddata->gui, "wadd_path_combo_entry");
	eddata->wadd_user_entry = glade_xml_get_widget (eddata->gui, "wadd_user_entry");
	eddata->wadd_password_entry = glade_xml_get_widget (eddata->gui, "wadd_password_entry");
	eddata->wadd_cancelbutton = glade_xml_get_widget (eddata->gui, "wadd_cancelbutton");
	eddata->wadd_okbutton = glade_xml_get_widget (eddata->gui, "wadd_okbutton");

	/* Signals. */

	g_signal_connect (G_OBJECT (eddata->dialog), 
			  "destroy",
			  G_CALLBACK (edit_destination_dialog_destroy_cb),
			  eddata);
	g_signal_connect_swapped (G_OBJECT (eddata->wadd_cancelbutton), 
				  "clicked",
				  G_CALLBACK (gtk_widget_destroy),
				  G_OBJECT (eddata->dialog));
	g_signal_connect (G_OBJECT (eddata->wadd_okbutton), 
			  "clicked",
			  G_CALLBACK (edit_destination_dialog__ok_clicked),
			  eddata);
	g_signal_connect (G_OBJECT (eddata->wadd_optionmenu),
			  "changed",
			  G_CALLBACK (protocol_changed_cb),
			  eddata);

	/* Set widgets data. */
	
	if (eddata->destination_entry != NULL) {
		GnomeVFSURI *uri = gnome_vfs_uri_new (eddata->destination_entry->data);

		if (uri != NULL) {
			const char  *scheme;
			int          idx = 0;
			char        *port;
			
			scheme = gnome_vfs_uri_get_scheme (uri);
			if ((scheme != NULL) && (*scheme != 0)) {
				if (strcmp (scheme, "ssh") == 0)
					idx = 1;
				else if (strcmp (scheme, "ftp") == 0)
					idx = 2;
			}
			gtk_option_menu_set_history (GTK_OPTION_MENU (eddata->wadd_optionmenu), idx);
			
			port = g_strdup_printf ("%d", gnome_vfs_uri_get_host_port (uri));
			_gtk_entry_set_filename_text (GTK_ENTRY (eddata->wadd_port_entry), port);
			g_free (port);
			
			_gtk_entry_set_filename_text (GTK_ENTRY (eddata->wadd_site_entry), gnome_vfs_uri_get_host_name (uri));
			_gtk_entry_set_filename_text (GTK_ENTRY (eddata->wadd_path_combo_entry), gnome_vfs_uri_get_path (uri));
			_gtk_entry_set_filename_text (GTK_ENTRY (eddata->wadd_user_entry), gnome_vfs_uri_get_user_name (uri));
			_gtk_entry_set_filename_text (GTK_ENTRY (eddata->wadd_password_entry), gnome_vfs_uri_get_password (uri));
			gnome_vfs_uri_unref (uri);
		}

	} else
		_gtk_entry_set_filename_text (GTK_ENTRY (eddata->wadd_path_combo_entry), g_get_home_dir());

	edit_destination_dialog_update_sensitivity (eddata);

	/* Run dialog. */

	gtk_widget_grab_focus (eddata->wadd_path_combo_entry);

	gtk_window_set_transient_for (GTK_WINDOW (eddata->dialog), GTK_WINDOW (ddata->dialog));
	gtk_window_set_modal (GTK_WINDOW (eddata->dialog), TRUE);
	gtk_widget_show_all (eddata->dialog);
}




typedef struct {
	DialogData         *data;
	GThumbWindow       *window;

	GladeXML           *gui;
	GtkWidget          *dialog;

	GtkWidget          *wat_dialog;
	GtkWidget          *wat_theme_treeview;
	GtkWidget          *wat_ok_button;
	GtkWidget          *wat_cancel_button;
	GtkWidget          *wat_install_button;
	GtkWidget          *wat_go_to_folder_button;
	GtkWidget          *wat_thumbnail_caption_button;
	GtkWidget          *wat_image_caption_button;

	GtkListStore       *list_store;
} ThemeDialogData;


enum {
	THEME_NAME_COLUMN,
	NUM_OF_COLUMNS
};


/* called when the main dialog is closed. */
static void
theme_dialog_destroy_cb (GtkWidget       *widget, 
			 ThemeDialogData *tdata)
{
	g_object_unref (tdata->gui);
	g_free (tdata);
}


static void
theme_dialog__ok_clicked (GtkWidget       *widget, 
			  ThemeDialogData *tdata)
{
	GtkTreeSelection *selection;
	gboolean          theme_selected;
	GtkTreeIter       iter;

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (tdata->wat_theme_treeview));
	theme_selected = gtk_tree_selection_get_selected (selection, NULL, &iter);

	if (theme_selected) {
		char *utf8_name;

		gtk_tree_model_get (GTK_TREE_MODEL (tdata->list_store),
				    &iter,
				    THEME_NAME_COLUMN, &utf8_name,
				    -1);
		gtk_entry_set_text (GTK_ENTRY (tdata->data->wa_theme_combo_entry), utf8_name);
		g_free (utf8_name);
	}

	gtk_widget_destroy (tdata->dialog);
}


static void
add_theme_dir (ThemeDialogData *tdata,
	       char            *theme_dir)
{
	GnomeVFSResult  result;
	GList          *file_list = NULL;
	GList          *scan;

	debug (DEBUG_INFO, "theme dir: %s", theme_dir);

	if (theme_dir != NULL)
		result = gnome_vfs_directory_list_load (&file_list, 
							theme_dir, 
							GNOME_VFS_FILE_INFO_DEFAULT);
	else
		result = GNOME_VFS_ERROR_NOT_A_DIRECTORY;
	
	if (result == GNOME_VFS_OK) 
		for (scan = file_list; scan; scan = scan->next) {
			GnomeVFSFileInfo *info = scan->data;
			char             *utf8_name;
			GtkTreeIter       iter;

			if (info->type != GNOME_VFS_FILE_TYPE_DIRECTORY)
				continue;

			if ((strcmp (info->name, ".") == 0)
			    || (strcmp (info->name, "..") == 0))
				continue;

			utf8_name = g_filename_to_utf8 (info->name, -1, NULL, NULL, NULL);
			
			gtk_list_store_append (tdata->list_store, &iter);
			gtk_list_store_set (tdata->list_store, &iter,
					    THEME_NAME_COLUMN, utf8_name,
					    -1);
			
			g_free (utf8_name);
		}

	if (file_list != NULL)
		gnome_vfs_file_info_list_free (file_list);
}


static void
load_themes (ThemeDialogData *tdata)
{
	char *theme_dir;

	theme_dir = g_build_path (G_DIR_SEPARATOR_S,
				  g_get_home_dir (),
				  ".gnome2",
				  "gthumb/albumthemes",
				  NULL);
	add_theme_dir (tdata, theme_dir);
	g_free (theme_dir);

	theme_dir = g_build_path (G_DIR_SEPARATOR_S,
				  GTHUMB_DATADIR,
				  "gthumb/albumthemes",
				  NULL);
	add_theme_dir (tdata, theme_dir);
	g_free (theme_dir);	
}


static void
theme_dialog__row_activated_cb (GtkTreeView       *tree_view,
				GtkTreePath       *path,
				GtkTreeViewColumn *column,
				ThemeDialogData   *tdata)
{
	theme_dialog__ok_clicked (NULL, tdata);
}


static void
ensure_local_theme_dir_exists (void)
{
	char *theme_dir;

	theme_dir = g_build_path (G_DIR_SEPARATOR_S,
				  g_get_home_dir (),
				  ".gnome2",
				  "gthumb/albumthemes",
				  NULL);
	mkdir (theme_dir, 0700);
	g_free (theme_dir);
}


static void
install_theme__ok_cb (GObject  *object,
		      gpointer  data)
{
	ThemeDialogData  *tdata;
	GtkWidget        *file_sel = data;
	char             *theme_archive;
	char             *command_line;
	GError           *err = NULL;

	tdata = g_object_get_data (G_OBJECT (file_sel), "theme_dialog_data");
	theme_archive = g_strdup (gtk_file_selection_get_filename (GTK_FILE_SELECTION (file_sel)));
	gtk_widget_destroy (file_sel);

	if (theme_archive == NULL)
		return;

	/**/

	ensure_local_theme_dir_exists ();
            
	if (file_extension_is (theme_archive, ".tar.gz")
	    || file_extension_is (theme_archive, ".tgz"))
		command_line = g_strdup_printf ("tar -C %s%s -zxf %s",
						g_get_home_dir (),
						"/.gnome2/gthumb/albumthemes",
						theme_archive);
	
	else if (file_extension_is (theme_archive, ".tar.bz2"))
		command_line = g_strdup_printf ("tar -C %s%s -xf %s --use-compress-program bzip2",
						g_get_home_dir (),
						"/.gnome2/gthumb/albumthemes",
						theme_archive);
	
	if ((command_line != NULL) 
	    && ! g_spawn_command_line_sync (command_line, NULL, NULL, NULL, &err)
	    && (err != NULL))
		_gtk_error_dialog_from_gerror_run (NULL, &err);
	
	g_free (command_line);
	g_free (theme_archive);

	/**/

	gtk_list_store_clear (tdata->list_store);
	load_themes (tdata);
}


static void
theme_dialog__install_theme_clicked (GtkWidget       *widget, 
				     ThemeDialogData *tdata)
{
	GtkWidget *file_sel;
	char      *home;

	file_sel = gtk_file_selection_new (_("Select Album Theme"));
	g_object_set_data (G_OBJECT (file_sel), "theme_dialog_data", tdata);

	home = g_strconcat (g_get_home_dir (), "/", NULL);
	gtk_file_selection_set_filename (GTK_FILE_SELECTION (file_sel), home);
	g_free (home);

	g_signal_connect (G_OBJECT (GTK_FILE_SELECTION (file_sel)->ok_button),
			  "clicked",
			  G_CALLBACK (install_theme__ok_cb),
			  file_sel);

	g_signal_connect_swapped (G_OBJECT (GTK_FILE_SELECTION (file_sel)->cancel_button),
				  "clicked",
				  G_CALLBACK (gtk_widget_destroy),
				  G_OBJECT (file_sel));

	gtk_window_set_transient_for (GTK_WINDOW (file_sel),
				      GTK_WINDOW (tdata->dialog));
	gtk_window_set_modal (GTK_WINDOW (file_sel), TRUE);
	gtk_widget_show (file_sel);
}


static void
theme_dialog__go_to_folder_clicked (GtkWidget       *widget, 
				    ThemeDialogData *tdata)
{
	char         *path;
        GnomeVFSURI  *uri;
	GError       *err;

	path = g_strdup_printf ("file://%s/.gnome2/gthumb/albumthemes", 
			       g_get_home_dir ());

	uri = gnome_vfs_uri_new (path);
	if (! gnome_vfs_uri_exists (uri)) 
		gnome_vfs_make_directory_for_uri (uri, 0775);
        gnome_vfs_uri_unref (uri);

	if (! gnome_url_show (path, &err))
		_gtk_error_dialog_from_gerror_run (GTK_WINDOW (tdata->dialog),
						   &err);
        g_free (path);
}


static void show_thumbnail_caption_dialog_cb (GtkWidget  *widget,  ThemeDialogData *data);
static void show_image_caption_dialog_cb     (GtkWidget  *widget,  ThemeDialogData *data);


static void
show_album_theme_cb (GtkWidget  *widget,
		     DialogData *data)
{
	ThemeDialogData   *tdata;
	GtkCellRenderer   *renderer;
	GtkTreeViewColumn *column;

	tdata = g_new (ThemeDialogData, 1);

	tdata->data = data;
	tdata->window = data->window;

	tdata->gui = glade_xml_new (GTHUMB_GLADEDIR "/" GLADE_EXPORTER_FILE, NULL, NULL);
        if (!tdata->gui) {
		g_free (tdata);
                g_warning ("Could not find " GLADE_EXPORTER_FILE "\n");
                return;
        }

	/* Get the widgets. */

	tdata->dialog = glade_xml_get_widget (tdata->gui, "web_album_theme_dialog");
	tdata->wat_theme_treeview = glade_xml_get_widget (tdata->gui, "wat_theme_treeview");
	tdata->wat_ok_button = glade_xml_get_widget (tdata->gui, "wat_ok_button");
	tdata->wat_cancel_button = glade_xml_get_widget (tdata->gui, "wat_cancel_button");
	tdata->wat_install_button = glade_xml_get_widget (tdata->gui, "wat_install_button");
	tdata->wat_go_to_folder_button = glade_xml_get_widget (tdata->gui, "wat_go_to_folder_button");
	tdata->wat_thumbnail_caption_button = glade_xml_get_widget (tdata->gui, "wat_thumbnail_caption_button");
	tdata->wat_image_caption_button = glade_xml_get_widget (tdata->gui, "wat_image_caption_button");

	/* Signals. */

	g_signal_connect (G_OBJECT (tdata->dialog), 
			  "destroy",
			  G_CALLBACK (theme_dialog_destroy_cb),
			  tdata);
	g_signal_connect_swapped (G_OBJECT (tdata->wat_cancel_button), 
				  "clicked",
				  G_CALLBACK (gtk_widget_destroy),
				  G_OBJECT (tdata->dialog));
	g_signal_connect (G_OBJECT (tdata->wat_ok_button), 
			  "clicked",
			  G_CALLBACK (theme_dialog__ok_clicked),
			  tdata);
	g_signal_connect (G_OBJECT (tdata->wat_theme_treeview),
			  "row_activated",
			  G_CALLBACK (theme_dialog__row_activated_cb),
			  tdata);

	g_signal_connect (G_OBJECT (tdata->wat_install_button),
			  "clicked",
			  G_CALLBACK (theme_dialog__install_theme_clicked),
			  tdata);
	g_signal_connect (G_OBJECT (tdata->wat_go_to_folder_button), 
			  "clicked",
			  G_CALLBACK (theme_dialog__go_to_folder_clicked),
			  tdata);
	g_signal_connect (G_OBJECT (tdata->wat_thumbnail_caption_button), 
			  "clicked",
			  G_CALLBACK (show_thumbnail_caption_dialog_cb),
			  tdata);
	g_signal_connect (G_OBJECT (tdata->wat_image_caption_button), 
			  "clicked",
			  G_CALLBACK (show_image_caption_dialog_cb),
			  tdata);

	/* Set widgets data. */

	tdata->list_store = gtk_list_store_new (NUM_OF_COLUMNS, G_TYPE_STRING);
	gtk_tree_view_set_model (GTK_TREE_VIEW (tdata->wat_theme_treeview), GTK_TREE_MODEL (tdata->list_store));
	g_object_unref (tdata->list_store);

	gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (tdata->wat_theme_treeview), FALSE);
	gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (tdata->wat_theme_treeview), FALSE);

	column = gtk_tree_view_column_new ();
	renderer = gtk_cell_renderer_text_new ();
        gtk_tree_view_column_pack_start (column, renderer, TRUE);
        gtk_tree_view_column_set_attributes (column, renderer,
                                             "text", THEME_NAME_COLUMN,
                                             NULL);

        gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);        
	gtk_tree_view_column_set_sort_column_id (column, THEME_NAME_COLUMN);
        gtk_tree_view_append_column (GTK_TREE_VIEW (tdata->wat_theme_treeview), column);
	gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (tdata->list_store), THEME_NAME_COLUMN, GTK_SORT_ASCENDING);

	load_themes (tdata);

	/* Run dialog. */

	gtk_widget_grab_focus (tdata->wat_theme_treeview); 

	gtk_window_set_transient_for (GTK_WINDOW (tdata->dialog), GTK_WINDOW (data->dialog));
	gtk_window_set_modal (GTK_WINDOW (tdata->dialog), FALSE);
	gtk_widget_show_all (tdata->dialog);
}




typedef struct {
	DialogData         *data;
	GThumbWindow       *window;

	GladeXML           *gui;
	GtkWidget          *dialog;

	GtkWidget          *c_comment_checkbutton;
	GtkWidget          *c_imagedim_checkbutton;
	GtkWidget          *c_filename_checkbutton;
	GtkWidget          *c_filesize_checkbutton;

	GtkWidget          *c_exif_date_time_checkbutton;
	GtkWidget          *c_exif_exposure_time_checkbutton;
	GtkWidget          *c_exif_exposure_mode_checkbutton;
	GtkWidget          *c_exif_flash_checkbutton;
	GtkWidget          *c_exif_shutter_speed_checkbutton;
	GtkWidget          *c_exif_aperture_value_checkbutton;
	GtkWidget          *c_exif_focal_length_checkbutton;
	GtkWidget          *c_exif_camera_model_checkbutton;

	gboolean            thumbnail_caption;
} CaptionDialogData;


/* called when the dialog is closed. */
static void
caption_dialog_destroy_cb (GtkWidget         *widget, 
			   CaptionDialogData *cdata)
{
	g_object_unref (cdata->gui);
	g_free (cdata);
}


static void
caption_dialog__ok_clicked (GtkWidget         *widget, 
			    CaptionDialogData *cdata)
{
	const char       *gconf_key;
	GthCaptionFields  caption = 0;

	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (cdata->c_comment_checkbutton)))
		caption |= GTH_CAPTION_COMMENT;
	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (cdata->c_imagedim_checkbutton)))
		caption |= GTH_CAPTION_IMAGE_DIM;
	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (cdata->c_filename_checkbutton)))
		caption |= GTH_CAPTION_FILE_NAME;
	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (cdata->c_filesize_checkbutton)))
		caption |= GTH_CAPTION_FILE_SIZE;
	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (cdata->c_exif_date_time_checkbutton)))
		caption |= GTH_CAPTION_EXIF_DATE_TIME;
	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (cdata->c_exif_exposure_time_checkbutton)))
		caption |= GTH_CAPTION_EXIF_EXPOSURE_TIME;
	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (cdata->c_exif_exposure_mode_checkbutton)))
		caption |= GTH_CAPTION_EXIF_EXPOSURE_MODE;
	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (cdata->c_exif_flash_checkbutton)))
		caption |= GTH_CAPTION_EXIF_FLASH;
	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (cdata->c_exif_shutter_speed_checkbutton)))
		caption |= GTH_CAPTION_EXIF_SHUTTER_SPEED;
	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (cdata->c_exif_aperture_value_checkbutton)))
		caption |= GTH_CAPTION_EXIF_APERTURE_VALUE;
	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (cdata->c_exif_focal_length_checkbutton)))
		caption |= GTH_CAPTION_EXIF_FOCAL_LENGTH;
	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (cdata->c_exif_camera_model_checkbutton)))
		caption |= GTH_CAPTION_EXIF_CAMERA_MODEL;

	if (cdata->thumbnail_caption) {
		catalog_web_exporter_set_index_caption (cdata->data->exporter, caption);
		gconf_key = PREF_WEB_ALBUM_INDEX_CAPTION;
	} else {
		catalog_web_exporter_set_image_caption (cdata->data->exporter, caption);
		gconf_key = PREF_WEB_ALBUM_IMAGE_CAPTION;
	}

	eel_gconf_set_integer (gconf_key, caption);

	gtk_widget_destroy (cdata->dialog);
}


static void
show_caption_dialog_cb (GtkWidget       *widget,
			ThemeDialogData *tdata,
			gboolean         thumbnail_caption)
{
	CaptionDialogData *cdata;
	GtkWidget         *ok_button;
	GtkWidget         *cancel_button;
#ifdef HAVE_LIBEXIF
	const char        *gconf_key;
	GthCaptionFields   caption = 0;
#else
	GtkWidget         *vbox;
#endif
	cdata = g_new (CaptionDialogData, 1);

	cdata->data = tdata->data;
	cdata->window = tdata->window;
	cdata->thumbnail_caption = thumbnail_caption;

	cdata->gui = glade_xml_new (GTHUMB_GLADEDIR "/" GLADE_EXPORTER_FILE, NULL, NULL);
        if (!cdata->gui) {
		g_free (cdata);
                g_warning ("Could not find " GLADE_EXPORTER_FILE "\n");
                return;
        }

	/* Get the widgets. */

	cdata->dialog = glade_xml_get_widget (cdata->gui, "caption_dialog");
	cdata->c_comment_checkbutton = glade_xml_get_widget (cdata->gui, "c_comment_checkbutton");
	cdata->c_imagedim_checkbutton = glade_xml_get_widget (cdata->gui, "c_imagedim_checkbutton");
	cdata->c_filename_checkbutton = glade_xml_get_widget (cdata->gui, "c_filename_checkbutton");
	cdata->c_filesize_checkbutton = glade_xml_get_widget (cdata->gui, "c_filesize_checkbutton");
	cdata->c_exif_date_time_checkbutton = glade_xml_get_widget (cdata->gui, "c_exif_date_time_checkbutton");
	cdata->c_exif_exposure_time_checkbutton = glade_xml_get_widget (cdata->gui, "c_exif_exposure_time_checkbutton");
	cdata->c_exif_exposure_mode_checkbutton = glade_xml_get_widget (cdata->gui, "c_exif_exposure_mode_checkbutton");
	cdata->c_exif_flash_checkbutton = glade_xml_get_widget (cdata->gui, "c_exif_flash_checkbutton");
	cdata->c_exif_shutter_speed_checkbutton = glade_xml_get_widget (cdata->gui, "c_exif_shutter_speed_checkbutton");
	cdata->c_exif_aperture_value_checkbutton = glade_xml_get_widget (cdata->gui, "c_exif_aperture_value_checkbutton");
	cdata->c_exif_focal_length_checkbutton = glade_xml_get_widget (cdata->gui, "c_exif_focal_length_checkbutton");
	cdata->c_exif_camera_model_checkbutton = glade_xml_get_widget (cdata->gui, "c_exif_camera_model_checkbutton");

	ok_button = glade_xml_get_widget (cdata->gui, "c_okbutton");
	cancel_button = glade_xml_get_widget (cdata->gui, "c_cancelbutton");

	/* Signals. */

	g_signal_connect (G_OBJECT (cdata->dialog), 
			  "destroy",
			  G_CALLBACK (caption_dialog_destroy_cb),
			  cdata);
	g_signal_connect_swapped (G_OBJECT (cancel_button), 
				  "clicked",
				  G_CALLBACK (gtk_widget_destroy),
				  G_OBJECT (cdata->dialog));
	g_signal_connect (G_OBJECT (ok_button), 
			  "clicked",
			  G_CALLBACK (caption_dialog__ok_clicked),
			  cdata);

	/* Set widgets data. */

#ifdef HAVE_LIBEXIF

	if (cdata->thumbnail_caption) 
		gconf_key = PREF_WEB_ALBUM_INDEX_CAPTION;
	 else 
		gconf_key = PREF_WEB_ALBUM_IMAGE_CAPTION;
	caption = eel_gconf_get_integer (gconf_key, 0);

	if (caption & GTH_CAPTION_COMMENT)
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (cdata->c_comment_checkbutton), TRUE);
	if (caption & GTH_CAPTION_IMAGE_DIM)
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (cdata->c_imagedim_checkbutton), TRUE);
	if (caption & GTH_CAPTION_FILE_NAME)
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (cdata->c_filename_checkbutton), TRUE);
	if (caption & GTH_CAPTION_FILE_SIZE)
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (cdata->c_filesize_checkbutton), TRUE);
	if (caption & GTH_CAPTION_EXIF_DATE_TIME)
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (cdata->c_exif_date_time_checkbutton), TRUE);
	if (caption & GTH_CAPTION_EXIF_EXPOSURE_TIME)
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (cdata->c_exif_exposure_time_checkbutton), TRUE);
	if (caption & GTH_CAPTION_EXIF_EXPOSURE_MODE)
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (cdata->c_exif_exposure_mode_checkbutton), TRUE);
	if (caption & GTH_CAPTION_EXIF_FLASH)
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (cdata->c_exif_flash_checkbutton), TRUE);
	if (caption & GTH_CAPTION_EXIF_SHUTTER_SPEED)
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (cdata->c_exif_shutter_speed_checkbutton), TRUE);
	if (caption & GTH_CAPTION_EXIF_APERTURE_VALUE)
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (cdata->c_exif_aperture_value_checkbutton), TRUE);
	if (caption & GTH_CAPTION_EXIF_FOCAL_LENGTH)
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (cdata->c_exif_focal_length_checkbutton), TRUE);
	if (caption & GTH_CAPTION_EXIF_CAMERA_MODEL)
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (cdata->c_exif_camera_model_checkbutton), TRUE);

#else /* ! HAVE_LIBEXIF */
	vbox = glade_xml_get_widget (cdata->gui, "c_exif_data_vbox");
	gtk_widget_hide (vbox);
#endif /* ! HAVE_LIBEXIF */

	/* Run dialog. */

	gtk_window_set_transient_for (GTK_WINDOW (cdata->dialog), GTK_WINDOW (tdata->dialog));
	gtk_window_set_modal (GTK_WINDOW (cdata->dialog), TRUE);
	gtk_widget_show (cdata->dialog);
}


static void
show_thumbnail_caption_dialog_cb (GtkWidget       *widget,
				  ThemeDialogData *data)
{
	show_caption_dialog_cb (widget, data, TRUE);
}


static void
show_image_caption_dialog_cb (GtkWidget       *widget,
			      ThemeDialogData *data)
{
	show_caption_dialog_cb (widget, data, FALSE);
}
