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
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <libgnome/libgnome.h>
#include "catalog.h"
#include "typedefs.h"
#include "file-utils.h"
#include "gthumb-error.h"


#define SEARCH_HEADER "# Search\n"
#define MAX_LINE_LENGTH 4096


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

	path = g_strconcat (g_get_home_dir (),
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
	if (rmdir (full_path) == 0)
		return TRUE;

	if (gerror != NULL) {
		const char *rel_path;
		char       *base_path;
		char       *utf8_path;
		const char *details;

		base_path = get_catalog_full_path (NULL);
		rel_path = full_path + strlen (base_path) + 1;
		g_free (base_path);

		utf8_path = g_locale_to_utf8 (rel_path, -1, 0, 0, 0);

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
				       _("Cannot remove library \"%s\" : %s"),
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
	if (unlink (full_path) != 0) {
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
					       _("Cannot remove catalog \"%s\" : %s"),
					       catalog,
					       errno_to_string ());
			g_free (catalog);
		}
		return FALSE;
	}

	return TRUE;
}


gboolean
file_is_search_result (const char *fullpath)
{
	char      line[MAX_LINE_LENGTH];
	FILE     *f;

	f = fopen (fullpath, "r");
	if (!f)	{
		/* FIXME */
		g_print ("ERROR: Failed opening catalog file: %s\n", fullpath);
		return FALSE;
	}

	fgets (line, sizeof (line), f);
	fclose (f);

	return strncmp (line, SEARCH_HEADER, strlen (SEARCH_HEADER)) == 0;
}


Catalog*  
catalog_new ()
{
	Catalog *catalog;

	catalog = g_new (Catalog, 1);
	catalog->path = NULL;
	catalog->list = NULL;
	catalog->search_data = NULL;

	return catalog;
}


void
catalog_free (Catalog *catalog)
{
	if (catalog->path)
		g_free (catalog->path);
	if (catalog->list) 
		path_list_free (catalog->list);
	if (catalog->search_data)
		search_data_free (catalog->search_data);
	g_free (catalog);
}


void
catalog_set_path (Catalog *catalog,
		  char *full_path)
{
	g_return_if_fail (catalog != NULL);

	if (catalog->path != NULL)
		g_free (catalog->path);

	catalog->path = NULL;
	if (full_path)
		catalog->path = g_strdup (full_path);
}


void
catalog_set_search_data (Catalog *catalog,
			 SearchData *search_data)
{
	g_return_if_fail (catalog != NULL);

	if (catalog->search_data != NULL)
		search_data_free (catalog->search_data);

	if (search_data != NULL) {
		catalog->search_data = search_data_new ();
		search_data_copy (catalog->search_data, search_data);
	}
}


gboolean
catalog_is_search_result (Catalog *catalog)
{
	g_return_val_if_fail (catalog != NULL, FALSE);
	return (catalog->search_data != NULL);
}


static void
copy_unquoted (char *unquoted, char *line)
{
	gint l = strlen (line);

	strncpy (unquoted, line + 1, l - 3);
	unquoted[l - 3] = 0;
}


gboolean
catalog_load_from_disk (Catalog     *catalog,
			const char  *fullpath,
			GError     **gerror)
{
	gchar  line[MAX_LINE_LENGTH];
	FILE  *f;

	f = fopen (fullpath, "r");
	if (f == NULL) {
		if (gerror != NULL)
			*gerror = g_error_new (GTHUMB_ERROR,
					       errno,
					       _("Cannot open catalog \"%s\" : %s"),
					       fullpath,
					       errno_to_string ());
		return FALSE;
	}

	if (catalog->path != NULL)
		g_free (catalog->path);
	if (catalog->list != NULL)
		path_list_free (catalog->list);
	if (catalog->search_data != NULL)
		search_data_free (catalog->search_data);

	catalog->path = g_strdup (fullpath);
	catalog->list = NULL;
	catalog->search_data = NULL;

	/*
	 * Catalog Format : A list of quoted filenames, one name per line :
	 *     "filename_1"
	 *     "filename_2"
	 *     ...
	 *     "filename_n"
	 *
	 * Search Format : Search data followed by files list :
	 *    # Search           Line 1 : search data starts with SEARCH_HEADER
	 *    "/home/pippo"      Line 2 : directory to start from.
	 *    "FALSE"            Line 3 : recursive.
	 *    "*.jpg"            Line 4 : file pattern.
	 *    ""                 Line 5 : comment pattern.
	 *    "Rome; Paris"      Line 6 : place pattern.
	 *    0"Formula 1"       Line 7 : old versions have only a keywords 
	 *                                pattern, new versions have a 0 or 1
	 *                                (0 = must match at leaset one 
	 *                                keyword; 1 = must match all keywords)
	 *				  followed by the keywords pattern.
	 *    1022277600         Line 8 : date in time_t format.
	 *    0                  Line 9 : date scope (DATE_ANY      = 0, 
	 *                                            DATE_BEFORE   = 1,
	 *                                            DATE_EQUAL_TO = 2,
	 *                                            DATE_AFTER    = 3)
	 *    "filename_1"       From Line 10 as catalog format.
	 *    "filename_2"
	 *    ...
	 *    "filename_n"
	 *   
	 */
	while (fgets (line, sizeof (line), f)) {
		char *file_name;

		if (*line == 0)
			continue;

		/* The file name is quoted. */
		if (*line != '"') {
			char     unquoted [MAX_LINE_LENGTH];
			time_t   date;
			int      date_scope;
			gboolean all_keywords = FALSE;
			int      line_ofs = 0;

			/* search data starts with SEARCH_HEADER */

			if (strcmp (line, SEARCH_HEADER) != 0) 
				continue;

			/* load search data. */

			catalog->search_data = search_data_new ();

			/* * start from */

			fgets (line, sizeof (line), f);
			copy_unquoted (unquoted, line);
			search_data_set_start_from (catalog->search_data,
						    unquoted);

			/* * recursive */

			fgets (line, sizeof (line), f);
			copy_unquoted (unquoted, line);
			search_data_set_recursive (catalog->search_data, strcmp (unquoted, "TRUE") == 0);

			/* * file pattern */

			fgets (line, sizeof (line), f);
			copy_unquoted (unquoted, line);
			search_data_set_file_pattern (catalog->search_data,
						      unquoted);

			/* * comment pattern */

			fgets (line, sizeof (line), f);
			copy_unquoted (unquoted, line);
			search_data_set_comment_pattern (catalog->search_data,
							 unquoted);

			/* * place pattern */

			fgets (line, sizeof (line), f);
			copy_unquoted (unquoted, line);
			search_data_set_place_pattern (catalog->search_data,
						       unquoted);

			/* * keywords pattern */

			fgets (line, sizeof (line), f);
			if (*line != '"') {
				line_ofs = 1;
				/* * all keywords */
				all_keywords = (*line == '1');
			}
			copy_unquoted (unquoted, line + line_ofs);
			search_data_set_keywords_pattern (catalog->search_data,
							  unquoted,
							  all_keywords);

			/* * date */

			fscanf (f, "%ld\n", &date);
			search_data_set_date (catalog->search_data, date);

			/* * date scope */

			fscanf (f, "%d\n", &date_scope);
			search_data_set_date_scope (catalog->search_data, 
						    date_scope);
		}

		file_name = g_strndup (line + 1, strlen (line) - 3);
		catalog->list = g_list_prepend (catalog->list, file_name);
	}
	fclose (f);

	return TRUE;
}


static void
error_on_saving (char    *path, 
		 GError **gerror)
{
	if (gerror != NULL)
		*gerror = g_error_new (GTHUMB_ERROR,
				       errno,
				       _("Cannot save catalog \"%s\" : %s"),
				       path,
				       errno_to_string ());
}


gboolean
catalog_write_to_disk (Catalog     *catalog,
		       GError     **gerror)
{
	FILE *f;
	gchar *path;
	GList *scan;

	g_return_val_if_fail (catalog != NULL, FALSE);
	g_return_val_if_fail (catalog->path != NULL, FALSE);

	path = catalog->path;
	f = fopen (path, "w");
	if (!f)	{
		if (gerror != NULL)
			*gerror = g_error_new (GTHUMB_ERROR,
					       errno,
					       _("Cannot open catalog \"%s\" : %s"),
					       path,
					       errno_to_string ());
		return FALSE;
	}

	if (catalog->search_data != NULL) {
		SearchData *search_data = catalog->search_data;

		/* write search data. */

		if (! fprintf (f, SEARCH_HEADER)) {
			fclose (f);
			error_on_saving (path, gerror);
			return FALSE;
		}

		if (! fprintf (f, "\"%s\"\n", search_data->start_from)) {
			fclose (f);
			error_on_saving (path, gerror);
			return FALSE;
		}

		if (! fprintf (f, "\"%s\"\n", (search_data->recursive ? "TRUE" : "FALSE"))) {
			fclose (f);
			error_on_saving (path, gerror);
			return FALSE;
		}

		if (! fprintf (f, "\"%s\"\n", search_data->file_pattern)) {
			fclose (f);
			error_on_saving (path, gerror);
			return FALSE;
		}

		if (! fprintf (f, "\"%s\"\n", search_data->comment_pattern)) {
			fclose (f);
			error_on_saving (path, gerror);
			return FALSE;
		}

		if (! fprintf (f, "\"%s\"\n", search_data->place_pattern)) {
			fclose (f);
			error_on_saving (path, gerror);
			return FALSE;
		}

		if (! fprintf (f, "%d", catalog->search_data->all_keywords)) {
			fclose (f);
			error_on_saving (path, gerror);
			return FALSE;
		}

		if (! fprintf (f, "\"%s\"\n", search_data->keywords_pattern)) {
			fclose (f);
			return FALSE;
		}

		if (! fprintf (f, "%ld\n", search_data->date)) {
			fclose (f);
			error_on_saving (path, gerror);
			return FALSE;
		}

		if (! fprintf (f, "%d\n", catalog->search_data->date_scope)) {
			fclose (f);
			error_on_saving (path, gerror);
			return FALSE;
		}
	}

	/* write the file list. */

	for (scan = catalog->list; scan; scan = scan->next) 
		if (! fprintf (f, "\"%s\"\n", (gchar*) scan->data)) {
			fclose (f);
			error_on_saving (path, gerror);
			return FALSE;
		}
	
	fclose (f);

	return TRUE;
}


void
catalog_add_item (Catalog *catalog,
		  const char *file_path)
{
	g_return_if_fail (catalog != NULL);
	g_return_if_fail (file_path != NULL);	

	if (g_list_find (catalog->list, file_path) == NULL)
		catalog->list = g_list_prepend (catalog->list, 
						g_strdup (file_path));
}


void
catalog_remove_item (Catalog *catalog,
		     const char *file_path)
{
	GList *scan;

	g_return_if_fail (catalog != NULL);
	g_return_if_fail (file_path != NULL);	

	for (scan = catalog->list; scan; scan = scan->next)
		if (strcmp ((gchar*) scan->data, file_path) == 0)
			break;

	if (scan == NULL)
		return;

	catalog->list = g_list_remove_link (catalog->list, scan);

	g_free (scan->data);
	g_list_free (scan);
}


void
catalog_remove_all_items (Catalog *catalog)
{
	g_return_if_fail (catalog != NULL);

	g_list_foreach (catalog->list, (GFunc) g_free, NULL);
	g_list_free (catalog->list);
	catalog->list = NULL;
}

