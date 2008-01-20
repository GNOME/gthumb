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

#include <string.h>
#include <glib.h>
#include <libgnomevfs/gnome-vfs-types.h>
#include <libgnomevfs/gnome-vfs-file-info.h>
#include <libgnomevfs/gnome-vfs-ops.h>
#include <libgnomevfs/gnome-vfs-utils.h>
#include "file-data.h"
#include "glib-utils.h"
#include "file-utils.h"
#include "comments.h"
#include "gth-exif-utils.h"


#define MAX_COMMENT_LEN 60


GType
file_data_get_type (void)
{
        static GType type = 0;
  
        if (type == 0)
                type = g_boxed_type_register_static ("GthFileData", (GBoxedCopyFunc) file_data_ref, (GBoxedFreeFunc) file_data_unref);
  
        return type;
}


FileData *
file_data_new (const char       *path,
	       GnomeVFSFileInfo *info)
{
	FileData *fd;

	fd = g_new0 (FileData, 1);

	fd->ref = 1;
	fd->path = add_scheme_if_absent (path);
	fd->name = file_name_from_path (fd->path);
	fd->display_name = gnome_vfs_unescape_string_for_display (fd->name);
	if (info != NULL) {
		if (info->valid_fields | GNOME_VFS_FILE_INFO_FIELDS_SIZE)
			fd->size = info->size;
		if (info->valid_fields | GNOME_VFS_FILE_INFO_FIELDS_CTIME)
			fd->ctime = info->ctime;
		if (info->valid_fields | GNOME_VFS_FILE_INFO_FIELDS_MTIME)
			fd->mtime = info->mtime;
		if (info->valid_fields | GNOME_VFS_FILE_INFO_FIELDS_MIME_TYPE)
			fd->mime_type = get_static_string (info->mime_type);
	}
	else {
		fd->size = (GnomeVFSFileSize) 0;
		fd->ctime = (time_t) 0;
		fd->mtime = (time_t) 0;
	}

	/* The Exif DateTime tag is only recorded on an as-needed basis during
	   DateTime sorts. The tag in memory is refreshed if the file mtime has
	   changed, so it is recorded as well. */

	fd->exif_data_loaded = FALSE;
	fd->exif_time = 0;
	fd->metadata = NULL;

	fd->error = FALSE;
	fd->thumb_loaded = FALSE;
	fd->thumb_created = FALSE;
	fd->comment = g_strdup ("");

	return fd;
}


FileData *
file_data_new_from_local_path (const char *path)
{
	FileData *fd;
	char     *uri;
	
	uri = get_uri_from_local_path (path);
	fd = file_data_new (uri, NULL);
	file_data_update (fd);
	g_free (uri);
	
	return fd;
}


FileData *
file_data_ref (FileData *fd)
{
	g_return_val_if_fail (fd != NULL, NULL);
	fd->ref++;
	return fd;
}


FileData *
file_data_dup (FileData *source)
{
	FileData *fd;
	
	if (source == NULL)
		return NULL;
	
	fd = g_new0 (FileData, 1);

	fd->ref = 1;
	fd->path = g_strdup (source->path);
	fd->name = file_name_from_path (fd->path);
	fd->display_name = g_strdup (source->display_name);
	fd->mime_type = get_static_string (source->mime_type);
	fd->size = source->size;
	fd->ctime = source->ctime;
	fd->mtime = source->mtime;
	fd->exif_data_loaded = source->exif_data_loaded;
	fd->exif_time = source->exif_time;
	fd->metadata = dup_metadata (source->metadata);
	fd->error = source->error;
	fd->thumb_loaded = source->thumb_loaded;
	fd->thumb_created = source->thumb_created;
	fd->comment = (source->comment != NULL) ? g_strdup (source->comment) : NULL;
	fd->comment_data = comment_data_dup (source->comment_data);
	
	return fd;
}

void
file_data_unref (FileData *fd)
{
	if (fd == NULL)
		return;

	fd->ref--;

	if (fd->ref == 0) {
		g_free (fd->path);
		g_free (fd->display_name);
		if (fd->comment_data != NULL)
			comment_data_free (fd->comment_data);
		g_free (fd->comment);
		if (fd->metadata != NULL)
			free_metadata (fd->metadata);
		g_free (fd);
	}
}


void
file_data_update (FileData *fd)
{
	GnomeVFSFileInfo *info;
	GnomeVFSResult    result;
	time_t		  old_mtime;

	g_return_if_fail (fd != NULL);

	fd->error = FALSE;
	fd->thumb_loaded = FALSE;
	fd->thumb_created = FALSE;

	old_mtime = fd->mtime;

	info = gnome_vfs_file_info_new ();
	result = gnome_vfs_get_file_info (fd->path,
					  info,
					  (GNOME_VFS_FILE_INFO_FOLLOW_LINKS 
					   | GNOME_VFS_FILE_INFO_GET_MIME_TYPE 
					   | GNOME_VFS_FILE_INFO_FORCE_FAST_MIME_TYPE));

	if (result != GNOME_VFS_OK) {
		fd->error = TRUE;
		fd->size = 0L;
		fd->mtime = 0;
		fd->ctime = 0;
		fd->exif_data_loaded = FALSE;
		fd->mime_type = NULL;
		return;
	}

	fd->name = file_name_from_path (fd->path);

	g_free (fd->display_name);
	fd->display_name = gnome_vfs_unescape_string_for_display (fd->name);

	fd->mime_type = get_static_string (info->mime_type);
	fd->size = info->size;
	fd->mtime = info->mtime;
	fd->ctime = info->ctime;

	/* update metadata only if required */
	if ((old_mtime != fd->mtime) && fd->exif_data_loaded) {
		fd->exif_data_loaded = FALSE;
		if (fd->metadata != NULL)
                        free_metadata (fd->metadata);
		file_data_insert_metadata (fd);	
		}

	gnome_vfs_file_info_unref (info);
}


/* doesn't update the mime-type */
void
file_data_update_info (FileData *fd)
{
	GnomeVFSFileInfo *info;
	GnomeVFSResult    result;
	time_t            old_mtime;

	g_return_if_fail (fd != NULL);

	fd->error = FALSE;
	fd->thumb_loaded = FALSE;
	fd->thumb_created = FALSE;

	old_mtime = fd->mtime;

	info = gnome_vfs_file_info_new ();
	result = gnome_vfs_get_file_info (fd->path,
					  info,
					  (GNOME_VFS_FILE_INFO_FOLLOW_LINKS 
					   | GNOME_VFS_FILE_INFO_GET_MIME_TYPE 
					   | GNOME_VFS_FILE_INFO_FORCE_FAST_MIME_TYPE));

	if (result != GNOME_VFS_OK) {
		fd->error = TRUE;
		fd->size = 0L;
		fd->mtime = 0;
		fd->ctime = 0;
		fd->exif_data_loaded = FALSE;
		fd->mime_type = NULL;
		return;
	}

	fd->name = file_name_from_path (fd->path);

	g_free (fd->display_name);
	fd->display_name = gnome_vfs_unescape_string_for_display (fd->name);

	fd->size = info->size;
	fd->mtime = info->mtime;
	fd->ctime = info->ctime;

        /* update metadata only if required */
        if ((old_mtime != fd->mtime) && fd->exif_data_loaded) {
		fd->exif_data_loaded = FALSE;
                if (fd->metadata != NULL)
                        free_metadata (fd->metadata);
                file_data_insert_metadata (fd);
                }

	gnome_vfs_file_info_unref (info);
}


void
file_data_update_mime_type (FileData *fd,
			    gboolean  fast_mime_type)
{
	fd->mime_type = get_file_mime_type (fd->path, fast_mime_type || ! is_local_file (fd->path));
}


void
file_data_update_all (FileData *fd,
		      gboolean  fast_mime_type)
{
	file_data_update_info (fd);
	file_data_update_mime_type (fd, fast_mime_type);
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

void
file_data_insert_metadata (FileData *fd)
{
	g_return_if_fail (fd != NULL);

	if (fd->exif_data_loaded)
		return;

	fd->metadata = update_metadata (fd->metadata, fd->path, fd->mime_type);
	fd->exif_time = get_metadata_time_from_fd (fd);
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
file_data_list_from_uri_list (GList *list)
{
	GList *result = NULL;
	GList *scan;
	
	for (scan = list; scan; scan = scan->next) {
		char *path = scan->data;
		result = g_list_prepend (result, file_data_new (path, NULL));
	}
	
	return g_list_reverse (result);
}


GList*
uri_list_from_file_data_list (GList *list)
{
	GList *result = NULL;
	GList *scan;
	
	for (scan = list; scan; scan = scan->next) {
		FileData *fd = scan->data;
		result = g_list_prepend (result, g_strdup (fd->path));
	}
	
	return g_list_reverse (result);	
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


GList*
file_data_list_find_path (GList      *list,
			  const char *path)
{
	GList *scan;
	
	for (scan = list; scan; scan = scan->next) {
		FileData *data = scan->data;
		
		if (strcmp (data->path, path) == 0)
			return scan;
	}
	
	return NULL;
}


char *
file_data_local_path (FileData *fd)
{
	return get_local_path_from_uri (fd->path);
}
