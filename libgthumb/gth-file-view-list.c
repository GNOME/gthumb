/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2003 Free Software Foundation, Inc.
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
#include <glib.h>
#include <gtk/gtktreemodel.h>
#include <gtk/gtktreeview.h>
#include <gtk/gtktreeselection.h>
#include <gtk/gtkliststore.h>
#include <gnome.h>
#include "typedefs.h"
#include "gth-file-view.h"
#include "gth-file-view-list.h"
#include "gthumb-marshal.h"
#include "gthumb-enum-types.h"
#include "file-data.h"
#include "icons/pixbufs.h"
#include "pixbuf-utils.h"


enum {
	COLUMN_FILE_DATA,
	COLUMN_ICON,
	COLUMN_NAME,
	COLUMN_PATH,
	COLUMN_SIZE,
	COLUMN_TIME,
	COLUMN_COMMENT,
	NUMBER_OF_COLUMNS
};


static GtkTargetEntry target_table[] = {
	{ "text/uri-list", 0, 0 } 
};


struct _GthFileViewListPrivate {
	GtkTreeView    *tree_view;
	GtkListStore   *list_store;
	GthSortMethod   sort_method;
	GtkSortType     sort_type;
	GthViewMode     view_mode;
	int             max_image_size;
	GnomeIconTheme *icon_theme;
	gboolean        enable_thumbs;

	GdkPixbuf      *unknown_pixbuf_small;
	GdkPixbuf      *unknown_pixbuf_big;
};


static GthFileViewClass *parent_class;


static void
gfv_set_hadjustment (GthFileView   *file_view,
		     GtkAdjustment *hadj)
{
	GthFileViewList *gfv_list = (GthFileViewList *) file_view;
	GtkTreeView     *tree_view = gfv_list->priv->tree_view;

	gtk_tree_view_set_hadjustment (tree_view, hadj);
}


static GtkAdjustment *
gfv_get_hadjustment (GthFileView   *file_view)
{
	GthFileViewList *gfv_list = (GthFileViewList *) file_view;
	GtkTreeView     *tree_view = gfv_list->priv->tree_view;

	return gtk_tree_view_get_hadjustment (tree_view);
}


static void
gfv_set_vadjustment (GthFileView   *file_view,
		     GtkAdjustment *vadj)
{
	GthFileViewList *gfv_list = (GthFileViewList *) file_view;
	GtkTreeView     *tree_view = gfv_list->priv->tree_view;

	gtk_tree_view_set_vadjustment (tree_view, vadj);
}


static GtkAdjustment *
gfv_get_vadjustment (GthFileView   *file_view)
{
	GthFileViewList *gfv_list = (GthFileViewList *) file_view;
	GtkTreeView     *tree_view = gfv_list->priv->tree_view;

	return gtk_tree_view_get_vadjustment (tree_view);
}


static GtkWidget *
gfv_get_widget (GthFileView   *file_view)
{
	GthFileViewList *gfv_list = (GthFileViewList *) file_view;

	return GTK_WIDGET (gfv_list->priv->tree_view);
}


/**/


static GdkPixbuf *
get_sized_pixbuf (GthFileViewList *gfv_list,
		  GdkPixbuf       *pixbuf)
{
	GdkPixbuf *result;
	int        x, y, w, h;

	if (gfv_list->priv->max_image_size == 0)
		return NULL;

	result = gdk_pixbuf_new (GDK_COLORSPACE_RGB, TRUE, 8, 
				 gfv_list->priv->max_image_size,
				 gfv_list->priv->max_image_size);
	gdk_pixbuf_fill (result, 0x00000000);

	if (pixbuf == NULL)
		return result;

	w = gdk_pixbuf_get_width (pixbuf);
	h = gdk_pixbuf_get_height (pixbuf);
	x = (gfv_list->priv->max_image_size - w) / 2; 
	y = (gfv_list->priv->max_image_size - h) / 2; 

	gdk_pixbuf_copy_area (pixbuf,
			      0, 0,
			      w, h,
			      result,
			      x, y);

	return result;
}


static void
gfv_insert (GthFileView  *file_view,
	    int           pos, 
	    GdkPixbuf    *pixbuf,
	    const char   *text,
	    const char   *comment)
{
	GthFileViewList *gfv_list = (GthFileViewList *) file_view;
	GtkListStore    *list_store = gfv_list->priv->list_store;
	GtkTreeIter      iter;
	GdkPixbuf       *real_pixbuf;

	if (! gfv_list->priv->enable_thumbs)
		real_pixbuf = g_object_ref (gfv_list->priv->unknown_pixbuf_small);
	else if (pixbuf == NULL)
		real_pixbuf = get_sized_pixbuf (gfv_list, gfv_list->priv->unknown_pixbuf_big);
	else
		real_pixbuf = get_sized_pixbuf (gfv_list, pixbuf);

	gtk_list_store_insert (list_store, &iter, pos);
	gtk_list_store_set (list_store, &iter,
			    COLUMN_ICON, real_pixbuf,
			    COLUMN_NAME, text,
			    COLUMN_COMMENT, comment,
			    -1);

	if (real_pixbuf != NULL)
		g_object_unref (real_pixbuf);
}


static int
gfv_append (GthFileView  *file_view,
	    GdkPixbuf    *pixbuf,
	    const char   *text,
	    const char   *comment)
{
	GthFileViewList *gfv_list = (GthFileViewList *) file_view;
	GtkListStore    *list_store = gfv_list->priv->list_store;
	GtkTreeIter      iter;
	GdkPixbuf       *real_pixbuf;
	GtkTreePath     *path;
	int              pos;

	if (! gfv_list->priv->enable_thumbs)
		real_pixbuf = g_object_ref (gfv_list->priv->unknown_pixbuf_small);
	else if (pixbuf == NULL)
		real_pixbuf = get_sized_pixbuf (gfv_list, gfv_list->priv->unknown_pixbuf_big);
	else
		real_pixbuf = get_sized_pixbuf (gfv_list, pixbuf);

	gtk_list_store_append (list_store, &iter);
	gtk_list_store_set (list_store, &iter,
			    COLUMN_ICON, real_pixbuf,
			    COLUMN_NAME, text,
			    COLUMN_COMMENT, comment,
			    -1);

	if (real_pixbuf != NULL)
		g_object_unref (real_pixbuf);

	path = gtk_tree_model_get_path (GTK_TREE_MODEL (gfv_list->priv->list_store), &iter);
	pos = gtk_tree_path_get_indices (path)[0];
	gtk_tree_path_free (path);

	return 	pos;
}


static int
gfv_append_with_data (GthFileView  *file_view,
		      GdkPixbuf    *pixbuf,
		      const char   *text,
		      const char   *comment,
		      gpointer      data)
{
	GthFileViewList *gfv_list = (GthFileViewList *) file_view;
	GtkListStore    *list_store = gfv_list->priv->list_store;
	GtkTreeIter      iter;
	GdkPixbuf       *real_pixbuf;
	GtkTreePath     *path;
	int              pos;

	if (! gfv_list->priv->enable_thumbs)
		real_pixbuf = g_object_ref (gfv_list->priv->unknown_pixbuf_small);
	else if (pixbuf == NULL)
		real_pixbuf = get_sized_pixbuf (gfv_list, gfv_list->priv->unknown_pixbuf_big);
	else
		real_pixbuf = get_sized_pixbuf (gfv_list, pixbuf);

	gtk_list_store_append (list_store, &iter);
	gtk_list_store_set (list_store, &iter,
			    COLUMN_ICON, real_pixbuf,
			    COLUMN_NAME, text,
			    COLUMN_COMMENT, comment,
			    COLUMN_FILE_DATA, data,
			    -1);

	if (real_pixbuf != NULL)
		g_object_unref (real_pixbuf);

	path = gtk_tree_model_get_path (GTK_TREE_MODEL (gfv_list->priv->list_store), &iter);
	pos = gtk_tree_path_get_indices (path)[0];
	gtk_tree_path_free (path);

	return 	pos;
}


static void
gfv_remove (GthFileView  *file_view, 
	    int           pos)
{
	GthFileViewList *gfv_list = (GthFileViewList *) file_view;
	GtkTreeIter      iter;
	GtkTreePath     *path;

	path = gtk_tree_path_new_from_indices (pos, -1);
	if (! gtk_tree_model_get_iter (GTK_TREE_MODEL (gfv_list->priv->list_store),
				       &iter,
				       path)) {
		gtk_tree_path_free (path);
		return;
	}
	gtk_tree_path_free (path);

	gtk_list_store_remove (gfv_list->priv->list_store, &iter);
}


static void
gfv_clear (GthFileView  *file_view)
{
	GthFileViewList *gfv_list = (GthFileViewList *) file_view;

	gtk_list_store_clear (gfv_list->priv->list_store);

	if (GTK_WIDGET_REALIZED (gfv_list->priv->tree_view))
                gtk_tree_view_scroll_to_point (GTK_TREE_VIEW (gfv_list->priv->tree_view), 0, 0);
}


static void
gfv_set_image_pixbuf (GthFileView  *file_view,
		      int           pos,
		      GdkPixbuf    *pixbuf)
{
	GthFileViewList *gfv_list = (GthFileViewList *) file_view;
	GtkTreeIter      iter;
	GtkTreePath     *path;

	path = gtk_tree_path_new_from_indices (pos, -1);
	if (! gtk_tree_model_get_iter (GTK_TREE_MODEL (gfv_list->priv->list_store),
				       &iter,
				       path)) {
		gtk_tree_path_free (path);
		return;
	}
	gtk_tree_path_free (path);

	gtk_list_store_set (gfv_list->priv->list_store, &iter,
			    COLUMN_ICON, get_sized_pixbuf (gfv_list, pixbuf),
                            -1);
}


static void
gfv_set_unknown_pixbuf (GthFileView  *file_view,
			int           pos)
{
	GthFileViewList *gfv_list = (GthFileViewList *) file_view;
	GtkTreeIter      iter;
	GtkTreePath     *path;
	GdkPixbuf       *real_pixbuf;

	path = gtk_tree_path_new_from_indices (pos, -1);
	if (! gtk_tree_model_get_iter (GTK_TREE_MODEL (gfv_list->priv->list_store),
				       &iter,
				       path)) {
		gtk_tree_path_free (path);
		return;
	}
	gtk_tree_path_free (path);

	if (! gfv_list->priv->enable_thumbs)
		real_pixbuf = g_object_ref (gfv_list->priv->unknown_pixbuf_small);
	else 
		real_pixbuf = get_sized_pixbuf (gfv_list, gfv_list->priv->unknown_pixbuf_big);

	gtk_list_store_set (gfv_list->priv->list_store, &iter,
			    COLUMN_ICON, real_pixbuf,
                            -1);

	g_object_unref (real_pixbuf);
}


static void
gfv_set_image_text (GthFileView  *file_view,
		    int           pos,
		    const char   *text)
{
	GthFileViewList *gfv_list = (GthFileViewList *) file_view;
	GtkTreeIter      iter;
	GtkTreePath     *path;

	path = gtk_tree_path_new_from_indices (pos, -1);
	if (! gtk_tree_model_get_iter (GTK_TREE_MODEL (gfv_list->priv->list_store),
				       &iter,
				       path)) {
		gtk_tree_path_free (path);
		return;
	}
	gtk_tree_path_free (path);

	gtk_list_store_set (gfv_list->priv->list_store, &iter,
			    COLUMN_NAME, text,
                            -1);
}


static const char*
gfv_get_image_text (GthFileView  *file_view,
		    int           pos)
{
	GthFileViewList *gfv_list = (GthFileViewList *) file_view;
	GtkTreeIter      iter;
	GtkTreePath     *path;
	char            *text = NULL;

	path = gtk_tree_path_new_from_indices (pos, -1);
	if (! gtk_tree_model_get_iter (GTK_TREE_MODEL (gfv_list->priv->list_store),
				       &iter,
				       path)) {
		gtk_tree_path_free (path);
		return NULL;
	}
	gtk_tree_path_free (path);

	gtk_tree_model_get (GTK_TREE_MODEL (gfv_list->priv->list_store), &iter,
                            COLUMN_NAME, &text,
                            -1);

	return text;
}


static void
gfv_set_image_comment (GthFileView  *file_view,
		       int           pos,
		       const char   *comment)
{
	GthFileViewList *gfv_list = (GthFileViewList *) file_view;
	GtkTreeIter      iter;
	GtkTreePath     *path;

	path = gtk_tree_path_new_from_indices (pos, -1);
	if (! gtk_tree_model_get_iter (GTK_TREE_MODEL (gfv_list->priv->list_store),
				       &iter,
				       path)) {
		gtk_tree_path_free (path);
		return;
	}
	gtk_tree_path_free (path);

	gtk_list_store_set (gfv_list->priv->list_store, &iter,
			    COLUMN_COMMENT, comment,
                            -1);
}


static const char*
gfv_get_image_comment (GthFileView  *file_view,
		       int           pos)
{
	GthFileViewList *gfv_list = (GthFileViewList *) file_view;
	GtkTreeIter      iter;
	GtkTreePath     *path;
	char            *comment = NULL;

	path = gtk_tree_path_new_from_indices (pos, -1);
	if (! gtk_tree_model_get_iter (GTK_TREE_MODEL (gfv_list->priv->list_store),
				       &iter,
				       path)) {
		gtk_tree_path_free (path);
		return NULL;
	}
	gtk_tree_path_free (path);

	gtk_tree_model_get (GTK_TREE_MODEL (gfv_list->priv->list_store), &iter,
			    COLUMN_COMMENT, &comment,
			    -1);

	return comment;
}


static int
gfv_get_images (GthFileView  *file_view)
{
	GthFileViewList *gfv_list = (GthFileViewList *) file_view;
	GtkTreeModel    *model = GTK_TREE_MODEL (gfv_list->priv->list_store);
	GtkTreeIter      iter;
	int              len = 0;

	if (! gtk_tree_model_get_iter_first (model, &iter))
		return len;
	
	do {
		len++;
	} while (gtk_tree_model_iter_next (model, &iter));

	return len;
}


static GList *
gfv_get_list (GthFileView  *file_view)
{
	GthFileViewList *gfv_list = (GthFileViewList *) file_view;
	GtkTreeModel    *model = GTK_TREE_MODEL (gfv_list->priv->list_store);
	GtkTreeIter      iter;
	GList           *list = NULL;

	if (! gtk_tree_model_get_iter_first (model, &iter))
		return NULL;
	
	do {
		gpointer iter_data;

		gtk_tree_model_get (model, &iter,
				    COLUMN_FILE_DATA, &iter_data,
				    -1);
		list = g_list_prepend (list, iter_data);
	} while (gtk_tree_model_iter_next (model, &iter));

	return g_list_reverse (list);
}


static GList *
gfv_get_selection (GthFileView  *file_view)
{
	GthFileViewList  *gfv_list = (GthFileViewList *) file_view;
	GtkTreeSelection *selection;
	GList            *sel_rows, *scan;
	GList            *list = NULL;

	selection = gtk_tree_view_get_selection (gfv_list->priv->tree_view);
	sel_rows = gtk_tree_selection_get_selected_rows (selection, NULL);

	if (sel_rows == NULL)
		return NULL;

	for (scan = sel_rows; scan; scan = scan->next) {
		GtkTreePath *path = scan->data;
		int          pos;

		pos = gtk_tree_path_get_indices (path)[0];
		list = g_list_prepend (list, gth_file_view_get_image_data (file_view, pos));
	}

	g_list_foreach (sel_rows, (GFunc) gtk_tree_path_free, NULL);
	g_list_free (sel_rows);

	return g_list_reverse (list);
}


/* Managing the selection */


static void
gfv_select_image (GthFileView     *file_view,
		  int              pos)
{
	GthFileViewList  *gfv_list = (GthFileViewList *) file_view;
	GtkTreeSelection *selection;
	GtkTreePath      *path;

	selection = gtk_tree_view_get_selection (gfv_list->priv->tree_view);
	path = gtk_tree_path_new_from_indices (pos, -1);
	gtk_tree_selection_select_path (selection, path);
	gtk_tree_path_free (path);
}


static void
gfv_unselect_image (GthFileView     *file_view,
		    int              pos)
{
	GthFileViewList  *gfv_list = (GthFileViewList *) file_view;
	GtkTreeSelection *selection;
	GtkTreePath      *path;

	selection = gtk_tree_view_get_selection (gfv_list->priv->tree_view);
	path = gtk_tree_path_new_from_indices (pos, -1);
	gtk_tree_selection_unselect_path (selection, path);
	gtk_tree_path_free (path);
}


static void
gfv_select_all (GthFileView     *file_view)
{
	GthFileViewList  *gfv_list = (GthFileViewList *) file_view;
	GtkTreeSelection *selection;

	selection = gtk_tree_view_get_selection (gfv_list->priv->tree_view);
	gtk_tree_selection_select_all (selection);
}


static void
gfv_unselect_all (GthFileView     *file_view)
{
	GthFileViewList  *gfv_list = (GthFileViewList *) file_view;
	GtkTreeSelection *selection;

	selection = gtk_tree_view_get_selection (gfv_list->priv->tree_view);
	gtk_tree_selection_unselect_all (selection);
}


static GList *
gfv_get_file_list_selection (GthFileView *file_view)
{
	GthFileViewList  *gfv_list = (GthFileViewList *) file_view;
	GtkTreeSelection *selection;
	GList            *sel_rows, *scan;
	GList            *list = NULL;

	selection = gtk_tree_view_get_selection (gfv_list->priv->tree_view);
	sel_rows = gtk_tree_selection_get_selected_rows (selection, NULL);

	if (sel_rows == NULL)
		return NULL;

	for (scan = sel_rows; scan; scan = scan->next) {
		GtkTreePath *path = scan->data;
		int          pos;
		FileData    *fd;

		pos = gtk_tree_path_get_indices (path)[0];
		fd = gth_file_view_get_image_data (file_view, pos);
		if ((fd != NULL) && (fd->path != NULL))
			list = g_list_prepend (list, g_strdup (fd->path));
		file_data_unref (fd);
	}

	g_list_foreach (sel_rows, (GFunc) gtk_tree_path_free, NULL);
	g_list_free (sel_rows);

	return g_list_reverse (list);
}


static gboolean
gfv_pos_is_selected (GthFileView     *file_view,
		     int              pos)
{
	GthFileViewList  *gfv_list = (GthFileViewList *) file_view;
	GtkTreeSelection *selection;
	GtkTreeIter       iter;
	GtkTreePath      *path;

	selection = gtk_tree_view_get_selection (gfv_list->priv->tree_view);

	path = gtk_tree_path_new_from_indices (pos, -1);
	if (! gtk_tree_model_get_iter (GTK_TREE_MODEL (gfv_list->priv->list_store),
				       &iter,
				       path)) {
		gtk_tree_path_free (path);
		return FALSE;
	}
	gtk_tree_path_free (path);

	return gtk_tree_selection_iter_is_selected (selection, &iter);
}


static gboolean
gfv_only_one_is_selected (GthFileView    *file_view)
{
	GthFileViewList  *gfv_list = (GthFileViewList *) file_view;
	GtkTreeSelection *selection;
	GList            *sel_rows;
	gboolean          ret_val;

	selection = gtk_tree_view_get_selection (gfv_list->priv->tree_view);
	sel_rows = gtk_tree_selection_get_selected_rows (selection, NULL);

	ret_val = ((sel_rows != NULL) && (sel_rows->next == NULL));

	g_list_foreach (sel_rows, (GFunc) gtk_tree_path_free, NULL);
	g_list_free (sel_rows);

	return ret_val;
}


static gboolean
gfv_selection_not_null (GthFileView    *file_view)
{
	GthFileViewList  *gfv_list = (GthFileViewList *) file_view;
	GtkTreeSelection *selection;
	GList            *sel_rows;
	gboolean          ret_val;

	selection = gtk_tree_view_get_selection (gfv_list->priv->tree_view);
	sel_rows = gtk_tree_selection_get_selected_rows (selection, NULL);

	ret_val = (sel_rows != NULL);

	g_list_foreach (sel_rows, (GFunc) gtk_tree_path_free, NULL);
	g_list_free (sel_rows);

	return ret_val;
}


static int
gfv_get_first_selected (GthFileView *file_view)
{
	GthFileViewList  *gfv_list = (GthFileViewList *) file_view;
	GtkTreeSelection *selection;
	GList            *sel_rows, *scan;
	int               min = -1;

	selection = gtk_tree_view_get_selection (gfv_list->priv->tree_view);
	sel_rows = gtk_tree_selection_get_selected_rows (selection, NULL);

	if (sel_rows == NULL)
		return -1;

	for (scan = sel_rows; scan; scan = scan->next) {
		GtkTreePath *path = scan->data;
		int          pos;

		pos = gtk_tree_path_get_indices (path)[0];
		if (min == -1)
			min = pos;
		else
			min = MIN (min, pos);
	}

	g_list_foreach (sel_rows, (GFunc) gtk_tree_path_free, NULL);
	g_list_free (sel_rows);

	return min;
}


static int
gfv_get_last_selected (GthFileView *file_view)
{
	GthFileViewList  *gfv_list = (GthFileViewList *) file_view;
	GtkTreeSelection *selection;
	GList            *sel_rows, *scan;
	int               max = -1;

	selection = gtk_tree_view_get_selection (gfv_list->priv->tree_view);
	sel_rows = gtk_tree_selection_get_selected_rows (selection, NULL);

	if (sel_rows == NULL)
		return -1;

	for (scan = sel_rows; scan; scan = scan->next) {
		GtkTreePath *path = scan->data;
		int          pos;

		pos = gtk_tree_path_get_indices (path)[0];
		if (max == -1)
			max = pos;
		else
			max = MAX (max, pos);
	}

	g_list_foreach (sel_rows, (GFunc) gtk_tree_path_free, NULL);
	g_list_free (sel_rows);

	return max;
}


/* Setting spacing values */


static void
gfv_set_image_width (GthFileView     *file_view,
		     int              width)
{
	GthFileViewList  *gfv_list = (GthFileViewList *) file_view;
	gfv_list->priv->max_image_size = width;
}


/* Attaching information to the items */


static void
gfv_set_image_data (GthFileView     *file_view,
		    int              pos, 
		    gpointer         data)
{
	GthFileViewList *gfv_list = (GthFileViewList *) file_view;
	GtkTreeIter      iter;
	GtkTreePath     *path;

	path = gtk_tree_path_new_from_indices (pos, -1);
	if (! gtk_tree_model_get_iter (GTK_TREE_MODEL (gfv_list->priv->list_store),
				       &iter,
				       path)) {
		gtk_tree_path_free (path);
		return;
	}
	gtk_tree_path_free (path);

	gtk_list_store_set (gfv_list->priv->list_store, &iter,
			    COLUMN_FILE_DATA, data,
                            -1);
}


static void
gfv_set_image_data_full (GthFileView     *file_view,
			 int              pos, 
			 gpointer         data,
			 GtkDestroyNotify destroy)
{
	/* FIXME: not implemented yet */
	gth_file_view_set_image_data (file_view, pos, data);
}


static int
gfv_find_image_from_data (GthFileView     *file_view,
			  gpointer         data)
{
	GthFileViewList *gfv_list = (GthFileViewList *) file_view;
	GtkTreeModel    *model = GTK_TREE_MODEL (gfv_list->priv->list_store);
	GtkTreeIter      iter;
	int              pos = 0;

	if (! gtk_tree_model_get_iter_first (model, &iter))
		return -1;
	
	do {
		gpointer iter_data;

		gtk_tree_model_get (model, &iter,
				    COLUMN_FILE_DATA, &iter_data,
				    -1);
		if (data == iter_data)
			return pos;

		pos++;
	} while (gtk_tree_model_iter_next (model, &iter));

	return -1;
}


static gpointer
gfv_get_image_data (GthFileView     *file_view,
		    int              pos)
{
	GthFileViewList *gfv_list = (GthFileViewList *) file_view;
	GtkTreeIter      iter;
	GtkTreePath     *path;
	FileData        *fdata;

	path = gtk_tree_path_new_from_indices (pos, -1);
	if (! gtk_tree_model_get_iter (GTK_TREE_MODEL (gfv_list->priv->list_store),
				       &iter,
				       path)) {
		gtk_tree_path_free (path);
		return NULL;
	}
	gtk_tree_path_free (path);

	gtk_tree_model_get (GTK_TREE_MODEL (gfv_list->priv->list_store), &iter,
                            COLUMN_FILE_DATA, &fdata,
                            -1);
	file_data_ref (fdata);

	return fdata;
}


/* Visibility */


static void
gfv_enable_thumbs (GthFileView *file_view,
		   gboolean     enable_thumbs)
{
	GthFileViewList *gfv_list = (GthFileViewList *) file_view;
	gfv_list->priv->enable_thumbs = enable_thumbs;
}


static void
gfv_set_view_mode (GthFileView *file_view,
		   GthViewMode  mode)
{
	GthFileViewList   *gfv_list = (GthFileViewList *) file_view;
	GtkTreeViewColumn *column;

	gfv_list->priv->view_mode = mode;

	column = gtk_tree_view_get_column (gfv_list->priv->tree_view, 1);

	/* FIXME
        gtk_tree_view_column_set_visible (column, mode == GTH_VIEW_MODE_ALL);
	*/

        gtk_tree_view_column_set_visible (column, FALSE);
}


static GthViewMode
gfv_get_view_mode (GthFileView *file_view)
{
	GthFileViewList *gfv_list = (GthFileViewList *) file_view;
	return gfv_list->priv->view_mode;
}


static void
gfv_moveto (GthFileView *file_view,
	    int          pos, 
	    double       yalign)
{
	GthFileViewList *gfv_list = (GthFileViewList *) file_view;
	GtkTreePath     *path;

	path = gtk_tree_path_new_from_indices (pos, -1);
	gtk_tree_view_scroll_to_cell (gfv_list->priv->tree_view,
				      path,
				      NULL,
				      FALSE,
				      0.0,
				      0.0);
	gtk_tree_path_free (path);
}


static GthVisibility
gfv_image_is_visible (GthFileView *file_view,
		      int          pos)
{
	GthFileViewList *gfv_list = (GthFileViewList *) file_view;
	GdkRectangle     visible_rect, image_rect;
	GtkTreePath     *path;
	GthVisibility    visibility;

	gtk_tree_view_get_visible_rect (gfv_list->priv->tree_view,
					&visible_rect);

	path = gtk_tree_path_new_from_indices (pos, -1);
	gtk_tree_view_get_cell_area (gfv_list->priv->tree_view,
				     path,
				     NULL,
				     &image_rect);
	gtk_tree_path_free (path);

	if ((image_rect.y + image_rect.height < visible_rect.y)
	    || (image_rect.y > visible_rect.y + visible_rect.height))
		visibility = GTH_VISIBILITY_NONE;

	else if (image_rect.y < visible_rect.y)
		visibility = GTH_VISIBILITY_PARTIAL_TOP;
	
	else if (image_rect.y + image_rect.height > visible_rect.y + visible_rect.height)
		visibility = GTH_VISIBILITY_PARTIAL_BOTTOM;
	
	else
		visibility = GTH_VISIBILITY_FULL;
	
	return visibility;
}


static int
gfv_get_image_at (GthFileView *file_view, 
		  int          x, 
		  int          y)
{
	GthFileViewList *gfv_list = (GthFileViewList *) file_view;
	GtkTreePath     *path;
	int              pos;

	if (! gtk_tree_view_get_path_at_pos (gfv_list->priv->tree_view,
					     x,
					     y,
					     &path,
					     NULL,
					     NULL,
					     NULL))
		return -1;

	pos = gtk_tree_path_get_indices (path)[0];
	gtk_tree_path_free (path);

	return pos;
}


static int
gfv_get_first_visible (GthFileView *file_view)
{
	GthFileViewList *gfv_list = (GthFileViewList *) file_view;
	GdkRectangle     rect;
	GtkTreePath     *path;
	int              pos;

	gtk_tree_view_get_visible_rect (gfv_list->priv->tree_view,
					&rect);

	if (! gtk_tree_view_get_path_at_pos (gfv_list->priv->tree_view,
					     0, 
					     0,
					     &path,
					     NULL,
					     NULL,
					     NULL))
		return -1;

	pos = gtk_tree_path_get_indices (path)[0];
	gtk_tree_path_free (path);

	return pos;
}


static int
gfv_get_last_visible (GthFileView *file_view)
{
	GthFileViewList *gfv_list = (GthFileViewList *) file_view;
	GdkRectangle     rect;
	GtkTreePath     *path;
	int              pos;

	gtk_tree_view_get_visible_rect (gfv_list->priv->tree_view,
					&rect);
	if (! gtk_tree_view_get_path_at_pos (gfv_list->priv->tree_view,
					     0, 
					     rect.height - 1,
					     &path,
					     NULL,
					     NULL,
					     NULL))
		return gth_file_view_get_images (file_view) - 1;

	pos = gtk_tree_path_get_indices (path)[0];
	gtk_tree_path_free (path);

	return pos;
}


static void
gfv_sorted (GthFileView   *file_view,
	    GthSortMethod  sort_method,
	    GtkSortType    sort_type)
{
	GthFileViewList *gfv_list = (GthFileViewList *) file_view;
	int              col;
	GtkAdjustment   *adj;

	gfv_list->priv->sort_method = sort_method;
	gfv_list->priv->sort_type = sort_type;

	switch (sort_method) {
	case GTH_SORT_METHOD_BY_NAME: col = COLUMN_NAME; break;
	case GTH_SORT_METHOD_BY_PATH: col = COLUMN_PATH; break;
	case GTH_SORT_METHOD_BY_SIZE: col = COLUMN_SIZE; break;
	case GTH_SORT_METHOD_BY_TIME: col = COLUMN_TIME; break;
	default: col = -1; break;
	}

	gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (gfv_list->priv->list_store),
					      col,
					      sort_type);

	adj = gtk_tree_view_get_vadjustment (gfv_list->priv->tree_view);
	gtk_adjustment_changed (adj);
}


static void
gfv_unsorted (GthFileView *file_view)
{
	/* FIXME */
}


/* Misc */


static void
gfv_image_activated (GthFileView *file_view, 
		     int          pos)
{
	GthFileViewList *gfv_list = (GthFileViewList *) file_view;
	GtkTreePath     *path;

	path = gtk_tree_path_new_from_indices (pos, -1);
	gtk_tree_view_row_activated (gfv_list->priv->tree_view, path, NULL);
	gtk_tree_path_free (path);
}


static void
gfv_set_cursor (GthFileView *file_view, 
		int          pos)
{
	GthFileViewList *gfv_list = (GthFileViewList *) file_view;
	GtkTreePath     *path;

	path = gtk_tree_path_new_from_indices (pos, -1);
	gtk_tree_view_set_cursor (gfv_list->priv->tree_view, path, NULL, FALSE);
	gtk_tree_path_free (path);
}


static int
gfv_get_cursor (GthFileView *file_view)
{
	GthFileViewList *gfv_list = (GthFileViewList *) file_view;
	GtkTreePath     *path;
	int              pos;

	gtk_tree_view_get_cursor (gfv_list->priv->tree_view, &path, NULL);
	pos = gtk_tree_path_get_indices (path)[0];
	gtk_tree_path_free (path);

	return pos;
}


static GdkPixbuf *
create_unknown_pixbuf (GthFileViewList *gfv_list, gboolean big)
{
	int         icon_width, icon_height, icon_size;
	char       *icon_name;
	char       *icon_path;
	GdkPixbuf  *pixbuf = NULL;
	int         width, height;

	gtk_icon_size_lookup_for_settings (gtk_widget_get_settings (GTK_WIDGET (gfv_list->priv->tree_view)),
                                           (big ? GTK_ICON_SIZE_DIALOG: GTK_ICON_SIZE_LARGE_TOOLBAR),
                                           &icon_width, &icon_height);
	icon_size = MAX (icon_width, icon_height);

	icon_name = gnome_icon_lookup (gfv_list->priv->icon_theme,
				       NULL,
				       NULL,
				       NULL,
				       NULL,
				       "image/*",
				       GNOME_ICON_LOOKUP_FLAGS_NONE,
				       NULL);
	icon_path = gnome_icon_theme_lookup_icon (gfv_list->priv->icon_theme,
						  icon_name,
						  icon_size,
						  NULL,
						  NULL);
	g_free (icon_name);

	if (icon_path == NULL) 
		pixbuf = gdk_pixbuf_new_from_inline (-1, 
						     dir_16_rgba, 
						     FALSE, 
						     NULL);
	else {
		pixbuf = gdk_pixbuf_new_from_file (icon_path, NULL);
		g_free (icon_path);
	}

	width = gdk_pixbuf_get_width (pixbuf);
	height = gdk_pixbuf_get_height (pixbuf);
	if (scale_keepping_ratio (&width, &height, icon_size, icon_size)) {
		GdkPixbuf *scaled;
		scaled = gdk_pixbuf_scale_simple (pixbuf,
						  width,
						  height,
						  GDK_INTERP_BILINEAR);
		g_object_unref (pixbuf);
		pixbuf = scaled;
	}

	return pixbuf;
}


static void
gfv_update_icon_theme (GthFileView *file_view)
{
	GthFileViewList *gfv_list = (GthFileViewList *) file_view;

	if (gfv_list->priv->unknown_pixbuf_small != NULL) 
		g_object_unref (gfv_list->priv->unknown_pixbuf_small);

	if (gfv_list->priv->unknown_pixbuf_big != NULL) 
		g_object_unref (gfv_list->priv->unknown_pixbuf_big);

	gfv_list->priv->unknown_pixbuf_small = create_unknown_pixbuf (gfv_list, FALSE);
	gfv_list->priv->unknown_pixbuf_big = create_unknown_pixbuf (gfv_list, TRUE);
}


/* Interactive search */


static void
gfv_set_enable_search (GthFileView *file_view,
		       gboolean     enable_search)
{
	GthFileViewList *gfv_list = (GthFileViewList *) file_view;
	GtkTreeView     *tree_view = gfv_list->priv->tree_view;

	gtk_tree_view_set_enable_search (tree_view, enable_search);
}


static gboolean
gfv_get_enable_search (GthFileView *file_view)
{
	GthFileViewList *gfv_list = (GthFileViewList *) file_view;
	GtkTreeView     *tree_view = gfv_list->priv->tree_view;

	return gtk_tree_view_get_enable_search (tree_view);
}


/**/


static void
gth_file_view_list_finalize (GObject *object)
{
  	GthFileViewList *gfv_list;
	g_return_if_fail (GTH_IS_FILE_VIEW_LIST (object));

	gfv_list = (GthFileViewList*) object;

	g_object_unref (gfv_list->priv->icon_theme);
	g_object_unref (gfv_list->priv->unknown_pixbuf_small);
	g_object_unref (gfv_list->priv->unknown_pixbuf_big);

	g_free (gfv_list->priv);

        /* Chain up */
	G_OBJECT_CLASS (parent_class)->finalize (object);
}


static void
gth_file_view_list_class_init (GthFileViewListClass *file_view_list_class)
{
	GObjectClass     *gobject_class;
	GthFileViewClass *file_view_class;

	parent_class = g_type_class_peek_parent (file_view_list_class);
	gobject_class = (GObjectClass*) file_view_list_class;
	file_view_class = (GthFileViewClass*) file_view_list_class;

	/* Methods */

	gobject_class->finalize = gth_file_view_list_finalize;

	file_view_class->set_hadjustment      = gfv_set_hadjustment;
	file_view_class->get_hadjustment      = gfv_get_hadjustment;
	file_view_class->set_vadjustment      = gfv_set_vadjustment;
	file_view_class->get_vadjustment      = gfv_get_vadjustment;
	file_view_class->get_widget           = gfv_get_widget;
	file_view_class->insert               = gfv_insert;
	file_view_class->append               = gfv_append;
	file_view_class->append_with_data     = gfv_append_with_data;
	file_view_class->remove               = gfv_remove;
	file_view_class->clear                = gfv_clear;
	file_view_class->set_image_pixbuf     = gfv_set_image_pixbuf;
	file_view_class->set_unknown_pixbuf   = gfv_set_unknown_pixbuf;
	file_view_class->set_image_text       = gfv_set_image_text;
	file_view_class->get_image_text       = gfv_get_image_text;
	file_view_class->set_image_comment    = gfv_set_image_comment;
	file_view_class->get_image_comment    = gfv_get_image_comment;
	file_view_class->get_images           = gfv_get_images;
	file_view_class->get_list             = gfv_get_list;
	file_view_class->get_selection        = gfv_get_selection;
	file_view_class->select_image         = gfv_select_image;
	file_view_class->unselect_image       = gfv_unselect_image;
	file_view_class->select_all           = gfv_select_all;
	file_view_class->unselect_all         = gfv_unselect_all;
	file_view_class->get_file_list_selection = gfv_get_file_list_selection;
	file_view_class->pos_is_selected      = gfv_pos_is_selected;
	file_view_class->only_one_is_selected = gfv_only_one_is_selected;
	file_view_class->selection_not_null   = gfv_selection_not_null;
	file_view_class->get_first_selected   = gfv_get_first_selected;
	file_view_class->get_last_selected    = gfv_get_last_selected;
	file_view_class->set_image_width      = gfv_set_image_width;
	file_view_class->set_image_data       = gfv_set_image_data;
	file_view_class->set_image_data_full  = gfv_set_image_data_full;
	file_view_class->find_image_from_data = gfv_find_image_from_data;
	file_view_class->get_image_data       = gfv_get_image_data;
	file_view_class->enable_thumbs        = gfv_enable_thumbs;
	file_view_class->set_view_mode        = gfv_set_view_mode;
	file_view_class->get_view_mode        = gfv_get_view_mode;
	file_view_class->moveto               = gfv_moveto;
	file_view_class->image_is_visible     = gfv_image_is_visible;
	file_view_class->get_image_at         = gfv_get_image_at;
	file_view_class->get_first_visible    = gfv_get_first_visible;
	file_view_class->get_last_visible     = gfv_get_last_visible;
	file_view_class->sorted               = gfv_sorted;
	file_view_class->unsorted             = gfv_unsorted;
	file_view_class->image_activated      = gfv_image_activated;
	file_view_class->set_cursor           = gfv_set_cursor;
	file_view_class->get_cursor           = gfv_get_cursor;
	file_view_class->update_icon_theme    = gfv_update_icon_theme;
	file_view_class->set_enable_search    = gfv_set_enable_search;
	file_view_class->get_enable_search    = gfv_get_enable_search;
}


static void
gth_file_view_list_init (GthFileViewList *gfv_list)
{
	GthFileViewListPrivate *priv;

	priv = g_new0 (GthFileViewListPrivate, 1);
	gfv_list->priv = priv;

	priv->sort_method = GTH_SORT_METHOD_NONE;
	priv->sort_type   = GTK_SORT_ASCENDING;
}


GType
gth_file_view_list_get_type (void)
{
	static guint type = 0;

	if (! type) {
		GTypeInfo type_info = {
			sizeof (GthFileViewListClass),
			NULL,
			NULL,
			(GClassInitFunc) gth_file_view_list_class_init,
			NULL,
			NULL,
			sizeof (GthFileViewList),
			0,
			(GInstanceInitFunc) gth_file_view_list_init
                };

		type = g_type_register_static (GTH_TYPE_FILE_VIEW,
					       "GthFileViewList",
					       &type_info,
					       0);
        }

	return type;
}


static void 
selection_changed_cb (GtkTreeSelection *selection,
		      GthFileViewList  *gfv_list)
{
	gth_file_view_selection_changed (GTH_FILE_VIEW (gfv_list));
}


static void
row_activated_cb (GtkTreeView       *tree_view,
		  GtkTreePath       *path,
		  GtkTreeViewColumn *column,
		  GthFileViewList   *gfv_list)
{
	int pos;

	pos = gtk_tree_path_get_indices (path)[0];
	gth_file_view_item_activated (GTH_FILE_VIEW (gfv_list), pos);
}


static void
cursor_changed_cb (GtkTreeView     *tree_view,
		   GthFileViewList *gfv_list)
{
	GtkTreePath *path;
	int          pos;

	gtk_tree_view_get_cursor (gfv_list->priv->tree_view, &path, NULL);
	pos = gtk_tree_path_get_indices (path)[0];
	gtk_tree_path_free (path);
	gth_file_view_cursor_changed (GTH_FILE_VIEW (gfv_list), pos);
}


static void
add_columns (GtkTreeView *treeview)
{
	static char       *titles[] = {N_("Comment")};
	GtkCellRenderer   *renderer;
	GtkTreeViewColumn *column;
	int                i, j;

	/* The Name column. */
	column = gtk_tree_view_column_new ();
	gtk_tree_view_column_set_title (column, _("Name"));

	renderer = gtk_cell_renderer_pixbuf_new ();
	gtk_tree_view_column_pack_start (column, renderer, FALSE);
	gtk_tree_view_column_set_attributes (column, renderer,
                                             "pixbuf", COLUMN_ICON,
                                             NULL);

	renderer = gtk_cell_renderer_text_new ();
	gtk_tree_view_column_pack_start (column,
					 renderer,
					 TRUE);
	gtk_tree_view_column_set_attributes (column, renderer,
                                             "text", COLUMN_NAME,
                                             NULL);

	gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
	/*
	gtk_tree_view_column_set_sort_column_id (column, COLUMN_NAME);
	gtk_tree_view_column_set_sort_column_id (column, COLUMN_PATH);
	gtk_tree_view_column_set_sort_column_id (column, COLUMN_SIZE);
	gtk_tree_view_column_set_sort_column_id (column, COLUMN_TIME);
	*/
	gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);

	/* Other columns */
	for (j = 0, i = COLUMN_COMMENT; i < NUMBER_OF_COLUMNS; i++, j++) {
		renderer = gtk_cell_renderer_text_new ();
		column = gtk_tree_view_column_new_with_attributes (_(titles[j]),
								   renderer,
								   "text", i,
								   NULL);

		gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
		/*gtk_tree_view_column_set_sort_column_id (column, i);*/

		gtk_tree_view_append_column (treeview, column);
	}
}


/* Sort */


static int
comp_func_name (gconstpointer  ptr1,
		gconstpointer  ptr2)
{
	const FileData *fd1 = ptr1, *fd2 = ptr2;

	if ((fd1 == NULL) || (fd2 == NULL))
		return 0;

	return strcasecmp (fd1->name, fd2->name);
}


static int
comp_func_size (gconstpointer  ptr1,
		gconstpointer  ptr2)
{
	const FileData *fd1 = ptr1, *fd2 = ptr2;

	if ((fd1 == NULL) || (fd2 == NULL))
		return 0;

	if (fd1->size < fd2->size) return -1;
	if (fd1->size > fd2->size) return 1;

	/* if the size is the same order by name. */
	return comp_func_name (ptr1, ptr2);
}


static int
comp_func_time (gconstpointer  ptr1,
		gconstpointer  ptr2)
{
	const FileData *fd1 = ptr1, *fd2 = ptr2;

	if ((fd1 == NULL) || (fd2 == NULL))
		return 0;

	if (fd1->mtime < fd2->mtime) return -1;
	if (fd1->mtime > fd2->mtime) return 1;

	/* if time is the same order by name. */
	return comp_func_name (ptr1, ptr2);
}


static int
comp_func_path (gconstpointer  ptr1,
		gconstpointer  ptr2)
{
	const FileData *fd1 = ptr1, *fd2 = ptr2;

	if ((fd1 == NULL) || (fd2 == NULL))
		return 0;

	return strcmp (fd1->path, fd2->path);
}


static int
comp_func_none (gconstpointer  ptr1,
		gconstpointer  ptr2)
{
	return 0;
}


static GCompareFunc 
get_compfunc_from_method (GthSortMethod sort_method)
{
	GCompareFunc func;

	switch (sort_method) {
	case GTH_SORT_METHOD_BY_NAME:
		func = comp_func_name;
		break;
	case GTH_SORT_METHOD_BY_TIME:
		func = comp_func_time;
		break;
	case GTH_SORT_METHOD_BY_SIZE:
		func = comp_func_size;
		break;
	case GTH_SORT_METHOD_BY_PATH:
		func = comp_func_path;
		break;
	case GTH_SORT_METHOD_NONE:
	default:
		func = comp_func_none;
		break;
	}

	return func;
}


static int
default_sort_func (GtkTreeModel *model, 
		   GtkTreeIter  *a, 
		   GtkTreeIter  *b, 
		   gpointer      user_data)
{
	GthFileViewList *gfv_list = user_data;
	FileData        *fdata1, *fdata2;
	GCompareFunc     cmp_func;
	int              ret_val;

        gtk_tree_model_get (model, a, COLUMN_FILE_DATA, &fdata1, -1);
        gtk_tree_model_get (model, b, COLUMN_FILE_DATA, &fdata2, -1);

	g_return_val_if_fail (fdata1 != NULL, 0);
	g_return_val_if_fail (fdata2 != NULL, 0);

	cmp_func = get_compfunc_from_method (gfv_list->priv->sort_method);
	ret_val = (*cmp_func) (fdata1, fdata2);

	return ret_val;
}


GthFileView *
gth_file_view_list_new (guint image_width)
{
	GthFileViewList        *gfv_list;
	GthFileViewListPrivate *priv;
	GtkTreeSelection       *selection;

	gfv_list = GTH_FILE_VIEW_LIST (g_object_new (GTH_TYPE_FILE_VIEW_LIST, NULL));
	priv = gfv_list->priv;

	priv->list_store = gtk_list_store_new (NUMBER_OF_COLUMNS, 
					       G_TYPE_POINTER,
					       GDK_TYPE_PIXBUF,
					       G_TYPE_STRING,
					       G_TYPE_STRING,
					       G_TYPE_STRING,
					       G_TYPE_STRING,
					       G_TYPE_STRING);

	priv->tree_view = (GtkTreeView*) gtk_tree_view_new_with_model (GTK_TREE_MODEL (priv->list_store));
	g_object_unref (priv->list_store);

	gtk_tree_view_set_rules_hint (priv->tree_view, FALSE);
	gtk_tree_view_set_headers_visible (priv->tree_view, FALSE);

	add_columns (priv->tree_view);

	gtk_tree_view_set_enable_search (priv->tree_view, TRUE);
	gtk_tree_view_set_search_column (priv->tree_view, COLUMN_NAME);

	gtk_tree_sortable_set_default_sort_func (GTK_TREE_SORTABLE (priv->list_store),
						 default_sort_func,
						 gfv_list, NULL);

	gtk_tree_sortable_set_sort_func (GTK_TREE_SORTABLE (priv->list_store),
                                         COLUMN_NAME, default_sort_func,
                                         gfv_list, NULL);
	gtk_tree_sortable_set_sort_func (GTK_TREE_SORTABLE (priv->list_store),
                                         COLUMN_PATH, default_sort_func,
                                         gfv_list, NULL);
	gtk_tree_sortable_set_sort_func (GTK_TREE_SORTABLE (priv->list_store),
                                         COLUMN_SIZE, default_sort_func,
                                         gfv_list, NULL);
	gtk_tree_sortable_set_sort_func (GTK_TREE_SORTABLE (priv->list_store),
                                         COLUMN_TIME, default_sort_func,
                                         gfv_list, NULL);

	selection = gtk_tree_view_get_selection (priv->tree_view);
	gtk_tree_selection_set_mode (selection, GTK_SELECTION_MULTIPLE);

	g_signal_connect (G_OBJECT (selection),
			  "changed",
			  G_CALLBACK (selection_changed_cb),
			  gfv_list);
	g_signal_connect (G_OBJECT (priv->tree_view),
			  "row_activated",
			  G_CALLBACK (row_activated_cb),
			  gfv_list);
	g_signal_connect (G_OBJECT (priv->tree_view),
			  "cursor_changed",
			  G_CALLBACK (cursor_changed_cb),
			  gfv_list);

	gtk_drag_source_set (GTK_WIDGET (priv->tree_view),
			     GDK_BUTTON1_MASK,
			     target_table, G_N_ELEMENTS (target_table),
			     GDK_ACTION_MOVE | GDK_ACTION_COPY);

	/**/

	priv->icon_theme = gnome_icon_theme_new ();
	gnome_icon_theme_set_allow_svg (priv->icon_theme, TRUE);
	priv->unknown_pixbuf_small = create_unknown_pixbuf (gfv_list, FALSE);
	priv->unknown_pixbuf_big = create_unknown_pixbuf (gfv_list, TRUE);

	return GTH_FILE_VIEW (gfv_list);
}
