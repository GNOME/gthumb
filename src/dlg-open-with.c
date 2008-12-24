/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2001 The Free Software Foundation, Inc.
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
#include <string.h>

#include <gtk/gtk.h>
#include <glade/glade.h>
#include <gio/gio.h>

#include "file-utils.h"
#include "gconf-utils.h"
#include "gtk-utils.h"
#include "typedefs.h"
#include "main.h"


#define GLADE_FILE "gthumb.glade"

enum { ICON_COLUMN, TEXT_COLUMN, DATA_COLUMN, N_COLUMNS };

typedef struct {
	GtkWindow    *window;
	GladeXML     *gui;

	GtkWidget    *dialog;
	GtkWidget    *app_list_tree_view;
	GtkWidget    *recent_list_tree_view;
	GtkWidget    *app_entry;
	GtkWidget    *del_button;

	GList        *apps;
	GList        *uris;

	GtkTreeModel *app_model;
	GtkTreeModel *recent_model;
} DialogData;


/* called when the main dialog is closed. */
static void
open_with__destroy_cb (GtkWidget  *widget, 
		       DialogData *data)
{
	g_object_unref (G_OBJECT (data->gui));

	if (data->uris != NULL)
		path_list_free (data->uris);

	if (data->apps != NULL)
		g_list_free (data->apps);

	g_free (data);
}


static void
open_cb (GtkWidget *widget,
	 gpointer   callback_data)
{
	DialogData  *data = callback_data;
	const gchar *application_utf8;
	char        *application;
	GList       *scan;
	GSList      *sscan, *editors;
	GAppInfo    *registered_app = NULL;
	gboolean     present = FALSE;
	GError      *error = NULL;
	const char  *command = NULL;

	application_utf8 = gtk_entry_get_text (GTK_ENTRY (data->app_entry));
	application = g_locale_from_utf8 (application_utf8, -1, NULL, NULL, NULL);

	/* add the command to the editors list if not already present. */

	for (scan = data->apps; scan && ! present; scan = scan->next) {
		GAppInfo *app = scan->data;
		if (strcmp (g_app_info_get_executable (app), application) == 0) {
			command = g_app_info_get_executable (app);
			present = TRUE;
			registered_app = app;
		}
	}

	editors = eel_gconf_get_locale_string_list (PREF_EDITORS);
	for (sscan = editors; sscan && ! present; sscan = sscan->next) {
		char *recent_command = sscan->data;
		if (strcmp (recent_command, application) == 0) {
			command = recent_command;
			present = TRUE;
		}
	}

	if (! present) {
		editors = g_slist_prepend (editors, g_strdup (application));
		eel_gconf_set_locale_string_list (PREF_EDITORS, editors);
		command = application;
	}

	/* exec the application. */

	if (registered_app != NULL) {
		g_app_info_launch_uris (registered_app, data->uris, NULL, &error);
		if (error == NULL)
			gtk_widget_destroy (data->dialog);
	} 
	else if ((command != NULL) && exec_command (command, data->uris))
		gtk_widget_destroy (data->dialog);

	g_free (application);
	g_slist_foreach (editors, (GFunc) g_free, NULL);
	g_slist_free (editors);
}


static void
app_list_selection_changed_cb (GtkTreeSelection *selection,
			       gpointer          p)
{
	DialogData              *data = p;
	GtkTreeIter              iter;
        GAppInfo                *app;

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (data->app_list_tree_view));
	if (selection == NULL)
		return;
	
	if (! gtk_tree_selection_get_selected (selection, NULL, &iter))
		return;

        gtk_tree_model_get (data->app_model, &iter,
                            DATA_COLUMN, &app,
                            -1);
	_gtk_entry_set_locale_text (GTK_ENTRY (data->app_entry), g_app_info_get_executable (app));
}


static void
app_activated_cb (GtkTreeView       *tree_view,
                  GtkTreePath       *path,
                  GtkTreeViewColumn *column,
                  gpointer           callback_data)
{
        DialogData *data = callback_data;
	open_cb (NULL, data);
}


static void
recent_list_selection_changed_cb (GtkTreeSelection *selection,
				  gpointer          p)
{
	DialogData   *data = p;
	GtkTreeIter   iter;
	char         *editor;

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (data->recent_list_tree_view));
	if (selection == NULL)
		return;

	if (! gtk_tree_selection_get_selected (selection, NULL, &iter))
		return;

	gtk_tree_model_get (data->recent_model, &iter,
                            0, &editor,
                            -1);
	_gtk_entry_set_locale_text (GTK_ENTRY (data->app_entry), editor);
        g_free (editor);
}


static void
recent_activated_cb (GtkTreeView       *tree_view,
		     GtkTreePath       *path,
		     GtkTreeViewColumn *column,
		     gpointer           callback_data)
{
        DialogData   *data = callback_data;
	open_cb (NULL, data);
}


static void
delete_recent_cb (GtkWidget *widget,
                  gpointer   callback_data)
{
	DialogData       *data = callback_data;
	GtkTreeSelection *selection;
	GtkTreeIter       iter;
	gchar            *editor;
	GSList           *link, *editors;

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (data->recent_list_tree_view));
	if (! gtk_tree_selection_get_selected (selection, NULL, &iter))
		return;

	gtk_tree_model_get (data->recent_model, &iter,
			    0, &editor,
			    -1);
	gtk_list_store_remove (GTK_LIST_STORE (data->recent_model), &iter);

	editors = eel_gconf_get_locale_string_list (PREF_EDITORS);
	link = g_slist_find_custom (editors, editor, (GCompareFunc) strcmp);
	g_free (editor);

	g_return_if_fail (link != NULL);

	editors = g_slist_remove_link (editors, link);
	g_free (link->data);
	g_slist_free (link);

	eel_gconf_set_locale_string_list (PREF_EDITORS, editors);
	g_slist_foreach (editors, (GFunc) g_free, NULL);
	g_slist_free (editors);
}


/* create the "open with" dialog. */
void
dlg_open_with (GtkWindow  *window,
	       GList      *uris)
{
	DialogData              *data;
	GList                   *scan;
	GSList                  *sscan, *editors;
	GAppInfo                *app;
	GList                   *app_names = NULL;
	GtkWidget               *ok_btn, *cancel_btn;
	GtkTreeIter              iter;
	GtkCellRenderer         *renderer;
	GtkTreeViewColumn       *column;
	GtkIconTheme            *theme;
	int                      icon_size;

	data = g_new (DialogData, 1);

	data->uris = uris;
	data->window = window;
	data->gui = glade_xml_new (GTHUMB_GLADEDIR "/" GLADE_FILE, NULL, NULL);
        if (! data->gui) {
                g_warning ("Could not find " GLADE_FILE "\n");

		g_free (data);
		path_list_free (uris);
                return;
        }

	/* Get the widgets. */

	data->dialog = glade_xml_get_widget (data->gui, "open_with_dialog");
	data->app_list_tree_view = glade_xml_get_widget (data->gui, "app_list_treeview");
	data->recent_list_tree_view = glade_xml_get_widget (data->gui, "recent_treeview");
	ok_btn = glade_xml_get_widget (data->gui, "open_with_ok_btn");
	cancel_btn = glade_xml_get_widget (data->gui, "open_with_cancel_btn");
	data->app_entry = glade_xml_get_widget (data->gui, "open_with_app_entry");
	data->del_button = glade_xml_get_widget (data->gui, "delete_button");

	/* Set the signals handlers. */

	g_signal_connect (G_OBJECT (data->dialog), 
			  "destroy",
			  G_CALLBACK (open_with__destroy_cb),
			  data);
	g_signal_connect (G_OBJECT (gtk_tree_view_get_selection (GTK_TREE_VIEW (data->app_list_tree_view))),
                          "changed",
                          G_CALLBACK (app_list_selection_changed_cb),
                          data);
	g_signal_connect (G_OBJECT (data->app_list_tree_view),
                          "row_activated",
                          G_CALLBACK (app_activated_cb),
                          data);

	g_signal_connect (G_OBJECT (gtk_tree_view_get_selection (GTK_TREE_VIEW (data->recent_list_tree_view))),
                          "changed",
                          G_CALLBACK (recent_list_selection_changed_cb),
                          data);
	g_signal_connect (G_OBJECT (data->recent_list_tree_view),
                          "row_activated",
                          G_CALLBACK (recent_activated_cb),
                          data);

	g_signal_connect_swapped (G_OBJECT (cancel_btn), 
				  "clicked",
				  G_CALLBACK (gtk_widget_destroy),
				  G_OBJECT (data->dialog));
	g_signal_connect (G_OBJECT (ok_btn), 
			  "clicked",
			  G_CALLBACK (open_cb),
			  data);
	g_signal_connect_swapped (G_OBJECT (cancel_btn), 
				  "clicked",
				  G_CALLBACK (gtk_widget_destroy),
				  G_OBJECT (data->dialog));
	g_signal_connect (G_OBJECT (data->del_button), 
			  "clicked",
			  G_CALLBACK (delete_recent_cb),
			  data);

	/* Set data. */

	/* * registered editors list. */

	data->apps = NULL;
	for (scan = data->uris; scan; scan = scan->next) {
		const char *uri = scan->data;
		const char *result;

		result = get_file_mime_type (uri, FALSE);
		if (result != NULL)
			data->apps = g_list_concat (data->apps, g_app_info_get_all_for_type (result));
	}

	data->app_model = GTK_TREE_MODEL (gtk_list_store_new (N_COLUMNS, 
							      GDK_TYPE_PIXBUF,
							      G_TYPE_STRING,
							      G_TYPE_POINTER));

	gtk_tree_view_set_model (GTK_TREE_VIEW (data->app_list_tree_view),
				 data->app_model);
	g_object_unref (G_OBJECT (data->app_model));

	theme = gtk_icon_theme_get_default ();
	icon_size = get_folder_pixbuf_size_for_list (GTK_WIDGET (window));

	for (scan = data->apps; scan; scan = scan->next) {
		gboolean    found;
		char        *utf8_name;
		GThemedIcon *ticon;
		GStrv       icon_names;
		GdkPixbuf   *icon;

		app = scan->data;

		found = FALSE;
		if (app_names != NULL) {
			GList *p;
			for (p = app_names; p && !found; p = p->next)
				if (strcmp ((char*)p->data, g_app_info_get_executable (app)) == 0)
					found = TRUE;
		}

		if (found)
			continue;

		/* do not include gthumb itself */
		if (strncmp (g_app_info_get_executable (app), "gthumb", 6) == 0)
			continue;

		app_names = g_list_prepend (app_names, (char*)g_app_info_get_executable (app));
		
		gtk_list_store_append (GTK_LIST_STORE (data->app_model),
				       &iter);

		utf8_name = g_locale_to_utf8 (g_app_info_get_name (app), -1, NULL, NULL, NULL);
		ticon = G_THEMED_ICON (g_app_info_get_icon (app));
		g_object_get (ticon, "names", &icon_names, NULL);
		icon = create_pixbuf (theme, icon_names[0], icon_size);		
		gtk_list_store_set (GTK_LIST_STORE (data->app_model), &iter,
				    ICON_COLUMN, icon,
				    TEXT_COLUMN, utf8_name,
				    DATA_COLUMN, app,
				    -1);
		g_free (utf8_name);
		g_free (icon_names);
		g_object_unref (ticon);
	}

	column = gtk_tree_view_column_new ();

	renderer = gtk_cell_renderer_pixbuf_new ();
	gtk_tree_view_column_pack_start (column, renderer, FALSE);
	gtk_tree_view_column_set_attributes (column, renderer,
					     "pixbuf", ICON_COLUMN,
					     NULL);

	renderer = gtk_cell_renderer_text_new ();
	gtk_tree_view_column_pack_start (column,
                                         renderer,
                                         TRUE);
        gtk_tree_view_column_set_attributes (column, renderer,
                                             "text", TEXT_COLUMN,
                                             NULL);

	gtk_tree_view_append_column (GTK_TREE_VIEW (data->app_list_tree_view),
				     column);

	if (app_names) 
		g_list_free (app_names);

	/* * recent editors list. */

	data->recent_model = GTK_TREE_MODEL (gtk_list_store_new (2, G_TYPE_STRING, G_TYPE_STRING));
	gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (data->recent_model), 1, GTK_SORT_ASCENDING);
	
	gtk_tree_view_set_model (GTK_TREE_VIEW (data->recent_list_tree_view),
				 data->recent_model);
	g_object_unref (G_OBJECT (data->recent_model));

	editors = eel_gconf_get_locale_string_list (PREF_EDITORS);
	for (sscan = editors; sscan; sscan = sscan->next) {
		char *editor = sscan->data;
		char *utf8;

		gtk_list_store_append (GTK_LIST_STORE (data->recent_model), &iter);
		utf8 = g_locale_to_utf8 (editor, -1, 0, 0, 0);
		gtk_list_store_set (GTK_LIST_STORE (data->recent_model), &iter,
				    0, editor,
				    1, utf8,
				    -1);
		g_free (utf8);
	}

	g_slist_foreach (editors, (GFunc) g_free, NULL);
	g_slist_free (editors);

	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes (NULL,
							   renderer,
							   "text", 1,
							   NULL);
	gtk_tree_view_column_set_sort_column_id (column, 0);
	gtk_tree_view_append_column (GTK_TREE_VIEW (data->recent_list_tree_view),
				     column);

	/* Run dialog. */

	gtk_window_set_transient_for (GTK_WINDOW (data->dialog), window);
	gtk_window_set_modal (GTK_WINDOW (data->dialog), TRUE);
	gtk_widget_show_all (data->dialog);
}
