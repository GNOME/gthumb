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
#include <gdk/gdk.h>
#include <gdk/gdkkeysyms.h>
#include <gtk/gtkentry.h>
#include <gtk/gtkframe.h>
#include <gtk/gtkmain.h>
#include <gtk/gtkwindow.h>
#include <gtk/gtktreeview.h>
#include <gtk/gtktreeselection.h>
#include <gtk/gtkliststore.h>
#include <gtk/gtkcellrenderertext.h>
#include <gtk/gtkscrolledwindow.h>

#include "main.h"
#include "auto-completion.h"
#include "file-utils.h"
#include "typedefs.h"
#include "gthumb-window.h"

#include "icons/pixbufs.h"


#define MAX_VISIBLE_ROWS 8

enum {
	AC_LIST_COLUMN_ICON,
	AC_LIST_COLUMN_NAME,
	AC_LIST_NUM_COLUMNS
};

GThumbWindow        *gthumb_window         = NULL;
static gchar        *ac_dir                = NULL;
static gchar        *ac_path               = NULL;
static GList        *ac_subdirs            = NULL; 
static GList        *ac_alternatives       = NULL;
static gboolean      ac_change_to_hidden   = FALSE;

static GtkWidget    *ac_window             = NULL;
static GtkWidget    *ac_scrolled_window    = NULL;
static GtkWidget    *ac_list_view          = NULL;
static GtkListStore *ac_list_store         = NULL;
static GtkWidget    *ac_entry              = NULL;


static void 
ac_dir_free ()
{
	if (!ac_dir)
		return;

	g_free (ac_dir);
	ac_dir = NULL;
}


static void 
ac_path_free ()
{
	if (!ac_path)
		return;

	g_free (ac_path);
	ac_path = NULL;
}


static void 
ac_subdirs_free ()
{
	if (!ac_subdirs)
		return;

	g_list_foreach (ac_subdirs, (GFunc) g_free, NULL);
	g_list_free (ac_subdirs);
	ac_subdirs = NULL;
}


static void 
ac_alternatives_free ()
{
	if (!ac_alternatives)
		return;

	g_list_foreach (ac_alternatives, (GFunc) g_free, NULL);
	g_list_free (ac_alternatives);
	ac_alternatives = NULL;
}


void
auto_compl_reset () 
{
	ac_dir_free ();
	ac_path_free ();
	ac_subdirs_free ();
	ac_alternatives_free ();
}


int
auto_compl_get_n_alternatives (const char *path)
{
	char       *dir;
	const char *name;
	int         path_len;
	GList      *scan;
	int         n;

	if (path == NULL)
		return 0;

	name = file_name_from_path (path);
	ac_change_to_hidden = (name != NULL) && (*name == '.');

	if (strcmp (path, "/") == 0)
		dir = g_strdup ("/");
	else
		dir = remove_level_from_path (path);
	
	if (! path_is_dir (dir)) {
		g_free (dir);
		return 0;
	}

	if ((ac_dir == NULL) || (strcmp (dir, ac_dir) != 0)) {
		GList *row_dir_list;

		ac_dir_free ();
		ac_subdirs_free ();

		ac_dir = g_strdup (dir);
		path_list_new (ac_dir, NULL, &row_dir_list);
		ac_subdirs = dir_list_filter_and_sort (row_dir_list, 
						       FALSE,
						       TRUE);
		path_list_free (row_dir_list);
	}

	ac_path_free ();
	ac_alternatives_free ();

	ac_path = g_strdup (path);
	path_len = strlen (ac_path);
	n = 0;

	for (scan = ac_subdirs; scan; scan = scan->next) {
		const char *subdir = (gchar*) scan->data;

		if (strncmp (path, subdir, path_len) != 0) 
			continue;
		
		ac_alternatives = g_list_prepend (ac_alternatives, 
						  g_strdup (subdir));
		n++;
	}

	g_free (dir);
	ac_alternatives = g_list_reverse (ac_alternatives);

	return n;
}


static gint
get_common_prefix_length () 
{
	int    n;
	GList *scan;
	char   c1, c2;

	g_return_val_if_fail (ac_path != NULL, 0);
	g_return_val_if_fail (ac_alternatives != NULL, 0);

	/* If the number of alternatives is 1 return its length. */

	if (ac_alternatives->next == NULL)
		return strlen ((char*) ac_alternatives->data);

	n = strlen (ac_path);
	while (TRUE) {
		scan = ac_alternatives;

		c1 = ((char*) scan->data) [n];

		if (c1 == 0)
			return n;

		/* check that all other alternatives have the same 
		 * character at position n. */

		for (scan = scan->next; scan; scan = scan->next) {
			c2 = ((char*) scan->data) [n];
			if (c1 != c2)
				return n;
		}

		n++;
	}

	return -1;
}


gchar *
auto_compl_get_common_prefix () 
{
	char *alternative;
	int n;

	if (ac_path == NULL)
		return NULL;

	if (ac_alternatives == NULL)
		return NULL;

	n = get_common_prefix_length ();
	alternative = (gchar*) ac_alternatives->data;

	return g_strndup (alternative, n);
}


GList * 
auto_compl_get_alternatives ()
{
	return ac_alternatives;
}


static gboolean
ac_window_button_press_cb (GtkWidget      *widget,
			   GdkEventButton *event,
			   gpointer       *data)
{
	GtkWidget *event_widget;
	int        x, y, w, h;
	
	event_widget = gtk_get_event_widget ((GdkEvent *)event);

	gdk_window_get_origin (ac_window->window, &x, &y);
	gdk_drawable_get_size (ac_window->window, &w, &h);

	/* Checks if the button press happened inside the window, 
	 * if not closes the window. */
	if ((event->x >= x) && (event->x <= x + w)
	    && (event->y  >= y) && (event->y <= y + h)) {
		/* In window. */
		return FALSE;
	}

	auto_compl_hide_alternatives ();
	return TRUE;
}


static void
ac_list_row_activated_cb (GtkTreeView       *tree_view,
			  GtkTreePath       *path,
			  GtkTreeViewColumn *column,
			  gpointer           user_data)
{
	GtkTreeIter  iter;
	char        *full_path;
	char        *name;
	char        *utf8_name;

	if (! gtk_tree_model_get_iter (GTK_TREE_MODEL (ac_list_store),
				       &iter,
				       path))
		return;
	gtk_tree_model_get (GTK_TREE_MODEL (ac_list_store), &iter,
                            AC_LIST_COLUMN_NAME, &utf8_name,
                            -1);
	g_return_if_fail (utf8_name != NULL);

	name = g_filename_from_utf8 (utf8_name, -1, NULL, NULL, NULL);
	g_free (utf8_name);

	full_path = g_build_path ("/", ac_dir, name, NULL);

	auto_compl_hide_alternatives ();

	if (gthumb_window->sidebar_content == GTH_SIDEBAR_DIR_LIST)
		window_go_to_directory (gthumb_window, full_path);
	else
		window_go_to_catalog_directory (gthumb_window, full_path);

	g_free (full_path);
	g_free (name);
}


static gboolean
ac_window_key_press_cb (GtkWidget   *widget,
			GdkEventKey *event,
			gpointer    *data)
{
	if (event->keyval == GDK_Escape) {
		auto_compl_hide_alternatives ();
		return TRUE;
	}

	/* Allow keyboard navigation in the alternatives clist */

	if (event->keyval == GDK_Up 
	    || event->keyval == GDK_Down 
	    || event->keyval == GDK_Page_Up 
	    || event->keyval == GDK_Page_Down 
	    || event->keyval == GDK_space)
		return FALSE;

	if (event->keyval == GDK_Return) {
		GtkTreeSelection *selection;
		GtkTreeIter       iter;
		GtkTreePath      *path;

		selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (ac_list_view));
		if (!gtk_tree_selection_get_selected (selection, NULL, &iter))
			return FALSE;
		
		path = gtk_tree_model_get_path (GTK_TREE_MODEL (ac_list_store),
						&iter);
		ac_list_row_activated_cb (GTK_TREE_VIEW (ac_list_view),
					  path, NULL, NULL);
		gtk_tree_path_free (path);

		return FALSE;
	}

	if ((event->keyval == GDK_Tab) 
	    && ((event->state & GDK_CONTROL_MASK) == GDK_CONTROL_MASK)) 
		return TRUE;

	auto_compl_hide_alternatives ();
	gtk_widget_event (ac_entry, (GdkEvent*) event);

	return TRUE;
}


static void
add_columns (GtkTreeView *treeview)
{
	GtkCellRenderer   *renderer;
	GtkTreeViewColumn *column;
	
	/* The Name column. */

	column = gtk_tree_view_column_new ();
	
	renderer = gtk_cell_renderer_pixbuf_new ();
	gtk_tree_view_column_pack_start (column, renderer, FALSE);
	gtk_tree_view_column_set_attributes (column, renderer,
					     "pixbuf", AC_LIST_COLUMN_ICON,
					     NULL);
	
	renderer = gtk_cell_renderer_text_new ();
        gtk_tree_view_column_pack_start (column,
                                         renderer,
                                         TRUE);
        gtk_tree_view_column_set_attributes (column, renderer,
                                             "text", AC_LIST_COLUMN_NAME,
                                             NULL);

        gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);        
	gtk_tree_view_column_set_sort_column_id (column, AC_LIST_COLUMN_NAME);
        gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);
}


/* displays a list of alternatives under the entry widget. */
void
auto_compl_show_alternatives (GThumbWindow *window,
			      GtkWidget    *entry)
{
	GdkPixbuf *pixbuf;
	int        x, y, w, h;
	GList     *scan;
	int        n, width;

	gthumb_window = window;

	if (ac_window == NULL) {
		GtkWidget *scroll;
		GtkWidget *frame;

		ac_window = gtk_window_new (GTK_WINDOW_POPUP);

		ac_list_store = gtk_list_store_new (AC_LIST_NUM_COLUMNS,
						    GDK_TYPE_PIXBUF,
						    G_TYPE_STRING);
		ac_list_view = gtk_tree_view_new_with_model (GTK_TREE_MODEL (ac_list_store));
		gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (ac_list_view), TRUE);
		gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (ac_list_view), FALSE);
		gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (ac_list_view), FALSE);

		add_columns (GTK_TREE_VIEW (ac_list_view));

		scroll = gtk_scrolled_window_new (NULL, NULL);
		ac_scrolled_window = scroll;
		gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scroll),
						GTK_POLICY_AUTOMATIC,
						GTK_POLICY_AUTOMATIC);

		frame = gtk_frame_new (NULL);
		gtk_frame_set_shadow_type (GTK_FRAME (frame), 
					   GTK_SHADOW_ETCHED_IN);

		gtk_container_add (GTK_CONTAINER (ac_window), frame);
		gtk_container_add (GTK_CONTAINER (frame), scroll);
		gtk_container_add (GTK_CONTAINER (scroll), ac_list_view);

		g_signal_connect (G_OBJECT (ac_window),
				  "button_press_event",
				  G_CALLBACK (ac_window_button_press_cb),
				  NULL);
		g_signal_connect (G_OBJECT (ac_window),
				  "key_press_event",
				  G_CALLBACK (ac_window_key_press_cb),
				  NULL);
		g_signal_connect (G_OBJECT (ac_list_view), 
				  "row_activated", 
				  G_CALLBACK (ac_list_row_activated_cb), 
				  NULL);
	}

	ac_entry = entry;

	width = 0;
	n = 0;

	pixbuf = (window->sidebar_content == GTH_SIDEBAR_DIR_LIST) ? get_folder_pixbuf (get_default_folder_pixbuf_size (window->app)) : gdk_pixbuf_new_from_inline (-1, library_19_rgba, FALSE, NULL);

	gtk_list_store_clear (ac_list_store);
	for (scan = ac_alternatives; scan; scan = scan->next) {
		GtkTreeIter  iter;
		const char  *name_only;
		char        *utf8_name;

		name_only = file_name_from_path (scan->data);

		/* Only show hidden directories when the user is trying to
		 * change into an hidden directory. */
		if (! ac_change_to_hidden && (*name_only == '.'))
			continue;

		utf8_name = g_filename_to_utf8 (name_only, -1, NULL, NULL, NULL);
		gtk_list_store_append (ac_list_store, &iter);
		gtk_list_store_set (ac_list_store, &iter,
				    AC_LIST_COLUMN_ICON, pixbuf,
				    AC_LIST_COLUMN_NAME, utf8_name,
				    -1);
		g_free (utf8_name);

		n++;
	}
	g_object_unref (pixbuf);

	gdk_window_get_origin (entry->window, &x, &y);
	gdk_window_get_geometry (entry->window, NULL, NULL, &w, &h, NULL);
	gtk_window_move (GTK_WINDOW (ac_window), x, y + h);
	gtk_widget_set_size_request (ac_window, w, 200);

	gtk_widget_show_all (ac_window);

	gdk_pointer_grab (ac_window->window, 
			  TRUE,
			  (GDK_POINTER_MOTION_MASK 
			   | GDK_BUTTON_PRESS_MASK 
			   | GDK_BUTTON_RELEASE_MASK),
			  NULL, 
			  NULL, 
			  GDK_CURRENT_TIME);
	gdk_keyboard_grab (ac_window->window,
			   FALSE,
			   GDK_CURRENT_TIME);
	gtk_grab_add (ac_window);
}


void
auto_compl_hide_alternatives ()
{
	if (ac_window && GTK_WIDGET_VISIBLE (ac_window)) {
		gdk_pointer_ungrab (GDK_CURRENT_TIME);
		gdk_keyboard_ungrab (GDK_CURRENT_TIME);
		gtk_grab_remove (ac_window);

		gtk_widget_destroy (ac_window);
		ac_window = NULL;
	}
}


gboolean
auto_compl_alternatives_visible ()
{
	return (ac_window != NULL);
}

