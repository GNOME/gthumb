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
#include <string.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include "gth-file-data.h"
#include "glib-utils.h"
#include "gtk-utils.h"
#include "gth-file-source.h"
#include "gth-folder-tree.h"
#include "gth-icon-cache.h"
#include "gth-main.h"
#include "gth-marshal.h"


#define EMPTY_URI   "..."
#define LOADING_URI "."
#define PARENT_URI  ".."


typedef enum {
	ENTRY_TYPE_FILE,
	ENTRY_TYPE_PARENT,
	ENTRY_TYPE_LOADING,
	ENTRY_TYPE_EMPTY
} EntryType;


enum {
	COLUMN_STYLE,
	COLUMN_WEIGHT,
	COLUMN_ICON,
	COLUMN_TYPE,
	COLUMN_FILE_DATA,
	COLUMN_SORT_KEY,
	COLUMN_SORT_ORDER,
	COLUMN_NAME,
	COLUMN_NO_CHILD,
	COLUMN_LOADED,
	NUM_COLUMNS
};

enum {
	FOLDER_POPUP,
	LIST_CHILDREN,
	LOAD,
	OPEN,
	OPEN_PARENT,
	RENAME,
	LAST_SIGNAL
};


struct _GthFolderTreePrivate
{
	GFile           *root;
	GtkTreeStore    *tree_store;
	GthIconCache    *icon_cache;
	GtkCellRenderer *text_renderer;
	GtkTreePath     *hover_path;
	GtkTreePath     *click_path;
};


static gpointer parent_class = NULL;
static guint    gth_folder_tree_signals[LAST_SIGNAL] = { 0 };


static void
gth_folder_tree_finalize (GObject *object)
{
	GthFolderTree *folder_tree;

	folder_tree = GTH_FOLDER_TREE (object);

	if (folder_tree->priv != NULL) {
		if (folder_tree->priv->root != NULL)
			g_object_unref (folder_tree->priv->root);
		gth_icon_cache_free (folder_tree->priv->icon_cache);
		g_free (folder_tree->priv);
		folder_tree->priv = NULL;
	}

	G_OBJECT_CLASS (parent_class)->finalize (object);
}


static void
gth_folder_tree_class_init (GthFolderTreeClass *class)
{
	GObjectClass *object_class;

	parent_class = g_type_class_peek_parent (class);
	object_class = (GObjectClass*) class;

	object_class->finalize = gth_folder_tree_finalize;

	gth_folder_tree_signals[FOLDER_POPUP] =
		g_signal_new ("folder_popup",
			      G_TYPE_FROM_CLASS (class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (GthFolderTreeClass, folder_popup),
			      NULL, NULL,
			      gth_marshal_VOID__OBJECT_UINT,
			      G_TYPE_NONE,
			      2,
			      G_TYPE_OBJECT,
			      G_TYPE_UINT);
	gth_folder_tree_signals[LIST_CHILDREN] =
		g_signal_new ("list_children",
			      G_TYPE_FROM_CLASS (class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (GthFolderTreeClass, list_children),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__OBJECT,
			      G_TYPE_NONE,
			      1,
			      G_TYPE_OBJECT);
	gth_folder_tree_signals[LOAD] =
		g_signal_new ("load",
			      G_TYPE_FROM_CLASS (class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (GthFolderTreeClass, load),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__OBJECT,
			      G_TYPE_NONE,
			      1,
			      G_TYPE_OBJECT);
	gth_folder_tree_signals[OPEN] =
		g_signal_new ("open",
			      G_TYPE_FROM_CLASS (class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (GthFolderTreeClass, open),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__OBJECT,
			      G_TYPE_NONE,
			      1,
			      G_TYPE_OBJECT);
	gth_folder_tree_signals[OPEN_PARENT] =
		g_signal_new ("open_parent",
			      G_TYPE_FROM_CLASS (class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (GthFolderTreeClass, open_parent),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__VOID,
			      G_TYPE_NONE,
			      0);
	gth_folder_tree_signals[RENAME] =
		g_signal_new ("rename",
			      G_TYPE_FROM_CLASS (class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (GthFolderTreeClass, rename),
			      NULL, NULL,
			      gth_marshal_VOID__OBJECT_STRING,
			      G_TYPE_NONE,
			      2,
			      G_TYPE_OBJECT,
			      G_TYPE_STRING);
}


static void
text_renderer_edited_cb (GtkCellRendererText *renderer,
			 char                *path,
			 char                *new_text,
			 gpointer             user_data)
{
	GthFolderTree *folder_tree = user_data;
	GtkTreePath   *tree_path;
	GtkTreeIter    iter;
	EntryType      entry_type;
	GthFileData   *file_data;
	char          *name;

	tree_path = gtk_tree_path_new_from_string (path);
	if (! gtk_tree_model_get_iter (GTK_TREE_MODEL (folder_tree->priv->tree_store),
				       &iter,
				       tree_path))
	{
		gtk_tree_path_free (tree_path);
		return;
	}
	gtk_tree_path_free (tree_path);

	gtk_tree_model_get (GTK_TREE_MODEL (folder_tree->priv->tree_store),
			    &iter,
			    COLUMN_TYPE, &entry_type,
			    COLUMN_FILE_DATA, &file_data,
			    COLUMN_NAME, &name,
			    -1);

	if ((entry_type == ENTRY_TYPE_FILE) && (g_utf8_collate (name, new_text) != 0))
		g_signal_emit (folder_tree, gth_folder_tree_signals[RENAME], 0, file_data->file, new_text);

	_g_object_unref (file_data);
	g_free (name);
}


static void
add_columns (GthFolderTree *folder_tree,
	     GtkTreeView   *treeview)
{
	GtkCellRenderer   *renderer;
	GtkTreeViewColumn *column;

	column = gtk_tree_view_column_new ();

	renderer = gtk_cell_renderer_pixbuf_new ();
	gtk_tree_view_column_pack_start (column, renderer, FALSE);
	gtk_tree_view_column_set_attributes (column, renderer,
					     "pixbuf", COLUMN_ICON,
					     NULL);

	folder_tree->priv->text_renderer = renderer = gtk_cell_renderer_text_new ();
	g_object_set (G_OBJECT (renderer),
		      "ellipsize", PANGO_ELLIPSIZE_END,
		      "editable", TRUE,
		      NULL);
	g_signal_connect (folder_tree->priv->text_renderer,
			  "edited",
			  G_CALLBACK (text_renderer_edited_cb),
			  folder_tree);

	gtk_tree_view_column_pack_start (column, renderer, TRUE);
	gtk_tree_view_column_set_attributes (column, renderer,
					     "text", COLUMN_NAME,
					     "style", COLUMN_STYLE,
					     "weight", COLUMN_WEIGHT,
					     NULL);
	gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
	gtk_tree_view_column_set_sort_column_id (column, COLUMN_NAME);

	gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);
}


static void
open_uri (GthFolderTree *folder_tree,
	  GthFileData   *file_data,
	  EntryType      entry_type)
{
	if (entry_type == ENTRY_TYPE_PARENT)
		g_signal_emit (folder_tree, gth_folder_tree_signals[OPEN_PARENT], 0);
	else if (entry_type == ENTRY_TYPE_FILE)
		g_signal_emit (folder_tree, gth_folder_tree_signals[OPEN], 0, file_data->file);
}


static gboolean
row_activated_cb (GtkTreeView       *tree_view,
		  GtkTreePath       *path,
		  GtkTreeViewColumn *column,
		  gpointer           user_data)
{
	GthFolderTree *folder_tree = user_data;
	GtkTreeIter    iter;
	EntryType      entry_type;
	GthFileData   *file_data;

	if (! gtk_tree_model_get_iter (GTK_TREE_MODEL (folder_tree->priv->tree_store),
				       &iter,
				       path))
	{
		return FALSE;
	}

	gtk_tree_model_get (GTK_TREE_MODEL (folder_tree->priv->tree_store),
			    &iter,
			    COLUMN_TYPE, &entry_type,
			    COLUMN_FILE_DATA, &file_data,
			    -1);
	open_uri (folder_tree, file_data, entry_type);

	_g_object_unref (file_data);

	return TRUE;
}


static gboolean
row_expanded_cb (GtkTreeView  *tree_view,
		 GtkTreeIter  *expanded_iter,
		 GtkTreePath  *expanded_path,
		 gpointer      user_data)
{
	GthFolderTree *folder_tree = user_data;
	EntryType      entry_type;
	GthFileData   *file_data;
	gboolean       loaded;

	if ((folder_tree->priv->click_path == NULL)
	    || gtk_tree_path_compare (folder_tree->priv->click_path, expanded_path) != 0)
	{
		return FALSE;
	}

	gtk_tree_model_get (GTK_TREE_MODEL (folder_tree->priv->tree_store),
			    expanded_iter,
			    COLUMN_TYPE, &entry_type,
			    COLUMN_FILE_DATA, &file_data,
			    COLUMN_LOADED, &loaded,
			    -1);

	if ((entry_type == ENTRY_TYPE_FILE) && ! loaded)
		g_signal_emit (folder_tree, gth_folder_tree_signals[LIST_CHILDREN], 0, file_data->file);

	gtk_tree_path_free (folder_tree->priv->click_path);
	folder_tree->priv->click_path = NULL;

	_g_object_unref (file_data);

	return FALSE;
}


static int
button_press_cb (GtkWidget      *widget,
		 GdkEventButton *event,
		 gpointer        data)
{
	GthFolderTree     *folder_tree = data;
	GtkTreeStore      *tree_store = folder_tree->priv->tree_store;
	GtkTreePath       *path;
	GtkTreeIter        iter;
	gboolean           retval;
	GtkTreeViewColumn *column;
	int                cell_x;
	int                cell_y;

	retval = FALSE;

	gtk_widget_grab_focus (widget);

	if (folder_tree->priv->click_path != NULL) {
		gtk_tree_path_free (folder_tree->priv->click_path);
		folder_tree->priv->click_path = NULL;
	}

	if ((event->state & GDK_SHIFT_MASK) || (event->state & GDK_CONTROL_MASK))
		return retval;

	if ((event->button != 1) && (event->button != 3))
		return retval;

	if (! gtk_tree_view_get_path_at_pos (GTK_TREE_VIEW (folder_tree),
					     event->x, event->y,
					     &path,
					     &column,
					     &cell_x,
					     &cell_y))
	{
		if (event->button == 3) {
			GtkTreeSelection *selection;

			/* Update the selection. */

			selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (folder_tree));
			gtk_tree_selection_unselect_all (selection);

			g_signal_emit (folder_tree,
				       gth_folder_tree_signals[FOLDER_POPUP],
				       0,
				       NULL,
				       event->time);
			retval = TRUE;
		}

		return retval;
	}

	if (! gtk_tree_model_get_iter (GTK_TREE_MODEL (tree_store),
				       &iter,
				       path))
	{
		gtk_tree_path_free (path);
		return retval;
	}

 	if (event->button == 3) {
 		EntryType    entry_type;
		GthFileData *file_data;

		gtk_tree_model_get (GTK_TREE_MODEL (tree_store),
				    &iter,
				    COLUMN_TYPE, &entry_type,
				    COLUMN_FILE_DATA, &file_data,
				    -1);

		if (entry_type == ENTRY_TYPE_FILE) {
			GtkTreeSelection *selection;

			/* Update the selection. */

			selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (folder_tree));
			if (! gtk_tree_selection_iter_is_selected (selection, &iter)) {
				gtk_tree_selection_unselect_all (selection);
				gtk_tree_selection_select_iter (selection, &iter);
			}

			/* Show the folder popup menu. */

			g_signal_emit (folder_tree,
				       gth_folder_tree_signals[FOLDER_POPUP],
				       0,
				       file_data,
				       event->time);
			retval = TRUE;
		}

		_g_object_unref (file_data);
 	}
	else if ((event->button == 1) && (event->type == GDK_BUTTON_PRESS)) {
		GtkTreeSelection *selection;
		int               start_pos;
		int               width;
		GValue            value = { 0, };
		int               expander_size;
		int               horizontal_separator;

		if (! gtk_tree_view_column_cell_get_position (column,
							      folder_tree->priv->text_renderer,
							      &start_pos,
							      &width))
		{
			start_pos = 0;
			width = 0;
		}

		g_value_init (&value, G_TYPE_INT);
		gtk_style_get_style_property (gtk_widget_get_style (GTK_WIDGET (folder_tree)),
					      GTK_TYPE_TREE_VIEW,
					      "expander-size",
					      &value);
		expander_size = g_value_get_int (&value);

		gtk_style_get_style_property (gtk_widget_get_style (GTK_WIDGET (folder_tree)),
					      GTK_TYPE_TREE_VIEW,
					      "horizontal-separator",
					      &value);
		horizontal_separator = g_value_get_int (&value);

		start_pos += (gtk_tree_path_get_depth (path) - 1) * (expander_size + (horizontal_separator * 2));

		selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (folder_tree));
		if ((cell_x > start_pos) && gtk_tree_selection_iter_is_selected (selection, &iter))
			 retval = TRUE;

		folder_tree->priv->click_path = gtk_tree_path_copy (path);
	}
	else if ((event->button == 1) && (event->type == GDK_2BUTTON_PRESS)) {
		gtk_tree_view_row_activated (GTK_TREE_VIEW (folder_tree), path, NULL);
		retval = TRUE;
	}

	gtk_tree_path_free (path);

	return retval;
}


static void
load_uri (GthFolderTree *folder_tree,
	  EntryType      entry_type,
	  GthFileData   *file_data)
{
	if (entry_type == ENTRY_TYPE_FILE)
		g_signal_emit (folder_tree, gth_folder_tree_signals[LOAD], 0, file_data->file);
}


static int
button_release_cb (GtkWidget      *widget,
		   GdkEventButton *event,
		   gpointer        data)
{
	GthFolderTree    *folder_tree = data;
	GtkTreeStore     *tree_store = folder_tree->priv->tree_store;
	GtkTreePath      *path;
	GtkTreeIter       iter;
	GtkTreeSelection *selection;

	if ((event->state & GDK_SHIFT_MASK) || (event->state & GDK_CONTROL_MASK))
		return FALSE;

return FALSE;

	if (event->button != 1)
		return FALSE;

	if (! gtk_tree_view_get_path_at_pos (GTK_TREE_VIEW (folder_tree),
					     event->x, event->y,
					     &path, NULL, NULL, NULL))
	{
		return FALSE;
	}

	if (! gtk_tree_model_get_iter (GTK_TREE_MODEL (tree_store),
				       &iter,
				       path))
	{
		gtk_tree_path_free (path);
		return FALSE;
	}

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (folder_tree));
	if (gtk_tree_selection_iter_is_selected (selection, &iter)) {
		EntryType    entry_type;
		GthFileData *file_data;

		gtk_tree_model_get (GTK_TREE_MODEL (tree_store),
				    &iter,
				    COLUMN_TYPE, &entry_type,
				    COLUMN_FILE_DATA, &file_data,
				    -1);

		load_uri (folder_tree, entry_type, file_data);

		_g_object_unref (file_data);
	}

	gtk_tree_path_free (path);

	return FALSE;
}


static gboolean
selection_changed_cb (GtkTreeSelection *selection,
		      gpointer          user_data)
{
	GthFolderTree *folder_tree = user_data;
	GtkTreeIter    iter;
	GtkTreePath   *selected_path;
	EntryType      entry_type;
	GthFileData   *file_data;

	if (! gtk_tree_selection_get_selected (selection, NULL, &iter))
		return FALSE;

	selected_path = gtk_tree_model_get_path (GTK_TREE_MODEL (folder_tree->priv->tree_store), &iter);

	if ((folder_tree->priv->click_path == NULL)
	    || gtk_tree_path_compare (folder_tree->priv->click_path, selected_path) != 0)
	{
		gtk_tree_path_free (selected_path);
		return FALSE;
	}

	/* FIXME
	gtk_tree_path_free (folder_tree->priv->click_path);
	folder_tree->priv->click_path = NULL;
	*/

	/*if (! gtk_tree_view_row_expanded (GTK_TREE_VIEW (folder_tree), selected_path))
		gtk_tree_view_expand_row (GTK_TREE_VIEW (folder_tree), selected_path, FALSE);*/

	gtk_tree_model_get (GTK_TREE_MODEL (folder_tree->priv->tree_store),
			    &iter,
			    COLUMN_TYPE, &entry_type,
			    COLUMN_FILE_DATA, &file_data,
			    -1);

	load_uri (folder_tree, entry_type, file_data);

	_g_object_unref (file_data);
	gtk_tree_path_free (selected_path);

	return FALSE;
}


static gint
column_name_compare_func (GtkTreeModel *model,
			  GtkTreeIter  *a,
			  GtkTreeIter  *b,
			  gpointer      user_data)
{
	char       *key_a;
	char       *key_b;
	int         order_a;
	int         order_b;
	PangoStyle  style_a;
	PangoStyle  style_b;
	gboolean    result;

	gtk_tree_model_get (model, a,
			    COLUMN_SORT_KEY, &key_a,
			    COLUMN_SORT_ORDER, &order_a,
			    COLUMN_STYLE, &style_a,
			    -1);
	gtk_tree_model_get (model, b,
			    COLUMN_SORT_KEY, &key_b,
			    COLUMN_SORT_ORDER, &order_b,
			    COLUMN_STYLE, &style_b,
			    -1);

	if (order_a == order_b) {
		if (style_a == style_b)
			result = strcmp (key_a, key_b);
		else if (style_a == PANGO_STYLE_ITALIC)
			result = -1;
		else
			result = 1;
	}
	else if (order_a < order_b)
		result = -1;
	else
		result = 1;

	g_free (key_a);
	g_free (key_b);

	return result;
}


static gboolean
gth_folder_tree_get_iter (GthFolderTree *folder_tree,
			  GFile         *file,
			  GtkTreeIter   *file_iter,
			  GtkTreeIter   *root)
{
	GtkTreeModel *tree_model = GTK_TREE_MODEL (folder_tree->priv->tree_store);
	GtkTreeIter   iter;

	if (file == NULL)
		return FALSE;

	if (root != NULL) {
		GthFileData *root_file_data;
		EntryType    root_type;
		gboolean     found;

		gtk_tree_model_get (tree_model, root,
				    COLUMN_FILE_DATA, &root_file_data,
				    COLUMN_TYPE, &root_type,
				    -1);
		found = (root_type == ENTRY_TYPE_FILE) && (root_file_data != NULL) && g_file_equal (file, root_file_data->file);

		_g_object_unref (root_file_data);

		if (found) {
			*file_iter = *root;
			return TRUE;
		}
	}

	if (! gtk_tree_model_iter_children (tree_model, &iter, root))
		return FALSE;

	do {
		if (gth_folder_tree_get_iter (folder_tree, file, file_iter, &iter))
			return TRUE;
	}
	while (gtk_tree_model_iter_next (tree_model, &iter));

	return FALSE;
}


static gboolean
_gth_folder_tree_child_type_present (GthFolderTree *folder_tree,
				     GtkTreeIter   *parent,
				     EntryType      entry_type)
{
	GtkTreeIter iter;

	if (! gtk_tree_model_iter_children (GTK_TREE_MODEL (folder_tree->priv->tree_store), &iter, parent))
		return FALSE;

	do {
		EntryType file_entry_type;

		gtk_tree_model_get (GTK_TREE_MODEL (folder_tree->priv->tree_store), &iter,
				    COLUMN_TYPE, &file_entry_type,
				    -1);

		if (entry_type == file_entry_type)
			return TRUE;
	}
	while (gtk_tree_model_iter_next (GTK_TREE_MODEL (folder_tree->priv->tree_store), &iter));

	return FALSE;
}


static void
_gth_folder_tree_add_loading_item (GthFolderTree *folder_tree,
				   GtkTreeIter   *parent)
{
	char        *sort_key;
	GtkTreeIter  iter;

	if (_gth_folder_tree_child_type_present (folder_tree, parent, ENTRY_TYPE_LOADING))
		return;

	sort_key = g_utf8_collate_key_for_filename (LOADING_URI, -1);

	gtk_tree_store_append (folder_tree->priv->tree_store, &iter, parent);
	gtk_tree_store_set (folder_tree->priv->tree_store, &iter,
			    COLUMN_STYLE, PANGO_STYLE_ITALIC,
			    COLUMN_TYPE, ENTRY_TYPE_LOADING,
			    COLUMN_NAME, _("Loading..."),
			    COLUMN_SORT_KEY, sort_key,
			    COLUMN_SORT_ORDER, 0,
			    -1);

	g_free (sort_key);
}


static void
_gth_folder_tree_add_empty_item (GthFolderTree *folder_tree,
				 GtkTreeIter   *parent)
{
	char        *sort_key;
	GtkTreeIter  iter;

	if (_gth_folder_tree_child_type_present (folder_tree, parent, ENTRY_TYPE_EMPTY))
		return;

	sort_key = g_utf8_collate_key_for_filename (EMPTY_URI, -1);

	gtk_tree_store_append (folder_tree->priv->tree_store, &iter, parent);
	gtk_tree_store_set (folder_tree->priv->tree_store, &iter,
			    COLUMN_STYLE, PANGO_STYLE_ITALIC,
			    COLUMN_TYPE, ENTRY_TYPE_EMPTY,
			    COLUMN_NAME, _("(Empty)"),
			    COLUMN_SORT_KEY, sort_key,
			    COLUMN_SORT_ORDER, 0,
			    -1);

	g_free (sort_key);
}


static void
_gth_folder_tree_set_file_data (GthFolderTree *folder_tree,
				GtkTreeIter   *iter,
				GthFileData   *file_data)
{
	GIcon       *icon;
	GdkPixbuf   *pixbuf;
	const char  *name;
	char        *sort_key;

	icon = g_file_info_get_icon (file_data->info);
	pixbuf = gth_icon_cache_get_pixbuf (folder_tree->priv->icon_cache, icon);
	name = g_file_info_get_display_name (file_data->info);
	sort_key = g_utf8_collate_key_for_filename (name, -1);

	gtk_tree_store_set (folder_tree->priv->tree_store, iter,
			    COLUMN_STYLE, PANGO_STYLE_NORMAL,
			    COLUMN_ICON, pixbuf,
			    COLUMN_TYPE, ENTRY_TYPE_FILE,
			    COLUMN_FILE_DATA, file_data,
			    COLUMN_NAME, name,
			    COLUMN_SORT_KEY, sort_key,
			    COLUMN_SORT_ORDER, g_file_info_get_sort_order (file_data->info),
			    COLUMN_NO_CHILD, g_file_info_get_attribute_boolean (file_data->info, "gthumb::no-child"),
			    COLUMN_LOADED, FALSE,
			    -1);

	g_free (sort_key);
	_g_object_unref (pixbuf);
}


static gboolean
_gth_folder_tree_iter_has_no_child (GthFolderTree *folder_tree,
				    GtkTreeIter   *iter)
{
	gboolean no_child;

	if (iter == NULL)
		return FALSE;

	gtk_tree_model_get (GTK_TREE_MODEL (folder_tree->priv->tree_store), iter,
			    COLUMN_NO_CHILD, &no_child,
			    -1);

	return no_child;
}


static gboolean
_gth_folder_tree_add_file (GthFolderTree *folder_tree,
			   GtkTreeIter   *parent,
			   GthFileData   *fd)
{
	GtkTreeIter iter;

	if (g_file_info_get_file_type (fd->info) != G_FILE_TYPE_DIRECTORY)
		return FALSE;

	/* add the folder */

	gtk_tree_store_append (folder_tree->priv->tree_store, &iter, parent);
	_gth_folder_tree_set_file_data (folder_tree, &iter, fd);

	if (g_file_info_get_attribute_boolean (fd->info, "gthumb::entry-point"))
		gtk_tree_store_set (folder_tree->priv->tree_store, &iter,
				    COLUMN_WEIGHT, PANGO_WEIGHT_BOLD,
				    -1);

	if (! _gth_folder_tree_iter_has_no_child (folder_tree, &iter))
		_gth_folder_tree_add_loading_item (folder_tree, &iter);

	return TRUE;
}


static void
gth_folder_tree_construct (GthFolderTree *folder_tree)
{
	GtkTreeSelection *selection;

	folder_tree->priv->icon_cache = gth_icon_cache_new (gtk_icon_theme_get_for_screen (gtk_widget_get_screen (GTK_WIDGET (folder_tree))),
							    _gtk_icon_get_pixel_size (GTK_WIDGET (folder_tree), GTK_ICON_SIZE_MENU));

	folder_tree->priv->tree_store = gtk_tree_store_new (NUM_COLUMNS,
							    PANGO_TYPE_STYLE,
							    PANGO_TYPE_WEIGHT,
							    GDK_TYPE_PIXBUF,
							    G_TYPE_INT,
							    G_TYPE_OBJECT,
							    G_TYPE_STRING,
							    G_TYPE_INT,
							    G_TYPE_STRING,
							    G_TYPE_BOOLEAN,
							    G_TYPE_BOOLEAN);
	gtk_tree_view_set_model (GTK_TREE_VIEW (folder_tree), GTK_TREE_MODEL (folder_tree->priv->tree_store));
	gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (folder_tree), FALSE);

	add_columns (folder_tree, GTK_TREE_VIEW (folder_tree));

	gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (folder_tree), FALSE);
	gtk_tree_view_set_enable_search (GTK_TREE_VIEW (folder_tree), TRUE);
	gtk_tree_view_set_search_column (GTK_TREE_VIEW (folder_tree), COLUMN_NAME);
	gtk_tree_sortable_set_sort_func (GTK_TREE_SORTABLE (folder_tree->priv->tree_store), COLUMN_NAME, column_name_compare_func, folder_tree, NULL);
	gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (folder_tree->priv->tree_store), COLUMN_NAME, GTK_SORT_ASCENDING);

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (folder_tree));
	gtk_tree_selection_set_mode (selection, GTK_SELECTION_SINGLE);
	g_signal_connect (selection,
			  "changed",
			  G_CALLBACK (selection_changed_cb),
			  folder_tree);

	/**/

	g_signal_connect (G_OBJECT (folder_tree),
			  "button_press_event",
			  G_CALLBACK (button_press_cb),
			  folder_tree);
	g_signal_connect (G_OBJECT (folder_tree),
			  "button_release_event",
			  G_CALLBACK (button_release_cb),
			  folder_tree);
	g_signal_connect (G_OBJECT (folder_tree),
			  "row-activated",
			  G_CALLBACK (row_activated_cb),
			  folder_tree);
	g_signal_connect (G_OBJECT (folder_tree),
			  "row-expanded",
			  G_CALLBACK (row_expanded_cb),
			  folder_tree);
}


static void
gth_folder_tree_init (GthFolderTree *folder_tree)
{
	folder_tree->priv = g_new0 (GthFolderTreePrivate, 1);
	gth_folder_tree_construct (folder_tree);
}


GType
gth_folder_tree_get_type (void)
{
	static GType type = 0;

	if (! type) {
		GTypeInfo type_info = {
			sizeof (GthFolderTreeClass),
			NULL,
			NULL,
			(GClassInitFunc) gth_folder_tree_class_init,
			NULL,
			NULL,
			sizeof (GthFolderTree),
			0,
			(GInstanceInitFunc) gth_folder_tree_init
		};

		type = g_type_register_static (GTK_TYPE_TREE_VIEW,
					       "GthFolderTree",
					       &type_info,
					       0);
	}

	return type;
}


GtkWidget *
gth_folder_tree_new (const char *uri)
{
	GthFolderTree *folder_tree;

	folder_tree = g_object_new (GTH_TYPE_FOLDER_TREE, NULL);
	if (uri != NULL)
		folder_tree->priv->root = g_file_new_for_uri (uri);

	return (GtkWidget *) folder_tree;
}


void
gth_folder_tree_set_list (GthFolderTree *folder_tree,
			  GFile         *root,
			  GList         *files,
			  gboolean       open_parent)
{
	gtk_tree_store_clear (folder_tree->priv->tree_store);

	if (folder_tree->priv->root != NULL) {
		g_object_unref (folder_tree->priv->root);
		folder_tree->priv->root = NULL;
	}
	if (root != NULL)
		folder_tree->priv->root = g_file_dup (root);

	/* add the parent folder item */

	if (open_parent) {
		char        *sort_key;
		GdkPixbuf   *pixbuf;
		GtkTreeIter  iter;

		sort_key = g_utf8_collate_key_for_filename (PARENT_URI, -1);
		pixbuf = gtk_widget_render_icon (GTK_WIDGET (folder_tree), GTK_STOCK_GO_UP, GTK_ICON_SIZE_MENU, "folder-list");

		gtk_tree_store_append (folder_tree->priv->tree_store, &iter, NULL);
		gtk_tree_store_set (folder_tree->priv->tree_store, &iter,
				    COLUMN_STYLE, PANGO_STYLE_ITALIC,
				    COLUMN_ICON, pixbuf,
				    COLUMN_TYPE, ENTRY_TYPE_PARENT,
				    COLUMN_NAME, _("(Open Parent)"),
				    COLUMN_SORT_KEY, sort_key,
				    COLUMN_SORT_ORDER, 0,
				    -1);

		g_object_unref (pixbuf);
		g_free (sort_key);
	}

	/* add the folder list */

	gth_folder_tree_set_children (folder_tree, root, files);
}


/* After changing the children list, the node expander is not hilighted
 * anymore, this prevents the user to close the expander without moving the
 * mouse pointer.  The problem can be fixed emitting a fake motion notify
 * event, this way the expander gets hilighted again and a click on the
 * expander will correctly collapse the node. */
static void
emit_fake_motion_notify_event (GthFolderTree *folder_tree)
{
	GdkEventMotion event;
	int            x, y;

	gtk_widget_get_pointer (GTK_WIDGET (folder_tree), &x, &y);

	event.type = GDK_MOTION_NOTIFY;
	event.window = gtk_tree_view_get_bin_window (GTK_TREE_VIEW (folder_tree));
	event.send_event = TRUE;
	event.time = GDK_CURRENT_TIME;
	event.x = x;
	event.y = y;
	event.axes = NULL;
	event.state = 0;
	event.is_hint = FALSE;
	event.device = NULL;

	GTK_WIDGET_GET_CLASS (folder_tree)->motion_notify_event ((GtkWidget*) folder_tree, &event);
}


static GList *
_gth_folder_tree_get_children (GthFolderTree *folder_tree,
			       GtkTreeIter   *parent)
{
	GtkTreeModel *tree_model = GTK_TREE_MODEL (folder_tree->priv->tree_store);
	GtkTreeIter   iter;
	GList        *list;

	if (! gtk_tree_model_iter_children (tree_model, &iter, parent))
		return NULL;

	list = NULL;
	do {
		GthFileData *file_data;
		EntryType    file_type;

		gtk_tree_model_get (tree_model, &iter,
				    COLUMN_FILE_DATA, &file_data,
				    COLUMN_TYPE, &file_type,
				    -1);
		if ((file_type == ENTRY_TYPE_FILE) && (file_data != NULL))
			list = g_list_prepend (list, file_data);
	}
	while (gtk_tree_model_iter_next (tree_model, &iter));

	return g_list_reverse (list);
}


static void
_gth_folder_tree_remove_child_type (GthFolderTree *folder_tree,
				    GtkTreeIter   *parent,
				    EntryType      entry_type)
{
	GtkTreeIter iter;

	if (! gtk_tree_model_iter_children (GTK_TREE_MODEL (folder_tree->priv->tree_store), &iter, parent))
		return;

	do {
		EntryType file_entry_type;

		gtk_tree_model_get (GTK_TREE_MODEL (folder_tree->priv->tree_store), &iter,
				    COLUMN_TYPE, &file_entry_type,
				    -1);

		if (entry_type == file_entry_type) {
			gtk_tree_store_remove (folder_tree->priv->tree_store, &iter);
			break;
		}
	}
	while (gtk_tree_model_iter_next (GTK_TREE_MODEL (folder_tree->priv->tree_store), &iter));
}


void
gth_folder_tree_set_children (GthFolderTree *folder_tree,
			      GFile         *parent,
			      GList         *files)
{
	GtkTreeIter  parent_iter;
	GtkTreeIter *p_parent_iter;
	gboolean     is_empty;
	GList       *old_files;
	GList       *scan;
	GtkTreeIter  iter;

	if (g_file_equal (parent, folder_tree->priv->root))
		p_parent_iter = NULL;
	else if (gth_folder_tree_get_iter (folder_tree, parent, &parent_iter, NULL))
		p_parent_iter = &parent_iter;
	else
		return;

	if (_gth_folder_tree_iter_has_no_child (folder_tree, p_parent_iter))
		return;

	is_empty = TRUE;
	_gth_folder_tree_add_empty_item (folder_tree, p_parent_iter);
	_gth_folder_tree_remove_child_type (folder_tree, p_parent_iter, ENTRY_TYPE_LOADING);

	/* delete the children not present in the new file list */

	old_files = _gth_folder_tree_get_children (folder_tree, p_parent_iter);
	for (scan = old_files; scan; scan = scan->next) {
		GthFileData *file_data = scan->data;

		if (! gth_file_data_list_find_file (files, file_data->file)
		    && gth_folder_tree_get_iter (folder_tree, file_data->file, &iter, p_parent_iter))
		{
			gtk_tree_store_remove (folder_tree->priv->tree_store, &iter);
		}
	}
	_g_object_list_unref (old_files);

	/* add or update the new files */

	for (scan = files; scan; scan = scan->next) {
		GthFileData *file_data = scan->data;

		if (gth_folder_tree_get_iter (folder_tree, file_data->file, &iter, p_parent_iter)) {
			_gth_folder_tree_set_file_data (folder_tree, &iter, file_data);
			is_empty = FALSE;
		}
		else if (_gth_folder_tree_add_file (folder_tree, p_parent_iter, file_data))
			is_empty = FALSE;
	}

	if (! is_empty)
		_gth_folder_tree_remove_child_type (folder_tree, p_parent_iter, ENTRY_TYPE_EMPTY);

	if (p_parent_iter != NULL)
		gtk_tree_store_set (folder_tree->priv->tree_store, p_parent_iter,
				    COLUMN_LOADED, TRUE,
				    -1);

	emit_fake_motion_notify_event (folder_tree);
}


void
gth_folder_tree_loading_children (GthFolderTree *folder_tree,
				  GFile         *parent)
{
	GtkTreeIter  parent_iter;
	GtkTreeIter *p_parent_iter;
	GtkTreeIter  iter;
	gboolean     valid_iter;

	if (g_file_equal (parent, folder_tree->priv->root))
		p_parent_iter = NULL;
	else if (gth_folder_tree_get_iter (folder_tree, parent, &parent_iter, NULL))
		p_parent_iter = &parent_iter;
	else
		return;

	_gth_folder_tree_add_loading_item (folder_tree, p_parent_iter);

	/* remove anything but the loading item */
	gtk_tree_model_iter_children (GTK_TREE_MODEL (folder_tree->priv->tree_store), &iter, p_parent_iter);
	do {
		EntryType file_entry_type;

		gtk_tree_model_get (GTK_TREE_MODEL (folder_tree->priv->tree_store), &iter,
				    COLUMN_TYPE, &file_entry_type,
				    -1);

		if (file_entry_type != ENTRY_TYPE_LOADING)
			valid_iter = gtk_tree_store_remove (folder_tree->priv->tree_store, &iter);
		else
			valid_iter = gtk_tree_model_iter_next (GTK_TREE_MODEL (folder_tree->priv->tree_store), &iter);
	}
	while (valid_iter);

	emit_fake_motion_notify_event (folder_tree);
}


static gboolean
_gth_folder_tree_file_is_in_children (GthFolderTree *folder_tree,
				      GtkTreeIter   *parent_iter,
	 			      GFile         *file)
{
	GtkTreeIter iter;
	gboolean    found = FALSE;

	if (! gtk_tree_model_iter_children (GTK_TREE_MODEL (folder_tree->priv->tree_store), &iter, parent_iter))
		return FALSE;

	do {
		GthFileData *test_file_data;
		EntryType    file_entry_type;

		gtk_tree_model_get (GTK_TREE_MODEL (folder_tree->priv->tree_store), &iter,
				    COLUMN_FILE_DATA, &test_file_data,
				    COLUMN_TYPE, &file_entry_type,
				    -1);
		if ((file_entry_type == ENTRY_TYPE_FILE) && (test_file_data != NULL) && g_file_equal (file, test_file_data->file))
			found = TRUE;

		_g_object_unref (test_file_data);
	}
	while (! found && gtk_tree_model_iter_next (GTK_TREE_MODEL (folder_tree->priv->tree_store), &iter));

	return found;
}


void
gth_folder_tree_add_children (GthFolderTree *folder_tree,
			      GFile         *parent,
			      GList         *files)
{
	GtkTreeIter  parent_iter;
	GtkTreeIter *p_parent_iter;
	gboolean     is_empty;
	GList       *scan;

	if (g_file_equal (parent, folder_tree->priv->root))
		p_parent_iter = NULL;
	else if (gth_folder_tree_get_iter (folder_tree, parent, &parent_iter, NULL))
		p_parent_iter = &parent_iter;
	else
		return;

	is_empty = TRUE;
	for (scan = files; scan; scan = scan->next) {
		GthFileData *file_data = scan->data;

		if (_gth_folder_tree_file_is_in_children (folder_tree, p_parent_iter, file_data->file))
			continue;
		if (_gth_folder_tree_add_file (folder_tree, p_parent_iter, file_data))
			is_empty = FALSE;
	}

	if (! is_empty)
		_gth_folder_tree_remove_child_type (folder_tree, p_parent_iter, ENTRY_TYPE_EMPTY);
}


static gboolean
_gth_folder_tree_get_child (GthFolderTree *folder_tree,
			    GFile         *file,
			    GtkTreeIter   *file_iter,
			    GtkTreeIter   *parent)
{
	GtkTreeModel *tree_model = GTK_TREE_MODEL (folder_tree->priv->tree_store);
	GtkTreeIter   iter;

	if (! gtk_tree_model_iter_children (tree_model, &iter, parent))
		return FALSE;

	do {
		GthFileData *test_file_data;
		EntryType    file_entry_type;

		gtk_tree_model_get (tree_model, &iter,
				    COLUMN_FILE_DATA, &test_file_data,
				    COLUMN_TYPE, &file_entry_type,
				    -1);
		if ((file_entry_type == ENTRY_TYPE_FILE) && (test_file_data != NULL) && g_file_equal (file, test_file_data->file)) {
			_g_object_unref (test_file_data);
			*file_iter = iter;
			return TRUE;
		}

		_g_object_unref (test_file_data);
	}
	while (gtk_tree_model_iter_next (tree_model, &iter));

	return FALSE;
}


void
gth_folder_tree_update_children (GthFolderTree *folder_tree,
				 GFile         *parent,
				 GList         *files)
{
	GtkTreeIter  parent_iter;
	GtkTreeIter *p_parent_iter;
	GList       *scan;
	GtkTreeIter  iter;

	if (g_file_equal (parent, folder_tree->priv->root))
		p_parent_iter = NULL;
	else if (gth_folder_tree_get_iter (folder_tree, parent, &parent_iter, NULL))
		p_parent_iter = &parent_iter;
	else
		return;

	/* update each file if already present */

	for (scan = files; scan; scan = scan->next) {
		GthFileData *file_data = scan->data;

		if (_gth_folder_tree_get_child (folder_tree, file_data->file, &iter, p_parent_iter))
			_gth_folder_tree_set_file_data (folder_tree, &iter, file_data);
	}
}


void
gth_folder_tree_update_child (GthFolderTree *folder_tree,
			      GFile         *old_file,
			      GthFileData   *file_data)
{
	GtkTreeIter iter;

	if (gth_folder_tree_get_iter (folder_tree, old_file, &iter, NULL))
		_gth_folder_tree_set_file_data (folder_tree, &iter, file_data);
}


void
gth_folder_tree_delete_children (GthFolderTree *folder_tree,
				 GFile         *parent,
				 GList         *files)
{
	GtkTreeIter  parent_iter;
	GtkTreeIter *p_parent_iter;
	GList       *scan;

	if (g_file_equal (parent, folder_tree->priv->root))
		p_parent_iter = NULL;
	else if (gth_folder_tree_get_iter (folder_tree, parent, &parent_iter, NULL))
		p_parent_iter = &parent_iter;
	else
		return;

	if (_gth_folder_tree_iter_has_no_child (folder_tree, p_parent_iter))
		return;

	/* add the empty item first to not allow the folder to collapse. */
	_gth_folder_tree_add_empty_item (folder_tree, p_parent_iter);

	for (scan = files; scan; scan = scan->next) {
		GFile       *file = scan->data;
		GtkTreeIter  iter;

		if (gth_folder_tree_get_iter (folder_tree, file, &iter, p_parent_iter))
			gtk_tree_store_remove (folder_tree->priv->tree_store, &iter);
	}

	if (gtk_tree_model_iter_n_children (GTK_TREE_MODEL (folder_tree->priv->tree_store), p_parent_iter) > 1)
		_gth_folder_tree_remove_child_type (folder_tree, p_parent_iter, ENTRY_TYPE_EMPTY);
}


void
gth_folder_tree_start_editing (GthFolderTree *folder_tree,
			       GFile         *file)
{
	GtkTreeIter        iter;
	GtkTreePath       *tree_path;
	char              *path;
	GtkTreeViewColumn *tree_column;

	if (! gth_folder_tree_get_iter (folder_tree, file, &iter, NULL))
		return;

	tree_path = gtk_tree_model_get_path (GTK_TREE_MODEL (folder_tree->priv->tree_store), &iter);
	path = gtk_tree_path_to_string (tree_path);

	tree_column = gtk_tree_view_get_column (GTK_TREE_VIEW (folder_tree), 0);
	gtk_tree_view_expand_to_path (GTK_TREE_VIEW (folder_tree), tree_path);
	gtk_tree_view_collapse_row (GTK_TREE_VIEW (folder_tree), tree_path);
	gtk_tree_view_scroll_to_cell (GTK_TREE_VIEW (folder_tree), tree_path, NULL, TRUE, 0.5, 0.0);
	gtk_tree_view_set_cursor (GTK_TREE_VIEW (folder_tree),
				  tree_path,
				  tree_column,
				  TRUE);

	g_free (path);
	gtk_tree_path_free (tree_path);
}


GtkTreePath *
gth_folder_tree_get_path (GthFolderTree *folder_tree,
			  GFile         *file)
{
	GtkTreeIter iter;

	if (! gth_folder_tree_get_iter (folder_tree, file, &iter, NULL))
		return NULL;
	else
		return gtk_tree_model_get_path (GTK_TREE_MODEL (folder_tree->priv->tree_store), &iter);
}


gboolean
gth_folder_tree_is_loaded (GthFolderTree *folder_tree,
			   GtkTreePath   *path)
{
	GtkTreeIter iter;
	gboolean    loaded;

	if (! gtk_tree_model_get_iter (GTK_TREE_MODEL (folder_tree->priv->tree_store), &iter, path))
		return FALSE;

	gtk_tree_model_get (GTK_TREE_MODEL (folder_tree->priv->tree_store), &iter,
			    COLUMN_LOADED, &loaded,
			    -1);

	return loaded;
}


static void
gth_folder_tree_set_loaded (GthFolderTree *folder_tree,
			    GtkTreeIter   *root)
{
	GtkTreeModel *tree_model = GTK_TREE_MODEL (folder_tree->priv->tree_store);
	GtkTreeIter   iter;

	if (root != NULL)
		gtk_tree_store_set (folder_tree->priv->tree_store, root,
				    COLUMN_LOADED, FALSE,
				    -1);

	if (gtk_tree_model_iter_children (tree_model, &iter, root)) {
		do {
			gth_folder_tree_set_loaded (folder_tree, &iter);
		}
		while (gtk_tree_model_iter_next (tree_model, &iter));
	}
}


void
gth_folder_tree_reset_loaded (GthFolderTree *folder_tree)
{
	gth_folder_tree_set_loaded (folder_tree, NULL);
}


void
gth_folder_tree_expand_row (GthFolderTree *folder_tree,
			    GtkTreePath   *path,
			    gboolean       open_all)
{
	g_signal_handlers_block_by_func (folder_tree, row_expanded_cb, folder_tree);
	gtk_tree_view_expand_row (GTK_TREE_VIEW (folder_tree), path, open_all);
	g_signal_handlers_unblock_by_func (folder_tree, row_expanded_cb, folder_tree);
}


void
gth_folder_tree_select_path (GthFolderTree *folder_tree,
			     GtkTreePath   *path)
{
	GtkTreeSelection *selection;

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (folder_tree));
	g_signal_handlers_block_by_func (selection, selection_changed_cb, folder_tree);
	gtk_tree_selection_select_path (selection, path);
	g_signal_handlers_unblock_by_func (selection, selection_changed_cb, folder_tree);
}


GFile *
gth_folder_tree_get_root (GthFolderTree *folder_tree)
{
	return folder_tree->priv->root;
}


GthFileData *
gth_folder_tree_get_selected (GthFolderTree *folder_tree)
{
	GtkTreeSelection *selection;
	GtkTreeModel     *tree_model;
	GtkTreeIter       iter;
	EntryType         entry_type;
	GthFileData      *file_data;

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (folder_tree));
	if (selection == NULL)
		return NULL;

	tree_model = GTK_TREE_MODEL (folder_tree->priv->tree_store);
	if (! gtk_tree_selection_get_selected (selection, &tree_model, &iter))
		return NULL;

	gtk_tree_model_get (GTK_TREE_MODEL (folder_tree->priv->tree_store),
			    &iter,
			    COLUMN_TYPE, &entry_type,
			    COLUMN_FILE_DATA, &file_data,
			    -1);

	if (entry_type != ENTRY_TYPE_FILE) {
		_g_object_unref (file_data);
		file_data = NULL;
	}

	return file_data;
}


GthFileData *
gth_folder_tree_get_selected_or_parent (GthFolderTree *folder_tree)
{
	GtkTreeSelection *selection;
	GtkTreeModel     *tree_model;
	GtkTreeIter       iter;
	GtkTreeIter       parent;
	EntryType         entry_type;
	GthFileData      *file_data;

	file_data = gth_folder_tree_get_selected (folder_tree);
	if (file_data != NULL)
		return file_data;

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (folder_tree));
	if (selection == NULL)
		return NULL;

	tree_model = GTK_TREE_MODEL (folder_tree->priv->tree_store);
	if (! gtk_tree_selection_get_selected (selection, &tree_model, &iter))
		return NULL;

	if (! gtk_tree_model_iter_parent (tree_model, &parent, &iter))
		return NULL;

	gtk_tree_model_get (GTK_TREE_MODEL (folder_tree->priv->tree_store),
			    &parent,
			    COLUMN_TYPE, &entry_type,
			    COLUMN_FILE_DATA, &file_data,
			    -1);

	if (entry_type != ENTRY_TYPE_FILE) {
		_g_object_unref (file_data);
		file_data = NULL;
	}

	return file_data;
}
