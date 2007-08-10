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
#include <gdk-pixbuf/gdk-pixbuf-animation.h>
#include <libgnomeui/gnome-thumbnail.h>
#include <libgnomevfs/gnome-vfs.h>
#include <libgnomevfs/gnome-vfs-handle.h>
#include <libgnomevfs/gnome-vfs-mime.h>
#include <libgnomevfs/gnome-vfs-result.h>
#include <gconf/gconf-client.h>

#include "gthumb-init.h"
#include "gthumb-error.h"
#include "glib-utils.h"
#include "gconf-utils.h"
#include "file-utils.h"
#include "file-data.h"
#include "pixbuf-utils.h"
#include "typedefs.h"

#ifdef HAVE_LIBOPENRAW
#include <libopenraw-gnome/gdkpixbuf.h>
#endif

#define BUF_SIZE 4096
#define CHUNK_SIZE 128
#define MAX_SYMLINKS_FOLLOWED 32


/* Async directory list */


PathListData *
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
	pli->interrupt_func = NULL;
	pli->interrupt_data = NULL;
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
		char             *txt_uri;
		FileData         *file;
		
		switch (info->type) {
		case GNOME_VFS_FILE_TYPE_REGULAR:
			if (g_hash_table_lookup (pli->hidden_files, info->name) != NULL)
				break;
			full_uri = gnome_vfs_uri_append_file_name (pli->uri, info->name);
			txt_uri = gnome_vfs_uri_to_string (full_uri, GNOME_VFS_URI_HIDE_NONE);
			
			file = file_data_new (txt_uri, info);
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
					    128 /* items_per_notification FIXME: find a good value */,
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
path_list_new (const char  *uri,
	       GList      **files,
	       GList      **dirs)
{
	GnomeVFSResult  r;
	GnomeVFSURI    *dir_uri;
	GList          *info_list = NULL;
	GList          *scan;
	GList          *f_list = NULL;
	GList          *d_list = NULL;

	if (files) *files = NULL;
	if (dirs) *dirs = NULL;

	r = gnome_vfs_directory_list_load (&info_list,
					   uri,
					   GNOME_VFS_FILE_INFO_FOLLOW_LINKS);

	if (r != GNOME_VFS_OK)
		return FALSE;

	dir_uri = new_uri_from_path (uri);
	for (scan = info_list; scan; scan = scan->next) {
		GnomeVFSFileInfo *info = scan->data;
		GnomeVFSURI      *full_uri = NULL;
		char             *s_uri;

		full_uri = gnome_vfs_uri_append_file_name (dir_uri, info->name);
		s_uri = gnome_vfs_uri_to_string (full_uri, GNOME_VFS_URI_HIDE_NONE);

		if (info->type == GNOME_VFS_FILE_TYPE_DIRECTORY) {
			if (! SPECIAL_DIR (info->name))
				d_list = g_list_prepend (d_list, s_uri);
		} 
		else if (info->type == GNOME_VFS_FILE_TYPE_REGULAR)
			f_list = g_list_prepend (f_list, file_data_new (s_uri, info));
		else
			g_free (s_uri);
	}
	gnome_vfs_file_info_list_free (info_list);

	if (dirs)
		*dirs = g_list_reverse (d_list);
	else
		path_list_free (d_list);

	if (files)
		*files = g_list_reverse (f_list);
	else
		file_data_list_free (f_list);

	return TRUE;
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
		if (same_uri ((char*) scan->data, path))
			return scan;
	return NULL;
}


/* Directory utils */


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
	return (gnome_vfs_make_directory (path, mode) == GNOME_VFS_OK);
}


gboolean
dir_remove (const gchar *path)
{
	return (gnome_vfs_remove_directory (path) == GNOME_VFS_OK);
}


gboolean
dir_remove_recursive (const char *uri)
{
	GList    *files, *dirs;
	GList    *scan;
	gboolean  error = FALSE;

	if (! path_is_dir (uri))
		return FALSE;

	path_list_new (uri, &files, &dirs);

	for (scan = files; scan; scan = scan->next) {
		FileData *file = scan->data;
		if (! file_unlink (file->path)) {
			g_warning ("Cannot delete %s\n", file->path);
			error = TRUE;
		}
	}
	file_data_list_free (files);

	for (scan = dirs; scan; scan = scan->next) {
		char *sub_dir = scan->data;
		if (! dir_remove_recursive (sub_dir))
			error = TRUE;
	}
	path_list_free (dirs);

	if (! dir_remove (uri))
		error = TRUE;

	return ! error;
}


gboolean
local_dir_remove_recursive (const char *path)
{
	char     *uri;
	gboolean  result = FALSE;
	
	uri = get_uri_from_local_path (path);
	result = dir_remove_recursive (uri);
	g_free (uri);
	
	return result;
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


/* return TRUE to add the file to the file list. */
gboolean
file_filter (FileData *file,
	     gboolean  show_hidden_files)
{
	if (file->mime_type == NULL)
		return FALSE;

	if ((! show_hidden_files && file_is_hidden (file->name))
	    || ! (mime_type_is_image (file->mime_type) 
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
	while ((p >= 0) && (ptr[p] != '.'))
		p--;

	if (p < 0)
		return NULL;

	return path + p;
}


/* File utils */


const char *
get_mime_type (const char *uri)
{
	if (uri_scheme_is_file (uri))
		uri = get_file_path_from_uri (uri);
	return gnome_vfs_get_file_mime_type (uri, NULL, FALSE);
}


const char *
get_mime_type_from_ext (const char *ext)
{
	char       *filename;
	const char *result;

	filename = g_strconcat ("x.", ext, NULL);
	result = get_mime_type (filename);
	g_free (filename);

	return result;
}


gboolean mime_type_is_image (const char *mime_type)
{
	/* Valid image mime types:
	   	1. All image* types, 
		1b.	except for image/x-xcf, image/x-compressed-xcf
			because we can't read these gimp files, and fast,
			reliable converters are not really available.
		2. application/x-crw
			This is a RAW photo file, which for some reason
			uses an "application" prefix instead of "image".
	*/

	return (   ((strstr (mime_type, "image") != NULL) && (strstr (mime_type, "xcf") == NULL))
		|| (strcmp (mime_type, "application/x-crw") == 0) );
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
	return ( (strstr (mime_type, "video") != NULL) ||
		 (strcmp (mime_type, "application/ogg") == 0));
}


gboolean file_is_video (const gchar *name,
			gboolean     fast_file_type)
{
	const char *mime_type = NULL;

	mime_type = get_file_mime_type (name, fast_file_type);
	if (mime_type == NULL)
		return FALSE;

	return mime_type_is_video (mime_type);
}


gboolean mime_type_is_audio (const char *mime_type)
{
	return (strstr (mime_type, "audio") != NULL);
}


gboolean file_is_audio (const gchar *name,
			gboolean     fast_file_type)
{
	const char *mime_type = NULL;

	mime_type = get_file_mime_type (name, fast_file_type);
	if (mime_type == NULL)
		return FALSE;

	return mime_type_is_audio (mime_type);
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
	   gboolean    move)
{
	GnomeVFSURI         *from_uri, *to_uri;
	GnomeVFSXferOptions  opt;
	GnomeVFSResult       result;

	if (same_uri (from, to)) {
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
file_move (const char *from,
	   const char *to)
{
	return xfer_file (from, to, TRUE);
}


gboolean
local_file_move (const char *from,
	         const char *to)
{
	char     *from_uri;
	char     *to_uri;
	gboolean  result;
	
	from_uri = get_uri_from_local_path (from);
	to_uri = get_uri_from_local_path (to);
	
	result = xfer_file (from_uri, to_uri, TRUE);
	
	g_free (from_uri);
	g_free (to_uri);
	
	return result;
}


GnomeVFSResult
file_rename (const char *old_path,
	     const char *new_path)
{
	return gnome_vfs_move (old_path, new_path, TRUE);
}


gboolean
file_unlink (const char *path)
{
	return (gnome_vfs_unlink (path) == GNOME_VFS_OK);
}


void
delete_thumbnail (const char *path)
{
	char *uri;
	char *large_thumbnail;
	char *normal_thumbnail;

	uri = add_scheme_if_absent (path);

	/* delete associated thumbnails, if present */
	large_thumbnail = gnome_thumbnail_path_for_uri (uri, GNOME_THUMBNAIL_SIZE_LARGE);
	normal_thumbnail = gnome_thumbnail_path_for_uri (uri, GNOME_THUMBNAIL_SIZE_NORMAL);

	file_unlink (large_thumbnail);
	file_unlink (normal_thumbnail);

	g_free (large_thumbnail);
	g_free (normal_thumbnail);
	g_free (uri);
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


const char*
get_file_mime_type (const char *filename,
		    gboolean    fast_file_type)
{
	const char *result = NULL;
	const char *extension;

	if (filename == NULL)
		return NULL;

	if (fast_file_type) {
		char *sample_name;
		char *n1;

		sample_name = get_sample_name (filename);
		if (sample_name != NULL) {
			n1 = g_filename_to_utf8 (sample_name, -1, 0, 0, 0);
			if (n1 != NULL) {
				char *n2 = g_utf8_strdown (n1, -1);
				char *n3 = g_filename_from_utf8 (n2, -1, 0, 0, 0);
				if (n3 != NULL)
					result = gnome_vfs_mime_type_from_name_or_default (file_name_from_path (n3), NULL);
				g_free (n3);
				g_free (n2);
				g_free (n1);
			}

			g_free (sample_name);
		}
	} 
	else {
		if (uri_scheme_is_file (filename))
			filename = get_file_path_from_uri (filename);
		result = gnome_vfs_get_file_mime_type (filename, NULL, FALSE);
	}

	result = get_static_string (result);

	/* Check files with special or problematic extensions */
	extension = get_filename_extension (filename);
	if (extension != NULL) {

		/* Raw NEF files are sometimes mis-recognized as tiff files. Fix that. */
		if (!strcmp_null_tolerant (result, "image/tiff") && 
		    !strcasecmp (extension, "nef"))
			return "image/x-nikon-nef";

		/* Raw CR2 files are sometimes mis-recognized as tiff files. Fix that. */
		if (!strcmp_null_tolerant (result, "image/tiff") && 
		    !strcasecmp (extension, "cr2"))
			return "image/x-canon-cr2";

		/* Check unrecognized binary types for special types that are not
		   handled correctly in the normal mime databases. */

		if ((result == NULL) || 
		    (strcmp_null_tolerant (result, "application/octet-stream") == 0)) {

			/* If the file extension is not recognized, or the content is
			   determined to be binary data (octet-stream), check for HDR file
			   types, which are not well represented in the freedesktop mime
			   database currently. This section can be purged when they are.
			   This is an unpleasant hack. Some file extensions
			   may be missing here; please file a bug if they are. */
			if (   !strcasecmp (extension, "exr")	/* OpenEXR format */
			    || !strcasecmp (extension, "hdr")	/* Radiance rgbe */
			    || !strcasecmp (extension, "pic"))	/* Radiance rgbe */
				return "image/x-hdr";

			/* Bug 329072: gnome-vfs doesn't recognize pcx files.
			   This is the work-around until bug 329072 is fixed. */
			if (strcasecmp (extension, "pcx") == 0)
				return "image/x-pcx";
		}
	}

	return result;
}


gboolean
mime_type_is (const char *mime_type,
	      const char *value)
{
	return (strcmp_null_tolerant (mime_type, value) == 0);
}


gboolean
image_is_type (const char *uri,
	       const char *type,
	       gboolean    fast_file_type)
{
	const char *result = get_file_mime_type (uri, fast_file_type);
	return (strcmp_null_tolerant (result, type) == 0);
}


static gboolean
image_is_type__gconf_file_type (const char *uri,
			        const char *type)
{
	return image_is_type (uri, type, ! is_local_file (uri) || eel_gconf_get_boolean (PREF_FAST_FILE_TYPE, TRUE));
}


gboolean
image_is_jpeg (const char *name)
{
	return image_is_type__gconf_file_type (name, "image/jpeg");
}


gboolean
mime_type_is_raw (const char *mime_type)
{
	return 	   mime_type_is (mime_type, "application/x-crw")	/* ? */
		|| mime_type_is (mime_type, "image/x-dcraw")		/* dcraw */
		|| mime_type_is (mime_type, "image/x-minolta-mrw")  	/* freedesktop.org.xml */
		|| mime_type_is (mime_type, "image/x-canon-crw")    	/* freedesktop.org.xml */
		|| mime_type_is (mime_type, "image/x-canon-cr2")    	/* freedesktop.org.xml */
		|| mime_type_is (mime_type, "image/x-nikon-nef")	/* freedesktop.org.xml */
		|| mime_type_is (mime_type, "image/x-kodak-dcr")    	/* freedesktop.org.xml */
		|| mime_type_is (mime_type, "image/x-kodak-kdc")    	/* freedesktop.org.xml */
		|| mime_type_is (mime_type, "image/x-olympus-orf")	/* freedesktop.org.xml */
		|| mime_type_is (mime_type, "image/x-fuji-raf")    	/* freedesktop.org.xml */
		|| mime_type_is (mime_type, "image/x-raw")		/* mimelnk */
		;
}


gboolean
mime_type_is_hdr (const char *mime_type)
{
	/* Note that some HDR file extensions have been hard-coded into
	   the get_file_mime_type function above. */
	return mime_type_is (mime_type, "image/x-hdr");
}


gboolean
mime_type_is_tiff (const char *mime_type)
{
	return mime_type_is (mime_type, "image/tiff");
}


gboolean
image_is_gif (const char *name)
{
	return image_is_type__gconf_file_type (name, "image/gif");
}


gboolean
path_exists (const char *path)
{
	GnomeVFSFileInfo *info;
	GnomeVFSResult    result;
	gboolean          exists;

	if (! path || ! *path)
		return FALSE;

	info = gnome_vfs_file_info_new ();
	result = gnome_vfs_get_file_info (path,
					  info,
					  (GNOME_VFS_FILE_INFO_DEFAULT
					   | GNOME_VFS_FILE_INFO_FOLLOW_LINKS));
	exists = (result == GNOME_VFS_OK);
	gnome_vfs_file_info_unref (info);

	return exists;
}


gboolean
path_is_file (const char *path)
{
	GnomeVFSFileInfo *info;
	GnomeVFSResult    result;
	gboolean          is_file;

	if (! path || ! *path)
		return FALSE;

	info = gnome_vfs_file_info_new ();
	result = gnome_vfs_get_file_info (path,
					  info,
					  (GNOME_VFS_FILE_INFO_DEFAULT
					   | GNOME_VFS_FILE_INFO_FOLLOW_LINKS));
	is_file = FALSE;
	if (result == GNOME_VFS_OK)
		is_file = (info->type == GNOME_VFS_FILE_TYPE_REGULAR);

	gnome_vfs_file_info_unref (info);

	return is_file;
}


gboolean
path_is_dir (const char *path)
{
	GnomeVFSFileInfo *info;
	GnomeVFSResult    result;
	gboolean          is_dir;

	if (! path || ! *path)
		return FALSE;

	info = gnome_vfs_file_info_new ();
	result = gnome_vfs_get_file_info (path,
					  info,
					  (GNOME_VFS_FILE_INFO_DEFAULT
					   | GNOME_VFS_FILE_INFO_FOLLOW_LINKS));
	is_dir = FALSE;
	if (result == GNOME_VFS_OK)
		is_dir = (info->type == GNOME_VFS_FILE_TYPE_DIRECTORY);

	gnome_vfs_file_info_unref (info);

	return is_dir;
}


GnomeVFSFileSize
get_file_size (const char *uri)
{
	GnomeVFSFileInfo *info;
	GnomeVFSResult    result;
	GnomeVFSFileSize  size;

	if (! uri || ! *uri)
		return 0;

	info = gnome_vfs_file_info_new ();
	result = gnome_vfs_get_file_info (uri,
					  info,
					  (GNOME_VFS_FILE_INFO_DEFAULT
					   | GNOME_VFS_FILE_INFO_FOLLOW_LINKS));
	size = 0;
	if (result == GNOME_VFS_OK)
		size = info->size;

	gnome_vfs_file_info_unref (info);

	return size;
}


time_t
get_file_mtime (const char *path)
{
	GnomeVFSFileInfo *info;
	GnomeVFSResult    result;
	time_t            mtime;

	if (! path || ! *path) return 0;

	info = gnome_vfs_file_info_new ();
	result = gnome_vfs_get_file_info (path,
					  info,
					  (GNOME_VFS_FILE_INFO_DEFAULT
					   | GNOME_VFS_FILE_INFO_FOLLOW_LINKS));
	mtime = 0;
	if ((result == GNOME_VFS_OK)
	    && (info->valid_fields & GNOME_VFS_FILE_INFO_FIELDS_MTIME))
		mtime = info->mtime;

	gnome_vfs_file_info_unref (info);

	return mtime;
}


time_t
get_file_ctime (const gchar *path)
{
	GnomeVFSFileInfo *info;
	GnomeVFSResult result;
	time_t ctime;

	if (! path || ! *path) return 0;

	info = gnome_vfs_file_info_new ();
	result = gnome_vfs_get_file_info (path,
					  info,
					  (GNOME_VFS_FILE_INFO_DEFAULT
					   | GNOME_VFS_FILE_INFO_FOLLOW_LINKS));
	ctime = 0;
	if ((result == GNOME_VFS_OK)
	    && (info->valid_fields & GNOME_VFS_FILE_INFO_FIELDS_CTIME))
		ctime = info->ctime;

	gnome_vfs_file_info_unref (info);

	return ctime;
}


void
set_file_mtime (const gchar *path,
		time_t       mtime)
{
	GnomeVFSFileInfo *file_info;

	file_info = gnome_vfs_file_info_new ();
	file_info->mtime = mtime;
	file_info->atime = mtime;
	gnome_vfs_set_file_info (path,
				 file_info,
				 GNOME_VFS_SET_FILE_INFO_TIME);
	gnome_vfs_file_info_unref (file_info);
}


long
checksum_simple (const char *path)
{
	GnomeVFSHandle   *handle;
	GnomeVFSResult    result;
	char              buffer[1024];
	GnomeVFSFileSize  bytes_read;
	long              sum = 0;

	result = gnome_vfs_open (&handle, path, GNOME_VFS_OPEN_READ);
	if (result != GNOME_VFS_OK)
		return -1;

	while (gnome_vfs_read (handle,
			       buffer,
			       1024,
			       &bytes_read) == GNOME_VFS_OK) {
		int i;
		for (i = 0; i < bytes_read; i++)
			sum += buffer[i];
	}

	gnome_vfs_close (handle);

	return sum;
}


/* URI/Path utils */


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
	if (uri == NULL)
		return NULL;
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


const char *
remove_host_from_uri (const char *uri)
{
	const char *idx;

	idx = strstr (uri, "://");
	if (idx == NULL)
		return uri;
	idx = strstr (idx + 3, "/");
	if (idx == NULL)
		return NULL;
	return idx;
}


char *
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
	if ((path == "") || (path[0] == '/'))
		return g_strconcat ("file://", path, NULL);
	return g_strdup (path);
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

			name = gnome_vfs_unescape_string_for_display (tmp_path + 1 + base_uri_len);
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
					name = gnome_vfs_unescape_string_for_display (uri + 1 + base_path_len);
			} 
			else
				name = gnome_vfs_unescape_string_for_display (tmp_path);
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


char *
get_local_path_from_uri (const char *uri)
{
       	return gnome_vfs_unescape_string (remove_host_from_uri (uri), NULL);
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
	return uricmp (uri1, uri2) == 0;
}


char *
basename_for_display (const char *uri)
{
	return gnome_vfs_unescape_string_for_display (file_name_from_path (uri));
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
get_path_relative_to_dir (const char *uri,
			  const char *destdir)
{
	char     *sourcedir;
	char    **sourcedir_v;
	char    **destdir_v;
	int       i, j;
	char     *result;
	GString  *relpath;

	sourcedir = remove_level_from_path (remove_host_from_uri (uri));
	sourcedir_v = g_strsplit (sourcedir, "/", 0);
	destdir_v = g_strsplit (remove_host_from_uri (destdir), "/", 0);

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

	g_string_append (relpath, file_name_from_path (uri));

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
char *
remove_special_dirs_from_path (const char *uri)
{
	const char  *path;
	char       **pathv;
	GList       *list = NULL;
	int          i;
	GString     *result_s;
	char        *scheme;
	char        *result;
	int	     start_at;

	scheme = get_uri_host (uri);

	if ((scheme == NULL) && ! g_path_is_absolute (uri))
		return g_strdup (uri);

	path = remove_host_from_uri (uri);

	if ((path == NULL) || (strstr (path, ".") == NULL))
		return g_strdup (uri);

	pathv = g_strsplit (path, "/", 0);

	/* Trimmed uris might not start with a slash */
	if (*path != '/')
		start_at = 0;
	else
		start_at = 1;

	/* Ignore first slash, if present. It will be re-added later. */
	for (i = start_at; pathv[i] != NULL; i++) {
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

	/* re-insert URI scheme */
	if (scheme != NULL) {
		g_string_append (result_s, scheme);

		if (start_at == 0)
			/* delete trailing slash, because an extra one is added below */
			g_string_truncate (result_s, result_s->len - 1);

		g_free (scheme);
	}

	if (list == NULL)
		g_string_append_c (result_s, '/');
	else {
		GList *scan;

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


char *
new_path_from_uri (GnomeVFSURI *uri)
{
	if (uri == NULL)
		return NULL;
	return gnome_vfs_uri_to_string (uri, GNOME_VFS_URI_HIDE_NONE);
}


static char *
build_uri (const char *s1,
	   const char *s2)
{
	char *s = NULL;

	if ((s1 == NULL) && (s2 == NULL))
		return NULL;

	if ((s1 == NULL) || (strcmp (s1, "") == 0))
		return g_strdup (s2);
	if ((s2 == NULL) || (strcmp (s2, "") == 0))
		return g_strdup (s1);

	if ((s1[strlen (s1) - 1] == GNOME_VFS_URI_PATH_CHR) || (s2[0] == GNOME_VFS_URI_PATH_CHR))
		s = g_strconcat (s1, s2, NULL);
	else
		s = g_strconcat (s1, GNOME_VFS_URI_PATH_STR, s2, NULL);

	return s;
}


GnomeVFSResult
resolve_symlinks (const char  *text_uri,
		  const char  *relative_link,
		  char       **resolved_text_uri,
		  int          n_followed_symlinks)
{
	GnomeVFSResult     result = GNOME_VFS_OK;
	char              *resolved_uri;
	char 		  *uri;
	char              *tmp;
	GnomeVFSFileInfo  *info;
	const char        *path;
	char             **names;
	int                i;

	*resolved_text_uri = NULL;

	if (text_uri == NULL)
		return GNOME_VFS_OK;
	if (*text_uri == '\0')
		return GNOME_VFS_ERROR_INVALID_URI;

	info = gnome_vfs_file_info_new ();

	resolved_uri = get_uri_host (text_uri);

	tmp = build_uri (text_uri, relative_link);
	uri = remove_special_dirs_from_path (tmp);
	g_free (tmp);

	/* split the uri and resolve one name at a time. */

	path = remove_host_from_uri (uri);
	if (path == NULL) {
		*resolved_text_uri = resolved_uri;
		return GNOME_VFS_OK;
	}

	names = g_strsplit (path, GNOME_VFS_URI_PATH_STR, -1);
	g_free (uri);

	for (i = 0; (result == GNOME_VFS_OK) && (names[i] != NULL); i++) {
		char  *try_uri;
		char  *symlink;
	    	char **symlink_names;
	    	int    j;
	    	char  *base_uri;

		if (strcmp (names[i], "") == 0)
			continue;

		gnome_vfs_file_info_clear (info);

		try_uri = g_strconcat (resolved_uri, GNOME_VFS_URI_PATH_STR, names[i], NULL);

		result = gnome_vfs_get_file_info (try_uri, info, GNOME_VFS_FILE_INFO_DEFAULT);
		if (result != GNOME_VFS_OK) {
			g_free (try_uri);
			break;
		}

		/* if names[i] isn't a symbolic link add it to the resolved uri and continue */

		if (!((info->type == GNOME_VFS_FILE_TYPE_SYMBOLIC_LINK) &&
		      (info->valid_fields & GNOME_VFS_FILE_INFO_FIELDS_SYMLINK_NAME))) {

			g_free (resolved_uri);
			resolved_uri = try_uri;

			continue;
		}

		g_free (try_uri);

		/* names[i] is a symbolic link */

		n_followed_symlinks++;
		if (n_followed_symlinks > MAX_SYMLINKS_FOLLOWED) {
			result = GNOME_VFS_ERROR_TOO_MANY_LINKS;
			break;
		}

		/* get the symlink escaping the value info->symlink_name */

		symlink = g_strdup ("");
		symlink_names = g_strsplit (info->symlink_name, GNOME_VFS_URI_PATH_STR, -1);
		for (j = 0; symlink_names[j] != NULL; j++) {
			char *symlink_name = symlink_names[j];
	    		char *e_symlink_name;

		    	if ((strcmp (symlink_name, "..") == 0) || (strcmp (symlink_name, ".") == 0))
		    		e_symlink_name = g_strdup (symlink_name);
		    	if (strcmp (symlink_name, "") == 0)
		    		e_symlink_name = g_strdup (GNOME_VFS_URI_PATH_STR);
		    	else
		    		e_symlink_name = gnome_vfs_escape_string (symlink_name);

		    	if (strcmp (symlink, "") == 0) {
		    		g_free (symlink);
		    		symlink = e_symlink_name;
		    	}
		    	else {
		    		char *tmp;

		   		tmp = build_uri (symlink, e_symlink_name);

	    			g_free (symlink);
	    			g_free (e_symlink_name);

	    			symlink = tmp;
	    		}
	    	}
	    	g_strfreev (symlink_names);

		/* if the symlink is absolute reset the base uri, else use
		 * the currently resolved uri as base. */

	    	if (symlink[0] == GNOME_VFS_URI_PATH_CHR) {
	    		g_free (resolved_uri);
	    		base_uri = get_uri_host (text_uri);
	    	} else
	    		base_uri = resolved_uri;

		/* resolve the new uri recursively */

	    	result = resolve_symlinks (base_uri, symlink, &resolved_uri, n_followed_symlinks);

	    	g_free (base_uri);
		g_free (symlink);
	}

	g_strfreev (names);
	gnome_vfs_file_info_unref (info);

	if (result == GNOME_VFS_OK)
		*resolved_text_uri = resolved_uri;

	return result;
}


GnomeVFSResult
resolve_all_symlinks (const char  *text_uri,
		      char       **resolved_text_uri)
{
	return resolve_symlinks (text_uri, "", resolved_text_uri, 0);
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
delete_catalog_dir (const char  *full_path,
		    gboolean     recursive,
		    GError     **gerror)
{
	if (dir_remove (full_path))
		return TRUE;

	if (gerror != NULL) {
		const char *rel_path;
		char       *base_path;
		char       *utf8_path;
		const char *details;

		base_path = get_catalog_full_path (NULL);
		rel_path = full_path + strlen (base_path) + 1;
		g_free (base_path);

		utf8_path = gnome_vfs_unescape_string_for_display (rel_path);

		switch (gnome_vfs_result_from_errno ()) {
		case GNOME_VFS_ERROR_DIRECTORY_NOT_EMPTY:
			details = _("Library not empty");
			break;
		default:
			details = errno_to_string ();
			break;
		}

		*gerror = g_error_new (GTHUMB_ERROR,
				       errno,
				       _("Cannot remove library \"%s\": %s"),
				       utf8_path,
				       details);
		g_free (utf8_path);
	}

	return FALSE;
}


gboolean
delete_catalog (const char  *full_path,
		GError     **gerror)
{
	if (! file_unlink (full_path)) {
		if (gerror != NULL) {
			const char *rel_path;
			char       *base_path;
			char       *catalog;

			base_path = get_catalog_full_path (NULL);
			rel_path = full_path + strlen (base_path) + 1;
			g_free (base_path);
			catalog = remove_extension_from_path (rel_path);

			*gerror = g_error_new (GTHUMB_ERROR,
					       errno,
					       _("Cannot remove catalog \"%s\": %s"),
					       catalog,
					       errno_to_string ());
			g_free (catalog);
		}
		return FALSE;
	}

	return TRUE;
}


gboolean
file_is_search_result (const char *path)
{
	GnomeVFSResult  r;
	GnomeVFSHandle *handle;
	char            line[50] = "";

	r = gnome_vfs_open (&handle, path, GNOME_VFS_OPEN_READ);
	if (r != GNOME_VFS_OK)
		return FALSE;

	r = gnome_vfs_read (handle, line, strlen (SEARCH_HEADER), NULL);
	gnome_vfs_close (handle);

	if ((r != GNOME_VFS_OK) || (line[0] == 0))
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


/* extesion */


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
	while ((p > 0) && (ptr[p] != '.'))
		p--;
	if (p == 0)
		p = len;
	new_path = g_strndup (path, (guint) p);

	return new_path;
}


/* temp */


static const char *try_folder[] = { "~", "tmp", NULL };


static const char *
get_folder_from_try_folder_list (int n)
{
        const char *folder;

        folder = try_folder[n];
        if (strcmp (folder, "~") == 0)
                folder = g_get_home_dir ();
        else if (strcmp (folder, "tmp") == 0)
                folder = g_get_tmp_dir ();

        return folder;
}


char *
get_temp_dir_name (void)
{
        GnomeVFSFileSize  max_size = 0;
        char             *best_folder = NULL;
        int               i;
        char             *template;
        char             *result = NULL;

        /* find the folder with the bigger free space. */

        for (i = 0; try_folder[i] != NULL; i++) {
                const char       *folder;
                char             *uri;
                GnomeVFSFileSize  size;

                folder = get_folder_from_try_folder_list (i);
                uri = get_uri_from_local_path (folder);

                size = get_destination_free_space (uri);
                if (size > max_size) {
                        max_size = size;
                        g_free (best_folder);
                        best_folder = get_local_path_from_uri (uri);
                }
                else
                        g_free (uri);
        }

	if (best_folder == NULL)
		return NULL;

        template = g_strconcat (best_folder, "/.gth-XXXXXX", NULL);
        g_free (best_folder);
        
        result = mkdtemp (template);

        if ((result == NULL) || (*result == '\0')) {
                g_free (template);
                result = NULL;
        }

        return result;
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
		name = g_strdup_printf ("%d%s", count++, ext);
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
        GnomeVFSResult       result;
        GnomeVFSAsyncHandle *handle;
        CopyDoneFunc         done_func;
        gpointer             done_data;
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
	
	if (copy_data->done_func != NULL) {
		copy_data->handle = NULL;	
		(copy_data->done_func) (copy_data->target_uri, copy_data->result, copy_data->done_data);
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
		copy_file_async_done (copy_data);
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
	CopyData       *copy_data;
	GList          *source_uri_list, *target_uri_list;
	GnomeVFSResult  result;

	copy_data = copy_data_new (source_uri, target_uri, done_func, done_data);

	if (! path_is_file (source_uri)) {
		copy_data->result = GNOME_VFS_ERROR_NOT_FOUND;
		g_idle_add (copy_file_async_done, copy_data);
		return NULL;
	}

	source_uri_list = g_list_append (NULL, gnome_vfs_uri_new (source_uri));
	target_uri_list = g_list_append (NULL, gnome_vfs_uri_new (target_uri));

	result = gnome_vfs_async_xfer (&copy_data->handle,
				       source_uri_list,
				       target_uri_list,
				       GNOME_VFS_XFER_DEFAULT | GNOME_VFS_XFER_FOLLOW_LINKS,
				       GNOME_VFS_XFER_ERROR_MODE_ABORT,
				       GNOME_VFS_XFER_OVERWRITE_MODE_ABORT,
				       GNOME_VFS_PRIORITY_DEFAULT,
				       copy_file_async_progress_update_cb, copy_data,
				       NULL, NULL);

	gnome_vfs_uri_list_free (source_uri_list);
	gnome_vfs_uri_list_free (target_uri_list);

	if (result != GNOME_VFS_OK) {
		copy_data->result = result;
		g_idle_add (copy_file_async_done, copy_data);
	}
	
	return copy_data;
}


/* -- */

GnomeVFSResult
_gnome_vfs_read_line (GnomeVFSHandle   *handle,
		      gpointer          buffer,
		      GnomeVFSFileSize  buffer_size,
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
		if (offset + CHUNK_SIZE >= buffer_size)
			return GNOME_VFS_ERROR_INTERNAL;

		result = gnome_vfs_read (handle, buffer + offset, CHUNK_SIZE, &priv_bytes_read);
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


GnomeVFSResult
_gnome_vfs_write_line (GnomeVFSHandle   *handle,
		       const char       *format,
		       ...)
{
	GnomeVFSResult  result;
	va_list         args;
	char           *str;

	g_return_val_if_fail (format != NULL, GNOME_VFS_ERROR_INTERNAL);

	va_start (args, format);
	str = g_strdup_vprintf (format, args);
	va_end (args);

	result = gnome_vfs_write (handle, str, strlen(str), NULL);

	g_free (str);

	if (result != GNOME_VFS_OK)
		return result;

	return gnome_vfs_write (handle, "\n", 1, NULL);
}


GnomeVFSFileSize
get_destination_free_space (const char *uri)
{
        GnomeVFSURI      *vfs_uri;
        GnomeVFSResult    result;
        GnomeVFSFileSize  free_space;

        vfs_uri = gnome_vfs_uri_new (uri);
        result = gnome_vfs_get_volume_free_space (vfs_uri, &free_space);
        gnome_vfs_uri_unref (vfs_uri);

        if (result != GNOME_VFS_OK)
                return (GnomeVFSFileSize) 0;
        else
                return free_space;
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
	gboolean	  everything_OK = TRUE;

	info = gnome_vfs_file_info_new ();
	vfs_result = gnome_vfs_get_file_info (path,
					      info,
					      (GNOME_VFS_FILE_INFO_DEFAULT
					       | GNOME_VFS_FILE_INFO_FOLLOW_LINKS
					       | GNOME_VFS_FILE_INFO_GET_ACCESS_RIGHTS));

	if (vfs_result != GNOME_VFS_OK)
		everything_OK = FALSE;

	if ((mode & R_OK) && ! (info->permissions & GNOME_VFS_PERM_ACCESS_READABLE))
		everything_OK = FALSE;

	if ((mode & W_OK) && ! (info->permissions & GNOME_VFS_PERM_ACCESS_WRITABLE))
		everything_OK = FALSE;

	if ((mode & X_OK) && ! (info->permissions & GNOME_VFS_PERM_ACCESS_EXECUTABLE))
		everything_OK = FALSE;

	gnome_vfs_file_info_unref (info);

	return everything_OK;
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
	return g_strconcat (g_get_home_dir (),
			    "/",
			    RC_REMOTE_CACHE_DIR,
			    ((filename == NULL) ? "" : "/"),
			    filename,
			    extension,
			    NULL);
}


char *
get_cache_filename_from_uri (const char *uri)
{
	char *name;
	char *path;
	
	if (is_local_file (uri))
		return get_local_path_from_uri (uri);
	
	name = gnome_thumbnail_md5 (uri);
	if (name == NULL)
		return NULL;

	path = get_cache_full_path (name, NULL);
	g_free (name);
	
	return path;	
}


char *
get_cache_uri_from_uri (const char *uri)
{
	char *filename;
	char *cache_uri;
	
	filename = get_cache_filename_from_uri (uri);
	cache_uri = get_uri_from_local_path (filename);
	g_free (filename);
	
	return cache_uri;
}


#define MAX_CACHE_SIZE (256 * 1024 * 1024)
static GnomeVFSFileSize  cache_used_space = 0;
static GList            *cache_files = NULL;
static gboolean          cache_loaded = FALSE;


void
free_cache (void)
{
	char  *cache_dir;
	char  *cache_uri;
	GList *files = NULL;
	
	cache_dir = get_cache_full_path (NULL, NULL);
	cache_uri = get_uri_from_local_path (cache_dir);
	g_free (cache_dir);
	
	if (path_list_new (cache_uri, &files, NULL)) {
		GList *scan;
		for (scan = files; scan; scan = scan->next ) {
			FileData *file = scan->data;
			file_unlink (file->path);
		}
	}

	file_data_list_free (files);
	g_free (cache_uri);
	
	file_data_list_free (cache_files);
	cache_files = NULL;
	cache_used_space = 0;
}


static gint
comp_func_time (gconstpointer a, 
		gconstpointer b)
{
	FileData *data_a, *data_b;

	data_a = (FileData*) a;
	data_b = (FileData*) b;

	return data_a->mtime > data_b->mtime;
}


void
check_cache_free_space (void)
{
	char  *cache_dir;
	char  *cache_uri;
	GList *scan;

	cache_dir = get_cache_full_path (NULL, NULL);
	cache_uri = get_uri_from_local_path (cache_dir);
	g_free (cache_dir);
	
	if (! cache_loaded) {
		if (! path_list_new (cache_uri, &cache_files, NULL)) {
			file_data_list_free (cache_files);
			cache_files = NULL;
			cache_loaded = FALSE;
			g_free (cache_uri);
			return;
		}
		cache_files = g_list_sort (cache_files, comp_func_time);
		
		cache_used_space = 0;
		for (scan = cache_files; scan; scan = scan->next) {
			FileData *file = scan->data;
			cache_used_space += file->size; 
		}		
		
		cache_loaded = TRUE;
	}
	
	debug (DEBUG_INFO, "cache size: %"GNOME_VFS_SIZE_FORMAT_STR".\n", cache_used_space);
	
	if (cache_used_space > MAX_CACHE_SIZE) {
		int n = 0;
		
		/* the first file is the last copied, so reverse the list to
		 * delete the older files first. */
		 
		cache_files = g_list_reverse (cache_files);
		for (scan = cache_files; scan && (cache_used_space > MAX_CACHE_SIZE / 2); ) {
			FileData *file = scan->data;
		
			file_unlink (file->path);
			cache_used_space -= file->size;
			
			cache_files = g_list_remove_link (cache_files, scan);
			file_data_list_free (scan);
			scan = cache_files;
			
			n++;
		}
		cache_files = g_list_reverse (cache_files);
		
		debug (DEBUG_INFO, "deleted %d files, new cache size: %"GNOME_VFS_SIZE_FORMAT_STR".\n", n, cache_used_space);
	}
	
	g_free (cache_uri);
}


typedef struct {
	CopyDoneFunc done_func;
	gpointer     done_data;
} CopyToCacheData;


static void
copy_remote_file_to_cache_done (const char     *uri, 
				GnomeVFSResult  result, 
				gpointer        callback_data)
{
	CopyToCacheData *data = callback_data;

	if (result == GNOME_VFS_OK) {
		FileData *cache_file;
		
		cache_file = file_data_new (uri, NULL);
		file_data_update (cache_file);
		cache_used_space += cache_file->size;
		cache_files = g_list_prepend (cache_files, cache_file);
	}
	
	if (data->done_func != NULL)
		data->done_func (uri, result, data->done_data);
	
	g_free (data);
}


CopyData *
copy_remote_file_to_cache (FileData     *file,
			   CopyDoneFunc  done_func,
			   gpointer      done_data)
{
	CopyData *copy_data = NULL;
	char     *cache_uri;
	
	cache_uri = get_cache_uri_from_uri (file->path);
	if (is_local_file (file->path) || (file->mtime <= get_file_mtime (cache_uri))) {
		copy_data = copy_data_new (file->path, cache_uri, done_func, done_data);
		g_idle_add (copy_file_async_done, copy_data);
	}
	else {
		CopyToCacheData *data;
		
		check_cache_free_space ();
		
		data = g_new0 (CopyToCacheData, 1);
		data->done_func = done_func;
		data->done_data = done_data;
						
		copy_file_async (file->path, 
				 cache_uri, 
				 copy_remote_file_to_cache_done, 
				 data);
	}
	g_free (cache_uri);
	
	return copy_data;
}


CopyData *
update_file_from_cache (FileData     *file,
			CopyDoneFunc  done_func,
			gpointer      done_data)
{
	CopyData *copy_data = NULL;	
	char     *cache_uri;
	
	cache_uri = get_cache_uri_from_uri (file->path);
	if (is_local_file (file->path) || (file->mtime >= get_file_mtime (cache_uri))) {
		copy_data = copy_data_new (file->path, cache_uri, done_func, done_data);
		g_idle_add (copy_file_async_done, copy_data);
	}
	else
		copy_file_async (cache_uri, file->path, done_func, done_data);
	g_free (cache_uri);
	
	return copy_data;
}


char*
obtain_local_file (const char *remote_filename)
{
	char *md5_file;
	char *cache_file;
	char *local_file;
	char *command;

	/* If the file is local, simply return a copy of the filename, without
	   any "file:///" prefix. */
 
	if (is_local_file (remote_filename))
		return get_local_path_from_uri (remote_filename);

	/* If the file is remote, copy it to a local cache. */
	md5_file = gnome_thumbnail_md5 (remote_filename);
	if (md5_file == NULL)
		return NULL;

	cache_file = get_cache_full_path (md5_file, get_extension (remote_filename));
	g_free (md5_file);
	if (cache_file == NULL)
		return NULL;

	if (! path_exists (cache_file) || (get_file_mtime (cache_file) < get_file_mtime (remote_filename))) {
		GnomeVFSURI    *source_uri = gnome_vfs_uri_new (remote_filename);
		GnomeVFSURI    *target_uri = gnome_vfs_uri_new (cache_file);
		GnomeVFSResult  result;
	
		/* delete some files if space is running out */
		/*check_cache_space ();*/

		/* Move a new file into the cache. */
		result = gnome_vfs_xfer_uri (source_uri, target_uri,
					     GNOME_VFS_XFER_DEFAULT | GNOME_VFS_XFER_FOLLOW_LINKS,
					     GNOME_VFS_XFER_ERROR_MODE_ABORT,
					     GNOME_VFS_XFER_OVERWRITE_MODE_REPLACE,
					     NULL,
					     NULL);

		gnome_vfs_uri_unref (source_uri);
		gnome_vfs_uri_unref (target_uri);

		if (result != GNOME_VFS_OK) {
			g_free (cache_file);
       			return NULL;
		}

		debug (DEBUG_INFO, "Copied %s into cache.\n", remote_filename);
	}

	local_file = get_local_path_from_uri (cache_file);
	g_free (cache_file);

	/* update mtimes so cache pruning works properly (delete oldest first) */
	command = g_strdup_printf ("touch %s", local_file);
	system (command);
	g_free (command);

	return local_file;
}


gboolean
copy_cache_file_to_remote_uri (const char *local_filename,
			       const char *dest_uri)
{
	/* make a remote copy of a local cache file */

	GnomeVFSURI    *source_uri;
	GnomeVFSURI    *target_uri;
	GnomeVFSResult  result;

	source_uri = gnome_vfs_uri_new (local_filename);
	target_uri = gnome_vfs_uri_new (dest_uri);

	result = gnome_vfs_xfer_uri (source_uri, target_uri,
				     GNOME_VFS_XFER_DEFAULT | GNOME_VFS_XFER_FOLLOW_LINKS,
				     GNOME_VFS_XFER_ERROR_MODE_ABORT,
				     GNOME_VFS_XFER_OVERWRITE_MODE_REPLACE,
				     NULL,
				     NULL);

	gnome_vfs_uri_unref (target_uri);
	gnome_vfs_uri_unref (source_uri);

	return (result == GNOME_VFS_OK);
}


/* Pixbuf + VFS */


static GdkPixbuf*
get_pixbuf_using_external_converter (FileData   *file,
				     int         requested_width,
				     int         requested_height)
{
	char       *local_file;
	char       *local_uri;
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

	local_file = get_local_path_from_uri (file->path);
	if (local_file == NULL)
		return NULL;

	is_thumbnail = requested_width > 0;
	local_uri = get_uri_from_local_path (local_file);

	is_raw = mime_type_is_raw (file->mime_type);
	is_hdr = mime_type_is_hdr (file->mime_type);

	/* The output filename, and its persistence, depend on the input file
	   type, and whether or not a thumbnail has been requested. */

	md5_file = gnome_thumbnail_md5 (local_uri);
	
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
		g_free (local_file);
		g_free (local_uri);
		g_free (cache_file);
		g_free (cache_file_esc);
		return NULL;
	}

	local_file_esc = g_shell_quote (local_file);

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
                		if (gnome_vfs_is_executable_command_string (thumb_command))
			        	g_spawn_command_line_sync (thumb_command, NULL, NULL, NULL, NULL);
				g_free (thumb_command);

				first_part = remove_extension_from_path (local_file);
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
			if (gnome_vfs_is_executable_command_string (command))
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
	g_free (local_file);
	g_free (local_uri);

	return pixbuf;
}


static GdkPixbuf*
gth_pixbuf_new_from_video (FileData               *file,
			   GnomeThumbnailFactory  *factory,
			   GError                **error)
{
      	GdkPixbuf *pixbuf = NULL;
	char      *existing_video_thumbnail;

	existing_video_thumbnail = gnome_thumbnail_factory_lookup (factory,
								   file->path,
								   file->mtime);
	if (existing_video_thumbnail != NULL) {
		char *thumbnail_path = get_local_path_from_uri (existing_video_thumbnail);

		pixbuf = gdk_pixbuf_new_from_file (thumbnail_path, error);
		g_free (thumbnail_path);
		g_free (existing_video_thumbnail);
	}
	else if (gnome_thumbnail_factory_has_valid_failed_thumbnail (factory, file->path, file->mtime)) 
		return NULL;
	else 
		pixbuf = gnome_thumbnail_factory_generate_thumbnail (factory,
								     file->path, 
								     file->mime_type);

	return pixbuf;
}


GdkPixbuf*
gth_pixbuf_new_from_file (FileData               *file,
			  GError                **error,
			  int                     requested_width,
			  int                     requested_height,
			  GnomeThumbnailFactory  *factory)
{
	GdkPixbuf *pixbuf = NULL;
	char      *local_file = NULL;

	if (file == NULL)
		return NULL;

	local_file = get_cache_filename_from_uri (file->path);
	if (local_file == NULL)
		return NULL;

	if (mime_type_is_video (file->mime_type)) {
		if (factory != NULL) 
			return gth_pixbuf_new_from_video (file, factory, error);
		return NULL;
	}

#ifdef HAVE_LIBOPENRAW
	/* Raw thumbnails - using libopenraw is much faster than using dcraw for
	   thumbnails. Use libopenraw for full raw images too, once it matures. */
	if ((pixbuf == NULL) 
	    && mime_type_is_raw (file->mime_type) 
	    && (requested_width > 0))
		pixbuf = or_gdkpixbuf_extract_thumbnail (local_file, requested_width);
#endif

	/* Use dcraw for raw images, pfstools for HDR images, and tifftopnm for tiff thumbnails */
	if ((pixbuf == NULL) &&
	     (mime_type_is_raw (file->mime_type) ||
	      mime_type_is_hdr (file->mime_type) )) 
		pixbuf = get_pixbuf_using_external_converter (file,
							      requested_width,
							      requested_height);

	/* Otherwise, use standard gdk_pixbuf loaders */
	if ((pixbuf == NULL) && (requested_width > 0)) {
		int w, h;
		
		if (gdk_pixbuf_get_file_info (local_file, &w, &h) == NULL) {
			w = -1;
			h = -1;
		}
		
		/* scale the image only if the original size is larger than
		 * the requested size. */
		if ((w > requested_width) || (h > requested_height))
			pixbuf = gdk_pixbuf_new_from_file_at_scale (local_file,
                        	                                    requested_width,
                                	                            requested_height,
                                        	                    TRUE,
                                                	            error);
		else
			pixbuf = gdk_pixbuf_new_from_file (local_file, error);
	}
	else if (pixbuf == NULL)
		/* otherwise, no scaling required */
		pixbuf = gdk_pixbuf_new_from_file (local_file, error);

	g_free (local_file);
	
	return pixbuf;
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

	if (mime_type_is (file->mime_type, "image/gif")) {
		char *local_file;
		
		local_file = get_cache_filename_from_uri (file->path);
		animation = gdk_pixbuf_animation_new_from_file (local_file, error);
		g_free (local_file);

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
	GHashTable     *hidden_files;
	char           *dot_hidden_uri;
	GnomeVFSHandle *handle;
	GnomeVFSResult  result;
	char            line [BUF_SIZE];

	hidden_files = g_hash_table_new_full (g_str_hash,
					      g_str_equal,
					      (GDestroyNotify) g_free,
					      NULL);

	if (eel_gconf_get_boolean (PREF_SHOW_HIDDEN_FILES, FALSE))
		return hidden_files;

	dot_hidden_uri = g_build_filename (uri, ".hidden", NULL);

	result = gnome_vfs_open (&handle, dot_hidden_uri, GNOME_VFS_OPEN_READ);
	if (result != GNOME_VFS_OK) {
		g_free (dot_hidden_uri);
		return hidden_files;
	}

	while (_gnome_vfs_read_line (handle,
				     line,
				     BUF_SIZE,
				     NULL) == GNOME_VFS_OK) {
		char *path;

		line[strlen (line)] = 0;
		path = gnome_vfs_escape_string (line);

		if (g_hash_table_lookup (hidden_files, path) == NULL)
			g_hash_table_insert (hidden_files, path, GINT_TO_POINTER (1));
		else
			g_free (path);
	}

	gnome_vfs_close (handle);

	g_free (dot_hidden_uri);

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
		ensure_dir_exists (user_dir, 0775);
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
