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
#include <libgnome/libgnome.h>
#include <libgnomeui/gnome-dialog.h>
#include <libgnomeui/gnome-dialog-util.h>
#include <glade/glade.h>
#include "typedefs.h"
#include "bookmarks.h"
#include "bookmark-list.h"
#include "main.h"
#include "gthumb-window.h"


typedef struct {
	GThumbWindow    *window;

	GladeXML  *gui;
	GtkWidget *dialog;

	GtkWidget *list_container;
	GtkWidget *btn_remove;
	GtkWidget *btn_ok;
	GtkWidget *up_button;
	GtkWidget *down_button;
	GtkWidget *bottom_button;
	GtkWidget *top_button;

	BookmarkList *book_list;
	Bookmarks *bookmarks;
} DialogData;


/* called when the main dialog is closed. */
static void
destroy_cb (GtkWidget *widget, 
	    DialogData *data)
{
	g_object_unref (G_OBJECT (data->gui));
	bookmark_list_free (data->book_list);
	bookmarks_free (data->bookmarks);
	g_free (data);

	all_windows_update_bookmark_list ();
}


static void
remove_cb (GtkWidget *widget,
	   DialogData *data)
{
	gchar *path;

	path = bookmark_list_get_selected_path (data->book_list);
	if (path == NULL)
		return;

	bookmarks_remove (data->bookmarks, path);
	g_free (path);

	bookmarks_write_to_disk (data->bookmarks);
	bookmark_list_set (data->book_list, data->bookmarks->list);
}


static void
move_up_cb (GtkWidget *widget,
	    DialogData *data)
{
	GList    *list, *link, *prev;
	char     *path;
	gpointer  ldata;

	path = bookmark_list_get_selected_path (data->book_list);
	if (path == NULL)
		return;
	
	list = data->bookmarks->list;

	link = g_list_find_custom (list, path, (GCompareFunc) strcmp);
	if (link == NULL) {
		g_free (path);
		return;
	}

	prev = g_list_previous (link);
	if (prev == NULL) {
		g_free (path);
		return;
	}

	ldata = link->data;

	list = g_list_remove_link (list, link);
	list = g_list_insert_before (list, prev, ldata);
	g_list_free (link);
	data->bookmarks->list = list;

	g_free (path);

	bookmarks_write_to_disk (data->bookmarks);
	bookmark_list_set (data->book_list, data->bookmarks->list);

	bookmark_list_select_item (data->book_list, g_list_index (list, ldata));
}


static void
move_down_cb (GtkWidget *widget,
	      DialogData *data)
{
	GList    *list, *link, *next;
	gpointer  ldata;
	char     *path;
	int       i;

	path = bookmark_list_get_selected_path (data->book_list);
	if (path == NULL)
		return;
	
	list = data->bookmarks->list;

	link = g_list_find_custom (list, path, (GCompareFunc) strcmp);
	if (link == NULL) {
		g_free (path);
		return;
	}

	next = g_list_next (link);
	if (next == NULL) {
		g_free (path);
		return;
	}

	ldata = link->data;

	i = g_list_position (list, link);
	list = g_list_remove_link (list, link);
	list = g_list_insert (list, ldata, i + 1);
	g_list_free (link);
	data->bookmarks->list = list;

	g_free (path);

	bookmarks_write_to_disk (data->bookmarks);
	bookmark_list_set (data->book_list, data->bookmarks->list);

	bookmark_list_select_item (data->book_list, g_list_index (list, ldata));
}


static void
move_top_cb (GtkWidget *widget,
	     DialogData *data)
{
	GList    *list, *link;
	gpointer  ldata;
	char     *path;

	path = bookmark_list_get_selected_path (data->book_list);
	if (path == NULL)
		return;
	
	list = data->bookmarks->list;

	link = g_list_find_custom (list, path, (GCompareFunc) strcmp);
	if (link == NULL) {
		g_free (path);
		return;
	}

	ldata = link->data;

	list = g_list_remove_link (list, link);
	list = g_list_insert (list, ldata, 0);
	g_list_free (link);
	data->bookmarks->list = list;

	g_free (path);

	bookmarks_write_to_disk (data->bookmarks);
	bookmark_list_set (data->book_list, data->bookmarks->list);

	bookmark_list_select_item (data->book_list, g_list_index (list, ldata));
}


static void
move_bottom_cb (GtkWidget *widget,
		DialogData *data)
{
	GList    *list, *link;
	gpointer  ldata;
	char     *path;

	path = bookmark_list_get_selected_path (data->book_list);
	if (path == NULL)
		return;
	
	list = data->bookmarks->list;

	link = g_list_find_custom (list, path, (GCompareFunc) strcmp);
	if (link == NULL) {
		g_free (path);
		return;
	}

	ldata = link->data;

	list = g_list_remove_link (list, link);
	list = g_list_insert (list, ldata, g_list_length (list));
	g_list_free (link);
	data->bookmarks->list = list;

	g_free (path);

	bookmarks_write_to_disk (data->bookmarks);
	bookmark_list_set (data->book_list, data->bookmarks->list);

	bookmark_list_select_item (data->book_list, g_list_index (list, ldata));
}


void
dlg_edit_bookmarks (GThumbWindow *window)
{
	DialogData *data;
	GtkWidget  *bm_bookmarks_label;

	data = g_new (DialogData, 1);

	data->window = window;

	data->gui = glade_xml_new (GTHUMB_GLADEDIR "/" GLADE_FILE, NULL, NULL);
        if (! data->gui) {
                g_warning ("Could not find " GLADE_FILE "\n");
                return;
        }

	/* Get the widgets. */

	data->dialog = glade_xml_get_widget (data->gui, "bookmarks_dialog");
	data->list_container = glade_xml_get_widget (data->gui, "bm_list_container");
	bm_bookmarks_label = glade_xml_get_widget (data->gui, "bm_bookmarks_label");
	data->btn_remove = glade_xml_get_widget (data->gui, "bm_remove_button");
	data->btn_ok = glade_xml_get_widget (data->gui, "bm_btn_ok");

	data->up_button = glade_xml_get_widget (data->gui, "bm_up_button");
	data->down_button = glade_xml_get_widget (data->gui, "bm_down_button");
	data->top_button = glade_xml_get_widget (data->gui, "bm_top_button");
	data->bottom_button = glade_xml_get_widget (data->gui, "bm_bottom_button");

	data->book_list = bookmark_list_new ();
	gtk_container_add (GTK_CONTAINER (data->list_container), 
			   data->book_list->root_widget);
	gtk_label_set_mnemonic_widget (GTK_LABEL (bm_bookmarks_label), 
				       data->book_list->list_view);

	/* Set widgets data. */

	data->bookmarks = bookmarks_new (RC_BOOKMARKS_FILE);
	bookmarks_load_from_disk (data->bookmarks);
	bookmark_list_set (data->book_list, data->bookmarks->list);

	/* Set the signals handlers. */
	
	g_signal_connect (G_OBJECT (data->dialog), 
			  "destroy",
			  G_CALLBACK (destroy_cb),
			  data);

	g_signal_connect_swapped (G_OBJECT (data->btn_ok), 
				  "clicked",
				  G_CALLBACK (gtk_widget_destroy),
				  G_OBJECT (data->dialog));

	g_signal_connect (G_OBJECT (data->btn_remove), 
			  "clicked",
			  G_CALLBACK (remove_cb),
			  data);

	g_signal_connect (G_OBJECT (data->up_button), 
			  "clicked",
			  G_CALLBACK (move_up_cb),
			  data);

	g_signal_connect (G_OBJECT (data->down_button), 
			  "clicked",
			  G_CALLBACK (move_down_cb),
			  data);

	g_signal_connect (G_OBJECT (data->top_button), 
			  "clicked",
			  G_CALLBACK (move_top_cb),
			  data);

	g_signal_connect (G_OBJECT (data->bottom_button), 
			  "clicked",
			  G_CALLBACK (move_bottom_cb),
			  data);

	/* run dialog. */

	gtk_window_set_transient_for (GTK_WINDOW (data->dialog), 
				      GTK_WINDOW (window->app));
	gtk_window_set_modal (GTK_WINDOW (data->dialog), TRUE);
	gtk_widget_show_all (data->dialog);
}



