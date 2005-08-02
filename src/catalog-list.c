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

#include <stdio.h>
#include <string.h>

#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include "typedefs.h"
#include "file-utils.h"
#include "catalog.h"
#include "catalog-list.h"
#include "pixbuf-utils.h"
#include "main.h"

#include "icons/pixbufs.h"

enum {
	CAT_LIST_TYPE_DIR,
	CAT_LIST_TYPE_SEARCH,
	CAT_LIST_TYPE_CATALOG
};

enum {
	CAT_LIST_COLUMN_ICON,
	CAT_LIST_COLUMN_NAME,
	CAT_LIST_COLUMN_PATH,
	CAT_LIST_COLUMN_TYPE,
	CAT_LIST_NUM_COLUMNS
};


static void
filename_cell_data_func (GtkTreeViewColumn *column,
			 GtkCellRenderer   *renderer,
			 GtkTreeModel      *model,
			 GtkTreeIter       *iter,
			 CatalogList       *catalog_list)
{
	char *text;
	GtkTreePath *path;
	PangoUnderline underline;

	gtk_tree_model_get (model, iter,
			    CAT_LIST_COLUMN_NAME, &text,
			    -1);

	if (catalog_list->single_click) {
		path = gtk_tree_model_get_path (model, iter);

		if (catalog_list->hover_path == NULL ||
		    gtk_tree_path_compare (path, catalog_list->hover_path)) 
			underline = PANGO_UNDERLINE_NONE;
		else 
			underline = PANGO_UNDERLINE_SINGLE;
		
		gtk_tree_path_free (path);

	} else 
		underline = PANGO_UNDERLINE_NONE;

	g_object_set (G_OBJECT (renderer),
		      "text", text,
		      "underline", underline,
		      NULL);

	g_free (text);
}


static void
add_columns (CatalogList *cat_list,
	     GtkTreeView *treeview)
{
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;
	
	/* The Name column. */

	column = gtk_tree_view_column_new ();
	
	renderer = gtk_cell_renderer_pixbuf_new ();
	gtk_tree_view_column_pack_start (column, renderer, FALSE);
	gtk_tree_view_column_set_attributes (column, renderer,
					     "pixbuf", CAT_LIST_COLUMN_ICON,
					     NULL);
	
	cat_list->text_renderer = renderer = gtk_cell_renderer_text_new ();

        gtk_tree_view_column_pack_start (column,
                                         renderer,
                                         TRUE);
        gtk_tree_view_column_set_attributes (column, renderer,
                                             "text", CAT_LIST_COLUMN_NAME,
                                             NULL);

        gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);        
	gtk_tree_view_column_set_sort_column_id (column, CAT_LIST_COLUMN_NAME);

	gtk_tree_view_column_set_cell_data_func (column, renderer,
						 (GtkTreeCellDataFunc) filename_cell_data_func,
						 cat_list, NULL);

        gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);
}


static gboolean
file_motion_notify_callback (GtkWidget *widget,
			     GdkEventMotion *event,
			     gpointer user_data)
{
	CatalogList *cat_list = user_data;
	GdkCursor   *cursor;
	GtkTreePath *last_hover_path;
	GtkTreeIter  iter;
 	
	if (! cat_list->single_click) 
		return FALSE;

	if (event->window != gtk_tree_view_get_bin_window (GTK_TREE_VIEW (cat_list->list_view))) 
                return FALSE;

	last_hover_path = cat_list->hover_path;

	gtk_tree_view_get_path_at_pos (GTK_TREE_VIEW (widget),
				       event->x, event->y,
				       &cat_list->hover_path,
				       NULL, NULL, NULL);

	if (cat_list->hover_path != NULL) 
		cursor = gdk_cursor_new (GDK_HAND2);
	else 
		cursor = NULL;
	
	gdk_window_set_cursor (event->window, cursor);

	/* only redraw if the hover row has changed */
	if (!(last_hover_path == NULL && cat_list->hover_path == NULL) &&
	    (!(last_hover_path != NULL && cat_list->hover_path != NULL) ||
	     gtk_tree_path_compare (last_hover_path, cat_list->hover_path))) {
		if (last_hover_path) {
			gtk_tree_model_get_iter (GTK_TREE_MODEL (cat_list->list_store),
						 &iter, last_hover_path);
			gtk_tree_model_row_changed (GTK_TREE_MODEL (cat_list->list_store),
						    last_hover_path, &iter);
		}
		
		if (cat_list->hover_path) {
			gtk_tree_model_get_iter (GTK_TREE_MODEL (cat_list->list_store),
						 &iter, cat_list->hover_path);
			gtk_tree_model_row_changed (GTK_TREE_MODEL (cat_list->list_store),
						    cat_list->hover_path, &iter);
		}
	}
	
	gtk_tree_path_free (last_hover_path);

 	return FALSE;
}


static gboolean 
file_leave_notify_callback (GtkWidget *widget,
			    GdkEventCrossing *event,
			    gpointer user_data)
{
	CatalogList *cat_list = user_data;
	GtkTreeIter  iter;

	if (cat_list->single_click && (cat_list->hover_path != NULL)) {
		gtk_tree_model_get_iter (GTK_TREE_MODEL (cat_list->list_store),
					 &iter, 
					 cat_list->hover_path);
		gtk_tree_model_row_changed (GTK_TREE_MODEL (cat_list->list_store),
					    cat_list->hover_path,
					    &iter);

		gtk_tree_path_free (cat_list->hover_path);
		cat_list->hover_path = NULL;

		return TRUE;
	}

	return FALSE;
}


CatalogList *
catalog_list_new (gboolean single_click_policy)
{
	CatalogList * cat_list;
	GtkWidget *scrolled;
	GtkTreeView *list_view;

	cat_list = g_new (CatalogList, 1);

	cat_list->single_click = single_click_policy;

	/* get base catalogs directory. */

	cat_list->path =  get_catalog_full_path (NULL);

	/* Create the widgets. */

	scrolled = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled),
					GTK_POLICY_AUTOMATIC, 
					GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled),
                                             GTK_SHADOW_ETCHED_IN);

	cat_list->list_store = gtk_list_store_new (CAT_LIST_NUM_COLUMNS,
						   GDK_TYPE_PIXBUF,
						   G_TYPE_STRING,
						   G_TYPE_STRING,
						   G_TYPE_INT);
	list_view = (GtkTreeView*) gtk_tree_view_new_with_model (GTK_TREE_MODEL (cat_list->list_store));
        gtk_tree_view_set_rules_hint (list_view, FALSE);
        add_columns (cat_list, list_view);
	gtk_tree_view_set_headers_visible (list_view, FALSE);
        gtk_tree_view_set_enable_search (list_view, TRUE);
        gtk_tree_view_set_search_column (list_view, CAT_LIST_COLUMN_NAME);

	cat_list->list_view = (GtkWidget*) list_view;
	gtk_container_add (GTK_CONTAINER (scrolled), cat_list->list_view);
	cat_list->root_widget = scrolled;

	g_signal_connect (G_OBJECT (list_view), 
			  "motion_notify_event",
			  G_CALLBACK (file_motion_notify_callback), 
			  cat_list);
	g_signal_connect (G_OBJECT (list_view), 
			  "leave_notify_event",
			  G_CALLBACK (file_leave_notify_callback), 
			  cat_list);

	return cat_list;
}


void
catalog_list_free (CatalogList *cat_list)
{
	g_return_if_fail (cat_list != NULL);
	g_free (cat_list->path);
	g_free (cat_list);
}


gchar *
catalog_list_get_path_from_tree_path (CatalogList *cat_list,
				      GtkTreePath *path)
{
	GtkTreeIter  iter;
	char        *name;

	g_return_val_if_fail (cat_list != NULL, NULL);

	if (! gtk_tree_model_get_iter (GTK_TREE_MODEL (cat_list->list_store),
                                       &iter,
                                       path)) 
                return NULL;
	
	gtk_tree_model_get (GTK_TREE_MODEL (cat_list->list_store), 
			    &iter,
			    CAT_LIST_COLUMN_PATH, &name,
			    -1);

	return name;
}


gchar *
catalog_list_get_path_from_iter (CatalogList *cat_list,
				 GtkTreeIter *iter)
{
	GtkTreePath *path;
	char        *result;

	g_return_val_if_fail (cat_list != NULL, NULL);

	path = gtk_tree_model_get_path (GTK_TREE_MODEL (cat_list->list_store),
					iter);
	result = catalog_list_get_path_from_tree_path (cat_list, path);
	gtk_tree_path_free (path);

	return result;
}


gchar *
catalog_list_get_name_from_iter (CatalogList *cat_list,
				 GtkTreeIter *iter)
{
	char *name;

	g_return_val_if_fail (cat_list != NULL, NULL);

	gtk_tree_model_get (GTK_TREE_MODEL (cat_list->list_store), 
			    iter,
			    CAT_LIST_COLUMN_NAME, &name,
			    -1);

	return name;
}


gchar *
catalog_list_get_path_from_row (CatalogList *cat_list,
				gint row)
{
	GtkTreePath *path;
	char        *result;

	g_return_val_if_fail (cat_list != NULL, NULL);

	path = gtk_tree_path_new ();
	gtk_tree_path_append_index (path, row);
	result = catalog_list_get_path_from_tree_path (cat_list, path);
	gtk_tree_path_free (path);

	return result;
}


gboolean
catalog_list_get_selected_iter (CatalogList *cat_list, 
				GtkTreeIter *iter)
{
	GtkTreeView *tree_view;
	GtkTreeSelection *selection;

	tree_view = GTK_TREE_VIEW (cat_list->list_view);
	selection = gtk_tree_view_get_selection (tree_view);
	return gtk_tree_selection_get_selected (selection, NULL, iter);
}


gchar *
catalog_list_get_selected_path (CatalogList *cat_list)
{
	GtkTreeIter iter;
	if (! catalog_list_get_selected_iter (cat_list, &iter))
		return NULL;
	return catalog_list_get_path_from_iter (cat_list, &iter);
}


gboolean
catalog_list_is_catalog (CatalogList *cat_list,
			 GtkTreeIter *iter)
{
	int type;

	g_return_val_if_fail (cat_list != NULL, FALSE);

	gtk_tree_model_get (GTK_TREE_MODEL (cat_list->list_store), 
			    iter,
			    CAT_LIST_COLUMN_TYPE, &type,
			    -1);
	return (type == CAT_LIST_TYPE_CATALOG);
}


gboolean
catalog_list_is_dir (CatalogList *cat_list,
		     GtkTreeIter *iter)
{
	int type;

	g_return_val_if_fail (cat_list != NULL, FALSE);

	gtk_tree_model_get (GTK_TREE_MODEL (cat_list->list_store), 
			    iter,
			    CAT_LIST_COLUMN_TYPE, &type,
			    -1);
	return (type == CAT_LIST_TYPE_DIR);
}


gboolean
catalog_list_is_search (CatalogList *cat_list,
			GtkTreeIter *iter)
{
	int type;

	g_return_val_if_fail (cat_list != NULL, FALSE);

	gtk_tree_model_get (GTK_TREE_MODEL (cat_list->list_store), 
			    iter,
			    CAT_LIST_COLUMN_TYPE, &type,
			    -1);
	return (type == CAT_LIST_TYPE_SEARCH);
}


gboolean
catalog_list_get_iter_from_path (CatalogList *cat_list,
				 const gchar *path,
				 GtkTreeIter *iter)
{
	GtkTreeModel *model;
	gboolean      exists;
	int           i = 0;

	g_return_val_if_fail (cat_list != NULL, -1);
	g_return_val_if_fail (path != NULL, -1);

	model = GTK_TREE_MODEL (cat_list->list_store);
	exists = gtk_tree_model_get_iter_first (model, iter);
	while (exists) {
		char *row_path;

		gtk_tree_model_get (model,
				    iter,
				    CAT_LIST_COLUMN_PATH, &row_path,
				    -1);

		g_return_val_if_fail (row_path != NULL, FALSE);

		if (strcmp (row_path, path) == 0) {
			g_free (row_path);
			return TRUE;
		}
		g_free (row_path);

		exists = gtk_tree_model_iter_next (model, iter);
		i++;
	}

	return FALSE;
}


void
catalog_list_update_underline (CatalogList *cat_list)
{
	GdkWindow  *win = gtk_tree_view_get_bin_window (GTK_TREE_VIEW (cat_list->list_view));
	GdkDisplay *display;

	cat_list->single_click = (pref_get_real_click_policy () == GTH_CLICK_POLICY_SINGLE);

	gdk_window_set_cursor (win, NULL);
	display = gtk_widget_get_display (GTK_WIDGET (cat_list->list_view));
	if (display != NULL) 
		gdk_display_flush (display);
}


gboolean
catalog_list_refresh (CatalogList *cat_list)
{
	GList     *scan;
	GList     *name_scan;
	GdkPixbuf *dir_pixbuf;
	GdkPixbuf *up_pixbuf;
	GdkPixbuf *catalog_pixbuf;
	GdkPixbuf *search_pixbuf;
	char      *base; 
	GList     *dir_list;            /* Contain the full path of the 
					 * sub dirs. */
	GList     *dir_name;            /* Contains only the names of the 
					   dirs. */
	GList     *file_list;           /* Contains the full path of the
					 * catalog files. */
	GList     *file_name;           /* Contains only the names of the 
					 * catalogs. */

	/* Set the file and dir lists. */

	if (! path_list_new (cat_list->path, &file_list, &dir_list)) {
		g_print ("ERROR: reading catalogs directory\n");
		return FALSE;
	}

	dir_list = g_list_sort (dir_list, (GCompareFunc) strcasecmp);
	file_list = g_list_sort (file_list, (GCompareFunc) strcasecmp);

	/* get the list of dirs names (without path). */

	dir_name = NULL;
	for (scan = dir_list; scan; scan = scan->next) {
		char *name_only = g_strdup (file_name_from_path (scan->data));
		dir_name = g_list_prepend (dir_name, name_only);
	}
	dir_name = g_list_reverse (dir_name);

	/* Add a ".." entry if the current dir is not the base catalog dir. */

	base = get_catalog_full_path (NULL);
	if (strcmp (base, cat_list->path) != 0) {
		char *prev_dir = remove_level_from_path (cat_list->path);

		dir_list = g_list_prepend (dir_list, prev_dir);
		dir_name = g_list_prepend (dir_name, g_strdup (".."));
	}
	g_free (base);

	/* get the list of files names (without path). */

	file_name = NULL;
	for (scan = file_list; scan; scan = scan->next) {
		char *name_only = remove_extension_from_path (file_name_from_path (scan->data));
		file_name = g_list_prepend (file_name, name_only);
	}
	file_name = g_list_reverse (file_name);

	/* Add items to the list. */

	gtk_list_store_clear (cat_list->list_store);

	dir_pixbuf = gdk_pixbuf_new_from_inline (-1, library_19_rgba, FALSE, NULL);
	up_pixbuf = gtk_widget_render_icon (cat_list->list_view,
					    GTK_STOCK_GO_UP,
					    GTK_ICON_SIZE_MENU,
					    NULL);
	catalog_pixbuf = gdk_pixbuf_new_from_inline (-1, catalog_16_rgba, FALSE, NULL);
	search_pixbuf = gdk_pixbuf_new_from_inline (-1, catalog_search_16_rgba, FALSE, NULL);

	name_scan = dir_name;
	for (scan = dir_list; scan; scan = scan->next) {
		char        *name = name_scan->data;
		char        *utf8_name;
		GtkTreeIter  iter;
		GdkPixbuf   *pixbuf;

		if (strcmp (name, "..") == 0)
			pixbuf = up_pixbuf;
		else 
			pixbuf = dir_pixbuf;

		utf8_name = g_filename_display_name (name);
		gtk_list_store_append (cat_list->list_store, &iter);
		gtk_list_store_set (cat_list->list_store, &iter,
				    CAT_LIST_COLUMN_ICON, pixbuf,
				    CAT_LIST_COLUMN_NAME, utf8_name,
				    CAT_LIST_COLUMN_PATH, scan->data,
				    CAT_LIST_COLUMN_TYPE, CAT_LIST_TYPE_DIR,
				    -1);
		g_free (utf8_name);
		name_scan = name_scan->next;
	}

	name_scan = file_name;
	for (scan = file_list; scan; scan = scan->next) {
		char        *name = name_scan->data;
		char        *utf8_name;
		GtkTreeIter  iter;
		GdkPixbuf   *pixbuf;
		int          type;

		if (file_is_search_result (scan->data)) {
			type = CAT_LIST_TYPE_SEARCH;
			pixbuf = search_pixbuf;
		} else {
			type = CAT_LIST_TYPE_CATALOG;
			pixbuf = catalog_pixbuf;
		}

		utf8_name = g_filename_display_name (name);
		gtk_list_store_append (cat_list->list_store, &iter);
		gtk_list_store_set (cat_list->list_store, &iter,
				    CAT_LIST_COLUMN_ICON, pixbuf,
				    CAT_LIST_COLUMN_NAME, utf8_name,
				    CAT_LIST_COLUMN_PATH, scan->data,
				    CAT_LIST_COLUMN_TYPE, type,
				    -1);
		g_free (utf8_name);
		name_scan = name_scan->next;
	}

	g_object_unref (dir_pixbuf);
	g_object_unref (up_pixbuf);
	g_object_unref (catalog_pixbuf);
	g_object_unref (search_pixbuf);

	path_list_free (dir_list);
	path_list_free (dir_name);
	path_list_free (file_list);
	path_list_free (file_name);

	return TRUE;
}


void
catalog_list_change_to (CatalogList *cat_list,
			const char  *path)
{
	g_return_if_fail (cat_list != NULL);

	if (path != cat_list->path) {
		g_free (cat_list->path);
		cat_list->path = remove_ending_separator (path);
	}

	catalog_list_refresh (cat_list);
}


void
catalog_list_select_iter (CatalogList *cat_list,
			  GtkTreeIter *iter)
{
	GtkTreeView *tree_view;
	GtkTreeSelection *selection;

	tree_view = GTK_TREE_VIEW (cat_list->list_view);
	selection = gtk_tree_view_get_selection (tree_view);
	gtk_tree_selection_select_iter (selection, iter);
}

