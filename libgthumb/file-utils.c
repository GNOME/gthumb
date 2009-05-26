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

#define GDK_PIXBUF_ENABLE_BACKEND

#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <strings.h>
#include <time.h>
#include <unistd.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <dirent.h>
#include <errno.h>
#include <config.h>

#include <glib.h>
#include <glib/gi18n.h>
#include <gio/gio.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <libgnomeui/gnome-thumbnail.h>
#include <libgnomevfs/gnome-vfs.h>
#include <libgnomevfs/gnome-vfs-handle.h>
#include <libgnomevfs/gnome-vfs-result.h>
#include <gconf/gconf-client.h>

#include "gthumb-init.h"
#include "gthumb-error.h"
#include "glib-utils.h"
#include "gconf-utils.h"
#include "gfile-utils.h"
#include "file-utils.h"
#include "file-data.h"
#include "pixbuf-utils.h"
#include "typedefs.h"
#include "gth-exif-utils.h"
#include "gth-sort-utils.h"

#ifdef HAVE_LIBOPENRAW
#include <libopenraw-gnome/gdkpixbuf.h>
#endif

#define BUF_SIZE 4096
#define CHUNK_SIZE 128
#define MAX_SYMLINKS_FOLLOWED 32
#define ITEMS_PER_NOTIFICATION 1024

/* Async directory list */


static PathListData *
path_list_data_new (void)
{
	PathListData *pli;

	pli = g_new0 (PathListData, 1);

	pli->uri = NULL;
	pli->result = GNOME_VFS_OK;
	pli->files = NULL;
	pli->dirs = NULL;
	pli->done_func = NULL;
	pli->done_data = NULL;
	pli->interrupted = FALSE;
	pli->hidden_files = NULL;

	return pli;
}


void
path_list_data_free (PathListData *pli)
{
	g_return_if_fail (pli != NULL);

	if (pli->uri != NULL)
		gnome_vfs_uri_unref (pli->uri);

	if (pli->files != NULL) {
		g_list_foreach (pli->files, (GFunc) file_data_unref, NULL);
		g_list_free (pli->files);
	}

	if (pli->dirs != NULL) {
		g_list_foreach (pli->dirs, (GFunc) g_free, NULL);
		g_list_free (pli->dirs);
	}

	if (pli->hidden_files != NULL)
		g_hash_table_unref (pli->hidden_files);

	g_free (pli);
}


static void
directory_load_cb (GnomeVFSAsyncHandle *handle,
		   GnomeVFSResult       result,
		   GList               *list,
		   guint                entries_read,
		   gpointer             data)
{
	PathListData *pli;
	GList *node;

	pli = (PathListData *) data;
	pli->result = result;

	if (pli->interrupted) {
		gnome_vfs_async_cancel (handle);
		pli->interrupted = FALSE;
		path_list_data_free (pli);
		return;
	}

	for (node = list; node != NULL; node = node->next) {
		GnomeVFSFileInfo *info     = node->data;
		GnomeVFSURI      *full_uri = NULL;
		char             *txt_uri;
		FileData         *file;
		
		switch (info->type) {
		case GNOME_VFS_FILE_TYPE_REGULAR:
			if (g_hash_table_lookup (pli->hidden_files, info->name) != NULL)
				break;
			full_uri = gnome_vfs_uri_append_file_name (pli->uri, info->name);
			txt_uri = gnome_vfs_uri_to_string (full_uri, GNOME_VFS_URI_HIDE_NONE);
			
			file = file_data_new (txt_uri);
			file_data_update_mime_type (file, pli->fast_file_type);
			if ((pli->filter_func != NULL) && pli->filter_func (pli, file, pli->filter_data))
				pli->files = g_list_prepend (pli->files, file);
			else
				file_data_unref  (file);
			g_free (txt_uri);
			break;

		case GNOME_VFS_FILE_TYPE_DIRECTORY:
			if (SPECIAL_DIR (info->name))
				break;
			if (g_hash_table_lookup (pli->hidden_files, info->name) != NULL)
				break;
			full_uri = gnome_vfs_uri_append_path (pli->uri, info->name);
			pli->dirs = g_list_prepend (pli->dirs, gnome_vfs_uri_to_string (full_uri, GNOME_VFS_URI_HIDE_NONE));
			break;

		default:
			break;
		}

		if (full_uri)
			gnome_vfs_uri_unref (full_uri);
	}

	if ((result == GNOME_VFS_ERROR_EOF) || (result != GNOME_VFS_OK)) {
		if (pli->done_func) {
			/* pli is deallocated in pli->done_func */
			pli->done_func (pli, pli->done_data);
			return;
		}
		path_list_data_free (pli);
	}
}


PathListHandle *
path_list_async_new (const char         *uri,
		     PathListFilterFunc  filter_func,
		     gpointer            filter_data,
		     gboolean            fast_file_type,
		     PathListDoneFunc    done_func,
		     gpointer            done_data)
{
	GnomeVFSAsyncHandle *handle;
	PathListData        *pli;
	PathListHandle      *pl_handle;

	if (uri == NULL) {
		if (done_func != NULL)
			(done_func) (NULL, done_data);
		return NULL;
	}

	pli = path_list_data_new ();

	pli->uri = new_uri_from_path (uri);
	if (pli->uri == NULL) {
		path_list_data_free (pli);
		if (done_func != NULL)
			(done_func) (NULL, done_data);
		return NULL;
	}

	pli->hidden_files = read_dot_hidden_file (uri);
	pli->filter_func = filter_func;
	pli->filter_data = filter_data;
	pli->done_func = done_func;
	pli->done_data = done_data;
	pli->fast_file_type = fast_file_type;
	
	gnome_vfs_async_load_directory_uri (&handle,
					    pli->uri,
					    GNOME_VFS_FILE_INFO_FOLLOW_LINKS,
					    ITEMS_PER_NOTIFICATION,
					    GNOME_VFS_PRIORITY_DEFAULT,
					    directory_load_cb,
					    pli);

	pl_handle = g_new (PathListHandle, 1);
	pl_handle->vfs_handle = handle;
	pl_handle->pli_data = pli;

	return pl_handle;
}


void
path_list_async_interrupt (PathListHandle *handle)
{
	handle->pli_data->interrupted = TRUE;
	g_free (handle);
}


gboolean
path_list_new (const char  *uri,
	       GList      **files,
	       GList      **dirs)
{
        GFile    *gfile;
	gboolean  result;

        gfile = gfile_new (uri);
	result = gfile_path_list_new (gfile, files, dirs);
	g_object_unref (gfile);

        return result;
}


GList *
path_list_dup (GList *path_list)
{
	GList *new_list = NULL;
	GList *scan;

	if (path_list == NULL)
		return new_list;

	for (scan = path_list; scan; scan = scan->next)
		new_list = g_list_prepend (new_list, g_strdup (scan->data));

	return g_list_reverse (new_list);
}


void
path_list_free (GList *list)
{
	if (list == NULL)
		return;
	g_list_foreach (list, (GFunc) g_free, NULL);
	g_list_free (list);
}


GList *
path_list_find_path (GList *list, const char *path)
{
	GList *scan;

	for (scan = list; scan; scan = scan->next)
		if (same_uri ((char*) scan->data, path))
			return scan;
	return NULL;
}


/* Directory utils */


gboolean
dir_make (const gchar *path)
{
        GFile    *gfile;
        gboolean  result;
	GError   *error = NULL;

        gfile = gfile_new (path);
        result = g_file_make_directory (gfile, NULL, &error);

	if (error != NULL) {
                gfile_warning ("Could not create directory", gfile, error);
                g_error_free (error);
	}

        g_object_unref (gfile);
        return result;
}


gboolean
dir_remove_recursive (const char *path)
{
	GFile    *gfile;
	gboolean  result;
	
	if (path == NULL)
		return FALSE;
	
	gfile = gfile_new (path);
	result = gfile_dir_remove_recursive (gfile);
	
	g_object_unref (gfile);
	return result;
}


gboolean
ensure_dir_exists (const char *path)
{
	GFile    *gfile;
	gboolean  result;
	
	if (path == NULL)
		return FALSE;

	gfile = gfile_new (path);
	result = gfile_ensure_dir_exists (gfile, NULL);
	
	g_object_unref (gfile);
	return result;
}


GList *
dir_list_filter_and_sort (GList    *dir_list,
			  gboolean  names_only,
			  gboolean  show_dot_files)
{
	GList *filtered;
	GList *scan;

	/* Apply filters on dir list. */
	filtered = NULL;
	scan = dir_list;
	while (scan) {
		const char *name_only = file_name_from_path (scan->data);

		if (! (file_is_hidden (name_only) && ! show_dot_files)
		    && (strcmp (name_only, CACHE_DIR) != 0)) {
			char *s;
			char *path = (char*) scan->data;

			s = g_strdup (names_only ? name_only : path);
			filtered = g_list_prepend (filtered, s);
		}
		scan = scan->next;
	}
	filtered = g_list_sort (filtered, (GCompareFunc) gth_sort_by_full_path);

	return filtered;
}


/* return TRUE to add the file to the file list. */
gboolean
file_filter (FileData *file,
	     gboolean  show_hidden_files,
	     gboolean  show_only_images)
{
	if (file->mime_type == NULL)
		return FALSE;

	if (! show_hidden_files && file_is_hidden (file->name))
		return FALSE;
		
	if (show_only_images && ! mime_type_is_image (file->mime_type))
		return FALSE;
		
	if (! (mime_type_is_image (file->mime_type) 
	       || mime_type_is_video (file->mime_type)
	       || mime_type_is_audio (file->mime_type)))
		return FALSE;
			
	return TRUE;
}


gboolean
visit_rc_directory_sync (const char *rc_dir,
			 const char *rc_ext,
			 const char *dir,
			 gboolean    recursive,
			 VisitFunc   do_something,
			 gpointer    data)
{
	char  *rc_dir_full_path;
	GList *files, *dirs;
	GList *scan;
	int    prefix_len, ext_len;
	char  *prefix;

	prefix = g_strconcat ("file://",
			      g_get_home_dir(),
			      "/",
			      rc_dir,
			      NULL);
	prefix_len = strlen (prefix);
	rc_dir_full_path = g_strconcat (prefix,
					dir,
					NULL);
	g_free (prefix);
	ext_len = strlen (rc_ext);

	if (! path_is_dir (rc_dir_full_path)) {
		g_free (rc_dir_full_path);
		return FALSE;
	}

	path_list_new (rc_dir_full_path, &files, &dirs);
	g_free (rc_dir_full_path);

	for (scan = files; scan; scan = scan->next) {
		FileData *file = scan->data;
		char     *rc_file, *real_file;

		rc_file = file->path;
		real_file = g_strndup (rc_file + prefix_len,
				       strlen (rc_file) - prefix_len - ext_len);
		if (do_something)
			(*do_something) (real_file, rc_file, data);

		g_free (real_file);
	}

	if (! recursive)
		return TRUE;

	for (scan = dirs; scan; scan = scan->next) {
		char *sub_dir = (gchar*) scan->data;

		visit_rc_directory_sync (rc_dir,
					 rc_ext,
					 sub_dir + prefix_len,
					 TRUE,
					 do_something,
					 data);
	}

	file_data_list_free (files);
	path_list_free (dirs);

	return TRUE;
}


/* File utils */


const char *
get_mime_type_from_ext (const char *ext)
{
	char       *filename;
	const char *result;

	filename = g_strconcat ("x.", ext, NULL);
	result = get_file_mime_type (filename, TRUE);
	g_free (filename);

	return result;
}


gboolean mime_type_is_image (const char *mime_type)
{
	/* Valid image mime types:
	   	1. All image* types, 
		2. application/x-crw
			This is a RAW photo file, which for some reason
			uses an "application" prefix instead of "image".
	*/

	g_return_val_if_fail (mime_type != NULL, FALSE);

	return ((strstr (mime_type, "image") != NULL) 
		|| (strcmp (mime_type, "application/x-crw") == 0));
}


gboolean file_is_image (const gchar *name,
			gboolean     fast_file_type)
{
	const char *mime_type = NULL;

	mime_type = get_file_mime_type (name, fast_file_type);
	if (mime_type == NULL)
		return FALSE;

	return mime_type_is_image (mime_type);
}


gboolean mime_type_is_video (const char *mime_type)
{
	g_return_val_if_fail (mime_type != NULL, FALSE);

	return ( ((strstr (mime_type, "video") != NULL) ||
		 (strcmp (mime_type, "application/ogg") == 0)));
}


gboolean mime_type_is_audio (const char *mime_type)
{
	g_return_val_if_fail (mime_type != NULL, FALSE);

	return ((strstr (mime_type, "audio") != NULL));
}


gboolean file_is_image_video_or_audio (const gchar *name,
				 gboolean     fast_file_type)
{
	const char *mime_type = NULL;

	mime_type = get_file_mime_type (name, fast_file_type);
	if (mime_type == NULL)
		return FALSE;

	return mime_type_is_image (mime_type) ||
	       mime_type_is_video (mime_type) ||
	       mime_type_is_audio (mime_type);
}


gboolean
file_is_hidden (const gchar *name)
{
	if (name[0] != '.') return FALSE;
	if (name[1] == '\0') return FALSE;
	if ((name[1] == '.') && (name[2] == '\0')) return FALSE;

	return TRUE;
}


static gboolean
xfer_file (const char *from,
	   const char *to,
	   gboolean    move,
           gboolean    overwrite,
	   GError    **error)
{
	GFile      *sfile;
	GFile      *dfile;
	gboolean    result;

	sfile = gfile_new (from);
	dfile = gfile_new (to);
	
	result = gfile_xfer (sfile, dfile, move, overwrite, error);

	g_object_unref (sfile);
	g_object_unref (dfile);
	
	return result;
}


gboolean
file_copy (const char *from,
	   const char *to,
	   gboolean    overwrite,
	   GError    **error)
{
	return xfer_file (from, to, overwrite, FALSE, error);
}


gboolean
file_move (const char *from,
	   const char *to,
           gboolean    overwrite,
	   GError    **error)
{
	return xfer_file (from, to, overwrite, TRUE, error);
}


void
file_unlink_with_gerror (const char  *path,
		         GError     **gerror)
{
	/* Also works with empty directories */
	GFile *gfile;

	g_assert (path != NULL);

	gfile = gfile_new (path);
	if (g_file_query_exists (gfile, NULL))
		g_file_delete (gfile, NULL, gerror);
	g_object_unref (gfile);
}


gboolean
file_unlink (const char *path)
{
	gboolean  result = TRUE;
	GError   *error = NULL;

	g_assert (path != NULL);

	/* Also works with empty directories */
	file_unlink_with_gerror (path, &error);

	if (error != NULL) {
                g_warning ("File/path delete failed: %s", error->message);
                g_error_free (error);
		result = FALSE;
        }

	return result;
}


void
delete_thumbnail (const char *path)
{
	char     *large_thumbnail;
	char     *normal_thumbnail;
	FileData *fd;

	fd = file_data_new (path);

	/* delete associated thumbnails, if present */
	large_thumbnail = gnome_thumbnail_path_for_uri (fd->uri, GNOME_THUMBNAIL_SIZE_LARGE);
	normal_thumbnail = gnome_thumbnail_path_for_uri (fd->uri, GNOME_THUMBNAIL_SIZE_NORMAL);

	file_unlink (large_thumbnail);
	file_unlink (normal_thumbnail);

	g_free (large_thumbnail);
	g_free (normal_thumbnail);
	file_data_unref (fd);
}


const char*
get_file_mime_type (const char *path,
		    gboolean    fast_file_type)
{
	GFile      *file;
	const char *result;
	
	if (path == NULL)
		return NULL;

	file = gfile_new (path);

	result = gfile_get_file_mime_type (file, fast_file_type);

	g_object_unref (file);

	if (result == NULL) {
		char *value;
                value = g_content_type_guess (path, NULL, 0, NULL);
                result = get_static_string (value);
		g_free (value);
	}

	if (result == NULL)
		g_warning ("Could not get content type for %s.\n",path);

	return result;
}



gboolean
mime_type_is (const char *mime_type,
	      const char *value)
{
	return (strcmp_null_tolerant (mime_type, value) == 0);
}


gboolean
image_is_jpeg (const char *path)
{
	GFile    *file;
	gboolean  result;
	
	file = gfile_new (path);
	
	result = gfile_image_is_jpeg (file);
	
	g_object_unref (file);

	return result;
}


static gboolean
mime_type_is_raw (const char *mime_type)
{
	return 	   mime_type_is (mime_type, "application/x-crw")	/* ? */
		|| mime_type_is (mime_type, "image/x-raw")              /* mimelnk */
		|| mime_type_is (mime_type, "image/x-dcraw")		/* freedesktop.org.xml */
		|| g_content_type_is_a (mime_type, "image/x-dcraw");	/* freedesktop.org.xml - this should
									   catch most RAW formats, which are
									   registered as sub-classes of
									   image/x-dcraw */
}


gboolean
mime_type_is_hdr (const char *mime_type)
{
	/* Note that some HDR file extensions have been hard-coded into
	   the get_file_mime_type function above. */
	return mime_type_is (mime_type, "image/x-hdr");
}


gboolean
path_exists (const char *path)
{
	if (! path || ! *path)
		return FALSE;
	
	return g_file_test (path, G_FILE_TEST_EXISTS);
}


gboolean
path_is_file (const char *path)
{
	GFile    *file;
	gboolean  result;
	
	if (path == NULL)
		return FALSE;
	
	file = gfile_new (path);
	
	result = gfile_path_is_file (file);
	
	g_object_unref (file);

	return result;
}


gboolean
path_is_dir (const char *path)
{
	GFile    *file;
	gboolean  result;
	
	if (path == NULL)
		return FALSE;

	file = gfile_new (path);

	result = gfile_path_is_dir (file);
	
	g_object_unref (file);

	return result;
}


goffset
get_file_size (const char *path)
{
	GFile    *file;
	goffset   result;
	
	if (path == NULL)
		return 0;

	file = gfile_new (path);

	result = gfile_get_file_size (file);
	
	g_object_unref (file);

	return result;
}


time_t
get_file_mtime (const char *path)
{
        GFileInfo *info;
        GFile     *gfile;
        GError    *error = NULL;
        GTimeVal   tv;
	time_t     mtime;

        gfile = gfile_new (path);
        info = g_file_query_info (gfile,
                                  G_FILE_ATTRIBUTE_TIME_MODIFIED,
                                  G_FILE_QUERY_INFO_NONE,
                                  NULL,
                                  &error);

        if (error == NULL) {
                g_file_info_get_modification_time (info, &tv);
                mtime = tv.tv_sec;
		g_object_unref (info);
        } else {
                // gfile_warning ("Failed to get file information", gfile, error);
                g_error_free (error);
                mtime = (time_t) 0;
        }

        g_object_unref (gfile);
	return mtime;
}


time_t
get_file_ctime (const gchar *path)
{
        GFileInfo *info;
        GFile     *gfile;
        GError    *error = NULL;
        time_t     ctime;

        gfile = gfile_new (path);
        info = g_file_query_info (gfile,
                                  G_FILE_ATTRIBUTE_TIME_CHANGED,
                                  G_FILE_QUERY_INFO_NONE,
                                  NULL,
                                  &error);

        if (error == NULL) {
                ctime = g_file_info_get_attribute_uint64 (info, G_FILE_ATTRIBUTE_TIME_CHANGED);
                g_object_unref (info);
        } else {
                // gfile_warning ("Failed to get file information", gfile, error);
                g_error_free (error);
                ctime = (time_t) 0;
        }

        g_object_unref (gfile);
	return ctime;
}


void
set_file_mtime (const gchar *path,
		time_t       mtime)
{
        GFileInfo *info;
        GFile     *gfile;
        GError    *error = NULL;
        GTimeVal   tv;

	tv.tv_sec = mtime;
 	tv.tv_usec = 0;

        gfile = gfile_new (path);
        info = g_file_info_new ();
	g_file_info_set_modification_time (info, &tv);
	g_file_set_attributes_from_info (gfile, info, G_FILE_QUERY_INFO_NONE, NULL, &error);
	g_object_unref (info);

        if (error != NULL) {
                gfile_warning ("Failed to set file mtime", gfile, error);
                g_error_free (error);
        }

        g_object_unref (gfile);
}


/* URI/Path utils */


char *
get_utf8_display_name_from_uri (const char *escaped_uri)
{
	char        *utf8_name = NULL;
	GFile       *gfile;

	/* g_file_get_parse_name can handle escaped and unescaped uris */
	
	if (escaped_uri == NULL)
		return NULL;

	if (strcmp (escaped_uri,"/") == 0) {
		utf8_name = g_strdup ("/");
	} else if (strcmp (escaped_uri,"..") == 0) {
		utf8_name = g_strdup ("..");
	} else if (uri_scheme_is_catalog (escaped_uri)) {
		gfile = gfile_new (escaped_uri+10);
		utf8_name = g_file_get_parse_name (gfile);
                g_object_unref (gfile);
	} else if (uri_has_scheme (escaped_uri) || escaped_uri[0]=='/') {
        	gfile = gfile_new (escaped_uri);
		utf8_name = g_file_get_parse_name (gfile);
        	g_object_unref (gfile);
	} else {
		char *result;
		char *fake_uri;

		/* This is a bit hackish. */
		fake_uri = g_strconcat ("file:///", escaped_uri, NULL);
                gfile = gfile_new (fake_uri);
                result = g_file_get_parse_name (gfile);
                g_object_unref (gfile);

		/* g_file_get_parse_name strips off the "file://" bit,
		   we need to skip the one remaining leading slash */
		utf8_name = g_strdup_printf ("%s",result+1);
		g_free (result);
	}

	return utf8_name;
}


const char *
get_home_uri (void)
{
	static char *home_uri = NULL;
	if (home_uri == NULL)
		home_uri = g_strconcat ("file://", g_get_home_dir (), NULL);
	return home_uri;
}


char *
get_uri_scheme (const char *uri)
{
	const char *idx;

	idx = strstr (uri, "://");
	if (idx == NULL)
		return NULL;
	return g_strndup (uri, (idx - uri) + 3);
}


const char *
remove_host_from_uri (const char *uri)
{
	const char *idx, *sep;

	if (uri == NULL)
		return NULL;

	idx = strstr (uri, "://");
	if (idx == NULL)
		return uri;
	idx += 3;
	if (*idx == '\0')
		return "/";
	sep = strstr (idx, "/");
	if (sep == NULL)
		return idx;
	return sep;
}


static char *
get_uri_host (const char *uri)
{
	const char *idx;

	idx = strstr (uri, "://");
	if (idx == NULL)
		return g_strdup ("file://");
	
	idx = strstr (idx + 3, "/");
	if (idx == NULL) {
		char *scheme;
		
		scheme = get_uri_scheme (uri);
		if (scheme == NULL)
			scheme = g_strdup ("file://");
		return scheme;
	}
	
	return g_strndup (uri, (idx - uri));
}


gboolean
uri_has_scheme (const char *uri)
{
	if (uri == NULL)
		return FALSE;

	return strstr (uri, "://") != NULL;
}


char *
get_base_uri (const char *uri)
{
	const char *idx;

	if (uri == NULL)
		return NULL;

	if (*uri == '/')
		return g_strdup ("/");

	idx = strstr (uri, "://");
	if (idx == NULL)
		return NULL;
	idx = strstr (idx + 3, "/");
	if (idx == NULL)
		return g_strdup (uri);
	else
		return g_strndup (uri, idx - uri);
}


gboolean
uri_scheme_is_file (const char *uri)
{
	if (uri == NULL)
		return FALSE;
	if (g_utf8_strlen (uri, -1) < FILE_PREFIX_L)
		return FALSE;
	return strncmp (uri, FILE_PREFIX, FILE_PREFIX_L) == 0;
}


gboolean
uri_scheme_is_catalog (const char *uri)
{
	if (uri == NULL)
		return FALSE;
	if (g_utf8_strlen (uri, -1) < CATALOG_PREFIX_L)
		return FALSE;
	return strncmp (uri, CATALOG_PREFIX, CATALOG_PREFIX_L) == 0;
}


gboolean
uri_scheme_is_search (const char *uri)
{
	if (uri == NULL)
		return FALSE;
	if (g_utf8_strlen (uri, -1) < SEARCH_PREFIX_L)
		return FALSE;
	return strncmp (uri, SEARCH_PREFIX, SEARCH_PREFIX_L) == 0;
}


char *
add_scheme_if_absent (const char *path)
{
	if (path == NULL)
		return NULL;
	if ((*path == '\0') || (path[0] == '/'))
		return g_strconcat ("file://", path, NULL);
	return g_strdup (path);
}


char *
add_filename_to_uri (const char *uri,
		     const char *filename)
{
	gboolean  add_separator;
	
	if (str_ends_with (uri, ":///") || str_ends_with (uri, "/"))
		add_separator = FALSE;
	else
		add_separator = TRUE;
		
	return g_strconcat (uri,
			    (add_separator ? "/" : ""),
			    filename,
			    NULL);
}


char *
get_uri_from_local_path (const char *local_path)
{
        char *escaped;
        char *uri;

        escaped = escape_uri (local_path);
        if (escaped[0] == '/') {
                uri = g_strconcat ("file://", escaped, NULL);
                g_free (escaped);
        }
        else
                uri = escaped;

        return uri;
}


char *
get_uri_display_name (const char *uri)
{
	char     *name = NULL;
	char     *tmp_path;
	gboolean  catalog_or_search;

	/* if it is a catalog then remove the extension */
	catalog_or_search = uri_scheme_is_catalog (uri) || uri_scheme_is_search (uri);

	if (catalog_or_search || is_local_file (uri))
		tmp_path = g_strdup (remove_host_from_uri (uri));
	else
		tmp_path = g_strdup (uri);

	if (catalog_or_search && file_extension_is (uri, CATALOG_EXT))
		tmp_path[strlen (tmp_path) - strlen (CATALOG_EXT)] = 0;

	if ((tmp_path == NULL) || (strcmp (tmp_path, "") == 0)
	    || (strcmp (tmp_path, "/") == 0)) {
	    	if (catalog_or_search)
	    		name = g_strdup (_("Catalogs"));
	    	else {
			if (uri_scheme_is_file (uri))
				name = g_strdup (_("File System"));
			else
				name = g_strdup (uri);
	    	}
	} 
	else {
		if (catalog_or_search) {
			char *base_uri;
			int   base_uri_len;

			base_uri = get_catalog_full_path (NULL);
			base_uri_len = strlen (remove_host_from_uri (base_uri));
			g_free (base_uri);

			name = get_utf8_display_name_from_uri (tmp_path + 1 + base_uri_len);
		} 
		else {
			const char *base_path;
			int         base_path_len;

			if (uri_has_scheme (uri))
				base_path = get_home_uri ();
			else
				base_path = g_get_home_dir ();
			base_path_len = strlen (base_path);

			if (strncmp (uri, base_path, base_path_len) == 0) {
				int uri_len = strlen (uri);
				if (uri_len == base_path_len)
					name = g_strdup (_("Home"));
				else if (uri_len > base_path_len)
					name = get_utf8_display_name_from_uri (uri + 1 + base_path_len);
			} 
			else
				name = get_utf8_display_name_from_uri (tmp_path);
		}
	}

	g_free (tmp_path);

	return name;
}


/* like g_basename but does not warns about NULL and does not
 * alloc a new string. */
G_CONST_RETURN char *
file_name_from_path (const char *file_name)
{
	register gssize base;
	register gssize last_char;

	if (file_name == NULL)
		return NULL;

	if (file_name[0] == '\0')
		return "";

	last_char = strlen (file_name) - 1;

	if (file_name [last_char] == G_DIR_SEPARATOR)
		return "";

	base = last_char;
	while ((base >= 0) && (file_name [base] != G_DIR_SEPARATOR))
		base--;

	return file_name + base + 1;
}


/* Check whether the path_src is contained in path_dest */
gboolean
path_in_path (const char  *path_src,
	      const char  *path_dest)
{
	int path_src_l, path_dest_l;

	if ((path_src == NULL) || (path_dest == NULL))
		return FALSE;

	path_src_l = strlen (path_src);
	path_dest_l = strlen (path_dest);

	return ((path_dest_l > path_src_l)
		&& (strncmp (path_src, path_dest, path_src_l) == 0)
		&& (path_dest[path_src_l] == '/'));
}


int
uricmp (const char *path1,
	const char *path2)
{
	char *uri1, *uri2;
	int   result;

	if (path1 == NULL) {
		if (path2 == NULL) 
			return 0;
		else
			return -1;
	}
	
	uri1 = add_scheme_if_absent (path1);
	uri2 = add_scheme_if_absent (path2);

	result = strcmp_null_tolerant (uri1, uri2);

	g_free (uri1);
	g_free (uri2);

	return result;
}


gboolean
same_uri (const char *uri1,
	  const char *uri2)
{
	FileData *fd1;
	FileData *fd2;
	gboolean  result = FALSE;

	fd1 = file_data_new (uri1);
	fd2 = file_data_new (uri2);

	if (strcmp (fd1->utf8_path, fd2->utf8_path) == 0)
		result = TRUE;

	file_data_unref (fd1);
	file_data_unref (fd2);

	return result;
}


char *
basename_for_display (const char *uri)
{
        char *result;
	char *utf8_name;

	utf8_name = get_utf8_display_name_from_uri (uri);
	result = g_strdup (file_name_from_path (utf8_name));
	g_free (utf8_name);

	return result;
}


/* example 1 : uri      = file:///xxx/yyy/zzz/foo
 *             desturi  = file:///xxx/www
 *             return   : ../yyy/zzz/foo
 *
 * example 2 : uri      = file:///xxx/yyy/foo
 *             desturi  = file:///xxx
 *             return   : yyy/foo
 *
 * example 3 : uri      = smb:///xxx/yyy/foo
 *             desturi  = file://hhh/xxx
 *             return   : smb:///xxx/yyy/foo
 *
 * example 4 : uri      = file://hhh/xxx
 *             desturi  = file://hhh/xxx
 *             return   : ./
 */
char *
get_path_relative_to_uri (const char *uri,
			  const char *desturi)
{
	char     *sourcedir;
	char    **sourcedir_v;
	char    **desturi_v;
	int       i, j;
	char     *result;
	GString  *relpath;

	if (strcmp (uri, desturi) == 0)
		return g_strdup ("./");
	
	if (strcmp (get_uri_host (uri), get_uri_host (desturi)) != 0)
		return g_strdup (uri);

	sourcedir = remove_level_from_path (remove_host_from_uri (uri));
	sourcedir_v = g_strsplit (sourcedir, "/", 0);
	desturi_v = g_strsplit (remove_host_from_uri (desturi), "/", 0);

	relpath = g_string_new (NULL);

	i = 0;
	while ((sourcedir_v[i] != NULL)
	       && (desturi_v[i] != NULL)
	       && (strcmp (sourcedir_v[i], desturi_v[i]) == 0))
		i++;

	j = i;

	while (desturi_v[i++] != NULL)
		g_string_append (relpath, "../");

	while (sourcedir_v[j] != NULL) {
		g_string_append (relpath, sourcedir_v[j]);
		g_string_append_c (relpath, '/');
		j++;
	}

	g_string_append (relpath, file_name_from_path (uri));

	g_strfreev (sourcedir_v);
	g_strfreev (desturi_v);
	g_free (sourcedir);

	result = relpath->str;
	g_string_free (relpath, FALSE);

	return result;
}


char *
remove_level_from_path (const char *path)
{
	int         p;
	const char *ptr = path;
	char       *base_path;
	char       *parent_path;

	if (path == NULL)
		return NULL;

	p = strlen (path) - 1;
	if (p < 0)
		return NULL;

	base_path = get_base_uri (path);
	if (base_path == NULL)
		return NULL;

	while ((p > 0) && (ptr[p] != '/'))
		p--;
	if ((p == 0) && (ptr[p] == '/'))
		p = 1;

	if (strlen (base_path) > p)
		parent_path = base_path;
	else {
		parent_path = g_strndup (path, (guint)p);
		g_free (base_path);
	}

	return parent_path;
}


gchar *
remove_ending_separator (const gchar *path)
{
	int len, copy_len;

	if (path == NULL)
		return NULL;

	copy_len = len = strlen (path);
	if ((len > 1)
	    && (path[len - 1] == '/')
	    && ! ((len > 3)
		  && (path[len - 2] == '/')
		  && (path[len - 3] == ':')))
		copy_len--;

	return g_strndup (path, copy_len);
}


/*
 * example 1) input : "/xxx/yyy/.."         output : "/xxx"
 * example 2) input : "/xxx/yyy/../www"     output : "/xxx/www"
 * example 3) input : "/xxx/../yyy/../www"  output : "/www"
 * example 4) input : "/xxx/../yyy/../"     output : "/"
 * example 5) input : "/xxx/./"             output : "/xxx"
 * example 6) input : "/xxx/./yyy"          output : "/xxx/yyy"
 *
 * Note : PATH must be absolute.
 */

/* Is this still needed - gfile seems to figure it out on its own */
char *
remove_special_dirs_from_path (const char *uri)
{
	GFile      *file;
	char       *result;

	if (uri == NULL)
		return NULL;

	file = gfile_new (uri);
	result = g_file_get_uri (file);
	g_object_unref (file);

	return result;
}


GnomeVFSURI *
new_uri_from_path (const char *path)
{
	char        *uri_txt;
	GnomeVFSURI *uri;

	if (path[0] == '/')
		uri_txt = g_strconcat ("file://", path, NULL);
	else
		uri_txt = g_strdup (path);
	uri = gnome_vfs_uri_new (uri_txt);
	g_free (uri_txt);

	g_return_val_if_fail (uri != NULL, NULL);

	return uri;
}


static char *
build_uri_2 (const char *s1,
	     const char *s2)
{
	int      s1_len;
	gboolean add_separator;
	gboolean skip_s2_separator;
	
	if ((s1 == NULL) && (s2 == NULL))
		return NULL;

	if ((s1 == NULL) || (strcmp (s1, "") == 0))
		return g_strdup (s2);
	if ((s2 == NULL) || (strcmp (s2, "") == 0))
		return g_strdup (s1);

	s1_len = strlen (s1);

	/* Add a separator between s1 and s2 if s2 doesn't start with
	 * a separator and either s1 doesn't end with a separator or it ends
	 * with '://'. */

	add_separator = (s2[0] != '/') && ((s1[s1_len - 1] != '/') || str_ends_with (s1, "://"));
	
	/* Skip the s2 separator if s2 starts with a separator and s1 ends
	 * with a separator and doesn't end with '://'. */
	
	skip_s2_separator = (s2[0] == '/') && (s1[s1_len - 1] == '/') && ! str_ends_with (s1, "://");
	
	return g_strconcat (s1,
			    (add_separator ? "/" : ""),
			    (skip_s2_separator ? s2 + 1 : s2),
			    NULL);
}


char *
build_uri (const char *s1,
	   const char *s2,
	   ...)
{
	va_list  args;
	char    *r;
	char    *sx;
	
	r = build_uri_2 (s1, s2);
	
	va_start (args, s2);
	
	while ((sx = va_arg (args, char*)) != NULL) {
		char *tmp;
		
		tmp = build_uri_2 (r, sx);
		g_free (r);
		r = tmp;
	}
	
	va_end (args);
	
	return r;
}


char *resolve_all_symlinks (const char *uri)
{
	GFile *gfile_full;
	GFile *gfile_curr;
	GFile *parent;
	char  *child = NULL;
	int    i=0;
	char  *result;

	if ((uri == NULL) || (*uri == '\0'))
		return NULL;

	if (!is_local_file (uri))
		return g_strdup (uri);

	gfile_full = gfile_new (uri);
	gfile_curr = gfile_new (uri);

	if (!g_file_query_exists (gfile_curr, NULL))
		return g_strdup (uri);

	while ((parent = g_file_get_parent (gfile_curr)) != NULL) {
		GFileInfo  *info;
		GError     *error = NULL;
		const char *symlink;

		info = g_file_query_info (gfile_curr,
					  G_FILE_ATTRIBUTE_STANDARD_SYMLINK_TARGET,
					  G_FILE_QUERY_INFO_NONE,
					  NULL, 
					  &error);
		if (error != NULL) {
			g_warning ("Couldn't obtain symlink info: %s",error->message);
			g_object_unref (parent);
			g_object_unref (gfile_curr);
                        g_object_unref (gfile_full);
			g_free (child);
			g_error_free (error);
			return g_strdup (uri);
		}

		if (((symlink = g_file_info_get_symlink_target (info)) != NULL) && (i<MAX_SYMLINKS_FOLLOWED)) {
			i++;

			g_object_unref (gfile_curr);
			gfile_curr = g_file_resolve_relative_path (parent, symlink);

			g_object_unref (gfile_full);
			if (child != NULL)
				gfile_full = g_file_resolve_relative_path (gfile_curr, child);
			else
				gfile_full = g_file_dup (gfile_curr);

		} else {
			g_object_unref (gfile_curr);
			gfile_curr = g_file_dup (parent);

			g_free (child);
			child = g_file_get_relative_path (parent, gfile_full);
		}

		g_object_unref (info);
		g_object_unref (parent);
	}

	result = g_file_get_uri (gfile_full);
	g_object_unref (gfile_curr);
	g_object_unref (gfile_full);
	g_free (child);

	return result;
}


gboolean
uri_is_root (const char *uri)
{
	int len;

	if (uri == NULL)
		return FALSE;

	if (strcmp (uri, "/") == 0)
		return TRUE;

	len = strlen (uri);
	if (strncmp (uri + len - 3, "://", 3) == 0)
		return TRUE;
	if (strncmp (uri + len - 2, ":/", 2) == 0)
		return TRUE;
	if (strncmp (uri + len - 1, ":", 1) == 0)
		return TRUE;

	return FALSE;
}


/* Catalogs */


char *
get_catalog_full_path (const char *relative_path)
{
	char *path;
	char *separator;

	/* Do not allow .. in the relative_path otherwise the user can go
	 * to any directory, while he shouldn't exit from RC_CATALOG_DIR. */
	if ((relative_path != NULL) && (strstr (relative_path, "..") != NULL))
		return NULL;

	if (relative_path == NULL)
		separator = NULL;
	else
		separator = (relative_path[0] == '/') ? "" : "/";

	path = g_strconcat ("file://",
			    g_get_home_dir (),
			    "/",
			    RC_CATALOG_DIR,
			    separator,
			    relative_path,
			    NULL);

	return path;
}


gboolean
file_is_search_result (const char *path)
{
        GFileInputStream *istream;
        GDataInputStream *dstream;
        GError           *error = NULL;
        GFile            *gfile;
        char             *line;
	gsize		  length;

	gfile = gfile_new (path);
	istream = g_file_read (gfile, NULL, &error);
        if (error != NULL) {
                g_error_free (error);
		g_object_unref (istream);
		g_object_unref (gfile);
                return FALSE;
        }

	dstream = g_data_input_stream_new (G_INPUT_STREAM(istream));
	line = g_data_input_stream_read_line (dstream, &length, NULL, NULL);

	g_object_unref (istream);
	g_object_unref (dstream);
        g_object_unref (gfile);

        if (line == NULL)
                return FALSE;

	return strncmp (line, SEARCH_HEADER, strlen (SEARCH_HEADER)) == 0;
}


/* escape */


char *
escape_underscore (const char *name)
{
	const char *s;
	char       *e_name, *t;
	int         l = 0, us = 0;

	if (name == NULL)
		return NULL;

	for (s = name; *s != 0; s++) {
		if (*s == '_')
			us++;
		l++;
	}

	if (us == 0)
		return g_strdup (name);

	e_name = g_malloc (sizeof (char) * (l + us + 1));

	t = e_name;
	for (s = name; *s != 0; s++)
		if (*s == '_') {
			*t++ = '_';
			*t++ = '_';
		} else
			*t++ = *s;
	*t = 0;

	return e_name;
}


char *
escape_uri (const char *uri)
{
	const char *start = NULL;
	const char *uri_no_method;
	char       *method;
	char       *epath, *euri;

	if (uri == NULL)
		return NULL;

	start = strstr (uri, "://");
	if (start != NULL) {
		uri_no_method = start + strlen ("://");
		method = g_strndup (uri, start - uri);
	} else {
		uri_no_method = uri;
		method = NULL;
	}

	epath = gnome_vfs_escape_host_and_path_string (uri_no_method);

	if (method != NULL) {
		euri = g_strdup_printf ("%s://%s", method, epath);
		g_free (epath);
	} else
		euri = epath;

	g_free (method);

	return euri;
}


/* characters to escape */
static gchar bad_char[] = { '$', '\'', '`', '"', '\\', '!', '?', '*',
			    ' ', '(', ')', '[', ']', '&', '|', '@' , '#',
			    ';', '<', '>' };

/* the size of bad_char */
static const gint bad_chars = sizeof (bad_char) / sizeof (gchar);


/* counts how many characters to escape in @str. */
static gint
count_chars_to_escape (const gchar *str)
{
	const gchar *s;
	gint i, n;

	n = 0;
	for (s = str; *s != 0; s++)
		for (i = 0; i < bad_chars; i++)
			if (*s == bad_char[i]) {
				n++;
				break;
			}
	return n;
}


/* escape with backslash the file name. */
gchar*
shell_escape (const gchar *filename)
{
	gchar *escaped;
	gint i, new_l;
	const gchar *s;
	gchar *t;

	if (filename == NULL)
		return NULL;

	new_l = strlen (filename) + count_chars_to_escape (filename);
	escaped = g_malloc (new_l + 1);

	s = filename;
	t = escaped;
	while (*s) {
		gboolean is_bad;

		is_bad = FALSE;
		for (i = 0; (i < bad_chars) && !is_bad; i++)
			is_bad = (*s == bad_char[i]);

		if (is_bad)
			*t++ = '\\';
		*t++ = *s++;
	}
	*t = 0;

	return escaped;
}


/* extension */


gboolean
file_extension_is (const char *filename,
		   const char *ext)
{
	return ! strcasecmp (filename + strlen (filename) - strlen (ext), ext);
}


char *
get_filename_extension (const char *filename)
{
        GFile  *gfile;
	char   *result;

        gfile = gfile_new (filename);
        result = gfile_get_filename_extension (gfile);
        g_object_unref (gfile);

        return result;
}


char *
remove_extension_from_path (const char *path)
{
	int         len;
	int         p;
	const char *ptr = path;
	char       *new_path;

	if (! path)
		return NULL;

	len = strlen (path);
	if (len == 1)
		return g_strdup (path);

	p = len - 1;
	while ((p > 0) && (ptr[p] != '.'))
		p--;
	if (p == 0)
		p = len;
	new_path = g_strndup (path, (guint) p);

	return new_path;
}


/* temp */

/* Note: callers of get_temp_dir_name seem to expect a path */

char *
get_temp_dir_name (void)
{
	GFile *dir;
	char  *path;
	
	dir = gfile_get_temp_dir_name ();
	
	if (dir == NULL)
		return NULL;
	
	path = g_file_get_path (dir);
	
	g_object_unref (dir);
	
	return path;
}


/* WARNING: this is not secure unless only the current user has write access to
tmpdir.  Use get_temp_dir_name to create tmpdir securely. */
char *
get_temp_file_name (const char *tmpdir, 
		    const char *ext)
{
	static GStaticMutex  count_mutex = G_STATIC_MUTEX_INIT;
	static int           count = 0;
	char                *name, *filename;

	if (tmpdir == NULL)
		return NULL;

	g_static_mutex_lock (&count_mutex);
	if (ext != NULL)
		name = g_strdup_printf ("%d.%s", count++, ext);
	else
		name = g_strdup_printf ("%d", count++);
	g_static_mutex_unlock (&count_mutex);

	filename = g_build_filename (tmpdir, name, NULL);

	g_free (name);

	return filename;
}


/* VFS extensions */


/* -- copy_file_async --  */


struct _CopyData {
        char                *source_uri;
        char                *target_uri;
        GError              *error;
        GnomeVFSResult       result;
        GnomeVFSAsyncHandle *handle;
        CopyDoneFunc         done_func;
        gpointer             done_data;
        guint                idle_id;
};


static CopyData*
copy_data_new (const char   *source_uri,
	       const char   *target_uri,
	       CopyDoneFunc  done_func,
	       gpointer      done_data)
{
	CopyData *copy_data;
	
	copy_data = g_new0 (CopyData, 1);
	copy_data->source_uri = g_strdup (source_uri);
	copy_data->target_uri = g_strdup (target_uri);
	copy_data->done_func = done_func;
	copy_data->done_data = done_data;
	copy_data->result = GNOME_VFS_OK;
	copy_data->idle_id = 0;
	copy_data->error = NULL;
	
	return copy_data;
}


static void
copy_data_free (CopyData *data)
{
        if (data == NULL)
                return;
        g_free (data->source_uri);
        g_free (data->target_uri);
        g_free (data);
}


void
copy_data_cancel (CopyData *data)
{
	if (data == NULL)
		return;
	if (data->handle != NULL) 
		gnome_vfs_async_cancel (data->handle);
	copy_data_free (data);
}


static gboolean
copy_file_async_done (gpointer data)
{
	CopyData *copy_data = data;
	
	if (copy_data->idle_id != 0) {
		g_source_remove (copy_data->idle_id);
		copy_data->idle_id = 0;
	}
	
	if (copy_data->done_func != NULL) {
		copy_data->handle = NULL;	
		(copy_data->done_func) (copy_data->target_uri, copy_data->error, copy_data->done_data);
	}
	copy_data_free (copy_data);
	
	return FALSE;
}


static int
copy_file_async_progress_update_cb (GnomeVFSAsyncHandle      *handle,
				    GnomeVFSXferProgressInfo *info,
				    gpointer                  user_data)
{
	CopyData *copy_data = user_data;

	if (info->status != GNOME_VFS_XFER_PROGRESS_STATUS_OK) {
		copy_data->result = info->vfs_status;
		return FALSE;
	}
	else if (info->phase == GNOME_VFS_XFER_PHASE_COMPLETED) 
		copy_data->idle_id = g_idle_add (copy_file_async_done, copy_data);
	/*else if ((info->phase == GNOME_VFS_XFER_PHASE_COPYING)
		 || (info->phase == GNOME_VFS_XFER_PHASE_MOVING))
		g_signal_emit (G_OBJECT (copy_data->archive),
			       fr_archive_signals[PROGRESS],
			       0,
			       (double) info->total_bytes_copied / info->bytes_total);*/
			       
	return TRUE;
}


CopyData *
copy_file_async (const char   *source_uri,
		 const char   *target_uri,
		 CopyDoneFunc  done_func,
		 gpointer      done_data)
{
	/* TODO: copy_data->error is not set properly? */

	CopyData       *copy_data;
	GList          *source_uri_list, *target_uri_list;
	GnomeVFSResult  result;

	copy_data = copy_data_new (source_uri, target_uri, done_func, done_data);

	if (! path_is_file (source_uri)) {
		copy_data->result = GNOME_VFS_ERROR_NOT_FOUND;
		copy_data->idle_id = g_idle_add (copy_file_async_done, copy_data);
		return NULL;
	}

	source_uri_list = g_list_append (NULL, gnome_vfs_uri_new (source_uri));
	target_uri_list = g_list_append (NULL, gnome_vfs_uri_new (target_uri));

	result = gnome_vfs_async_xfer (&copy_data->handle,
				       source_uri_list,
				       target_uri_list,
				       GNOME_VFS_XFER_DEFAULT | GNOME_VFS_XFER_FOLLOW_LINKS,
				       GNOME_VFS_XFER_ERROR_MODE_ABORT,
				       GNOME_VFS_XFER_OVERWRITE_MODE_REPLACE,
				       GNOME_VFS_PRIORITY_DEFAULT,
				       copy_file_async_progress_update_cb, copy_data,
				       NULL, NULL);

	gnome_vfs_uri_list_free (source_uri_list);
	gnome_vfs_uri_list_free (target_uri_list);

	if (result != GNOME_VFS_OK) {
		copy_data->result = result;
		copy_data->idle_id = g_idle_add (copy_file_async_done, copy_data);
	}
	
	return copy_data;
}


/* -- */


guint64
get_destination_free_space (const char *path)
{
	GFile   *file;
	guint64  result;
	
	if (path == NULL)
		return 0;
	
	file = gfile_new (path);

	result = gfile_get_destination_free_space (file);
	
	g_object_unref (file);

	return result;
}


gboolean
is_mime_type_writable (const char *mime_type)
{
	GSList *list, *scan;

	list = gdk_pixbuf_get_formats();
	for (scan = list; scan; scan = scan->next) {
		GdkPixbufFormat *format = scan->data;
		char **mime_types;
		int i;
		mime_types = gdk_pixbuf_format_get_mime_types (format);
		for (i = 0; mime_types[i] != NULL; i++)
			if (strcmp (mime_type, mime_types[i]) == 0)
				return gdk_pixbuf_format_is_writable (format);
		g_strfreev (mime_types);
	}
	g_slist_free (list);

	return FALSE;
}


/* VFS caching */


gboolean
is_local_file (const char *filename)
{
	return (! uri_has_scheme (filename)) || uri_scheme_is_file (filename);
}


char *
get_cache_full_path (const char *filename, 
		     const char *extension)
{
	if (extension == NULL)
                return g_strconcat (g_get_home_dir (),
                                    "/",
                                    RC_REMOTE_CACHE_DIR,
                                    ((filename == NULL) ? "" : "/"),
                                    filename,
                                    NULL);	
	else
		return g_strconcat (g_get_home_dir (),
				    "/",
				    RC_REMOTE_CACHE_DIR,
				    ((filename == NULL) ? "" : "/"),
				    filename,
				    ".",
				    extension,
				    NULL);
}


void
free_cache (void)
{
	char  *cache_dir;
	GFile *cache_gfile;
	GList *files = NULL;
	
	cache_dir = get_cache_full_path (NULL, NULL);
	cache_gfile = gfile_new (cache_dir);
	g_free (cache_dir);
	
	if (gfile_path_list_new (cache_gfile, &files, NULL)) {
		GList *scan;
		for (scan = files; scan; scan = scan->next ) {
			FileData *file = scan->data;
			file_unlink (file->path);
		}
	}

	g_object_unref (cache_gfile);
	file_data_list_free (files);
}


static GdkPixbuf*
get_pixbuf_using_external_converter (FileData   *file,
				     int         requested_width,
				     int         requested_height)
{
	char	   *cache_file;
	char       *md5_file;
	char	   *cache_file_full;
	char	   *cache_file_esc;
	char	   *local_file_esc;
	char	   *command = NULL;
	GdkPixbuf  *pixbuf = NULL;
	gboolean    is_raw;
	gboolean    is_hdr;
	gboolean    is_thumbnail;

	if (! file_data_has_local_path (file, NULL))
		return NULL;

	is_thumbnail = requested_width > 0;

	is_raw = mime_type_is_raw (file->mime_type);
	is_hdr = mime_type_is_hdr (file->mime_type);

	/* The output filename, and its persistence, depend on the input file
	   type, and whether or not a thumbnail has been requested. */

	md5_file = gnome_thumbnail_md5 (file->local_path);
	
	if (is_raw && !is_thumbnail)
		/* Full-sized converted RAW file */
		cache_file_full = get_cache_full_path (md5_file, "conv.pnm");
	else if (is_raw && is_thumbnail)
		/* RAW: thumbnails generated in pnm format. The converted file is later removed. */
		cache_file_full = get_cache_full_path (md5_file, "conv-thumb.pnm");
	else if (is_hdr && is_thumbnail)
		/* HDR: thumbnails generated in tiff format. The converted file is later removed. */
		cache_file_full = get_cache_full_path (md5_file, "conv-thumb.tiff");
	else
		/* Full-sized converted HDR files */
		cache_file_full = get_cache_full_path (md5_file, "conv.tiff");

	cache_file = g_strdup (remove_host_from_uri (cache_file_full));
	cache_file_esc = g_shell_quote (cache_file);

	g_free (cache_file_full);
	g_free (md5_file);

	if (cache_file == NULL) {
		g_free (cache_file);
		g_free (cache_file_esc);
		return NULL;
	}

	local_file_esc = g_shell_quote (file->local_path);

	/* Do nothing if an up-to-date converted file is already in the cache */
	if (! path_is_file (cache_file) || (file->mtime > get_file_mtime (cache_file))) {
		if (is_raw) {
			if (is_thumbnail) {
				char *first_part;
			       	char *jpg_thumbnail;
				char *tiff_thumbnail;
				char *ppm_thumbnail;
				char *thumb_command;

				/* Check for an embedded thumbnail first */
				thumb_command = g_strdup_printf ("dcraw -e %s", local_file_esc);
		        	g_spawn_command_line_sync (thumb_command, NULL, NULL, NULL, NULL);
				g_free (thumb_command);

				first_part = remove_extension_from_path (file->local_path);
				jpg_thumbnail = g_strdup_printf ("%s.thumb.jpg", first_part);
				tiff_thumbnail = g_strdup_printf ("%s.thumb.tiff", first_part);
				ppm_thumbnail = g_strdup_printf ("%s.thumb.ppm", first_part);

				if (path_exists (jpg_thumbnail)) {
					g_free (cache_file);
					cache_file = g_strdup (jpg_thumbnail);
				} 
				else if (path_exists (tiff_thumbnail)) {
                                        g_free (cache_file);
                                        cache_file = g_strdup (tiff_thumbnail);
				} 
				else if (path_exists (ppm_thumbnail)) {
                                        g_free (cache_file);
                                        cache_file = g_strdup (ppm_thumbnail);
				} 
				else {
					/* No embedded thumbnail. Read the whole file. */
					/* Add -h option to speed up thumbnail generation. */
					command = g_strdup_printf ("dcraw -w -c -h %s > %s",
								   local_file_esc,
								   cache_file_esc);
				}
				g_free (first_part);
				g_free (jpg_thumbnail);
				g_free (tiff_thumbnail);
				g_free (ppm_thumbnail);
			} 
			else {
				/* -w option = camera-specified white balance */
                                command = g_strdup_printf ("dcraw -w -c %s > %s",
                                                           local_file_esc,
                                                           cache_file_esc);
			}
		}

		if (is_hdr) {
			/* HDR files. We can use the pfssize tool to speed up
			   thumbnail generation considerably, so we treat
			   thumbnailing as a special case. */
			char *resize_command;

			if (is_thumbnail)
				resize_command = g_strdup_printf (" | pfssize --maxx %d --maxy %d",
								  requested_width,
								  requested_height);
			else
				resize_command = g_strdup_printf (" ");

			command = g_strconcat ( "pfsin ",
						local_file_esc,
						resize_command,
						" |  pfsclamp  --rgb  | pfstmo_drago03 | pfsout ",
						cache_file_esc,
						NULL );
			g_free (resize_command);
		}

		if (command != NULL) {
		       	system (command);
			g_free (command);
		}
	}

	pixbuf = gdk_pixbuf_new_from_file (cache_file, NULL);

	/* Thumbnail files are already cached, so delete the conversion cache copies */
	if (is_thumbnail)
		file_unlink (cache_file);

	g_free (cache_file);
	g_free (cache_file_esc);
	g_free (local_file_esc);

	return pixbuf;
}


static GdkPixbuf*
gth_pixbuf_new_from_video (FileData               *file,
			   GnomeThumbnailFactory  *factory,
			   GError                **error,
			   gboolean                resolve_symlinks)
{
      	GdkPixbuf *pixbuf = NULL;
	char      *thumbnail_uri;

	thumbnail_uri = gnome_thumbnail_factory_lookup (factory,
							file->uri,
							file->mtime);
	if (thumbnail_uri != NULL) {
		FileData *fd_thumb;
		fd_thumb = file_data_new (thumbnail_uri);
		pixbuf = gdk_pixbuf_new_from_file (fd_thumb->local_path, error);
		file_data_unref (fd_thumb);
	}
	else if (gnome_thumbnail_factory_has_valid_failed_thumbnail (factory, 
								     file->uri, 
								     file->mtime)) 
	{ 
		pixbuf = NULL;
	}
	else 
		pixbuf = gnome_thumbnail_factory_generate_thumbnail (factory,
								     file->uri, 
								     file->mime_type);

	g_free (thumbnail_uri);

	return pixbuf;
}


#ifndef GDK_PIXBUF_CHECK_VERSION
#define GDK_PIXBUF_CHECK_VERSION(major,minor,micro) \
    (GDK_PIXBUF_MAJOR > (major) || \
     (GDK_PIXBUF_MAJOR == (major) && GDK_PIXBUF_MINOR > (minor)) || \
     (GDK_PIXBUF_MAJOR == (major) && GDK_PIXBUF_MINOR == (minor) && \
      GDK_PIXBUF_MICRO >= (micro)))
#endif


GdkPixbuf*
gth_pixbuf_new_from_file (FileData               *file,
			  GError                **error,
			  int                     requested_width,
			  int                     requested_height,
			  GnomeThumbnailFactory  *factory)
{
	GdkPixbuf     *pixbuf = NULL;
	GdkPixbuf     *rotated = NULL;

	if (file == NULL)
		return NULL;

	if (! file_data_has_local_path (file, NULL))
		return NULL;

	if (mime_type_is_video (file->mime_type)) {
		if (factory != NULL) 
			return gth_pixbuf_new_from_video (file, factory, error, (requested_width == 0));
		return NULL;
	}

#ifdef HAVE_LIBOPENRAW
	/* Raw thumbnails - using libopenraw is much faster than using dcraw for
	   thumbnails. Use libopenraw for full raw images too, once it matures. */
	if ((pixbuf == NULL) 
	    && mime_type_is_raw (file->mime_type) 
	    && (requested_width > 0))
		pixbuf = or_gdkpixbuf_extract_thumbnail (file->local_path, requested_width);
#endif

	/* Use dcraw for raw images, pfstools for HDR images */
	if ((pixbuf == NULL) &&
	     (mime_type_is_raw (file->mime_type) ||
	      mime_type_is_hdr (file->mime_type) )) 
		pixbuf = get_pixbuf_using_external_converter (file,
							      requested_width,
							      requested_height);

	/* Otherwise, use standard gdk_pixbuf loaders */
	if ((pixbuf == NULL) && (requested_width > 0)) {
		int w, h;
		
		if (gdk_pixbuf_get_file_info (file->local_path, &w, &h) == NULL) {
			w = -1;
			h = -1;
		}
		
		/* scale the image only if the original size is larger than
		 * the requested size. */
		if ((w > requested_width) || (h > requested_height))
			pixbuf = gdk_pixbuf_new_from_file_at_scale (file->local_path,
                        	                                    requested_width,
                                	                            requested_height,
                                        	                    TRUE,
                                                	            error);
		else
			pixbuf = gdk_pixbuf_new_from_file (file->local_path, error);
	}
	else if (pixbuf == NULL)
		/* otherwise, no scaling required */
		pixbuf = gdk_pixbuf_new_from_file (file->local_path, error);

	/* Did any of the loaders work? */
	if (pixbuf == NULL)
		return NULL;

	/* rotate pixbuf if required, based on exif orientation tag (jpeg only) */

	debug (DEBUG_INFO, "Check orientation tag of %s. Width %d\n\r", file->local_path, requested_width);

#if GDK_PIXBUF_CHECK_VERSION(2,11,5)
        /* New in gtk 2.11.5 - see bug 439567 */
        rotated = gdk_pixbuf_apply_embedded_orientation (pixbuf);      	
        debug (DEBUG_INFO, "Applying orientation using gtk function.\n\r");
#else
	/* The old way - delete this once gtk 2.12 is widely used */
	if (mime_type_is (file->mime_type, "image/jpeg")) {
		GthTransform  orientation;
		GthTransform  transform = GTH_TRANSFORM_NONE;
		
		orientation = get_orientation_from_fd (file);
		transform = (orientation >= 1 && orientation <= 8 ? orientation : GTH_TRANSFORM_NONE);
		
		debug (DEBUG_INFO, "read_orientation_field says orientation is %d, transform needed is %d.\n\r", orientation, transform);
		rotated = _gdk_pixbuf_transform (pixbuf, transform);
	}
#endif	

	if (rotated == NULL) {
		rotated = pixbuf;
		g_object_ref (rotated);
	}

	g_object_unref (pixbuf);

	return rotated;
}


GdkPixbufAnimation*
gth_pixbuf_animation_new_from_file (FileData               *file,
				    GError                **error,
				    int                     requested_width,
				    int                     requested_height,
				    GnomeThumbnailFactory  *factory)
{
	GdkPixbufAnimation *animation = NULL;
	GdkPixbuf          *pixbuf = NULL;

	if (file->mime_type == NULL)
		return NULL;

	if (! file_data_has_local_path (file, NULL))
		return NULL;

	if (mime_type_is (file->mime_type, "image/gif")) {
		animation = gdk_pixbuf_animation_new_from_file (file->local_path, error);
		return animation;
	}
 
	if (pixbuf == NULL) 
		pixbuf = gth_pixbuf_new_from_file (file,
						   error,
						   requested_width,
						   requested_height,
						   factory);

	if (pixbuf != NULL) {
		animation = gdk_pixbuf_non_anim_new (pixbuf);
		g_object_unref (pixbuf);
	}

	return animation;
}


GHashTable *
read_dot_hidden_file (const char *uri)
{
	GHashTable       *hidden_files;
	char             *dot_hidden_uri;
        GFileInputStream *istream;
        GDataInputStream *dstream;
        GError           *error = NULL;
        GFile            *gfile;
	char             *line;
	gsize             length;

	hidden_files = g_hash_table_new_full (g_str_hash,
					      g_str_equal,
					      (GDestroyNotify) g_free,
					      NULL);

	if (eel_gconf_get_boolean (PREF_SHOW_HIDDEN_FILES, FALSE))
		return hidden_files;

	dot_hidden_uri = g_build_filename (uri, ".hidden", NULL);

	gfile = gfile_new (dot_hidden_uri);
	g_free (dot_hidden_uri);

        istream = g_file_read (gfile, NULL, &error);
        if (error != NULL) {
                g_error_free (error);
		return hidden_files;
	}
	dstream = g_data_input_stream_new (G_INPUT_STREAM(istream));

        while ((line = g_data_input_stream_read_line (dstream, &length, NULL, &error))) {
		char *path;

		line[strlen (line)] = 0;
		path = gnome_vfs_escape_string (line);

		if (g_hash_table_lookup (hidden_files, path) == NULL)
			g_hash_table_insert (hidden_files, path, GINT_TO_POINTER (1));
		else
			g_free (path);
	}

        if (error) {
                gfile_warning ("Error reading line from .hidden file", gfile, error);
                g_error_free (error);
        }

        g_object_unref (dstream);
        g_object_unref (istream);
        g_object_unref (gfile);

	return hidden_files;
}


char *
xdg_user_dir_lookup (const char *type)
{
	/* This function is used by gthumb to determine the default "PICTURES"
	   directory for imports. The actually directory name is localized. 
	   Bug 425365. */

	FILE *file;
	char *home_dir, *config_home, *config_file;
	char buffer[512];
	char *user_dir;
	char *p, *d;
	int len;
	int relative;

	home_dir = getenv ("HOME");
	if (home_dir == NULL)
		return strdup ("/tmp");

	config_home = getenv ("XDG_CONFIG_HOME");
	if (config_home == NULL || config_home[0] == 0) {
		config_file = malloc (strlen (home_dir) + strlen ("/.config/user-dirs.dirs") + 1);
		strcpy (config_file, home_dir);
		strcat (config_file, "/.config/user-dirs.dirs");
	} 
	else {
		config_file = malloc (strlen (config_home) + strlen ("/user-dirs.dirs") + 1);
		strcpy (config_file, config_home);
		strcat (config_file, "/user-dirs.dirs");
	}

	file = fopen (config_file, "r");
	free (config_file);
	if (file == NULL)
		goto error;

	user_dir = NULL;

	while (fgets (buffer, sizeof (buffer), file)) {
		/* Remove newline at end */
		len = strlen (buffer);
		if (len > 0 && buffer[len-1] == '\n')
		buffer[len-1] = 0;
      
		p = buffer;
		while (*p == ' ' || *p == '\t')
			p++;
      
		if (strncmp (p, "XDG_", 4) != 0)
			continue;
		p += 4;

		if (strncmp (p, type, strlen (type)) != 0)
			continue;
		p += strlen (type);

		if (strncmp (p, "_DIR", 4) != 0)
		continue;
		p += 4;

		while (*p == ' ' || *p == '\t')
			p++;

		if (*p != '=')
			continue;
		p++;
      
		while (*p == ' ' || *p == '\t')
			p++;

		if (*p != '"')
			continue;
		p++;
      
		relative = 0;
		if (strncmp (p, "$HOME/", 6) == 0) {
			p += 6;
			relative = 1;
		} else if (*p != '/')
			continue;
      
		if (relative) {
			user_dir = malloc (strlen (home_dir) + 1 + strlen (p) + 1);
			strcpy (user_dir, home_dir);
			strcat (user_dir, "/");
		} else {
			user_dir = malloc (strlen (p) + 1);
			*user_dir = 0;
		}
      
		d = user_dir + strlen (user_dir);
		while (*p && *p != '"')	{
			if ((*p == '\\') && (*(p+1) != 0))
				p++;
			*d++ = *p++;
		}
		*d = 0;
	}  
	
	fclose (file);

	if (user_dir) {
		ensure_dir_exists (user_dir);
		return user_dir;
	}

error:
	 /* Special case desktop for historical compatibility */
	if (strcmp (type, "DESKTOP") == 0) {
		user_dir = malloc (strlen (home_dir) + strlen ("/Desktop") + 1);
		strcpy (user_dir, home_dir);
		strcat (user_dir, "/Desktop");
		return user_dir;
	} else
		return strdup (home_dir);
}
