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

#ifndef CATALOG_LIST_H
#define CATALOG_LIST_H

#include <gtk/gtkwidget.h>
#include <gtk/gtkliststore.h>
#include <gtk/gtktreeview.h>

typedef struct {
	gchar        * path;            /* The current dir to show. */
	GtkListStore * list_store;
	GtkWidget    * list_view;       /* The widget that shows the list. */
	GtkWidget    * root_widget;     /* The widget that contains all. */
	GtkCellRenderer  *text_renderer;
	gboolean       use_underline;
} CatalogList;


CatalogList *  catalog_list_new                      (gboolean use_underline);

void           catalog_list_free                     (CatalogList *cat_list);

gboolean       catalog_list_refresh                  ();

void           catalog_list_update_underline         (CatalogList *cat_list);

gchar *        catalog_list_get_path_from_tree_path  (CatalogList *cat_list,
						      GtkTreePath *path);

gchar *        catalog_list_get_path_from_iter       (CatalogList *cat_list,
						      GtkTreeIter *iter);

gchar *        catalog_list_get_name_from_iter       (CatalogList *cat_list,
						      GtkTreeIter *iter);

gchar *        catalog_list_get_path_from_row        (CatalogList *cat_list,
						      gint row);

gboolean       catalog_list_get_selected_iter        (CatalogList *cat_list,
						      GtkTreeIter *iter);

gchar *        catalog_list_get_selected_path        (CatalogList *cat_list);

gboolean       catalog_list_is_catalog               (CatalogList *cat_list,
						      GtkTreeIter *iter);

gboolean       catalog_list_is_dir                   (CatalogList *cat_list,
						      GtkTreeIter *iter);

gboolean       catalog_list_is_search                (CatalogList *cat_list,
						      GtkTreeIter *iter);

gboolean       catalog_list_get_iter_from_path       (CatalogList *cat_list,
						      const gchar *path,
						      GtkTreeIter *iter);

void           catalog_list_change_to                (CatalogList *cat_list,
						      const gchar *path);

void           catalog_list_select_iter              (CatalogList *cat_list,
						      GtkTreeIter *iter);


#endif /* CATALOG_LIST_H */
