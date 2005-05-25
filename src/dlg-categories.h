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

#ifndef DLG_CATEGORIES_H
#define DLG_CATEGORIES_H

#include "typedefs.h"
#include "gth-window.h"

void        dlg_choose_categories    (GtkWindow     *parent,
				      GList         *file_list,
				      GList         *default_categories_list,
				      GList        **add_categories_list,
				      GList        **remove_categories_list,
				      DoneFunc       done_func,
				      gpointer       done_data);
GtkWidget*  dlg_categories_new       (GthWindow     *window);
void        dlg_categories_update    (GtkWidget     *dlg);
void        dlg_categories_close     (GtkWidget     *dlg);

#endif /* DLG_CATEGORIES_H */
