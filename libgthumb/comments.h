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

#include <glib.h>
#include <time.h>

#ifndef COMMENTS_H
#define COMMENTS_H


typedef struct {
	char      *place;
	time_t     time;
	char      *comment;
	char     **keywords;
	int        keywords_n;
	gboolean   utf8_format;
} CommentData;


CommentData *  comment_data_new                    ();

void           comment_data_free                   (CommentData *data);

CommentData *  comment_data_dup                    (CommentData *data);

void           comment_data_free_keywords          (CommentData *data);

void           comment_data_free_comment           (CommentData *data);

void           comment_data_remove_keyword         (CommentData *data,
						    const char  *keyword);

void           comment_data_add_keyword            (CommentData *data,
						    const char  *keyword);

gboolean       comment_data_is_void                (CommentData *data);


/* -- */

gchar *        comments_get_comment_filename       (const gchar *source);

gchar *        comments_get_comment_dir            (const gchar *directory);

void           comment_copy                        (const gchar *src,
						    const gchar *dest);

void           comment_move                        (const gchar *src,
						    const gchar *dest);

void           comment_delete                      (const gchar *filename);

void           comments_remove_old_comments        (const gchar *dir, 
						    gboolean     recursive, 
						    gboolean     clear_all);

void           comments_remove_old_comments_async  (const gchar *dir, 
						    gboolean     recursive, 
						    gboolean     clear_all);

CommentData *  comments_load_comment               (const gchar *filename);

void           comments_save_comment               (const gchar *filename,
						    CommentData *data);

void           comments_save_comment_non_null      (const gchar *filename,
						    CommentData *data);

void           comments_save_categories            (const gchar *filename,
						    CommentData *data);

char *         comments_get_comment_as_string      (CommentData *data,
						    char        *sep1,
						    char        *sep2);


#endif /* COMMENTS_H */
