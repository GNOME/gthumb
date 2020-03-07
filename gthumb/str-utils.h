/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2019 Free Software Foundation, Inc.
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
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _STR_UTILS_H
#define _STR_UTILS_H

#include <glib.h>

G_BEGIN_DECLS

typedef gboolean (*StripFunc) (gunichar ch);

gboolean	_g_str_equal		(const char	 *str1,
					 const char	 *str2);
gboolean	_g_str_n_equal		(const char	 *str1,
					 const char	 *str2,
					 gsize		  size);
void		_g_str_set		(char		**str,
					 const char	 *value);
char **		_g_strv_take_from_str_list
					(GList		 *str_list,
					 int		  size);
char *		_g_str_random		(int		  len);
char *		_g_str_remove_suffix	(const char	 *str,
					 const char	 *suffix);
const char *	_g_str_get_static	(const char	 *str);
GHashTable *	_g_str_split_as_hash_table
					(const char	 *str);

/* StrV utils */

int		_g_strv_find		(char		**strv,
					 const char	 *str);
gboolean	_g_strv_contains	(char		**strv,
					 const char	 *str);
char **		_g_strv_prepend		(char		**strv,
					 const char	 *str);
char **		_g_strv_concat		(char		**strv1,
					 char		**strv2);
gboolean	_g_strv_remove		(char		**strv,
					 const char	 *str);

/* UTF-8 utils */

char *		_g_utf8_strndup		(const char	 *str,
					 gssize		  size);
const char *	_g_utf8_find_str	(const char	 *haystack,
					 const char	 *needle);
char **		_g_utf8_split		(const char	 *str,
					 const char	 *separator,
					 int		  max_tokens);
char **		_g_utf8_split_template	(const char	 *tmpl);
char *		_g_utf8_replace_str	(const char	 *str,
					 const char	 *old_str,
					 const char	 *new_str);
char *		_g_utf8_replace_pattern	(const char	 *str,
					 const char	 *pattern,
					 const char	 *replacement);
char *		_g_utf8_last_char	(const char	 *str,
					 glong		 *p_size);
gboolean	_g_utf8_n_equal		(const char	 *str1,
					 const char	 *str2,
					 glong		  size);
const char *	_g_utf8_after_ascii_space
					(const char	 *str);
gboolean	_g_utf8_has_prefix	(const char	 *str,
					 const char	 *prefix);
gboolean	_g_utf8_all_spaces	(const char	 *str);
char *		_g_utf8_try_from_any	(const char	 *str);
char *		_g_utf8_from_any	(const char	 *str);
char *		_g_utf8_strip_func	(const char	 *str,
					 StripFunc	  is_space_func);
char *		_g_utf8_strip		(const char	 *str);
char *		_g_utf8_rstrip_func	(const char	 *str,
					 StripFunc	  func);
char *		_g_utf8_rstrip		(const char	 *str);
char *		_g_utf8_translate	(const char	 *str,
					 ...) G_GNUC_NULL_TERMINATED;
char *		_g_utf8_escape_xml	(const char	 *str);
char *		_g_utf8_text_escape_xml	(const char	 *str);

G_END_DECLS

#endif /* _STR_UTILS_H */
