/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2001-2009 The Free Software Foundation, Inc.
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
#include <gthumb.h>
#include "gth-catalog.h"
#include "gth-file-source-catalogs.h"


#define GET_WIDGET(name) _gtk_builder_get_widget (data->builder, (name))


typedef struct {
	GthBrowser    *browser;
	GtkBuilder    *builder;
	GtkWidget     *dialog;
	GtkWidget     *source_tree;
	GList         *files;
	GthFileData   *selected_catalog;
	GthFileSource *file_source;
	GFile         *gio_file;
	GthCatalog    *catalog;
	char          *buffer;
	gsize          length;
} DialogData;


static void
destroy_cb (GtkWidget  *widget,
	    DialogData *data)
{
	g_free (data->buffer);
	_g_object_unref (data->catalog);
	_g_object_unref (data->gio_file);
	_g_object_list_unref (data->files);
	_g_object_unref (data->selected_catalog);
	g_object_unref (data->builder);
	g_object_unref (data->file_source);
	g_free (data);
}


static GthFileData *
get_selected_catalog (DialogData *data)
{
	GthFileData *file_data = NULL;
	GFile       *file;

	file = gth_folder_tree_get_selected_or_parent (GTH_FOLDER_TREE (data->source_tree));
	if (file != NULL) {
		GthFileSource *file_source;
		GFileInfo     *info;

		file_source = gth_main_get_file_source (file);
		info = gth_file_source_get_file_info (file_source, file);
		if (g_file_info_get_attribute_boolean (info, "gthumb::no-child"))
			file_data = gth_file_data_new (file, info);

		g_object_unref (info);
		g_object_unref (file);
	}

	return file_data;
}


static void
catalog_save_done_cb (void     *buffer,
		      gsize     count,
		      GError   *error,
		      gpointer  user_data)
{
	DialogData *data = user_data;

	if (error != NULL) {
		_gtk_error_dialog_from_gerror_show (GTK_WINDOW (data->dialog), _("Could not add the files to the catalog"), &error);
		return;
	}

	gth_monitor_folder_changed (gth_main_get_default_monitor (),
				    data->selected_catalog->file,
				    data->files,
				    GTH_MONITOR_EVENT_CREATED);

	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("view_destination_checkbutton"))))
		gth_browser_go_to (data->browser, data->selected_catalog->file);

	gtk_widget_destroy (data->dialog);
}


static void
catalog_ready_cb (GObject  *catalog,
		  GError   *error,
		  gpointer  user_data)
{
	DialogData *data = user_data;
	GList      *scan;

	if (error != NULL) {
		_gtk_error_dialog_from_gerror_show (GTK_WINDOW (data->dialog), _("Could not add the files to the catalog"), &error);
		return;
	}

	data->catalog = (GthCatalog *) catalog;

	for (scan = data->files; scan; scan = scan->next) {
		GthFileData *file_to_add = scan->data;
		gth_catalog_insert_file (data->catalog, -1, file_to_add->file);
	}

	data->buffer = gth_catalog_to_data (data->catalog, &data->length);
	g_write_file_async (data->gio_file,
			    data->buffer,
			    data->length,
			    G_PRIORITY_DEFAULT,
			    NULL,
			    catalog_save_done_cb,
			    data);
}


static void
add_button_clicked_cb (GtkWidget  *widget,
		       DialogData *data)
{
	data->selected_catalog = get_selected_catalog (data);
	if (data->selected_catalog == NULL)
		return;

	gth_catalog_load_from_file (data->selected_catalog->file,
				    catalog_ready_cb,
				    data);
}


static void
source_tree_open_cb (GthFolderTree *folder_tree,
		     GFile         *file,
                     gpointer       user_data)
{
	add_button_clicked_cb (NULL, (DialogData *)user_data);
}


static void
source_tree_selection_changed_cb (GtkTreeSelection *treeselection,
                                  gpointer          user_data)
{
	DialogData  *data = user_data;
	GthFileData *file_data;

	file_data = get_selected_catalog (data);
	gtk_widget_set_sensitive (GTK_WIDGET (GET_WIDGET ("add_button")), file_data != NULL);
	_g_object_unref (file_data);
}


static GFile *
get_catalog_parent (GFile *selected_parent)
{
	GFile *parent = NULL;

	if (selected_parent != NULL) {
		GthFileSource *file_source;
		GFileInfo     *info;

		file_source = gth_main_get_file_source (selected_parent);
		info = gth_file_source_get_file_info (file_source, selected_parent);
		if ((g_file_info_get_file_type (info) == G_FILE_TYPE_DIRECTORY) &&
		    ! g_file_info_get_attribute_boolean (info, "gthumb::no-child"))
		{
			parent = g_file_dup (selected_parent);
		}
		else
			parent = g_file_get_parent (selected_parent);

		g_object_unref (info);
		g_object_unref (file_source);
	}
	else
		parent = g_file_new_for_uri ("catalog:///");

	return parent;
}


static void
new_catalog_button_clicked_cb (GtkWidget  *widget,
		       	       DialogData *data)
{
	GFile         *selected_parent;
	GFile         *parent;
	GthFileSource *file_source;
	GFile         *gio_parent;
	GError        *error;
	GFile         *gio_file;

	selected_parent = gth_folder_tree_get_selected_or_parent (GTH_FOLDER_TREE (data->source_tree));
	if (selected_parent != NULL) {
		GthFileSource *file_source;
		GFileInfo     *info;

		file_source = gth_main_get_file_source (selected_parent);
		info = gth_file_source_get_file_info (file_source, selected_parent);
		if (g_file_info_get_attribute_boolean (info, "gthumb::no-child"))
			parent = g_file_get_parent (selected_parent);
		else
			parent = g_file_dup (selected_parent);

		g_object_unref (info);
		g_object_unref (file_source);
	}
	else
		parent = g_file_new_for_uri ("catalog:///");

	file_source = gth_main_get_file_source (parent);
	gio_parent = gth_file_source_to_gio_file (file_source, parent);
	gio_file = _g_file_create_unique (gio_parent, _("New Catalog"), ".catalog", &error);
	if (gio_file != NULL) {
		GFile        *file;
		GList        *list;
		GFileInfo    *info;
		GthFileData  *file_data;
		GList        *file_data_list;

		file = gth_catalog_file_from_gio_file (gio_file, NULL);
		info = gth_file_source_get_file_info (file_source, file);
		file_data = gth_file_data_new (file, info);
		file_data_list = g_list_prepend (NULL, file_data);
		gth_folder_tree_add_children (GTH_FOLDER_TREE (data->source_tree), parent, file_data_list);
		gth_folder_tree_start_editing (GTH_FOLDER_TREE (data->source_tree), file);

		list = g_list_prepend (NULL, g_object_ref (file));
		gth_monitor_folder_changed (gth_main_get_default_monitor (),
					    parent,
					    list,
					    GTH_MONITOR_EVENT_CREATED);

		_g_object_list_unref (list);
		g_list_free (file_data_list);
		g_object_unref (file_data);
		g_object_unref (info);
		g_object_unref (file);
	}
	else
		_gtk_error_dialog_from_gerror_show (GTK_WINDOW (data->dialog), _("Could not create the catalog"), &error);

	g_object_unref (gio_file);
	g_object_unref (gio_parent);
	g_object_unref (file_source);
}


static void
new_library_button_clicked_cb (GtkWidget  *widget,
		       	       DialogData *data)
{
	char   *display_name;
	GFile  *selected_catalog;
	GFile  *parent;
	GFile  *new_library;
	GError *error = NULL;

	display_name = _gtk_request_dialog_run (GTK_WINDOW (data->dialog),
						GTK_DIALOG_MODAL,
						_("Enter the library name: "),
						"",
						1024,
						GTK_STOCK_CANCEL,
						_("C_reate"));
	if (display_name == NULL)
		return;

	selected_catalog = gth_folder_tree_get_selected (GTH_FOLDER_TREE (data->source_tree));
	parent = get_catalog_parent (selected_catalog);
	new_library = g_file_get_child_for_display_name (parent, display_name, &error);

	if ((new_library != NULL) && (strchr (display_name, '/') != NULL)) {
		error = g_error_new (G_IO_ERROR, G_IO_ERROR_INVALID_FILENAME, _("The name \"%s\" is not valid because it contains the character \"/\". " "Please use a different name."), display_name);
		g_object_unref (new_library);
		new_library = NULL;
	}

	if (error == NULL) {
		GFile *gio_file;

		gio_file = gth_file_source_to_gio_file (data->file_source, new_library);
		g_file_make_directory (new_library, NULL, &error);

		g_object_unref (gio_file);
	}

	if (error != NULL)
		_gtk_error_dialog_from_gerror_show (GTK_WINDOW (data->dialog), _("Could not create a new library"), &error);

	if (new_library != NULL)
		g_object_unref (new_library);
	g_object_unref (parent);
	g_object_unref (selected_catalog);
	g_free (display_name);
}


void
dlg_add_to_catalog (GthBrowser *browser,
		    GList      *list)
{
	DialogData       *data;
	GFile            *base;
	GtkTreeSelection *selection;

	data = g_new0 (DialogData, 1);
	data->browser = browser;
	data->builder = _gtk_builder_new_from_file ("add-to-catalog.ui", "catalogs");
	data->files = _g_object_list_ref (list);
	data->file_source = g_object_new (GTH_TYPE_FILE_SOURCE_CATALOGS, NULL);

	data->dialog = _gtk_builder_get_widget (data->builder, "add_to_catalog_dialog");

	base = g_file_new_for_uri ("catalog:///");
	data->source_tree = gth_source_tree_new (base);
	g_object_unref (base);

	gtk_widget_show (data->source_tree);
	gtk_container_add (GTK_CONTAINER (GET_WIDGET ("catalog_list_scrolled_window")), data->source_tree);

	gtk_label_set_mnemonic_widget (GTK_LABEL (GET_WIDGET ("catalogs_label")), data->source_tree);
	gtk_widget_set_sensitive (GTK_WIDGET (GET_WIDGET ("add_button")), FALSE);

	/* Set the signals handlers. */

	g_signal_connect (G_OBJECT (data->dialog),
			  "destroy",
			  G_CALLBACK (destroy_cb),
			  data);
	g_signal_connect_swapped (G_OBJECT (GET_WIDGET ("cancel_button")),
				  "clicked",
				  G_CALLBACK (gtk_widget_destroy),
				  G_OBJECT (data->dialog));
	g_signal_connect (G_OBJECT (data->source_tree),
			  "open",
			  G_CALLBACK (source_tree_open_cb),
			  data);
	g_signal_connect (G_OBJECT (GET_WIDGET ("add_button")),
			  "clicked",
			  G_CALLBACK (add_button_clicked_cb),
			  data);
	g_signal_connect (G_OBJECT (GET_WIDGET ("new_catalog_button")),
			  "clicked",
			  G_CALLBACK (new_catalog_button_clicked_cb),
			  data);
	g_signal_connect (G_OBJECT (GET_WIDGET ("new_library_button")),
			  "clicked",
			  G_CALLBACK (new_library_button_clicked_cb),
			  data);

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (data->source_tree));
	g_signal_connect (selection,
			  "changed",
			  G_CALLBACK (source_tree_selection_changed_cb),
			  data);

	/* run dialog. */

	gtk_window_set_transient_for (GTK_WINDOW (data->dialog), GTK_WINDOW (browser));
	gtk_window_set_modal (GTK_WINDOW (data->dialog), TRUE);
	gtk_widget_show (data->dialog);
}
