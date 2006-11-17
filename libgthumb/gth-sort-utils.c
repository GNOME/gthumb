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
#include <glib.h>
#include <gnome.h>

gint gth_sort_by_comment_then_name (const gchar *string1, const gchar *string2,
			  const gchar *name1, const gchar *name2)
{
	gint collate_result;
	gint name_result;

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

gint gth_sort_by_size_then_name (GnomeVFSFileSize size1, GnomeVFSFileSize size2,
				const gchar *name1, const gchar *name2)
{
	if (size1 < size2) return -1;
	if (size1 > size2) return 1;

	return gth_sort_by_filename_but_ignore_path (name1, name2);
}


gint gth_sort_by_filetime_then_name (time_t time1, time_t time2,
				const gchar *name1, const gchar *name2)
{
	if (time1 < time2) return -1;
	if (time1 > time2) return 1;

	return gth_sort_by_filename_but_ignore_path (name1, name2);
}


gint gth_sort_by_filename_but_ignore_path (const gchar *name1, const gchar *name2)
{
	/* This sorts by the filename. It ignores the path portion, if present. */

	return strcasecmp (file_name_from_path (name1), 
			   file_name_from_path (name2));
}
	

gint gth_sort_by_full_path (const char *path1, const char *path2)
{
	return uricmp (path1, path2);
}

