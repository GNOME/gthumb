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

#ifndef GTH_FILE_LIST_H
#define GTH_FILE_LIST_H

#include <glib.h>
#include "typedefs.h"
#include "thumb-loader.h"
#include "file-data.h"
#include "gth-file-view.h"

#define GTH_TYPE_FILE_LIST            (gth_file_list_get_type ())
#define GTH_FILE_LIST(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTH_TYPE_FILE_LIST, GthFileList))
#define GTH_FILE_LIST_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GTH_TYPE_FILE_LIST, GthFileListClass))
#define GTH_IS_FILE_LIST(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTH_TYPE_FILE_LIST))
#define GTH_IS_FILE_LIST_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GTH_TYPE_FILE_LIST))
#define GTH_FILE_LIST_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), GTH_TYPE_FILE_LIST, GthFileListClass))

typedef struct _GthFileList       GthFileList;
typedef struct _GthFileListClass  GthFileListClass;

struct _GthFileList {
	GObject __parent;

	GList       *list;                /* A list of FileData elements. */
	GthSortMethod sort_method;         /* How to sort the list. */
	GtkSortType   sort_type;           /* ascending or discending sort. */

	GtkWidget   *root_widget;         /* The widget that contains all. */
	GtkWidget   *drag_source;  
	GthFileView *view;                /* The view that contains the 
					   * file list. */

	gboolean     show_dot_files;      /* Whether to show files that starts
					   * with a dot (hidden files).*/
	gboolean     enable_thumbs;       /* Whether to show the thumbnails. */

	int          thumb_size;          /* Thumbnails max size. */

	ProgressFunc progress_func;
	gpointer     progress_data;

	gboolean     interrupt_set_list;  /* Whether to interrupt the set_list
					   * process. */
	DoneFunc     interrupt_done_func; /* Function to call when the
					   * interruption has completed. */
	gpointer     interrupt_done_data;

	/* -- thumbs update data -- */

	ThumbLoader *thumb_loader;
	gboolean     doing_thumbs;        /* Thumbs creation process is 
					   * active. */
	gboolean     interrupt_thumbs;    /* Thumbs creation interruption is 
					   * underway. */

	int          thumbs_num;
	FileData    *thumb_fd;
	int          thumb_pos;           /* The position of the item we are 
					   * genereting a thumbnail. */

	guint        scroll_timer;

	gboolean     starting_update;
};


struct _GthFileListClass {
	GObjectClass __parent;

	/* -- signals -- */
	
	void (*busy) (GthFileList *file_list);

	void (*idle) (GthFileList *file_list);
};


GType        gth_file_list_get_type             (void);

GthFileList* gth_file_list_new                  (void);

void         gth_file_list_set_list             (GthFileList  *file_list,
						 GList        *new_list,
						 GthSortMethod sort_method,
						 GtkSortType   sort_type,
						 DoneFunc      done_func,
						 gpointer      done_func_data);

void         gth_file_list_add_list             (GthFileList  *file_list,
						 GList        *new_list,
						 DoneFunc      done_func,
						 gpointer      done_func_data);

void         gth_file_list_interrupt_set_list   (GthFileList  *file_list,
						 DoneFunc      done_func,
						 gpointer      done_data);

void         gth_file_list_set_sort_method      (GthFileList  *file_list,
						 GthSortMethod method,
						 gboolean      update);

void         gth_file_list_set_sort_type        (GthFileList  *file_list,
						 GtkSortType   sort_type,
						 gboolean      update);

void         gth_file_list_interrupt_thumbs     (GthFileList  *file_list, 
						 DoneFunc      done_func,
						 gpointer      done_func_data);

int          gth_file_list_pos_from_path        (GthFileList  *file_list, 
						 const char   *path);

GList*       gth_file_list_get_all              (GthFileList  *file_list);

GList*       gth_file_list_get_all_from_view    (GthFileList  *file_list);

int          gth_file_list_get_length           (GthFileList  *file_list);

GList*       gth_file_list_get_selection        (GthFileList  *file_list);

GList*       gth_file_list_get_selection_as_fd  (GthFileList  *file_list);

int          gth_file_list_get_selection_length (GthFileList  *file_list);

char*        gth_file_list_path_from_pos        (GthFileList  *file_list,
						 int           pos);

gboolean     gth_file_list_is_selected          (GthFileList  *file_list, 
						 int           pos);

void         gth_file_list_select_image_by_pos  (GthFileList  *file_list,
						 int           pos);

void         gth_file_list_select_all           (GthFileList  *file_list);

void         gth_file_list_unselect_all         (GthFileList  *file_list);

void         gth_file_list_enable_thumbs        (GthFileList  *file_list,
						 gboolean      enable,
						 gboolean      update);

void         gth_file_list_set_progress_func    (GthFileList  *file_list,
						 ProgressFunc  func,
						 gpointer      data);

int          gth_file_list_next_image           (GthFileList  *file_list,
						 int           starting_pos,
						 gboolean      without_error,
						 gboolean      only_selected);

int          gth_file_list_prev_image           (GthFileList  *file_list,
						 int           starting_pos,
						 gboolean      without_error,
						 gboolean      only_selected);

void         gth_file_list_delete_pos           (GthFileList  *file_list,
						 int           pos);

void         gth_file_list_rename_pos           (GthFileList  *file_list,
						 int           pos, 
						 const char   *path);

void         gth_file_list_update_comment       (GthFileList  *file_list,
						 int           pos);

void         gth_file_list_update_thumb         (GthFileList  *file_list,
						 int           pos);

void         gth_file_list_update_thumb_list    (GthFileList  *file_list,
						 GList        *list /*path list*/);

void         gth_file_list_restart_thumbs       (GthFileList  *file_list,
						 gboolean      _continue);

void         gth_file_list_set_thumbs_size      (GthFileList  *file_list,
						 int           size);

#endif /* GTH_FILE_LIST_H */
