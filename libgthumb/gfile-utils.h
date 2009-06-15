/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2008 Free Software Foundation, Inc.
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

#ifndef GFILE_UTILS_H
#define GFILE_UTILS_H

#include <gio/gio.h>
#include "glib-utils.h"


#define UNREF(obj) {				\
	if (obj != NULL) {			\
		g_object_unref (obj);		\
		obj = NULL;			\
	}					\
}

/*
 * NOTE: All these functions accept/return _only_ uri-style GFiles.
 */

/* GFile to string */

char *        gfile_get_path                   (GFile *file);

/* Debug */

void          gfile_debug                      (const char *cfile,
					        int         line,
						const char *function,
						const char *msg,
	                                        GFile      *file);
void          gfile_warning                    (const char *msg,
	                                        GFile      *file,
	                                        GError     *err);
/* Constructors */

GFile *       gfile_new                        (const char *path);
GFile *       gfile_new_va                     (const char *path,
                                                ...);

/* File utils */

GFile *       gfile_append_path                (GFile      *dir,
		                                const char *path,
                                                ...);

gboolean      gfile_is_local                   (GFile      *file);
gboolean      gfile_is_hidden                  (GFile      *file);
char *        gfile_get_filename_extension     (GFile      *file);
const char*   gfile_get_mime_type              (GFile      *file,
                                                gboolean    fast_file_type);
gboolean      gfile_image_is_jpeg              (GFile      *file);
gboolean      gfile_is_file                    (GFile      *file);
gboolean      gfile_is_dir                     (GFile      *file);
gboolean      gfile_path_contains              (GFile      *file,
						const char *find_this);
goffset       gfile_get_size                   (GFile      *file);
char *        gfile_get_display_name           (GFile      *file);
void          gfile_set_mtime                  (GFile      *gfile,
                                                time_t      mtime);

/* Directory utils */

gboolean      gfile_ensure_dir_exists          (GFile      *dir,
			                        GError    **error);
guint64       gfile_get_destination_free_space (GFile      *file);
GFile *       gfile_get_home_dir               (void);
GFile *       gfile_get_tmp_dir                (void);
GFile *       gfile_get_temp_dir_name          (void);
gboolean      gfile_dir_remove_recursive       (GFile      *dir);

gboolean      gfile_path_list_new              (GFile      *gfile,
                                                GList     **files,
                                                GList     **dirs);
gboolean      gfile_list_new                   (GFile      *gfile,
                                                GList     **files,
                                                GList     **dirs);

/* Xfer */

gboolean      gfile_xfer                       (GFile      *sfile,
		                                GFile      *dfile,
		                                gboolean    move,
                                                gboolean    overwrite,
						GError    **error);
gboolean      gfile_copy                       (GFile      *sfile,
		                                GFile      *dfile,
                                                gboolean    overwrite,
						GError    **error);
gboolean      gfile_move                       (GFile      *sfile,
		                                GFile      *dfile,
                                                gboolean    overwrite,
						GError    **error);
/* line-based read/write */
gssize        gfile_output_stream_write_line   (GFileOutputStream *ostream,
                                		GError           **error,
		                                const char        *format,
                		                ...);
gssize        gfile_output_stream_write        (GFileOutputStream  *ostream, 
                                                GError            **error,
                                                const char         *str);

/* lists */
void          gfile_list_free                  (GList      *list);
GList *       gfile_list_find_gfile            (GList      *list,
					        GFile      *gfile);

#endif /* GFILE_UTILS_H */
