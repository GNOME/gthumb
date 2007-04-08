/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2001-2006 The Free Software Foundation, Inc.
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
#include <string.h>
#include <libgnomevfs/gnome-vfs-types.h>
#include <libgnomevfs/gnome-vfs-file-info.h>
#include <libgnomevfs/gnome-vfs-ops.h>
#include <libgnomevfs/gnome-vfs-utils.h>
#include "file-data.h"
#include "file-utils.h"
#include "glib-utils.h"
#include "comments.h"
#include "gth-exif-utils.h"


#define MAX_COMMENT_LEN 60


FileData *
file_data_new (const char       *path,
	       GnomeVFSFileInfo *info)
{
	FileData *fd;

	fd = g_new0 (FileData, 1);

	fd->ref = 1;
	fd->path = get_uri_from_path (path);
	fd->name = file_name_from_path (fd->path);
	fd->display_name = gnome_vfs_unescape_string_for_display (fd->name);
	if (info != NULL) {
		fd->size = info->size;
		fd->ctime = info->ctime;
		fd->mtime = info->mtime;

		if (info->mime_type != NULL)
			fd->mime_type = info->mime_type;
	}

	/* The Exif DateTime tag is only recorded on an as-needed basis during
	   DateTime sorts. The tag in memory is refreshed if the file mtime has
	   changed, so it is recorded as well. */

	fd->exif_data_loaded = FALSE;
	fd->exif_time = 0;

	fd->error = FALSE;
	fd->thumb_loaded = FALSE;
	fd->thumb_created = FALSE;
	fd->comment = g_strdup ("");

	fd->metadata_hash = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_free);
	fd->metadata_time = 0;

	return fd;
}


void
file_data_ref (FileData *fd)
{
	g_return_if_fail (fd != NULL);
	fd->ref++;
}


void
file_data_unref (FileData *fd)
{
	g_return_if_fail (fd != NULL);

	fd->ref--;

	if (fd->ref == 0) {
		g_hash_table_destroy (fd->metadata_hash);
		g_free (fd->path);
		g_free (fd->display_name);
		if (fd->comment_data != NULL)
			comment_data_free (fd->comment_data);
		g_free (fd->comment);

		g_free (fd);
	}
}


void
file_data_update (FileData *fd)
{
	GnomeVFSFileInfo *info;
	GnomeVFSResult    result;

	g_return_if_fail (fd != NULL);

	fd->error = FALSE;
	fd->thumb_loaded = FALSE;
	fd->thumb_created = FALSE;

	fd->mime_type = NULL;

	info = gnome_vfs_file_info_new ();
	result = gnome_vfs_get_file_info (fd->path,
					  info,
					  (GNOME_VFS_FILE_INFO_DEFAULT
					   | GNOME_VFS_FILE_INFO_FOLLOW_LINKS));

	if (result != GNOME_VFS_OK) {
		fd->error = TRUE;
		fd->size = 0L;
		fd->mtime = 0;
		fd->ctime = 0;
		fd->exif_data_loaded = FALSE;
		return;
	}

	fd->name = file_name_from_path (fd->path);

	g_free (fd->display_name);
	fd->display_name = gnome_vfs_unescape_string_for_display (fd->name);

	if (info->mime_type != NULL)
		fd->mime_type = info->mime_type;

	fd->size = info->size;
	fd->mtime = info->mtime;
	fd->ctime = info->ctime;
	fd->exif_data_loaded = FALSE;

	gnome_vfs_file_info_unref (info);
}


void
file_data_set_path (FileData   *fd,
		    const char *path)
{
	g_return_if_fail (fd != NULL);
	g_return_if_fail (path != NULL);

	g_free (fd->path);
	fd->path = g_strdup (path);

	file_data_update (fd);
}


void
file_data_load_comment_data (FileData *fd)
{
	g_return_if_fail (fd != NULL);

	if (fd->comment_data != NULL)
		return;
	fd->comment_data = comments_load_comment (fd->path, FALSE);
}


gboolean
compare_key (gpointer key, gpointer value, gpointer substring) 
{
	/* return true if the string is in the key name */
	return (strstr ((const char *) key, (const char *) substring) != NULL);
}


void
file_data_load_exif_data (FileData *fd)
{
	char       *time_string = NULL;
	const char *value;
	GTimeVal    time_val;

	g_return_if_fail (fd != NULL);

	/* date is already valid, return */
	if (fd->exif_data_loaded)
		return;

	/* if the image is a jpeg, use libexif to quickly
	   read the datetime tags */ 
	if ((fd->metadata_time == 0) && (image_is_jpeg (fd->path)))
		fd->exif_time = get_exif_time (fd->path);

	/* if that didn't work, load the metadata using the
	   slower exiftool functions */

	if (fd->exif_time == 0) {

		/* Read metadata from file, if not already loaded */
		if (fd->metadata_time == 0)
			fd->metadata_time = get_metadata_for_file (fd->path, 
								   fd->metadata_hash);

		/* Read tags in order of preference */
		value = g_hash_table_find (fd->metadata_hash, 
						   compare_key, 
						   "DateTimeOriginal");
		if (value == NULL)
			value = g_hash_table_find (fd->metadata_hash, 
						   compare_key, 
						   "CreateDate");
		if (value == NULL)
			value = g_hash_table_find (fd->metadata_hash, 
						   compare_key, 
						   "ModifyDate");
		if (value == NULL)
                        value = g_hash_table_find (fd->metadata_hash, 
                                                   compare_key, 
                                                   "DateTime");
		if (value != NULL)
			time_string = strip_sort_codes (value);

		if (time_string)
                	fd->exif_time = exif_string_to_time_t (time_string);

                g_free (time_string);
        }

	fd->exif_data_loaded = TRUE;
}


void
file_data_update_comment (FileData *fd)
{
	g_return_if_fail (fd != NULL);

	if (fd->comment != NULL)
		g_free (fd->comment);
	if (fd->comment_data != NULL)
		comment_data_free (fd->comment_data);

	fd->comment_data = comments_load_comment (fd->path, FALSE);

	if (fd->comment_data == NULL) {
		fd->comment = g_strdup ("");
		return;
	}

	fd->comment = comments_get_comment_as_string (fd->comment_data, "\n", "\n");
	if (fd->comment == NULL)
		fd->comment = g_strdup ("");
}


GList*
file_data_list_dup (GList *list)
{
	GList *new_list = NULL, *scan;

	if (list == NULL)
		return NULL;

	for (scan = list; scan; scan = scan->next) {
		FileData *data = scan->data;
		file_data_ref (data);
		new_list = g_list_prepend (new_list, data);
	}

	return g_list_reverse (new_list);
}


void
file_data_list_free (GList *list)
{
	if (list != NULL) {
		g_list_foreach (list, (GFunc) file_data_unref, NULL);
		g_list_free (list);
	}
}


const char *
file_data_local_path (FileData *fd)
{
	return get_file_path_from_uri (fd->path);
}
