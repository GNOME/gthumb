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
#include "gtk-utils.h"

#define PROCESS_DELAY 25
#define PROCESS_MAX_FILES 33


char *
cache_get_nautilus_cache_name (const char *path) 
{
	char           *parent;
	char           *escaped_path;
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

	escaped_path = gnome_vfs_uri_to_string (uri, GNOME_VFS_URI_HIDE_NONE);
	gnome_vfs_uri_unref (uri);

	uri_txt = gnome_vfs_unescape_string (escaped_path, NULL);
	g_free (escaped_path);

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
			unlink (cache_dest);
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
			unlink (cache_dest);
		if (file_move (cache_src, cache_dest))
			set_file_mtime (cache_dest, dest_mtime);
		
		g_free (cache_dest);
	}
	g_free (cache_src);
}


void
cache_delete (const char *filename)
{
	char *cache_name;

	cache_name = cache_get_nautilus_cache_name (filename);
	unlink (cache_name);
	g_free (cache_name);
}


/* ----- nautilus_cache_remove_old_previews_async implememtation. ------ */


typedef struct {
	gboolean            recursive;
	gboolean            clear_all;
	GList              *dirs;
	GList              *visited_dirs;
	char               *nautilus_thumb_dir;
	int                 nautilus_thumb_dir_l;
	GtkWidget          *dialog;
	gboolean            interrupted;
	guint               process_timeout;
	PathListData       *pld;
	GList              *scan;
	PathListHandle     *handle;
} NautilusCacheRemoveData;


void
nautilus_cache_data_free (NautilusCacheRemoveData *ncrd)
{
	if (ncrd == NULL)
		return;

	if (ncrd->dirs != NULL) {
		g_list_foreach (ncrd->dirs, (GFunc) g_free, NULL);
		g_list_free (ncrd->dirs);
	}

	if (ncrd->visited_dirs != NULL) {
		g_list_foreach (ncrd->visited_dirs, (GFunc) g_free, NULL);
		g_list_free (ncrd->visited_dirs);
	}

	if (ncrd->nautilus_thumb_dir)
		g_free (ncrd->nautilus_thumb_dir);

	if (ncrd->pld != NULL)
		path_list_data_free (ncrd->pld);

	if (ncrd->handle != NULL)
		g_free (ncrd->handle);

	gtk_widget_destroy (ncrd->dialog);

	g_free (ncrd);
}


static void visit_dir_async (const char              *dir,
			     NautilusCacheRemoveData *ncrd);


/**/

static gboolean
png_text_to_pixbuf_option (png_text   text_ptr,
                           char     **key,
                           char     **value)
{
        if (text_ptr.text_length > 0) {
                *value = g_convert (text_ptr.text, -1,
				    "UTF-8", "ISO-8859-1",
				    NULL, NULL, NULL);
        } else {
                *value = g_strdup (text_ptr.text);
        }

        if (*value) {
                *key = g_strconcat ("tEXt::", text_ptr.key, NULL);
                return TRUE;
        } else {
                g_warning ("Couldn't convert text chunk value to UTF-8.");
                *key = NULL;
                return FALSE;
        }
}


static void
png_simple_error_callback(png_structp     png_save_ptr,
                          png_const_charp error_msg)
{
	GError **error;
	
	error = png_get_error_ptr(png_save_ptr);
	
	/* I don't trust libpng to call the error callback only once,
	 * so check for already-set error
	 */
	if (error && *error == NULL) {
		g_set_error (error,
			     GDK_PIXBUF_ERROR,
			     GDK_PIXBUF_ERROR_FAILED,
			     "Fatal error in PNG image file: %s",
			     error_msg);
	}

	longjmp (png_save_ptr->jmpbuf, 1);
}


static void
png_simple_warning_callback(png_structp     png_save_ptr,
                            png_const_charp warning_msg)
{
	/* Don't print anything; we should not be dumping junk to
	 * stderr, since that may be bad for some apps. If it's
	 * important enough to display, we need to add a GError
	 * **warning return location wherever we have an error return
	 * location.
	 */
}


static char *
get_real_name_from_nautilus_cache (const char *cache_path)
{
	FILE          *f;
	char          *result;
	png_structp    png_ptr = NULL;
	png_infop      info_ptr = NULL;
	png_textp      text_ptr = NULL;
	int            num_texts;
	
	f = fopen (cache_path, "r");
	
	if (f == NULL)
		return NULL;
	
	png_ptr = png_create_read_struct (PNG_LIBPNG_VER_STRING,
					  NULL,
					  png_simple_error_callback,
					  png_simple_warning_callback);
	
        if (png_ptr == NULL) {
		fclose (f);
		return NULL;
	}
	
	info_ptr = png_create_info_struct (png_ptr);
	if (info_ptr == NULL) {
		png_destroy_read_struct (&png_ptr, NULL, NULL);
		fclose (f);
		return NULL;
	}

	if (setjmp (png_ptr->jmpbuf)) {
		png_destroy_read_struct (&png_ptr, NULL, NULL);
		fclose (f);
		return NULL;
	}
	
	result = NULL;
	png_init_io (png_ptr, f);
	png_read_info (png_ptr, info_ptr);
	
	if (png_get_text (png_ptr, info_ptr, &text_ptr, &num_texts)) {
		int    i;
		char  *key;
		char  *value;

		for (i = 0; i < num_texts; i++) {
			png_text_to_pixbuf_option (text_ptr[i], &key, &value);
			if ((key != NULL)
			    && (strcmp (key, "tEXt::Thumb::URI") == 0)
			    && (value != NULL)) {
				int ofs = 0;
				if (strncmp (value, "file://", 7) == 0)
					ofs = 7;
				result = g_strdup (value + ofs);
			}
                        g_free (key);
			g_free (value);
		}
	}
	
	png_destroy_read_struct (&png_ptr, &info_ptr, NULL);
	
	fclose (f);
	
	return result;
}


/**/


static void
process__final_step (NautilusCacheRemoveData *ncrd)
{
	char  *sub_dir;

	if (! ncrd->recursive || ncrd->interrupted) {
		nautilus_cache_data_free (ncrd);
		return;
	}

	ncrd->dirs = g_list_concat (ncrd->pld->dirs, ncrd->dirs);
	ncrd->pld->dirs = NULL;
	path_list_data_free (ncrd->pld);
	ncrd->pld = NULL;

	if (ncrd->dirs == NULL) {
		if (ncrd->clear_all) {
			const GList *scan = ncrd->visited_dirs;

			for (; scan; scan = scan->next) {
				char *dir = scan->data;
				rmdir (dir);
			}
		}
		nautilus_cache_data_free (ncrd);
		return;
	}

	sub_dir = (char*) ncrd->dirs->data;
	ncrd->dirs = g_list_remove_link (ncrd->dirs, ncrd->dirs);

	ncrd->visited_dirs = g_list_prepend (ncrd->visited_dirs,
					     g_strdup (sub_dir));
	visit_dir_async (sub_dir, ncrd);

	g_free (sub_dir);
}


static gboolean
process_files_cb (gpointer data)
{
	NautilusCacheRemoveData *ncrd = data;
	GList *scan = ncrd->scan;
	int    i = 0;

	if (ncrd->process_timeout != 0) {
		g_source_remove (ncrd->process_timeout);
		ncrd->process_timeout = 0;
	}

	if ((ncrd->scan == NULL) || ncrd->interrupted) {
		process__final_step (ncrd);
		return FALSE;
	}

	g_free (ncrd->handle);
	ncrd->handle = NULL;

	for (; scan && i < PROCESS_MAX_FILES; scan = scan->next, i++) {
		char *rc_file;
		char *real_file;

		rc_file = (char*) scan->data;
		real_file = get_real_name_from_nautilus_cache (rc_file);

		if (real_file == NULL)
			continue;

		if (ncrd->clear_all || ! path_is_file (real_file)) {
			if ((unlink (rc_file) < 0)) 
				g_warning ("Cannot delete %s\n", rc_file);
		}

		g_free (real_file);
	}

	ncrd->scan = scan;
	ncrd->process_timeout = g_timeout_add (PROCESS_DELAY, 
					       process_files_cb, 
					       ncrd);

	return FALSE;
}


static void
path_list_done_cb (PathListData *pld, 
		   gpointer      data)
{
	NautilusCacheRemoveData *ncrd = data;

	g_free (ncrd->handle);
	ncrd->handle = NULL;

	ncrd->pld = pld;

	if (pld->result != GNOME_VFS_ERROR_EOF || ncrd->interrupted) {
		char *path;

		path = gnome_vfs_uri_to_string (pld->uri,
						GNOME_VFS_URI_HIDE_NONE);
		g_warning ("Error reading cache directory %s.", path);
		g_free (path);

		nautilus_cache_data_free (ncrd);
		return;
	}

	ncrd->scan = pld->files;
	ncrd->process_timeout = g_timeout_add (PROCESS_DELAY, 
					       process_files_cb, 
					       ncrd);
}


static void
visit_dir_async (const gchar *dir,
		 NautilusCacheRemoveData *data)
{
	data->handle = path_list_async_new (dir, path_list_done_cb, data);
}


static void
ncrop_interrupt_cb (GtkDialog *dialog,
		    int response_id,
		    NautilusCacheRemoveData *ncrd)
{
	ncrd->interrupted = TRUE;
	if (ncrd->handle != NULL) 
		path_list_async_interrupt (ncrd->handle,
					   (DoneFunc) process__final_step,
					   ncrd);
}


static void 
nautilus_cache_remove_old_previews_async (gboolean recursive,
					  gboolean clear_all)
{
	NautilusCacheRemoveData *ncrd;
	const char              *message;

	if (clear_all)
		message = _("Deleting all thumbnails, wait please...");
	else
		message = _("Deleting old thumbnails, wait please...");

	ncrd = g_new0 (NautilusCacheRemoveData, 1);

	ncrd->recursive = recursive;
	ncrd->clear_all = clear_all;
	ncrd->dirs = NULL;
	ncrd->visited_dirs = NULL;
	ncrd->interrupted = FALSE;
	ncrd->process_timeout = 0;
	ncrd->handle = NULL;

	ncrd->nautilus_thumb_dir = g_strconcat (g_get_home_dir (),
						"/.thumbnails",
						NULL);
	ncrd->nautilus_thumb_dir_l = strlen (ncrd->nautilus_thumb_dir);

	ncrd->dialog =  _gtk_message_dialog_new (NULL,
						 GTK_DIALOG_MODAL,
						 GTK_MESSAGE_INFO,
						 message,
						 NULL,
						 GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE,
						 NULL);
	g_signal_connect (G_OBJECT (ncrd->dialog),
			  "response",
			  G_CALLBACK (ncrop_interrupt_cb),
			  ncrd);

	gtk_widget_show (ncrd->dialog);

	visit_dir_async (ncrd->nautilus_thumb_dir, ncrd);
}


void
cache_remove_old_previews_async (const char *dir,
				 gboolean    recursive,
				 gboolean    clear_all)
{
	nautilus_cache_remove_old_previews_async (recursive, clear_all);
}
