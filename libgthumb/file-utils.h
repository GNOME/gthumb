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

#ifndef FILE_UTILS_H
#define FILE_UTILS_H

#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <libgnomevfs/gnome-vfs-uri.h>
#include <libgnomevfs/gnome-vfs-result.h>
#include <libgnomevfs/gnome-vfs-file-size.h>
#include <libgnomevfs/gnome-vfs-async-ops.h>
#include <libgnomevfs/gnome-vfs-mime-handlers.h>
#include "typedefs.h"

#define SPECIAL_DIR(x) (! strcmp (x, "..") || ! strcmp (x, "."))
#define errno_to_string() (gnome_vfs_result_to_string (gnome_vfs_result_from_errno ()))


typedef struct _PathListData PathListData;

typedef void (*PathListDoneFunc) (PathListData *dld, gpointer data);

struct _PathListData {
	GnomeVFSURI      *uri;
	GnomeVFSResult    result;
	GList            *files;               /* char* items. */
	GList            *dirs;                /* char* items. */
	PathListDoneFunc  done_func;
	gpointer          done_data;
	DoneFunc          interrupt_func;
	gpointer          interrupt_data;
	gboolean          interrupted;
};


typedef struct {
	GnomeVFSAsyncHandle *vfs_handle;
	PathListData *pli_data;
} PathListHandle;


PathListData *      path_list_data_new           ();
void                path_list_data_free          (PathListData *dli);
void                path_list_handle_free        (PathListHandle *handle);

gboolean            path_is_file                 (const gchar *s);

gboolean            path_is_dir                  (const gchar *s);

gboolean            dir_is_empty                 (const gchar *s);

GnomeVFSFileSize    get_file_size                (const gchar *s);

time_t              get_file_mtime               (const gchar *s);

time_t              get_file_ctime               (const gchar *s);

void                set_file_mtime               (const gchar *s,
						  time_t       mtime);

gboolean            file_copy                    (const gchar *from, 
						  const gchar *to);

gboolean            file_move                    (const gchar *from, 
						  const gchar *to);

gboolean            ensure_dir_exists            (const gchar *a_path,
						  mode_t mode);

gboolean            file_is_hidden               (const gchar *name);

G_CONST_RETURN gchar * file_name_from_path       (const gchar *path);

gchar *             remove_level_from_path       (const gchar *path);

gchar *             remove_extension_from_path   (const gchar *path);

gchar *             remove_ending_separator      (const gchar *path);

gboolean            path_in_path                 (const char  *path_src,
						  const char  *path_dest);

/* Return TRUE on success, it is up to you to free
 * the lists with path_list_free()
 */
gboolean            path_list_new                (const gchar *path, 
						  GList **files, 
						  GList **dirs);

GList *             path_list_dup                (GList *path_list);

void                path_list_free               (GList *list);

GList *             path_list_find_path          (GList *list, 
						  const char *path);

PathListHandle *    path_list_async_new          (const gchar *uri, 
						  PathListDoneFunc f,
						  gpointer  data);

void                path_list_async_interrupt    (PathListHandle   *handle,
						  DoneFunc          f,
						  gpointer          data);

gboolean            visit_rc_directory           (const gchar *rc_dir,
						  const gchar *rc_ext,
						  const char *dir,
						  gboolean recursive,
						  gboolean clear_all);

typedef void (*VisitFunc) (gchar *real_file, gchar *rc_file, gpointer data);
typedef void (*VisitDoneFunc) (const GList *dir_list, gpointer data);

void                visit_rc_directory_async     (const gchar *rc_dir,
						  const gchar *rc_ext,
						  const char *dir,
						  gboolean recursive,
						  VisitFunc do_something,
						  VisitDoneFunc done_func,
						  gpointer data);

gboolean            rmdir_recursive              (const gchar *directory);

gboolean            file_is_image                (const gchar *name,
						  gboolean fast_file_type);

gboolean            image_is_jpeg                (const char *name);


gboolean            file_extension_is            (const char *filename, 
						  const char *ext);

long                checksum_simple              (const gchar *path);

GList *             dir_list_filter_and_sort     (GList *dir_list, 
						  gboolean names_only,
						  gboolean show_dot_files);

char*               shell_escape                 (const char *filename);

char *              escape_underscore            (const char *name);

char *              get_terminal                 (gboolean with_exec_flag);

char *              application_get_command      (const GnomeVFSMimeApplication *app);

char *              get_path_relative_to_dir     (const char *filename, 
						  const char *destdir);

char *              remove_special_dirs_from_path (const char *path);
     
GnomeVFSURI *       new_uri_from_path             (const char *path);

char *              new_path_from_uri             (GnomeVFSURI *uri);

GnomeVFSResult      resolve_all_symlinks          (const char  *text_uri,
						   char       **resolved_text_uri);

GnomeVFSFileSize    get_dest_free_space          (const char  *path);

gboolean            is_mime_type_writable        (const char *mime_type);


#endif /* FILE_UTILS_H */
