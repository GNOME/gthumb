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

#ifndef FILE_DATA_H
#define FILE_DATA_H

#include <glib.h>
#include <glib-object.h>
#include <time.h>
#include <sys/stat.h>
#include "comments.h"

typedef struct {
	guint               ref : 8;

	char               *path;          /* Full path name. */
	const char         *name;          /* File name only. */
	char               *utf8_path;	   /* Always unescaped UTF8 */
	const char	   *utf8_name;     /* Always unescaped UTF8 */
	char		   *local_path;    /* May be a gvfs mount point */
	const char         *mime_type;
	goffset             size;
	time_t              ctime;
	time_t              mtime;
	gboolean            can_read;
	guint               exif_data_loaded : 1;
	time_t		    exif_time;	       /* Do not access this directly. Use
						  get_exif_time (fd) instead. This is
						  a cache variable. */
	guint               error : 1;         /* Whether an error occurred loading
					        * this file. */
	guint               thumb_loaded : 1;  /* Whether we have a thumb of this
					        * image. */
	guint               thumb_created : 1; /* Whether a thumb has been
						* created for this image. */

	char               *comment;
	char               *categories;
	CommentData        *comment_data;
	
	GList              *metadata;
} FileData;

#define GTH_TYPE_FILE_DATA (file_data_get_type ())

GType        file_data_get_type            (void);
FileData *   file_data_new                 (const char       *path);
FileData *   file_data_dup                 (FileData         *fd);
FileData *   file_data_ref                 (FileData         *fd);
void         file_data_unref               (FileData         *fd);
void         file_data_set_path            (FileData         *fd,
					    const char       *path);
void         file_data_update              (FileData         *fd);
void         file_data_update_mime_type    (FileData         *fd,
					    gboolean          fast_mime_type);
void         file_data_update_all          (FileData         *fd,
					    gboolean          fast_mime_type);				    
void         file_data_load_comment_data   (FileData         *fd);
void         file_data_update_comment      (FileData         *fd);

GList*       file_data_list_from_uri_list  (GList            *list);
GList*       uri_list_from_file_data_list  (GList            *list);
GList*       file_data_list_dup            (GList            *list);
void         file_data_list_free           (GList            *list);
GList*       file_data_list_find_path      (GList            *list,
					    const char       *path);
char *       file_data_local_path          (FileData         *fd);

#endif /* FILE_DATA_H */
