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

#ifndef BOOKMARKS_LIST_H
#define BOOKMARKS_LIST_H

#include <glib.h>
#include <gtk/gtk.h>

typedef struct {
	GList        *list;
	GtkListStore *list_store;
	GtkWidget    *list_view;      /* The widget that shows the list. */
	GtkWidget    *root_widget;    /* The widget that contains all. */
	gboolean      menu_icons;
} BookmarkList;


BookmarkList * bookmark_list_new                     (gboolean menu_icons);

void           bookmark_list_free                    (BookmarkList *book_list);

void           bookmark_list_set                     (BookmarkList *book_list,
						      GList        *list);

char *         bookmark_list_get_selected_path       (BookmarkList *book_list);

void           bookmark_list_select_item             (BookmarkList *book_list,
						      int           row);

#endif /* BOOKMARKS_LIST_H */

