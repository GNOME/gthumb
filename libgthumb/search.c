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

#include <string.h>
#include <glib.h>
#include "search.h"
#include "glib-utils.h"


SearchData*
search_data_new ()
{
	SearchData *data;

	data = g_new (SearchData, 1);

	data->start_from         = NULL;
	data->recursive          = FALSE;
	data->file_pattern       = NULL;
	data->comment_pattern    = NULL;
	data->place_pattern      = NULL;
	data->keywords_pattern   = NULL;
	data->all_keywords       = FALSE;
	data->date               = 0;
	data->date_scope         = DATE_ANY;

	return data;
}


void
search_data_free (SearchData *data)
{
	if (data == NULL)
		return;

	if (data->file_pattern) {
		g_free (data->file_pattern);
		data->file_pattern = NULL;
	}

	if (data->comment_pattern) {
		g_free (data->comment_pattern);
		data->comment_pattern = NULL;
	}

	if (data->place_pattern) {
		g_free (data->place_pattern);
		data->place_pattern = NULL;
	}

	if (data->keywords_pattern) {
		g_free (data->keywords_pattern);
		data->keywords_pattern = NULL;
	}

	if (data->start_from) {
		g_free (data->start_from);
		data->start_from = NULL;
	}

	g_free (data);
}


static void
set_string (gchar **dest,
	    const gchar *source)
{
	if (*dest)
		g_free (*dest);
	*dest = g_strdup (source);
}


void
search_data_set_start_from (SearchData *data,
			    const gchar *start_from)
{
	g_return_if_fail (data != NULL);
	set_string (& (data->start_from), start_from);
}


void
search_data_set_recursive (SearchData *data,
			   gboolean recursive)
{
	g_return_if_fail (data != NULL);
	data->recursive = recursive;
}


void
search_data_set_file_pattern (SearchData *data,
			      const gchar *pattern)
{
	g_return_if_fail (data != NULL);
	set_string (& (data->file_pattern), pattern);
}


void
search_data_set_comment_pattern (SearchData *data,
				 const gchar *pattern)
{
	g_return_if_fail (data != NULL);
	set_string (& (data->comment_pattern), pattern);
}


void
search_data_set_place_pattern (SearchData *data,
			       const gchar *pattern)
{
	g_return_if_fail (data != NULL);
	set_string (& (data->place_pattern), pattern);
}


void
search_data_set_keywords_pattern (SearchData *data,
				  const char *pattern,
				  gboolean    all_keywords)
{
	g_return_if_fail (data != NULL);
	set_string (& (data->keywords_pattern), pattern);
	data->all_keywords = all_keywords;
}


void
search_data_set_date (SearchData *data,
		      time_t date)
{
	g_return_if_fail (data != NULL);
	data->date = date;
}


void
search_data_set_date_scope (SearchData *data,
			    DateScope date_scope)
{
	g_return_if_fail (data != NULL);
	data->date_scope = date_scope;
}


void
search_data_copy (SearchData *dest,
		  SearchData *source)
{
	g_return_if_fail (dest != NULL);
	g_return_if_fail (source != NULL);

	search_data_set_start_from       (dest, source->start_from);
	search_data_set_recursive        (dest, source->recursive);
	search_data_set_file_pattern     (dest, source->file_pattern);
	search_data_set_comment_pattern  (dest, source->comment_pattern);
	search_data_set_place_pattern    (dest, source->place_pattern);
	search_data_set_keywords_pattern (dest, source->keywords_pattern,
					  source->all_keywords);
	search_data_set_date             (dest, source->date);
	search_data_set_date_scope       (dest, source->date_scope);
}


char **
search_util_get_patterns (const char *pattern_string,
			  gboolean    exact_match)
{
	char  *case_pattern;
	char **patterns;
	int    i;

	case_pattern = g_utf8_casefold (pattern_string, -1);
	patterns = _g_utf8_strsplit (case_pattern, ';');
	g_free (case_pattern);

	for (i = 0; patterns[i] != NULL; i++) {
		char *stripped = _g_utf8_strstrip (patterns[i]);

		if (stripped == NULL)
			continue;

		if (exact_match) {
			char *temp = patterns[i];
			patterns[i] = stripped;
			g_free (temp);
			continue;
		}

		if (g_utf8_strchr (stripped, -1, '*') == NULL) {
			char *temp = patterns[i];
			patterns[i] = g_strconcat ("*", stripped, "*", NULL);
			g_free (temp);
		}

		g_free (stripped);
	}

	return patterns;
}


char **
search_util_get_file_patterns (const char *pattern_string)
{
	char  *case_pattern;
	char **patterns;
	int    i;

	case_pattern = g_utf8_casefold (pattern_string, -1);
	patterns = _g_utf8_strsplit (case_pattern, ';');
	g_free (case_pattern);

	for (i = 0; patterns[i] != NULL; i++) {
		char *stripped = _g_utf8_strstrip (patterns[i]);

		if (stripped == NULL)
			continue;

		if (g_utf8_strchr (stripped, -1, '*') == NULL) {
			char *temp = patterns[i];
			patterns[i] = g_strconcat ("*", stripped, "*", NULL);
			g_free (temp);
		} else {
			char *temp = patterns[i];
			patterns[i] = g_strconcat ("*/", stripped, NULL);
			g_free (temp);
		}

		g_free (stripped);
	}

	return patterns;
}
