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
#include "file-utils.h" /* for PathListHandle */

#define GTH_TYPE_DIR_LIST            (gth_dir_list_get_type ())
#define GTH_DIR_LIST(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTH_TYPE_DIR_LIST, GthDirList))
#define GTH_DIR_LIST_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GTH_TYPE_DIR_LIST, GthDirListClass))
#define GTH_IS_DIR_LIST(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTH_TYPE_DIR_LIST))
#define GTH_IS_DIR_LIST_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GTH_TYPE_DIR_LIST))
#define GTH_DIR_LIST_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), GTH_TYPE_DIR_LIST, GthDirListClass))

typedef struct _GthDirList       GthDirList;
typedef struct _GthDirListClass  GthDirListClass;

struct _GthDirList {
	GObject        __parent;

	char            *path;           /* The directory we are showing. */
	char            *try_path;       /* The directory we are loading. */
	GList           *list;           /* The directory name list (GFile*
				          * elements). */
	GList           *file_list;      /* The file name list (gchar* elements). */
	GtkListStore    *list_store;
	GtkWidget       *list_view;      /* The widget that shows the list. */
	GtkWidget       *root_widget;    /* The widget that contains all. */
	GtkCellRenderer *text_renderer;
	gboolean         show_dot_files; /* Whether to show file names that begin
				          * with a dot. */
	gboolean         show_only_images;
	gboolean         single_click;
	GtkTreePath     *hover_path;
	GFile           *old_dir;
	PathListHandle  *dir_load_handle;
};

struct _GthDirListClass {
	GObjectClass __parent;

	/* -- signals -- */

	void (*started) (GthDirList     *dir_list);
	void (*done)    (GthDirList     *dir_list,
	                 GnomeVFSResult  result);
};

GType          gth_dir_list_get_type                (void);
GthDirList *   gth_dir_list_new                     (void);
void           gth_dir_list_show_hidden_files       (GthDirList  *dir_list,
				     		     gboolean     show);
void           gth_dir_list_show_only_images        (GthDirList  *dir_list,
				     		     gboolean     show);
void           gth_dir_list_change_to               (GthDirList  *dir_list,
						     const char  *path);
void           gth_dir_list_stop                    (GthDirList  *dir_list);
void           gth_dir_list_add_directory           (GthDirList  *dir_list,
						     GFile	 *gfile);
void           gth_dir_list_remove_directory        (GthDirList  *dir_list,
						     GFile	 *path);
char *         gth_dir_list_get_name_from_iter      (GthDirList  *dir_list,
						     GtkTreeIter *iter);
char *         gth_dir_list_get_path_from_tree_path (GthDirList  *dir_list,
						     GtkTreePath *path);
char *         gth_dir_list_get_path_from_iter      (GthDirList  *dir_list,
						     GtkTreeIter *iter);
char *         gth_dir_list_get_path_from_row       (GthDirList  *dir_list,
						     int          row);
gboolean       gth_dir_list_get_selected_iter       (GthDirList  *dir_list,
						     GtkTreeIter *iter);
char *         gth_dir_list_get_selected_path       (GthDirList  *dir_list);
GList *        gth_dir_list_get_file_list           (GthDirList  *dir_list);
void           gth_dir_list_update_underline        (GthDirList  *dir_list);
void           gth_dir_list_update_icon_theme       (GthDirList  *dir_list);

#endif /* DIR_LIST_H */

