/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2001, 2003 Free Software Foundation, Inc.
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

#ifndef CATALOG_H
#define CATALOG_H

#include <glib.h>
#include <gtk/gtkenums.h>
#include "search.h"
#include "typedefs.h"


typedef struct {
	char          *path;  /* File name of the catalog. */
	GList         *list;  /* The list of file names the catalog contains. */
	SearchData    *search_data;
	GthSortMethod  sort_method;
	GtkSortType    sort_type;
} Catalog;


/* functions to use the Catalog structure. */


Catalog*  catalog_new                        (void);
void      catalog_free                       (Catalog    *catalog);
void      catalog_set_path                   (Catalog    *catalog,
					      char       *full_path);
void      catalog_set_search_data            (Catalog    *catalog,
					      SearchData *search_data);
gboolean  catalog_is_search_result           (Catalog    *catalog);
gboolean  catalog_load_from_disk             (Catalog     *catalog,
					      const char  *full_path,
					      GError     **error);
gboolean  catalog_load_search_data_from_disk (Catalog     *catalog,
					      const char  *uri,
					      GError     **gerror);
gboolean  catalog_write_to_disk              (Catalog     *catalog,
					      GError     **error);
void      catalog_add_item                   (Catalog     *catalog,
					      const char  *file_path);
void      catalog_insert_items               (Catalog     *catalog,
					      GList       *list,
				              int          pos);
int       catalog_remove_item                (Catalog     *catalog,
					      const char  *file_path);
void      catalog_remove_all_items           (Catalog     *catalog);


#endif /* CATALOG_H */
