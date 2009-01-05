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

#ifndef DLG_FILE_UTILS_H
#define DLG_FILE_UTILS_H

#include <libgnomevfs/gnome-vfs-result.h>
#include "gth-window.h"


gboolean dlg_file_delete__confirm   (GthWindow   *window,
				     GList       *list,
				     const char  *message);
void     dlg_file_move__ask_dest    (GthWindow   *window,
				     const char  *default_dir,
				     GList       *list);
void     dlg_file_copy__ask_dest    (GthWindow   *window,
				     const char  *default_dir,
				     GList       *list);
void     dlg_file_rename_series     (GthWindow   *window,
				     GList       *old_names,
				     GList       *new_names);
gboolean dlg_check_folder           (GthWindow   *window,
				     const char  *path);

/**/

typedef void (*FileOpDoneFunc)    (GError 	*result,
				   gpointer       data);

void     dlg_files_copy           (GthWindow      *window,
				   GList          *file_list,
				   const char     *dest_path,
				   gboolean        remove_source,
				   gboolean        include_cache,
				   gboolean        overwrite_all,
				   FileOpDoneFunc  done_func,
				   gpointer        done_data);
void     dlg_files_move_to_trash  (GthWindow      *window,
				   GList          *file_list,
				   FileOpDoneFunc  done_func,
				   gpointer        done_data);
void     dlg_files_delete         (GthWindow      *window,
				   GList          *file_list,
				   FileOpDoneFunc  done_func,
				   gpointer        done_data);
void     dlg_folder_copy          (GthWindow      *window,
				   const char     *src_path,
				   const char     *dest_path,
				   gboolean        remove_source,
				   gboolean        include_cache,
				   gboolean        overwrite_all,
				   FileOpDoneFunc  done_func,
				   gpointer        done_data);
void     dlg_folder_move_to_trash (GthWindow      *window,
				   const char     *folder,
				   FileOpDoneFunc  done_func,
				   gpointer        done_data);
void     dlg_folder_delete        (GthWindow      *window,
				   const char     *folder,
				   FileOpDoneFunc  done_func,
				   gpointer        done_data);

/* items means files and folders. */

void     dlg_copy_items           (GthWindow      *window,
				   GList          *item_list,
				   const char     *destination,
				   gboolean        remove_source,
				   gboolean        include_cache,
				   gboolean        overwrite_all,
				   FileOpDoneFunc  done_func,
				   gpointer        done_data);

#endif /* DLG_FILE_UTILS_H */

