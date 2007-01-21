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

#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <libgnome/gnome-url.h>
#include <libgnome/gnome-help.h>
#include <libgnomevfs/gnome-vfs-directory.h>
#include <libgnomevfs/gnome-vfs-ops.h>
#include <libgnomevfs/gnome-vfs-utils.h>
#include <glade/glade.h>

#include "catalog-web-exporter.h"
#include "dlg-file-utils.h"
#include "file-utils.h"
#include "gtk-utils.h"
#include "gth-file-view.h"
#include "main.h"
#include "pixbuf-utils.h"
#include "gconf-utils.h"
#include "gth-browser.h"
#include "gth-utils.h"
#include "glib-utils.h"

static int           sort_method_to_idx[] = { -1, 0, 1, 2, 3, 4, 5 };
static GthSortMethod idx_to_sort_method[] = { GTH_SORT_METHOD_BY_NAME, 
					      GTH_SORT_METHOD_BY_PATH, 
					      GTH_SORT_METHOD_BY_SIZE, 
					      GTH_SORT_METHOD_BY_TIME,
					      GTH_SORT_METHOD_BY_EXIF_DATE,
					      GTH_SORT_METHOD_BY_COMMENT,
					      GTH_SORT_METHOD_MANUAL};
static int           idx_to_resize_width[] = { 320, 320, 640, 640, 800, 800, 1024, 1024, 1280, 1280 };
static int           idx_to_resize_height[] = { 200, 320, 480, 640, 600, 800, 768, 1024, 960, 1280 };


#define str_void(x) (((x) == NULL) || (*(x) == 0))
#define GLADE_EXPORTER_FILE "gthumb_web_exporter.glade"
#define MAX_PREVIEW_SIZE 220
#define DEFAULT_ALBUM_THEME "Wiki"

typedef struct {
	GthBrowser         *browser;

	GladeXML           *gui;
	GtkWidget          *dialog;

	GtkWidget          *progress_dialog;
	GtkWidget          *progress_progressbar;
	GtkWidget          *progress_info;
	GtkWidget          *progress_cancel;

	GtkWidget          *btn_ok;

	GtkWidget          *wa_destination_filechooserbutton;
	GtkWidget          *wa_index_file_entry;
	GtkWidget          *wa_copy_images_checkbutton;
	GtkWidget          *wa_resize_images_checkbutton;
	GtkWidget          *wa_resize_images_optionmenu;
	GtkWidget          *wa_resize_images_hbox;
	GtkWidget          *wa_resize_images_options_hbox;

	GtkWidget          *wa_rows_spinbutton;
	GtkWidget          *wa_cols_spinbutton;
	GtkWidget          *wa_single_index_checkbutton;
	GtkWidget          *wa_rows_hbox;
	GtkWidget          *wa_cols_hbox;
	GtkWidget          *wa_sort_images_combobox;
	GtkWidget          *wa_reverse_order_checkbutton;

	GtkWidget          *wa_header_entry;
	GtkWidget          *wa_footer_entry;
	GtkWidget          *wa_theme_combo_entry;
	GtkWidget          *wa_theme_combo;
	GtkWidget          *wa_select_theme_button;

	/**/

	CatalogWebExporter *exporter;
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


/* called when the "help" button is clicked. */
static void
help_cb (GtkWidget  *widget,
         DialogData *data)
{
	gthumb_display_help (GTK_WINDOW (data->dialog), "gthumb-web-album");
}


static void
export (GtkWidget  *widget,
	DialogData *data)
{
	CatalogWebExporter *exporter = data->exporter;
	char               *location;
	char               *esc_path, *path;
	char               *theme, *index_file;
	const char         *header;
	const char         *footer;

	/* Save options. */

	esc_path = gtk_file_chooser_get_uri (GTK_FILE_CHOOSER (data->wa_destination_filechooserbutton));
	path = gnome_vfs_unescape_string (esc_path, "");
	location = remove_ending_separator (path);
	g_free (path);
	g_free (esc_path);

	eel_gconf_set_path (PREF_WEB_ALBUM_DESTINATION, location);

	index_file = _gtk_entry_get_filename_text (GTK_ENTRY (data->wa_index_file_entry));
	eel_gconf_set_string (PREF_WEB_ALBUM_INDEX_FILE, index_file);

	eel_gconf_set_boolean (PREF_WEB_ALBUM_COPY_IMAGES, gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (data->wa_copy_images_checkbutton)));

	eel_gconf_set_boolean (PREF_WEB_ALBUM_RESIZE_IMAGES, gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (data->wa_resize_images_checkbutton)));

	eel_gconf_set_integer (PREF_WEB_ALBUM_RESIZE_WIDTH, idx_to_resize_width[gtk_option_menu_get_history (GTK_OPTION_MENU (data->wa_resize_images_optionmenu))]);

	eel_gconf_set_integer (PREF_WEB_ALBUM_RESIZE_HEIGHT, idx_to_resize_height[gtk_option_menu_get_history (GTK_OPTION_MENU (data->wa_resize_images_optionmenu))]);

	eel_gconf_set_integer (PREF_WEB_ALBUM_ROWS, gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (data->wa_rows_spinbutton)));

	eel_gconf_set_integer (PREF_WEB_ALBUM_COLUMNS, gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (data->wa_cols_spinbutton)));

	eel_gconf_set_boolean (PREF_WEB_ALBUM_SINGLE_INDEX, gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (data->wa_single_index_checkbutton)));

	pref_set_web_album_sort_order (idx_to_sort_method [gtk_combo_box_get_active (GTK_COMBO_BOX (data->wa_sort_images_combobox))]);

	eel_gconf_set_boolean (PREF_WEB_ALBUM_REVERSE, gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (data->wa_reverse_order_checkbutton)));

	header = gtk_entry_get_text (GTK_ENTRY (data->wa_header_entry));
	eel_gconf_set_string (PREF_WEB_ALBUM_HEADER, header);

	footer = gtk_entry_get_text (GTK_ENTRY (data->wa_footer_entry));
	eel_gconf_set_string (PREF_WEB_ALBUM_FOOTER, footer);

	theme = _gtk_entry_get_filename_text (GTK_ENTRY (data->wa_theme_combo_entry));
	eel_gconf_set_string (PREF_WEB_ALBUM_THEME, theme);

	if (strcmp (theme, "") == 0) {
		g_free (location);
		return;
	}

	/**/

	if (! dlg_check_folder (GTH_WINDOW (data->browser), location)) {
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
	catalog_web_exporter_set_single_index (exporter, eel_gconf_get_boolean (PREF_WEB_ALBUM_SINGLE_INDEX, FALSE));
	
	catalog_web_exporter_set_sorted (exporter, pref_get_web_album_sort_order (), eel_gconf_get_boolean (PREF_WEB_ALBUM_REVERSE, FALSE));
	catalog_web_exporter_set_header (exporter, header);
	catalog_web_exporter_set_footer (exporter, footer);
	catalog_web_exporter_set_style (exporter, theme);

	g_free (location);
	g_free (theme);
	g_free (index_file);
	
	/* Export. */

	gtk_window_set_transient_for (GTK_WINDOW (data->progress_dialog),
				      GTK_WINDOW (data->browser));
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
get_idx_from_size (int width, int height)
{
	int idx;
	if (width == 320)
		idx = 0;
	else if (width == 640)
		idx = 1;
	else if (width == 800)
		idx = 2;
	else if (width == 1024)
		idx = 3;
	else if (width == 1280)
		idx = 4;
	else
		idx = 1;
	return 2 * idx + (width == height? 1 : 0);
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
single_index_toggled_cb (GtkToggleButton *button,
			 DialogData      *data)
{
	gtk_widget_set_sensitive (data->wa_rows_hbox, !gtk_toggle_button_get_active (button));
}

 

static gboolean
theme_present (const char *theme_name,
	       const char *theme_dir)
{
	GnomeVFSResult  result;
	GList          *file_list = NULL;
	GList          *scan;
	gboolean        found = FALSE;

	if (theme_name == NULL)
		return FALSE;

	if (theme_dir != NULL)
		result = gnome_vfs_directory_list_load (&file_list, 
							theme_dir, 
							GNOME_VFS_FILE_INFO_DEFAULT);
	else
		result = GNOME_VFS_ERROR_NOT_A_DIRECTORY;
	
	if (result == GNOME_VFS_OK) 
		for (scan = file_list; scan; scan = scan->next) {
			GnomeVFSFileInfo *info = scan->data;

			if (info->type != GNOME_VFS_FILE_TYPE_DIRECTORY)
				continue;

			if ((strcmp (info->name, ".") == 0)
			    || (strcmp (info->name, "..") == 0))
				continue;

			if (strcmp (info->name, theme_name) == 0) {
				found = TRUE;
				break;
			}
		}

	return found;
}


static char *
get_default_theme (void)
{
	char     *current_theme;
	char     *theme_dir;
	gboolean  found = FALSE;

	current_theme = eel_gconf_get_string (PREF_WEB_ALBUM_THEME, DEFAULT_ALBUM_THEME);

	theme_dir = g_build_path (G_DIR_SEPARATOR_S,
				  g_get_home_dir (),
				  ".gnome2",
				  "gthumb/albumthemes",
				  NULL);
	found = theme_present (current_theme, theme_dir);
	if (!found) {
		g_free (theme_dir);
		theme_dir = g_build_path (G_DIR_SEPARATOR_S,
					  GTHUMB_DATADIR,
					  "gthumb/albumthemes",
					  NULL);
		found = theme_present (current_theme, theme_dir);
	}

	g_free (theme_dir);

	if (! found) {
		g_free (current_theme);
		return g_strdup ("");
	}

	return current_theme;
}


/* create the main dialog. */
void
dlg_web_exporter (GthBrowser *browser)
{
	DialogData   *data;
	GtkWidget    *btn_cancel;
	GtkWidget    *btn_help;
	GList        *list;
	char         *svalue;
	char         *esc_uri;
	gboolean      reorderable;
	int           idx;

	data = g_new0 (DialogData, 1);

	data->browser = browser;
	
	list = gth_window_get_file_list_selection (GTH_WINDOW (browser));
	if (list == NULL) {
		g_warning ("No file selected.");
		g_free (data);
		return;
	}

	reorderable = gth_file_view_get_reorderable (gth_browser_get_file_view (browser));

	data->exporter = catalog_web_exporter_new (GTH_WINDOW (browser), list);
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
	data->wa_destination_filechooserbutton = glade_xml_get_widget (data->gui, "wa_destination_filechooserbutton");
	data->wa_index_file_entry = glade_xml_get_widget (data->gui, "wa_index_file_entry");
	data->wa_copy_images_checkbutton = glade_xml_get_widget (data->gui, "wa_copy_images_checkbutton");

	data->wa_resize_images_checkbutton = glade_xml_get_widget (data->gui, "wa_resize_images_checkbutton");
	data->wa_resize_images_optionmenu = glade_xml_get_widget (data->gui, "wa_resize_images_optionmenu");
	data->wa_resize_images_hbox = glade_xml_get_widget (data->gui, "wa_resize_images_hbox");
	data->wa_resize_images_options_hbox = glade_xml_get_widget (data->gui, "wa_resize_images_options_hbox");

	data->wa_rows_spinbutton = glade_xml_get_widget (data->gui, "wa_rows_spinbutton");
	data->wa_cols_spinbutton = glade_xml_get_widget (data->gui, "wa_cols_spinbutton");
	data->wa_single_index_checkbutton = glade_xml_get_widget (data->gui, "wa_single_index_checkbutton");
	data->wa_rows_hbox = glade_xml_get_widget (data->gui, "wa_rows_hbox");
	data->wa_cols_hbox = glade_xml_get_widget (data->gui, "wa_cols_hbox");
	data->wa_sort_images_combobox = glade_xml_get_widget (data->gui, "wa_sort_images_combobox");
	data->wa_reverse_order_checkbutton = glade_xml_get_widget (data->gui, "wa_reverse_order_checkbutton");

	data->wa_header_entry = glade_xml_get_widget (data->gui, "wa_header_entry");
	data->wa_footer_entry = glade_xml_get_widget (data->gui, "wa_footer_entry");
	data->wa_theme_combo_entry = glade_xml_get_widget (data->gui, "wa_theme_combo_entry");
	data->wa_select_theme_button = glade_xml_get_widget (data->gui, "wa_select_theme_button");

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

	gtk_option_menu_set_history (GTK_OPTION_MENU (data->wa_resize_images_optionmenu), get_idx_from_size (eel_gconf_get_integer (PREF_WEB_ALBUM_RESIZE_WIDTH, 640), eel_gconf_get_integer (PREF_WEB_ALBUM_RESIZE_HEIGHT, 480)));

	gtk_spin_button_set_value (GTK_SPIN_BUTTON (data->wa_rows_spinbutton), eel_gconf_get_integer (PREF_WEB_ALBUM_ROWS, 4));

	gtk_spin_button_set_value (GTK_SPIN_BUTTON (data->wa_cols_spinbutton), eel_gconf_get_integer (PREF_WEB_ALBUM_COLUMNS, 4));

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (data->wa_single_index_checkbutton), eel_gconf_get_boolean (PREF_WEB_ALBUM_SINGLE_INDEX, FALSE));

	gtk_widget_set_sensitive (data->wa_rows_hbox, !eel_gconf_get_boolean (PREF_WEB_ALBUM_SINGLE_INDEX, FALSE));

	/**/

	gtk_combo_box_append_text (GTK_COMBO_BOX (data->wa_sort_images_combobox),
				   _("by path"));
	gtk_combo_box_append_text (GTK_COMBO_BOX (data->wa_sort_images_combobox),
				   _("by size"));
	gtk_combo_box_append_text (GTK_COMBO_BOX (data->wa_sort_images_combobox),
				   _("by file modified time"));
	gtk_combo_box_append_text (GTK_COMBO_BOX (data->wa_sort_images_combobox),
				   _("by Exif DateTime tag"));
	gtk_combo_box_append_text (GTK_COMBO_BOX (data->wa_sort_images_combobox),
                                   _("by comment"));
	if (reorderable)
		gtk_combo_box_append_text (GTK_COMBO_BOX (data->wa_sort_images_combobox),
					   _("manual order"));

	idx = sort_method_to_idx [pref_get_web_album_sort_order ()];
	if (!reorderable && (sort_method_to_idx[GTH_SORT_METHOD_MANUAL] == idx))
		idx = sort_method_to_idx[GTH_SORT_METHOD_BY_NAME];
	gtk_combo_box_set_active (GTK_COMBO_BOX (data->wa_sort_images_combobox), idx);

	/**/

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (data->wa_reverse_order_checkbutton), eel_gconf_get_boolean (PREF_WEB_ALBUM_REVERSE, FALSE));

	svalue = eel_gconf_get_string (PREF_WEB_ALBUM_HEADER, "");
	gtk_entry_set_text (GTK_ENTRY (data->wa_header_entry), svalue);
	g_free (svalue);

	svalue = eel_gconf_get_string (PREF_WEB_ALBUM_FOOTER, "");
	gtk_entry_set_text (GTK_ENTRY (data->wa_footer_entry), svalue);
	g_free (svalue);

	svalue =get_default_theme();
	_gtk_entry_set_filename_text (GTK_ENTRY (data->wa_theme_combo_entry), svalue);
	g_free (svalue);

	catalog_web_exporter_set_index_caption (data->exporter, eel_gconf_get_integer (PREF_WEB_ALBUM_INDEX_CAPTION, 0));
	catalog_web_exporter_set_image_caption (data->exporter, eel_gconf_get_integer (PREF_WEB_ALBUM_IMAGE_CAPTION, 0));

	/**/

	svalue = eel_gconf_get_path (PREF_WEB_ALBUM_DESTINATION, NULL);
	if (svalue == NULL)
		esc_uri = gnome_vfs_escape_host_and_path_string (g_get_home_dir ());
	else
		esc_uri = gnome_vfs_escape_host_and_path_string (svalue);
	gtk_file_chooser_set_uri (GTK_FILE_CHOOSER (data->wa_destination_filechooserbutton), esc_uri);
	g_free (esc_uri);
	g_free (svalue);

	/* Signals. */

	g_signal_connect (G_OBJECT (data->dialog), 
			  "destroy",
			  G_CALLBACK (destroy_cb),
			  data);
        g_signal_connect (G_OBJECT (btn_help),
                          "clicked",
                          G_CALLBACK (help_cb),
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

	g_signal_connect (G_OBJECT (data->wa_single_index_checkbutton),
			  "toggled",
			  G_CALLBACK (single_index_toggled_cb),
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

	/* Run dialog. */

	gtk_window_set_transient_for (GTK_WINDOW (data->dialog), GTK_WINDOW (browser));
	gtk_window_set_modal (GTK_WINDOW (data->dialog), FALSE);
	gtk_widget_show_all (data->dialog);
}




typedef struct {
	DialogData         *data;
	GthBrowser         *browser;

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
	GtkWidget          *wat_preview_image;

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

			utf8_name = g_filename_display_name (info->name);
			
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
	char             *theme_dir;
	const char       *theme_name;
	GtkTreeModel     *model;
	GtkTreeSelection *selection;
	GtkTreeIter       iter;

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

	/* Select the current theme */

	model = GTK_TREE_MODEL (tdata->list_store);
	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (tdata->wat_theme_treeview));
	theme_name = gtk_entry_get_text (GTK_ENTRY (tdata->data->wa_theme_combo_entry));
	if (! gtk_tree_model_get_iter_first (model, &iter)) 
		return;

	do {
		char *utf8_name;
		gtk_tree_model_get (model, &iter, 
				    THEME_NAME_COLUMN, &utf8_name, 
				    -1);
		if (strcmp (utf8_name, theme_name) == 0)
			gtk_tree_selection_select_iter (selection, &iter);
	} while (gtk_tree_model_iter_next (model, &iter));
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
				  get_home_uri (),
				  ".gnome2",
				  "gthumb/albumthemes",
				  NULL);

	dir_make (theme_dir, 0700);

	g_free (theme_dir);
}


static void
install_theme__ok_cb (GtkDialog  *file_sel,
		      int         button_number,
		      gpointer    data)
{
	ThemeDialogData  *tdata;
	char             *theme_archive, *e_theme_archive;
	char             *command_line = NULL;
	GError           *err = NULL;

	tdata = g_object_get_data (G_OBJECT (file_sel), "theme_dialog_data");
	e_theme_archive = g_strdup (gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (file_sel)));
	gtk_widget_destroy (GTK_WIDGET (file_sel));

	if (e_theme_archive == NULL)
		return;

	/**/

	theme_archive = gnome_vfs_unescape_string (e_theme_archive, "");
	g_free (e_theme_archive);

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
install_theme_response_cb (GtkDialog  *file_sel,
			   int         button_number,
			   gpointer    userdata)
{
	if (button_number == GTK_RESPONSE_ACCEPT) 
		install_theme__ok_cb (file_sel, button_number, userdata);
	 else 
		gtk_widget_destroy (GTK_WIDGET (file_sel));
}


static void
theme_dialog__install_theme_clicked (GtkWidget       *widget, 
				     ThemeDialogData *tdata)
{
	GtkWidget *file_sel;

	file_sel = gtk_file_chooser_dialog_new (_("Select Album Theme"), NULL,
						GTK_FILE_CHOOSER_ACTION_OPEN,
						GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
						GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
						NULL);
	gtk_window_set_modal (GTK_WINDOW (file_sel), TRUE);
	g_object_set_data (G_OBJECT (file_sel), "theme_dialog_data", tdata);

	gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (file_sel), g_get_home_dir ());

	g_signal_connect (GTK_DIALOG (file_sel),
			  "response",
			  G_CALLBACK (install_theme_response_cb),
			  NULL);

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
	GError       *err;

	path = g_strdup_printf ("file://%s/.gnome2/gthumb/albumthemes", 
			       g_get_home_dir ());

	ensure_dir_exists (path, 0775);

	if (! gnome_url_show (path, &err))
		_gtk_error_dialog_from_gerror_run (GTK_WINDOW (tdata->dialog),
						   &err);
        g_free (path);
}


/* called when an item of the catalog list is selected. */
static void
theme_dialog__sel_changed_cb (GtkTreeSelection *selection,
			      gpointer p)
{
	ThemeDialogData  *tdata = p;
	gboolean          theme_selected;
	GtkTreeIter       iter;
	char             *utf8_name;
	char             *theme, *path;

	theme_selected = gtk_tree_selection_get_selected (selection, NULL, &iter);
	
	if (!theme_selected) 
		return;

	gtk_tree_model_get (GTK_TREE_MODEL (tdata->list_store),
			    &iter,
			    THEME_NAME_COLUMN, &utf8_name,
			    -1);

	theme = g_filename_from_utf8 (utf8_name, -1, 0, 0, 0);
	path = g_build_path (G_DIR_SEPARATOR_S,
			     g_get_home_dir (),
			     ".gnome2",
			     "gthumb/albumthemes",
			     theme,
			     NULL);

	if (!path_is_dir (path)) {
		g_free (path);
		path = g_build_path (G_DIR_SEPARATOR_S,
				     GTHUMB_DATADIR,
				     "gthumb/albumthemes",
				     theme,
				     NULL);
	}

	if (path_is_dir (path)) {
		char *filename = g_build_path (G_DIR_SEPARATOR_S,
					       path, 
					       "preview.png", 
					       NULL);
		if (path_is_file (filename)) {
			GdkPixbuf *image = gth_pixbuf_new_from_uri (filename, NULL);
			int        w = gdk_pixbuf_get_width (image);
			int        h = gdk_pixbuf_get_height (image);
			if (scale_keepping_ratio (&w, &h, MAX_PREVIEW_SIZE, MAX_PREVIEW_SIZE)) {
				GdkPixbuf *tmp = image;
				image = gdk_pixbuf_scale_simple (tmp, w, h, GDK_INTERP_BILINEAR);
				g_object_unref (tmp);
			}
			gtk_image_set_from_pixbuf (GTK_IMAGE (tdata->wat_preview_image), image);
			g_object_unref (image);

		} else
			gtk_image_set_from_stock (GTK_IMAGE (tdata->wat_preview_image), GTK_STOCK_MISSING_IMAGE, GTK_ICON_SIZE_BUTTON);
		g_free (filename);
	}

	g_free (utf8_name);
	g_free (path);
	g_free (theme);
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
	GtkTreeSelection  *selection;

	tdata = g_new (ThemeDialogData, 1);

	tdata->data = data;
	tdata->browser = data->browser;

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
	tdata->wat_preview_image = glade_xml_get_widget (tdata->gui, "wat_preview_image");

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

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (tdata->wat_theme_treeview));
        g_signal_connect (G_OBJECT (selection),
			  "changed",
			  G_CALLBACK (theme_dialog__sel_changed_cb),
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
	GthBrowser         *browser;

	GladeXML           *gui;
	GtkWidget          *dialog;

	GtkWidget          *c_comment_checkbutton;
	GtkWidget          *c_place_checkbutton;
	GtkWidget          *c_date_time_checkbutton;
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
	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (cdata->c_place_checkbutton)))
		caption |= GTH_CAPTION_PLACE;
	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (cdata->c_date_time_checkbutton)))
		caption |= GTH_CAPTION_DATE_TIME;
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
	const char        *gconf_key;
	GthCaptionFields   caption = 0;
	cdata = g_new (CaptionDialogData, 1);

	cdata->data = tdata->data;
	cdata->browser = tdata->browser;
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
	cdata->c_place_checkbutton = glade_xml_get_widget (cdata->gui, "c_place_checkbutton");
	cdata->c_date_time_checkbutton = glade_xml_get_widget (cdata->gui, "c_date_time_checkbutton");
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

	if (cdata->thumbnail_caption) 
		gconf_key = PREF_WEB_ALBUM_INDEX_CAPTION;
	 else 
		gconf_key = PREF_WEB_ALBUM_IMAGE_CAPTION;
	caption = eel_gconf_get_integer (gconf_key, 0);

	if (caption & GTH_CAPTION_COMMENT)
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (cdata->c_comment_checkbutton), TRUE);
	if (caption & GTH_CAPTION_PLACE)
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (cdata->c_place_checkbutton), TRUE);
	if (caption & GTH_CAPTION_DATE_TIME)
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (cdata->c_date_time_checkbutton), TRUE);
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
