/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2008 Free Software Foundation, Inc.
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


#include <stdlib.h>
#include <string.h>
#include <glib.h>

#include "gfile-utils.h"


/*
 * NOTE: All these functions accept/return _only_ uri-style GFiles.
 */



/* GFile to string */

char *
gfile_get_uri (GFile *file)
{
	return g_file_get_uri (file);
}


char *
gfile_get_path (GFile *file)
{
	char  *escaped, *unescaped;

	escaped = g_file_get_path (file);
	unescaped = g_uri_unescape_string (escaped, NULL);
	
	g_free (escaped);
	
	return unescaped;
}


/* Debug */

void 
gfile_debug (const char *cfile,
	     int         line,
	     const char *function,
	     const char *msg,
	     GFile      *file)
{
	char *uri;
	char *dbg;
	
	if (file == NULL)
		uri = g_strdup ("(null)");
	else
		uri = gfile_get_uri (file);
	
	dbg = g_strdup_printf ("%s: %s\n", msg, uri);
	
	debug (cfile, line, function, dbg);
	
        g_free (uri);
        g_free (dbg);
}


void 
gfile_warning (const char *msg,
	       GFile      *file,
	       GError     *err)
{
	char *uri;
	char *warning;
	
	uri = gfile_get_uri (file);
	
	if (err == NULL)
		warning = g_strdup_printf ("%s: file %s\n", msg, uri);
	else
		warning = g_strdup_printf ("%s: file %s: %s\n", msg, uri, err->message);
	
	g_warning (warning);
	
        g_free (uri);
        g_free (warning);
}


/* Constructor enforcing the "uri only" GFile policy */

GFile *
gfile_new (const char *path)
{
	GFile *file;
	char  *uri;
	
	g_assert (path != NULL);
	
	if (strstr (path, "://") == NULL)
		uri = g_strconcat ("file://", path, NULL);
	else
		uri = g_strdup (path);

	file = g_file_new_for_uri (uri);
	
	g_free (uri);
	
	return file;
}


GFile *
gfile_new_va (const char *path,
              ...)
{
	va_list  args;
	GFile   *file;
	char    *pathx;
	
	g_assert (path != NULL);
	
	file = gfile_new (path);
	
	va_start (args, path);
	
	while ((pathx = va_arg (args, char*)) != NULL) {
		GFile *tmp;
		
		tmp = g_file_dup (file);
		g_object_unref (file);
		
		file = g_file_resolve_relative_path (tmp, pathx);
		g_object_unref (tmp);
	}
	
	va_end (args);

	return file;
}


/* File utils */

GFile *
gfile_append_path (GFile      *dir,
		   const char *path,
                   ...)
{
	va_list  args;
	GFile   *file;
	char    *pathx;
	
	if (path == NULL)
		return g_file_dup (dir);
	
	file = g_file_resolve_relative_path (dir, path);
	
	va_start (args, path);
	
	while ((pathx = va_arg (args, char*)) != NULL) {
		GFile *tmp;
		
		tmp = g_file_dup (file);
		g_object_unref (file);

		file = g_file_resolve_relative_path (tmp, pathx);
		g_object_unref (tmp);
	}
	
	va_end (args);

	return file;
}


gboolean
gfile_is_local (GFile *file)
{
	return g_file_has_uri_scheme (file, "file");
}


static gboolean
gfile_is_filetype (GFile      *file,
		   GFileType   file_type)
{
	gboolean   result = FALSE;
	GFileInfo *info;
	GError    *error = NULL;

	if (! g_file_query_exists (file, NULL))
		return FALSE;

	info = g_file_query_info (file, 
			          G_FILE_ATTRIBUTE_STANDARD_TYPE, 
			          0, 
			          NULL, 
			          &error);
	if (error == NULL) {
		result = (g_file_info_get_file_type (info) == file_type);
	}
	else {
		gfile_warning ("Failed to get file type", file, error);
		g_error_free (error);
	}

	g_object_unref (info);

	return result;
}


gboolean
gfile_path_is_file (GFile *file)
{
	return gfile_is_filetype (file, G_FILE_TYPE_REGULAR);
}


gboolean
gfile_path_is_dir (GFile *file)
{
	return gfile_is_filetype (file, G_FILE_TYPE_DIRECTORY);
}


goffset
gfile_get_file_size (GFile *file)
{
	GFileInfo *info;
	goffset    size = 0;
	GError    *err = NULL;

	//FIXME: shouldn't we get rid of this test and fix the callers instead
	if (file == NULL)
		return 0;
	
        info = g_file_query_info (file, G_FILE_ATTRIBUTE_STANDARD_SIZE, 0, NULL, &err);
        if (err == NULL) {
                size = g_file_info_get_size (info);
        }
        else {
                gfile_warning ("Failed to get file size", file, err);
                g_error_free (err);
        }

        g_object_unref (info);

        return size;
}


/* Directory utils */

static gboolean
make_directory_tree (GFile    *dir,
		     mode_t    mode,
		     GError  **error)
{
	gboolean  success = TRUE;
	GFile    *parent;

	parent = g_file_get_parent (dir);
	if (parent != NULL) {
		success = make_directory_tree (parent, mode, error);
		g_object_unref (parent);
		if (! success)
			return FALSE;
	}

	success = g_file_make_directory (dir, NULL, error);
	if ((error != NULL) && (*error != NULL) && g_error_matches (*error, G_IO_ERROR, G_IO_ERROR_EXISTS)) {
		g_clear_error (error);
		success = TRUE;
	}

	if (success)
		g_file_set_attribute_uint32 (dir,
					     G_FILE_ATTRIBUTE_UNIX_MODE,
					     mode,
					     0,
					     NULL,
					     NULL);

	return success;
}


gboolean
gfile_ensure_dir_exists (GFile    *dir,
			 mode_t    mode,
			 GError  **error)
{
	GError *priv_error = NULL;

	//FIXME: shouldn't we get rid of this test and fix the callers instead
	if (dir == NULL)
		return FALSE;

	if (error == NULL)
		error = &priv_error;

	if (! make_directory_tree (dir, mode, error)) {
		
		gfile_warning ("could not create directory", dir, *error);
		if (priv_error != NULL)
			g_clear_error (&priv_error);
		
		return FALSE;
	}

	return TRUE;
}


guint64
gfile_get_destination_free_space (GFile *file)
{
	guint64    freespace = 0;
	GFileInfo *info;
	GError    *err = NULL;

	info = g_file_query_filesystem_info (file, 
			                     G_FILE_ATTRIBUTE_FILESYSTEM_FREE, 
			                     NULL, 
			                     &err);
	if (info != NULL) {
		freespace = g_file_info_get_attribute_uint64 (info, 
				                              G_FILE_ATTRIBUTE_FILESYSTEM_FREE);
		g_object_unref (info);
	}
	else {
		gfile_warning ("Could not get filesystem free space on volume containing the file", 
			       file, 
			       err);
		g_error_free (err);
	}

	return freespace;
}


GFile *
gfile_get_home_dir (void)
{
	GFile *dir;
	
	dir = gfile_new (g_get_home_dir ());
	
	return dir;
}


GFile *
gfile_get_tmp_dir (void)
{
	GFile *dir;
	
	dir = gfile_new (g_get_tmp_dir ());
	
	return dir;
}


#define MAX_TEMP_FOLDER_TO_TRY  2

static GFile *
ith_temp_folder_to_try (int n)
{
	GFile *dir = NULL;
	
	if (n == 1)
		dir = gfile_get_home_dir ();
	else if (n == 2)
		dir = gfile_get_tmp_dir ();
	
	return dir;
}


GFile *
gfile_make_temp_in_dir (GFile *in_dir)
{
	char  *path0;
	char  *path1;
	char  *template;
	GFile *dir;
	
	path0 = g_file_get_path (in_dir);
	template = g_strconcat (path0, "/.gt-XXXXXX", NULL);
	g_free (path0);

	path1 = mkdtemp (template);
	
	if (path1 == NULL)
		return NULL;
	
	dir = gfile_new (path1);
	
	g_free (path1);
	
	return dir;
}


GFile *
gfile_get_temp_dir_name (void)
{
	int      i;
	guint64  max_size = 0;
	GFile   *best_folder = NULL;
	GFile   *tmp_dir;

	/* find the folder with more free space. */

	for (i = 1; i <= MAX_TEMP_FOLDER_TO_TRY; i++) {
		GFile    *folder;
		guint64   size;

		folder = ith_temp_folder_to_try (i);
		size = gfile_get_destination_free_space (folder);
		if (max_size < size) {
			max_size = size;
			UNREF (best_folder)
			best_folder = folder;
		}
		else
			g_object_unref (folder);
	}

	if (best_folder == NULL)
		return NULL;
	
	if (! gfile_is_local (best_folder)) {
		g_object_unref (best_folder);
		return NULL;
	}

	tmp_dir = gfile_make_temp_in_dir (best_folder);
	
	g_object_unref (best_folder);

	return tmp_dir;
}


static gboolean
delete_directory_recursive (GFile   *dir,
			    GError **error)
{
	GFileEnumerator *file_enum;
	GFileInfo       *info;
	gboolean         error_occurred = FALSE;

	if (error != NULL)
		*error = NULL;

	file_enum = g_file_enumerate_children (dir,
					       G_FILE_ATTRIBUTE_STANDARD_NAME ","
					       G_FILE_ATTRIBUTE_STANDARD_TYPE,
					       0, NULL, error);

	while (! error_occurred && (info = g_file_enumerator_next_file (file_enum, NULL, error)) != NULL) {
		GFile *child;

		child = g_file_get_child (dir, g_file_info_get_name (info));

		switch (g_file_info_get_file_type (info)) {
		case G_FILE_TYPE_DIRECTORY:
			if (! delete_directory_recursive (child, error))
				error_occurred = TRUE;
			break;
		default:
			if (! g_file_delete (child, NULL, error))
				error_occurred = TRUE;
			break;
		}

		g_object_unref (child);
		g_object_unref (info);
	}

	if (! error_occurred && ! g_file_delete (dir, NULL, error))
 		error_occurred = TRUE;

	g_object_unref (file_enum);

	return ! error_occurred;
}


gboolean
gfile_dir_remove_recursive (GFile *dir)
{
	gboolean   result;
	GError    *error = NULL;

	result = delete_directory_recursive (dir, &error);
	if (! result) {
		gfile_warning ("Cannot delete directory", dir, error);
		g_clear_error (&error);
	}

	return result;
}


/* Xfer */


/* empty functions */
static void _empty_file_progress_cb  (goffset current_num_bytes,
				      goffset total_num_bytes,
				      gpointer user_data)
{
}

gboolean
gfile_xfer (GFile    *sfile,
	    GFile    *dfile,
	    gboolean  move)
{
	GError *error = NULL;

	if (g_file_equal (sfile, dfile)) {
		gfile_warning ("cannot copy file: source and destination are the same", 
			       sfile, 
			       NULL);
		return FALSE;
	}

	if (move)
		g_file_move (sfile, dfile,
			     G_FILE_COPY_OVERWRITE,
			     NULL, _empty_file_progress_cb,
			     NULL, &error);
	else
		g_file_copy (sfile, dfile,
			     G_FILE_COPY_OVERWRITE,
			     NULL, _empty_file_progress_cb,
			     NULL, &error);
	
	if (error != NULL) {
		gfile_warning ("error during file copy", sfile, error);
		g_error_free (error);
		return FALSE;
	}
	
	return TRUE;
}


gboolean
gfile_copy (GFile *sfile,
	    GFile *dfile)
{
	return gfile_xfer (sfile, dfile, FALSE);
}

gboolean
gfile_move (GFile *sfile,
	    GFile *dfile)
{
	return gfile_xfer (sfile, dfile, TRUE);
}
