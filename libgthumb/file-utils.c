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

#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <dirent.h>
#include <errno.h>

#include <glib.h>
#include <glib/gi18n.h>
#include <libgnomevfs/gnome-vfs.h>
#include <libgnomevfs/gnome-vfs-handle.h>
#include <libgnomevfs/gnome-vfs-mime.h>
#include <libgnomevfs/gnome-vfs-result.h>
#include <gconf/gconf-client.h>

#include "gthumb-init.h"
#include "gconf-utils.h"
#include "file-utils.h"


#define BUF_SIZE 4096
#define MAX_SYMLINKS_FOLLOWED 32


/* -- path_list_async_new implementation -- */


PathListData * 
path_list_data_new ()
{
	PathListData *pli;

	pli = g_new0 (PathListData, 1);

	pli->uri = NULL;
	pli->result = GNOME_VFS_OK;
	pli->files = NULL;
	pli->dirs = NULL;
	pli->done_func = NULL;
	pli->done_data = NULL;
	pli->interrupt_func = NULL;
	pli->interrupt_data = NULL;
	pli->interrupted = FALSE;

	return pli;
}


void 
path_list_data_free (PathListData *pli)
{
	g_return_if_fail (pli != NULL);

	if (pli->uri != NULL)
		gnome_vfs_uri_unref (pli->uri);

	if (pli->files != NULL) {
		g_list_foreach (pli->files, (GFunc) g_free, NULL);
		g_list_free (pli->files);
	}

	if (pli->dirs != NULL) {
		g_list_foreach (pli->dirs, (GFunc) g_free, NULL);
		g_list_free (pli->dirs);
	}

	g_free (pli);
}


void
path_list_handle_free (PathListHandle *handle)
{
	if (handle->pli_data != NULL)
		path_list_data_free (handle->pli_data);
	g_free (handle);
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
		if (pli->interrupt_func) 
			pli->interrupt_func (pli->interrupt_data);
		path_list_data_free (pli);
		return;
	}

	for (node = list; node != NULL; node = node->next) {
		GnomeVFSFileInfo *info     = node->data;
		GnomeVFSURI      *full_uri = NULL;
		char             *str_uri;
		char             *unesc_uri;

		switch (info->type) {
		case GNOME_VFS_FILE_TYPE_REGULAR:
			full_uri = gnome_vfs_uri_append_file_name (pli->uri, info->name);
			str_uri = gnome_vfs_uri_to_string (full_uri, GNOME_VFS_URI_HIDE_NONE);
			unesc_uri = gnome_vfs_unescape_string (str_uri, NULL);
			pli->files = g_list_prepend (pli->files, unesc_uri);
			g_free (str_uri);
			break;

		case GNOME_VFS_FILE_TYPE_DIRECTORY:
			if (SPECIAL_DIR (info->name))
				break;

			full_uri = gnome_vfs_uri_append_path (pli->uri, info->name);
			str_uri = gnome_vfs_uri_to_string (full_uri, GNOME_VFS_URI_HIDE_NONE);
			unesc_uri = gnome_vfs_unescape_string (str_uri, NULL);

			pli->dirs = g_list_prepend (pli->dirs,  unesc_uri);
			g_free (str_uri);
			break;

		default:
			break;
		}

		if (full_uri)
			gnome_vfs_uri_unref (full_uri);
	}

	if ((result == GNOME_VFS_ERROR_EOF) 
	    || (result != GNOME_VFS_OK)) {
		if (pli->done_func) 
			/* pli is deallocated in pli->done_func */
			pli->done_func (pli, pli->done_data);
		else
			path_list_data_free (pli);

		return;
	} 
}


PathListHandle *
path_list_async_new (const char       *uri, 
		     PathListDoneFunc  f,
		     gpointer         data)
{
	GnomeVFSAsyncHandle *handle;
	PathListData        *pli;
	PathListHandle      *pl_handle;

	if (uri == NULL) {
		if (f != NULL)
			(f) (NULL, data);
		return NULL;
	}

	pli = path_list_data_new ();

	pli->uri = new_uri_from_path (uri);
	if (pli->uri == NULL) {
		path_list_data_free (pli);
		if (f != NULL)
			(f) (NULL, data);
		return NULL;
	}

	pli->done_func = f;
	pli->done_data = data;

	gnome_vfs_async_load_directory_uri (
		&handle,
		pli->uri,
		GNOME_VFS_FILE_INFO_FOLLOW_LINKS,
		128 /* items_per_notification FIXME */,
		GNOME_VFS_PRIORITY_DEFAULT,
		directory_load_cb,
		pli);

	pl_handle = g_new (PathListHandle, 1);
	pl_handle->vfs_handle = handle;
	pl_handle->pli_data = pli;

	return pl_handle;
}


void
path_list_async_interrupt (PathListHandle   *handle,
			   DoneFunc          f,
			   gpointer          data)
{
	handle->pli_data->interrupted = TRUE;
	handle->pli_data->interrupt_func = f;
	handle->pli_data->interrupt_data = data;

	g_free (handle);
}


gboolean 
path_list_new (const char  *path, 
	       GList      **files, 
	       GList      **dirs)
{
	GnomeVFSResult  r;
	char           *epath;
	GnomeVFSURI    *dir_uri;
	GList          *info_list = NULL;
	GList          *scan;
	GList          *f_list = NULL;
	GList          *d_list = NULL;

	if (files) *files = NULL;
	if (dirs) *dirs = NULL;

	epath = escape_uri (path);
	r = gnome_vfs_directory_list_load (&info_list,
					   epath,
					   GNOME_VFS_FILE_INFO_FOLLOW_LINKS);
	g_free (epath);

	if (r != GNOME_VFS_OK) 
		return FALSE;

	dir_uri = new_uri_from_path (path);
	for (scan = info_list; scan; scan = scan->next) {
		GnomeVFSFileInfo *info = scan->data;
		GnomeVFSURI      *full_uri = NULL;
		char             *s_uri, *unesc_uri;

		full_uri = gnome_vfs_uri_append_file_name (dir_uri, info->name);
		s_uri = gnome_vfs_uri_to_string (full_uri, GNOME_VFS_URI_HIDE_NONE);
		unesc_uri = gnome_vfs_unescape_string (s_uri, NULL);
		g_free (s_uri);

		if (info->type == GNOME_VFS_FILE_TYPE_DIRECTORY) {
			if (! SPECIAL_DIR (info->name))
				d_list = g_list_prepend (d_list, unesc_uri);
		} else if (info->type == GNOME_VFS_FILE_TYPE_REGULAR)
			f_list = g_list_prepend (f_list, unesc_uri);
		else
			g_free (unesc_uri);
	}
	gnome_vfs_file_info_list_free (info_list);

	if (dirs) 
		*dirs = g_list_reverse (d_list);
	else
		path_list_free (d_list);

	if (files) 
		*files = g_list_reverse (f_list);
	else
		path_list_free (f_list);

	return TRUE;
}


GList *
path_list_dup (GList *path_list)
{
	GList *new_list = NULL;
	GList *scan;

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


void
path_list_print (GList *list) 
{
	GList *scan;
	for (scan = list; scan; scan = scan->next) {
		char *path = scan->data;
		g_print ("--> %s\n", path);
	}
}


GList *
path_list_find_path (GList *list, const char *path)
{
	GList *scan;

	for (scan = list; scan; scan = scan->next)
		if (strcmp ((char*) scan->data, path) == 0)
			return scan;
	return NULL;
}


gboolean
dir_is_empty (const gchar *path)
{
	DIR *dp;
	int n;

	if (strcmp (path, "/") == 0)
		return FALSE;

	dp = opendir (path);
	n = 0;
	while (readdir (dp) != NULL) {
		n++;
		if (n > 2) {
			closedir (dp);
			return FALSE;
		}
	}
	closedir (dp);

	return TRUE;
}


gboolean
dir_make (const gchar *path, 
	  mode_t       mode)
{
	GnomeVFSResult  r;
	char           *epath;

	epath = escape_uri (path);
	r = gnome_vfs_make_directory (epath, mode);
	g_free (epath);

	return (r == GNOME_VFS_OK);
}


gboolean
dir_remove (const gchar *path)
{
	GnomeVFSResult  r;
	char           *epath;

	epath = escape_uri (path);
	r = gnome_vfs_remove_directory (epath);
	g_free (epath);

	return (r == GNOME_VFS_OK);
}


gboolean
dir_remove_recursive (const gchar *directory)
{
	GList    *files, *dirs;
	GList    *scan;
	gboolean  error = FALSE;

	if (! path_is_dir (directory)) 
		return FALSE;

	path_list_new (directory, &files, &dirs);

	for (scan = files; scan; scan = scan->next) {
		char *file = scan->data;
		if (! file_unlink (file)) {
			g_warning ("Cannot delete %s\n", file);
			error = TRUE;
		}
	}
	path_list_free (files);

	for (scan = dirs; scan; scan = scan->next) {
		char *sub_dir = scan->data;
		if (!dir_remove_recursive (sub_dir))
			error = TRUE;
	}
	path_list_free (dirs);

	if (!dir_remove (directory))
		error = TRUE;

	return !error;
}


gboolean 
ensure_dir_exists (const char *a_path, 
		   mode_t      mode)
{
	char *path;
	char *p;

	if (! a_path) 
		return FALSE;

	if (path_is_dir (a_path))
		return TRUE;

	path = g_strdup (a_path);

	p = strstr (path, "://");
	if (p == NULL)   /* Not a URI */
		p = path;
	else  /* Is a URI */
		p = p + 3;  /* Move p past the :// */

	while (*p != '\0') {
		p++;
		if ((*p == '/') || (*p == '\0')) {
			gboolean end = TRUE;
			
			if (*p != '\0') {
				*p = '\0';
				end = FALSE;
			}
			
			if (! path_is_dir (path)) {
				if (!dir_make (path, mode)) {
					g_warning ("directory creation failed: %s.", path);
					g_free (path);
					return FALSE;
				}
			}
			if (! end) *p = '/';
		}
	}

	g_free (path);

	return TRUE;
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
        filtered = g_list_sort (filtered, (GCompareFunc) strcasecmp);

        return filtered;
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
	
	for (scan = files; scan; scan = scan->next) {
                char *rc_file, *real_file;
		
                rc_file = (char*) scan->data;
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

        return TRUE;
}


static const char *
get_extension (const char *path)
{
	int         len;
	int         p;
	const char *ptr = path;

	if (! path) 
		return NULL;

	len = strlen (path);
	if (len <= 1) 
		return NULL;

	p = len - 1;
	while ((ptr[p] != '.') && (p >= 0)) 
		p--;

	if (p < 0)
		return NULL;

	return path + p;
}


static char*
get_sample_name (const char *filename)
{
	const char *ext;
	ext = get_extension (filename);
	if (ext == NULL)
		return NULL;
	return g_strconcat ("a", get_extension (filename), NULL);
}


gboolean
file_is_image (const gchar *name,
	       gboolean     fast_file_type)
{
	const char *result = NULL;
	gboolean    is_an_image;

	if (fast_file_type) {
		char *filename, *n1;

		filename = get_sample_name (name);
		if (filename == NULL)
			return FALSE;

		n1 = g_filename_to_utf8 (filename, -1, 0, 0, 0);
		if (n1 != NULL) {
			char *n2, *n3;
			n2 = g_utf8_strdown (n1, -1);
			n3 = g_filename_from_utf8 (n2, -1, 0, 0, 0);

			if (n3 != NULL)
				result = gnome_vfs_mime_type_from_name_or_default (n3, NULL);

			g_free (n3);
			g_free (n2);
			g_free (n1);
		}
	} else 
		result = gnome_vfs_get_file_mime_type (name, NULL, FALSE);

	/* Unknown file type. */
	if (result == NULL)
		return FALSE;

	/* If the description contains the word 'image' than we suppose 
	 * it is an image that gdk-pixbuf can load. */
	is_an_image = strstr (result, "image") != NULL;

	return is_an_image;
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
	   gboolean    move) 
{
	GnomeVFSURI         *from_uri, *to_uri;
	GnomeVFSXferOptions  opt;
	GnomeVFSResult       result;

	if (strcmp (from, to) == 0) {
		g_warning ("cannot copy file %s: source and destination are the same\n", from);
		return FALSE;
	}

	from_uri = new_uri_from_path (from);
	to_uri = new_uri_from_path (to);
	opt = move ? GNOME_VFS_XFER_REMOVESOURCE : GNOME_VFS_XFER_DEFAULT;
	result = gnome_vfs_xfer_uri (from_uri,
				     to_uri,
				     opt,
				     GNOME_VFS_XFER_ERROR_MODE_ABORT,
				     GNOME_VFS_XFER_OVERWRITE_MODE_REPLACE,
				     NULL,
				     NULL);

	gnome_vfs_uri_unref (from_uri);
	gnome_vfs_uri_unref (to_uri);

	return (result == GNOME_VFS_OK);
}


gboolean 
file_copy (const char *from, 
	   const char *to)
{
	return xfer_file (from, to, FALSE);
}


gboolean
file_move (const gchar *from, 
	   const gchar *to)
{
	return xfer_file (from, to, TRUE);
}


gboolean
file_unlink (const gchar *path)
{
	GnomeVFSResult  r;
	char           *epath;

	epath = escape_uri (path);
	r = gnome_vfs_unlink (epath);
	g_free (epath);

	return (r == GNOME_VFS_OK);
}


static gboolean 
image_is_type__common (const char *name,
		       const char *type,
		       gboolean    fast_file_type)
{
	const char *result = NULL;

	if (fast_file_type) {
		char *n1 = g_filename_to_utf8 (name, -1, 0, 0, 0);
		if (n1 != NULL) {
			char *n2 = g_utf8_strdown (n1, -1);
			char *n3 = g_filename_from_utf8 (n2, -1, 0, 0, 0);
			if (n3 != NULL)
				result = gnome_vfs_mime_type_from_name_or_default (file_name_from_path (n3), NULL);
			g_free (n3);
			g_free (n2);
			g_free (n1);
		}
	} else 
		result = gnome_vfs_get_file_mime_type (name, NULL, FALSE);
	
	/* Unknown file type. */
	if (result == NULL)
		return FALSE;

	return (strcmp (result, type) == 0);
}


static gboolean 
image_is_type (const char *name,
	       const char *type)
{
	return image_is_type__common (name, type, eel_gconf_get_boolean (PREF_FAST_FILE_TYPE, TRUE));
}


gboolean 
image_is_jpeg (const char *name)
{
	return image_is_type (name, "image/jpeg");
}


gboolean 
image_is_gif (const char *name)
{
	return image_is_type (name, "image/gif");
}


gboolean 
image_is_gif__accurate (const char *name)
{
	return image_is_type__common (name, "image/gif", FALSE);
}


gboolean
path_is_file (const char *path)
{
	GnomeVFSFileInfo *info;
	GnomeVFSResult    result;
	gboolean          is_file;
	char             *escaped;

	if (! path || ! *path) 
		return FALSE; 

	info = gnome_vfs_file_info_new ();
	escaped = escape_uri (path);

	result = gnome_vfs_get_file_info (escaped, 
					  info, 
					  (GNOME_VFS_FILE_INFO_DEFAULT 
					   | GNOME_VFS_FILE_INFO_FOLLOW_LINKS));
	is_file = FALSE;
	if (result == GNOME_VFS_OK)		
		is_file = (info->type == GNOME_VFS_FILE_TYPE_REGULAR);
	
	g_free (escaped);
	gnome_vfs_file_info_unref (info);

	return is_file;
}


gboolean
path_is_dir (const char *path)
{
	GnomeVFSFileInfo *info;
	GnomeVFSResult    result;
	gboolean          is_dir;
	char             *escaped;

	if (! path || ! *path) 
		return FALSE; 

	info = gnome_vfs_file_info_new ();
	escaped = escape_uri (path);

	result = gnome_vfs_get_file_info (escaped, 
					  info, 
					  (GNOME_VFS_FILE_INFO_DEFAULT 
					   | GNOME_VFS_FILE_INFO_FOLLOW_LINKS));
	is_dir = FALSE;
	if (result == GNOME_VFS_OK)
		is_dir = (info->type == GNOME_VFS_FILE_TYPE_DIRECTORY);

	g_free (escaped);
	gnome_vfs_file_info_unref (info);

	return is_dir;
}


GnomeVFSFileSize 
get_file_size (const char *path)
{
	GnomeVFSFileInfo *info;
	GnomeVFSResult    result;
	GnomeVFSFileSize  size;
	char             *escaped;

	if (! path || ! *path) return 0; 

	info = gnome_vfs_file_info_new ();
	escaped = escape_uri (path);
	result = gnome_vfs_get_file_info (escaped, 
					  info,
					  (GNOME_VFS_FILE_INFO_DEFAULT 
					   | GNOME_VFS_FILE_INFO_FOLLOW_LINKS)); 
	size = 0;
	if (result == GNOME_VFS_OK)
		size = info->size;

	g_free (escaped);
	gnome_vfs_file_info_unref (info);

	return size;
}


time_t 
get_file_mtime (const char *path)
{
	GnomeVFSFileInfo *info;
	GnomeVFSResult    result;
	char             *escaped;
	time_t            mtime;

	if (! path || ! *path) return 0; 

	info = gnome_vfs_file_info_new ();
	escaped = escape_uri (path);
	result = gnome_vfs_get_file_info (escaped, 
					  info, 
					  (GNOME_VFS_FILE_INFO_DEFAULT 
					   | GNOME_VFS_FILE_INFO_FOLLOW_LINKS));
	mtime = 0;
	if ((result == GNOME_VFS_OK)
	    && (info->valid_fields & GNOME_VFS_FILE_INFO_FIELDS_MTIME))
		mtime = info->mtime;

	g_free (escaped);
	gnome_vfs_file_info_unref (info);

	return mtime;
}


time_t 
get_file_ctime (const gchar *path)
{
	GnomeVFSFileInfo *info;
	GnomeVFSResult result;
	gchar *escaped;
	time_t ctime;

	if (! path || ! *path) return 0; 

	info = gnome_vfs_file_info_new ();
	escaped = escape_uri (path);
	result = gnome_vfs_get_file_info (escaped, 
					  info, 
					  (GNOME_VFS_FILE_INFO_DEFAULT 
					   | GNOME_VFS_FILE_INFO_FOLLOW_LINKS));
	ctime = 0;
	if ((result == GNOME_VFS_OK)
	    && (info->valid_fields & GNOME_VFS_FILE_INFO_FIELDS_CTIME))
		ctime = info->ctime;

	g_free (escaped);
	gnome_vfs_file_info_unref (info);

	return ctime;
}


void
set_file_mtime (const gchar *path,
		time_t       mtime)
{
	GnomeVFSFileInfo *file_info;
	char             *escaped_path;

	file_info = gnome_vfs_file_info_new ();
	file_info->mtime = mtime;
	file_info->atime = mtime;

	escaped_path = escape_uri (path);
	gnome_vfs_set_file_info (escaped_path,
				 file_info,
				 GNOME_VFS_SET_FILE_INFO_TIME);
	gnome_vfs_file_info_unref (file_info);
	g_free (escaped_path);
}


long 
checksum_simple (const gchar *path)
{
	FILE *f;
	long sum = 0;
	gint c;

	f = fopen (path, "r");
	if (!f) return -1;

	while ((c = fgetc (f)) != EOF)
		sum += c;
	fclose (f);

	return sum;
}


const char *
get_home_uri (void)
{
	static char *home_uri = NULL;
	if (home_uri == NULL)
		home_uri = g_strconcat ("file://", g_get_home_dir (), NULL);
	return home_uri;
}


const char *
get_file_path_from_uri (const char *uri)
{
	if (uri_scheme_is_file (uri))
		return uri + FILE_PREFIX_L;
	else if (uri[0] == '/')
		return uri;
	else
		return NULL;
}


const char *
get_catalog_path_from_uri (const char *uri)
{
	if (g_utf8_strlen (uri, -1) < CATALOG_PREFIX_L)
		return NULL;
	return uri + CATALOG_PREFIX_L;
}


const char *
get_search_path_from_uri (const char *uri)
{
	if (g_utf8_strlen (uri, -1) < SEARCH_PREFIX_L)
		return NULL;
	return uri + SEARCH_PREFIX_L;
}


const char *
remove_scheme_from_uri (const char *uri)
{
	const char *idx;

	idx = strstr (uri, "://");
	if (idx == NULL)
		return uri;
	return idx + 3;
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


gboolean
uri_has_scheme (const char *uri)
{
	return strstr (uri, "://") != NULL;
}


gboolean
uri_scheme_is_file (const char *uri)
{
	if (uri == NULL)
		return FALSE;
	/*
	return ! uri_scheme_is_catalog (uri) && ! uri_scheme_is_search (uri);
	*/
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
get_uri_from_path (const char *path)
{
	if (path == NULL)
		return NULL;
	if ((path == "") || (path[0] == '/'))
		return g_strconcat ("file://", path, NULL);
	return g_strdup (path);
}


char *
get_uri_display_name (const char *uri)
{
	return NULL;
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


/* example 1 : filename = /xxx/yyy/zzz/foo 
 *             destdir  = /xxx/www 
 *             return   : ../yyy/zzz/foo
 *
 * example 2 : filename = /xxx/yyy/foo
 *             destdir  = /xxx
 *             return   : yyy/foo
 */
char *
get_path_relative_to_dir (const char *filename, 
			  const char *destdir)
{
	char     *sourcedir;
	char    **sourcedir_v;
	char    **destdir_v;
	int       i, j;
	char     *result;
	GString  *relpath;

	sourcedir = remove_level_from_path (filename);
	sourcedir_v = g_strsplit (sourcedir, "/", 0);
	destdir_v = g_strsplit (destdir, "/", 0);

	relpath = g_string_new (NULL);

	i = 0;
	while ((sourcedir_v[i] != NULL)
	       && (destdir_v[i] != NULL)
	       && (strcmp (sourcedir_v[i], destdir_v[i]) == 0))
		i++;
	
	j = i;

	while (destdir_v[i++] != NULL) 
		g_string_append (relpath, "../");

	while (sourcedir_v[j] != NULL) {
		g_string_append (relpath, sourcedir_v[j]);
		g_string_append_c (relpath, '/');
		j++;
	}

	g_string_append (relpath, file_name_from_path (filename));

	g_strfreev (sourcedir_v);
	g_strfreev (destdir_v);
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
	char       *new_path;

	if (! path) 
		return NULL;

	p = strlen (path) - 1;
	if (p < 0) 
		return NULL;

	while ((p > 0) && (ptr[p] != '/')) 
		p--;
	if ((p == 0) && (ptr[p] == '/')) 
		p++;
	new_path = g_strndup (path, (guint)p);

	return new_path;
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
char *
remove_special_dirs_from_path (const char *uri)
{
	const char  *path;
	char       **pathv;
	GList       *list = NULL, *scan;
	int          i;
	GString     *result_s;
	char        *scheme;
	char        *result;

	path = remove_scheme_from_uri (uri);

	if ((path == NULL) 
	    || (*path != '/')
	    || (strstr (path, ".") == NULL))
		return g_strdup (path);

	pathv = g_strsplit (path, "/", 0);

	/* start from 1 to remove the first / that will be re-added later. */
	for (i = 1; pathv[i] != NULL; i++) {
		if (strcmp (pathv[i], ".") == 0) {
			/* nothing to do. */
		} else if (strcmp (pathv[i], "..") == 0) {
			if (list == NULL) {
				/* path error. */
				g_strfreev (pathv);
				return NULL;
			}
			list = g_list_delete_link (list, list);
		} else 
			list = g_list_prepend (list, pathv[i]);
	}

	result_s = g_string_new (NULL);
	scheme = get_uri_scheme (uri);
	if (scheme != NULL) {
		g_string_append (result_s, scheme);
		g_free (scheme);
	}
	if (list == NULL)
		g_string_append_c (result_s, '/');
	else {
		list = g_list_reverse (list);
		for (scan = list; scan; scan = scan->next) {
			g_string_append_c (result_s, '/');
			g_string_append (result_s, scan->data);
		}
	}
	result = result_s->str;
	g_string_free (result_s, FALSE);
	g_strfreev (pathv);

	return result;
}


GnomeVFSURI *
new_uri_from_path (const char *path)
{
	char        *escaped;
	char        *uri_txt;
	GnomeVFSURI *uri;

	escaped = escape_uri (path);
	if (escaped[0] == '/')
		uri_txt = g_strconcat ("file://", escaped, NULL);
	else
		uri_txt = g_strdup (escaped);

	uri = gnome_vfs_uri_new (uri_txt);

	g_free (uri_txt);
	g_free (escaped);

	g_return_val_if_fail (uri != NULL, NULL);

	return uri;
}


char *
new_path_from_uri (GnomeVFSURI *uri)
{
	char *path;
	char *unescaped_path;

	if (uri == NULL)
		return NULL;

	path = gnome_vfs_uri_to_string (uri, GNOME_VFS_URI_HIDE_TOPLEVEL_METHOD);
	unescaped_path = gnome_vfs_unescape_string (path, NULL);
	g_free (path);

	return unescaped_path;
}


GnomeVFSResult
resolve_all_symlinks (const char  *text_uri,
		      char       **resolved_text_uri)
{
	GnomeVFSResult    res = GNOME_VFS_OK;
	char             *my_text_uri;
	GnomeVFSFileInfo *info;
	const char       *p;
	int               n_followed_symlinks = 0;
	gboolean          first_level = TRUE;

	*resolved_text_uri = NULL;

	if (text_uri == NULL) 
		return res;

	my_text_uri = g_strdup (text_uri);
	info = gnome_vfs_file_info_new ();

	for (p = my_text_uri; (p != NULL) && (*p != 0); ) {
		char           *new_text_uri;
		GnomeVFSURI    *new_uri;

		while (*p == GNOME_VFS_URI_PATH_CHR)
			p++;
		while (*p != 0 && *p != GNOME_VFS_URI_PATH_CHR)
			p++;

		new_text_uri = g_strndup (my_text_uri, p - my_text_uri);
		new_uri = new_uri_from_path (new_text_uri);
		g_free (new_text_uri);

		gnome_vfs_file_info_clear (info);
		res = gnome_vfs_get_file_info_uri (new_uri, info, GNOME_VFS_FILE_INFO_DEFAULT);

		if (res != GNOME_VFS_OK) {
			char *old_uri = my_text_uri;
			my_text_uri = g_build_filename (old_uri, p, NULL);
			g_free (old_uri);
			p = NULL;

		} else if (info->type == GNOME_VFS_FILE_TYPE_SYMBOLIC_LINK &&
			   info->valid_fields & GNOME_VFS_FILE_INFO_FIELDS_SYMLINK_NAME) {
			GnomeVFSURI *resolved_uri;
			char        *tmp_resolved_text_uri = NULL;
			char        *symbolic_link = NULL;

			n_followed_symlinks++;
			if (n_followed_symlinks > MAX_SYMLINKS_FOLLOWED) {
				res = GNOME_VFS_ERROR_TOO_MANY_LINKS;
				gnome_vfs_uri_unref (new_uri);
				goto out;
			}

			if (first_level && (info->symlink_name[0] != '/')) 
				symbolic_link = g_strconcat ("/", info->symlink_name, NULL);
			else 
				symbolic_link = g_strdup (info->symlink_name);
			resolved_uri = gnome_vfs_uri_resolve_relative (new_uri, symbolic_link);
			g_free (symbolic_link);

			tmp_resolved_text_uri = new_path_from_uri (resolved_uri);
			gnome_vfs_uri_unref (resolved_uri);

			if (*p != 0) {
				char *old_uri = my_text_uri;
				my_text_uri = g_build_filename (tmp_resolved_text_uri, p, NULL);
				g_free (old_uri);
				g_free (tmp_resolved_text_uri);
			} else {
				g_free (my_text_uri);
				my_text_uri = tmp_resolved_text_uri;
			}

			p = my_text_uri;
		}
		first_level = FALSE;

		gnome_vfs_uri_unref (new_uri); 
	}

	res = GNOME_VFS_OK;
	*resolved_text_uri = my_text_uri;
 out:
	gnome_vfs_file_info_unref (info);
	return res;
}


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
			    ';' };

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


gboolean
file_extension_is (const char *filename, 
		   const char *ext)
{
	return ! strcasecmp (filename + strlen (filename) - strlen (ext), ext);
}


const char *
get_filename_extension (const char *filename)
{
	char *last_dot;

	last_dot = strrchr (filename, '.');
	if (last_dot == NULL)
		return NULL;

	return last_dot + 1;
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
	while ((ptr[p] != '.') && (p > 0)) 
		p--;
	if (p == 0) 
		p = len;
	new_path = g_strndup (path, (guint) p);

	return new_path;
}


char *
get_temp_dir_name (void)
{
	static int  count = 0;
	char       *tmp_dir = NULL;
	
	do {
		g_free (tmp_dir);
		tmp_dir = g_strdup_printf ("%s%s.%d.%d",
					   g_get_tmp_dir (),
					   "/gthumb",
					   getpid (),
					   count++);

	} while (path_is_dir (tmp_dir));

	if (mkdir (tmp_dir, 0700) != 0) {
		g_free (tmp_dir);
		return NULL;
	}

	return tmp_dir;
}


char *
get_temp_file_name (const char *ext)
{
	char *dir, *name, *filename;

	dir = get_temp_dir_name ();
	name = g_strconcat ("temp", ext, NULL);
	filename = g_build_filename (dir, name, NULL);
	g_free (name);
	g_free (dir);
	
	return filename;
}


GnomeVFSResult
_gnome_vfs_read_line (GnomeVFSHandle   *handle,
		      gpointer          buffer,
		      GnomeVFSFileSize  bytes,
		      GnomeVFSFileSize *bytes_read)
{
	GnomeVFSResult    result = GNOME_VFS_OK;
	GnomeVFSFileSize  offset = 0;
	GnomeVFSFileSize  file_offset;
	GnomeVFSFileSize  priv_bytes_read = 0;
	char             *eol = NULL;

	result = gnome_vfs_tell (handle, &file_offset);

	((char*)buffer)[0] = '\0';

	while ((result == GNOME_VFS_OK) && (eol == NULL)) {
		if (bytes - offset <= 0)
			return GNOME_VFS_ERROR_INTERNAL;
		result = gnome_vfs_read (handle, buffer + offset, bytes - offset, &priv_bytes_read);
		if (result != GNOME_VFS_OK)
			break;
		eol = strchr ((char*)buffer + offset, '\n');
		if (eol != NULL) {
			GnomeVFSFileSize line_size = eol - (char*)buffer;
			*eol = '\0';
			gnome_vfs_seek (handle, GNOME_VFS_SEEK_START, file_offset + line_size + 1);
			if (bytes_read != NULL)
				*bytes_read = line_size;
		} 
		else 
			offset += priv_bytes_read;
	}

	return result;
}


GnomeVFSFileSize
get_dest_free_space (const char  *path)
{
	GnomeVFSURI      *uri;
	GnomeVFSResult    result;
	GnomeVFSFileSize  ret_val;

	uri = new_uri_from_path (path);
	result = gnome_vfs_get_volume_free_space (uri, &ret_val);
	gnome_vfs_uri_unref (uri);

	if (result != GNOME_VFS_OK)
		return (GnomeVFSFileSize) 0;
	else
		return ret_val;
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


gboolean
check_permissions (const char *path,
		   int         mode) 
{
	GnomeVFSFileInfo *info;
	GnomeVFSResult    vfs_result;
	char             *escaped;

	info = gnome_vfs_file_info_new ();
	escaped = escape_uri (path);
	vfs_result = gnome_vfs_get_file_info (escaped, 
					      info, 
					      (GNOME_VFS_FILE_INFO_DEFAULT 
					       | GNOME_VFS_FILE_INFO_FOLLOW_LINKS
					       | GNOME_VFS_FILE_INFO_GET_ACCESS_RIGHTS));
	g_free (escaped);

	if (vfs_result != GNOME_VFS_OK) 
		return FALSE; 

	if ((mode & R_OK) && ! (info->permissions & GNOME_VFS_PERM_ACCESS_READABLE)) 
		return FALSE; 

	if ((mode & W_OK) && ! (info->permissions & GNOME_VFS_PERM_ACCESS_WRITABLE)) 
		return FALSE; 

	if ((mode & X_OK) && ! (info->permissions & GNOME_VFS_PERM_ACCESS_WRITABLE)) 
		return FALSE; 

	return TRUE;
}
