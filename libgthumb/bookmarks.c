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

#include "typedefs.h"
#include "bookmarks.h"
#include "catalog.h"
#include "file-utils.h"
#include "preferences.h"


#define MAX_LINES 100


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


char *
bookmarks_utils__get_menu_item_name (const char *path)
{
	char     *tmp_path;
	gboolean  catalog_or_search;
	char     *name;

	tmp_path = g_strdup (remove_scheme_from_uri (path));

	/* if it is a catalog then remove the extension */
	
	catalog_or_search = uri_scheme_is_catalog (path) || uri_scheme_is_search (path);
	if (catalog_or_search)
		tmp_path[strlen (tmp_path) - strlen (CATALOG_EXT)] = 0;
			 
	if ((tmp_path == NULL) 
	    || (strcmp (tmp_path, "") == 0)
	    || (strcmp (tmp_path, "/") == 0))
		name = g_strdup (_("File System"));
	else {
		if (catalog_or_search) {
			char *base_path;
			int   l;

			base_path = get_catalog_full_path (NULL);
			l = strlen (base_path);
			g_free (base_path);

			name = g_strdup (tmp_path + 1 + l);
		} else {
			const char *base_path;
			int         l;
			
			base_path = g_get_home_dir ();
			l = strlen (base_path);
			
			if (strncmp (tmp_path, base_path, l) == 0) {
				int tmp_l = strlen (tmp_path);
				if (tmp_l == l)
					name = g_strdup (_("Home"));
				else if (tmp_l > l)
					name = g_strdup (tmp_path + 1 + l);
				else
					name = g_strdup (file_name_from_path (base_path));
			} else
				name = g_strdup (tmp_path);
		}
	}
	
	g_free (tmp_path);
	
	return name;
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
		   bookmarks_utils__get_menu_item_name (path));

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


#define MAX_LINE_LENGTH 4096
void
bookmarks_load_from_disk (Bookmarks *bookmarks)
{
	gchar  line [MAX_LINE_LENGTH];
	gchar *path;
	FILE  *f;

	g_return_if_fail (bookmarks != NULL);

	bookmarks_free_data (bookmarks);
	if (bookmarks->rc_filename == NULL)
		return;

	path = g_strconcat (g_get_home_dir (),
			    "/",
			    bookmarks->rc_filename,
			    NULL);

	f = fopen (path, "r");
	g_free (path);

	if (!f)	return;

	while (fgets (line, sizeof (line), f)) {
		char *path;

		if (line[0] != '"')
			continue;

		line[strlen (line) - 2] = 0;
		path = line + 1;

		bookmarks->list = g_list_prepend (bookmarks->list, 
						  g_strdup (path));
		my_insert (bookmarks->names, 
			   path, 
			   bookmarks_utils__get_menu_item_name (path));

		my_insert (bookmarks->tips, 
			   path, 
			   get_menu_item_tip (path));
	}
	fclose (f);

	bookmarks->list = g_list_reverse (bookmarks->list);
}
#undef MAX_LINE_LENGTH	


void
bookmarks_write_to_disk (Bookmarks *bookmarks)
{
	FILE *f;
	gchar *path;
	GList *scan;
	gint lines;

	g_return_if_fail (bookmarks != NULL);
	if (bookmarks->rc_filename == NULL)
		return;

	path = g_strconcat (g_get_home_dir (),
			    "/",
			    bookmarks->rc_filename,
			    NULL);

	f = fopen (path, "w+");
	g_free (path);
	
	if (!f)	{
		g_print ("ERROR opening bookmark file\n");
		return;
	}
	
	/* write the file list. */
	lines = 0;
	scan = bookmarks->list; 
	while (((bookmarks->max_lines < 0) || (lines < bookmarks->max_lines)) 
	       && scan) {
		if (! fprintf (f, "\"%s\"\n", (gchar*) scan->data)) {
			g_print ("ERROR saving to bookmark file\n");
			fclose (f);
			
			return;
		}
		lines++;
		scan = scan->next;
	}

	fclose (f);
}
#undef lines


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
