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
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <config.h>
#include <string.h>
#include "glib-utils.h"


static void
test_g_rand_string (void)
{
	char *id;

	id = _g_rand_string (8);
	g_assert_cmpint (strlen (id), == , 8);
	g_free (id);

	id = _g_rand_string (16);
	g_assert_cmpint (strlen (id), == , 16);
	g_free (id);
}


static void
test_regexp (void)
{
	GRegex   *re;
	char    **a;
	int       i, n, j;
	char    **b;
	char     *attributes;

	re = g_regex_new ("%prop\\{([^}]+)\\}", 0, 0, NULL);
	a = g_regex_split (re, "thunderbird -compose %prop{ Exif::Image::DateTime } %ask %prop{ File::Size }", 0);
	for (i = 0, n = 0; a[i] != NULL; i += 2)
		n++;

	b = g_new (char *, n + 1);
	for (i = 1, j = 0; a[i] != NULL; i += 2) {
		b[j++] = g_strstrip (a[i]);
	}
	b[j] = NULL;
	attributes = g_strjoinv (",", b);

	g_assert_cmpstr (attributes, ==, "Exif::Image::DateTime,File::Size");

	g_free (attributes);
	g_free (b);
	g_strfreev (a);
	g_regex_unref (re);
}


static void
test_g_utf8_has_prefix (void)
{
	g_assert_true (_g_utf8_has_prefix ("lang=正體字/繁體字 中华人民共和国", "lang="));
}


static void
test_g_utf8_first_space (void)
{
	g_assert_cmpint (_g_utf8_first_ascii_space (NULL), ==, -1);
	g_assert_cmpint (_g_utf8_first_ascii_space (""), ==, -1);
	g_assert_cmpint (_g_utf8_first_ascii_space ("lang=FR langue d’oïl"), ==, 7);
	g_assert_cmpint (_g_utf8_first_ascii_space ("正體字"), ==, -1);
	g_assert_cmpint (_g_utf8_first_ascii_space ("lang=正體字/繁體字 中华人民共和国"), ==, 12);
}


static void
test_remove_lang_from_utf8_string (const char *value,
				   const char *expected)
{
	char *result = NULL;

	if (_g_utf8_has_prefix (value, "lang=")) {
		int pos = _g_utf8_first_ascii_space (value);
		if (pos > 0)
			result = _g_utf8_remove_prefix (value, pos + 1);
	}

	g_assert_true (result != NULL);
	g_assert_true (g_utf8_collate (result, expected) == 0);

	g_free (result);
}


static void
test_remove_lang_from_utf8_string_all (void)
{
	test_remove_lang_from_utf8_string ("lang=EN hello", "hello");
	test_remove_lang_from_utf8_string ("lang=FR langue d’oïl", "langue d’oïl");
	test_remove_lang_from_utf8_string ("lang=正體字/繁體字 中华人民共和国", "中华人民共和国");
}


int
main (int   argc,
      char *argv[])
{
	g_test_init (&argc, &argv, NULL);

	g_test_add_func ("/glib-utils/_g_rand_string/1", test_g_rand_string);
	g_test_add_func ("/glib-utils/regex", test_regexp);
	g_test_add_func ("/glib-utils/_g_utf8_has_prefix/1", test_g_utf8_has_prefix);
	g_test_add_func ("/glib-utils/_g_utf8_first_space/1", test_g_utf8_first_space);
	g_test_add_func ("/glib-utils/remove_lang_from_utf8_string/1", test_remove_lang_from_utf8_string_all);

	return g_test_run ();
}
