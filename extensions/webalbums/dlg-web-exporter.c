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
#include "gth-web-exporter.h"
#include "preferences.h"

#define GET_WIDGET(name) _gtk_builder_get_widget (data->builder, (name))
#define STRING_IS_VOID(x) (((x) == NULL) || (*(x) == 0))
#define DEFAULT_ALBUM_THEME "Wiki"

enum {
	THEME_COLUMN_ID,
	THEME_COLUMN_NAME,
	THEME_COLUMN_PREVIEW
};

enum {
	SORT_TYPE_COLUMN_DATA,
	SORT_TYPE_COLUMN_NAME
};

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
	GList      *file_list;
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
	_g_object_list_unref (data->file_list);
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
	char            *s_value;
	GFile           *destination;
	int              i_value;
	const char      *header;
	const char      *footer;
	char            *thumbnail_caption;
	char            *image_caption;
	GtkTreeIter      iter;
	char            *theme_name;
	GthFileDataSort *sort_type;
	GthTask         *task;

	/* save the options */

	s_value = gtk_file_chooser_get_uri (GTK_FILE_CHOOSER (GET_WIDGET ("destination_filechooserbutton")));
	destination = g_file_new_for_uri (s_value);
	eel_gconf_set_path (PREF_WEBALBUMS_DESTINATION, s_value);
	g_free (s_value);

	eel_gconf_set_boolean (PREF_WEBALBUMS_USE_SUBFOLDERS, gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("use_subfolders_checkbutton"))));
	eel_gconf_set_boolean (PREF_WEBALBUMS_COPY_IMAGES, gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("copy_images_checkbutton"))));
	eel_gconf_set_boolean (PREF_WEBALBUMS_RESIZE_IMAGES, gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("resize_images_checkbutton"))));

	i_value = gtk_combo_box_get_active (GTK_COMBO_BOX (GET_WIDGET ("resize_images_combobox")));
	eel_gconf_set_integer (PREF_WEBALBUMS_RESIZE_WIDTH, resize_size[i_value].width);
	eel_gconf_set_integer (PREF_WEBALBUMS_RESIZE_HEIGHT, resize_size[i_value].height);

	eel_gconf_set_integer (PREF_WEBALBUMS_COLUMNS, gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (GET_WIDGET ("cols_spinbutton"))));
	eel_gconf_set_integer (PREF_WEBALBUMS_ROWS, gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (GET_WIDGET ("rows_spinbutton"))));
	eel_gconf_set_boolean (PREF_WEBALBUMS_SINGLE_INDEX, gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("single_index_checkbutton"))));

	if (gtk_combo_box_get_active_iter (GTK_COMBO_BOX (GET_WIDGET ("sort_combobox")), &iter)) {
		GthFileDataSort *sort_type;

		gtk_tree_model_get (GTK_TREE_MODEL (GET_WIDGET ("sort_liststore")),
				    &iter,
				    SORT_TYPE_COLUMN_DATA, &sort_type,
				    -1);
		eel_gconf_set_string (PREF_WEBALBUMS_SORT_TYPE, sort_type->name);
	}

	eel_gconf_set_boolean (PREF_WEBALBUMS_SORT_INVERSE, gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("reverse_order_checkbutton"))));

	header = gtk_entry_get_text (GTK_ENTRY (GET_WIDGET ("header_entry")));
	eel_gconf_set_string (PREF_WEBALBUMS_HEADER, header);

	footer = gtk_entry_get_text (GTK_ENTRY (GET_WIDGET ("footer_entry")));
	eel_gconf_set_string (PREF_WEBALBUMS_FOOTER, footer);

	theme_name = NULL;
	{
		GList *list;

		list = gtk_icon_view_get_selected_items (GTK_ICON_VIEW (GET_WIDGET ("theme_iconview")));
		if (list != NULL) {
			GtkTreePath *path;
			GtkTreeIter  iter;

			path = g_list_first (list)->data;
			gtk_tree_model_get_iter (GTK_TREE_MODEL (GET_WIDGET ("theme_liststore")), &iter, path);
			gtk_tree_model_get (GTK_TREE_MODEL (GET_WIDGET ("theme_liststore")), &iter,
					    THEME_COLUMN_NAME, &theme_name,
					    -1);
		}

		g_list_foreach (list, (GFunc) gtk_tree_path_free, NULL);
		g_list_free (list);
	}
	g_return_if_fail (theme_name != NULL);
	eel_gconf_set_string (PREF_WEBALBUMS_THEME, theme_name);

	thumbnail_caption = gth_metadata_chooser_get_selection (GTH_METADATA_CHOOSER (data->thumbnail_caption_chooser));
	eel_gconf_set_string (PREF_WEBALBUMS_THUMBNAIL_CAPTION, thumbnail_caption);

	image_caption = gth_metadata_chooser_get_selection (GTH_METADATA_CHOOSER (data->image_caption_chooser));
	eel_gconf_set_string (PREF_WEBALBUMS_IMAGE_CAPTION, image_caption);

	/* exec the task */

	task = gth_web_exporter_new (data->browser, data->file_list);

	gth_web_exporter_set_header (GTH_WEB_EXPORTER (task), header);
	gth_web_exporter_set_footer (GTH_WEB_EXPORTER (task), footer);
	gth_web_exporter_set_style (GTH_WEB_EXPORTER (task), theme_name);
	gth_web_exporter_set_destination (GTH_WEB_EXPORTER (task), destination);
	gth_web_exporter_set_use_subfolders (GTH_WEB_EXPORTER (task), eel_gconf_get_boolean (PREF_WEBALBUMS_USE_SUBFOLDERS, TRUE));
	gth_web_exporter_set_copy_images (GTH_WEB_EXPORTER (task), eel_gconf_get_boolean (PREF_WEBALBUMS_COPY_IMAGES, FALSE));
	gth_web_exporter_set_resize_images (GTH_WEB_EXPORTER (task),
					    eel_gconf_get_boolean (PREF_WEBALBUMS_RESIZE_IMAGES, FALSE),
					    eel_gconf_get_integer (PREF_WEBALBUMS_RESIZE_WIDTH, 640),
					    eel_gconf_get_integer (PREF_WEBALBUMS_RESIZE_HEIGHT, 480));

	s_value = eel_gconf_get_string (PREF_WEBALBUMS_SORT_TYPE, "file::mtime");
	sort_type = gth_main_get_sort_type (s_value);
	gth_web_exporter_set_sort_order (GTH_WEB_EXPORTER (task),
					 sort_type,
					 eel_gconf_get_boolean (PREF_WEBALBUMS_SORT_INVERSE, FALSE));
	g_free (s_value);

	gth_web_exporter_set_row_col (GTH_WEB_EXPORTER (task),
				      eel_gconf_get_integer (PREF_WEBALBUMS_ROWS, 4),
				      eel_gconf_get_integer (PREF_WEBALBUMS_COLUMNS, 4));
	gth_web_exporter_set_single_index (GTH_WEB_EXPORTER (task), eel_gconf_get_boolean (PREF_WEBALBUMS_SINGLE_INDEX, FALSE));
	gth_web_exporter_set_thumbnail_caption (GTH_WEB_EXPORTER (task), thumbnail_caption);
	gth_web_exporter_set_image_caption (GTH_WEB_EXPORTER (task), image_caption);

	gth_browser_exec_task (data->browser, task, FALSE);
	gtk_widget_destroy (data->dialog);

	g_free (image_caption);
	g_free (thumbnail_caption);
	g_free (theme_name);
	g_object_unref (destination);
}


static void
update_sensitivity (DialogData *data)
{
	gtk_widget_set_sensitive (GET_WIDGET ("resize_images_combobox"), gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("resize_images_checkbutton"))));
	gtk_widget_set_sensitive (GET_WIDGET ("resize_images_hbox"), gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("copy_images_checkbutton"))));
}


static void
footer_entry_icon_press_cb (GtkEntry             *entry,
			    GtkEntryIconPosition  icon_pos,
			    GdkEvent             *event,
			    gpointer              user_data)
{
	DialogData *data = user_data;
	GtkWidget  *help_box;

	help_box = GET_WIDGET ("template_help_table");
	if (GTK_WIDGET_VISIBLE (help_box))
		gtk_widget_hide (help_box);
	else
		gtk_widget_show (help_box);
}


static void
add_themes_from_dir (DialogData *data,
		     GFile      *dir)
{
	GFileEnumerator *enumerator;
	GFileInfo       *file_info;
	char            *default_theme;

	enumerator = g_file_enumerate_children (dir,
						(G_FILE_ATTRIBUTE_STANDARD_NAME ","
						 G_FILE_ATTRIBUTE_STANDARD_TYPE ","
						 G_FILE_ATTRIBUTE_STANDARD_DISPLAY_NAME),
						G_FILE_QUERY_INFO_NONE,
						NULL,
						NULL);
	if (enumerator == NULL)
		return;

	default_theme = eel_gconf_get_string (PREF_WEBALBUMS_THEME, DEFAULT_ALBUM_THEME);

	while ((file_info = g_file_enumerator_next_file (enumerator, NULL, NULL)) != NULL) {
		GFile     *file;
		char      *filename;
		GdkPixbuf *preview;

		if (g_file_info_get_file_type (file_info) != G_FILE_TYPE_DIRECTORY) {
			g_object_unref (file_info);
			continue;
		}

		file = _g_file_get_child (dir, g_file_info_get_name (file_info), "preview.png", NULL);
		filename = g_file_get_path (file);
		preview = gdk_pixbuf_new_from_file_at_size (filename, 128, 128, NULL);
		if (preview != NULL) {
			GtkTreeIter iter;

			gtk_list_store_append (GTK_LIST_STORE (GET_WIDGET ("theme_liststore")), &iter);
			gtk_list_store_set (GTK_LIST_STORE (GET_WIDGET ("theme_liststore")), &iter,
					    THEME_COLUMN_ID, g_file_info_get_name (file_info),
					    THEME_COLUMN_NAME, g_file_info_get_display_name (file_info),
					    THEME_COLUMN_PREVIEW, preview,
					    -1);

			if (g_str_equal (default_theme, g_file_info_get_name (file_info))) {
				GtkTreePath *path;

				path = gtk_tree_model_get_path (GTK_TREE_MODEL (GET_WIDGET ("theme_liststore")), &iter);
				gtk_icon_view_select_path (GTK_ICON_VIEW (GET_WIDGET ("theme_iconview")), path);
				gtk_tree_path_free (path);
			}
		}

		g_object_unref (preview);
		g_free (filename);
		g_object_unref (file);
		g_object_unref (file_info);
	}

	g_free (default_theme);
	g_object_unref (enumerator);
}


static void
load_themes (DialogData *data)
{
	char  *style_path;
	GFile *style_dir;
	GFile *data_dir;

	/* local themes */

	style_path = gth_user_dir_get_file (GTH_DIR_DATA, GTHUMB_DIR, "albumthemes", NULL);
	style_dir = g_file_new_for_path (style_path);
	add_themes_from_dir (data, style_dir);
	g_object_unref (style_dir);
	g_free (style_path);

	/* system themes */

	data_dir = g_file_new_for_path (WEBALBUM_DATADIR);
	style_dir = _g_file_get_child (data_dir, "albumthemes", NULL);
	add_themes_from_dir (data, style_dir);
	g_object_unref (style_dir);
	g_object_unref (data_dir);
}


void
dlg_web_exporter (GthBrowser *browser,
		  GList      *file_list)
{
	DialogData *data;
	int         i;
	int         active_index;
	char       *default_sort_type;
	GList      *scan;
	char       *caption;

	if (gth_browser_get_dialog (browser, "web_exporter") != NULL) {
		gtk_window_present (GTK_WINDOW (gth_browser_get_dialog (browser, "web_exporter")));
		return;
	}

	data = g_new0 (DialogData, 1);
	data->browser = browser;
	data->file_list = _g_object_list_ref (file_list);
	data->builder = _gtk_builder_new_from_file ("web-album-exporter.ui", "webalbums");

	data->dialog = _gtk_builder_get_widget (data->builder, "web_album_dialog");
	gth_browser_set_dialog (browser, "web_exporter", data->dialog);
	g_object_set_data (G_OBJECT (data->dialog), "dialog_data", data);

	data->thumbnail_caption_chooser = gth_metadata_chooser_new (GTH_METADATA_ALLOW_EVERYWHERE);
	gtk_widget_show (data->thumbnail_caption_chooser);
	gtk_container_add (GTK_CONTAINER (GET_WIDGET ("thumbnail_caption_scrolledwindow")), data->thumbnail_caption_chooser);

	data->image_caption_chooser = gth_metadata_chooser_new (GTH_METADATA_ALLOW_EVERYWHERE);
	gtk_widget_show (data->image_caption_chooser);
	gtk_container_add (GTK_CONTAINER (GET_WIDGET ("image_caption_scrolledwindow")), data->image_caption_chooser);

	/* Set widgets data. */

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("use_subfolders_checkbutton")), eel_gconf_get_boolean (PREF_WEBALBUMS_USE_SUBFOLDERS, TRUE));
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("copy_images_checkbutton")), eel_gconf_get_boolean (PREF_WEBALBUMS_COPY_IMAGES, FALSE));
	gtk_widget_set_sensitive (GET_WIDGET ("resize_images_checkbutton"), eel_gconf_get_boolean (PREF_WEBALBUMS_COPY_IMAGES, FALSE));
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("resize_images_checkbutton")), eel_gconf_get_boolean (PREF_WEBALBUMS_RESIZE_IMAGES, FALSE));
	gtk_widget_set_sensitive (GET_WIDGET ("resize_images_options_hbox"), eel_gconf_get_boolean (PREF_WEBALBUMS_RESIZE_IMAGES, FALSE));
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (GET_WIDGET ("cols_spinbutton")), eel_gconf_get_integer (PREF_WEBALBUMS_COLUMNS, 4));
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (GET_WIDGET ("rows_spinbutton")), eel_gconf_get_integer (PREF_WEBALBUMS_ROWS, 4));
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("single_index_checkbutton")), eel_gconf_get_boolean (PREF_WEBALBUMS_SINGLE_INDEX, FALSE));
	gtk_widget_set_sensitive (GET_WIDGET ("columns_rows_table"), ! eel_gconf_get_boolean (PREF_WEBALBUMS_SINGLE_INDEX, FALSE));

	active_index = 0;
	for (i = 0; i < G_N_ELEMENTS (resize_size); i++) {
		GtkTreeIter  iter;
		char        *name;

		gtk_list_store_append (GTK_LIST_STORE (GET_WIDGET ("size_liststore")), &iter);

		if ((resize_size[i].width == eel_gconf_get_integer (PREF_WEBALBUMS_RESIZE_WIDTH, 640))
		    && (resize_size[i].height == eel_gconf_get_integer (PREF_WEBALBUMS_RESIZE_HEIGHT, 480)))
		{
			active_index = i;
		}

		/* Translators: this is an image size, such as 1024 × 768 */
		name = g_strdup_printf (_("%d × %d"), resize_size[i].width, resize_size[i].height);
		gtk_list_store_set (GTK_LIST_STORE (GET_WIDGET ("size_liststore")), &iter,
				    0, name,
				    -1);

		g_free (name);
	}
	gtk_combo_box_set_active (GTK_COMBO_BOX (GET_WIDGET ("resize_images_combobox")), active_index);

	default_sort_type = eel_gconf_get_string (PREF_WEBALBUMS_SORT_TYPE, "file::mtime");
	active_index = 0;
	for (i = 0, scan = gth_main_get_all_sort_types (); scan; scan = scan->next, i++) {
		GthFileDataSort *sort_type = scan->data;
		GtkTreeIter      iter;

		if (g_str_equal (sort_type->name, default_sort_type))
			active_index = i;

		gtk_list_store_append (GTK_LIST_STORE (GET_WIDGET ("sort_liststore")), &iter);
		gtk_list_store_set (GTK_LIST_STORE (GET_WIDGET ("sort_liststore")), &iter,
				    SORT_TYPE_COLUMN_DATA, sort_type,
				    SORT_TYPE_COLUMN_NAME, sort_type->display_name,
				    -1);
	}
	gtk_combo_box_set_active (GTK_COMBO_BOX (GET_WIDGET ("sort_combobox")), active_index);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("reverse_order_checkbutton")), eel_gconf_get_boolean (PREF_WEBALBUMS_SORT_INVERSE, FALSE));

	g_free (default_sort_type);

	gtk_entry_set_text (GTK_ENTRY (GET_WIDGET ("header_entry")),
			    g_file_info_get_edit_name (gth_browser_get_location_data (browser)->info));

	caption = eel_gconf_get_string (PREF_WEBALBUMS_THUMBNAIL_CAPTION, "");
	gth_metadata_chooser_set_selection (GTH_METADATA_CHOOSER (data->thumbnail_caption_chooser), caption);
	g_free (caption);

	caption = eel_gconf_get_string (PREF_WEBALBUMS_IMAGE_CAPTION, "");
	gth_metadata_chooser_set_selection (GTH_METADATA_CHOOSER (data->image_caption_chooser), caption);
	g_free (caption);

	load_themes (data);
	update_sensitivity (data);

	{
		char *destination;

		destination = eel_gconf_get_path (PREF_WEBALBUMS_DESTINATION, NULL);
		if (destination == NULL)
			destination = g_strdup (get_home_uri ());
		gtk_file_chooser_set_current_folder_uri (GTK_FILE_CHOOSER (GET_WIDGET ("destination_filechooserbutton")), destination);

		g_free (destination);
	}

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
