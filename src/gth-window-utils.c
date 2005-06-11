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

#include "catalog.h"
#include "comments.h"
#include "file-utils.h"
#include "glib-utils.h"
#include "gth-window-utils.h"
#include "gtk-utils.h"
#include "main.h"


void
remove_files_from_catalog (GthWindow  *window,
			   const char *catalog_path,
			   GList      *list)
{
	Catalog *catalog;
	GList   *scan;
	GError  *gerror;

	catalog = catalog_new ();
	if (! catalog_load_from_disk (catalog, catalog_path, &gerror)) {
		_gtk_error_dialog_from_gerror_run (GTK_WINDOW (window), &gerror);
		path_list_free (list);
		catalog_free (catalog);
		return;
	}

	for (scan = list; scan; scan = scan->next) 
		catalog_remove_item (catalog, (char*) scan->data);

	if (! catalog_write_to_disk (catalog, &gerror)) {
		_gtk_error_dialog_from_gerror_run (GTK_WINDOW (window), &gerror);
		path_list_free (list);
		catalog_free (catalog);
		return;
	}

	all_windows_notify_cat_files_deleted (catalog_path, list);

	path_list_free (list);
	catalog_free (catalog);
}

