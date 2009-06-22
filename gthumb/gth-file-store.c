/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2008-2009 Free Software Foundation, Inc.
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
#include <glib/gi18n.h>
#include "glib-utils.h"
#include "gth-file-store.h"


#undef DEBUG_FILE_STORE
#define VALID_ITER(iter, store) (((iter) != NULL) && ((iter)->stamp == (store)->priv->stamp) && ((iter)->user_data != NULL))
#define REALLOC_STEP 32


static GType column_type[GTH_FILE_STORE_N_COLUMNS] = { G_TYPE_INVALID, };


typedef struct {
	GthFileData *file;
	GdkPixbuf   *thumbnail;
	gboolean     is_icon;
	char        *metadata;

	/*< private >*/

	guint        pos;
	guint        abs_pos;
	gboolean     visible;
	gboolean     changed;
} GthFileRow;


struct _GthFileStorePrivate
{
	GthFileRow         **all_rows;
	GthFileRow         **rows;
	guint                size;
	guint                tot_rows;
	guint                num_rows;
	int                  stamp;
	gboolean             load_thumbs;
	GthTest             *filter;
	GList               *queue;
	int                  queue_size;
	GthFileDataCompFunc  cmp_func;
	gboolean             inverse_sort;
	gboolean             update_filter;
};


static GObjectClass *parent_class = NULL;


static GthFileRow *
_gth_file_row_new (void)
{
	return g_new0 (GthFileRow, 1);
}


static void
_gth_file_row_set_file (GthFileRow  *row,
			GthFileData *file)
{
	if (file != NULL) {
		g_object_ref (file);
		if (row->file != NULL)
			g_object_unref (row->file);
		row->file = file;
	}
}


static void
_gth_file_row_set_thumbnail (GthFileRow *row,
			     GdkPixbuf  *thumbnail)
{
	if (thumbnail != NULL) {
		g_object_ref (thumbnail);
		if (row->thumbnail != NULL)
			g_object_unref (row->thumbnail);
		row->thumbnail = thumbnail;
	}
}


static void
_gth_file_row_set_metadata (GthFileRow *row,
			    const char *metadata)
{
	if (metadata == NULL)
		return;

	if (metadata == row->metadata)
		return;

	g_free (row->metadata);
	row->metadata = NULL;
	row->metadata = g_strdup (metadata);
}


GthFileRow *
_gth_file_row_copy (GthFileRow *row)
{
	GthFileRow *row2;

	row2 = _gth_file_row_new ();
	_gth_file_row_set_file (row2, row->file);
	_gth_file_row_set_thumbnail (row2, row->thumbnail);
	_gth_file_row_set_metadata (row2, row->metadata);
	row2->is_icon = row->is_icon;
	row2->pos = row->pos;
	row2->abs_pos = row->abs_pos;
	row2->visible = row->visible;
	row2->changed = row->changed;

	return row2;
}


static void
_gth_file_row_free (GthFileRow *row)
{
	if (row->file != NULL)
		g_object_unref (row->file);
	if (row->thumbnail != NULL)
		g_object_unref (row->thumbnail);
	g_free (row->metadata);
	g_free (row);
}


static void
_gth_file_store_clear_queue (GthFileStore *file_store)
{
	g_list_free (file_store->priv->queue);
	file_store->priv->queue = NULL;
	file_store->priv->queue_size = 0;
	file_store->priv->update_filter = FALSE;
}


static void
_gth_file_store_free_rows (GthFileStore *file_store)
{
	int i;

	for (i = 0; i < file_store->priv->tot_rows; i++)
		_gth_file_row_free (file_store->priv->all_rows[i]);
	g_free (file_store->priv->all_rows);
	file_store->priv->all_rows = NULL;
	file_store->priv->tot_rows = 0;

	g_free (file_store->priv->rows);
	file_store->priv->rows = NULL;
	file_store->priv->num_rows = 0;

	file_store->priv->size = 0;

	_gth_file_store_clear_queue (file_store);
}


static void
gth_file_store_finalize (GObject *object)
{
	GthFileStore *file_store;

	file_store = GTH_FILE_STORE (object);

	if (file_store->priv != NULL) {
		_gth_file_store_free_rows (file_store);
		if (file_store->priv->filter != NULL)
			g_object_unref (file_store->priv->filter);
		g_free (file_store->priv);
		file_store->priv = NULL;
	}

	G_OBJECT_CLASS (parent_class)->finalize (object);
}


static void
gth_file_store_class_init (GthFileStoreClass *class)
{
	GObjectClass *object_class;

	parent_class = g_type_class_peek_parent (class);

	object_class = (GObjectClass*) class;
	object_class->finalize = gth_file_store_finalize;
}


static void
gth_file_store_init (GthFileStore *file_store)
{
	file_store->priv = g_new0 (GthFileStorePrivate, 1);
	file_store->priv->rows = NULL;
	file_store->priv->size = 0;
	file_store->priv->num_rows = 0;
	file_store->priv->stamp = g_random_int ();
	file_store->priv->filter = gth_test_new ();

	if (column_type[0] == G_TYPE_INVALID) {
		column_type[GTH_FILE_STORE_FILE_COLUMN] = GTH_TYPE_FILE_DATA;
		column_type[GTH_FILE_STORE_THUMBNAIL_COLUMN] = GDK_TYPE_PIXBUF;
		column_type[GTH_FILE_STORE_IS_ICON_COLUMN] = G_TYPE_BOOLEAN;
		column_type[GTH_FILE_STORE_FILENAME_COLUMN] = G_TYPE_STRING;
		column_type[GTH_FILE_STORE_METADATA_COLUMN] = G_TYPE_STRING;
	}
}


static GtkTreeModelFlags
gth_file_store_get_flags (GtkTreeModel *tree_model)
{
	return GTK_TREE_MODEL_LIST_ONLY /*| GTK_TREE_MODEL_ITERS_PERSIST*/;
}


static gint
gth_file_store_get_n_columns (GtkTreeModel *tree_model)
{
	return GTH_FILE_STORE_N_COLUMNS;
}


static GType
gth_file_store_get_column_type (GtkTreeModel *tree_model,
				int           index)
{
	g_return_val_if_fail ((index >= 0) && (index < GTH_FILE_STORE_N_COLUMNS), G_TYPE_INVALID);

	return column_type[index];
}


static gboolean
gth_file_store_get_iter (GtkTreeModel *tree_model,
			 GtkTreeIter  *iter,
			 GtkTreePath  *path)
{
	GthFileStore *file_store;
	GthFileRow   *row;
	int          *indices, n;

	g_return_val_if_fail (path != NULL, FALSE);

	file_store = GTH_FILE_STORE (tree_model);

	indices = gtk_tree_path_get_indices (path);
	n = indices[0];
	if ((n < 0) || (n >= file_store->priv->num_rows))
		return FALSE;

	row = file_store->priv->rows[n];
	g_return_val_if_fail (row != NULL, FALSE);
	g_return_val_if_fail (row->pos == n, FALSE);

	iter->stamp = file_store->priv->stamp;
	iter->user_data = row;

	return TRUE;
}


static GtkTreePath *
gth_file_store_get_path (GtkTreeModel *tree_model,
			 GtkTreeIter  *iter)
{
	GthFileStore *file_store;
	GthFileRow   *row;
	GtkTreePath  *path;

	g_return_val_if_fail (iter != NULL, NULL);
	g_return_val_if_fail (iter->user_data != NULL, NULL);

	file_store = GTH_FILE_STORE (tree_model);

	g_return_val_if_fail (VALID_ITER (iter, file_store), NULL);

	row = (GthFileRow*) iter->user_data;

	path = gtk_tree_path_new ();
	gtk_tree_path_append_index (path, row->pos);

	return path;
}


static void
gth_file_store_get_value (GtkTreeModel *tree_model,
			  GtkTreeIter  *iter,
			  int           column,
			  GValue       *value)
{
	GthFileStore *file_store;
	GthFileRow   *row;

	g_return_if_fail ((column >= 0) && (column < GTH_FILE_STORE_N_COLUMNS));

	file_store = GTH_FILE_STORE (tree_model);

	g_return_if_fail (VALID_ITER (iter, file_store));

	row = (GthFileRow*) iter->user_data;

	switch (column) {
	case GTH_FILE_STORE_FILE_COLUMN:
		g_value_init (value, GTH_TYPE_FILE_DATA);
		g_value_set_object (value, row->file);
		break;
	case GTH_FILE_STORE_THUMBNAIL_COLUMN:
		g_value_init (value, GDK_TYPE_PIXBUF);
		g_value_set_object (value, row->thumbnail);
		break;
	case GTH_FILE_STORE_IS_ICON_COLUMN:
		g_value_init (value, G_TYPE_BOOLEAN);
		g_value_set_boolean (value, row->is_icon);
		break;
	case GTH_FILE_STORE_FILENAME_COLUMN:
		g_value_init (value, G_TYPE_STRING);
		g_value_set_string (value, g_file_info_get_display_name (row->file->info));
		break;
	case GTH_FILE_STORE_METADATA_COLUMN:
		g_value_init (value, G_TYPE_STRING);
		g_value_set_string (value, row->metadata);
		break;
	}
}


static gboolean
gth_file_store_iter_next (GtkTreeModel  *tree_model,
			  GtkTreeIter   *iter)
{
	GthFileStore *file_store;
	GthFileRow   *row;

	if ((iter == NULL) || (iter->user_data == NULL))
		return FALSE;

	file_store = GTH_FILE_STORE (tree_model);

	g_return_val_if_fail (VALID_ITER (iter, file_store), FALSE);

  	row = (GthFileRow*) iter->user_data;
	if ((row->pos + 1) >= file_store->priv->num_rows)
		return FALSE;

	iter->stamp = file_store->priv->stamp;
	iter->user_data = file_store->priv->rows[row->pos + 1];

	return TRUE;
}


static gboolean
gth_file_store_iter_children (GtkTreeModel *tree_model,
			      GtkTreeIter  *iter,
			      GtkTreeIter  *parent)
{
	GthFileStore *file_store;

	if (parent != NULL)
		return FALSE;

	g_return_val_if_fail (parent->user_data != NULL, FALSE);

	file_store = GTH_FILE_STORE (tree_model);

	if (file_store->priv->num_rows == 0)
		return FALSE;

	iter->stamp = file_store->priv->stamp;
	iter->user_data = file_store->priv->rows[0];

	return TRUE;
}


static gboolean
gth_file_store_iter_has_child (GtkTreeModel *tree_model,
			       GtkTreeIter  *iter)
{
	return FALSE;
}


static gint
gth_file_store_iter_n_children (GtkTreeModel *tree_model,
				GtkTreeIter  *iter)
{
	GthFileStore *file_store;

	file_store = GTH_FILE_STORE (tree_model);

	if (iter == NULL)
		return file_store->priv->num_rows;

	g_return_val_if_fail (VALID_ITER (iter, file_store), 0);

  	return 0;
}


static gboolean
gth_file_store_iter_nth_child (GtkTreeModel *tree_model,
			       GtkTreeIter  *iter,
			       GtkTreeIter  *parent,
			       int           n)
{
	GthFileStore *file_store;

  	file_store = GTH_FILE_STORE (tree_model);

	if (parent != NULL) {
		g_return_val_if_fail (VALID_ITER (parent, file_store), FALSE);
		return FALSE;
	}

	if (n >= file_store->priv->num_rows)
		return FALSE;

	iter->stamp = file_store->priv->stamp;
	iter->user_data = file_store->priv->rows[n];

	return TRUE;
}


static gboolean
gth_file_store_iter_parent (GtkTreeModel *tree_model,
			    GtkTreeIter  *iter,
			    GtkTreeIter  *child)
{
	GthFileStore *file_store;

	g_return_val_if_fail (child == NULL, FALSE);

	file_store = GTH_FILE_STORE (tree_model);

	g_return_val_if_fail (VALID_ITER (child, file_store), FALSE);

	return FALSE;
}


static gboolean
gth_file_store_row_draggable (GtkTreeDragSource *drag_source,
                              GtkTreePath       *path)
{
	return TRUE;
}


static gboolean
gth_file_store_drag_data_get (GtkTreeDragSource *drag_source,
                              GtkTreePath       *path,
                              GtkSelectionData  *selection_data)
{
	gboolean      retval = FALSE;
	GthFileStore *file_store;
	int          *indices, n;
	GthFileRow   *row;

	g_return_val_if_fail (path != NULL, FALSE);

	file_store = GTH_FILE_STORE (drag_source);

	indices = gtk_tree_path_get_indices (path);
	n = indices[0];
	if ((n < 0) || (n >= file_store->priv->num_rows))
		return FALSE;

	row = file_store->priv->rows[n];
	g_return_val_if_fail (row != NULL, FALSE);
	g_return_val_if_fail (row->pos == n, FALSE);

	if (gtk_selection_data_targets_include_uri (selection_data)) {
		char **uris;

		uris = g_new (char *, 2);
		uris[0] = g_file_get_uri (row->file->file);
		uris[1] = NULL;
		gtk_selection_data_set_uris (selection_data, uris);
		retval = TRUE;

		g_strfreev (uris);
	}
	else if (gtk_selection_data_targets_include_text (selection_data)) {
		char *parse_name;

		parse_name = g_file_get_parse_name (row->file->file);
		gtk_selection_data_set_text (selection_data, parse_name, -1);
		retval = TRUE;

		g_free (parse_name);
	}

	return retval;
}

static gboolean
gth_file_store_drag_data_delete (GtkTreeDragSource *drag_source,
                                 GtkTreePath       *path)
{
	GthFileStore *file_store;
	int          *indices, n;
	GthFileRow   *row;

	g_return_val_if_fail (path != NULL, FALSE);

	file_store = GTH_FILE_STORE (drag_source);

	indices = gtk_tree_path_get_indices (path);
	n = indices[0];
	if ((n < 0) || (n >= file_store->priv->num_rows))
		return FALSE;

	row = file_store->priv->rows[n];
	g_return_val_if_fail (row != NULL, FALSE);
	g_return_val_if_fail (row->pos == n, FALSE);

	gth_file_store_remove (file_store, row->abs_pos);

	return TRUE;
}


static void
gtk_tree_model_iface_init (GtkTreeModelIface *iface)
{
	iface->get_flags       = gth_file_store_get_flags;
	iface->get_n_columns   = gth_file_store_get_n_columns;
	iface->get_column_type = gth_file_store_get_column_type;
	iface->get_iter        = gth_file_store_get_iter;
	iface->get_path        = gth_file_store_get_path;
	iface->get_value       = gth_file_store_get_value;
	iface->iter_next       = gth_file_store_iter_next;
	iface->iter_children   = gth_file_store_iter_children;
	iface->iter_has_child  = gth_file_store_iter_has_child;
	iface->iter_n_children = gth_file_store_iter_n_children;
	iface->iter_nth_child  = gth_file_store_iter_nth_child;
	iface->iter_parent     = gth_file_store_iter_parent;
}


static void
gtk_tree_drag_source_iface_init (GtkTreeDragSourceIface *iface)
{
	iface->row_draggable = gth_file_store_row_draggable;
	iface->drag_data_get = gth_file_store_drag_data_get;
	iface->drag_data_delete = gth_file_store_drag_data_delete;
}


GType
gth_file_store_get_type (void)
{
	static GType type = 0;

	if (! type) {
		GTypeInfo type_info = {
			sizeof (GthFileStoreClass),
			NULL,
			NULL,
			(GClassInitFunc) gth_file_store_class_init,
			NULL,
			NULL,
			sizeof (GthFileStore),
			0,
			(GInstanceInitFunc) gth_file_store_init
		};
		static const GInterfaceInfo gtk_tree_model_info = {
			(GInterfaceInitFunc) gtk_tree_model_iface_init,
			NULL,
			NULL
		};
		static const GInterfaceInfo gtk_tree_drag_source_info = {
			(GInterfaceInitFunc) gtk_tree_drag_source_iface_init,
			NULL,
			NULL
		};

		type = g_type_register_static (G_TYPE_OBJECT, "GthFileStore", &type_info, 0);
		g_type_add_interface_static (type, GTK_TYPE_TREE_MODEL, &gtk_tree_model_info);
		g_type_add_interface_static (type, GTK_TYPE_TREE_DRAG_SOURCE, &gtk_tree_drag_source_info);
	}

	return type;
}


GthFileStore *
gth_file_store_new (void)
{
	return (GthFileStore*) g_object_new (GTH_TYPE_FILE_STORE, NULL);
}


int
gth_file_store_get_abs_pos (GthFileStore *file_store,
			    int           pos)
{
	g_return_val_if_fail ((pos >= 0) && (pos < file_store->priv->num_rows), -1);

	return file_store->priv->rows[pos]->abs_pos;
}


static void
_gth_file_store_increment_stamp (GthFileStore *file_store)
{
	do {
		file_store->priv->stamp++;
	}
	while (file_store->priv->stamp == 0);
}


G_GNUC_UNUSED
static GList *
_gth_file_store_get_files (GthFileStore *file_store)
{
	GList *files = NULL;
	int    i;

	for (i = 0; i < file_store->priv->tot_rows; i++) {
		GthFileRow *row = file_store->priv->all_rows[i];
		files = g_list_prepend (files, g_object_ref (row->file));
	}

	return  g_list_reverse (files);
}


static void
_gth_file_store_row_changed (GthFileStore *file_store,
			     GthFileRow   *row)
{
	GtkTreePath *path;
	GtkTreeIter  iter;

	path = gtk_tree_path_new ();
	gtk_tree_path_append_index (path, row->pos);
	gth_file_store_get_iter (GTK_TREE_MODEL (file_store), &iter, path);

	gtk_tree_model_row_changed (GTK_TREE_MODEL (file_store), path, &iter);

	gtk_tree_path_free (path);
}


static int
campare_row_func (gconstpointer a,
		  gconstpointer b,
		  gpointer      user_data)
{
	GthFileStore *file_store = user_data;
	GthFileRow   *row_a = *((GthFileRow **) a);
	GthFileRow   *row_b = *((GthFileRow **) b);
	int           result;

	if (file_store->priv->cmp_func == NULL)
		result = 1 /*strcmp (gth_file_data_get_filename_sort_key (row_a->file),
			       gth_file_data_get_filename_sort_key (row_b->file))*/;
	else
		result = file_store->priv->cmp_func (row_a->file, row_b->file);

	if (file_store->priv->inverse_sort)
		result = result * -1;

	return result;
}


static void
_gth_file_store_sort (GthFileStore *file_store,
		      gconstpointer pbase,
		      gint          total_elems)
{
	g_qsort_with_data (pbase,
			   total_elems,
			   (gsize) sizeof (GthFileRow *),
			   campare_row_func,
			   file_store);
}


static int
campare_by_pos (gconstpointer a,
		gconstpointer b,
		gpointer      user_data)
{
	GthFileRow *row_a = *((GthFileRow **) a);
	GthFileRow *row_b = *((GthFileRow **) b);

	if (row_a->pos == row_b->pos)
		return 0;
	else if (row_a->pos > row_b->pos)
		return 1;
	else
		return -1;
}


static void
_gth_file_store_compact_rows (GthFileStore *file_store)
{
	int i, j;

	for (i = 0, j = 0; i < file_store->priv->tot_rows; i++)
		if (file_store->priv->all_rows[i] != NULL) {
			file_store->priv->all_rows[j] = file_store->priv->all_rows[i];
			file_store->priv->all_rows[j]->abs_pos = j;
			j++;
		}
	file_store->priv->tot_rows = j;

	for (i = 0, j = 0; i < file_store->priv->num_rows; i++)
		if (file_store->priv->rows[i] != NULL) {
			file_store->priv->rows[j] = file_store->priv->rows[i];
			file_store->priv->rows[j]->pos = j;
			j++;
		}
	file_store->priv->num_rows = j;
}


static void
_gth_file_store_hide_row (GthFileStore *file_store,
			  GthFileRow   *row)
{
	int          k;
	GtkTreePath *path;

	file_store->priv->rows[row->pos] = NULL;
	for (k = row->pos; k < file_store->priv->num_rows - 1;  k++) {
		file_store->priv->rows[k] = file_store->priv->rows[k + 1];
		file_store->priv->rows[k]->pos--;
	}
	file_store->priv->num_rows--;

	path = gtk_tree_path_new_from_indices (row->pos, -1);
	gtk_tree_model_row_deleted (GTK_TREE_MODEL (file_store), path);
	gtk_tree_path_free (path);
}


static void
_gth_file_store_update_visibility (GthFileStore *file_store,
				   GList        *add_queue)
{
	GthFileRow **all_rows = NULL;
	guint        all_rows_n = 0;
	GthFileRow **old_rows = NULL;
	guint        old_rows_n = 0;
	GthFileRow **new_rows = NULL;
	guint        new_rows_n = 0;
	int          i;
	GList       *files;
	GList       *scan;
	GthFileData *file;
	int          j, k;
	gboolean     row_deleted;
	gboolean     row_inserted;


#ifdef DEBUG_FILE_STORE
g_print ("UPDATE VISIBILITY\n");
#endif

	/* store the current state */

	all_rows_n = file_store->priv->tot_rows + g_list_length (add_queue);
	all_rows = g_new (GthFileRow *, all_rows_n);
	for (i = 0; i < file_store->priv->tot_rows; i++)
		all_rows[i] = _gth_file_row_copy (file_store->priv->all_rows[i]);

	for (scan = add_queue; scan; scan = scan->next)
		all_rows[i++] = (GthFileRow *) scan->data;

	old_rows_n = file_store->priv->num_rows;
	old_rows = g_new (GthFileRow *, old_rows_n);
	for (i = 0, j = 0; i < all_rows_n; i++) {
		if (all_rows[i]->visible) {
			old_rows[j] = all_rows[i];
			old_rows[j]->visible = FALSE;
			j++;
		}
	}

	/* reorder and filter */

	_gth_file_store_sort (file_store, all_rows, all_rows_n);

	files = NULL;
	for (i = 0; i < all_rows_n; i++) {
		GthFileRow *row = all_rows[i];

		row->abs_pos = i;
		files = g_list_prepend (files, g_object_ref (row->file));
	}
	files = g_list_reverse (files);

	new_rows_n = 0;
	gth_test_set_file_list (file_store->priv->filter, files);
	while ((file = gth_test_get_next (file_store->priv->filter)) != NULL) {
		GthFileRow *row = NULL;

		for (i = 0; i < all_rows_n; i++) {
			row = all_rows[i];
			if (row->file == file)
				break;
		}

		g_assert (i < all_rows_n);

		row->visible = TRUE;
		new_rows_n++;
	}

	_g_object_list_unref (files);

	/* create the new visible rows array */

	new_rows = g_new (GthFileRow *, new_rows_n);
	for (i = 0, j = 0; i < all_rows_n; i++) {
		GthFileRow *row = all_rows[i];

		if (! row->visible)
			continue;

		row->pos = j++;
		new_rows[row->pos] = row;
	}

	/* emit the signals required to go from the old state to the new state */

	/* hide filtered out files */

	row_deleted = FALSE;
	for (i = old_rows_n - 1; i >= 0; i--) {
		/* search old_rows[i] in new_rows */

		for (j = 0; j < new_rows_n; j++)
			if (old_rows[i]->file == new_rows[j]->file)
				break;

		if (j < new_rows_n)
			continue;

		/* old_rows[i] is not present in new_rows, emit a deleted
		 * signal. */

#ifdef DEBUG_FILE_STORE
g_print ("  DELETE: %d\n", old_rows[i]->pos);
#endif

		_gth_file_store_hide_row (file_store, old_rows[i]);

		old_rows[i] = NULL;
		row_deleted = TRUE;
	}

	/* compact the old visible rows array if needed */

	if (row_deleted) {
		for (i = 0, j = 0; i < old_rows_n; i++) {
			if (old_rows[i] != NULL) {
				old_rows[j] = old_rows[i];
				old_rows[j]->abs_pos = j;
				j++;
			}
		}
		old_rows_n = j;
	}

	/* Both old_rows and file_store->priv->rows now contain the files
	 * visible before and after the filtering.
	 * Reorder the two arrays according to the new_rows order */

	if (old_rows_n > 0) {
		gboolean    order_changed;
		int        *new_order;
		GthFileRow *tmp_row;

		order_changed = FALSE;
		new_order = g_new0 (int, old_rows_n);
		for (i = 0, k = 0; i < new_rows_n; i++) {
			/* search new_rows[i] in old_rows */

			for (j = 0; j < old_rows_n; j++)
				if (new_rows[i]->file == old_rows[j]->file)
					break;

			if (j >= old_rows_n)
				continue;

			/* old_rows[j] == new_rows[i] */

			/* k is the new position of old_rows[j] in new_rows
			 * without considering the new elements, that is
			 * the elements in new_rows not present in old_rows */

			new_order[k] = j;
			old_rows[j]->pos = k;

			/* swap the position of j and k in file_store->priv->rows
			 * old_rows can't be reordered now because we need
			 * to know the old positions, it will be ordered
			 * later with g_qsort_with_data */

			if (k != j) {
				tmp_row = file_store->priv->rows[j];
				file_store->priv->rows[j] = file_store->priv->rows[k];
				file_store->priv->rows[j]->pos = j;
				file_store->priv->rows[k] = tmp_row;
				file_store->priv->rows[k]->pos = k;

				order_changed = TRUE;
			}

			k++;
		}
		if (order_changed) {

#ifdef DEBUG_FILE_STORE
g_print ("  REORDER: ");
for (i = 0; i < old_rows_n; i++)
	g_print ("%d ", new_order[i]);
g_print ("\n");
#endif

			gtk_tree_model_rows_reordered (GTK_TREE_MODEL (file_store), NULL, NULL, new_order);
		}

		g_free (new_order);
	}

	/* set the new state */

	_gth_file_store_increment_stamp (file_store);

	for (i = 0; i < file_store->priv->tot_rows; i++)
		_gth_file_row_free (file_store->priv->all_rows[i]);
	g_free (file_store->priv->all_rows);
	file_store->priv->all_rows = all_rows;
	file_store->priv->tot_rows = all_rows_n;

	g_qsort_with_data (old_rows,
			   old_rows_n,
			   (gsize) sizeof (GthFileRow *),
			   campare_by_pos,
			   NULL);

	g_free (file_store->priv->rows);
	file_store->priv->rows = g_new (GthFileRow *, new_rows_n);
	for (i = 0; i < old_rows_n; i++)
		file_store->priv->rows[i] = old_rows[i];

	g_assert (file_store->priv->num_rows == old_rows_n);

	/* add the new files */

	row_inserted = FALSE;
	for (i = 0; i < new_rows_n; i++) {
		GtkTreePath *path;
		GtkTreeIter  iter;

		if ((i < file_store->priv->num_rows) && (new_rows[i]->file == file_store->priv->rows[i]->file))
			continue;

#ifdef DEBUG_FILE_STORE
g_print ("  INSERT: %d\n", i);
#endif

		/* add the file to the visible rows */

		for (k = file_store->priv->num_rows; k > i; k--) {
			file_store->priv->rows[k] = file_store->priv->rows[k - 1];
			file_store->priv->rows[k]->pos++;
		}
		file_store->priv->rows[i] = new_rows[i];
		file_store->priv->rows[i]->pos = i;
		file_store->priv->num_rows++;

		path = gtk_tree_path_new ();
		gtk_tree_path_append_index (path, i);
		gth_file_store_get_iter (GTK_TREE_MODEL (file_store), &iter, path);
		gtk_tree_model_row_inserted (GTK_TREE_MODEL (file_store), path, &iter);
		gtk_tree_path_free (path);

		row_inserted = TRUE;
	}

	g_free (new_rows);
	g_free (old_rows);
}


void
gth_file_store_set_filter (GthFileStore *file_store,
			   GthTest      *filter)
{
	if (file_store->priv->filter != NULL) {
		g_object_unref (file_store->priv->filter);
		file_store->priv->filter = NULL;
	}

	if (filter != NULL)
		file_store->priv->filter = g_object_ref (filter);
	else
		file_store->priv->filter = gth_test_new ();

	_gth_file_store_update_visibility (file_store, NULL);
}


static void
_gth_file_store_reorder (GthFileStore *file_store)
{
	int *new_order;
	int  i, j;

	_gth_file_store_sort (file_store, file_store->priv->all_rows, file_store->priv->tot_rows);

	/* compute the new position for each row */

	new_order = g_new (int, file_store->priv->num_rows);
	for (i = 0, j = -1; i < file_store->priv->tot_rows; i++) {
		GthFileRow *row = file_store->priv->all_rows[i];
		if (row->visible) {
			j = j + 1;
			file_store->priv->rows[j] = row;
			new_order[j] = row->pos;
			row->pos = j;
		}
		row->abs_pos = i;
	}
	if (new_order != NULL)
		gtk_tree_model_rows_reordered (GTK_TREE_MODEL (file_store), NULL, NULL, new_order);
	g_free (new_order);
}


void
gth_file_store_set_sort_func (GthFileStore        *file_store,
			      GthFileDataCompFunc  cmp_func,
			      gboolean             inverse_sort)
{
	file_store->priv->cmp_func = cmp_func;
	file_store->priv->inverse_sort = inverse_sort;
	_gth_file_store_reorder (file_store);
}


GList *
gth_file_store_get_all (GthFileStore *file_store)
{
	GList *list = NULL;
	int    i;

	for (i = 0; i < file_store->priv->tot_rows; i++)
		list = g_list_prepend (list, g_object_ref (file_store->priv->all_rows[i]->file));

	return g_list_reverse (list);
}


int
gth_file_store_n_files (GthFileStore *file_store)
{
	return file_store->priv->tot_rows;
}


GList *
gth_file_store_get_visibles (GthFileStore *file_store)
{
	GList *list = NULL;
	int    i;

	for (i = 0; i < file_store->priv->num_rows; i++)
		list = g_list_prepend (list, g_object_ref (file_store->priv->rows[i]->file));

	return g_list_reverse (list);
}


int
gth_file_store_n_visibles (GthFileStore *file_store)
{
	return file_store->priv->num_rows;
}


GthFileData *
gth_file_store_get_file (GthFileStore *file_store,
			 GtkTreeIter  *iter)
{
	g_return_val_if_fail (VALID_ITER (iter, file_store), NULL);

	return ((GthFileRow *) iter->user_data)->file;
}


GthFileData *
gth_file_store_get_file_at_pos (GthFileStore *file_store,
				int           pos)
{
	GthFileData *file_data = NULL;
	GtkTreePath *path;
	GtkTreeIter  iter;

	if (pos < 0)
		return NULL;

	path = gtk_tree_path_new ();
	gtk_tree_path_append_index (path, pos);
	if (gtk_tree_model_get_iter (GTK_TREE_MODEL (file_store), &iter, path))
		file_data = gth_file_store_get_file (file_store, &iter);

	gtk_tree_path_free (path);

	return file_data;
}


int
gth_file_store_find (GthFileStore *file_store,
		     GFile        *file)
{
	int i;

	for (i = 0; i < file_store->priv->tot_rows; i++) {
		GthFileRow *row = file_store->priv->all_rows[i];

		if (row == NULL)
			continue;

		if (g_file_equal (row->file->file, file))
			return i;
	}

	return -1;
}


int
gth_file_store_find_visible (GthFileStore *file_store,
			     GFile        *file)
{
	int i;

	for (i = 0; i < file_store->priv->num_rows; i++) {
		GthFileRow *row = file_store->priv->rows[i];

		if (row == NULL)
			continue;

		if (g_file_equal (row->file->file, file))
			return i;
	}

	return -1;
}


void
gth_file_store_add (GthFileStore *file_store,
		    GthFileData  *file,
		    GdkPixbuf    *thumbnail,
		    gboolean      is_icon,
		    const char   *metadata)
{
	gth_file_store_queue_add (file_store, file, thumbnail, is_icon, metadata);
	gth_file_store_exec_add (file_store);
}


void
gth_file_store_queue_add (GthFileStore *file_store,
			  GthFileData  *file,
			  GdkPixbuf    *thumbnail,
			  gboolean      is_icon,
			  const char   *metadata)
{
	GthFileRow *row;

	g_return_if_fail (file != NULL);

	/*file_store->priv->tot_rows++;
	if (file_store->priv->tot_rows > file_store->priv->size) {
		file_store->priv->size += REALLOC_STEP;
		file_store->priv->all_rows = g_realloc (file_store->priv->all_rows, sizeof (GthFileRow*) * file_store->priv->size);
	}

	row = _gth_file_row_new ();
	_gth_file_row_set_file (row, file);
	_gth_file_row_set_thumbnail (row, thumbnail);
	_gth_file_row_set_metadata (row, metadata);
	row->is_icon = is_icon;
  	row->abs_pos = file_store->priv->tot_rows - 1;
	file_store->priv->all_rows[row->abs_pos] = row;*/

	row = _gth_file_row_new ();
	_gth_file_row_set_file (row, file);
	_gth_file_row_set_thumbnail (row, thumbnail);
	_gth_file_row_set_metadata (row, metadata);
	row->is_icon = is_icon;
	file_store->priv->queue = g_list_prepend (file_store->priv->queue, row);
}


void
gth_file_store_exec_add (GthFileStore *file_store)
{
	_gth_file_store_update_visibility (file_store, file_store->priv->queue);
	_gth_file_store_clear_queue (file_store);
}


void
gth_file_store_set (GthFileStore *file_store,
		    int           abs_pos,
		    GthFileData  *file,
		    GdkPixbuf    *thumbnail,
		    gboolean      is_icon,
		    const char   *metadata)
{
	gth_file_store_queue_set (file_store, abs_pos, file, thumbnail, is_icon, metadata);
	gth_file_store_exec_set (file_store);
}


void
gth_file_store_queue_set (GthFileStore *file_store,
			  int           abs_pos,
			  GthFileData  *file,
			  GdkPixbuf    *thumbnail,
			  gboolean      is_icon,
			  const char   *metadata)
{
	GthFileRow *row;

	g_return_if_fail ((abs_pos >= 0) && (abs_pos < file_store->priv->tot_rows));

	row = file_store->priv->all_rows[abs_pos];
	_gth_file_row_set_file (row, file);
	_gth_file_row_set_thumbnail (row, thumbnail);
	_gth_file_row_set_metadata (row, metadata);
	row->is_icon = is_icon;
	row->changed = TRUE;

	if (file != NULL)
		file_store->priv->update_filter = TRUE;
}


static void
_gth_file_store_list_changed (GthFileStore *file_store)
{
	int i;

	for (i = 0; i < file_store->priv->num_rows; i++) {
		GthFileRow *row = file_store->priv->rows[i];

		if (row->visible && row->changed)
			_gth_file_store_row_changed (file_store, row);
		row->changed = FALSE;
	}
}


void
gth_file_store_exec_set (GthFileStore *file_store)
{
	_gth_file_store_list_changed (file_store);
	_gth_file_store_clear_queue (file_store);
	if (file_store->priv->update_filter)
		_gth_file_store_update_visibility (file_store, NULL);
}


void
gth_file_store_remove (GthFileStore *file_store,
		       int           abs_pos)
{
	gth_file_store_queue_remove (file_store, abs_pos);
	gth_file_store_exec_remove (file_store);
}


void
gth_file_store_queue_remove (GthFileStore *file_store,
			     int           abs_pos)
{
	g_return_if_fail ((abs_pos >= 0) && (abs_pos < file_store->priv->tot_rows));

	file_store->priv->queue = g_list_prepend (file_store->priv->queue, file_store->priv->all_rows[abs_pos]);
}


void
gth_file_store_exec_remove (GthFileStore *file_store)
{
	GList *scan;

	if (file_store->priv->queue == NULL)
		return;

	file_store->priv->queue = g_list_reverse (file_store->priv->queue);
	for (scan = file_store->priv->queue; scan; scan = scan->next) {
		GthFileRow *row = scan->data;

		if (row->visible)
			_gth_file_store_hide_row (file_store, row);

		file_store->priv->all_rows[row->abs_pos] = NULL;
		_gth_file_row_free (row);
	}
	_gth_file_store_compact_rows (file_store);
	_gth_file_store_clear_queue (file_store);

	_gth_file_store_update_visibility (file_store, NULL);
}


void
gth_file_store_clear (GthFileStore *file_store)
{
	int i;

	for (i = file_store->priv->tot_rows - 1; i >= 0; i--) {
		GthFileRow *row = file_store->priv->all_rows[i];

		if (row->visible) {
			GtkTreePath *path;

			path = gtk_tree_path_new ();
			gtk_tree_path_append_index (path, row->pos);
			gtk_tree_model_row_deleted (GTK_TREE_MODEL (file_store), path);
			gtk_tree_path_free (path);
		}
	}

	_gth_file_store_free_rows (file_store);
	_gth_file_store_increment_stamp (file_store);
}
