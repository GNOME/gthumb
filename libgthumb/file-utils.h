/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2001, 2003, 2005 Free Software Foundation, Inc.
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
#include <gdk-pixbuf/gdk-pixbuf.h>
#include "typedefs.h"
#include "file-data.h"

#define GNOME_DESKTOP_USE_UNSTABLE_API
#include <libgnomeui/gnome-desktop-thumbnail.h>

#define SPECIAL_DIR(x) (! strcmp (x, "..") || ! strcmp (x, "."))
#define errno_to_string() (gnome_vfs_result_to_string (gnome_vfs_result_from_errno ()))

typedef void (*CopyDoneFunc) (const char *, GError *, gpointer);

/* Async directory list */

typedef struct _PathListData PathListData;
typedef gboolean (*PathListFilterFunc) (PathListData *pld, FileData *, gpointer data);
typedef void (*PathListDoneFunc) (PathListData *pld, gpointer data);

struct _PathListData {
	GFile		   *gfile;
	GFileEnumerator    *gfile_enum;
	GList              *files;               /* char* items. */
	GList              *dirs;                /* GFile* items. */
	PathListFilterFunc  filter_func;
	gpointer            filter_data;
	PathListDoneFunc    done_func;
	gpointer            done_data;
	GCancellable       *cancelled;
	GHashTable         *hidden_files;
	gboolean            fast_file_type;
	GError             *error;
	GList		   *file_list;
	GList		   *current_file;
};

typedef struct {
	PathListData *pli_data;
} PathListHandle;

void                path_list_data_free           (PathListData       *dli);
PathListHandle *    path_list_async_new           (const char         *uri,
						   PathListFilterFunc  filter_func,
		     				   gpointer            filter_data,
		     				   gboolean            fast_file_type,
						   PathListDoneFunc    done_func,
						   gpointer            done_data);
void                path_list_async_interrupt     (PathListHandle     *handle);
gboolean            path_list_new                 (const char         *path,
						   GList             **files,
						   GList             **dirs);
GList *             path_list_dup                 (GList              *path_list);
void                path_list_free                (GList              *list);
GList *             path_list_find_path           (GList              *list,
						   const char         *path);

/* Directory utils */

gboolean            dir_make                      (const char       *uri);
gboolean            dir_remove_recursive          (const char       *path);

gboolean            ensure_dir_exists             (const char       *path);
GList *             dir_list_filter_and_sort      (GList            *dir_list,
						   gboolean          show_dot_files);
gboolean            file_filter                   (FileData         *file,
	     					   gboolean          show_hidden_files,
	     					   gboolean          show_only_images,
						   gboolean	     fast_file_type);

typedef void (*VisitFunc) (char *real_file, char *rc_file, gpointer data);
gboolean            visit_rc_directory_sync       (const char       *rc_dir,
						   const char       *rc_ext,
						   const char       *dir,
						   gboolean          recursive,
						   VisitFunc         do_something,
						   gpointer          data);

/* File utils */

gboolean	    mime_type_is_image	          (const char       *mime_type);
gboolean            mime_type_is_video            (const char       *mime_type);
gboolean            mime_type_is_audio            (const char       *mime_type);
gboolean	    file_is_image 		  (const gchar      *name,
						   gboolean          fast_file_type);
gboolean            file_is_image_video_or_audio  (const gchar      *name,
                                                   gboolean          fast_file_type);
gboolean            file_is_hidden                (const char       *name);
gboolean            file_copy                     (const char       *from,
						   const char       *to,
                                                   gboolean          overwrite,
						   GError	   **error);
gboolean            file_move                     (const char       *from,
						   const char       *to,
					           gboolean          overwrite,
						   GError	   **error);
void                file_unlink_with_gerror       (const char       *full_path,
                                                   GError          **gerror);
gboolean            file_unlink                   (const char       *path);
void		    delete_thumbnail	          (const char       *path);
gboolean            mime_type_is                  (const char       *mime_type,
	      				 	   const char       *value);
gboolean            path_exists                   (const char       *s);
gboolean            path_is_file                  (const char       *s);
gboolean            path_is_dir                   (const char       *s);
goffset             get_file_size                 (const char       *path);
time_t              get_file_mtime                (const char       *s);
time_t              get_file_ctime                (const char       *s);
void                set_file_mtime                (const char       *s,
						   time_t            mtime);

/* URI/Path utils */

char *		    get_utf8_display_name_from_uri (const char      *uri);
const char *        get_home_uri                  (void);
char *              get_uri_scheme                (const char       *uri);
const char *        remove_host_from_uri          (const char       *uri);
char *              get_base_uri                  (const char       *uri);
gboolean            uri_has_scheme                (const char       *uri);
gboolean            uri_scheme_is_file            (const char       *uri);
gboolean            uri_scheme_is_catalog         (const char       *uri);
gboolean            uri_scheme_is_search          (const char       *uri);
char *              add_scheme_if_absent          (const char       *path);
char *              add_filename_to_uri           (const char       *uri,
						   const char       *filename);
char *              get_uri_from_local_path       (const char       *path);
char *              get_uri_display_name          (const char       *uri);
G_CONST_RETURN char*file_name_from_path           (const char       *path);
gboolean            path_in_path                  (const char       *path_src,
						   const char       *path_dest);
int                 uricmp                        (const char       *uri1,
						   const char       *uri2);
gboolean            same_uri                      (const char       *uri1,
						   const char       *uri2);
char               *basename_for_display          (const char       *uri);
char               *gfile_get_basename_for_uri    (const char       *uri);

char *              get_path_relative_to_uri      (const char       *uri,
						   const char       *desturi);
char *              remove_level_from_path        (const char       *path);

char *              remove_ending_separator       (const char       *path);
char *              remove_special_dirs_from_path (const char       *path);

GnomeVFSURI *       new_uri_from_path             (const char       *path);
char *              build_uri                     (const char       *s1,
						   const char       *s2,
						   ...);
char *              resolve_all_symlinks          (const char       *uri);
gboolean            uri_is_root                   (const char       *uri);

/* Catalogs */

char *              get_catalog_full_path         (const char       *relative_path);
gboolean            file_is_search_result         (const char       *full_path);

/* escape */

char *              escape_underscore             (const char       *name);
char *              escape_uri                    (const char       *uri);
char *              shell_escape                  (const char       *filename);

/* extesion */

gboolean            file_extension_is             (const char       *filename,
						   const char       *ext);
char *              get_filename_extension        (const char       *filename);
char *              remove_extension_from_path    (const char       *path);

/* temp */

char *              get_temp_dir_name             (void);
char *              get_temp_file_name            (const char       *tmpdir,
						   const char       *ext);

/* VFS extensions */

typedef struct _CopyData CopyData;

void                copy_data_cancel              (CopyData         *data);
CopyData *          copy_file_async               (const char       *source_uri,
		 				   const char       *target_uri,
		 				   CopyDoneFunc      done_func,
		 				   gpointer          done_data);
guint64             get_destination_free_space    (const char       *path);
const char*         get_file_mime_type            (const char       *path,
						   gboolean          fast_file_type);
const char *        get_mime_type_from_ext        (const char       *ext);
gboolean            is_mime_type_writable         (const char       *mime_type);
gboolean	    is_local_file                 (const char       *filename);
void                free_cache                    (void);
GHashTable *        read_dot_hidden_file          (const char       *uri);

/* Pixbuf + VFS */

GdkPixbuf*	    gth_pixbuf_new_from_file	       (FileData               *file,
			  				GError                **error,
			  				int                     requested_width,
			  				int                     requested_height,
			  				GnomeDesktopThumbnailFactory  *factory);
GdkPixbufAnimation* gth_pixbuf_animation_new_from_file (FileData              *file,
						        GError               **error,
						        int                    requested_width,
						        int	               requested_height,
						        GnomeDesktopThumbnailFactory *factory);

char *              xdg_user_dir_lookup               (const char            *type);
					       
#endif /* FILE_UTILS_H */
