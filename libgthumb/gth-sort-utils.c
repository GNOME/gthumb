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
#include <libgnomevfs/gnome-vfs-utils.h>
#include "file-data.h"
#include "file-utils.h"
#include "gth-exif-utils.h"
#include "gth-sort-utils.h"


/* Files that start with these characters sort after files that don't. */
#define SORT_LAST_CHAR1 '.'
#define SORT_LAST_CHAR2 '#'


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
        if (string2 == NULL)
                return 1;
        if (string1 == NULL)
                return -1;

	collate_result = g_utf8_collate ( g_utf8_casefold (string1,-1), g_utf8_casefold (string2,-1) );

	if (collate_result)
		return collate_result;
	else
		return name_result;
}


int gth_sort_by_size_then_name (GnomeVFSFileSize  size1,
                                GnomeVFSFileSize  size2,
				const char       *name1,
				const char       *name2)
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
gth_sort_by_filename_but_ignore_path (const char *name1,
				      const char *name2)
{
	/* This sorts by the filename. It ignores the path portion, if present. */

	/* Based heavily on the Nautilus compare_by_display_name (libnautilus-private/nautilus-file.c)
	   function, for consistent Nautilus / gthumb behaviour. */

	char     *key_1, *key_2;
	gboolean  sort_last_1, sort_last_2;
	int       compare;
	char     *unesc_name1, *unesc_name2;

	sort_last_1 = file_name_from_path (name1)[0] == SORT_LAST_CHAR1
			|| file_name_from_path (name1)[0] == SORT_LAST_CHAR2;
	sort_last_2 = file_name_from_path (name2)[0] == SORT_LAST_CHAR1
			|| file_name_from_path (name2)[0] == SORT_LAST_CHAR2;

	if (sort_last_1 && !sort_last_2) {
		compare = +1;
	} else if (!sort_last_1 && sort_last_2) {
		compare = -1;
	} else {
		unesc_name1 = gnome_vfs_unescape_string (name1, "");
		unesc_name2 = gnome_vfs_unescape_string (name2, "");

		key_1 = g_utf8_collate_key_for_filename (file_name_from_path (unesc_name1), -1);
		key_2 = g_utf8_collate_key_for_filename (file_name_from_path (unesc_name2), -1);

		compare = strcmp (key_1, key_2);

		g_free(key_1);
		g_free(key_2);
		g_free(unesc_name1);
		g_free(unesc_name2);
	}

	return compare;
}


int gth_sort_by_full_path (const char *path1,
			   const char *path2)
{
	return uricmp (path1, path2);
}


int
gth_sort_none (gconstpointer  ptr1,
                gconstpointer  ptr2)
{
        return 0;
}

