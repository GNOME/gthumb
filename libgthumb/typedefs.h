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

#ifndef TYPEDEFS_H
#define TYPEDEFS_H


#include <glib.h>

#define FILE_PREFIX    "file://"
#define CATALOG_PREFIX "catalog://"
#define SEARCH_PREFIX  "search://"

#define FILE_PREFIX_L    7
#define CATALOG_PREFIX_L 10
#define SEARCH_PREFIX_L  9

#define GLADE_FILE                "gthumb.glade"

#define RC_DIR                    ".gnome2/gthumb"

#define RC_CATALOG_DIR            ".gnome2/gthumb/collections"
#define RC_COMMENTS_DIR           ".gnome2/gthumb/comments"
#define RC_BOOKMARKS_FILE         ".gnome2/gthumb/bookmarks"
#define RC_HISTORY_FILE           ".gnome2/gthumb/history"
#define RC_CATEGORIES_FILE        ".gnome2/gthumb/categories"

#define OLD_RC_CATALOG_DIR        ".gqview/collections"
#define OLD_RC_COMMENTS_DIR       ".gqview/comments"
#define OLD_RC_BOOKMARKS_FILE     ".gqview/bookmarks"
#define OLD_RC_HISTORY_FILE       ".gqview/history"
#define OLD_RC_CATEGORIES_FILE    ".gqview/categories"

#define CACHE_DIR                 ".thumbnails"

#define CACHE_THUMB_EXT           ".png"
#define CATALOG_EXT               ".gqv"
#define COMMENT_EXT               ".xml"

#define CLIST_WIDTH_DEFAULT       100
#define CLIST_HEIGHT_DEFAULT      100

#define CLIST_ROW_PAD             5
#define CLIST_IMAGE_ROW_PAD       2

#define BOOKMARKS_MENU_MAX_LENGTH 35
#define HISTORY_LIST_MAX_LENGTH   35

typedef enum { /*< skip >*/
	DIRECTION_FORWARD = 0,
	DIRECTION_REVERSE = 1
} DirectionType;


typedef enum { /*< skip >*/
	TOOLBAR_STYLE_SYSTEM = 0,
	TOOLBAR_STYLE_TEXT_BELOW,
	TOOLBAR_STYLE_TEXT_BESIDE,
	TOOLBAR_STYLE_ICONS,
	TOOLBAR_STYLE_TEXT
} ToolbarStyle;


typedef enum { /*< skip >*/
	NO_LIST = 0,
	DIR_LIST,
	CATALOG_LIST,
	CANCEL_BTN,
	NUM_CONTENT_TYPES
} SidebarContent;


/* keep the order of the items in sync with the order of the 
 * sort_by_radio_list structure in the file menu.h */
typedef enum { /*< skip >*/
	SORT_NONE,
	SORT_BY_NAME,
	SORT_BY_PATH,
	SORT_BY_SIZE,
	SORT_BY_TIME
} SortMethod;


typedef enum { /*< skip >*/
	CLICK_POLICY_FOLLOW_NAUTILUS = 0,
	CLICK_POLICY_SINGLE,
	CLICK_POLICY_DOUBLE
} ClickPolicy;


typedef enum { /*< skip >*/
	OVERWRITE_SKIP,
	OVERWRITE_RENAME,
	OVERWRITE_ASK,
	OVERWRITE_OVERWRITE,
} OverwriteMode;


typedef void (*ErrorFunc)      (gpointer data);

typedef void (*DoneFunc)       (gpointer data);

typedef void (*ProgressFunc)   (gfloat percent, 
				gpointer data);

typedef void (*AreaReadyFunc)  (guint x,
				guint y, 
				guint w, 
				guint h, 
				gpointer data);


#endif /* TYPEDEFS_H */

