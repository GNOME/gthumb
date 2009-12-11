/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2009 Free Software Foundation, Inc.
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

#ifndef CALLBACKS_H
#define CALLBACKS_H

#include <gthumb.h>
#include <extensions/catalogs/gth-catalog.h>

void         search__gth_browser_construct_cb            (GthBrowser  *browser);
void         search__gth_browser_update_sensitivity_cb   (GthBrowser  *browser);
void         search__gth_browser_update_extra_widget_cb  (GthBrowser  *browser);
GthCatalog * search__gth_catalog_load_from_data_cb       (const void  *buffer);
void         search__dlg_catalog_properties              (GthBrowser  *browser,
							  GtkBuilder  *builder,
							  GthFileData *file_data);
void         search__dlg_catalog_properties_save         (GtkBuilder  *builder,
							  GthFileData *file_data,
							  GthCatalog  *catalog);
void         search__dlg_catalog_properties_saved        (GthBrowser  *browser,
							  GthFileData *file_data,
							  GthCatalog  *catalog);

#endif /* CALLBACKS_H */
