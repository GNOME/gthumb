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
#include "gth-import-task.h"


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
} DialogData;


static void
destroy_dialog (gpointer user_data)
{
	DialogData *data = user_data;

	g_signal_handler_disconnect (gth_main_get_default_monitor (), data->monitor_event);

	if (data->import) {
/*
		GthTask *task;

		task = gth_import_task_new (destination, subfolder, categories, delete);
		gth_browser_exec_task (data->browser, task, FALSE);
*/
	}

	gtk_widget_destroy (data->dialog);
	gth_browser_set_dialog (data->browser, "photo_importer", NULL);

	g_object_unref (data->vfs_source);
	g_object_unref (data->builder);
	_g_object_unref (data->source);
	_g_object_unref (data->cancellable);
	_g_object_list_unref (data->files);
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
	data->done_func = done_func;

	if (data->cancelling)
		return;

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
list_ready_cb (GList    *files,
	       GError   *error,
	       gpointer  user_data)
{
	DialogData *data = user_data;

	data->loading_list = FALSE;

	if (data->cancelling) {
		g_print ("...CANCELED\n");
		gth_file_list_cancel (GTH_FILE_LIST (data->file_list), cancel_done, data);
		return;
	}

	g_print ("...DONE\n");

	if (error != NULL) {
		_gtk_error_dialog_from_gerror_show (GTK_WINDOW (data->dialog), _("Could not load the folder"), &error);
		return;
	}

	data->files = _g_object_list_ref (files);
	gth_file_list_set_files (GTH_FILE_LIST (data->file_list), data->files);
}


static void
list_source_files (gpointer user_data)
{
	DialogData *data = user_data;
	GList      *list;

	g_print ("LOADING...\n");

	_g_object_list_unref (data->files);
	data->files = NULL;

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
		gth_file_list_clear (GTH_FILE_LIST (data->file_list), _("(Empty)"));
		return;
	}

	gtk_tree_model_get (GTK_TREE_MODEL (data->source_store), &iter,
			    SOURCE_LIST_COLUMN_VOLUME, &volume,
			    -1);

	if (volume == NULL) {
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
filter_checkbutton_toggled_cb (GtkToggleButton *togglebutton,
			       gpointer         user_data)
{
	DialogData *data = user_data;
	GthTest    *test = NULL;

	if (! gtk_toggle_button_get_active (togglebutton))
		test = gth_main_get_test ("file::type::is_media");

	gth_file_list_set_filter (GTH_FILE_LIST (data->file_list), test);

	_g_object_unref (test);
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
					gtk_combo_box_set_active_iter (GTK_COMBO_BOX (data->source_list), &iter);
					source_available = TRUE;
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
		source_list_changed_cb (NULL, data);
	}

	_g_object_list_unref (mounts);
}


static void
entry_points_changed_cb (GthMonitor *monitor,
			 DialogData *data)
{
	update_source_list (data);
}


void
dlg_photo_importer (GthBrowser *browser,
		    GFile      *source)
{
	DialogData      *data;
	GtkCellRenderer *renderer;
	GthFileDataSort *sort_type;
	GthTest         *test;

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

	update_source_list (data);

	data->subfolder_type_list = _gtk_combo_box_new_with_texts (_("No subfolder"),
								   _("Day photo taken"),
								   _("Month photo taken"),
								   _("Current date"),
								   _("Current date and time"),
								   _("Custom"),
								   NULL);
	gtk_combo_box_set_active (GTK_COMBO_BOX (data->subfolder_type_list), 0);
	gtk_widget_show (data->subfolder_type_list);
	gtk_box_pack_start (GTK_BOX (GET_WIDGET ("subfolder_type_box")), data->subfolder_type_list, TRUE, TRUE, 0);

	gtk_label_set_mnemonic_widget (GTK_LABEL (GET_WIDGET ("subfolder_label")), data->subfolder_type_list);

	data->file_list = gth_file_list_new ();
	sort_type = gth_main_get_sort_type ("file::mtime");
	gth_file_list_set_sort_func (GTH_FILE_LIST (data->file_list), sort_type->cmp_func, FALSE);
	gth_file_list_enable_thumbs (GTH_FILE_LIST (data->file_list), TRUE);
	gth_file_list_set_thumb_size (GTH_FILE_LIST (data->file_list), 128);
	gth_file_list_set_caption (GTH_FILE_LIST (data->file_list), "standard::display-name,gth::file::display-size");

	test = gth_main_get_test ("file::type::is_media");
	gth_file_list_set_filter (GTH_FILE_LIST (data->file_list), test);
	g_object_unref (test);

	gtk_widget_show (data->file_list);
	gtk_box_pack_start (GTK_BOX (GET_WIDGET ("filelist_box")), data->file_list, TRUE, TRUE, 0);

	/* Set the signals handlers. */

	g_signal_connect (G_OBJECT (data->dialog),
			  "delete-event",
			  G_CALLBACK (delete_event_cb),
			  data);
	/*g_signal_connect (G_OBJECT (data->dialog),
			  "destroy",
			  G_CALLBACK (destroy_cb),
			  data);*/
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
	g_signal_connect (GET_WIDGET ("filter_checkbutton"),
			  "toggled",
			  G_CALLBACK (filter_checkbutton_toggled_cb),
			  data);

	data->monitor_event = g_signal_connect (gth_main_get_default_monitor (),
						"entry_points_changed",
						G_CALLBACK (entry_points_changed_cb),
						data);

	/* Run dialog. */

	gtk_window_set_transient_for (GTK_WINDOW (data->dialog), GTK_WINDOW (browser));
	gtk_window_set_modal (GTK_WINDOW (data->dialog), FALSE);
	gtk_widget_show (data->dialog);

	load_file_list (data);
}
