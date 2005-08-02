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

#ifndef DIR_LIST_H
#define DIR_LIST_H

#include <gtk/gtk.h>
#include <libgnomevfs/gnome-vfs-types.h>
#include <file-utils.h> /* for PathListHandle */

typedef struct _DirList DirList;

typedef void (*DirListDoneFunc) (DirList *dir_list, gpointer data);


struct _DirList {
	gchar     *path;           /* The directory we are showing. */
	gchar     *try_path;       /* The directory we are loading. */

	GList     *list;           /* The directory name list (gchar* 
				    * elements). */
	GList     *file_list;      /* The file name list (gchar* elements). */

	GtkListStore     *list_store;
	GtkWidget        *list_view;      /* The widget that shows the list. */
	GtkWidget        *root_widget;    /* The widget that contains all. */
	GtkCellRenderer  *text_renderer;

	gboolean   show_dot_files; /* Whether to show file names that begin 
				    * with a dot. */
	gboolean   single_click;
	GtkTreePath *hover_path;

	gchar     *old_dir;

	DirListDoneFunc done_func; /* Function called when the 
				    * dir_list_change_to function has been
				    * completed. Can be NULL. */
	gpointer   done_data;

	PathListHandle *dir_load_handle;
	GnomeVFSResult result;     /* Result of the directory loading. */
};


DirList *      dir_list_new                      ();

void           dir_list_update_underline         (DirList         *dir_list);

void           dir_list_update_icon_theme        (DirList         *dir_list);

void           dir_list_free                     (DirList         *dir_list);

void           dir_list_change_to                (DirList         *dir_list, 
						  const gchar     *path,
						  DirListDoneFunc  func,
						  gpointer         data);

void           dir_list_interrupt_change_to      (DirList         *dir_list,
						  DoneFunc         f,
						  gpointer         data);

void           dir_list_add_directory            (DirList         *dir_list,
						  const char      *path);

void           dir_list_remove_directory         (DirList         *dir_list,
						  const char      *path);

char *         dir_list_get_name_from_iter       (DirList         *dir_list,
						  GtkTreeIter     *iter);

gchar *        dir_list_get_path_from_tree_path  (DirList         *dir_list,
						  GtkTreePath     *path);

gchar *        dir_list_get_path_from_iter       (DirList         *dir_list,
						  GtkTreeIter     *iter);

gchar *        dir_list_get_path_from_row        (DirList         *dir_list,
						  gint             row);

int            dir_list_get_row_from_path        (DirList         *dir_list,
						  const char      *path);

gboolean       dir_list_get_selected_iter        (DirList         *dir_list, 
						  GtkTreeIter     *iter);

gchar *        dir_list_get_selected_path        (DirList         *dir_list);

GList *        dir_list_get_file_list            (DirList         *dir_list);


#endif /* DIR_LIST_H */

