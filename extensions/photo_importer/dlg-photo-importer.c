/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2009 Free Software Foundation, Inc.
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
#include <gthumb.h>
#include "dlg-photo-importer.h"
#include "enum-types.h"
#include "gth-import-task.h"
#include "preferences.h"


enum {
	SOURCE_LIST_COLUMN_VOLUME,
	SOURCE_LIST_COLUMN_ICON,
	SOURCE_LIST_COLUMN_NAME,
	SOURCE_LIST_COLUMNS
};

#define GET_WIDGET(name) _gtk_builder_get_widget (data->builder, (name))


typedef struct {
	GthBrowser    *browser;
	GtkWidget     *dialog;
	GtkBuilder    *builder;
	GFile         *source;
	GFile         *last_source;
	GtkListStore  *source_store;
	GtkWidget     *source_list;
	GtkWidget     *subfolder_type_list;
	GtkWidget     *file_list;
	GCancellable  *cancellable;
	GList         *files;
	gboolean       loading_list;
	gboolean       import;
	GthFileSource *vfs_source;
	DoneFunc       done_func;
	gboolean       cancelling;
	gulong         monitor_event;
	GtkWidget     *filter_combobox;
	GtkWidget     *tags_entry;
	GList         *general_tests;
} DialogData;


static void
destroy_dialog (gpointer user_data)
{
	DialogData       *data = user_data;
	GFile            *destination;
	gboolean          single_subfolder;
	GthSubfolderType  subfolder_type;
	gboolean          delete_imported;
	gboolean          adjust_orientation;

	g_signal_handler_disconnect (gth_main_get_default_monitor (), data->monitor_event);

	destination = gtk_file_chooser_get_current_folder_file (GTK_FILE_CHOOSER (GET_WIDGET ("destination_filechooserbutton")));
	if (destination != NULL) {
		char *uri;

		uri = g_file_get_uri (destination);
		eel_gconf_set_string (PREF_PHOTO_IMPORT_DESTINATION, uri);

		g_free (uri);
	}

	single_subfolder = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("single_subfolder_checkbutton")));
	eel_gconf_set_boolean (PREF_PHOTO_IMPORT_SUBFOLDER_SINGLE, single_subfolder);

	subfolder_type = gtk_combo_box_get_active (GTK_COMBO_BOX (data->subfolder_type_list));
	eel_gconf_set_enum (PREF_PHOTO_IMPORT_SUBFOLDER_TYPE, GTH_TYPE_SUBFOLDER_TYPE, subfolder_type);

	delete_imported = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("delete_checkbutton")));
	eel_gconf_set_boolean (PREF_PHOTO_IMPORT_DELETE, delete_imported);

	adjust_orientation = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("adjust_orientation_checkbutton")));
	eel_gconf_set_boolean (PREF_PHOTO_IMPORT_ADJUST_ORIENTATION, adjust_orientation);

	if (data->import) {
		GthFileStore  *file_store;
		GList         *files;
		char         **tags;
		GthTask       *task;

		file_store = (GthFileStore *) gth_file_view_get_model (GTH_FILE_VIEW (gth_file_list_get_view (GTH_FILE_LIST (data->file_list))));
		files = gth_file_store_get_checked (file_store);
		tags = gth_tags_entry_get_tags (GTH_TAGS_ENTRY (data->tags_entry));
		task = gth_import_task_new (data->browser,
					    files,
					    destination,
					    subfolder_type,
					    single_subfolder,
					    tags,
					    delete_imported,
					    adjust_orientation);
		gth_browser_exec_task (data->browser, task, FALSE);

		g_strfreev (tags);
		g_object_unref (task);
		_g_object_list_unref (files);
	}

	_g_object_unref (destination);

	gtk_widget_destroy (data->dialog);
	gth_browser_set_dialog (data->browser, "photo_importer", NULL);

	g_object_unref (data->vfs_source);
	g_object_unref (data->builder);
	_g_object_unref (data->source);
	_g_object_unref (data->last_source);
	_g_object_unref (data->cancellable);
	_g_object_list_unref (data->files);
	_g_string_list_free (data->general_tests);

	g_free (data);
}


static void
cancel_done (gpointer user_data)
{
	DialogData *data = user_data;

	g_cancellable_reset (data->cancellable);
	data->cancelling = FALSE;
	data->done_func (data);
}


static void
cancel (DialogData *data,
	DoneFunc    done_func)
{
	if (data->cancelling)
		return;

	data->done_func = done_func;
	data->cancelling = TRUE;
	if (data->loading_list)
		g_cancellable_cancel (data->cancellable);
	else
		gth_file_list_cancel (GTH_FILE_LIST (data->file_list), cancel_done, data);
}


static void
close_dialog (gpointer unused,
	      DialogData *data)
{
	cancel (data, destroy_dialog);
}


static gboolean
delete_event_cb (GtkWidget *widget,
                 GdkEvent  *event,
                 gpointer   user_data)
{
	close_dialog (NULL, (DialogData *) user_data);
	return TRUE;
}


static void
ok_clicked_cb (GtkWidget  *widget,
	       DialogData *data)
{
	data->import = TRUE;
	close_dialog (NULL, data);
}


static void
update_sensitivity (DialogData *data)
{
	gboolean can_import;

	can_import = data->source != NULL;
	gtk_widget_set_sensitive (GET_WIDGET ("ok_button"), can_import);
	gtk_widget_set_sensitive (GET_WIDGET ("list_command_box"), can_import);
}


static void
update_status (DialogData *data)
{
	GthFileStore *file_store;
	int           n_checked;
	goffset       size;
	GList        *checked;
	GList        *scan;
	char         *ssize;
	char         *status;

	file_store = (GthFileStore *) gth_file_view_get_model (GTH_FILE_VIEW (gth_file_list_get_view (GTH_FILE_LIST (data->file_list))));

	n_checked = 0;
	size = 0;
	checked = gth_file_store_get_checked (file_store);
	for (scan = checked; scan; scan = scan->next) {
		GthFileData *file_data = scan->data;

		size += g_file_info_get_size (file_data->info);
		n_checked += 1;
	}
	ssize = g_format_size_for_display (size);
	status = g_strdup_printf (_("Files: %d (%s)"), n_checked, ssize);
	gtk_label_set_text (GTK_LABEL (GET_WIDGET ("status_label")), status);

	g_free (status);
	g_free (ssize);
}


static void
file_store_changed_cb (GthFileStore *file_store,
		       DialogData   *data)
{
	update_status (data);
}


static void
list_ready_cb (GList    *files,
	       GError   *error,
	       gpointer  user_data)
{
	DialogData *data = user_data;

	data->loading_list = FALSE;

	if (data->cancelling) {
		gth_file_list_cancel (GTH_FILE_LIST (data->file_list), cancel_done, data);
	}
	else if (error != NULL) {
		_gtk_error_dialog_from_gerror_show (GTK_WINDOW (data->dialog), _("Could not load the folder"), &error);
	}
	else {
		_g_object_unref (data->last_source);
		data->last_source = g_file_dup (data->source);

		data->files = _g_object_list_ref (files);
		gth_file_list_set_files (GTH_FILE_LIST (data->file_list), data->files);
	}

	update_sensitivity (data);
}


static void
list_source_files (gpointer user_data)
{
	DialogData *data = user_data;
	GList      *list;

	_g_object_clear (&data->last_source);
	_g_object_list_unref (data->files);
	data->files = NULL;

	if (data->source == NULL) {
		gth_file_list_clear (GTH_FILE_LIST (data->file_list), _("(Empty)"));
		update_sensitivity (data);
		return;
	}

	gth_file_list_clear (GTH_FILE_LIST (data->file_list), _("Getting folder listing..."));

	data->loading_list = TRUE;
	list = g_list_prepend (NULL, data->source);
	_g_query_all_metadata_async (list,
				     TRUE,
				     TRUE,
				     DEFINE_STANDARD_ATTRIBUTES (",preview::icon,standard::fast-content-type,gth::file::display-size"),
				     data->cancellable,
				     list_ready_cb,
				     data);

	g_list_free (list);
}


static void
load_file_list (DialogData *data)
{
	update_sensitivity (data);
	if (_g_file_equal (data->source, data->last_source))
		return;
	cancel (data, list_source_files);
}


static void
source_list_changed_cb (GtkWidget  *widget,
			DialogData *data)
{
	GtkTreeIter  iter;
	GVolume     *volume;
	GMount      *mount;

	if (! gtk_combo_box_get_active_iter (GTK_COMBO_BOX (data->source_list), &iter)) {
		_g_object_clear (&data->source);
		_g_object_clear (&data->last_source);
		gth_file_list_clear (GTH_FILE_LIST (data->file_list), _("(Empty)"));
		return;
	}

	gtk_tree_model_get (GTK_TREE_MODEL (data->source_store), &iter,
			    SOURCE_LIST_COLUMN_VOLUME, &volume,
			    -1);

	if (volume == NULL) {
		_g_object_clear (&data->source);
		_g_object_clear (&data->last_source);
		gth_file_list_clear (GTH_FILE_LIST (data->file_list), _("Empty"));
		return;
	}

	mount = g_volume_get_mount (volume);
	data->source = g_mount_get_root (mount);
	load_file_list (data);

	g_object_unref (mount);
	g_object_unref (volume);
}


static void
update_source_list (DialogData *data)
{
	gboolean  source_available = FALSE;
	GList    *mounts;
	GList    *scan;

	gtk_list_store_clear (data->source_store);

	mounts = g_volume_monitor_get_mounts (g_volume_monitor_get ());
	for (scan = mounts; scan; scan = scan->next) {
		GMount  *mount = scan->data;
		GVolume *volume;

		if (g_mount_is_shadowed (mount))
			continue;

		volume = g_mount_get_volume (mount);
		if (volume != NULL) {
			if (g_volume_can_mount (volume)) {
				GtkTreeIter  iter;
				GFile       *root;
				GIcon       *icon;
				char        *name;

				gtk_list_store_append (data->source_store, &iter);

				root = g_mount_get_root (mount);
				if (data->source == NULL)
					data->source = g_file_dup (root);

				icon = g_mount_get_icon (mount);
				name = g_volume_get_name (volume);
				gtk_list_store_set (data->source_store, &iter,
						    SOURCE_LIST_COLUMN_VOLUME, volume,
						    SOURCE_LIST_COLUMN_ICON, icon,
						    SOURCE_LIST_COLUMN_NAME, name,
						    -1);

				if (g_file_equal (data->source, root)) {
					source_available = TRUE;
					gtk_combo_box_set_active_iter (GTK_COMBO_BOX (data->source_list), &iter);
				}

				g_free (name);
				g_object_unref (icon);
				g_object_unref (root);
			}

			g_object_unref (volume);
		}
	}

	if (! source_available) {
		_g_object_unref (data->source);
		data->source = NULL;
		load_file_list (data);
	}

	_g_object_list_unref (mounts);
}


static void
entry_points_changed_cb (GthMonitor *monitor,
			 DialogData *data)
{
	update_source_list (data);
}


static void
filter_combobox_changed_cb (GtkComboBox *widget,
			    DialogData  *data)
{
	int         idx;
	const char *test_id;
	GthTest    *test;

	idx = gtk_combo_box_get_active (widget);
	test_id = g_list_nth (data->general_tests, idx)->data;
	test = gth_main_get_test (test_id);
	gth_file_list_set_filter (GTH_FILE_LIST (data->file_list), test);

	g_object_unref (test);
}


static void
select_all_button_clicked_cb (GtkButton  *button,
			      DialogData *data)
{
	GthFileStore *file_store;
	GtkTreeIter   iter;

	file_store = (GthFileStore *) gth_file_view_get_model (GTH_FILE_VIEW (gth_file_list_get_view (GTH_FILE_LIST (data->file_list))));
	if (gth_file_store_get_first (file_store, &iter)) {
		do {
			gth_file_store_queue_set (file_store,
						  &iter,
						  GTH_FILE_STORE_CHECKED_COLUMN, TRUE,
						  -1);
		}
		while (gth_file_store_get_next (file_store, &iter));

		gth_file_store_exec_set (file_store);
	}
}


static void
select_none_button_clicked_cb (GtkButton  *button,
			       DialogData *data)
{
	GthFileStore *file_store;
	GtkTreeIter   iter;

	file_store = (GthFileStore *) gth_file_view_get_model (GTH_FILE_VIEW (gth_file_list_get_view (GTH_FILE_LIST (data->file_list))));
	if (gth_file_store_get_first (file_store, &iter)) {
		do {
			gth_file_store_queue_set (file_store,
						  &iter,
						  GTH_FILE_STORE_CHECKED_COLUMN, FALSE,
						  -1);
		}
		while (gth_file_store_get_next (file_store, &iter));

		gth_file_store_exec_set (file_store);
	}
}


static GthFileData *
create_example_file_data (void)
{
	GFile       *file;
	GFileInfo   *info;
	GthFileData *file_data;
	GthMetadata *metadata;

	file = g_file_new_for_uri ("file://home/user/document.txt");
	info = g_file_info_new ();
	file_data = gth_file_data_new (file, info);

	metadata = g_object_new (GTH_TYPE_METADATA,
				 "raw", "2005:03:09 13:23:51",
				 "formatted", "2005:03:09 13:23:51",
				 NULL);
	g_file_info_set_attribute_object (info, "Embedded::Image::DateTime", G_OBJECT (metadata));

	g_object_unref (metadata);
	g_object_unref (info);
	g_object_unref (file);

	return file_data;
}


static void
update_destination (DialogData *data)
{
	GFile             *destination;
	GthSubfolderType   subfolder_type;
	gboolean           single_subfolder;
	GthFileData       *example_data;
	GFile             *destination_example;
	char              *uri;
	char              *example;

	destination = gtk_file_chooser_get_current_folder_file (GTK_FILE_CHOOSER (GET_WIDGET ("destination_filechooserbutton")));
	if (destination == NULL)
		return;

	subfolder_type = gtk_combo_box_get_active (GTK_COMBO_BOX (data->subfolder_type_list));
	single_subfolder = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("single_subfolder_checkbutton")));

	example_data = create_example_file_data ();
	destination_example = gth_import_task_get_file_destination (example_data, destination, subfolder_type, single_subfolder);

	uri = g_file_get_uri (destination_example);
	example = g_strdup_printf (_("example: %s"), uri);
	gtk_label_set_text (GTK_LABEL (GET_WIDGET ("example_label")), example);

	gtk_widget_set_sensitive (GET_WIDGET ("single_subfolder_checkbutton"), subfolder_type != GTH_SUBFOLDER_TYPE_NONE);

	g_free (example);
	g_free (uri);
	g_object_unref (destination_example);
	g_object_unref (example_data);
	g_object_unref (destination);
}


static void
subfolder_type_list_changed_cb (GtkWidget  *widget,
				DialogData *data)
{
	update_destination (data);
}


static void
destination_selection_changed_cb (GtkWidget  *widget,
				  DialogData *data)
{
	update_destination (data);
}


static void
subfolder_hierarchy_checkbutton_toggled_cb (GtkWidget  *widget,
					    DialogData *data)
{
	update_destination (data);
}


static void
preferences_button_clicked_cb (GtkWidget  *widget,
			       DialogData *data)
{
	gtk_window_present (GTK_WINDOW (GET_WIDGET ("preferences_dialog")));
}


void
dlg_photo_importer (GthBrowser *browser,
		    GFile      *source)
{
	DialogData      *data;
	GtkCellRenderer *renderer;
	GthFileDataSort *sort_type;
	GList           *tests, *scan;
	char            *general_filter;
	int              i, active_filter;
	int              i_general;

	if (gth_browser_get_dialog (browser, "photo_importer") != NULL) {
		gtk_window_present (GTK_WINDOW (gth_browser_get_dialog (browser, "photo_importer")));
		return;
	}

	data = g_new0 (DialogData, 1);
	data->browser = browser;
	data->builder = _gtk_builder_new_from_file ("photo-importer.ui", "photo_importer");
	data->source = _g_object_ref (source);
	data->cancellable = g_cancellable_new ();
	data->vfs_source = g_object_new (GTH_TYPE_FILE_SOURCE_VFS, NULL);
	gth_file_source_monitor_entry_points (GTH_FILE_SOURCE (data->vfs_source));

	/* Get the widgets. */

	data->dialog = _gtk_builder_get_widget (data->builder, "photo_importer_dialog");
	gth_browser_set_dialog (browser, "photo_importer", data->dialog);
	g_object_set_data (G_OBJECT (data->dialog), "dialog_data", data);

	data->source_store = gtk_list_store_new (SOURCE_LIST_COLUMNS, G_TYPE_OBJECT, G_TYPE_ICON, G_TYPE_STRING);
	data->source_list = gtk_combo_box_new_with_model (GTK_TREE_MODEL (data->source_store));
	gtk_widget_show (data->source_list);
	gtk_box_pack_start (GTK_BOX (GET_WIDGET ("source_box")), data->source_list, TRUE, TRUE, 0);

	gtk_label_set_mnemonic_widget (GTK_LABEL (GET_WIDGET ("source_label")), data->source_list);

	g_object_unref (data->source_store);

	renderer = gtk_cell_renderer_pixbuf_new ();
	gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (data->source_list), renderer, FALSE);
	gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (data->source_list),
					renderer,
					"gicon", SOURCE_LIST_COLUMN_ICON,
					NULL);

	renderer = gtk_cell_renderer_text_new ();
	gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (data->source_list), renderer, TRUE);
	gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (data->source_list),
					renderer,
					"text", SOURCE_LIST_COLUMN_NAME,
					NULL);

	data->subfolder_type_list = _gtk_combo_box_new_with_texts (_("No subfolder"),
								   _("File date"),
								   _("Current date"),
								   NULL);
	gtk_combo_box_set_active (GTK_COMBO_BOX (data->subfolder_type_list), 0);
	gtk_widget_show (data->subfolder_type_list);
	gtk_box_pack_start (GTK_BOX (GET_WIDGET ("subfolder_type_box")), data->subfolder_type_list, TRUE, TRUE, 0);

	/*gtk_label_set_mnemonic_widget (GTK_LABEL (GET_WIDGET ("subfolder_label")), data->subfolder_type_list);*/

	data->file_list = gth_file_list_new (GTH_FILE_LIST_TYPE_SELECTOR);
	sort_type = gth_main_get_sort_type ("file::mtime");
	gth_file_list_set_sort_func (GTH_FILE_LIST (data->file_list), sort_type->cmp_func, FALSE);
	gth_file_list_enable_thumbs (GTH_FILE_LIST (data->file_list), TRUE);
	gth_file_list_set_thumb_size (GTH_FILE_LIST (data->file_list), 128);
	gth_file_list_set_caption (GTH_FILE_LIST (data->file_list), "standard::display-name,gth::file::display-size");

	gtk_widget_show (data->file_list);
	gtk_box_pack_start (GTK_BOX (GET_WIDGET ("filelist_box")), data->file_list, TRUE, TRUE, 0);

	/*gtk_label_set_mnemonic_widget (GTK_LABEL (GET_WIDGET ("files_label")), data->file_list);*/

	/**/

	tests = gth_main_get_all_tests ();
	general_filter = "file::type::is_media"; /* default value */
	active_filter = 0;

	data->filter_combobox = gtk_combo_box_new_text ();
	for (i = 0, i_general = -1, scan = tests; scan; scan = scan->next, i++) {
		const char *registered_test_id = scan->data;
		GthTest    *test;

		if (strncmp (registered_test_id, "file::type::", 12) != 0)
			continue;

		i_general += 1;
		test = gth_main_get_test (registered_test_id);
		if (strcmp (registered_test_id, general_filter) == 0) {
			active_filter = i_general;
			gth_file_list_set_filter (GTH_FILE_LIST (data->file_list), test);
		}

		data->general_tests = g_list_prepend (data->general_tests, g_strdup (gth_test_get_id (test)));
		gtk_combo_box_append_text (GTK_COMBO_BOX (data->filter_combobox), gth_test_get_display_name (test));
		g_object_unref (test);
	}
	data->general_tests = g_list_reverse (data->general_tests);

	gtk_combo_box_set_active (GTK_COMBO_BOX (data->filter_combobox), active_filter);
	gtk_widget_show (data->filter_combobox);
	gtk_container_add (GTK_CONTAINER (GET_WIDGET ("filter_box")), data->filter_combobox);

	gtk_label_set_mnemonic_widget (GTK_LABEL (GET_WIDGET ("filter_label")), data->filter_combobox);
	gtk_label_set_use_underline (GTK_LABEL (GET_WIDGET ("filter_label")), TRUE);

	_g_string_list_free (tests);

	{
		char  *last_destination;
		GFile *folder;

		last_destination = eel_gconf_get_string (PREF_PHOTO_IMPORT_DESTINATION, NULL);
		if ((last_destination == NULL) || (*last_destination == 0)) {
			char *default_path;

			default_path = xdg_user_dir_lookup ("PICTURES");
			folder = g_file_new_for_path (default_path);

			g_free (default_path);
		}
		else
			folder = g_file_new_for_uri (last_destination);

		gtk_file_chooser_set_current_folder_file (GTK_FILE_CHOOSER (GET_WIDGET ("destination_filechooserbutton")),
							  folder,
							  NULL);

		g_object_unref (folder);
		g_free (last_destination);
	}

	data->tags_entry = gth_tags_entry_new (NULL);
	gtk_widget_show (data->tags_entry);
	gtk_box_pack_start (GTK_BOX (GET_WIDGET ("tags_entry_box")), data->tags_entry, TRUE, TRUE, 0);
	gtk_label_set_mnemonic_widget (GTK_LABEL (GET_WIDGET ("tags_label")), data->tags_entry);

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("delete_checkbutton")), eel_gconf_get_boolean (PREF_PHOTO_IMPORT_DELETE, FALSE));
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("adjust_orientation_checkbutton")), eel_gconf_get_boolean (PREF_PHOTO_IMPORT_ADJUST_ORIENTATION, TRUE));
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("single_subfolder_checkbutton")), eel_gconf_get_boolean (PREF_PHOTO_IMPORT_SUBFOLDER_SINGLE, FALSE));
	gtk_combo_box_set_active (GTK_COMBO_BOX (data->subfolder_type_list), eel_gconf_get_enum (PREF_PHOTO_IMPORT_SUBFOLDER_TYPE, GTH_TYPE_SUBFOLDER_TYPE, GTH_SUBFOLDER_TYPE_FILE_DATE));
	update_destination (data);

	/* Set the signals handlers. */

	g_signal_connect (G_OBJECT (data->dialog),
			  "delete-event",
			  G_CALLBACK (delete_event_cb),
			  data);
	g_signal_connect (GET_WIDGET ("ok_button"),
			  "clicked",
			  G_CALLBACK (ok_clicked_cb),
			  data);
	g_signal_connect (GET_WIDGET ("cancel_button"),
			  "clicked",
			  G_CALLBACK (close_dialog),
			  data);
	g_signal_connect (data->source_list,
			  "changed",
			  G_CALLBACK (source_list_changed_cb),
			  data);
	g_signal_connect (data->filter_combobox,
			  "changed",
			  G_CALLBACK (filter_combobox_changed_cb),
			  data);
	g_signal_connect (GET_WIDGET ("select_all_button"),
			  "clicked",
			  G_CALLBACK (select_all_button_clicked_cb),
			  data);
	g_signal_connect (GET_WIDGET ("select_none_button"),
			  "clicked",
			  G_CALLBACK (select_none_button_clicked_cb),
			  data);
	g_signal_connect (gth_file_view_get_model (GTH_FILE_VIEW (gth_file_list_get_view (GTH_FILE_LIST (data->file_list)))),
			  "visibility_changed",
			  G_CALLBACK (file_store_changed_cb),
			  data);
	g_signal_connect (gth_file_view_get_model (GTH_FILE_VIEW (gth_file_list_get_view (GTH_FILE_LIST (data->file_list)))),
			  "check_changed",
			  G_CALLBACK (file_store_changed_cb),
			  data);
	g_signal_connect (data->subfolder_type_list,
			  "changed",
			  G_CALLBACK (subfolder_type_list_changed_cb),
			  data);
	g_signal_connect (GET_WIDGET ("destination_filechooserbutton"),
			  "selection_changed",
			  G_CALLBACK (destination_selection_changed_cb),
			  data);
	g_signal_connect (GET_WIDGET ("single_subfolder_checkbutton"),
			  "toggled",
			  G_CALLBACK (subfolder_hierarchy_checkbutton_toggled_cb),
			  data);
	g_signal_connect (GET_WIDGET ("preferences_button"),
			  "clicked",
			  G_CALLBACK (preferences_button_clicked_cb),
			  data);
	g_signal_connect (GET_WIDGET ("preferences_dialog"),
			  "delete-event",
			  G_CALLBACK (gtk_widget_hide_on_delete),
			  NULL);
	g_signal_connect_swapped (GET_WIDGET ("p_close_button"),
				  "clicked",
				  G_CALLBACK (gtk_widget_hide_on_delete),
				  GET_WIDGET ("preferences_dialog"));

	data->monitor_event = g_signal_connect (gth_main_get_default_monitor (),
						"entry_points_changed",
						G_CALLBACK (entry_points_changed_cb),
						data);

	/* Run dialog. */

	gtk_window_set_transient_for (GTK_WINDOW (data->dialog), GTK_WINDOW (browser));
	gtk_window_set_modal (GTK_WINDOW (data->dialog), FALSE);
	gtk_widget_show (data->dialog);

	update_source_list (data);
}
