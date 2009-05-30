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

#include <config.h>

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <glib/gi18n.h>

#include "catalog.h"
#include "typedefs.h"
#include "file-utils.h"
#include "gfile-utils.h"
#include "glib-utils.h"
#include "gthumb-error.h"


#define SORT_FIELD "# sort: "
#define MAX_LINE_LENGTH 4096

static char *sort_names[] = { "none", "name", "path", "size", "time", "exif-date", "comment", "manual" };


Catalog*
catalog_new ()
{
	Catalog *catalog;

	catalog = g_new (Catalog, 1);
	catalog->path = NULL;
	catalog->list = NULL;
	catalog->search_data = NULL;
	catalog->sort_method = GTH_SORT_METHOD_NONE;
	catalog->sort_type = GTK_SORT_ASCENDING;

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


static void
copy_unquoted (char *unquoted, char *line)
{
	int len = strlen (line);

	strncpy (unquoted, line + 1, len - 2);
	unquoted[len - 2] = 0;
}


gboolean
catalog_load_from_disk__common (Catalog     *catalog,
				const char  *uri,
				GError     **gerror,
				gboolean     load_file_list)
{
        GFileInputStream *istream;
        GDataInputStream *dstream;
        GError           *error = NULL;
        GFile            *gfile;
        char             *line;
        gsize             length;
	gboolean          file_list = FALSE;

	gfile = gfile_new (uri);
        istream = g_file_read (gfile, NULL, &error);

        if (error != NULL) {
                gfile_warning ("Cannot open catalog file for reading",
                               gfile,
                               error);
                g_propagate_error (gerror, error);
                return FALSE;
        }

        dstream = g_data_input_stream_new (G_INPUT_STREAM(istream));
	
	if (catalog->path != NULL)
		g_free (catalog->path);
	if (catalog->list != NULL)
		path_list_free (catalog->list);
	if (catalog->search_data != NULL)
		search_data_free (catalog->search_data);

	catalog->path = g_strdup (uri);
	catalog->list = NULL;
	catalog->search_data = NULL;

	/*
	 * Catalog Format : A list of quoted filenames, one name per line :
	 *     # sort: name      Optional: possible values: none, name, path, size, time, manual
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
	while ((line = g_data_input_stream_read_line (dstream, &length, NULL, &error))) {
		char *file_name;

		if ((*line == 0) || (error != NULL))
			continue;

		/* search data starts with SEARCH_HEADER */
		if (! file_list && strcmp (line, SEARCH_HEADER) == 0) {
			char     unquoted [MAX_LINE_LENGTH];
			time_t   date;
			int      date_scope;
			gboolean all_keywords = FALSE;
			int      line_ofs = 0;

			/* load search data. */

			catalog->search_data = search_data_new ();

			/* * start from */

			g_free (line);
			line = g_data_input_stream_read_line (dstream, &length, NULL, &error);
			if (error != NULL) continue;
			copy_unquoted (unquoted, line);
			search_data_set_start_from (catalog->search_data,
						    unquoted);

			/* * recursive */

			g_free (line);
                        line = g_data_input_stream_read_line (dstream, &length, NULL, &error);
			if (error != NULL) continue;
			copy_unquoted (unquoted, line);
			search_data_set_recursive (catalog->search_data, strcmp (unquoted, "TRUE") == 0);

			/* * file pattern */

			g_free (line);
                        line = g_data_input_stream_read_line (dstream, &length, NULL, &error);
			if (error != NULL) continue;
			copy_unquoted (unquoted, line);
			search_data_set_file_pattern (catalog->search_data,
						      unquoted);

			/* * comment pattern */

			g_free (line);
                        line = g_data_input_stream_read_line (dstream, &length, NULL, &error);
			if (error != NULL) continue;
			copy_unquoted (unquoted, line);
			search_data_set_comment_pattern (catalog->search_data,
							 unquoted);

			/* * place pattern */

			g_free (line);
                        line = g_data_input_stream_read_line (dstream, &length, NULL, &error);
			if (error != NULL) continue;
			copy_unquoted (unquoted, line);
			search_data_set_place_pattern (catalog->search_data,
						       unquoted);

			/* * keywords pattern */

			g_free (line);
                        line = g_data_input_stream_read_line (dstream, &length, NULL, &error);
			if (error != NULL) continue;
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

			g_free (line);
                        line = g_data_input_stream_read_line (dstream, &length, NULL, &error);
			if (error != NULL) continue;
			sscanf (line, "%ld", &date);
			search_data_set_date (catalog->search_data, date);

			/* * date scope */

			g_free (line);
                        line = g_data_input_stream_read_line (dstream, &length, NULL, &error);
			if (error != NULL) continue;
			sscanf (line, "%d", &date_scope);
			search_data_set_date_scope (catalog->search_data,
						    date_scope);

			g_free (line);
			continue;
		}

		if (! file_list && (strncmp (line, SORT_FIELD, strlen (SORT_FIELD)) == 0)) {
			char          *sort_type;
			GthSortMethod  sort_method = GTH_SORT_METHOD_NONE;
			int            i;

			sort_type = line + strlen (SORT_FIELD);
			sort_type[strlen (sort_type)] = 0;

			for (i = 0; i < G_N_ELEMENTS (sort_names); i++)
				if (strcmp (sort_type, sort_names[i]) == 0) {
					sort_method = i;
					break;
				}

			catalog->sort_method = sort_method;

			continue;
		}

		if (!load_file_list)
			break;

		file_list  = TRUE;
		file_name = g_strndup (line + 1, strlen (line) - 2);
		catalog->list = g_list_prepend (catalog->list, file_name);
	}

        g_object_unref (dstream);
        g_object_unref (istream);
        g_object_unref (gfile);
	if (error != NULL) {
		g_propagate_error (gerror, error);
	}

	catalog->list = g_list_reverse (catalog->list);

	return TRUE;
}


gboolean
catalog_load_from_disk (Catalog     *catalog,
			const char  *uri,
			GError     **gerror)
{
	return catalog_load_from_disk__common (catalog, uri, gerror, TRUE);
}


gboolean
catalog_load_search_data_from_disk (Catalog     *catalog,
				    const char  *uri,
				    GError     **gerror)
{
	return catalog_load_from_disk__common (catalog, uri, gerror, FALSE);
}


gboolean
catalog_write_to_disk (Catalog     *catalog,
		       GError     **gerror)
{
        GFileOutputStream *ostream;
        GError            *error = NULL;
        GFile             *gfile;
	GList             *scan;

	g_return_val_if_fail (catalog != NULL, FALSE);
	g_return_val_if_fail (catalog->path != NULL, FALSE);

	gfile = gfile_new (catalog->path);
        ostream = g_file_replace (gfile, NULL, FALSE, G_FILE_CREATE_NONE, NULL, &error);

        if (error) {
                gfile_warning ("Cannot open catalog file for writing",
                               gfile,
                               error);
                g_propagate_error (gerror, error);
                return FALSE;
        }

	if (catalog->search_data != NULL) {
		SearchData *search_data = catalog->search_data;

		/* write search data. */
		if (!error) gfile_output_stream_write_line (ostream,
							    &error,
							    SEARCH_HEADER);
		if (!error) gfile_output_stream_write_line (ostream,
                                                            &error,
							    "\"%s\"",
							    search_data->start_from);
		if (!error) gfile_output_stream_write_line (ostream,
                                                            &error,
							    "\"%s\"",
							   (search_data->recursive ? "TRUE" : "FALSE"));
		if (!error) gfile_output_stream_write_line (ostream,
                                                            &error,
                                                            "\"%s\"",
                                                            search_data->file_pattern);
		if (!error) gfile_output_stream_write_line (ostream,
                                                            &error,
                                                            "\"%s\"",
                                                            search_data->comment_pattern);
		if (!error) gfile_output_stream_write_line (ostream,
                                                            &error,
                                                            "\"%s\"",
                                                            search_data->place_pattern);
		if (!error) gfile_output_stream_write_line (ostream,
                                                            &error,
							    "%d\"%s\"",
							    catalog->search_data->all_keywords,
                                                            search_data->keywords_pattern);
		if (!error) gfile_output_stream_write_line (ostream,
                                                            &error,
                                                            "%ld",
                                                            search_data->date);
		if (!error) gfile_output_stream_write_line (ostream,
                                                            &error,
                                                            "%d",
                                                            search_data->date_scope);
	}

	/* sort method */

	if (!error) gfile_output_stream_write_line (ostream,
						    &error,
						    "%s%s",
						    SORT_FIELD,
						    sort_names[catalog->sort_method]);

	/* write the file list. */

	for (scan = catalog->list; scan; scan = scan->next)
		if (!error) gfile_output_stream_write_line (ostream,
							    &error,
							    "\"%s\"",
							    (char*) scan->data);

	g_output_stream_close (G_OUTPUT_STREAM(ostream), NULL, &error);
	g_object_unref (ostream);

	if (error) {
                gfile_warning ("Cannot write data to catalog file",
                               gfile,
                               error);
		g_object_unref (gfile);
		g_propagate_error (gerror, error);
		return FALSE;
	} else {
		g_object_unref (gfile);
		return TRUE;
	}
}


void
catalog_add_item (Catalog *catalog,
		  const char *file_path)
{
	g_return_if_fail (catalog != NULL);
	g_return_if_fail (file_path != NULL);

	if (g_list_find_custom (catalog->list,
				file_path,
				(GCompareFunc) uricmp) == NULL)
		catalog->list = g_list_prepend (catalog->list,
						g_strdup (file_path));
}


void
catalog_insert_items (Catalog *catalog,
		      GList   *list,
		      int      pos)
{
	g_return_if_fail (catalog != NULL);
	catalog->list = _g_list_insert_list_before (catalog->list,
						    g_list_nth (catalog->list, pos),
						    list);
}


int
catalog_remove_item (Catalog *catalog,
		     const char *file_path)
{
	GList *scan;
	int    i = 0;

	g_return_val_if_fail (catalog != NULL, -1);
	g_return_val_if_fail (file_path != NULL, -1);

	for (scan = catalog->list; scan; scan = scan->next, i++)
		if (same_uri ((char*) scan->data, file_path))
			break;

	if (scan == NULL)
		return -1;

	catalog->list = g_list_remove_link (catalog->list, scan);

	g_free (scan->data);
	g_list_free (scan);

	return i;
}


/* -- catalog_get_file_data_list -- */


void
catalog_get_file_data_list (Catalog         *catalog,
			    CatalogDoneFunc  done_func,
			    gpointer         done_data)
{
	GList    *list = NULL;
	GList    *scan;
	
	/* FIXME: make this function async */
	
	for (scan = catalog->list; scan; scan = scan->next) {
		char     *path = scan->data;
		FileData *fd;
		
		fd = file_data_new (path);
		file_data_update (fd);  /* FIXME: when to update the mime-type */
		if (file_filter (fd, TRUE, FALSE))
			list = g_list_prepend (list, fd);
	}
	list = g_list_reverse (list);
	
	if (done_func) 
		(done_func) (catalog, list, done_data);
	
	file_data_list_free (list);
}
