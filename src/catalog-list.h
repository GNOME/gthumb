/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2001-2006 The Free Software Foundation, Inc.
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

#include <gtk/gtk.h>

typedef struct {
	char            *path;            /* The current dir to show. */
	GtkListStore    *list_store;
	GtkWidget       *list_view;       /* The widget that shows the list. */
	GtkWidget       *root_widget;     /* The widget that contains all. */
	GtkCellRenderer *text_renderer;
	gboolean         single_click;
	gboolean         dirs_only;
	GtkTreePath     *hover_path;
} CatalogList;

CatalogList *  catalog_list_new                      (gboolean     single_click_policy);
void           catalog_list_free                     (CatalogList *cat_list);
gboolean       catalog_list_refresh                  (CatalogList *cat_list);
void           catalog_list_update_click_policy      (CatalogList *cat_list);
char *         catalog_list_get_path_from_tree_path  (CatalogList *cat_list,
						      GtkTreePath *path);
char *         catalog_list_get_path_from_iter       (CatalogList *cat_list,
						      GtkTreeIter *iter);
char *         catalog_list_get_name_from_iter       (CatalogList *cat_list,
						      GtkTreeIter *iter);
char *         catalog_list_get_path_from_row        (CatalogList *cat_list,
						      int          row);
gboolean       catalog_list_get_selected_iter        (CatalogList *cat_list,
						      GtkTreeIter *iter);
char *         catalog_list_get_selected_path        (CatalogList *cat_list);
gboolean       catalog_list_is_catalog               (CatalogList *cat_list,
						      GtkTreeIter *iter);
gboolean       catalog_list_is_dir                   (CatalogList *cat_list,
						      GtkTreeIter *iter);
gboolean       catalog_list_is_search                (CatalogList *cat_list,
						      GtkTreeIter *iter);
gboolean       catalog_list_get_iter_from_path       (CatalogList *cat_list,
						      const char  *path,
						      GtkTreeIter *iter);
void           catalog_list_change_to                (CatalogList *cat_list,
						      const char  *path);
void           catalog_list_select_iter              (CatalogList *cat_list,
						      GtkTreeIter *iter);
void           catalog_list_show_dirs_only           (CatalogList *cat_list,
						      gboolean     value);

#endif /* CATALOG_LIST_H */
