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

#ifndef FILE_LIST_H
#define FILE_LIST_H

#include "typedefs.h"
#include "thumb-loader.h"
#include "file-data.h"

typedef struct {
	GList       *list;                /* A list of FileData elements. */
	SortMethod   sort_method;         /* How to sort the list. */
	GtkSortType  sort_type;           /* ascending or discending sort. */

	GtkWidget   *root_widget;         /* The widget that contains all. */
	GtkWidget   *ilist;               /* The image-list that contains the 
					   * list of files. */

	gboolean     show_dot_files;      /* Whether to show files that starts
					   * with a dot (hidden files).*/
	gboolean     enable_thumbs;       /* Whether to show the thumbnails. */

	gint         thumb_size;          /* The max size of the thumbnails. */

	ProgressFunc progress_func;
	gpointer     progress_data;

	gboolean     interrupt_set_list;  /* whether to interrupt the set_list
					   * process. */
	DoneFunc     interrupt_done_func; /* function to call when the
					   * interruption has completed. */
	gpointer     interrupt_done_data;

	/* -- thumbs update data -- */

	ThumbLoader *thumb_loader;
	gboolean     doing_thumbs;        /* thumbs creation process is 
					   * active. */
	gint         thumbs_num;
	FileData    *thumb_fd;
	gint         thumb_pos;           /* The pos of the item we are 
					   * genereting a thumbnail. */
} FileList;


FileList*   file_list_new                    (void);

void        file_list_free                   (FileList     *file_list);

void        file_list_set_list               (FileList     *file_list,
					      GList        *new_list,
					      DoneFunc      done_func,
					      gpointer      done_func_data);

void        file_list_add_list               (FileList     *file_list,
					      GList        *new_list,
					      DoneFunc      done_func,
					      gpointer      done_func_data);

void        file_list_interrupt_set_list     (FileList     *file_list,
					      DoneFunc      done_func,
					      gpointer      done_data);

void        file_list_set_sort_method        (FileList     *file_list,
					      SortMethod    method);

/* how to sort: ascending or discending. */
void        file_list_set_sort_type          (FileList     *file_list,
					      GtkSortType   sort_type);

void        file_list_interrupt_thumbs       (FileList     *file_list, 
					      DoneFunc      done_func,
					      gpointer      done_func_data);

gint        file_list_pos_from_path          (FileList     *file_list, 
					      const char   *path);

GList*      file_list_get_all                (FileList     *file_list);

gint        file_list_get_length             (FileList     *file_list);

GList*      file_list_get_selection          (FileList     *file_list);

GList*      file_list_get_selection_as_fd    (FileList     *file_list);

gint        file_list_get_selection_length   (FileList     *file_list);

gchar*      file_list_path_from_pos          (FileList     *file_list,
					      int           pos);

gboolean    file_list_is_selected            (FileList     *file_list, 
					      int           pos);

void        file_list_select_image_by_pos    (FileList     *file_list,
					      int           pos);

void        file_list_select_all             (FileList     *file_list);

void        file_list_unselect_all           (FileList     *file_list);

void        file_list_enable_thumbs          (FileList     *file_list,
					      gboolean      enable);

void        file_list_set_progress_func      (FileList     *file_list,
					      ProgressFunc  func,
					      gpointer      data);

gint        file_list_next_image             (FileList     *file_list,
					      int           starting_pos,
					      gboolean      without_error);

gint        file_list_prev_image             (FileList     *file_list,
					      int           starting_pos,
					      gboolean      without_error);

void        file_list_delete_pos             (FileList     *file_list,
					      int           pos);

void        file_list_rename_pos             (FileList     *file_list,
					      int           pos, 
					      const char   *path);

void        file_list_update_comment         (FileList     *file_list,
					      int           pos);

void        file_list_update_thumb           (FileList     *file_list,
					      int           pos);

void        file_list_update_thumb_list      (FileList     *file_list,
					      GList        *list /*path list*/);

void        file_list_restart_thumbs         (FileList     *file_list,
					      gboolean      _continue);

void        file_list_set_thumbs_size        (FileList     *file_list,
					      int           size);

#endif /* FILE_LIST_H */
