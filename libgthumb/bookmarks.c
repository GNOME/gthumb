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

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <glib/gi18n.h>
#include <libgnomevfs/gnome-vfs-ops.h>

#include "typedefs.h"
#include "bookmarks.h"
#include "catalog.h"
#include "file-utils.h"
#include "preferences.h"

#define MAX_LINE_LENGTH 4096
#define MAX_LINES 100
#define FILE_PERMISSIONS 0600


Bookmarks *
bookmarks_new (char *rc_filename)
{
	Bookmarks *bookmarks;

	bookmarks = g_new (Bookmarks, 1);
	bookmarks->list = NULL;
	bookmarks->names = g_hash_table_new (g_str_hash, g_str_equal);
	bookmarks->tips = g_hash_table_new (g_str_hash, g_str_equal);
	bookmarks->max_lines = MAX_LINES;

	if (rc_filename != NULL)
		bookmarks->rc_filename = g_strdup (rc_filename);
	else
		bookmarks->rc_filename = NULL;

	return bookmarks;
}


static gboolean
remove_all (gpointer key, gpointer value, gpointer user_data)
{
	g_free (key);
	g_free (value);

	return TRUE;
}


static void
bookmarks_free_data (Bookmarks *bookmarks)
{
	if (bookmarks->list != NULL) {
		path_list_free (bookmarks->list);
		bookmarks->list = NULL;
	}

	g_hash_table_foreach_remove (bookmarks->names, remove_all, NULL);
	g_hash_table_foreach_remove (bookmarks->tips, remove_all, NULL);
}


void
bookmarks_free (Bookmarks *bookmarks)
{
	g_return_if_fail (bookmarks != NULL);

	bookmarks_free_data (bookmarks);
	g_hash_table_destroy (bookmarks->names);
	g_hash_table_destroy (bookmarks->tips);

	if (bookmarks->rc_filename != NULL)
		g_free (bookmarks->rc_filename);

	g_free (bookmarks);
}


static char *
get_menu_item_tip (const char *path)
{	
	int   offset = 0;
	char *tmp_path;
	char *tip;

	tmp_path = g_strdup (path);

	if (uri_scheme_is_catalog (tmp_path) || uri_scheme_is_search (tmp_path)) {
		gchar *rc_dir_prefix;

		/* if it is a catalog then remove the extension */
		tmp_path[strlen (tmp_path) - strlen (CATALOG_EXT)] = 0;

		rc_dir_prefix = g_strconcat (g_get_home_dir (),
					     "/",
					     RC_CATALOG_DIR,
					     "/",
					     NULL);

		offset = strlen (rc_dir_prefix);

		g_free (rc_dir_prefix);
	}

	tip = g_strdup (remove_scheme_from_uri (tmp_path) + offset);
	g_free (tmp_path);

	return tip;
}


static void 
my_insert (GHashTable    *hash_table,
	   gconstpointer  key,
	   gpointer       value)
{
	if (g_hash_table_lookup (hash_table, key) != NULL) {
		g_free (value);
		return;
	}

	g_hash_table_insert (hash_table, g_strdup (key), value);
}


static void
my_remove (GHashTable    *hash_table,
	   gconstpointer  lookup_key)
{
	gpointer orig_key, value;

	if (g_hash_table_lookup_extended (hash_table, 
					  lookup_key,
					  &orig_key, 
					  &value)) {
		g_hash_table_remove (hash_table, lookup_key);
		g_free (orig_key);
		g_free (value);
	}
}


void
bookmarks_add (Bookmarks   *bookmarks,
	       const char  *path,
	       gboolean     avoid_duplicates,
	       gboolean     append)
{
	g_return_if_fail (bookmarks != NULL);
	g_return_if_fail (path != NULL);	

	if (avoid_duplicates) {
		GList *scan;
		for (scan = bookmarks->list; scan; scan = scan->next)
			if (strcmp ((char*) scan->data, path) == 0)
				return;
	}
	
	if (append)
		bookmarks->list = g_list_append (bookmarks->list, 
						 g_strdup (path));
	else
		bookmarks->list = g_list_prepend (bookmarks->list, 
						  g_strdup (path));

	my_insert (bookmarks->names, 
		   path, 
		   get_uri_display_name (path));

	my_insert (bookmarks->tips, 
		   path, 
		   get_menu_item_tip (path));
}


static GList *
get_link_from_path (GList      *list, 
		    const char *path)
{
	GList *scan;

	for (scan = list; scan; scan = scan->next)
		if (strcmp ((char*) scan->data, path) == 0)
			return scan;
	
	return NULL;
}


void
bookmarks_remove (Bookmarks  *bookmarks,
		  const char *path)
{
	GList *link;

	g_return_if_fail (bookmarks != NULL);
	g_return_if_fail (path != NULL);	

	link = get_link_from_path (bookmarks->list, path);
	if (link == NULL)
		return;

	bookmarks->list = g_list_remove_link (bookmarks->list, link);
	g_free (link->data);
	g_list_free (link);

	if (get_link_from_path (bookmarks->list, path) == NULL) {
		my_remove (bookmarks->names, path);
		my_remove (bookmarks->tips, path);
	}
}


void
bookmarks_remove_all_instances (Bookmarks   *bookmarks,
				const char  *path)
{
	GList *link;

	g_return_if_fail (bookmarks != NULL);
	g_return_if_fail (path != NULL);	

	link = get_link_from_path (bookmarks->list, path);

	while (link != NULL) {
		bookmarks->list = g_list_remove_link (bookmarks->list, link);
		g_free (link->data);
		g_list_free (link);

		link = get_link_from_path (bookmarks->list, path);
	}

	my_remove (bookmarks->names, path);
	my_remove (bookmarks->tips, path);
}


void
bookmarks_remove_from (Bookmarks *bookmarks,
		       GList     *here)
{
	GList *link;

	g_return_if_fail (bookmarks != NULL);

	if (here == NULL)
		return;

	for (link = bookmarks->list; link && (link != here); link = bookmarks->list) {
		char *path = link->data;

		bookmarks->list = g_list_remove_link (bookmarks->list, link);
		g_list_free (link);

		if (get_link_from_path (bookmarks->list, path) == NULL) {
			my_remove (bookmarks->names, path);
			my_remove (bookmarks->tips, path);
		}

		g_free (link->data);
	}
}


void
bookmarks_remove_all (Bookmarks *bookmarks)
{
	bookmarks_free_data (bookmarks);
}


void
bookmarks_load_from_disk (Bookmarks *bookmarks)
{
	GnomeVFSResult  result;
	GnomeVFSHandle *handle;
	char           *uri;
	char            line [MAX_LINE_LENGTH];

	g_return_if_fail (bookmarks != NULL);

	bookmarks_free_data (bookmarks);
	if (bookmarks->rc_filename == NULL)
		return;

	uri = g_strconcat (get_home_uri (),
			   "/",
			   bookmarks->rc_filename,
			   NULL);
	result = gnome_vfs_open (&handle, uri, GNOME_VFS_OPEN_READ);
	g_free (uri);

	if (result != GNOME_VFS_OK)
		return;

	while (_gnome_vfs_read_line (handle,
				     line,
				     MAX_LINE_LENGTH,
				     NULL) == GNOME_VFS_OK) {
		char *path;

		if (line[0] != '"')
			continue;

		line[strlen (line) - 1] = 0;
		path = line + 1;

		bookmarks->list = g_list_prepend (bookmarks->list, 
						  g_strdup (path));
		my_insert (bookmarks->names, 
			   path, 
			   get_uri_display_name (path));

		my_insert (bookmarks->tips, 
			   path, 
			   get_menu_item_tip (path));
	}

	gnome_vfs_close (handle);

	bookmarks->list = g_list_reverse (bookmarks->list);
}


void
bookmarks_write_to_disk (Bookmarks *bookmarks)
{
	GnomeVFSResult  result;
	GnomeVFSHandle *handle;
	char           *uri;
	int             lines;
	GList          *scan;

	g_return_if_fail (bookmarks != NULL);

	if (bookmarks->rc_filename == NULL)
		return;

	uri = g_strconcat (get_home_uri (),
			   "/",
			   bookmarks->rc_filename,
			   NULL);
	result = gnome_vfs_create (&handle, uri, GNOME_VFS_OPEN_WRITE, FALSE, FILE_PERMISSIONS);
	g_free (uri);

	if (result != GNOME_VFS_OK)
		return;

	/* write the file list. */

	lines = 0;
	scan = bookmarks->list; 
	while (((bookmarks->max_lines < 0) || (lines < bookmarks->max_lines)) 
	       && (scan != NULL)) {
		if (_gnome_vfs_write_line (handle,
					   "\"%s\"",
					   (char*) scan->data) != GNOME_VFS_OK) {
			g_print ("ERROR saving to bookmark file\n");
			break;
		}
		lines++;
		scan = scan->next;
	}

	gnome_vfs_close (handle);
}


const gchar *
bookmarks_get_menu_name (Bookmarks *bookmarks,
			 const gchar *path)
{
	return g_hash_table_lookup (bookmarks->names, path);
}


const gchar *
bookmarks_get_menu_tip (Bookmarks *bookmarks,
			const gchar *path)
{
	return g_hash_table_lookup (bookmarks->tips, path);
}


void
bookmarks_set_max_lines (Bookmarks   *bookmarks,
			 int          max_lines)
{
	bookmarks->max_lines = max_lines;
}


int
bookmarks_get_max_lines (Bookmarks   *bookmarks)
{
	return bookmarks->max_lines;
}
