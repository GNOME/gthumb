/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2001, 2003  Free Software Foundation, Inc.
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

#include <config.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>

#include <png.h>

#include <glib/gi18n.h>
#include <libgnomeui/gnome-thumbnail.h>
#include <libgnomevfs/gnome-vfs-types.h>
#include <libgnomevfs/gnome-vfs-file-info.h>
#include <libgnomevfs/gnome-vfs-ops.h>
#include <libgnomevfs/gnome-vfs-uri.h>
#include <libgnomevfs/gnome-vfs-utils.h>
#include <gtk/gtk.h>

#include "typedefs.h"
#include "thumb-cache.h"
#include "file-utils.h"
#include "file-data.h"
#include "gtk-utils.h"

#define PROCESS_DELAY 25
#define PROCESS_MAX_FILES 33


char *
cache_get_nautilus_cache_name (const char *path)
{
	char           *parent;
	char           *resolved_parent;
	char           *resolved_path = NULL;
	GnomeVFSResult  result;
	GnomeVFSURI    *uri;
	char           *uri_txt;
	char           *retval;

	parent = remove_level_from_path (path);
	result = resolve_all_symlinks (parent, &resolved_parent);
	g_free (parent);

	if (result == GNOME_VFS_OK)
		resolved_path = g_strconcat (resolved_parent,
					     "/",
					     file_name_from_path (path),
					     NULL);
	else
		resolved_path = g_strdup (path);

	uri = new_uri_from_path (resolved_path);

	g_free (resolved_path);
	g_free (resolved_parent);

	uri_txt = gnome_vfs_uri_to_string (uri, GNOME_VFS_URI_HIDE_NONE);
	gnome_vfs_uri_unref (uri);

	if (uri_txt == NULL)
		return NULL;

	retval = gnome_thumbnail_path_for_uri (uri_txt, GNOME_THUMBNAIL_SIZE_NORMAL);
	g_free (uri_txt);

	return retval;
}


void
cache_copy (const char *src,
	    const char *dest)
{
	char   *cache_src;
	time_t  dest_mtime = get_file_mtime (dest);

	cache_src = cache_get_nautilus_cache_name (src);
	if (path_is_file (cache_src)) {
		char *cache_dest = cache_get_nautilus_cache_name (dest);

		if (path_is_file (cache_dest))
			file_unlink (cache_dest);
		if (file_copy (cache_src, cache_dest))
			set_file_mtime (cache_dest, dest_mtime);

		g_free (cache_dest);
	}
	g_free (cache_src);
}


void
cache_move (const char *src,
	    const char *dest)
{
	char   *cache_src;
	time_t  dest_mtime = get_file_mtime (dest);

	cache_src = cache_get_nautilus_cache_name (src);

	if (path_is_file (cache_src)) {
		char *cache_dest = cache_get_nautilus_cache_name (dest);

		if (path_is_file (cache_dest))
			file_unlink (cache_dest);
		if (file_move (cache_src, cache_dest))
			set_file_mtime (cache_dest, dest_mtime);

		g_free (cache_dest);
	}
	g_free (cache_src);
}
