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
        GSList    *keywords;
	gboolean   utf8_format;     /* TRUE if text is saved in UTF8 format.
				     * gthumb for GNOME 1.x saved text in
				     * locale format, gthumb for
				     * GNOME 2.x saves in utf8 format. */
	gboolean   changed;
} CommentData;


CommentData *  comment_data_new                    (void);
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
						    gboolean     resolve_symlinks);
void           comment_copy                        (const char  *src,
						    const char  *dest);
void           comment_move                        (const char  *src,
						    const char  *dest);
void           comment_delete                      (const char  *uri);
CommentData *  comments_load_comment               (const char  *uri,
						    gboolean     try_embedded);
void           comments_save_comment               (const char  *uri,
						    CommentData *data);
void           comments_save_comment_non_null      (const char  *uri,
						    CommentData *data);
void           comments_save_tags                  (const char  *uri,
						    CommentData *data);
char *         comments_get_comment_as_string      (CommentData *data,
						    char        *sep1,
						    char        *sep2);
char *         comments_get_tags_as_string         (CommentData *data,
						    char        *sep);
char*          _g_escape_text_for_html             (const gchar *text,
						    gssize       length);


#endif /* COMMENTS_H */
