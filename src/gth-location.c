/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2005 Free Software Foundation, Inc.
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
#include <gtk/gtk.h>
#include <libgnomevfs/gnome-vfs-mime.h>
#include <libgnomevfs/gnome-vfs-file-info.h>
#include <libgnomevfs/gnome-vfs-utils.h>

#include "bookmarks.h"
#include "file-utils.h"
#include "glib-utils.h"
#include "gth-location.h"
#include "gthumb-stock.h"
#include "main.h"

#include "icons/pixbufs.h"

enum {
	ITEM_TYPE_NONE,
	ITEM_TYPE_PARENT,
	ITEM_TYPE_BOOKMARK,
	ITEM_TYPE_SEPARATOR,
	ITEM_TYPE_OPEN_LOCATION
};

enum {
	ICON_COLUMN,
	NAME_COLUMN,
	PATH_COLUMN,
	TYPE_COLUMN,
	N_COLUMNS
};

enum {
        CHANGED,
	OPEN_LOCATION,
        LAST_SIGNAL
};

struct _GthLocationPrivate
{
	char         *uri;
	int           current_idx;
	gboolean      catalog_uri;
	GtkWidget    *combo;
	GtkListStore *model;
};


static GtkHBoxClass *parent_class = NULL;
static guint gth_location_signals[LAST_SIGNAL] = { 0 };


static void
gth_location_destroy (GtkObject *object)
{
	GthLocation *loc;

	loc = GTH_LOCATION (object);

	if (loc->priv != NULL) {
		g_free (loc->priv->uri);
		loc->priv->uri = NULL;
		g_free (loc->priv);
		loc->priv = NULL;
	}

	GTK_OBJECT_CLASS (parent_class)->destroy (object);
}


static void
gth_location_class_init (GthLocationClass *class)
{
	GtkObjectClass *object_class;

	parent_class = g_type_class_peek_parent (class);
	object_class = (GtkObjectClass*) class;

	object_class->destroy = gth_location_destroy;

	gth_location_signals[CHANGED] =
                g_signal_new ("changed",
                              G_TYPE_FROM_CLASS (class),
                              G_SIGNAL_RUN_LAST,
                              G_STRUCT_OFFSET (GthLocationClass, changed),
                              NULL, NULL,
                              g_cclosure_marshal_VOID__STRING,
                              G_TYPE_NONE,
                              1,
			      G_TYPE_STRING);

	gth_location_signals[OPEN_LOCATION] =
                g_signal_new ("open_location",
                              G_TYPE_FROM_CLASS (class),
                              G_SIGNAL_RUN_LAST,
                              G_STRUCT_OFFSET (GthLocationClass, open_location),
                              NULL, NULL,
                              g_cclosure_marshal_VOID__VOID,
                              G_TYPE_NONE,
                              0);
}


static void
gth_location_init (GthLocation *loc)
{
	loc->priv = g_new0 (GthLocationPrivate, 1);
	loc->priv->current_idx = 0;
	loc->priv->catalog_uri = FALSE;
}


static void combo_changed_cb (GtkComboBox *widget,
			      gpointer     user_data);


static void
reset_active_index (GthLocation *loc) 
{
	g_signal_handlers_block_by_func (loc->priv->combo, combo_changed_cb, loc);
	gtk_combo_box_set_active (GTK_COMBO_BOX (loc->priv->combo), loc->priv->current_idx);
	g_signal_handlers_unblock_by_func (loc->priv->combo, combo_changed_cb, loc);
}


static void
combo_changed_cb (GtkComboBox *widget,
		  gpointer     user_data)
{
	GthLocation *loc = user_data;
	GtkTreeIter  iter;
	char        *path = NULL;
	int          item_type = ITEM_TYPE_NONE;

	if (!gtk_combo_box_get_active_iter (GTK_COMBO_BOX (loc->priv->combo), &iter))
		return;
	
	gtk_tree_model_get (GTK_TREE_MODEL (loc->priv->model),
			    &iter,
			    PATH_COLUMN, &path,
			    TYPE_COLUMN, &item_type,
			    -1);

	if (item_type == ITEM_TYPE_OPEN_LOCATION) {
		reset_active_index (loc);
		g_signal_emit (G_OBJECT (loc), gth_location_signals[OPEN_LOCATION], 0);
		return;
	}

	loc->priv->current_idx = gtk_combo_box_get_active (GTK_COMBO_BOX (loc->priv->combo));

	if (path == NULL) 
		return;
	g_signal_emit (G_OBJECT (loc), gth_location_signals[CHANGED], 0, path);
	g_free (path);
}


static gboolean
row_separator_func (GtkTreeModel *model,
		    GtkTreeIter  *iter,
		    gpointer      data)
{
	int item_type = ITEM_TYPE_NONE;
	gtk_tree_model_get (model,
			    iter,
			    TYPE_COLUMN, &item_type,
			    -1);
	return (item_type == ITEM_TYPE_SEPARATOR);
}


static void
gth_location_construct (GthLocation *loc)
{
	GtkCellRenderer *renderer;
	GValue           value = { 0, };
	GtkTreeIter      iter;

	loc->priv->model = gtk_list_store_new (N_COLUMNS, 
					       GDK_TYPE_PIXBUF, 
					       G_TYPE_STRING, 
					       G_TYPE_STRING,
					       G_TYPE_INT);
	loc->priv->combo = gtk_combo_box_new_with_model (GTK_TREE_MODEL (loc->priv->model));
	g_object_unref (loc->priv->model);
	g_signal_connect (loc->priv->combo, "changed",
			  G_CALLBACK (combo_changed_cb),
			  loc);
	gtk_combo_box_set_row_separator_func (GTK_COMBO_BOX (loc->priv->combo),
					      row_separator_func,
					      loc,
					      NULL);

	/* icon */

	renderer = gtk_cell_renderer_pixbuf_new ();
	gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (loc->priv->combo),
				    renderer,
				    FALSE);
	gtk_cell_layout_set_attributes  (GTK_CELL_LAYOUT (loc->priv->combo),
					 renderer,
					 "pixbuf", ICON_COLUMN,
					 NULL);

	/* path */

	renderer = gtk_cell_renderer_text_new ();
	gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (loc->priv->combo),
				    renderer,
				    TRUE);


	g_value_init (&value, PANGO_TYPE_ELLIPSIZE_MODE);
	g_value_set_enum (&value, PANGO_ELLIPSIZE_END);
	g_object_set_property (G_OBJECT (renderer), "ellipsize", &value);
	g_value_unset (&value);

	gtk_cell_layout_set_attributes  (GTK_CELL_LAYOUT (loc->priv->combo),
					 renderer,
					 "text", NAME_COLUMN,
					 NULL);
	/**/

	gtk_container_add (GTK_CONTAINER (loc), loc->priv->combo);


	/* Add standard items. */

	/* separator #1 */
	
	gtk_list_store_append (loc->priv->model, &iter);
	gtk_list_store_set (loc->priv->model, &iter,
			    TYPE_COLUMN, ITEM_TYPE_SEPARATOR,
			    -1);

	/* separator #2 */
	
	gtk_list_store_append (loc->priv->model, &iter);
	gtk_list_store_set (loc->priv->model, &iter,
			    TYPE_COLUMN, ITEM_TYPE_SEPARATOR,
			    -1);

	/* open location command */
	
	gtk_list_store_append (loc->priv->model, &iter);
	gtk_list_store_set (loc->priv->model, &iter,
			    TYPE_COLUMN, ITEM_TYPE_OPEN_LOCATION,
			    NAME_COLUMN, _("_Location..."),
			    -1);
}


GType
gth_location_get_type ()
{
        static GType type = 0;

        if (! type) {
                GTypeInfo type_info = {
			sizeof (GthLocationClass),
			NULL,
			NULL,
			(GClassInitFunc) gth_location_class_init,
			NULL,
			NULL,
			sizeof (GthLocation),
			0,
			(GInstanceInitFunc) gth_location_init
		};

		type = g_type_register_static (GTK_TYPE_HBOX,
					       "GthLocation",
					       &type_info,
					       0);
	}

        return type;
}


GtkWidget*     
gth_location_new (void)
{
	GtkWidget *widget;

	widget = GTK_WIDGET (g_object_new (GTH_TYPE_LOCATION, NULL));
	gth_location_construct (GTH_LOCATION (widget));

	return widget;
}


static gboolean
get_item_from_uri (GthLocation *loc,
		   const char  *uri,
		   GtkTreeIter *iter,
		   int         *idx)
{
	gboolean found = FALSE;

	if (idx != NULL)
		*idx = 0;

	if (!gtk_tree_model_get_iter_first (GTK_TREE_MODEL (loc->priv->model), iter))
		return FALSE;
	
	do {
		char *path = NULL;
		int   item_type = ITEM_TYPE_NONE;

		gtk_tree_model_get (GTK_TREE_MODEL (loc->priv->model),
				    iter,
				    PATH_COLUMN, &path,
				    TYPE_COLUMN, &item_type,
				    -1);
		if ((item_type == ITEM_TYPE_PARENT) && (path != NULL)) {
			if (strcmp (path, uri) == 0) {
				g_free (path);
				found = TRUE;
				break;
			}
		}
		g_free (path);
		if (idx != NULL)
			*idx = *idx + 1;
	} while (gtk_tree_model_iter_next (GTK_TREE_MODEL (loc->priv->model), iter));
	
	return found;
}


static void
clear_items (GthLocation *loc,
	     int          clear_type)
{
	GtkTreeIter iter;
	gboolean    valid;

	valid = gtk_tree_model_get_iter_first (GTK_TREE_MODEL (loc->priv->model), &iter);
	while (valid) {
		int   item_type = ITEM_TYPE_NONE;
		char *uri = NULL;

		gtk_tree_model_get (GTK_TREE_MODEL (loc->priv->model),
				    &iter,
				    TYPE_COLUMN, &item_type,
				    PATH_COLUMN, &uri,
				    -1);

		if (item_type == clear_type) 
			valid = gtk_list_store_remove (loc->priv->model, &iter);
		else
			valid = gtk_tree_model_iter_next (GTK_TREE_MODEL (loc->priv->model), &iter);

		g_free (uri);
	}
}


static void
update_uri (GthLocation *loc, 
	    gboolean     reset_history)
{
	GtkTreeIter  iter;
	int          idx, pos = 0;
	char        *uri, *uri_name, *base_uri;
	GdkPixbuf   *pixbuf;

	/* search the uri in the current list and activate the entry if present */

	if (! reset_history && get_item_from_uri (loc, loc->priv->uri, &iter, &idx)) {
		g_signal_handlers_block_by_func (loc->priv->combo, combo_changed_cb, loc);
		gtk_combo_box_set_active_iter (GTK_COMBO_BOX (loc->priv->combo), &iter);
		loc->priv->current_idx = idx;
		g_signal_handlers_unblock_by_func (loc->priv->combo, combo_changed_cb, loc);
		return;
	}

	/* ...else create a new list */

	clear_items (loc, ITEM_TYPE_PARENT);

	if (loc->priv->catalog_uri)
		base_uri = g_strconcat (g_get_home_dir(),
					"/",
					RC_CATALOG_DIR,
					NULL);
	else
		base_uri = g_strdup ("/");

	uri = g_strdup (loc->priv->uri);
	while (uri != NULL) {
		char *parent;

		if (loc->priv->catalog_uri) 
			pixbuf = gdk_pixbuf_new_from_inline (-1, library_19_rgba, FALSE, NULL);
		else 
			pixbuf = get_icon_for_uri (GTK_WIDGET (loc), uri);

		if (strcmp (uri, base_uri) == 0) {
			if (loc->priv->catalog_uri)
				uri_name = g_strdup (_("Catalogs"));
			else
				uri_name = g_strdup ("");
		} else
			uri_name = g_filename_display_basename (uri);

		gtk_list_store_insert (loc->priv->model, &iter, pos++);
		gtk_list_store_set (loc->priv->model, &iter,
				    TYPE_COLUMN, ITEM_TYPE_PARENT,
				    ICON_COLUMN, pixbuf,
				    NAME_COLUMN, uri_name,
				    PATH_COLUMN, uri,
				    -1);
		g_free (uri_name);
		g_object_unref (pixbuf);

		/**/

		if (strcmp (uri, base_uri) == 0)
			parent = NULL;
		else
			parent = remove_level_from_path (uri);
		g_free (uri);
		uri = parent;
	}

	g_free (base_uri);

	/**/

	loc->priv->current_idx = 0;
	reset_active_index (loc);
}


static void
gth_location_set_uri (GthLocation *loc,
		      const char  *uri,
		      gboolean     reset_history)
{
	if (uri != loc->priv->uri) {
		g_free (loc->priv->uri);
		if (uri != NULL) 
			loc->priv->uri = g_strdup (uri);
	}
	
	update_uri (loc, reset_history);
}


void
gth_location_set_folder_uri (GthLocation *loc,
			     const char  *uri,
			     gboolean     reset_history)
{
	loc->priv->catalog_uri = FALSE;
	gth_location_set_uri (loc, uri, reset_history);
	
}


void
gth_location_set_catalog_uri (GthLocation *loc,
			      const char  *uri,
			      gboolean     reset_history)
{
	loc->priv->catalog_uri = TRUE;
	gth_location_set_uri (loc, uri, reset_history);
}


G_CONST_RETURN char*
gth_location_get_uri (GthLocation *loc)
{
	return loc->priv->uri;
}


void
gth_location_set_bookmarks (GthLocation *loc,
			    GList       *bookmark_list,
			    int          max_size)
{
	GList       *scan;
	int          n = 0, pos;
	GtkTreeIter  iter;

	clear_items (loc, ITEM_TYPE_BOOKMARK);

	pos = gtk_tree_model_iter_n_children (GTK_TREE_MODEL (loc->priv->model), NULL) - 2;

	for (scan = bookmark_list; (scan != NULL) && (n < max_size); scan = scan->next) {
		const char *uri = scan->data;
		char       *uri_name;
		GdkPixbuf  *pixbuf;

		uri_name = bookmarks_utils__get_menu_item_name (uri);
		pixbuf = get_icon_for_uri (GTK_WIDGET (loc), uri);

		gtk_list_store_insert (loc->priv->model, &iter, pos++);
		gtk_list_store_set (loc->priv->model, &iter,
				    TYPE_COLUMN, ITEM_TYPE_BOOKMARK,
				    ICON_COLUMN, pixbuf,
				    NAME_COLUMN, uri_name,
				    PATH_COLUMN, uri,
				    -1);

		g_free (uri_name);
		g_object_unref (pixbuf);
		n++;
	}
}
