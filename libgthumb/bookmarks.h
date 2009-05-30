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

#ifndef BOOKMARKS_H
#define BOOKMARKS_H

#include <glib.h>

typedef struct {
	char       *rc_filename;
	int         max_lines;
	GList      *list; 
	GHashTable *names;
	GHashTable *tips;
} Bookmarks;	


/* Bookmarks functions. */

Bookmarks *        bookmarks_new               (char        *rc_filename);

void               bookmarks_free              (Bookmarks   *bookmarks);

void               bookmarks_add               (Bookmarks   *bookmarks,
						const char  *path,
						gboolean     avoid_duplicates,
						gboolean     append);

void               bookmarks_remove            (Bookmarks   *bookmarks,
						const char  *path);

void               bookmarks_remove_all_instances (Bookmarks   *bookmarks,
						   const char  *path);

void               bookmarks_remove_all        (Bookmarks   *bookmarks);

void               bookmarks_remove_from       (Bookmarks   *bookmarks,
						GList       *here);

void               bookmarks_load_from_disk    (Bookmarks   *bookmarks);

void               bookmarks_write_to_disk     (Bookmarks   *bookmarks);

const char *       bookmarks_get_menu_name     (Bookmarks   *bookmarks,
						const char  *path);

const char *       bookmarks_get_menu_tip      (Bookmarks   *bookmarks,
						const char  *path);

void               bookmarks_set_max_lines     (Bookmarks   *bookmarks,
						int          max_lines);

char *             bookmarks_utils__get_menu_item_name  (const char *path);


#endif /* BOOKMARKS_H */

