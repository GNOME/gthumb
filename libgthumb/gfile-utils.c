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


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <glib.h>

#include "preferences.h"
#include "glib-utils.h"
#include "gconf-utils.h"
#include "gfile-utils.h"
#include "file-data.h"
#include "file-utils.h"
#include "gtk-utils.h"


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
                warning = g_strdup_printf ("%s: file %s", msg, uri);
        else
                warning = g_strdup_printf ("%s: file %s: %s", msg, uri, err->message);

        g_warning ("%s\n", warning);

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


gboolean
gfile_is_hidden (GFile *file)
{
	GFileInfo *info;
	gboolean   result;

        g_assert (file != NULL);

        info = g_file_query_info (file,
                                  G_FILE_ATTRIBUTE_STANDARD_IS_HIDDEN,
                                  G_FILE_QUERY_INFO_NONE,
                                  NULL,
                                  NULL);
        if (info == NULL)
		return FALSE;

	result = g_file_info_get_is_hidden (info);

	g_object_unref (info);

        return result;
}


/* Be careful: result must be g_free'd  */
char *
gfile_get_filename_extension (GFile *file)
{
        char *basename;
        char *last_dot;
        char *extension;

        basename = g_file_get_basename (file);

        g_assert (basename != NULL);

        last_dot = strrchr (basename, '.');
        if (last_dot == NULL)
                return NULL;

        extension = g_strdup (last_dot + 1);

        g_free (basename);

        return extension;
}


const char*
gfile_get_mime_type (GFile      *file,
                     gboolean    fast_file_type)
{
        const char *value;
        const char *result = NULL;
        GFileInfo  *info;

        g_assert (file != NULL);

        info = g_file_query_info (file,
                                  fast_file_type ?
                                  G_FILE_ATTRIBUTE_STANDARD_FAST_CONTENT_TYPE :
                                  G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE,
                                  G_FILE_QUERY_INFO_NONE,
                                  NULL,
                                  NULL);
        if (info != NULL) {
                if (fast_file_type)
                        value = g_file_info_get_attribute_string (info,
                                        G_FILE_ATTRIBUTE_STANDARD_FAST_CONTENT_TYPE);
                else
                        value = g_file_info_get_content_type (info);

		if (!value) {
			char *utf8_path = g_file_get_parse_name (file);
			debug (DEBUG_INFO, "%s returned a NULL mime type", utf8_path);
			return NULL;
		}
		
                /*
                 * If the file content is determined to be binary data (octet-stream), check for
                 * HDR file types, which are not well represented in the freedesktop mime database
                 * currently. This section can be purged when they are. This is an unpleasant hack.
                 * Some file extensions may be missing here; please file a bug if they are.
                 */
                if (strcmp (value, "application/octet-stream") == 0) {

                        char* extension;

                        extension = gfile_get_filename_extension (file);

                        if (extension != NULL) {
                                if (   !strcasecmp (extension, "exr")	/* OpenEXR format */
                                                || !strcasecmp (extension, "hdr")	/* Radiance rgbe */
                                                || !strcasecmp (extension, "pic"))	/* Radiance rgbe */
                                        value = "image/x-hdr";

                                g_free (extension);
                        }
                }

                result = get_static_string (value);
        	g_object_unref (info);
	}

        return result;
}


static gboolean
_gfile_image_is_type (GFile      *file,
                      const char *mime_type,
                      gboolean    fast_file_type)
{
        const char *result;

        result = gfile_get_mime_type (file, fast_file_type);

        if (result == NULL)
                return FALSE;

        return (strcmp (result, mime_type) == 0);
}


static gboolean
_gfile_image_is_type__gconf_file_type (GFile      *file,
                                       const char *mime_type)
{
        return _gfile_image_is_type (file,
                                     mime_type,
                                     ! gfile_is_local (file)
                                     || eel_gconf_get_boolean (PREF_FAST_FILE_TYPE, TRUE));
}


gboolean
gfile_image_is_jpeg (GFile *file)
{
        return _gfile_image_is_type__gconf_file_type (file, "image/jpeg");
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
                                  G_FILE_QUERY_INFO_NONE,
                                  NULL,
                                  &error);
        if (error == NULL) {
                result = (g_file_info_get_file_type (info) == file_type);
        } else {
                gfile_warning ("Failed to get file type", file, error);
                g_error_free (error);
        }

        g_object_unref (info);

        return result;
}


gboolean
gfile_is_file (GFile *file)
{
        return gfile_is_filetype (file, G_FILE_TYPE_REGULAR);
}


gboolean
gfile_is_dir (GFile *file)
{
        return gfile_is_filetype (file, G_FILE_TYPE_DIRECTORY);
}


gboolean gfile_path_contains (GFile      *file,
                              const char *find_this)
{
	char     *utf8_path;
	gboolean  result;

	g_assert (file != NULL);
	g_assert (find_this != NULL);

	utf8_path = g_file_get_parse_name (file);
	result = (strstr (utf8_path, find_this) != NULL);
	g_free (utf8_path);

	return result;
}


goffset
gfile_get_file_size (GFile *file)
{
        GFileInfo *info;
        goffset    size = 0;
        GError    *err = NULL;

        //FIXME: shouldn't we get rid of this test and fix the callers instead?
        if (file == NULL)
                return 0;

        info = g_file_query_info (file,
                                  G_FILE_ATTRIBUTE_STANDARD_SIZE,
                                  G_FILE_QUERY_INFO_NONE,
                                  NULL,
                                  &err);
        if (err == NULL) {
                size = g_file_info_get_size (info);
        } else {
                gfile_warning ("Failed to get file size", file, err);
                g_error_free (err);
        }

        g_object_unref (info);

        return size;
}


char *
gfile_get_display_name (GFile *file)
{
        GFileInfo *info;
        char      *name = NULL;
        GError    *err = NULL;

	g_assert (file != NULL);

        info = g_file_query_info (file,
                                  G_FILE_ATTRIBUTE_STANDARD_DISPLAY_NAME,
                                  G_FILE_QUERY_INFO_NONE,
                                  NULL,
                                  &err);
        if (err == NULL) {
                name = g_strdup (g_file_info_get_display_name (info));
        } else {
                gfile_warning ("Failed to get file display name", file, err);
                g_error_free (err);
        }

        g_object_unref (info);

        return name;
}



/* Directory utils */

gboolean
gfile_ensure_dir_exists (GFile    *dir,
                         GError  **error)
{
        GError    *priv_error = NULL;
	GFileInfo *info;
	gboolean   result;
	gboolean   success;

        if (dir == NULL)
                return FALSE;

        if (error == NULL)
                error = &priv_error;

	/* Try making dir and any needed parent dirs */
	success = g_file_make_directory_with_parents (dir, NULL, error);
	if ((error != NULL) && (*error != NULL) && g_error_matches (*error, G_IO_ERROR, G_IO_ERROR_EXISTS)) {
		g_clear_error (error);
		success = TRUE;
	}

        if (!success) {
                gfile_warning ("could not create directory or its parents", dir, *error);
                if (priv_error != NULL)
                        g_clear_error (&priv_error);
                return FALSE;
        }

	/* Can we read, write, and execute the new dir? */
        info = g_file_query_info (dir,
                                  G_FILE_ATTRIBUTE_ACCESS_CAN_READ ","
                                  G_FILE_ATTRIBUTE_ACCESS_CAN_WRITE ","
                                  G_FILE_ATTRIBUTE_ACCESS_CAN_EXECUTE,
                                  G_FILE_QUERY_INFO_NONE,
                                  NULL,
                                  error);

        if (info == NULL) {
                gfile_warning ("Failed to get directory permission information", dir, *error);
		if (priv_error != NULL)
                        g_clear_error (&priv_error);
                result = FALSE;
        } else {
                result = g_file_info_get_attribute_boolean (info, G_FILE_ATTRIBUTE_ACCESS_CAN_READ) &&
                         g_file_info_get_attribute_boolean (info, G_FILE_ATTRIBUTE_ACCESS_CAN_WRITE) &&
                         g_file_info_get_attribute_boolean (info, G_FILE_ATTRIBUTE_ACCESS_CAN_EXECUTE);
        }

        g_object_unref (info);
        return result;
}


guint64
gfile_get_destination_free_space (GFile *file)
{
        guint64    freespace = G_MAXUINT64; /* bogus value for unsupported systems */
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
        } else {
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
_gfile_ith_temp_folder_to_try (int n)
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

                folder = _gfile_ith_temp_folder_to_try (i);
                size = gfile_get_destination_free_space (folder);
                if (max_size < size) {
                        max_size = size;
                        UNREF (best_folder)
                        best_folder = folder;
                } else
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
_gfile_delete_directory_recursive (GFile   *dir,
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
                        if (! _gfile_delete_directory_recursive (child, error))
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

        result = _gfile_delete_directory_recursive (dir, &error);
        if (! result) {
                gfile_warning ("Cannot delete directory", dir, error);
                g_clear_error (&error);
        }

        return result;
}


gboolean
gfile_path_list_new (GFile  *gfile,
                     GList **files,
                     GList **dirs)
{
        GFileEnumerator *file_enum;
        GFileInfo       *info;
        GList           *f_list = NULL;
        GList           *d_list = NULL;
	GError		*error = NULL;

        file_enum = g_file_enumerate_children (gfile,
                                               G_FILE_ATTRIBUTE_STANDARD_NAME ","
                                               G_FILE_ATTRIBUTE_STANDARD_TYPE,
                                               0, NULL, &error);

        if (error != NULL) {
                gfile_warning ("Error while reading contents of directory", gfile, error);
                g_error_free (error);
                return FALSE;
        }

        while ((info = g_file_enumerator_next_file (file_enum, NULL, NULL)) != NULL) {
                GFile *child;
                char  *uri;

                child = g_file_get_child (gfile, g_file_info_get_name (info));
                uri = gfile_get_uri (child);

                switch (g_file_info_get_file_type (info)) {
                case G_FILE_TYPE_DIRECTORY:
                        if (dirs) {
                                d_list = g_list_prepend (d_list, g_strdup (uri));
                        }
                        break;
                case G_FILE_TYPE_REGULAR:
                        if (files) {
                                f_list = g_list_prepend (f_list, file_data_new_from_path (uri));
                        }
                        break;
                default:
                        break;
                }

                g_object_unref (child);
                g_object_unref (info);
                g_free (uri);
        }

        if (dirs)
                *dirs = g_list_reverse (d_list);
        else
                path_list_free (d_list);

        if (files) {
                *files = g_list_reverse (f_list);
        }
        else
                file_data_list_free (f_list);

        g_object_unref (file_enum);

        return TRUE;
}


/* Xfer */

static void _empty_file_progress_cb  (goffset current_num_bytes,
                                      goffset total_num_bytes,
                                      gpointer user_data)
{
}


gboolean
gfile_xfer (GFile    *sfile,
            GFile    *dfile,
            gboolean  move,
            gboolean  overwrite,
	    GError  **error)
{
	int     attr;
	GError *priv_error = NULL;

	if (overwrite)
		attr = G_FILE_COPY_OVERWRITE;
	else
		attr = G_FILE_COPY_NONE;

        if (error == NULL)
                error = &priv_error;

        if (move)
                g_file_move (sfile, dfile,
#if GLIB_CHECK_VERSION(2,19,0)
			     G_FILE_COPY_TARGET_DEFAULT_PERMS |
#endif 
                             attr,
                             NULL, _empty_file_progress_cb,
                             NULL, error);
        else
                g_file_copy (sfile, dfile,
#if GLIB_CHECK_VERSION(2,19,0)
                             G_FILE_COPY_TARGET_DEFAULT_PERMS |
#endif 
                             attr,
                             NULL, _empty_file_progress_cb,
                             NULL, error);

        if (priv_error != NULL) {
                gfile_warning ("File move or copy failed", sfile, *error);
                g_clear_error (&priv_error);
                return FALSE;
        }

        return TRUE;
}


gboolean
gfile_copy (GFile   *sfile,
            GFile   *dfile,
            gboolean overwrite,
	    GError **error)
{
        return gfile_xfer (sfile, dfile, FALSE, overwrite, error);
}

gboolean
gfile_move (GFile   *sfile,
            GFile   *dfile,
            gboolean overwrite,
	    GError **error)
{
        return gfile_xfer (sfile, dfile, TRUE, overwrite, error);
}


gssize
gfile_output_stream_write (GFileOutputStream  *ostream,
                           GError            **error,
                           const char         *str)
{
        GError   *priv_error = NULL;
        gssize    result;


        g_assert (str != NULL);

        if (error == NULL)
                error = &priv_error;

        result = g_output_stream_write (G_OUTPUT_STREAM(ostream),
                                        str,
                                        strlen (str),
                                        NULL,
                                        error);

        if (*error != NULL)
                debug (DEBUG_INFO, "could not write string \"%s\": %s\n", str, (*error)->message);

        if (priv_error != NULL)
                g_clear_error (&priv_error);

        return result;
}


gssize
gfile_output_stream_write_line (GFileOutputStream  *ostream,
                                GError            **error,
                                const char         *format,
                                ...)
{
        va_list         args;
        char           *str;
        gssize          n1 = -1;
        gssize          n2 = -1;

        g_assert (format != NULL);

        va_start (args, format);
        str = g_strdup_vprintf (format, args);
        va_end (args);

        n1 = gfile_output_stream_write (ostream, error, str);

        if (n1 != -1)
                n2 = gfile_output_stream_write (ostream, error, "\n");

        g_free (str);

        return (n1 == -1 || n2 == -1 ? -1 : n1 + n2);
}


void
gfile_list_free (GList *list)
{
        if (list == NULL)
                return;
        g_list_foreach (list, (GFunc) g_object_unref, NULL);
        g_list_free (list);
}


GList *
gfile_list_find_gfile (GList *list, GFile *gfile)
{
        GList *scan;

        for (scan = list; scan; scan = scan->next)
                if (g_file_equal ((GFile *) scan->data, gfile))
                        return scan;
        return NULL;
}


void
gfile_set_mtime (GFile  *gfile,
                 time_t  mtime)
{
        GFileInfo *info;
        GError    *error = NULL;
        GTimeVal   tv;

        tv.tv_sec = mtime;
        tv.tv_usec = 0;

        info = g_file_info_new ();
        g_file_info_set_modification_time (info, &tv);
        g_file_set_attributes_from_info (gfile, info, G_FILE_QUERY_INFO_NONE, NULL, &error);
        g_object_unref (info);

        if (error != NULL) {
                gfile_warning ("Failed to set file mtime", gfile, error);
                g_error_free (error);
        }
}

