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

#include <string.h>
#include <strings.h>
#include <glib.h>
#include <gnome.h>
#include "file-data.h"
#include "file-utils.h"
#include "gth-exif-utils.h"
#include "gth-sort-utils.h"


/* Files that start with these characters sort after files that don't. */
#define SORT_LAST_CHAR1 '.'
#define SORT_LAST_CHAR2 '#'


int
case_insens_utf8_cmp (const char *string1,
                      const char *string2)
{
	gchar    *case_insens1;
	gchar    *case_insens2;
	gboolean  result;

	if ((string1 == NULL) && (string2 == NULL))
                return 0;
        if (string2 == NULL)
                return 1;
        if (string1 == NULL)
                return -1;

	case_insens1 = g_utf8_casefold (string1,-1);
	case_insens2 = g_utf8_casefold (string2,-1);

	result = g_utf8_collate (case_insens1, case_insens2);

	g_free (case_insens1);
	g_free (case_insens2);

	return result;
}


int
gth_sort_by_comment_then_name (const char *string1,
			       const char *string2,
			       const char *name1,
			       const char *name2)
{
	int collate_result;
	int name_result;

	name_result = gth_sort_by_filename_but_ignore_path (name1, name2);

	if ((string1 == NULL) && (string2 == NULL))
                return name_result;

	collate_result = case_insens_utf8_cmp (string1, string2);
	if (collate_result)
		return collate_result;
	else
		return name_result;
}


int gth_sort_by_size_then_name (goffset     size1,
                                goffset     size2,
				const char *name1,
				const char *name2)
{
	if (size1 < size2) return -1;
	if (size1 > size2) return 1;

	return gth_sort_by_filename_but_ignore_path (name1, name2);
}


int gth_sort_by_filetime_then_name (time_t      time1,
 				    time_t      time2,
				    const char *name1,
				    const char *name2)
{
	if (time1 < time2) return -1;
	if (time1 > time2) return 1;

	return gth_sort_by_filename_but_ignore_path (name1, name2);
}


int
gth_sort_by_exiftime_then_name (FileData *fd1,
		                FileData *fd2)
{
	time_t time1, time2;

	time1 = get_exif_time_or_mtime (fd1);
	time2 = get_exif_time_or_mtime (fd2);

	if (time1 < time2) return -1;
	if (time1 > time2) return 1;

	return gth_sort_by_filename_but_ignore_path (fd1->path, fd2->path);
}

int
gth_sort_by_filename (const char *name1,
                      const char *name2,
                      gboolean    ignore_path)
{
	char     *key_1, *key_2;
	gboolean  sort_last_1, sort_last_2;
	int       compare;
	char     *utf8_name1, *utf8_name2;

        if (name2 == NULL)
                return 1;
        if (name1 == NULL)
                return -1;

	sort_last_1 = file_name_from_path (name1)[0] == SORT_LAST_CHAR1
			|| file_name_from_path (name1)[0] == SORT_LAST_CHAR2;
	sort_last_2 = file_name_from_path (name2)[0] == SORT_LAST_CHAR1
			|| file_name_from_path (name2)[0] == SORT_LAST_CHAR2;

	if (sort_last_1 && !sort_last_2) {
		compare = +1;
	} else if (!sort_last_1 && sort_last_2) {
		compare = -1;
	} else {
		utf8_name1 = get_utf8_display_name_from_uri (name1);
		utf8_name2 = get_utf8_display_name_from_uri (name2);

		if (ignore_path) {
			key_1 = g_utf8_collate_key_for_filename (file_name_from_path (utf8_name1), -1);
			key_2 = g_utf8_collate_key_for_filename (file_name_from_path (utf8_name2), -1);
		} else {
                        key_1 = g_utf8_collate_key_for_filename (utf8_name1, -1);
                        key_2 = g_utf8_collate_key_for_filename (utf8_name2, -1);
		}

		compare = strcmp (key_1, key_2);

		g_free(key_1);
		g_free(key_2);
		g_free(utf8_name1);
		g_free(utf8_name2);
	}

	return compare;
}


int
gth_sort_by_filename_but_ignore_path (const char *name1,
				      const char *name2)
{
	return gth_sort_by_filename (name1, name2, TRUE);
}


int gth_sort_by_full_path (const char *path1,
			   const char *path2)
{
	return gth_sort_by_filename (path1, path2, FALSE);
}


int
gth_sort_none (gconstpointer  ptr1,
                gconstpointer  ptr2)
{
        return 0;
}

