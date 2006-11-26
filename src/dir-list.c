/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2001, 2003 Free Software Foundation, Inc.
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

#include <glib/gi18n.h>
#include <libgnomevfs/gnome-vfs-types.h>
#include <libgnomevfs/gnome-vfs-async-ops.h>

#include "typedefs.h"
#include "dir-list.h"
#include "gth-file-list.h"
#include "file-data.h"
#include "file-utils.h"
#include "gconf-utils.h"
#include "main.h"
#include "pixbuf-utils.h"
#include "icons/pixbufs.h"
#include "gthumb-stock.h"
#include "comments.h"


#define DEF_SHOW_HIDDEN FALSE


enum {
	DIR_LIST_COLUMN_ICON,
	DIR_LIST_COLUMN_NAME,
	DIR_LIST_COLUMN_UTF_NAME,
	DIR_LIST_NUM_COLUMNS
};


static void
filename_cell_data_func (GtkTreeViewColumn *column,
			 GtkCellRenderer   *renderer,
			 GtkTreeModel      *model,
			 GtkTreeIter       *iter,
			 DirList           *dir_list)
{
	char *text;
	GtkTreePath *path;
	PangoUnderline underline;

	gtk_tree_model_get (model, iter,
			    DIR_LIST_COLUMN_UTF_NAME, &text,
			    -1);

	if (dir_list->single_click) {
		path = gtk_tree_model_get_path (model, iter);

		if (dir_list->hover_path == NULL ||
		    gtk_tree_path_compare (path, dir_list->hover_path)) 
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
add_columns (DirList     *dir_list,
	     GtkTreeView *treeview)
{
	GtkCellRenderer   *renderer;
	GtkTreeViewColumn *column;
	GValue             value = { 0, };
	
	/* The Name column. */

	column = gtk_tree_view_column_new ();
	
	renderer = gtk_cell_renderer_pixbuf_new ();
	gtk_tree_view_column_pack_start (column, renderer, FALSE);
	gtk_tree_view_column_set_attributes (column, renderer,
					     "pixbuf", DIR_LIST_COLUMN_ICON,
					     NULL);
	
	dir_list->text_renderer = renderer = gtk_cell_renderer_text_new ();
	
        g_value_init (&value, PANGO_TYPE_ELLIPSIZE_MODE);
        g_value_set_enum (&value, PANGO_ELLIPSIZE_END);
        g_object_set_property (G_OBJECT (renderer), "ellipsize", &value);
        g_value_unset (&value);

        gtk_tree_view_column_pack_start (column,
                                         renderer,
                                         TRUE);
        gtk_tree_view_column_set_attributes (column, renderer,
                                             "text", DIR_LIST_COLUMN_UTF_NAME,
                                             NULL);

        gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);        
	gtk_tree_view_column_set_sort_column_id (column, DIR_LIST_COLUMN_UTF_NAME);
	gtk_tree_view_column_set_cell_data_func (column, renderer,
						 (GtkTreeCellDataFunc) filename_cell_data_func,
						 dir_list, NULL);

        gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);
}


static gboolean
file_motion_notify_callback (GtkWidget      *widget,
			     GdkEventMotion *event,
			     gpointer        user_data)
{
	DirList     *dir_list = user_data;
	GdkCursor   *cursor;
	GtkTreePath *last_hover_path;
	GtkTreeIter  iter;
 	
	if (! dir_list->single_click) 
		return FALSE;

	if (event->window != gtk_tree_view_get_bin_window (GTK_TREE_VIEW (dir_list->list_view))) 
                return FALSE;

	last_hover_path = dir_list->hover_path;

	gtk_tree_view_get_path_at_pos (GTK_TREE_VIEW (widget),
				       event->x, event->y,
				       &dir_list->hover_path,
				       NULL, NULL, NULL);

	if (dir_list->hover_path != NULL) 
		cursor = gdk_cursor_new (GDK_HAND2);
	else 
		cursor = NULL;
	
	gdk_window_set_cursor (event->window, cursor);

	/* only redraw if the hover row has changed */
	if (!(last_hover_path == NULL && dir_list->hover_path == NULL) &&
	    (!(last_hover_path != NULL && dir_list->hover_path != NULL) ||
	     gtk_tree_path_compare (last_hover_path, dir_list->hover_path))) {
		if (last_hover_path) {
			gtk_tree_model_get_iter (GTK_TREE_MODEL (dir_list->list_store),
						 &iter, last_hover_path);
			gtk_tree_model_row_changed (GTK_TREE_MODEL (dir_list->list_store),
						    last_hover_path, &iter);
		}
		
		if (dir_list->hover_path) {
			gtk_tree_model_get_iter (GTK_TREE_MODEL (dir_list->list_store),
						 &iter, dir_list->hover_path);
			gtk_tree_model_row_changed (GTK_TREE_MODEL (dir_list->list_store),
						    dir_list->hover_path, &iter);
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
	DirList     *dir_list = user_data;
	GtkTreeIter  iter;

	if (dir_list->single_click && (dir_list->hover_path != NULL)) {
		gtk_tree_model_get_iter (GTK_TREE_MODEL (dir_list->list_store),
					 &iter, 
					 dir_list->hover_path);
		gtk_tree_model_row_changed (GTK_TREE_MODEL (dir_list->list_store),
					    dir_list->hover_path,
					    &iter);

		gtk_tree_path_free (dir_list->hover_path);
		dir_list->hover_path = NULL;

		return TRUE;
	}

	return FALSE;
}


DirList *
dir_list_new ()
{
	DirList     *dir_list = NULL;
	GtkTreeView *list_view;
	GtkWidget   *scrolled;

	dir_list = g_new0 (DirList, 1);

	/* Set default values. */

	dir_list->path = NULL;
	dir_list->try_path = NULL;
	dir_list->list = NULL;
	dir_list->file_list = NULL;
	dir_list->show_dot_files = eel_gconf_get_boolean (PREF_SHOW_HIDDEN_FILES, DEF_SHOW_HIDDEN);
	dir_list->old_dir = NULL;
	dir_list->dir_load_handle = NULL;
	dir_list->result = GNOME_VFS_OK;

	dir_list->single_click = (pref_get_real_click_policy () == GTH_CLICK_POLICY_SINGLE);
	dir_list->hover_path = NULL;

	/* Create the widgets. */

	scrolled = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled),
					GTK_POLICY_AUTOMATIC, 
					GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled),
                                             GTK_SHADOW_ETCHED_IN);

	dir_list->list_store = gtk_list_store_new (DIR_LIST_NUM_COLUMNS,
						   GDK_TYPE_PIXBUF,
						   G_TYPE_STRING,
						   G_TYPE_STRING);
	list_view = (GtkTreeView*) gtk_tree_view_new_with_model (GTK_TREE_MODEL (dir_list->list_store));
        gtk_tree_view_set_rules_hint (list_view, FALSE);
        add_columns (dir_list, list_view);
	gtk_tree_view_set_headers_visible (list_view, FALSE);
        gtk_tree_view_set_enable_search (list_view, TRUE);
        gtk_tree_view_set_search_column (list_view, DIR_LIST_COLUMN_UTF_NAME);

	g_signal_connect (G_OBJECT (list_view), 
			  "motion_notify_event",
			  G_CALLBACK (file_motion_notify_callback), 
			  dir_list);
	g_signal_connect (G_OBJECT (list_view), 
			  "leave_notify_event",
			  G_CALLBACK (file_leave_notify_callback), 
			  dir_list);

	/**/

	dir_list->list_view = (GtkWidget*) list_view;
	gtk_container_add (GTK_CONTAINER (scrolled), dir_list->list_view);
	dir_list->root_widget = scrolled;

	return dir_list;
}


void
dir_list_update_underline (DirList *dir_list)
{
	GdkWindow  *win;
	GdkDisplay *display;

	dir_list->single_click = (pref_get_real_click_policy () == GTH_CLICK_POLICY_SINGLE);

	win = gtk_tree_view_get_bin_window (GTK_TREE_VIEW (dir_list->list_view));
	if (win != NULL)
		gdk_window_set_cursor (win, NULL);

	display = gtk_widget_get_display (GTK_WIDGET (dir_list->list_view));
	if (display != NULL) 
		gdk_display_flush (display);
}


void
dir_list_free (DirList *dir_list)
{
	g_return_if_fail (dir_list != NULL);

	path_list_free (dir_list->file_list);
	path_list_free (dir_list->list);
	g_free (dir_list->path);
	g_free (dir_list->try_path);
	g_free (dir_list->old_dir);
	g_free (dir_list->dir_load_handle);
	g_free (dir_list);
}


static gboolean
is_a_film (DirList    *dir_list,
	   const char *name)
{
	gboolean   film = FALSE;
	char      *folder;

	folder = g_build_filename (dir_list->path, name, NULL);
	film = folder_is_film (folder);
	g_free (folder);

	return film;
}


static void
dir_list_update_view (DirList *dir_list)
{
	GdkPixbuf *dir_pixbuf;
	GdkPixbuf *up_pixbuf;
	GdkPixbuf *film_pixbuf;
	GList     *scan;

	dir_pixbuf = get_folder_pixbuf (get_folder_pixbuf_size_for_list (dir_list->list_view));
	up_pixbuf = gtk_widget_render_icon (dir_list->list_view,
					    GTK_STOCK_GO_UP,
					    GTK_ICON_SIZE_MENU,
					    NULL);
	film_pixbuf = gtk_widget_render_icon (dir_list->list_view,
					      GTHUMB_STOCK_FILM,
					      GTK_ICON_SIZE_MENU,
					      NULL);

	gtk_list_store_clear (dir_list->list_store);

	for (scan = dir_list->list; scan; scan = scan->next) {
		char        *name = scan->data;
		char        *utf8_name;
		GtkTreeIter  iter;
		GdkPixbuf   *pixbuf;

		if (strcmp (name, "..") == 0)
			pixbuf = dir_pixbuf /*up_pixbuf*/;
		else if (is_a_film (dir_list, name))
			pixbuf = film_pixbuf;
		else
			pixbuf = dir_pixbuf;

		utf8_name = g_filename_display_name (name);
		gtk_list_store_append (dir_list->list_store, &iter);
		gtk_list_store_set (dir_list->list_store, &iter,
				    DIR_LIST_COLUMN_ICON, pixbuf,
				    DIR_LIST_COLUMN_UTF_NAME, utf8_name,
				    DIR_LIST_COLUMN_NAME, name,
				    -1);

		g_free (utf8_name);
	}

	g_object_unref (dir_pixbuf);
	g_object_unref (film_pixbuf);
	g_object_unref (up_pixbuf);
}


void
dir_list_update_icon_theme (DirList *dir_list)
{
	dir_list_update_view (dir_list);
}


static void
dir_list_refresh_continue (PathListData *pld, 
			   gpointer      data)
{
	DirList   *dir_list = data;
	GList     *new_dir_list = NULL;
	GList     *new_file_list = NULL;
	GList     *filtered;

	if (pld == NULL) {
		if (dir_list->done_func) 
			dir_list->done_func (dir_list, dir_list->done_data);
		dir_list->done_func = NULL;
		return;
	}

	dir_list = data;
	dir_list->result = pld->result;

	if (dir_list->dir_load_handle != NULL) {
		g_free (dir_list->dir_load_handle);
		dir_list->dir_load_handle = NULL;
	}

	if (pld->result != GNOME_VFS_ERROR_EOF) {
		path_list_data_free (pld);
		if (dir_list->done_func) 
			dir_list->done_func (dir_list, dir_list->done_data);
		dir_list->done_func = NULL;
		return;
	}

	/* Update path data. */

	if (dir_list->old_dir != NULL) {
		g_free (dir_list->old_dir);
		dir_list->old_dir = NULL;
	}

	if (dir_list->path == NULL)
		dir_list->old_dir = NULL;
	else {
		char *previous_dir = remove_level_from_path (dir_list->path);

		if (same_uri (previous_dir, dir_list->try_path))
			dir_list->old_dir = g_strdup (file_name_from_path (dir_list->path));
		else
			dir_list->old_dir = NULL;
		g_free (previous_dir);
	}

	if (dir_list->path != NULL) 
		g_free (dir_list->path);
	dir_list->path = dir_list->try_path;
	dir_list->try_path = NULL;

	/**/

	new_file_list = pld->files;
	new_dir_list = pld->dirs;

	pld->files = NULL;
	pld->dirs = NULL;
	path_list_data_free (pld);

	/* Delete the old dir list. */

	if (dir_list->list != NULL) {
		g_list_foreach (dir_list->list, (GFunc) g_free, NULL);
		g_list_free (dir_list->list);
		dir_list->list = NULL;
	}

	/* Set the new file list. */

	path_list_free (dir_list->file_list);
	dir_list->file_list = new_file_list;

	/* Set the new dir list */

	filtered = dir_list_filter_and_sort (new_dir_list, 
					     TRUE, 
					     eel_gconf_get_boolean (PREF_SHOW_HIDDEN_FILES, DEF_SHOW_HIDDEN));

	/* * Add the ".." entry if the current path is not "/". 
	 * path_list_new does not include the "." and ".." elements. */

	if (! same_uri (dir_list->path, "/"))
		filtered = g_list_prepend (filtered, g_strdup (".."));

	dir_list->list = filtered;

	g_list_foreach (new_dir_list, (GFunc) g_free, NULL);
	g_list_free (new_dir_list);

	/* Update the view. */

	dir_list_update_view (dir_list);

	/* Make past dir visible in the list. */

	if (dir_list->old_dir) {
		GList *scan;
		int    row;
		int    found = FALSE;

		row = -1;
		scan = dir_list->list;
		while (scan && !found) {
			if (same_uri (dir_list->old_dir, scan->data)) 
				found = TRUE;
			scan = scan->next;
			row++;
		}

		if (found) {
			GtkTreePath *path;
			path = gtk_tree_path_new ();
			gtk_tree_path_prepend_index (path, row);
			gtk_tree_view_scroll_to_cell (GTK_TREE_VIEW (dir_list->list_view),
						      path,
						      NULL,
						      TRUE,
						      0.5,
						      0.0);
			gtk_tree_selection_select_path (gtk_tree_view_get_selection (GTK_TREE_VIEW (dir_list->list_view)), path);
			gtk_tree_path_free (path);
		}
		g_free (dir_list->old_dir);
		dir_list->old_dir = NULL;
	}

	if (dir_list->done_func) 
		dir_list->done_func (dir_list, dir_list->done_data);
	dir_list->done_func = NULL;
}


void
dir_list_change_to (DirList         *dir_list,
		    const gchar     *path,
		    DirListDoneFunc  func,
		    gpointer         data)
{
	g_return_if_fail (dir_list != NULL);

	dir_list->done_func = func;
	dir_list->done_data = data;

	if ((path != dir_list->try_path) && (dir_list->try_path != NULL)) 
		g_free (dir_list->try_path);
	dir_list->try_path = g_strdup (path);

	if (dir_list->dir_load_handle != NULL)
		path_list_handle_free (dir_list->dir_load_handle);

	dir_list->dir_load_handle = path_list_async_new (dir_list->try_path, dir_list_refresh_continue, dir_list);
}


void
dir_list_interrupt_change_to (DirList  *dir_list,
			      DoneFunc  f,
			      gpointer  data)
{
	g_return_if_fail (dir_list != NULL);

	if (dir_list->dir_load_handle != NULL) {
		path_list_async_interrupt (dir_list->dir_load_handle, f, data);
		dir_list->dir_load_handle = NULL;
	}
}


void
dir_list_add_directory (DirList         *dir_list,
			const char      *path)
{
	const char  *name_only;
	GList       *scan;
	GdkPixbuf   *dir_pixbuf;
	GdkPixbuf   *film_pixbuf;
	GdkPixbuf   *pixbuf;
	char        *utf8_name;
	GtkTreeIter  iter;
	int          pos;

	if (path == NULL)
		return;
	name_only = file_name_from_path (path);

	/* check whether dir is already present */

	for (scan = dir_list->list; scan; scan = scan->next) 
		if (same_uri (name_only, (char*)scan->data))
			return;

	/* insert dir in the list */
	
	if (! (file_is_hidden (name_only) 
	       && ! eel_gconf_get_boolean (PREF_SHOW_HIDDEN_FILES, DEF_SHOW_HIDDEN))
	    && ! same_uri (name_only, CACHE_DIR)) 
		dir_list->list = g_list_prepend (dir_list->list, g_strdup (name_only));
	dir_list->list = g_list_sort (dir_list->list, (GCompareFunc) strcasecmp);

	/* get the dir position */

	for (pos = 0, scan = dir_list->list; scan; scan = scan->next) {
		if (same_uri (name_only, (char*) scan->data))
			break;
		pos++;
	}

	/* insert dir in the list view */

	dir_pixbuf = get_folder_pixbuf (get_folder_pixbuf_size_for_list (dir_list->list_view));
	film_pixbuf = gtk_widget_render_icon (dir_list->list_view, GTHUMB_STOCK_FILM, GTK_ICON_SIZE_MENU, NULL);

	if (is_a_film (dir_list, name_only))
		pixbuf = film_pixbuf;
	else
		pixbuf = dir_pixbuf;

	utf8_name = g_filename_display_name (name_only);
	gtk_list_store_insert (dir_list->list_store, &iter, pos);
	gtk_list_store_set (dir_list->list_store, &iter,
			    DIR_LIST_COLUMN_ICON, pixbuf,
			    DIR_LIST_COLUMN_UTF_NAME, utf8_name,
			    DIR_LIST_COLUMN_NAME, name_only,
			    -1);
	g_free (utf8_name);
	g_object_unref (dir_pixbuf);
	g_object_unref (film_pixbuf);
}


void
dir_list_remove_directory (DirList         *dir_list,
			   const char      *path)
{
	GList        *scan, *link = NULL;
	const char   *name_only;
	int           pos;
	GtkTreeIter   iter;

	if (path == NULL)
		return;

	name_only = file_name_from_path (path);

	for (pos = 0, scan = dir_list->list; scan; scan = scan->next) {
		if (same_uri (name_only, (char*)scan->data)) {
			link = scan;
			break;
		}
		pos++;
	}

	if (link == NULL) 
		return;

	/**/
	
	dir_list->list = g_list_remove_link (dir_list->list, link);
	g_free (link->data);
	g_list_free (link);

	/**/

	if (! gtk_tree_model_iter_nth_child (GTK_TREE_MODEL (dir_list->list_store),
					     &iter,
					     NULL,
					     pos)) 
		return;

	gtk_list_store_remove (dir_list->list_store, &iter);
}


char *
dir_list_get_name_from_iter (DirList     *dir_list,
			     GtkTreeIter *iter)
{
	char *utf8_name;

	g_return_val_if_fail (dir_list != NULL, NULL);

	gtk_tree_model_get (GTK_TREE_MODEL (dir_list->list_store), 
			    iter,
			    DIR_LIST_COLUMN_UTF_NAME, &utf8_name,
			    -1);

	return utf8_name;
}


gchar *
dir_list_get_path_from_iter (DirList     *dir_list,
			     GtkTreeIter *iter)
{
	char *name;
	char *new_path;

	gtk_tree_model_get (GTK_TREE_MODEL (dir_list->list_store), 
			    iter,
			    DIR_LIST_COLUMN_NAME, &name,
			    -1);

	if (name == NULL) 
		return NULL;

	if (strcmp (name, ".") == 0)
		new_path = g_strdup (dir_list->path);
	else if (strcmp (name, "..") == 0)
		new_path = remove_level_from_path (dir_list->path);
	else {
		if (same_uri (dir_list->path, "/"))
			new_path = g_strconcat (dir_list->path, 
						name, 
						NULL);
		else
			new_path = g_strconcat (dir_list->path, 
						"/", 
						name, 
						NULL);
	}
	g_free (name);

	return new_path;	
}


int
dir_list_get_row_from_path (DirList     *dir_list,
			    const char  *path)
{
	GList      *scan;
	const char *name;
	char       *parent;
	int         pos;

	parent = remove_level_from_path (path);
	if (! same_uri (dir_list->path, parent)) {
		g_free (parent);
		return -1;
	}
	g_free (parent);

	name = file_name_from_path (path);

	for (pos = 0, scan = dir_list->list; scan; scan = scan->next) {
		if (same_uri (name, (char*) scan->data)) 
			return pos;
		pos++;
	}

	return -1;
}


char *
dir_list_get_path_from_tree_path (DirList     *dir_list,
				  GtkTreePath *path)
{
	GtkTreeIter iter;

	g_return_val_if_fail (dir_list != NULL, NULL);

	if (! gtk_tree_model_get_iter (GTK_TREE_MODEL (dir_list->list_store),
                                       &iter,
                                       path)) 
                return NULL;
	
	return dir_list_get_path_from_iter (dir_list, &iter);
}


gchar *
dir_list_get_path_from_row (DirList *dir_list,
			    int      row)
{
	GtkTreePath *path;
	gchar       *result;

	g_return_val_if_fail (dir_list != NULL, NULL);

	path = gtk_tree_path_new ();
	gtk_tree_path_append_index (path, row);
	result = dir_list_get_path_from_tree_path (dir_list, path);
	gtk_tree_path_free (path);

	return result;
}


gboolean
dir_list_get_selected_iter (DirList     *dir_list, 
			    GtkTreeIter *iter)
{
	GtkTreeView      *tree_view;
	GtkTreeSelection *selection;

	tree_view = GTK_TREE_VIEW (dir_list->list_view);
	selection = gtk_tree_view_get_selection (tree_view);
	return gtk_tree_selection_get_selected (selection, NULL, iter);
}


gchar *
dir_list_get_selected_path (DirList *dir_list)
{
	GtkTreeIter iter;
	if (! dir_list_get_selected_iter (dir_list, &iter))
		return NULL;
	return dir_list_get_path_from_iter (dir_list, &iter);
}


GList *
dir_list_get_file_list (DirList *dir_list)
{
	g_return_val_if_fail (dir_list != NULL, NULL);
	return path_list_dup (dir_list->file_list);
}
