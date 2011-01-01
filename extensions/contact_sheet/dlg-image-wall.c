/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2010 Free Software Foundation, Inc.
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
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include "dlg-image-wall.h"
#include "gth-contact-sheet-creator.h"
#include "preferences.h"

#define GET_WIDGET(name) _gtk_builder_get_widget (data->builder, (name))
#define STRING_IS_VOID(x) (((x) == NULL) || (*(x) == 0))


enum {
	SORT_TYPE_COLUMN_DATA,
	SORT_TYPE_COLUMN_NAME
};

enum {
	THUMBNAIL_SIZE_TYPE_COLUMN_SIZE,
	THUMBNAIL_SIZE_TYPE_COLUMN_NAME
};

enum {
	FILE_TYPE_COLUMN_DEFAULT_EXTENSION,
	FILE_TYPE_COLUMN_MIME_TYPE
};

typedef struct {
	GthBrowser *browser;
	GList      *file_list;
	GtkBuilder *builder;
	GtkWidget  *dialog;
} DialogData;


static int thumb_size[] = { 64, 112, 128, 164, 200, 256, 312, 512 };
static int thumb_sizes = sizeof (thumb_size) / sizeof (int);


static int
get_idx_from_size (int size)
{
	int i;

	for (i = 0; i < thumb_sizes; i++)
		if (size == thumb_size[i])
			return i;
	return -1;
}


static void
destroy_cb (GtkWidget  *widget,
	    DialogData *data)
{
	gth_browser_set_dialog (data->browser, "image_wall", NULL);
	_g_object_list_unref (data->file_list);
	g_object_unref (data->builder);
	g_free (data);
}


static void
help_clicked_cb (GtkWidget  *widget,
		 DialogData *data)
{
	show_help_dialog (GTK_WINDOW (data->dialog), "image-wall");
}


static void
ok_clicked_cb (GtkWidget  *widget,
	       DialogData *data)
{
	char                 *s_value;
	GFile                *destination;
	const char           *template;
	char                 *mime_type;
	char                 *file_extension;
	GthContactSheetTheme *theme;
	int                   images_per_index;
	int                   single_page;
	int                   columns;
	GthFileDataSort      *sort_type;
	gboolean              sort_inverse;
	int                   thumbnail_size;
	GtkTreeIter           iter;
	GthTask              *task;

	/* save the options */

	s_value = gtk_file_chooser_get_uri (GTK_FILE_CHOOSER (GET_WIDGET ("destination_filechooserbutton")));
	destination = g_file_new_for_uri (s_value);
	eel_gconf_set_path (PREF_IMAGE_WALL_DESTINATION, s_value);
	g_free (s_value);

	template = gtk_entry_get_text (GTK_ENTRY (GET_WIDGET ("template_entry")));
	eel_gconf_set_string (PREF_IMAGE_WALL_TEMPLATE, template);

	mime_type = NULL;
	if (gtk_combo_box_get_active_iter (GTK_COMBO_BOX (GET_WIDGET ("filetype_combobox")), &iter)) {
		gtk_tree_model_get (GTK_TREE_MODEL (GET_WIDGET ("filetype_liststore")),
				    &iter,
				    FILE_TYPE_COLUMN_MIME_TYPE, &mime_type,
				    FILE_TYPE_COLUMN_DEFAULT_EXTENSION, &file_extension,
				    -1);
		eel_gconf_set_string (PREF_IMAGE_WALL_MIME_TYPE, mime_type);
	}

	images_per_index = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (GET_WIDGET ("images_per_index_spinbutton")));
	eel_gconf_set_integer (PREF_IMAGE_WALL_IMAGES_PER_PAGE, images_per_index);

	single_page = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("single_index_checkbutton")));
	eel_gconf_set_boolean (PREF_IMAGE_WALL_SINGLE_PAGE, single_page);

	columns = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (GET_WIDGET ("cols_spinbutton")));
	eel_gconf_set_integer (PREF_IMAGE_WALL_COLUMNS, columns);

	if (gtk_combo_box_get_active_iter (GTK_COMBO_BOX (GET_WIDGET ("sort_combobox")), &iter)) {
		gtk_tree_model_get (GTK_TREE_MODEL (GET_WIDGET ("sort_liststore")),
				    &iter,
				    SORT_TYPE_COLUMN_DATA, &sort_type,
				    -1);
		eel_gconf_set_string (PREF_IMAGE_WALL_SORT_TYPE, sort_type->name);
	}

	sort_inverse = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("reverse_order_checkbutton")));
	eel_gconf_set_boolean (PREF_IMAGE_WALL_SORT_INVERSE, sort_inverse);

	thumbnail_size = thumb_size[gtk_combo_box_get_active (GTK_COMBO_BOX (GET_WIDGET ("thumbnail_size_combobox")))];
	eel_gconf_set_integer (PREF_IMAGE_WALL_THUMBNAIL_SIZE, thumbnail_size);

	theme = gth_contact_sheet_theme_new ();
	theme->background_type = GTH_CONTACT_SHEET_BACKGROUND_TYPE_SOLID;
	gdk_color_parse ("#000", &theme->background_color1);
	theme->frame_style = GTH_CONTACT_SHEET_FRAME_STYLE_NONE;
	theme->frame_hpadding = 0;
	theme->frame_vpadding = 0;
	theme->frame_border = 0;
	theme->row_spacing = 0;
	theme->col_spacing = 0;

	/* exec the task */

	task = gth_contact_sheet_creator_new (data->browser, data->file_list);

	gth_contact_sheet_creator_set_header (GTH_CONTACT_SHEET_CREATOR (task), "");
	gth_contact_sheet_creator_set_footer (GTH_CONTACT_SHEET_CREATOR (task), "");
	gth_contact_sheet_creator_set_destination (GTH_CONTACT_SHEET_CREATOR (task), destination);
	gth_contact_sheet_creator_set_filename_template (GTH_CONTACT_SHEET_CREATOR (task), template);
	gth_contact_sheet_creator_set_mime_type (GTH_CONTACT_SHEET_CREATOR (task), mime_type, file_extension);
	gth_contact_sheet_creator_set_write_image_map (GTH_CONTACT_SHEET_CREATOR (task), FALSE);
	gth_contact_sheet_creator_set_theme (GTH_CONTACT_SHEET_CREATOR (task), theme);
	gth_contact_sheet_creator_set_images_per_index (GTH_CONTACT_SHEET_CREATOR (task), images_per_index);
	gth_contact_sheet_creator_set_single_index (GTH_CONTACT_SHEET_CREATOR (task), single_page);
	gth_contact_sheet_creator_set_columns (GTH_CONTACT_SHEET_CREATOR (task), columns);
	gth_contact_sheet_creator_set_sort_order (GTH_CONTACT_SHEET_CREATOR (task), sort_type, sort_inverse);
	gth_contact_sheet_creator_set_same_size (GTH_CONTACT_SHEET_CREATOR (task), FALSE);
	gth_contact_sheet_creator_set_thumb_size (GTH_CONTACT_SHEET_CREATOR (task), TRUE, thumbnail_size, thumbnail_size);
	gth_contact_sheet_creator_set_thumbnail_caption (GTH_CONTACT_SHEET_CREATOR (task), "");

	gth_browser_exec_task (data->browser, task, FALSE);
	gtk_widget_destroy (data->dialog);

	gth_contact_sheet_theme_unref (theme);
	g_free (file_extension);
	g_free (mime_type);
	g_object_unref (destination);
}


static void
update_sensitivity (DialogData *data)
{
	gtk_widget_set_sensitive (GET_WIDGET ("images_per_index_spinbutton"), ! gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("single_index_checkbutton"))));
}


static void
entry_help_icon_press_cb (GtkEntry             *entry,
			  GtkEntryIconPosition  icon_pos,
			  GdkEvent             *event,
			  gpointer              user_data)
{
	DialogData *data = user_data;
	GtkWidget  *help_box;

	if (GTK_WIDGET (entry) == GET_WIDGET ("template_entry"))
		help_box = GET_WIDGET ("template_help_table");

	if (gtk_widget_get_visible (help_box))
		gtk_widget_hide (help_box);
	else
		gtk_widget_show (help_box);
}


void
dlg_image_wall (GthBrowser *browser,
		GList      *file_list)
{
	DialogData *data;
	int         i;
	int         active_index;
	char       *default_sort_type;
	GList      *sort_types;
	GList      *scan;
	char       *s_value;
	char       *default_mime_type;
	GArray     *savers;

	if (gth_browser_get_dialog (browser, "image_wall") != NULL) {
		gtk_window_present (GTK_WINDOW (gth_browser_get_dialog (browser, "image_wall")));
		return;
	}

	data = g_new0 (DialogData, 1);
	data->browser = browser;
	data->file_list = _g_object_list_ref (file_list);
	data->builder = _gtk_builder_new_from_file ("image-wall.ui", "contact_sheet");

	data->dialog = _gtk_builder_get_widget (data->builder, "image_wall_dialog");
	gth_browser_set_dialog (browser, "image_wall", data->dialog);
	g_object_set_data (G_OBJECT (data->dialog), "dialog_data", data);

	/* Set widgets data. */

	s_value = eel_gconf_get_path (PREF_IMAGE_WALL_DESTINATION, NULL);
	if (s_value == NULL) {
		GFile *location = gth_browser_get_location (data->browser);
		if (location != NULL)
			s_value = g_file_get_uri (location);
		else
			s_value = g_strdup (get_home_uri ());
	}
	gtk_file_chooser_set_current_folder_uri (GTK_FILE_CHOOSER (GET_WIDGET ("destination_filechooserbutton")), s_value);
	g_free (s_value);

	s_value = eel_gconf_get_path (PREF_IMAGE_WALL_TEMPLATE, NULL);
	gtk_entry_set_text (GTK_ENTRY (GET_WIDGET ("template_entry")), s_value);
	g_free (s_value);

	default_mime_type = eel_gconf_get_string (PREF_IMAGE_WALL_MIME_TYPE, "image/jpeg");
	active_index = 0;
	savers = gth_main_get_type_set ("pixbuf-saver");
	for (i = 0; (savers != NULL) && (i < savers->len); i++) {
		GthPixbufSaver *saver;
		GtkTreeIter     iter;

		saver = g_object_new (g_array_index (savers, GType, i), NULL);

		if (g_str_equal (default_mime_type, gth_pixbuf_saver_get_mime_type (saver)))
			active_index = i;

		gtk_list_store_append (GTK_LIST_STORE (GET_WIDGET ("filetype_liststore")), &iter);
		gtk_list_store_set (GTK_LIST_STORE (GET_WIDGET ("filetype_liststore")), &iter,
				    FILE_TYPE_COLUMN_MIME_TYPE, gth_pixbuf_saver_get_mime_type (saver),
				    FILE_TYPE_COLUMN_DEFAULT_EXTENSION, gth_pixbuf_saver_get_default_ext (saver),
				    -1);

		g_object_unref (saver);
	}
	g_free (default_mime_type);

	gtk_combo_box_set_active (GTK_COMBO_BOX (GET_WIDGET ("filetype_combobox")), active_index);

	gtk_spin_button_set_value (GTK_SPIN_BUTTON (GET_WIDGET ("images_per_index_spinbutton")), eel_gconf_get_integer (PREF_IMAGE_WALL_IMAGES_PER_PAGE, 25));
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("single_index_checkbutton")), eel_gconf_get_boolean (PREF_IMAGE_WALL_SINGLE_PAGE, FALSE));
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (GET_WIDGET ("cols_spinbutton")), eel_gconf_get_integer (PREF_IMAGE_WALL_COLUMNS, 5));

	default_sort_type = eel_gconf_get_string (PREF_IMAGE_WALL_SORT_TYPE, "general::unsorted");
	active_index = 0;
	sort_types = gth_main_get_all_sort_types ();
	for (i = 0, scan = sort_types; scan; scan = scan->next, i++) {
		GthFileDataSort *sort_type = scan->data;
		GtkTreeIter      iter;

		if (g_str_equal (sort_type->name, default_sort_type))
			active_index = i;

		gtk_list_store_append (GTK_LIST_STORE (GET_WIDGET ("sort_liststore")), &iter);
		gtk_list_store_set (GTK_LIST_STORE (GET_WIDGET ("sort_liststore")), &iter,
				    SORT_TYPE_COLUMN_DATA, sort_type,
				    SORT_TYPE_COLUMN_NAME, _(sort_type->display_name),
				    -1);
	}
	g_list_free (sort_types);
	g_free (default_sort_type);

	gtk_combo_box_set_active (GTK_COMBO_BOX (GET_WIDGET ("sort_combobox")), active_index);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("reverse_order_checkbutton")), eel_gconf_get_boolean (PREF_IMAGE_WALL_SORT_INVERSE, FALSE));

	for (i = 0; i < thumb_sizes; i++) {
		char        *name;
		GtkTreeIter  iter;

		name = g_strdup_printf ("%d", thumb_size[i]);

		gtk_list_store_append (GTK_LIST_STORE (GET_WIDGET ("thumbnail_size_liststore")), &iter);
		gtk_list_store_set (GTK_LIST_STORE (GET_WIDGET ("thumbnail_size_liststore")), &iter,
				    THUMBNAIL_SIZE_TYPE_COLUMN_SIZE, thumb_size[i],
				    THUMBNAIL_SIZE_TYPE_COLUMN_NAME, name,
				    -1);

		g_free (name);
	}
	gtk_combo_box_set_active (GTK_COMBO_BOX (GET_WIDGET ("thumbnail_size_combobox")), get_idx_from_size (eel_gconf_get_integer (PREF_IMAGE_WALL_THUMBNAIL_SIZE, 128)));

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
	g_signal_connect (GET_WIDGET ("template_entry"),
			  "icon-press",
			  G_CALLBACK (entry_help_icon_press_cb),
			  data);
	g_signal_connect_swapped (GET_WIDGET ("single_index_checkbutton"),
				  "toggled",
				  G_CALLBACK (update_sensitivity),
				  data);

	/* Run dialog. */

	gtk_window_set_transient_for (GTK_WINDOW (data->dialog), GTK_WINDOW (browser));
	gtk_window_set_modal (GTK_WINDOW (data->dialog), FALSE);
	gtk_widget_show (data->dialog);
}
