/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2001-2006 The Free Software Foundation, Inc.
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
#include "gth-filter.h"
#include "gth-file-view.h"

#define GTH_TYPE_FILE_LIST            (gth_file_list_get_type ())
#define GTH_FILE_LIST(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTH_TYPE_FILE_LIST, GthFileList))
#define GTH_FILE_LIST_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GTH_TYPE_FILE_LIST, GthFileListClass))
#define GTH_IS_FILE_LIST(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTH_TYPE_FILE_LIST))
#define GTH_IS_FILE_LIST_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GTH_TYPE_FILE_LIST))
#define GTH_FILE_LIST_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), GTH_TYPE_FILE_LIST, GthFileListClass))

typedef struct _GthFileList       GthFileList;
typedef struct _GthFileListClass  GthFileListClass;
typedef struct _GthFileListPrivateData  GthFileListPrivateData;

struct _GthFileList {
	GObject      __parent;
	GList         *list;              /* A list of FileData elements. */
	GthFileView   *view;
	GtkWidget     *root_widget;       /* The widget that contains all. */
	GtkWidget     *drag_source;
	gboolean       enable_thumbs;     /* Whether to show the thumbnails. */
	gboolean       busy;
	GthFileListPrivateData *priv;
};

struct _GthFileListClass {
	GObjectClass __parent;

	/* -- signals -- */

	void (*busy) (GthFileList *file_list);
	void (*idle) (GthFileList *file_list);
	void (*done) (GthFileList *file_list);
};

GType        gth_file_list_get_type             (void);
GthFileList* gth_file_list_new                  (void);
GthFileView* gth_file_list_get_view             (GthFileList   *file_list);
void         gth_file_list_set_list             (GthFileList   *file_list,
						 GList         *new_list,
						 GthSortMethod  sort_method,
						 GtkSortType    sort_type);
void         gth_file_list_add_list             (GthFileList   *file_list,
						 GList         *new_list);
void         gth_file_list_delete_list          (GthFileList   *file_list,
						 GList         *uri_list);
void         gth_file_list_set_empty_list       (GthFileList   *file_list);
void         gth_file_list_stop                 (GthFileList   *file_list);
void         gth_file_list_set_sort_method      (GthFileList   *file_list,
						 GthSortMethod  method,
						 gboolean       update);
void         gth_file_list_set_sort_type        (GthFileList   *file_list,
						 GtkSortType    sort_type,
						 gboolean       update);
FileData *   gth_file_list_filedata_from_path   (GthFileList   *file_list,
			          		 const char    *path,
						 int           *pos);
int          gth_file_list_pos_from_path        (GthFileList   *file_list,
						 const char    *path);
GList*       gth_file_list_get_all              (GthFileList   *file_list);
GList*       gth_file_list_get_all_from_view    (GthFileList   *file_list);
GList*       gth_file_list_get_selection        (GthFileList   *file_list);
GList*       gth_file_list_get_selection_as_fd  (GthFileList   *file_list);
char*        gth_file_list_path_from_pos        (GthFileList   *file_list,
						 int            pos);
gboolean     gth_file_list_is_selected          (GthFileList   *file_list,
						 int            pos);
void         gth_file_list_select_image_by_pos  (GthFileList   *file_list,
						 int            pos);
void         gth_file_list_unselect_all         (GthFileList   *file_list);
void         gth_file_list_enable_thumbs        (GthFileList   *file_list,
						 gboolean       enable,
						 gboolean       update);
void         gth_file_list_set_progress_func    (GthFileList   *file_list,
						 ProgressFunc   func,
						 gpointer       data);
int          gth_file_list_next_image           (GthFileList   *file_list,
						 int            starting_pos,
						 gboolean       without_error,
						 gboolean       only_selected);
int          gth_file_list_prev_image           (GthFileList   *file_list,
						 int            starting_pos,
						 gboolean       without_error,
						 gboolean       only_selected);
void         gth_file_list_delete               (GthFileList   *file_list,
						 const char    *uri);
void         gth_file_list_rename               (GthFileList   *file_list,
		      				 const char    *from_uri,
		      				 const char    *to_uri);
void         gth_file_list_update_comment       (GthFileList   *file_list,
						 const char    *uri);
void         gth_file_list_update_thumb_list    (GthFileList   *file_list,
						 GList         *list /*path list*/);
void         gth_file_list_restart_thumbs       (GthFileList   *file_list,
						 gboolean       _continue);
void         gth_file_list_set_thumbs_size      (GthFileList   *file_list,
						 int            size);
void         gth_file_list_set_filter           (GthFileList   *file_list,
						 GthFilter     *filter);
void         gth_file_list_update_icon_theme    (GthFileList   *file_list);
void         gth_file_list_ignore_hidden_thumbs (GthFileList   *file_list,
						 gboolean       ignore);

#endif /* GTH_FILE_LIST_H */
