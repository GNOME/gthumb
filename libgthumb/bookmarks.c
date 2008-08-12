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
#include "gfile-utils.h"
#include "preferences.h"

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

	tmp_path = get_utf8_display_name_from_uri (path);

	if (uri_scheme_is_catalog (tmp_path) || uri_scheme_is_search (tmp_path)) {
		char *rc_dir_prefix;

		/* if it is a catalog then remove the extension */
		tmp_path[strlen (tmp_path) - strlen (CATALOG_EXT)] = 0;

		rc_dir_prefix = g_strconcat (g_get_home_dir (),
					     "/",
					     RC_CATALOG_DIR,
					     "/",
					     NULL);

		offset = strlen (rc_dir_prefix);
		tip = g_strdup (remove_host_from_uri (tmp_path) + offset);

		g_free (rc_dir_prefix);
	}
	else {
		tip = tmp_path;
		tmp_path = NULL;
	}

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
			if (same_uri ((char*) scan->data, path))
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
		if (same_uri ((char*) scan->data, path))
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

		if (get_link_from_path (bookmarks->list, path) == NULL) {
			my_remove (bookmarks->names, path);
			my_remove (bookmarks->tips, path);
		}

		g_free (link->data);
		g_list_free (link);
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
	GFileInputStream *istream;
	GDataInputStream *dstream;
	GError		 *error = NULL;
	GFile		 *gfile;
	GFile		 *home_dir;
	char             *line;
	gsize		  length;

	g_return_if_fail (bookmarks != NULL);

	bookmarks_free_data (bookmarks);
	if (bookmarks->rc_filename == NULL)
		return;

        home_dir = gfile_get_home_dir ();
        gfile = gfile_append_path (home_dir, bookmarks->rc_filename, NULL);
	g_object_unref (home_dir);

	istream = g_file_read (gfile, NULL, &error);

        if (error != NULL) {
                gfile_warning ("Cannot load bookmark file",
                               gfile,
                               error);
                g_error_free (error);
		return;
        }

	dstream = g_data_input_stream_new (G_INPUT_STREAM(istream));

	while ((line = g_data_input_stream_read_line (dstream, &length, NULL, &error))) {
		char *path;

		if (line[0] != '"')
			continue;

		line[strlen (line) - 1] = 0;
		path = line + 1;

		bookmarks->list = g_list_prepend (bookmarks->list, g_strdup (path));
		my_insert (bookmarks->names,
			   path,
			   get_uri_display_name (path));
		my_insert (bookmarks->tips,
			   path,
			   get_menu_item_tip (path));
	}

	g_object_unref (dstream);
	g_object_unref (istream);
	g_object_unref (gfile);

	bookmarks->list = g_list_reverse (bookmarks->list);
}


void
bookmarks_write_to_disk (Bookmarks *bookmarks)
{
        GFileOutputStream *ostream;
	GError            *error = NULL;
        GFile             *gfile;
        GFile             *home_dir;
	int                lines;
	GList             *scan;

	g_return_if_fail (bookmarks != NULL);

	if (bookmarks->rc_filename == NULL)
		return;

        home_dir = gfile_get_home_dir ();
        gfile = gfile_append_path (home_dir, bookmarks->rc_filename, NULL);
        g_object_unref (home_dir);

        ostream = g_file_replace (gfile, NULL, FALSE, G_FILE_CREATE_NONE, NULL, &error);

        if (error) {
                gfile_warning ("Cannot write to bookmark file",
                               gfile,
                               error);
                g_error_free (error);
                return;
        }

	/* write the file list. */

	lines = 0;
	scan = bookmarks->list;
	while (((bookmarks->max_lines < 0) || (lines < bookmarks->max_lines))
	       && (scan != NULL)) {
		gfile_output_stream_write_line (ostream, error, "\"%s\"", scan->data);

	        if (error) {
	                gfile_warning ("Cannot write line to bookmark file",
        	                       gfile,
                	               error);
	                g_error_free (error);
			break;
	        }
		lines++;
		scan = scan->next;
	}

	g_output_stream_close (G_OUTPUT_STREAM(ostream), NULL, NULL);
	g_object_unref (ostream);
	g_object_unref (gfile);	
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
