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

#ifndef SEARCH_H
#define SEARCH_H

#include <time.h>

typedef enum { /*< skip >*/
	DATE_ANY      = 0,
	DATE_BEFORE   = 1,
	DATE_EQUAL_TO = 2,
	DATE_AFTER    = 3
} DateScope;


typedef struct {
	char      *start_from;
	gboolean   recursive;
	char      *file_pattern;
	char      *comment_pattern;
	char      *place_pattern;
	char      *keywords_pattern; 
	gboolean   all_keywords;
	time_t     date;
	DateScope  date_scope;
} SearchData;


SearchData* search_data_new                  ();

void        search_data_free                 (SearchData  *data);

void        search_data_set_start_from       (SearchData  *data,
					      const char  *start_from);

void        search_data_set_recursive        (SearchData  *data,
					      gboolean     recursive);

void        search_data_set_file_pattern     (SearchData  *data,
					      const gchar *file_pattern);

void        search_data_set_comment_pattern  (SearchData  *data,
					      const gchar *comment_pattern);

void        search_data_set_place_pattern    (SearchData  *data,
					      const gchar *place_pattern);

void        search_data_set_keywords_pattern (SearchData  *data,
					      const gchar *keywords_pattern,
					      gboolean     all_keywords);

void        search_data_set_date             (SearchData  *data,
					      time_t       date);

void        search_data_set_date_scope       (SearchData  *data,
					      DateScope    date_scope);

void        search_data_copy                 (SearchData  *dest,
					      SearchData  *source);

gchar **    search_util_get_patterns         (const char  *pattern_string);


#endif /* SEARCH_H */
