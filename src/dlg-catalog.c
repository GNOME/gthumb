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
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <glade/glade.h>

#include "typedefs.h"
#include "catalog.h"
#include "catalog-list.h"
#include "file-utils.h"
#include "main.h"
#include "gth-window.h"
#include "gtk-utils.h"
#include "gconf-utils.h"


#define GLADE_FILE "gthumb.glade"


typedef struct {
	GladeXML     *gui;
	GtkWidget    *dialog;
	GtkWidget    *new_catalog_btn;
	GtkWidget    *new_dir_btn;
	GtkWidget    *ok_btn;
	GtkWidget    *cancel_btn;

	GthWindow    *window;
	CatalogList  *cat_list;
	char         *current_dir;

	/* extra data. */
	union {
		GList     *list;           /* Used by "add to catalog". */
		gchar     *catalog_path;   /* Used by "move to catalog dir". */
	} data;
} DialogData;



/* -- "new catalog" dialog. -- */


/* called when the user click on the "new catalog" button. */
static void
new_catalog_cb (GtkWidget *widget, 
		gpointer   p)
{
	DialogData *data = p;
	char       *name_utf8, *name;
	char       *path;
	int         fd;
	GtkTreeIter iter;

	name_utf8 = _gtk_request_dialog_run (GTK_WINDOW (data->window),
					     GTK_DIALOG_MODAL,
					     _("Enter the catalog name: "),
					     "",
					     1024,
					     GTK_STOCK_CANCEL,
					     _("C_reate"));
	
	if (name_utf8 == NULL)
		return;

	name = g_filename_from_utf8 (name_utf8, -1, 0, 0, 0);
	g_free (name_utf8);

	path = g_strconcat (data->current_dir,
			    "/",
			    name,
			    CATALOG_EXT,
			    NULL);
	g_free (name);

	fd = creat (path, 0644);
	close (fd);

	sync ();

	/* update the catalog list. */

	catalog_list_change_to (data->cat_list, data->current_dir);

	/* select the new catalog. */

	if (catalog_list_get_iter_from_path (data->cat_list, path, &iter))
		catalog_list_select_iter (data->cat_list, &iter);
	g_free (path);
}



/* -- "new catalog directory" dialog. -- */


/* called when the user click on the "new directory" button. */
static void
new_dir_cb (GtkWidget *widget, 
	    DialogData *data)
{
	char *utf8_name;
	char *name;
	char *path;

	utf8_name = _gtk_request_dialog_run (GTK_WINDOW (data->window),
					     GTK_DIALOG_MODAL,
					     _("Enter the library name: "),
					     "",
					     1024,
					     GTK_STOCK_CANCEL,
					     _("C_reate"));

	if (utf8_name == NULL)
		return;

	name = g_filename_from_utf8 (utf8_name, -1, 0, 0, 0);
	if (name == NULL)
		return;

	path = g_strconcat (data->current_dir,
			    "/",
			    name,
			    NULL);
	g_free (name);

	mkdir (path, 0775);
	g_free (path);

	/* update the catalog list. */
	catalog_list_change_to (data->cat_list, data->current_dir);
}



/* -- "add to catalog" dialog -- */



/* called when the "ok" button is clicked. */
static void
add_to_catalog__ok_cb (GtkWidget *widget, 
		       DialogData *data)
{
	Catalog  *catalog;
	gchar    *cat_path;
	GList    *scan;
	GError   *gerror;

	cat_path = catalog_list_get_selected_path (data->cat_list);
	if (cat_path == NULL)
		return;

	catalog = catalog_new ();
	if (! catalog_load_from_disk (catalog, cat_path, &gerror)) {
		_gtk_error_dialog_from_gerror_run (GTK_WINDOW (data->dialog), &gerror);
		return;
	}

	for (scan = data->data.list; scan; scan = scan->next)
		catalog_add_item (catalog, (char*) scan->data);

	if (! catalog_write_to_disk (catalog, &gerror)) 
		_gtk_error_dialog_from_gerror_run (GTK_WINDOW (data->dialog), &gerror);
	else
		all_windows_notify_cat_files_created (cat_path, data->data.list);

	catalog_free (catalog);
	
	eel_gconf_set_path (PREF_ADD_TO_CATALOG_LAST_CATALOG, cat_path);
	g_free (cat_path);

	gtk_widget_destroy (data->dialog);
}


/* called when an item of the catalog list is activated. */
static void 
add_to_catalog__activated_cb (GtkTreeView *tree_view,
			      GtkTreePath *path,
			      GtkTreeViewColumn *column,
			      gpointer p)
{
	DialogData *  data = p;
	gchar *       cat_path;

	cat_path = catalog_list_get_path_from_tree_path (data->cat_list, path);
	if (cat_path == NULL)
		return;
	
	if (path_is_dir (cat_path)) {
		catalog_list_change_to (data->cat_list, cat_path);
		g_free (data->current_dir);
		data->current_dir = cat_path;

	} else if (path_is_file (cat_path)) 
		add_to_catalog__ok_cb (NULL, data);
}


/* called when an item of the catalog list is selected. */
static void 
add_to_catalog__sel_changed_cb (GtkTreeSelection *selection,
				gpointer p)
{
	DialogData *  data = p;
	gchar *       cat_path;

	cat_path = catalog_list_get_selected_path (data->cat_list);
	gtk_widget_set_sensitive (data->ok_btn, ((cat_path != NULL)
						 && ! path_is_dir (cat_path)));
	if (cat_path != NULL)
		g_free (cat_path);
	return;
}


/* called when the main dialog is closed. */
static void
add_to_catalog__destroy_cb (GtkWidget *widget, 
			    DialogData *data)
{
	g_object_unref (G_OBJECT (data->gui));
	g_free (data->current_dir);
	catalog_list_free (data->cat_list);
	path_list_free (data->data.list);
	g_free (data);
}


/* create the "add to catalog" dialog. */
void
dlg_add_to_catalog (GthWindow *window,
		    GList     *list)
{
	GtkTreeSelection *selection;
	DialogData       *data;
	GtkWidget        *list_hbox;
	GtkWidget        *cat_catalogs_label;
	char             *last_catalog = NULL;

	data = g_new (DialogData, 1);

	data->window = window;
	data->cat_list = catalog_list_new (FALSE);
	data->data.list = list;

	last_catalog = eel_gconf_get_path (PREF_ADD_TO_CATALOG_LAST_CATALOG, NULL);
	if (path_is_file (last_catalog)) {
		data->current_dir = remove_level_from_path (last_catalog);
	} else
		data->current_dir = get_catalog_full_path (NULL);

	data->gui = glade_xml_new (GTHUMB_GLADEDIR "/" GLADE_FILE, NULL, NULL);
	if (! data->gui) {
		g_warning ("Could not find " GLADE_FILE "\n");
		return;
        }

	/* Get the widgets. */

	data->dialog = glade_xml_get_widget (data->gui, "catalog_dialog");
	list_hbox = glade_xml_get_widget (data->gui, "cat_list_hbox");
	data->new_catalog_btn = glade_xml_get_widget (data->gui, "cat_new_catalog_btn");
	data->new_dir_btn = glade_xml_get_widget (data->gui, "cat_new_dir_btn");
	cat_catalogs_label = glade_xml_get_widget (data->gui, "cat_catalogs_label");
	data->ok_btn = glade_xml_get_widget (data->gui, "cat_ok_btn");
	data->cancel_btn = glade_xml_get_widget (data->gui, "cat_cancel_btn");

	gtk_container_add (GTK_CONTAINER (list_hbox), data->cat_list->root_widget);
	gtk_label_set_mnemonic_widget (GTK_LABEL (cat_catalogs_label), 
				       data->cat_list->list_view);

	/* Set the signals handlers. */

	g_signal_connect (G_OBJECT (data->dialog), "destroy",
			  G_CALLBACK (add_to_catalog__destroy_cb),
			  data);

	g_signal_connect (G_OBJECT (data->new_catalog_btn),
			  "clicked",
			  G_CALLBACK (new_catalog_cb),
			  data);
	g_signal_connect (G_OBJECT (data->new_dir_btn), 
			  "clicked",
			  G_CALLBACK (new_dir_cb),
			  data);
	g_signal_connect (G_OBJECT (data->ok_btn),
			  "clicked",
			  G_CALLBACK (add_to_catalog__ok_cb),
			  data);
	g_signal_connect_swapped (G_OBJECT (data->cancel_btn), 
				  "clicked",
				  G_CALLBACK (gtk_widget_destroy),
				  G_OBJECT (data->dialog));

	g_signal_connect (G_OBJECT (data->cat_list->list_view),
                          "row_activated",
                          G_CALLBACK (add_to_catalog__activated_cb),
                          data);
	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (data->cat_list->list_view));
	g_signal_connect (G_OBJECT (selection),
                          "changed",
                          G_CALLBACK (add_to_catalog__sel_changed_cb),
                          data);

	/* set data. */

	catalog_list_change_to (data->cat_list, data->current_dir);
	gtk_widget_set_sensitive (data->ok_btn, FALSE);

	if (last_catalog != NULL) {
		GtkTreeIter iter;
		if (catalog_list_get_iter_from_path (data->cat_list, last_catalog, &iter))
			catalog_list_select_iter (data->cat_list, &iter);
		g_free (last_catalog);
	}

	/* run dialog. */

	gtk_window_set_transient_for (GTK_WINDOW (data->dialog),
				      GTK_WINDOW (window));

	gtk_window_set_modal (GTK_WINDOW (data->dialog), TRUE);
	gtk_widget_show_all (data->dialog);
}



/* -- "move to catalog directory" dialog -- */


static void
move_to_catalog_dir__destroy_cb (GtkWidget *widget, 
				 DialogData *data)
{
        g_object_unref (G_OBJECT (data->gui));
	g_free (data->current_dir);
	catalog_list_free (data->cat_list);
	g_free (data->data.catalog_path);
	g_free (data);
}


/* called when an item of the catalog list is selected. */
static void 
move_to_catalog_dir__activated_cb (GtkTreeView *tree_view,
				   GtkTreePath *path,
				   GtkTreeViewColumn *column,
				   gpointer p)
{
	DialogData *data = p;
	char       *cat_path;

	cat_path = catalog_list_get_path_from_tree_path (data->cat_list, path);

	if (! path_is_dir (cat_path)) {
		g_free (cat_path);
		return;
	} 

	catalog_list_change_to (data->cat_list, cat_path);
	g_free (data->current_dir);
	data->current_dir = cat_path;
}


/* called when the "ok" button is clicked. */
static void
move_to_catalog_dir__ok_cb (GtkWidget  *widget, 
			    DialogData *data)
{
	char *new_path;

	new_path = g_strconcat (data->cat_list->path,
				"/",
				file_name_from_path (data->data.catalog_path),
				NULL);

	file_move (data->data.catalog_path, new_path);
	g_free (new_path);

	all_windows_update_catalog_list ();
	gtk_widget_destroy (data->dialog);
}


/* create the "move to catalog directory" dialog. */
void
dlg_move_to_catalog_directory (GthWindow *window,
			       gchar     *catalog_path)
{
	DialogData *data;
	GtkWidget  *list_hbox;

	data = g_new (DialogData, 1);

	data->window = window;
	data->current_dir = get_catalog_full_path (NULL);
	data->cat_list = catalog_list_new (FALSE);
	data->data.catalog_path = catalog_path;

	data->gui = glade_xml_new (GTHUMB_GLADEDIR "/" GLADE_FILE, NULL, NULL);
	if (! data->gui) {
		g_warning ("Could not find " GLADE_FILE "\n");
		return;
        }

	/* Get the widgets. */

	data->dialog = glade_xml_get_widget (data->gui, "catalog_dialog");
        list_hbox = glade_xml_get_widget (data->gui, "cat_list_hbox");
        data->new_catalog_btn = glade_xml_get_widget (data->gui, "cat_new_catalog_btn");
        data->new_dir_btn = glade_xml_get_widget (data->gui, "cat_new_dir_btn");
        data->ok_btn = glade_xml_get_widget (data->gui, "cat_ok_btn");
        data->cancel_btn = glade_xml_get_widget (data->gui, "cat_cancel_btn");

	gtk_container_add (GTK_CONTAINER (list_hbox), data->cat_list->root_widget);

	/* Set the signals handlers. */

	g_signal_connect (G_OBJECT (data->dialog), 
			  "destroy",
			  G_CALLBACK (move_to_catalog_dir__destroy_cb),
			  data);

	g_signal_connect (G_OBJECT (data->new_dir_btn), 
			  "clicked",
			  G_CALLBACK (new_dir_cb),
			  data);
	g_signal_connect (G_OBJECT (data->ok_btn), 
			  "clicked",
			  G_CALLBACK (move_to_catalog_dir__ok_cb),
			  data);
	g_signal_connect_swapped (G_OBJECT (data->cancel_btn), 
				  "clicked",
				  G_CALLBACK (gtk_widget_destroy),
				  G_OBJECT (data->dialog));

	g_signal_connect (G_OBJECT (data->cat_list->list_view),
                          "row_activated",
                          G_CALLBACK (move_to_catalog_dir__activated_cb),
                          data);

	/* set data. */

	catalog_list_change_to (data->cat_list, data->current_dir);
	gtk_widget_set_sensitive (data->new_catalog_btn, FALSE);
	gtk_window_set_title (GTK_WINDOW (data->dialog), _("Move Catalog to..."));

	/* run dialog. */

	gtk_window_set_transient_for (GTK_WINDOW (data->dialog), 
				      GTK_WINDOW (window));
	
	gtk_window_set_modal (GTK_WINDOW (data->dialog), TRUE);

	gtk_widget_show_all (data->dialog);
}
