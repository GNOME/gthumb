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

#include <string.h>

#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include "main.h"
#include "gthumb-init.h"
#include "bookmarks.h"
#include "bookmark-list.h"
#include "typedefs.h"
#include "pixbuf-utils.h"
#include "preferences.h"
#include "file-utils.h"
#include "gthumb-stock.h"

#include "icons/pixbufs.h"


enum {
	BOOK_LIST_COLUMN_ICON,
	BOOK_LIST_COLUMN_NAME,
	BOOK_LIST_COLUMN_PATH,
	BOOK_LIST_NUM_COLUMNS
};


static void
add_columns (GtkTreeView *treeview)
{
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;

	/* The Name column. */

	column = gtk_tree_view_column_new ();

	renderer = gtk_cell_renderer_pixbuf_new ();
	gtk_tree_view_column_pack_start (column, renderer, FALSE);
	gtk_tree_view_column_set_attributes (column, renderer,
					     "pixbuf", BOOK_LIST_COLUMN_ICON,
					     NULL);

	renderer = gtk_cell_renderer_text_new ();
        gtk_tree_view_column_pack_start (column,
                                         renderer,
                                         TRUE);
        gtk_tree_view_column_set_attributes (column, renderer,
                                             "text", BOOK_LIST_COLUMN_NAME,
                                             NULL);

        gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
        gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);
}


BookmarkList *
bookmark_list_new (gboolean menu_icons)
{
	BookmarkList *book_list;
	GtkWidget    *scrolled;
	GtkTreeView  *list_view;

	book_list = g_new0 (BookmarkList, 1);

	book_list->list = NULL;
	book_list->menu_icons = menu_icons;

	/* Create the widgets. */
	scrolled = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled),
					GTK_POLICY_AUTOMATIC,
					GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled),
                                             GTK_SHADOW_ETCHED_IN);

	book_list->list_store = gtk_list_store_new (BOOK_LIST_NUM_COLUMNS,
						    GDK_TYPE_PIXBUF,
						    G_TYPE_STRING,
						    G_TYPE_STRING);
	list_view = (GtkTreeView*) gtk_tree_view_new_with_model (GTK_TREE_MODEL (book_list->list_store));
        gtk_tree_view_set_rules_hint (list_view, FALSE);
        add_columns (list_view);
	gtk_tree_view_set_headers_visible (list_view, FALSE);
        gtk_tree_view_set_enable_search (list_view, TRUE);
        gtk_tree_view_set_search_column (list_view, BOOK_LIST_COLUMN_NAME);

	book_list->list_view = (GtkWidget*) list_view;
	gtk_container_add (GTK_CONTAINER (scrolled), book_list->list_view);
	book_list->root_widget = scrolled;

	return book_list;
}


void
bookmark_list_free (BookmarkList *book_list)
{
	g_return_if_fail (book_list != NULL);

	if (book_list->list != NULL)
		path_list_free (book_list->list);
	g_free (book_list);
}


void
bookmark_list_set (BookmarkList *book_list,
		   GList        *list)
{
	GList *scan;

	g_return_if_fail (book_list != NULL);

	gtk_list_store_clear (book_list->list_store);
	if (book_list->list)
		path_list_free (book_list->list);

	/* Copy the list of paths. */

	book_list->list = NULL;
	for (scan = list; scan; scan = scan->next)
		book_list->list = g_list_prepend (
			book_list->list, g_strdup ((char*) scan->data));
	book_list->list = g_list_reverse (book_list->list);

	/* Insert the bookmarks in the list. */

	for (scan = book_list->list; scan; scan = scan->next) {
		char        *name = scan->data;
		char        *display_name;
		GtkTreeIter  iter;
		GdkPixbuf   *pixbuf;

		display_name = get_uri_display_name (name);
		if (display_name == NULL)
			display_name = g_strdup (_("(Invalid Name)"));

		if (book_list->menu_icons)
			pixbuf = gtk_widget_render_icon (book_list->list_view,
							 get_stock_id_for_uri (name),
							 GTK_ICON_SIZE_MENU,
							 NULL);
		else
			pixbuf = get_icon_for_uri (book_list->list_view, name);

		gtk_list_store_append (book_list->list_store, &iter);
		gtk_list_store_set (book_list->list_store, &iter,
				    BOOK_LIST_COLUMN_ICON, pixbuf,
				    BOOK_LIST_COLUMN_NAME, display_name,
				    BOOK_LIST_COLUMN_PATH, name,
				    -1);

		g_free (display_name);
		g_object_unref (pixbuf);
	}
}


gchar *
bookmark_list_get_path_from_tree_path (BookmarkList *book_list,
				       GtkTreePath *path)
{
	GtkTreeIter iter;
	gchar *name;

	g_return_val_if_fail (book_list != NULL, NULL);

	if (! gtk_tree_model_get_iter (GTK_TREE_MODEL (book_list->list_store),
                                       &iter,
                                       path))
                return NULL;

	gtk_tree_model_get (GTK_TREE_MODEL (book_list->list_store),
			    &iter,
			    BOOK_LIST_COLUMN_PATH, &name,
			    -1);
	return name;
}


gchar *
bookmark_list_get_path_from_row (BookmarkList *book_list,
				 gint row)
{
	GtkTreePath *path;
	char        *result;

	g_return_val_if_fail (book_list != NULL, NULL);

	path = gtk_tree_path_new ();
	gtk_tree_path_append_index (path, row);
	result = bookmark_list_get_path_from_tree_path (book_list, path);
	gtk_tree_path_free (path);

	return result;
}


gchar *
bookmark_list_get_selected_path (BookmarkList *book_list)
{
	GtkTreeSelection *selection;
	GtkTreeIter       iter;
	char             *name;

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (book_list->list_view));
        if (selection == NULL)
                return NULL;

	if (! gtk_tree_selection_get_selected (selection, NULL, &iter))
		return NULL;

	gtk_tree_model_get (GTK_TREE_MODEL (book_list->list_store),
			    &iter,
			    BOOK_LIST_COLUMN_PATH, &name,
			    -1);
	return name;
}


void
bookmark_list_select_item (BookmarkList *book_list,
			   int           row)
{
	GtkTreeSelection *selection;
	GtkTreePath      *tpath;

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (book_list->list_view));
        if (selection == NULL)
                return;

	tpath = gtk_tree_path_new ();
	gtk_tree_path_append_index (tpath, row);
	gtk_tree_selection_select_path (selection, tpath);
	gtk_tree_path_free (tpath);
}
