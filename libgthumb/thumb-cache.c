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

#include <config.h>
#include <stdio.h>
#include <sys/stat.h>   
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>

#include <libgnome/libgnome.h>
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


static gboolean
has_png_extension (const gchar *filename)
{
	gint l;

	if (filename == NULL)
		return FALSE;

	l = strlen (filename);
	if (l <= 4)
		return FALSE;

	return (strncmp (filename + l - 4, ".png", 4) == 0);
}


static gchar *
get_aa_name (gchar *filename)
{
	gchar *dot_pos;
	gchar *old_name;

	old_name = filename;
	dot_pos = strrchr (filename, '.');
	if (dot_pos) {
		*dot_pos = '\0';
		filename = g_strdup_printf ("%s.aa.%s", old_name, dot_pos + 1);
	} else 
		filename = g_strconcat (old_name, ".aa", NULL);
	g_free (old_name);

	return filename;
}


static gchar*
get_escaped_dir_name (const char *dir_path)
{
	const char *dir_name;
	char *dir_container;
	char *e_name;
	char *e_path;
	char *result;

	if (dir_path == NULL)
		return NULL;

	dir_name = file_name_from_path (dir_path);
	dir_container = remove_level_from_path (dir_path);
	
	if (strcmp (dir_container, "/") != 0) {
		char *tmp;
		char *e_name_tmp;
		
		tmp = g_strconcat (dir_container, "/", NULL);
		g_free (dir_container);
		dir_container = tmp;
		
		e_name_tmp = gnome_vfs_escape_string (dir_name);
		e_name = gnome_vfs_escape_slashes (e_name_tmp);
		g_free (e_name_tmp);
		
		e_path = gnome_vfs_escape_slashes (dir_container);
	} else {
		e_name = NULL;
		e_path = NULL;
	}
	g_free (dir_container);
	
	result = g_strconcat (g_get_home_dir (),
			      "/.nautilus/thumbnails/",
			      "file:%2F%2F",
			      e_path,
			      e_name,
			      NULL);
	
	g_free (e_name);
	g_free (e_path);
	
	return result;
}


gchar *
cache_get_nautilus_thumbnail_file (const gchar *source) 
{
	gint try;

	if (! source) return NULL;

	for (try = 0; try < 4; try++) {
		gchar *path;
		gchar *directory = NULL;
		gchar *filename = NULL;
		gchar *filedir = NULL;

		switch (try) {
		case 0: /* not aa image in .nautilus/thumbnails dir. */
		case 1: /* aa image in .nautilus/thumbnails dir. */
			filename = g_strdup (file_name_from_path (source));
			if (try == 1)
				filename = get_aa_name (filename);
			filedir = remove_level_from_path (source);
			directory = get_escaped_dir_name (filedir);
			g_free (filedir);
			break;

		case 2: /* not aa image in .thumbnails dir. */
		case 3: /* aa image in .thumbnails dir. */
			filename = g_strdup (file_name_from_path (source));
			if (try == 3)
				filename = get_aa_name (filename);
			filedir = remove_level_from_path (source);
			directory = g_strconcat (filedir,
						 "/", 
						 CACHE_DIR, 
						 NULL);
			g_free (filedir);
			break;
		}

		path = g_strconcat (directory,
				    "/",
				    filename,
				    has_png_extension (filename)? NULL: ".png",
				    NULL);

		g_free (directory);
		g_free (filename);

		if (path_is_file (path))  /* found. */
			return path;

		g_free (path);
	}

	return NULL;
}


gchar *
cache_get_gthumb_cache_name (const gchar *source) 
{
	gchar *path;
	gchar *directory;
	const gchar *filename;

	if (!source) return NULL;

	directory = remove_level_from_path (source);
	filename = file_name_from_path (source);

	path = g_strconcat (g_get_home_dir(), 
			    "/", 
			    RC_THUMBS_DIR, 
			    directory,
			    "/",
			    filename, 
			    CACHE_THUMB_EXT, 
			    NULL);

	g_free (directory);

	return path;
}


gchar *
cache_get_nautilus_cache_name (const gchar *source) 
{
	char *dir;
	char *escaped_dir;
	char *nautilus_name;

	if (! source) return NULL;

	/* get the Nautilus cache file name. */

	dir = remove_level_from_path (source);
	escaped_dir = get_escaped_dir_name (dir);
	nautilus_name = g_strconcat (escaped_dir,
				     "/",
				     file_name_from_path (source),
				     has_png_extension (source) ? NULL : ".png",
				     NULL);

	g_free (dir);
	g_free (escaped_dir);

	return nautilus_name;
}


gchar *
cache_get_nautilus_cache_dir (const gchar *source) 
{
	return get_escaped_dir_name (source);
}


void
cache_copy (const gchar *src,
	    const gchar *dest)
{
	char   *cache_src;
	time_t  dest_mtime = get_file_mtime (dest);
	
	cache_src = cache_get_gthumb_cache_name (src);
	if (path_is_file (cache_src)) {
		char *cache_dest = cache_get_gthumb_cache_name (dest);

		if (path_is_file (cache_dest)) 
			unlink (cache_dest);
		if (file_copy (cache_src, cache_dest))
			set_file_mtime (cache_dest, dest_mtime);

		g_free (cache_dest);
	}
	g_free (cache_src);

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

	cache_src = cache_get_gthumb_cache_name (src);
	if (path_is_file (cache_src)) {
		char *cache_dest = cache_get_gthumb_cache_name (dest);

		if (path_is_file (cache_dest)) 
			unlink (cache_dest);
		if (file_move (cache_src, cache_dest))
			set_file_mtime (cache_dest, dest_mtime);

		g_free (cache_dest);
	}
	g_free (cache_src);

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
cache_delete (const gchar *filename)
{
	char *cache_name;

	cache_name = cache_get_gthumb_cache_name (filename);
	unlink (cache_name);
	g_free (cache_name);

	cache_name = cache_get_nautilus_cache_name (filename);
	unlink (cache_name);
	g_free (cache_name);
}


void
cache_remove_old_previews (const gchar *dir,
			   gboolean recursive,
			   gboolean clear_all)
{
	visit_rc_directory (RC_THUMBS_DIR,
			    CACHE_THUMB_EXT,
			    dir,
			    recursive,
			    clear_all);
}


/* ----- cache_remove_old_previews_async implememtation. ------ */


typedef struct {
	gboolean   recursive;
	gboolean   clear_all;
	GtkWidget *dialog;
} CacheRemoveData;


static void nautilus_cache_remove_old_previews_async (gboolean recursive,
						      gboolean clear_all);


static void
cache_remove_done (const GList *dir_list,
		   gpointer     data)
{
	CacheRemoveData *crd = data;

	if (crd->clear_all) {
		const GList *scan;
		for (scan = dir_list; scan; scan = scan->next) {
			gchar *dir = scan->data;
			rmdir (dir);
		}
	}

	gtk_widget_destroy (crd->dialog);

	nautilus_cache_remove_old_previews_async (crd->recursive,
						  crd->clear_all);
	g_free (crd);
}


static void 
check_cache_file (gchar *real_file, 
		  gchar *rc_file, 
		  gpointer data)
{
	CacheRemoveData *crd = data;

	if (crd->clear_all || ! path_is_file (real_file)) {
		if ((unlink (rc_file) < 0))
			g_warning ("Cannot delete %s\n", rc_file);
	}
}


void
cache_remove_old_previews_async (const gchar *dir,
				 gboolean recursive,
				 gboolean clear_all)
{
	CacheRemoveData *crd;

	crd = g_new (CacheRemoveData, 1);
	crd->recursive = recursive;
	crd->clear_all = clear_all;
	crd->dialog = _gtk_message_dialog_new (NULL,
					       GTK_DIALOG_MODAL,
					       GTK_MESSAGE_INFO,
					       _("Deleting old thumbnails, wait please..."),
					       GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE,
					       NULL);
	g_signal_connect_swapped (G_OBJECT (crd->dialog),
				  "response",
				  G_CALLBACK (gtk_widget_hide),
				  crd->dialog);
	gtk_widget_show (crd->dialog);

	visit_rc_directory_async (RC_THUMBS_DIR,
				  CACHE_THUMB_EXT,
				  dir,
				  recursive,
				  check_cache_file,
				  cache_remove_done,
				  crd);
}


/* ----- nautilus_cache_remove_old_previews_async implememtation. ------ */


typedef struct {
	gboolean   recursive;
	gboolean   clear_all;
	GList     *dirs;
	GList     *visited_dirs;
	gchar     *nautilus_thumb_dir;
	int        nautilus_thumb_dir_l;
	GtkWidget *dialog;
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

	gtk_widget_destroy (ncrd->dialog);

	g_free (ncrd);
}


static void visit_dir_async (const gchar *dir,
			     NautilusCacheRemoveData *ncrd);


#define ESC_FILE_PREFIX    "file:%2F%2F"
#define ESC_FILE_PREFIX_L  11
#define ESC_SLASH          "%2F"
#define ESC_SLASH_L        3


static gchar *
get_real_name_from_nautilus_cache (NautilusCacheRemoveData *ncrd,
				   const char *cache_name_full_path)
{
	const char *cache_path;
	const char *cache_name;
	char *cache_dir;
	char *e_cache_dir;
	int   cache_path_l, real_name_l;
	char *real_name;
	char *tmp;

	if (strlen (cache_name_full_path) < ncrd->nautilus_thumb_dir_l + 1)
		return NULL;

	cache_path = cache_name_full_path + ncrd->nautilus_thumb_dir_l + 1;
	cache_path_l = strlen (cache_path);

	if (cache_path_l < ESC_FILE_PREFIX_L)
		return NULL;

	if (strncmp (cache_path, ESC_FILE_PREFIX, ESC_FILE_PREFIX_L) != 0)
		return NULL;

	cache_path = cache_path + ESC_FILE_PREFIX_L;
	cache_dir = remove_level_from_path (cache_path);
	cache_name = file_name_from_path (cache_path);

	tmp = gnome_vfs_unescape_string (cache_dir, NULL);
	e_cache_dir = gnome_vfs_unescape_string (tmp, NULL);
	g_free (tmp);
	g_free (cache_dir);

	real_name = g_strconcat (e_cache_dir, "/", cache_name, NULL);
	g_free (e_cache_dir);

	/* FIXME : delete this code */

#if 0
	/* --- 8< --- */

	{
		char *test_file;
		char *test_dir;
		char *test_e_dir;	
		char *test;

		test_file = g_strdup (file_name_from_path (real_name));
		test_dir = remove_level_from_path (real_name);
		test_e_dir = get_escaped_dir_name (test_dir);
		test = g_strconcat (test_e_dir, "/", test_file, NULL);
		
		if (strcmp (test, cache_name_full_path) != 0) {
			g_error ("%s --> %s --> %s\n", cache_name_full_path, real_name, test);
		}

		g_free (test_file);
		g_free (test_dir);
		g_free (test_e_dir);
		g_free (test);
	}

	/* --- 8< --- */
#endif

	/* Remove the png extension. */

	real_name_l = strlen (real_name);
        real_name[real_name_l - 4] = 0; /* 4 = strlen (".png") */
        if (! file_is_image (real_name, TRUE))
                real_name[real_name_l - 4] = '.';

	return real_name;
}


static void
path_list_done_cb (PathListData *pld, 
		   gpointer data)
{
	NautilusCacheRemoveData *ncrd = data;
	GList *scan;
	gchar *rc_file, *real_file;
	gchar *sub_dir;

	if (pld->result != GNOME_VFS_ERROR_EOF) {
		gchar *path;

		path = gnome_vfs_uri_to_string (pld->uri,
						GNOME_VFS_URI_HIDE_NONE);
		g_warning ("Error reading cache directory %s.", path);
		g_free (path);

		nautilus_cache_data_free (ncrd);
		return;
	}

	for (scan = pld->files; scan; scan = scan->next) {
		rc_file = (gchar*) scan->data;
		real_file = get_real_name_from_nautilus_cache (ncrd, rc_file);

		if (real_file == NULL)
			continue;

		if (ncrd->clear_all || ! path_is_file (real_file)) {
			if ((unlink (rc_file) < 0)) 
				g_warning ("Cannot delete %s\n", rc_file);
		}

		g_free (real_file);
	}

	if (! ncrd->recursive) {
		path_list_data_free (pld);
		nautilus_cache_data_free (ncrd);
		return;
	}

	ncrd->dirs = g_list_concat (pld->dirs, ncrd->dirs);
	pld->dirs = NULL;
	path_list_data_free (pld);

	if (ncrd->dirs == NULL) {
		if (ncrd->clear_all) {
			const GList *scan = ncrd->visited_dirs;

			for (; scan; scan = scan->next) {
				gchar *dir = scan->data;
				rmdir (dir);
			}
		}
		nautilus_cache_data_free (ncrd);
		return;
	}

	sub_dir = (gchar*) ncrd->dirs->data;
	ncrd->dirs = g_list_remove_link (ncrd->dirs, ncrd->dirs);

	ncrd->visited_dirs = g_list_prepend (ncrd->visited_dirs,
					     g_strdup (sub_dir));
	visit_dir_async (sub_dir, ncrd);

	g_free (sub_dir);
}


static void
visit_dir_async (const gchar *dir,
		 NautilusCacheRemoveData *data)
{
	PathListHandle *handle;

	handle = path_list_async_new (dir, path_list_done_cb, data);
	g_free (handle);
}


static void 
nautilus_cache_remove_old_previews_async (gboolean recursive,
					  gboolean clear_all)
{
	NautilusCacheRemoveData *ncrd;

	ncrd = g_new (NautilusCacheRemoveData, 1);

	ncrd->recursive = recursive;
	ncrd->clear_all = clear_all;
	ncrd->dirs = NULL;
	ncrd->visited_dirs = NULL;

	ncrd->nautilus_thumb_dir = g_strconcat (g_get_home_dir (),
						"/.nautilus/thumbnails",
						NULL);
	ncrd->nautilus_thumb_dir_l = strlen (ncrd->nautilus_thumb_dir);

	ncrd->dialog =  _gtk_message_dialog_new (NULL,
						 GTK_DIALOG_MODAL,
						 GTK_MESSAGE_INFO,
						 _("Deleting old thumbnails, wait please..."),
						 GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE,
						 NULL);
	g_signal_connect_swapped (G_OBJECT (ncrd->dialog),
				  "response",
				  G_CALLBACK (gtk_widget_hide),
				  ncrd->dialog);

	gtk_widget_show (ncrd->dialog);

	visit_dir_async (ncrd->nautilus_thumb_dir, ncrd);
}
