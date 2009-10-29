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
	GtkWidget     *parent_window;
	GtkWidget     *dialog;
	GList         *files;
	gboolean       view_destination;
	GFile         *catalog_file;
	GthCatalog    *catalog;
	char          *buffer;
	gsize          length;
} AddData;


static void
add_data_free (AddData *add_data)
{
	g_free (add_data->buffer);
	_g_object_unref (add_data->catalog);
	_g_object_list_unref (add_data->files);
	_g_object_unref (add_data->catalog_file);
	g_free (add_data);
}


typedef struct {
	GthBrowser *browser;
	GtkBuilder *builder;
	GtkWidget  *dialog;
	GtkWidget  *source_tree;
	AddData    *add_data;
} DialogData;


static void
destroy_cb (GtkWidget  *widget,
	    DialogData *data)
{
	g_object_unref (data->builder);
	g_free (data);
}


static GFile *
get_selected_catalog (DialogData *data)
{
	GthFileData *file_data;
	GFile       *file;

	file_data = gth_folder_tree_get_selected_or_parent (GTH_FOLDER_TREE (data->source_tree));
	if ((file_data != NULL) && ! g_file_info_get_attribute_boolean (file_data->info, "gthumb::no-child")) {
		_g_object_unref (file_data);
		file_data = NULL;
	}
	if (file_data != NULL)
		file = g_object_ref (file_data->file);
	else
		file = NULL;

	_g_object_unref (file_data);

	return file;
}


static void
catalog_save_done_cb (void     *buffer,
		      gsize     count,
		      GError   *error,
		      gpointer  user_data)
{
	AddData *add_data = user_data;

	if (error != NULL) {
		_gtk_error_dialog_from_gerror_show (GTK_WINDOW (add_data->parent_window), _("Could not add the files to the catalog"), &error);
		return;
	}

	gth_monitor_folder_changed (gth_main_get_default_monitor (),
				    add_data->catalog_file,
				    add_data->files,
				    GTH_MONITOR_EVENT_CREATED);

	if (add_data->view_destination)
		gth_browser_go_to (add_data->browser, add_data->catalog_file, NULL);

	if (add_data->dialog != NULL)
		gtk_widget_destroy (add_data->dialog);

	add_data_free (add_data);
}


static void
catalog_ready_cb (GObject  *catalog,
		  GError   *error,
		  gpointer  user_data)
{
	AddData *add_data = user_data;
	GList   *scan;
	GFile   *gio_file;

	if (error != NULL) {
		_gtk_error_dialog_from_gerror_show (GTK_WINDOW (add_data->parent_window), _("Could not add the files to the catalog"), &error);
		return;
	}

	add_data->catalog = (GthCatalog *) catalog;

	for (scan = add_data->files; scan; scan = scan->next) {
		GthFileData *file_to_add = scan->data;
		gth_catalog_insert_file (add_data->catalog, -1, file_to_add->file);
	}

	add_data->buffer = gth_catalog_to_data (add_data->catalog, &add_data->length);
	gio_file = gth_catalog_file_to_gio_file (add_data->catalog_file);
	g_write_file_async (gio_file,
			    add_data->buffer,
			    add_data->length,
			    G_PRIORITY_DEFAULT,
			    NULL,
			    catalog_save_done_cb,
			    add_data);

	g_object_unref (gio_file);
}


static void
add_data_exec (AddData *add_data)
{
	gth_catalog_load_from_file (add_data->catalog_file,
				    NULL,
				    catalog_ready_cb,
				    add_data);
}


static void
add_button_clicked_cb (GtkWidget  *widget,
		       DialogData *data)
{
	data->add_data->catalog_file = get_selected_catalog (data);
	if (data->add_data->catalog_file == NULL)
		return;

	data->add_data->view_destination = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("view_destination_checkbutton")));
	add_data_exec (data->add_data);
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
	DialogData *data = user_data;
	GFile      *selected_catalog;

	selected_catalog = get_selected_catalog (data);
	gtk_widget_set_sensitive (GTK_WIDGET (GET_WIDGET ("add_button")), selected_catalog != NULL);

	_g_object_unref (selected_catalog);
}


static void
new_catalog_button_clicked_cb (GtkWidget  *widget,
		       	       DialogData *data)
{
	char          *name;
	GthFileData   *selected_parent;
	GFile         *parent;
	GthFileSource *file_source;
	GFile         *gio_parent;
	GError        *error;
	GFile         *gio_file;

	name = _gtk_request_dialog_run (GTK_WINDOW (data->dialog),
				        GTK_DIALOG_MODAL,
				        _("Enter the catalog name: "),
				        "",
				        1024,
				        GTK_STOCK_CANCEL,
				        _("C_reate"));
	if (name == NULL)
		return;

	selected_parent = gth_folder_tree_get_selected_or_parent (GTH_FOLDER_TREE (data->source_tree));
	if (selected_parent != NULL) {
		GthFileSource *file_source;
		GFileInfo     *info;

		file_source = gth_main_get_file_source (selected_parent->file);
		info = gth_file_source_get_file_info (file_source, selected_parent->file, GFILE_BASIC_ATTRIBUTES);
		if (g_file_info_get_attribute_boolean (info, "gthumb::no-child"))
			parent = g_file_get_parent (selected_parent->file);
		else
			parent = g_file_dup (selected_parent->file);

		g_object_unref (info);
		g_object_unref (file_source);
	}
	else
		parent = g_file_new_for_uri ("catalog:///");

	file_source = gth_main_get_file_source (parent);
	gio_parent = gth_file_source_to_gio_file (file_source, parent);
	gio_file = _g_file_create_unique (gio_parent, name, ".catalog", &error);
	if (gio_file != NULL) {
		GFile *file;
		GList *list;

		file = gth_catalog_file_from_gio_file (gio_file, NULL);
		list = g_list_prepend (NULL, file);
		gth_monitor_folder_changed (gth_main_get_default_monitor (),
					    parent,
					    list,
					    GTH_MONITOR_EVENT_CREATED);

		g_list_free (list);
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
	char          *name;
	GthFileData   *selected_parent;
	GFile         *parent;
	GthFileSource *file_source;
	GFile         *gio_parent;
	GError        *error = NULL;
	GFile         *gio_file;

	name = _gtk_request_dialog_run (GTK_WINDOW (data->dialog),
					GTK_DIALOG_MODAL,
					_("Enter the library name: "),
					"",
					1024,
					GTK_STOCK_CANCEL,
					_("C_reate"));
	if (name == NULL)
		return;

	selected_parent = gth_folder_tree_get_selected_or_parent (GTH_FOLDER_TREE (data->source_tree));
	if (selected_parent != NULL) {
		GthFileSource *file_source;
		GFileInfo     *info;

		file_source = gth_main_get_file_source (selected_parent->file);
		info = gth_file_source_get_file_info (file_source, selected_parent->file, GFILE_BASIC_ATTRIBUTES);
		if (g_file_info_get_attribute_boolean (info, "gthumb::no-child"))
			parent = g_file_get_parent (selected_parent->file);
		else
			parent = g_file_dup (selected_parent->file);

		g_object_unref (info);
		g_object_unref (file_source);
	}
	else
		parent = g_file_new_for_uri ("catalog:///");

	file_source = gth_main_get_file_source (parent);
	gio_parent = gth_file_source_to_gio_file (file_source, parent);
	gio_file = _g_directory_create_unique (gio_parent, name, "", &error);
	if (gio_file != NULL) {
		GFile *file;
		GList *list;

		file = gth_catalog_file_from_gio_file (gio_file, NULL);
		list = g_list_prepend (NULL, file);
		gth_monitor_folder_changed (gth_main_get_default_monitor (),
					    parent,
					    list,
					    GTH_MONITOR_EVENT_CREATED);

		g_list_free (list);
		g_object_unref (file);
	}
	else
		_gtk_error_dialog_from_gerror_show (GTK_WINDOW (data->dialog), _("Could not create the library"), &error);

	g_object_unref (gio_file);
	g_object_unref (gio_parent);
	g_object_unref (file_source);

	/*

	selected_catalog = gth_folder_tree_get_selected (GTH_FOLDER_TREE (data->source_tree));
	parent = get_catalog_parent (selected_catalog->file);
	new_library = g_file_get_child_for_display_name (parent, name, &error);

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
	g_free (name);

	*/
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
	data->dialog = _gtk_builder_get_widget (data->builder, "add_to_catalog_dialog");

	data->add_data = g_new0 (AddData, 1);
	data->add_data->browser = browser;
	data->add_data->parent_window = data->add_data->dialog = data->dialog;
	data->add_data->files = _g_object_list_ref (list);

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


void
add_to_catalog (GthBrowser *browser,
		GFile      *catalog,
		GList      *list)
{
	AddData *add_data;

	add_data = g_new0 (AddData, 1);
	add_data->browser = browser;
	add_data->parent_window = (GtkWidget *) browser;
	add_data->catalog_file = g_object_ref (catalog);
	add_data->files = _g_object_list_ref (list);

	add_data_exec (add_data);
}
