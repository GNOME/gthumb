/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2008 The Free Software Foundation, Inc.
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
#include "gio-utils.h"
#include "glib-utils.h"
#include "gnome-desktop-thumbnail.h"
#include "gth-file-data.h"
#include "gth-user-dir.h"
#include "typedefs.h"


#define MAX_CACHE_SIZE (256 * 1024 * 1024)

static goffset   Cache_Used_Space = 0;
static GList    *Cache_Files = NULL; /* GthFileData list */
static gboolean  Cache_Loaded = FALSE;


char *
get_file_cache_full_path (const char *filename,
			  const char *extension)
{
	return gth_user_dir_get_file (GTH_DIR_CACHE,
				      GTHUMB_DIR,
				      filename,
				      extension,
				      NULL);
}


GFile *
_g_file_get_cache_file (GFile *file)
{
	GFile *cache_file;

	if (! g_file_is_native (file)) {
		char *uri;
		char *name;
		char *path;

		uri = g_file_get_uri (file);
		name = gnome_desktop_thumbnail_md5 (uri);
		if (name == NULL)
			return NULL;
		path = get_file_cache_full_path (name, NULL);

		cache_file = g_file_new_for_path (path);

		g_free (path);
		g_free (name);
		g_free (uri);
	}
	else
		cache_file = g_file_dup (file);

	return cache_file;
}


void
free_file_cache (void)
{
	char            *cache_path;
	GFile           *cache_dir;
	GFileEnumerator *fenum;
	GFileInfo       *info;

	cache_path = get_file_cache_full_path (NULL, NULL);
	cache_dir = g_file_new_for_path (cache_path);
	g_free (cache_path);

	fenum = g_file_enumerate_children (cache_dir,
					   "standard::name",
					   G_FILE_QUERY_INFO_NONE,
					   NULL,
					   NULL);
	if (fenum == NULL) {
		g_object_unref (cache_dir);
		return;
	}

	while ((info = g_file_enumerator_next_file (fenum, NULL, NULL)) != NULL) {
		GFile *file;

		file = g_file_get_child (cache_dir, g_file_info_get_name (info));
		g_file_delete (file, NULL, NULL);
		g_object_unref (file);
	}

	g_object_unref (fenum);
	g_object_unref (cache_dir);

	Cache_Files = NULL;
	Cache_Used_Space = 0;
}


static gint
comp_func_time (gconstpointer a,
		gconstpointer b)
{
	GFileInfo *info_a, *info_b;
	GTimeVal   time_a, time_b;

	info_a = (GFileInfo *) a;
	info_b = (GFileInfo *) b;

	g_file_info_get_modification_time (info_a, &time_a);
	g_file_info_get_modification_time (info_b, &time_b);

	return _g_time_val_cmp (&time_a, &time_b);
}


void
check_cache_free_space (void)
{
	char  *cache_path;
	GFile *cache_dir;

	cache_path = get_file_cache_full_path (NULL, NULL);
	cache_dir = g_file_new_for_path (cache_path);
	g_free (cache_path);

	if (! Cache_Loaded) {
		GFileEnumerator *fenum;
		GList           *info_list, *scan;
		GFileInfo       *info;

		fenum = g_file_enumerate_children (cache_dir,
						   "standard::name,standard::size",
						   G_FILE_QUERY_INFO_NONE,
						   NULL,
						   NULL);
		if (fenum == NULL) {
			g_object_unref (cache_dir);
			return;
		}

		info_list = NULL;
		while ((info = g_file_enumerator_next_file (fenum, NULL, NULL)) != NULL)
			info_list = g_list_prepend (info_list, g_object_ref (info));
		info_list = g_list_sort (info_list, comp_func_time);

		g_object_unref (fenum);

		Cache_Used_Space = 0;
		for (scan = info_list; scan; scan = scan->next) {
			GFileInfo *info = scan->data;
			GFile     *file;

			file = g_file_get_child (cache_dir, g_file_info_get_name (info));

			Cache_Files = g_list_prepend (Cache_Files, gth_file_data_new (file, info));
			Cache_Used_Space += g_file_info_get_size (info);

			g_object_unref (file);
			g_object_unref (info);
		}
		g_list_free (info_list);

		Cache_Loaded = TRUE;
	}

	debug (DEBUG_INFO, "cache size: %ul.\n", Cache_Used_Space);

	if (Cache_Used_Space > MAX_CACHE_SIZE) {
		GList *scan;
		int    n = 0;

		/* the first file is the last copied, so reverse the list to
		 * delete the older files first. */

		Cache_Files = g_list_reverse (Cache_Files);
		for (scan = Cache_Files; scan && (Cache_Used_Space > MAX_CACHE_SIZE / 2); ) {
			GthFileData *file_data = scan->data;

			g_file_delete (file_data->file, NULL, NULL);
			Cache_Used_Space -= g_file_info_get_size (file_data->info);

			Cache_Files = g_list_remove_link (Cache_Files, scan);
			_g_object_list_unref (scan);
			scan = Cache_Files;

			n++;
		}
		Cache_Files = g_list_reverse (Cache_Files);

		debug (DEBUG_INFO, "deleted %d files, new cache size: %ul.\n", n, Cache_Used_Space);
	}

	g_object_unref (cache_dir);
}


typedef struct {
	CopyDoneCallback  done_func;
	gpointer          done_data;
	GFile            *cache_file;
	gboolean          dummy;
} CopyToCacheData;


static void
copy_remote_file_to_cache_ready (GError   *error,
				 gpointer  callback_data)
{
	CopyToCacheData *copy_data = callback_data;

	if (! copy_data->dummy && (error == NULL)) {
		Cache_Used_Space += _g_file_get_size (copy_data->cache_file);
		Cache_Files = g_list_prepend (Cache_Files, g_object_ref (copy_data->cache_file));
	}

	if (copy_data->done_func != NULL)
		copy_data->done_func (error, copy_data->done_data);

	g_object_unref (copy_data->cache_file);
	g_free (copy_data);
}


void
copy_remote_file_to_cache (GthFileData      *file_data,
			   GCancellable     *cancellable,
			   CopyDoneCallback  done_func,
			   gpointer          done_data)
{
	CopyToCacheData *copy_data;
	GFile           *cache_file;
	GTimeVal         cache_mtime;

	cache_file = _g_file_get_cache_file (file_data->file);
	_g_file_get_modification_time (cache_file, &cache_mtime);

	copy_data = g_new0 (CopyToCacheData, 1);
	copy_data->done_func = done_func;
	copy_data->done_data = done_data;
	copy_data->cache_file = cache_file;

	if (! g_file_is_native (file_data->file) &&
	    (_g_time_val_cmp (&cache_mtime, gth_file_data_get_modification_time (file_data)) < 0))
	{
		copy_data->dummy = FALSE;
		_g_copy_file_async (file_data->file,
				    cache_file,
				    FALSE,
				    G_FILE_COPY_OVERWRITE,
				    G_PRIORITY_DEFAULT,
				    cancellable,
				    NULL,
				    NULL,
				    copy_remote_file_to_cache_ready,
				    copy_data);
	}
	else {
		copy_data->dummy = TRUE;
		_g_dummy_file_op_async (copy_remote_file_to_cache_ready, copy_data);
	}
}


GFile *
obtain_local_file (GthFileData *file_data)
{
	GFile    *cache_file;
	GTimeVal  cache_mtime;

	cache_file = _g_file_get_cache_file (file_data->file);
	_g_file_get_modification_time (cache_file, &cache_mtime);

	if (! g_file_is_native (file_data->file) &&
	    (_g_time_val_cmp (&cache_mtime, gth_file_data_get_modification_time (file_data)) < 0))
	{
		if (! g_file_copy (file_data->file,
				   cache_file,
				   G_FILE_COPY_OVERWRITE,
				   NULL,
				   NULL,
				   NULL,
				   NULL))
		{
			g_object_unref (cache_file);
			cache_file = NULL;
		}
	}

	return cache_file;
}


void
update_file_from_cache (GthFileData      *file_data,
			GCancellable     *cancellable,
			CopyDoneCallback  done_func,
			gpointer          done_data)
{
	GFile    *cache_file;
	GTimeVal  cache_mtime;

	cache_file = _g_file_get_cache_file (file_data->file);
	_g_file_get_modification_time (cache_file, &cache_mtime);

	if (! g_file_is_native (file_data->file)
	    && (_g_time_val_cmp (&cache_mtime, gth_file_data_get_modification_time (file_data)) > 0))
	{
		_g_copy_file_async (cache_file,
				    file_data->file,
				    FALSE,
				    G_FILE_COPY_OVERWRITE,
				    G_PRIORITY_DEFAULT,
				    cancellable,
				    NULL,
				    NULL,
				    done_func,
				    done_data);
	}
	else
		_g_dummy_file_op_async (done_func, done_data);

	g_object_unref (cache_file);
}
