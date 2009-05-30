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
#include "file-data.h"
#include "glib-utils.h"
#include "file-utils.h"
#include "gfile-utils.h"
#include "comments.h"
#include "gth-exif-utils.h"
#include "gtk-utils.h"

#define MAX_COMMENT_LEN 60


static void
fd_free_metadata (FileData *fd)
{
	free_metadata (fd->metadata);
	fd->metadata = NULL;
        fd->exif_data_loaded = FALSE;
	fd->exif_time = (time_t) 0;
}


GType
file_data_get_type (void)
{
        static GType type = 0;
  
        if (type == 0)
                type = g_boxed_type_register_static ("GthFileData", (GBoxedCopyFunc) file_data_ref, (GBoxedFreeFunc) file_data_unref);
  
        return type;
}


static void
load_info (FileData *fd)
{
	GFileInfo *info;
	GFile     *gfile;
	GFile     *gfile_resolved;
	GError    *error = NULL;
	GTimeVal   tv;
	char      *resolved_path;

	g_free (fd->local_path);
	gfile = gfile_new (fd->utf8_path);
	fd->local_path = g_file_get_path (gfile);

	if ( (fd->local_path != NULL) &&
	     ! is_local_file (fd->utf8_path) &&
	     ! strstr (fd->local_path, ".gvfs")) {
		/* This can happen when running gThumb over ssh with X-forwarding.
		   I don't know why, exactly. Possibly a gio bug. */
		g_warning ("Unexpected error: %s is not a valid mount point for %s",fd->local_path,fd->utf8_path);
		g_free (fd->local_path);
		fd->local_path = NULL;
	}

	g_free (fd->uri);

	resolved_path = resolve_all_symlinks (fd->utf8_path);
	gfile_resolved = gfile_new (resolved_path);
	fd->uri = g_file_get_uri (gfile_resolved);
	g_object_unref (gfile_resolved);
	g_free (resolved_path);

	info = g_file_query_info (gfile, 
				  G_FILE_ATTRIBUTE_STANDARD_SIZE ","
				  G_FILE_ATTRIBUTE_TIME_CHANGED ","
				  G_FILE_ATTRIBUTE_TIME_MODIFIED ","
				  G_FILE_ATTRIBUTE_ACCESS_CAN_READ ","
				  G_FILE_ATTRIBUTE_STANDARD_FAST_CONTENT_TYPE,
                                  G_FILE_QUERY_INFO_NONE,
                                  NULL,
                                  &error);

        if (error == NULL) {
		fd->size = g_file_info_get_size (info);
		fd->ctime = g_file_info_get_attribute_uint64 (info, G_FILE_ATTRIBUTE_TIME_CHANGED);
		g_file_info_get_modification_time (info, &tv);
		fd->mtime = tv.tv_sec;
		fd->can_read = g_file_info_get_attribute_boolean (info, G_FILE_ATTRIBUTE_ACCESS_CAN_READ);
		fd->mime_type = get_static_string (g_file_info_get_attribute_string (info, G_FILE_ATTRIBUTE_STANDARD_FAST_CONTENT_TYPE));
		g_object_unref (info);
        } else {
                // gfile_warning ("Failed to get file information", gfile, error);
                g_error_free (error);

		fd->size = (goffset) 0;
		fd->ctime = (time_t) 0;
		fd->mtime = (time_t) 0;
		fd->can_read = TRUE;
	}

	g_object_unref (gfile);
}


FileData *
file_data_new (const char *path)
{
	FileData *fd;

	if (path == NULL)
		return NULL;

	fd = g_new0 (FileData, 1);

	fd->ref = 1;
	fd->utf8_path = get_utf8_display_name_from_uri (path);
	fd->utf8_name = file_name_from_path (fd->utf8_path);

	load_info (fd);

	/* The Exif DateTime tag is only recorded on an as-needed basis during
	   DateTime sorts. The tag in memory is refreshed if the file mtime has
	   changed, so it is recorded as well. */

	fd_free_metadata (fd);

	fd->error = FALSE;
	fd->thumb_loaded = FALSE;
	fd->thumb_created = FALSE;
	fd->comment = g_strdup ("");
	fd->tags = g_strdup ("");

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
        fd->utf8_path = g_strdup (source->utf8_path);
        fd->utf8_name = file_name_from_path (fd->utf8_path);
	fd->local_path = g_strdup (source->local_path);
	fd->uri = g_strdup (source->uri);

	fd->mime_type = get_static_string (source->mime_type);
	fd->size = source->size;
	fd->ctime = source->ctime;
	fd->mtime = source->mtime;
	fd->can_read = source->can_read;
	fd->exif_data_loaded = FALSE;
	fd->exif_time = (time_t) 0;
	fd->metadata = NULL;
	fd->error = source->error;
	fd->thumb_loaded = source->thumb_loaded;
	fd->thumb_created = source->thumb_created;
	fd->comment = (source->comment != NULL) ? g_strdup (source->comment) : NULL;
	fd->tags = (source->tags != NULL) ? g_strdup (source->tags) : NULL;
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
		g_free (fd->utf8_path);
		g_free (fd->local_path);
		g_free (fd->uri);
		if (fd->comment_data != NULL)
			comment_data_free (fd->comment_data);
		g_free (fd->comment);
		g_free (fd->tags);
		fd_free_metadata (fd);
		g_free (fd);
	}
}


void
file_data_update (FileData *fd)
{
	g_return_if_fail (fd != NULL);

	fd->error = FALSE;
	fd->thumb_loaded = FALSE;
	fd->thumb_created = FALSE;
        fd->utf8_name = file_name_from_path (fd->utf8_path);

	load_info (fd);

	fd_free_metadata (fd);
}


void
file_data_update_mime_type (FileData *fd,
			    gboolean  fast_mime_type)
{
	fd->mime_type = get_file_mime_type (fd->utf8_path, fast_mime_type || ! is_local_file (fd->utf8_path));
}


void
file_data_update_all (FileData *fd,
		      gboolean  fast_mime_type)
{
	file_data_update (fd);
	file_data_update_mime_type (fd, fast_mime_type);
}


void
file_data_set_path (FileData   *fd,
		    const char *path)
{
	g_return_if_fail (fd != NULL);
	g_return_if_fail (path != NULL);

	g_free (fd->utf8_path);
        fd->utf8_path = get_utf8_display_name_from_uri (path);

	file_data_update (fd);
}


void
file_data_update_comment (FileData *fd)
{
	g_return_if_fail (fd != NULL);

	if (fd->comment != NULL)
		g_free (fd->comment);
	if (fd->comment_data != NULL)
		comment_data_free (fd->comment_data);

	fd->comment_data = comments_load_comment (fd->utf8_path, FALSE);

	if (fd->comment_data == NULL) {
                fd->comment_data = comment_data_new ();
		fd->comment = g_strdup ("");
		return;
	}

	fd->comment = comments_get_comment_as_string (fd->comment_data, "\n", "\n");
	if (fd->comment == NULL)
		fd->comment = g_strdup ("");

	fd->tags = comments_get_tags_as_string (fd->comment_data, ", ");
	if (fd->tags == NULL)
		fd->tags = g_strdup ("");
}


GList*
file_data_list_from_uri_list (GList *list)
{
	GList *result = NULL;
	GList *scan;
	
	for (scan = list; scan; scan = scan->next) {
		char *path = scan->data;
		result = g_list_prepend (result, file_data_new (path));
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
		
		if (strcmp (data->utf8_path, path) == 0)
			return scan;
	}
	
	return NULL;
}


gboolean
file_data_same_path (FileData   *fd1,
		     const char *path2)
{
	FileData *fd2;
	gboolean result;

	if ((fd1 == NULL) && (path2 == NULL))
		return TRUE;
	if ((fd1 == NULL) || (path2 == NULL))
		return FALSE;

	fd2 = file_data_new (path2);
	result = !strcmp (fd1->utf8_path,fd2->utf8_path);

	file_data_unref (fd2);
	return result;
}


gboolean
file_data_same (FileData *fd1,
                FileData *fd2)
{
        if ((fd1 == NULL) && (fd2 == NULL))
                return TRUE;
        if ((fd1 == NULL) || (fd2 == NULL))
                return FALSE;

        return !strcmp (fd1->utf8_path,fd2->utf8_path);
}


gboolean
file_data_has_local_path (FileData  *fd,
			  GtkWindow *window)
{
	if (fd == NULL)
		return FALSE;

	/* TODO: this is where we could trying mounting unmounted remote URIs */
        if (fd->local_path == NULL) {
		char *message;
		message = "%s has not been mounted, or the gvfs daemon has not provided a local mount point in ~/.gvfs/. gThumb can not access this remote file directly.";
		if (window == NULL)
			g_warning (message, fd->utf8_path);
		else
			_gtk_error_dialog_run (window, message, fd->utf8_path);
                return FALSE;
        } else {
		return TRUE;
	}
}

CommentData *
file_data_get_comment (FileData *fd,
                       gboolean  try_embedded)
{
	g_return_val_if_fail (fd != NULL, NULL);

	if (!fd->comment_data)
                fd->comment_data = comments_load_comment (fd->utf8_path, try_embedded);

	if (!fd->comment_data)
                fd->comment_data = comment_data_new ();

        return fd->comment_data;
}

