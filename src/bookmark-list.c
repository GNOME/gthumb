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

#include <gtk/gtk.h>
#include "main.h"
#include "gthumb-init.h"
#include "bookmark-list.h"
#include "typedefs.h"
#include "pixbuf-utils.h"
#include "preferences.h"
#include "file-utils.h"

#include "icons/pixbufs.h"


enum {
	BOOK_LIST_COLUMN_ICON,
	BOOK_LIST_COLUMN_NAME,
	BOOK_LIST_COLUMN_PATH,
	BOOK_LIST_NUM_COLUMNS
};


static gint
sort_by_name (gconstpointer  ptr1,
	      gconstpointer  ptr2)
{
	const gchar *name1, *name2;

	name1 = file_name_from_path ((gchar*) ptr1);
	name2 = file_name_from_path ((gchar*) ptr2);

	if ((name1 == NULL) || (name2 == NULL))
		return 0;

	return strcasecmp (name1, name2);
}


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
	gtk_tree_view_column_set_sort_column_id (column, BOOK_LIST_COLUMN_NAME);
        gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);
}


BookmarkList *
bookmark_list_new ()
{
	BookmarkList * book_list;
	GtkWidget *scrolled;
	GtkTreeView *list_view;

	book_list = g_new (BookmarkList, 1);

	book_list->list = NULL;

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
        gtk_tree_view_set_rules_hint (list_view, TRUE);
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


gchar *
get_menu_name (const gchar *label)
{
	const char *name_with_ext;
	gint offset = 0;

	if (pref_util_location_is_catalog (label)
	    || pref_util_location_is_search (label)) {
		gchar *rc_dir_prefix;

		rc_dir_prefix = g_strconcat (g_get_home_dir (),
					     "/",
					     RC_CATALOG_DIR,
					     "/",
					     NULL);

		offset = strlen (rc_dir_prefix);

		g_free (rc_dir_prefix);
	}

	name_with_ext = pref_util_remove_prefix (label) + offset;
	return remove_extension_from_path (name_with_ext);
}


void 
bookmark_list_set (BookmarkList *book_list,
		   GList *list)
{
	GList *scan;
	GdkPixbuf *dir_pixbuf, *search_pixbuf, *catalog_pixbuf;

	g_return_if_fail (book_list != NULL);

	dir_pixbuf = get_folder_pixbuf (LIST_ICON_SIZE);
	catalog_pixbuf = gdk_pixbuf_new_from_inline (-1, catalog_rgba, FALSE, NULL);
	search_pixbuf = gdk_pixbuf_new_from_inline (-1, catalog_search_rgba, FALSE, NULL);

	gtk_list_store_clear (book_list->list_store);

	if (book_list->list)
		path_list_free (book_list->list);

	/* Copy the list of paths. */

	book_list->list = NULL;

	for (scan = list; scan; scan = scan->next)
		book_list->list = g_list_prepend (
			book_list->list, g_strdup ((gchar*) scan->data));

	book_list->list = g_list_sort (book_list->list, sort_by_name);

	/* Insert the bookmarks in the list. */

	for (scan = book_list->list; scan; scan = scan->next) {
		gchar *       name = scan->data;
		gchar *       menu_name;
		gchar *       utf8_name;
		GtkTreeIter   iter;
		GdkPixbuf *   pixbuf;
		
		if (pref_util_location_is_catalog (name)) 
			pixbuf = catalog_pixbuf;
		else if (pref_util_location_is_search (name))
			pixbuf = search_pixbuf;
		else 
			pixbuf = dir_pixbuf;

		menu_name = get_menu_name (name);
		utf8_name = g_locale_to_utf8 (menu_name, -1, NULL, NULL, NULL);

		gtk_list_store_append (book_list->list_store, &iter);
		gtk_list_store_set (book_list->list_store, &iter,
				    BOOK_LIST_COLUMN_ICON, pixbuf,
				    BOOK_LIST_COLUMN_NAME, utf8_name,
				    BOOK_LIST_COLUMN_PATH, name,
				    -1);
		g_free (menu_name);
		g_free (utf8_name);
	}

	g_object_unref (G_OBJECT (dir_pixbuf));
	g_object_unref (G_OBJECT (search_pixbuf));
	g_object_unref (G_OBJECT (catalog_pixbuf));
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
	gchar *result;

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
	GtkTreeIter iter;
	gchar *name;

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
