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

#include <glib.h>
#include <time.h>

#ifndef COMMENTS_H
#define COMMENTS_H


typedef struct {
	char      *place;           /* All strings contain text in utf8 
				     * format. */
	time_t     time;
	char      *comment;
	char     **keywords;
	int        keywords_n;
	gboolean   utf8_format;     /* TRUE if text is saved in UTF8 format. 
				     * gthumb for GNOME 1.x saved text in 
				     * locale format, gthumb for
				     * GNOME 2.x saves in utf8 format. */
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

gboolean       comment_text_is_void                (CommentData *data);

/* -- */

char *         comments_get_comment_filename       (const char  *source,
						    gboolean     resolve_symlinks,
						    gboolean     unescape);

char *         comments_get_comment_dir            (const char  *directory,
						    gboolean     resolve_symlinks,
						    gboolean     unescape);

void           comment_copy                        (const char  *src,
						    const char  *dest);

void           comment_move                        (const char  *src,
						    const char  *dest);

void           comment_delete                      (const char  *filename);

void           comments_remove_old_comments        (const char  *dir, 
						    gboolean     recursive, 
						    gboolean     clear_all);

void           comments_remove_old_comments_async  (const char  *dir, 
						    gboolean     recursive, 
						    gboolean     clear_all);

CommentData *  comments_load_comment               (const char  *filename);

void           comments_save_comment               (const char  *filename,
						    CommentData *data);

void           comments_save_comment_non_null      (const char  *filename,
						    CommentData *data);

void           comments_save_categories            (const char  *filename,
						    CommentData *data);

char *         comments_get_comment_as_string      (CommentData *data,
						    char        *sep1,
						    char        *sep2);

char *         comments_get_comment_as_xml_string  (CommentData *data,
						    char        *sep1,
						    char        *sep2);

char*          _g_escape_text_for_html             (const gchar *text,
						    gssize       length);


#endif /* COMMENTS_H */
