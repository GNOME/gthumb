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
#include <libgnomevfs/gnome-vfs-types.h>
#include <libgnomevfs/gnome-vfs-file-info.h>
#include <libgnomevfs/gnome-vfs-ops.h>
#include <libgnomevfs/gnome-vfs-utils.h>
#include "file-data.h"
#include "file-utils.h"
#include "comments.h"


#define MAX_COMMENT_LEN 60


FileData *
file_data_new (const char       *path, 
	       GnomeVFSFileInfo *info)
{
	FileData *fd;

	fd = g_new (FileData, 1);

	fd->ref = 1;
	fd->path = g_strdup (path);
	fd->name = file_name_from_path (path);
	fd->utf8_name = g_locale_to_utf8 (fd->name, -1, 0, 0, 0);
	fd->size = info->size;
	fd->ctime = info->ctime;
	fd->mtime = info->mtime;
	fd->error = FALSE;
	fd->thumb = FALSE;
	fd->comment = g_strdup ("");

	return fd;
}


void
file_data_update (FileData *fd)
{
	GnomeVFSFileInfo *info;
	GnomeVFSResult    result;
	char             *escaped;

	g_return_if_fail (fd != NULL);

	escaped = gnome_vfs_escape_path_string (fd->path);
	info = gnome_vfs_file_info_new ();
	result = gnome_vfs_get_file_info (escaped,
					  info,
					  (GNOME_VFS_FILE_INFO_DEFAULT
					   | GNOME_VFS_FILE_INFO_FOLLOW_LINKS));
	g_free (escaped);

	if (result != GNOME_VFS_OK) {
		g_warning ("Cannot get info of file : %s\n", fd->path);
		return;
	}

	fd->size = info->size;
	fd->mtime = info->mtime;
	fd->ctime = info->ctime;

	gnome_vfs_file_info_unref (info);
}


void
file_data_set_path (FileData *fd,
		    const gchar *path)
{
	g_return_if_fail (fd != NULL);
	g_return_if_fail (path != NULL);

	g_free (fd->path);
	g_free (fd->utf8_name);

	fd->path = g_strdup (path);
	fd->name = file_name_from_path (path);
	fd->utf8_name = g_locale_to_utf8 (fd->name, -1, 0, 0, 0);

	file_data_update (fd);
}


void
file_data_update_comment (FileData *fd)
{
	CommentData *data;

	g_return_if_fail (fd != NULL);

	if (fd->comment != NULL)
		g_free (fd->comment);

	data = comments_load_comment (fd->path);

	if (data == NULL) {
		fd->comment = g_strdup ("");
		return;
	}

	fd->comment = comments_get_comment_as_string (data, "\n", "\n");
	if (fd->comment == NULL)
		fd->comment = g_strdup ("");

	comment_data_free (data);
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
		g_free (fd->path);
		g_free (fd->utf8_name);
		g_free (fd->comment);
		g_free (fd);
	}
}

