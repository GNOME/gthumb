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

#include <glib.h>
#include "image-list.h"
#include "file-data.h"


GList *
ilist_utils_get_file_list_selection (ImageList *ilist)
{
	gint row;
	FileData *fd;
	GList *scan, *file_list;

	file_list = NULL;
	scan = ilist->selection;
	while (scan) {
		row = GPOINTER_TO_INT (scan->data);
		fd = image_list_get_image_data (ilist, row);
		file_list = g_list_prepend (file_list, g_strdup (fd->path));

		scan =  scan->next;
	}
	file_list = g_list_reverse (file_list);

	return file_list;
}


gboolean 
ilist_utils_row_is_selected (ImageList *ilist, 
			     gint row)
{
        GList *scan = ilist->selection;

        while (scan) {
                if (GPOINTER_TO_INT (scan->data) == row) return TRUE;
                scan = scan->next;
	}

        return FALSE;
}


gboolean
ilist_utils_only_one_is_selected (ImageList *ilist)
{
	if (ilist->selection == NULL)
		return FALSE;
	return (ilist->selection->next == NULL);
}


gboolean
ilist_utils_selection_not_null (ImageList *ilist)
{
	return (ilist->selection != NULL);
}
