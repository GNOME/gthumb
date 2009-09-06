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
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Street #330, Boston, MA 02111-1307, USA.
 */

#include <config.h>
#include <gtk/gtk.h>
#include "dlg-preferences.h"
#include "gconf-utils.h"
#include "gth-browser.h"
#include "gth-enum-types.h"
#include "gth-file-source-vfs.h"
#include "gth-main.h"
#include "gth-preferences.h"
#include "gtk-utils.h"
#include "glib-utils.h"
#include "typedefs.h"


#define GET_WIDGET(name) _gtk_builder_get_widget (data->builder, (name))

enum {
	CAPTION_COLUMN_NAME,
	CAPTION_COLUMN_ID,
	CAPTION_COLUMN_USED,
	CAPTION_COLUMNS
};

typedef struct {
	GthBrowser *browser;
	GtkBuilder *builder;
	GtkWidget  *dialog;
	GtkWidget  *toolbar_style_combobox;
	GtkWidget  *thumbnail_size_combobox;
	GtkWidget  *thumbnail_caption_list_view;
} DialogData;

static int thumb_size[] = { 48, 64, 85, 95, 112, 128, 164, 200, 256 };
static int thumb_sizes = sizeof (thumb_size) / sizeof (int);


static int
get_idx_from_size (gint size)
{
	int i;

	for (i = 0; i < thumb_sizes; i++)
		if (size == thumb_size[i])
			return i;
	return -1;
}


static void
destroy_cb (GtkWidget *widget,
	    DialogData *data)
{
	gth_browser_set_dialog (data->browser, "preferences", NULL);
	g_object_unref (data->builder);
	g_free (data);
}


/* called when the "apply" button is clicked. */
static void
apply_cb (GtkWidget  *widget,
	  DialogData *data)
{
	/* Startup dir. */

	eel_gconf_set_boolean (PREF_GO_TO_LAST_LOCATION, gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("go_to_last_location_radiobutton"))));
	eel_gconf_set_boolean (PREF_USE_STARTUP_LOCATION, gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("use_startup_location_radiobutton"))));

	if (eel_gconf_get_boolean (PREF_USE_STARTUP_LOCATION, FALSE)) {
		char *location;

		location = gtk_file_chooser_get_uri (GTK_FILE_CHOOSER (GET_WIDGET ("startup_dir_filechooserbutton")));
		eel_gconf_set_path (PREF_STARTUP_LOCATION, location);
		gth_pref_set_startup_location (location);
		g_free (location);
	}

	gth_hook_invoke ("dlg-preferences-apply", data->dialog, data->browser, data->builder);
}


/* called when the "close" button is clicked. */
static void
close_cb (GtkWidget  *widget,
	  DialogData *data)
{
	apply_cb (widget, data);
	gtk_widget_destroy (data->dialog);
}


/* called when the "help" button is clicked. */
static void
help_cb (GtkWidget  *widget,
	 DialogData *data)
{
	show_help_dialog (GTK_WINDOW (data->dialog), "preferences");
}


static void
use_startup_toggled_cb (GtkWidget *widget,
			DialogData *data)
{
	gtk_widget_set_sensitive (GET_WIDGET ("startup_dir_filechooserbutton"), gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget)));
	gtk_widget_set_sensitive (GET_WIDGET ("set_to_current_button"), gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget)));
}


static void
set_to_current_cb (GtkWidget  *widget,
		   DialogData *data)
{
	GthFileSource *file_source;

	file_source = gth_main_get_file_source (gth_browser_get_location (data->browser));
	if (GTH_IS_FILE_SOURCE_VFS (file_source)) {
		GFile *gio_file;
		char  *uri;

		gio_file = gth_file_source_to_gio_file (file_source, gth_browser_get_location (data->browser));
		uri = g_file_get_uri (gio_file);
		gtk_file_chooser_set_current_folder_uri (GTK_FILE_CHOOSER (GET_WIDGET ("startup_dir_filechooserbutton")), uri);

		g_free (uri);
		g_object_unref (gio_file);
	}
	g_object_unref (file_source);
}


static void
toolbar_style_changed_cb (GtkWidget  *widget,
			  DialogData *data)
{
	eel_gconf_set_enum (PREF_UI_TOOLBAR_STYLE, GTH_TYPE_TOOLBAR_STYLE, gtk_combo_box_get_active (GTK_COMBO_BOX (data->toolbar_style_combobox)));
}


static void
ask_to_save_toggled_cb (GtkToggleButton *button,
			DialogData      *data)
{
	eel_gconf_set_boolean (PREF_MSG_SAVE_MODIFIED_IMAGE, gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("ask_to_save_checkbutton"))));
}


static void
thumbnail_size_changed_cb (GtkWidget  *widget,
			   DialogData *data)
{
	eel_gconf_set_integer (PREF_THUMBNAIL_SIZE, thumb_size[gtk_combo_box_get_active (GTK_COMBO_BOX (data->thumbnail_size_combobox))]);
}


static void
fast_file_type_toggled_cb (GtkToggleButton *button,
			   DialogData      *data)
{
	eel_gconf_set_boolean (PREF_FAST_FILE_TYPE, ! gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("slow_mime_type_checkbutton"))));
}


static void
update_caption_from_list_view (DialogData *data)
{
	GtkTreeModel *tree_model;
	GtkTreeIter   iter;
	GString      *attributes;

	attributes = g_string_new ("");
	tree_model = gtk_tree_view_get_model (GTK_TREE_VIEW (data->thumbnail_caption_list_view));
	if (gtk_tree_model_get_iter_first (tree_model, &iter)) {
		do {
			gboolean  used;
			char     *attribute_id;

			gtk_tree_model_get (tree_model, &iter,
					    CAPTION_COLUMN_ID, &attribute_id,
					    CAPTION_COLUMN_USED, &used,
					    -1);

			if (used) {
				if (attributes->len > 0)
					g_string_append (attributes, ",");
				g_string_append (attributes, attribute_id);
			}

			g_free (attribute_id);
		}
		while (gtk_tree_model_iter_next (tree_model, &iter));
	}

	if (attributes->len > 0)
		eel_gconf_set_string (PREF_THUMBNAIL_CAPTION, attributes->str);
	else
		eel_gconf_set_string (PREF_THUMBNAIL_CAPTION, "none");

	g_string_free (attributes, TRUE);
}


static void
cell_renderer_toggle_toggled_cb (GtkCellRendererToggle *cell_renderer,
				 char                  *path,
                                 gpointer               user_data)
{
	DialogData   *data = user_data;
	GtkTreePath  *tpath;
	GtkTreeModel *tree_model;
	GtkTreeIter   iter;

	tpath = gtk_tree_path_new_from_string (path);
	if (tpath == NULL)
		return;

	tree_model = gtk_tree_view_get_model (GTK_TREE_VIEW (data->thumbnail_caption_list_view));
	if (gtk_tree_model_get_iter (tree_model, &iter, tpath)) {
		gboolean used;

		gtk_tree_model_get (tree_model, &iter,
				    CAPTION_COLUMN_USED, &used,
				    -1);
		gtk_list_store_set (GTK_LIST_STORE (tree_model), &iter,
				    CAPTION_COLUMN_USED, ! used,
				    -1);
	}

	update_caption_from_list_view (data);

	gtk_tree_path_free (tpath);
}


static void
caption_list_view_add_columns (GtkTreeView *treeview,
			       DialogData  *data)
{
	GtkTreeViewColumn *column;
	GtkCellRenderer   *renderer;

	/* the checkbox column */

	column = gtk_tree_view_column_new ();
	renderer = gtk_cell_renderer_toggle_new ();
	g_signal_connect (renderer,
			  "toggled",
			  G_CALLBACK (cell_renderer_toggle_toggled_cb),
			  data);
	gtk_tree_view_column_pack_start (column, renderer, FALSE);
	gtk_tree_view_column_set_attributes (column, renderer,
					     "active", CAPTION_COLUMN_USED,
					     NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);

	/* the name column. */

	column = gtk_tree_view_column_new ();
	renderer = gtk_cell_renderer_text_new ();
        gtk_tree_view_column_pack_start (column, renderer, TRUE);
        gtk_tree_view_column_set_attributes (column, renderer,
                                             "text", CAPTION_COLUMN_NAME,
                                             NULL);
        gtk_tree_view_column_set_expand (column, TRUE);
	gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);
}


void
dlg_preferences (GthBrowser *browser)
{
	DialogData       *data;
	char             *startup_location;
	GthFileSource    *file_source;
	GtkListStore     *caption_store;
	GtkTreeIter       iter;
	char             *current_caption;
	char            **attributes_v;
	char            **active_attributes_v;
	int               i;

	if (gth_browser_get_dialog (browser, "preferences") != NULL) {
		gtk_window_present (GTK_WINDOW (gth_browser_get_dialog (browser, "preferences")));
		return;
	}

	data = g_new0 (DialogData, 1);
	data->browser = browser;
	data->builder = _gtk_builder_new_from_file ("preferences.ui", NULL);
	data->dialog = GET_WIDGET ("preferences_dialog");

	gth_browser_set_dialog (browser, "preferences", data->dialog);

	eel_gconf_preload_cache ("/apps/gthumb/browser", GCONF_CLIENT_PRELOAD_ONELEVEL);
	eel_gconf_preload_cache ("/apps/gthumb/ui", GCONF_CLIENT_PRELOAD_ONELEVEL);

	/* Set widgets data. */

	data->toolbar_style_combobox = _gtk_combo_box_new_with_texts (_("System settings"), _("Text below icons"), _("Text beside icons"), _("Icons only"), _("Text only"), NULL);
	data->thumbnail_size_combobox = _gtk_combo_box_new_with_texts (_("48"), _("64"), _("85"), _("95"), _("112"), _("128"), _("164"), _("200"), _("256"), NULL);

	caption_store = gtk_list_store_new (CAPTION_COLUMNS, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_BOOLEAN);
	data->thumbnail_caption_list_view = gtk_tree_view_new_with_model (GTK_TREE_MODEL (caption_store));
	g_object_unref (caption_store);

	gtk_tree_view_set_reorderable (GTK_TREE_VIEW (data->thumbnail_caption_list_view), FALSE);
        gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (data->thumbnail_caption_list_view), FALSE);

	caption_list_view_add_columns (GTK_TREE_VIEW (data->thumbnail_caption_list_view), data);

	gtk_widget_show (data->thumbnail_caption_list_view);
	gtk_container_add (GTK_CONTAINER (GET_WIDGET ("caption_scrolledwindow")), data->thumbnail_caption_list_view);

	/* caption list */

	current_caption = eel_gconf_get_string (PREF_THUMBNAIL_CAPTION, DEFAULT_THUMBNAIL_CAPTION);
	active_attributes_v = g_strsplit (current_caption, ",", -1);
	attributes_v = gth_main_get_metadata_attributes ("*");
	for (i = 0; attributes_v[i] != NULL; i++) {
		GthMetadataInfo *info;
		const char      *name;
		gboolean         used;

		info = gth_main_get_metadata_info (attributes_v[i]);
		if ((info == NULL) || ((info->flags & GTH_METADATA_ALLOW_IN_FILE_LIST) == 0))
			continue;

		gtk_list_store_append (caption_store, &iter);

		name = info->display_name;
		if (name == NULL)
			name = info->id;
		used = _g_strv_find (active_attributes_v, info->id) != -1;
		gtk_list_store_set (caption_store, &iter,
				    CAPTION_COLUMN_NAME, name,
				    CAPTION_COLUMN_ID, info->id,
				    CAPTION_COLUMN_USED, used,
				    -1);
	}
	g_strfreev (attributes_v);
	g_strfreev (active_attributes_v);
	g_free (current_caption);

	gtk_widget_show (data->toolbar_style_combobox);
	gtk_widget_show (data->thumbnail_size_combobox);

	gtk_box_pack_start (GTK_BOX (GET_WIDGET ("toolbar_style_combobox_box")), data->toolbar_style_combobox, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (GET_WIDGET ("thumbnail_size_box")), data->thumbnail_size_combobox, FALSE, FALSE, 0);

	gtk_label_set_mnemonic_widget (GTK_LABEL (GET_WIDGET ("caption_label")), data->thumbnail_caption_list_view);
	gtk_label_set_mnemonic_widget (GTK_LABEL (GET_WIDGET ("size_label")), data->thumbnail_size_combobox);

	/* * general */

	if (eel_gconf_get_boolean (PREF_USE_STARTUP_LOCATION, FALSE))
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("use_startup_location_radiobutton")), TRUE);
	else if (eel_gconf_get_boolean (PREF_GO_TO_LAST_LOCATION, TRUE))
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("go_to_last_location_radiobutton")), TRUE);
	else
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("current_location_radiobutton")), TRUE);

	if (! eel_gconf_get_boolean (PREF_USE_STARTUP_LOCATION, FALSE)) {
		gtk_widget_set_sensitive (GET_WIDGET ("startup_dir_filechooserbutton"), FALSE);
		gtk_widget_set_sensitive (GET_WIDGET ("set_to_current_button"), FALSE);
	}

	startup_location = eel_gconf_get_path (PREF_STARTUP_LOCATION, NULL);
	file_source = gth_main_get_file_source_for_uri (startup_location);
	if (GTH_IS_FILE_SOURCE_VFS (file_source)) {
		GFile *location;
		GFile *folder;
		char  *folder_uri;

		location = g_file_new_for_uri (startup_location);
		folder = gth_file_source_to_gio_file (file_source, location);
		folder_uri = g_file_get_uri (folder);
		gtk_file_chooser_set_current_folder_uri (GTK_FILE_CHOOSER (GET_WIDGET ("startup_dir_filechooserbutton")), folder_uri);

		g_free (folder_uri);
		g_object_unref (folder);
		g_object_unref (location);
	}
	g_object_unref (file_source);
	g_free (startup_location);

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("ask_to_save_checkbutton")), eel_gconf_get_boolean (PREF_MSG_SAVE_MODIFIED_IMAGE, DEFAULT_MSG_SAVE_MODIFIED_IMAGE));
	gtk_combo_box_set_active (GTK_COMBO_BOX (data->toolbar_style_combobox), eel_gconf_get_enum (PREF_UI_TOOLBAR_STYLE, GTH_TYPE_TOOLBAR_STYLE, GTH_TOOLBAR_STYLE_SYSTEM));

	/* * browser */

	gtk_combo_box_set_active (GTK_COMBO_BOX (data->thumbnail_size_combobox), get_idx_from_size (eel_gconf_get_integer (PREF_THUMBNAIL_SIZE, DEFAULT_THUMBNAIL_SIZE)));
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("slow_mime_type_checkbutton")), ! eel_gconf_get_boolean (PREF_FAST_FILE_TYPE, DEFAULT_FAST_FILE_TYPE));

	gth_hook_invoke ("dlg-preferences-construct", data->dialog, data->browser, data->builder);

	/* Set the signals handlers. */

	g_signal_connect (G_OBJECT (data->dialog),
			  "destroy",
			  G_CALLBACK (destroy_cb),
			  data);
	g_signal_connect (G_OBJECT (GET_WIDGET ("close_button")),
			  "clicked",
			  G_CALLBACK (close_cb),
			  data);
	g_signal_connect (G_OBJECT (GET_WIDGET ("help_button")),
			  "clicked",
			  G_CALLBACK (help_cb),
			  data);

	/* general */

	g_signal_connect (G_OBJECT (data->toolbar_style_combobox),
			  "changed",
			  G_CALLBACK (toolbar_style_changed_cb),
			  data);
	g_signal_connect (G_OBJECT (GET_WIDGET ("use_startup_location_radiobutton")),
			  "toggled",
			  G_CALLBACK (use_startup_toggled_cb),
			  data);
	g_signal_connect (G_OBJECT (GET_WIDGET ("set_to_current_button")),
			  "clicked",
			  G_CALLBACK (set_to_current_cb),
			  data);
	g_signal_connect (G_OBJECT (GET_WIDGET ("ask_to_save_checkbutton")),
			  "toggled",
			  G_CALLBACK (ask_to_save_toggled_cb),
			  data);

	/* browser */

	g_signal_connect (G_OBJECT (data->thumbnail_size_combobox),
			  "changed",
			  G_CALLBACK (thumbnail_size_changed_cb),
			  data);
	g_signal_connect (G_OBJECT (GET_WIDGET ("slow_mime_type_checkbutton")),
			  "toggled",
			  G_CALLBACK (fast_file_type_toggled_cb),
			  data);

	/* run dialog. */

	gtk_window_set_transient_for (GTK_WINDOW (data->dialog), GTK_WINDOW (browser));
	gtk_window_set_modal (GTK_WINDOW (data->dialog), FALSE);
	gtk_widget_show (data->dialog);
}
