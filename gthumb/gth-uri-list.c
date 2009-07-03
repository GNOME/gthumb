/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2008 Free Software Foundation, Inc.
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
#include "gth-file-source.h"
#include "gth-icon-cache.h"
#include "gth-main.h"
#include "gth-uri-list.h"

#define ORDER_CHANGED_DELAY 250


enum {
	ORDER_CHANGED,
	LAST_SIGNAL
};


enum {
	URI_LIST_COLUMN_ICON,
	URI_LIST_COLUMN_NAME,
	URI_LIST_COLUMN_URI,
	URI_LIST_NUM_COLUMNS
};


struct _GthUriListPrivate {
	GtkListStore *list_store;
	GthIconCache *icon_cache;
	guint         changed_id;
};


static gpointer parent_class = NULL;
static guint uri_list_signals[LAST_SIGNAL] = { 0 };


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
					     "pixbuf", URI_LIST_COLUMN_ICON,
					     NULL);

	renderer = gtk_cell_renderer_text_new ();
        gtk_tree_view_column_pack_start (column,
                                         renderer,
                                         TRUE);
        gtk_tree_view_column_set_attributes (column, renderer,
                                             "text", URI_LIST_COLUMN_NAME,
                                             NULL);

        gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
        gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);
}


static gboolean
order_changed (gpointer user_data)
{
	GthUriList *uri_list = user_data;

	if (uri_list->priv->changed_id != 0)
		g_source_remove (uri_list->priv->changed_id);
	uri_list->priv->changed_id = 0;

	g_signal_emit (G_OBJECT (uri_list),
		       uri_list_signals[ORDER_CHANGED],
		       0);

	return FALSE;
}


static void
row_deleted_cb (GtkTreeModel *tree_model,
		GtkTreePath  *path,
		gpointer      user_data)
{
	GthUriList *uri_list = user_data;

	if (uri_list->priv->changed_id != 0)
		g_source_remove (uri_list->priv->changed_id);
	uri_list->priv->changed_id = g_timeout_add (ORDER_CHANGED_DELAY, order_changed, uri_list);
}


static void
row_inserted_cb (GtkTreeModel *tree_model,
		 GtkTreePath  *path,
		 GtkTreeIter  *iter,
		 gpointer      user_data)
{
	GthUriList *uri_list = user_data;

	if (uri_list->priv->changed_id != 0)
		g_source_remove (uri_list->priv->changed_id);
	uri_list->priv->changed_id = g_timeout_add (ORDER_CHANGED_DELAY, order_changed, uri_list);
}


static void
gth_uri_list_init (GthUriList *uri_list)
{
	uri_list->priv = g_new0 (GthUriListPrivate, 1);

	uri_list->priv->list_store = gtk_list_store_new (URI_LIST_NUM_COLUMNS,
						         GDK_TYPE_PIXBUF,
						         G_TYPE_STRING,
						         G_TYPE_STRING);
	g_signal_connect (uri_list->priv->list_store,
			  "row-deleted",
			  G_CALLBACK (row_deleted_cb),
			  uri_list);
	g_signal_connect (uri_list->priv->list_store,
			  "row-inserted",
			  G_CALLBACK (row_inserted_cb),
			  uri_list);

	gtk_tree_view_set_model (GTK_TREE_VIEW (uri_list), GTK_TREE_MODEL (uri_list->priv->list_store));
	g_object_unref (uri_list->priv->list_store);

	gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (uri_list), FALSE);
        add_columns (GTK_TREE_VIEW (uri_list));
	gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (uri_list), FALSE);
        gtk_tree_view_set_enable_search (GTK_TREE_VIEW (uri_list), TRUE);
        gtk_tree_view_set_search_column (GTK_TREE_VIEW (uri_list), URI_LIST_COLUMN_NAME);
        gtk_tree_view_set_reorderable (GTK_TREE_VIEW (uri_list), TRUE);

        uri_list->priv->icon_cache = gth_icon_cache_new_for_widget (GTK_WIDGET (uri_list), GTK_ICON_SIZE_MENU);
	uri_list->priv->changed_id = 0;
}


static void
gth_uri_list_finalize (GObject *object)
{
	GthUriList *uri_list = GTH_URI_LIST (object);

	if (uri_list->priv != NULL) {
		g_free (uri_list->priv);
		uri_list->priv = NULL;
	}

	G_OBJECT_CLASS (parent_class)->finalize (object);
}


static void
gth_uri_list_class_init (GthUriListClass *klass)
{
	GObjectClass *gobject_class;

	parent_class = g_type_class_peek_parent (klass);
	gobject_class = G_OBJECT_CLASS (klass);

	gobject_class->finalize = gth_uri_list_finalize;

	uri_list_signals[ORDER_CHANGED] =
		g_signal_new ("order-changed",
			      G_TYPE_FROM_CLASS (klass),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (GthUriListClass, order_changed),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__VOID,
			      G_TYPE_NONE,
			      0);
}


GType
gth_uri_list_get_type (void)
{
	static GType type = 0;

	if (type == 0) {
		static const GTypeInfo g_define_type_info = {
			sizeof (GthUriListClass),
			(GBaseInitFunc) NULL,
			(GBaseFinalizeFunc) NULL,
			(GClassInitFunc) gth_uri_list_class_init,
			(GClassFinalizeFunc) NULL,
			NULL,
			sizeof (GthUriList),
			0,
			(GInstanceInitFunc) gth_uri_list_init,
			NULL
		};
		type = g_type_register_static (GTK_TYPE_TREE_VIEW,
					       "GthUriList",
					       &g_define_type_info,
					       0);
	}

	return type;
}


GtkWidget *
gth_uri_list_new (void)
{
	return g_object_new (GTH_TYPE_URI_LIST, NULL);
}


void
gth_uri_list_set_uris (GthUriList  *uri_list,
		       char       **uris)
{
	int i;

	g_return_if_fail (uri_list != NULL);

	g_signal_handlers_block_by_func (uri_list->priv->list_store, row_deleted_cb, uri_list);
	g_signal_handlers_block_by_func (uri_list->priv->list_store, row_inserted_cb, uri_list);

	gtk_list_store_clear (uri_list->priv->list_store);

	for (i = 0; uris[i] != NULL; i++) {
		char          *uri = uris[i];
		GFile         *file;
		GthFileSource *file_source;
		GFileInfo     *info;
		const char    *display_name;
		GIcon         *icon;
		GdkPixbuf     *pixbuf;
		GtkTreeIter    iter;

		file = g_file_new_for_uri (uri);
		file_source = gth_main_get_file_source (file);
		info = gth_file_source_get_file_info (file_source, file, GFILE_DISPLAY_ATTRIBUTES);

		display_name = g_file_info_get_display_name (info);
		icon = g_file_info_get_icon (info);
		pixbuf = gth_icon_cache_get_pixbuf (uri_list->priv->icon_cache, icon);

		gtk_list_store_append (uri_list->priv->list_store, &iter);
		gtk_list_store_set (uri_list->priv->list_store, &iter,
				    URI_LIST_COLUMN_ICON, pixbuf,
				    URI_LIST_COLUMN_NAME, display_name,
				    URI_LIST_COLUMN_URI, uri,
				    -1);

		g_object_unref (pixbuf);
		g_object_unref (file_source);
		g_object_unref (file);
	}

	g_signal_handlers_unblock_by_func (uri_list->priv->list_store, row_deleted_cb, uri_list);
	g_signal_handlers_unblock_by_func (uri_list->priv->list_store, row_inserted_cb, uri_list);
}


char *
gth_uri_list_get_uri (GthUriList  *uri_list,
		      GtkTreeIter *iter)
{
	char *uri;

	gtk_tree_model_get (GTK_TREE_MODEL (uri_list->priv->list_store),
			    iter,
			    URI_LIST_COLUMN_URI, &uri,
			    -1);

	return uri;
}


char *
gth_uri_list_get_selected (GthUriList *uri_list)
{
	GtkTreeSelection *selection;
	GtkTreeIter       iter;
	char             *uri;

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (uri_list));
	if (selection == NULL)
		return NULL;

	if (! gtk_tree_selection_get_selected (selection, NULL, &iter))
		return NULL;

	gtk_tree_model_get (GTK_TREE_MODEL (uri_list->priv->list_store),
			    &iter,
			    URI_LIST_COLUMN_URI, &uri,
			    -1);

	return uri;
}


GList *
gth_uri_list_get_uris (GthUriList *uri_list)
{
	GtkTreeModel *model = GTK_TREE_MODEL (uri_list->priv->list_store);
	GList        *uris = NULL;
	GtkTreeIter   iter;

	if (! gtk_tree_model_get_iter_first (model, &iter))
		return NULL;

	do {
		char *uri;

		gtk_tree_model_get (model, &iter, URI_LIST_COLUMN_URI, &uri, -1);
		uris = g_list_prepend (uris, uri);
	}
	while (gtk_tree_model_iter_next (model, &iter));

	return g_list_reverse (uris);
}
