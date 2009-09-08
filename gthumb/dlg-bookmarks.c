/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2001-2008 The Free Software Foundation, Inc.
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
#include <gtk/gtk.h>
#include "glib-utils.h"
#include "gth-browser.h"
#include "gth-main.h"
#include "gth-uri-list.h"
#include "gtk-utils.h"


typedef struct {
	GthBrowser *browser;
	GtkBuilder *builder;
	GtkWidget  *dialog;
	GtkWidget  *uri_list;
	gboolean    do_not_update;
	gulong      bookmarks_changed_id;
} DialogData;


/* called when the main dialog is closed. */
static void
destroy_cb (GtkWidget  *widget, 
	    DialogData *data)
{
	gth_browser_set_dialog (data->browser, "bookmarks", NULL);
	g_signal_handler_disconnect (gth_main_get_default_monitor (), data->bookmarks_changed_id);
	
	g_object_unref (data->builder);
	g_free (data);
}


static void
remove_cb (GtkWidget  *widget,
	   DialogData *data)
{
	char          *uri;
	GBookmarkFile *bookmarks;
	GError        *error = NULL;
	
	uri = gth_uri_list_get_selected (GTH_URI_LIST (data->uri_list));
	if (uri == NULL)
		return;
		
	bookmarks = gth_main_get_default_bookmarks ();
	if (! g_bookmark_file_remove_item (bookmarks, uri, &error)) {
		_gtk_error_dialog_from_gerror_show (GTK_WINDOW (data->dialog), _("Could not remove the bookmark"), &error);
	}
	gth_main_bookmarks_changed ();
	
	g_free (uri);
}


static void
go_to_cb (GtkWidget  *widget,
	  DialogData *data)
{
	char *uri;
		
	uri = gth_uri_list_get_selected (GTH_URI_LIST (data->uri_list));
	if (uri != NULL) {
		GFile *location;
		
		location = g_file_new_for_uri (uri);
		gth_browser_go_to (data->browser, location, NULL);
		
		g_object_unref (location);
		g_free (uri);
	}
}


static void
bookmarks_changed_cb (GthMonitor *monitor,
		      DialogData *data)
{
	GBookmarkFile  *bookmarks;
	char          **uris;
	
	bookmarks = gth_main_get_default_bookmarks ();
	uris = g_bookmark_file_get_uris (bookmarks, NULL);
	gth_uri_list_set_uris (GTH_URI_LIST (data->uri_list), uris);
	g_strfreev (uris);
}


static void
uri_list_order_changed_cb (GthUriList *uri_list,
		           DialogData *data)
{
	GBookmarkFile *bookmarks;
	GList         *uris;
	
	uris = gth_uri_list_get_uris (GTH_URI_LIST (data->uri_list));
	bookmarks = gth_main_get_default_bookmarks ();
	_g_bookmark_file_set_uris (bookmarks, uris);
	_g_string_list_free (uris);
	
	gth_main_bookmarks_changed ();
}


static void
uri_list_row_activated_cb (GtkTreeView       *tree_view,
                           GtkTreePath       *path,
                           GtkTreeViewColumn *column,
                           gpointer           user_data)
{
	DialogData   *data = user_data;
	GtkTreeModel *tree_model;
	GtkTreeIter   iter;
	char         *uri;
	GFile        *location;
	
	tree_model = gtk_tree_view_get_model (tree_view);
	if (! gtk_tree_model_get_iter (tree_model, &iter, path))
		return;
	
	uri = gth_uri_list_get_uri (GTH_URI_LIST (tree_view), &iter);
	if (uri == NULL)
		return;
		
	location = g_file_new_for_uri (uri);
	gth_browser_go_to (data->browser, location, NULL);
	
	g_object_unref (location);
	g_free (uri);
}


void
dlg_bookmarks (GthBrowser *browser)
{
	DialogData     *data;
	GtkWidget      *bm_list_container;
	GtkWidget      *bm_bookmarks_label;
	GtkWidget      *bm_remove_button;
	GtkWidget      *bm_close_button;
	GtkWidget      *bm_go_to_button;
	GBookmarkFile  *bookmarks;
	char          **uris;
	
	if (gth_browser_get_dialog (browser, "bookmarks") != NULL) {
		gtk_window_present (GTK_WINDOW (gth_browser_get_dialog (browser, "bookmarks")));
		return;
	}

	data = g_new0 (DialogData, 1);
	data->browser = browser;
	data->do_not_update = FALSE;
	data->builder = _gtk_builder_new_from_file ("bookmarks.ui", NULL); 

	/* Get the widgets. */

	data->dialog = _gtk_builder_get_widget (data->builder, "bookmarks_dialog");
	gth_browser_set_dialog (browser, "bookmarks", data->dialog);
	g_object_set_data (G_OBJECT (data->dialog), "dialog_data", data);

	bm_list_container = _gtk_builder_get_widget (data->builder, "bm_list_container");
	bm_bookmarks_label = _gtk_builder_get_widget (data->builder, "bm_bookmarks_label");
	bm_remove_button = _gtk_builder_get_widget (data->builder, "bm_remove_button");
	bm_close_button = _gtk_builder_get_widget (data->builder, "bm_close_button");
	bm_go_to_button = _gtk_builder_get_widget (data->builder, "bm_go_to_button");
	
	data->uri_list = gth_uri_list_new ();
	gtk_widget_show (data->uri_list);
	gtk_container_add (GTK_CONTAINER (bm_list_container), data->uri_list);
	gtk_label_set_mnemonic_widget (GTK_LABEL (bm_bookmarks_label), data->uri_list);

	/* Set widgets data. */

	bookmarks = gth_main_get_default_bookmarks ();
	uris = g_bookmark_file_get_uris (bookmarks, NULL);
	gth_uri_list_set_uris (GTH_URI_LIST (data->uri_list), uris);
	g_strfreev (uris);

	data->bookmarks_changed_id = g_signal_connect (gth_main_get_default_monitor (), 
				                       "bookmarks-changed",
				                       G_CALLBACK (bookmarks_changed_cb), 
				                       data);

	/* Set the signals handlers. */
	
	g_signal_connect (G_OBJECT (data->dialog), 
			  "destroy",
			  G_CALLBACK (destroy_cb),
			  data);
	g_signal_connect_swapped (G_OBJECT (bm_close_button), 
				  "clicked",
				  G_CALLBACK (gtk_widget_destroy),
				  G_OBJECT (data->dialog));
	g_signal_connect (G_OBJECT (bm_remove_button), 
			  "clicked",
			  G_CALLBACK (remove_cb),
			  data);
	g_signal_connect (G_OBJECT (bm_go_to_button), 
			  "clicked",
			  G_CALLBACK (go_to_cb),
			  data);
	g_signal_connect (G_OBJECT (data->uri_list), 
			  "order-changed",
			  G_CALLBACK (uri_list_order_changed_cb),
			  data);
	g_signal_connect (G_OBJECT (data->uri_list), 
			  "row-activated",
			  G_CALLBACK (uri_list_row_activated_cb),
			  data);
			  
	/* run dialog. */

	gtk_window_set_transient_for (GTK_WINDOW (data->dialog), 
				      GTK_WINDOW (browser));
	gtk_window_set_modal (GTK_WINDOW (data->dialog), FALSE);
	gtk_widget_show (data->dialog);
}
