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


char *
_g_strdup_with_max_size (const char *s,
			 int         max_size)
{
	char *result;
	int   l = strlen (s);

	if (l > max_size) {
		char *first_half;
		char *second_half;
		int   offset;
		int   half_max_size = max_size / 2;

		first_half = g_strndup (s, half_max_size);
		offset = half_max_size + l - max_size;
		second_half = g_strndup (s + offset, half_max_size);

		result = g_strconcat (first_half, "...", second_half, NULL);

		g_free (first_half);
		g_free (second_half);
	} else
		result = g_strdup (s);

	return result;
}


/**
 * example 1 : "xxx##yy#" --> [0] = xxx
 *                            [1] = ##
 *                            [2] = yy
 *                            [3] = #
 *                            [4] = NULL
 *
 * example 2 : ""         --> [0] = NULL
 **/
char **
_g_get_template_from_text (const char *template)
{
	const char  *chunk_start = template;
	char       **str_vect;
	GList       *str_list = NULL, *scan;
	int          n = 0;

	if (template == NULL)
		return NULL;

	while (*chunk_start != 0) {
		gboolean    reading_sharps;
		char       *chunk;
		const char *chunk_end;

		reading_sharps = (*chunk_start == '#');
		chunk_end = chunk_start;

		while (reading_sharps 
		       && (*chunk_end != 0) 
		       && (*chunk_end == '#'))
			chunk_end++;
		
		while (! reading_sharps 
		       && (*chunk_end != 0) 
		       && (*chunk_end != '#'))
			chunk_end++;
		
		chunk = g_strndup (chunk_start, chunk_end - chunk_start);
		str_list = g_list_prepend (str_list,  chunk);
		n++;

		chunk_start = chunk_end;
	}

	str_vect = g_new (char*, n + 1);

	str_vect[n--] = NULL;
	for (scan = str_list; scan; scan = scan->next)
		str_vect[n--] = scan->data;

	g_list_free (str_list);

	return str_vect;
}


char *
_g_get_name_from_template (char **template,
			   int    n)
{
	GString *s;
	int      i;
	char    *result;

	s = g_string_new (NULL);

	for (i = 0; template[i] != NULL; i++) {
		const char *chunk = template[i];

		if (*chunk != '#')
			g_string_append (s, chunk);
		else {
			char *s_n;
			int   s_n_len;
			int   sharps_len = strlen (chunk);

			s_n = g_strdup_printf ("%d", n);
			s_n_len = strlen (s_n);

			while (s_n_len < sharps_len) {
				g_string_append_c (s, '0');
				sharps_len--;
			}
				
			g_string_append (s, s_n);
			g_free (s_n);
		}
	}

	result = s->str;
	g_string_free (s, FALSE);

	return result;
}


char *
_g_substitute (const char *from,
	       const char  this,
	       const char *with_this)
{
	char       *result;
	GString    *r;
	const char *s;

	if ((from == NULL) || (with_this == NULL))
		return g_strdup ("");

	if (strchr (from, this) == NULL)
		return g_strdup (from);

	r = g_string_new (NULL);
	for (s = from; *s != 0; s++) 
		if (*s == this)
			g_string_append (r, with_this);
		else
			g_string_append_c (r, *s);

	result = r->str;
	g_string_free (r, FALSE);

	return result;
}
