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
#include <sys/stat.h>   
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include <ctype.h>

#include <glib/gi18n.h>
#include <libgnomevfs/gnome-vfs-types.h>
#include <libgnomevfs/gnome-vfs-uri.h>
#include <libgnomevfs/gnome-vfs-utils.h>
#include <libxml/tree.h>
#include <libxml/parser.h>
#include <libxml/xmlmemory.h>
#include <gtk/gtk.h>

#ifdef HAVE_LIBIPTCDATA
#include <libiptcdata/iptc-data.h>
#include <libiptcdata/iptc-jpeg.h>
#endif /* HAVE_LIBIPTCDATA */

#include "typedefs.h"
#include "comments.h"
#include "file-utils.h"
#include "glib-utils.h"
#include "gtk-utils.h"

#define COMMENT_TAG  ((xmlChar *)"Comment")
#define PLACE_TAG    ((xmlChar *)"Place")
#define TIME_TAG     ((xmlChar *)"Time")
#define NOTE_TAG     ((xmlChar *)"Note")
#define KEYWORDS_TAG ((xmlChar *)"Keywords")
#define FORMAT_TAG   ((xmlChar *)"format")
#define FORMAT_VER   ((xmlChar *)"2.0")


/* version 1.0 save comments in local format */
/* version 2.0 save comments in utf8 format */


CommentData *
comment_data_new (void)
{
	CommentData *data;

	data = g_new0 (CommentData, 1);

	data->place = NULL;
	data->time = 0;
	data->comment = NULL;
	data->keywords_n = 0;
	data->keywords = NULL;
	data->utf8_format = TRUE;

#ifdef HAVE_LIBIPTCDATA
	data->iptc_data = NULL;
#endif /* HAVE_LIBIPTCDATA */

	data->changed = FALSE;

	return data;
}


void
comment_data_free_keywords (CommentData *data)
{
	if (data->keywords != NULL) {
		int i;
		for (i = 0; i < data->keywords_n; i++)
			g_free (data->keywords[i]);
		g_free (data->keywords);
		data->keywords = NULL;
		data->keywords_n = 0;
	}
}


void
comment_data_free_comment (CommentData *data)
{
        if (data == NULL)
                return;

        if (data->place != NULL) {
                g_free (data->place);
		data->place = NULL;
	}

        if (data->comment != NULL) {
                g_free (data->comment);
		data->comment = NULL;
	}

	data->time = 0;
}


void
comment_data_free (CommentData *data)
{
        if (data == NULL)
                return;

	comment_data_free_comment (data);
	comment_data_free_keywords (data);

#ifdef HAVE_LIBIPTCDATA
	if (data->iptc_data != NULL)
		iptc_data_unref (data->iptc_data);
#endif /* HAVE_LIBIPTCDATA */

        g_free (data);
}


CommentData *
comment_data_dup (CommentData *data)
{
	CommentData *new_data;

	if (data == NULL)
		return NULL;

	new_data = comment_data_new ();

	if (data->place != NULL)
		new_data->place = g_strdup (data->place);
	new_data->time = data->time;
	if (data->comment != NULL)
		new_data->comment = g_strdup (data->comment);

	if (data->keywords != NULL) {
		int i;

		new_data->keywords = g_new0 (char*, data->keywords_n + 1);
		new_data->keywords_n = data->keywords_n;

		for (i = 0; i < data->keywords_n; i++) 
			new_data->keywords[i] = g_strdup (data->keywords[i]);
		new_data->keywords[i] = NULL;
	}
	new_data->utf8_format = data->utf8_format;

#ifdef HAVE_LIBIPTCDATA
	new_data->iptc_data = data->iptc_data;
	if (new_data->iptc_data != NULL)
		iptc_data_ref (new_data->iptc_data);
#endif /* HAVE_LIBIPTCDATA */

	return new_data;
}


gboolean
comment_data_equal (CommentData *data1, 
		    CommentData *data2)
{
	int i;

	if ((data1 == NULL) && (data2 == NULL))
		return TRUE;
	if ((data1 == NULL) || (data2 == NULL))
		return FALSE;

	if (strcmp_null_tollerant (data1->place, data2->place) != 0)
		return FALSE;
	if (data1->time != data2->time)
		return FALSE;
	if (strcmp_null_tollerant (data1->comment, data2->comment) != 0)
		return FALSE;
	if (data1->keywords_n != data2->keywords_n)
		return FALSE;
	for (i = 0; i < data1->keywords_n; i++)
		if (strcmp_null_tollerant (data1->keywords[i], 
					   data2->keywords[i]) != 0)
			return FALSE;

	return TRUE;
}


char *
comments_get_comment_filename__old (const char *source,
				    gboolean    resolve_symlinks,
				    gboolean    unescape) 
{
	char        *source_real = NULL;
	char        *directory = NULL;
	const char  *filename = NULL;
	char        *path = NULL;

	if (source == NULL) 
		return NULL;

	source_real = g_strdup (source);

	if (resolve_symlinks) {
		char           *resolved = NULL;
		GnomeVFSResult  result;

		result = resolve_all_symlinks (source_real, &resolved);

		if (result == GNOME_VFS_OK) { 
			g_free (source_real);
			source_real = resolved;
		} else
			g_free (resolved);
	}

	directory = remove_level_from_path (source_real);
	filename = file_name_from_path (source_real);

	path = g_strconcat (g_get_home_dir(), 
			    "/", 
			    RC_COMMENTS_DIR, 
			    directory,
			    "/",
			    filename, 
			    COMMENT_EXT, 
			    NULL);

	if (!unescape) {
		char *escaped_path = escape_uri (path);
		g_free (path);
		path = escaped_path;
	}

	g_free (directory);
	g_free (source_real);

	return path;
}


char *
comments_get_comment_dir__old (const char *directory,
			       gboolean    resolve_symlinks,
			       gboolean    unescape) 
{
	char        *directory_resolved = NULL;
	const char  *directory_real = directory;
	const char  *separator;
	char        *path;

	if (resolve_symlinks && (directory != NULL)) {
		GnomeVFSResult  result;
		result = resolve_all_symlinks (directory, &directory_resolved);
		if (result == GNOME_VFS_OK) 
			directory_real = directory_resolved;
	}

	if (directory_real == NULL)
		separator = NULL;
	else
		separator = (directory_real[0] == '/') ? "" : "/";

	path = g_strconcat (g_get_home_dir(), 
			    "/", 
			    RC_COMMENTS_DIR, 
			    separator,
			    directory_real,
			    NULL);

	g_free (directory_resolved);

	if (unescape) {
		char *unesc_path = gnome_vfs_unescape_string (path, NULL);
		g_free (path);
		path = unesc_path;
	}

	return path;
}


char *
comments_get_comment_filename (const char *source,
			       gboolean    resolve_symlinks,
			       gboolean    unescape) 
{
	char  *source_real = NULL;
	char  *directory = NULL;
	char  *filename = NULL;
	char  *path = NULL;

	if (source == NULL) 
		return NULL;

	source_real = g_strdup (source);

	if (resolve_symlinks) {
		char           *resolved = NULL;
		GnomeVFSResult  result;

		result = resolve_all_symlinks (source_real, &resolved);

		if (result == GNOME_VFS_OK) { 
			g_free (source_real);
			source_real = resolved;
		} else
			g_free (resolved);
	}

	directory = remove_level_from_path (source_real);
	filename = g_strconcat (file_name_from_path (source_real), COMMENT_EXT, NULL);

	path = g_build_filename (directory, ".comments", filename, NULL);

	if (!unescape) {
		char *escaped_path = escape_uri (path);
		g_free (path);
		path = escaped_path;
	}

	g_free (directory);
	g_free (filename);
	g_free (source_real);

	return path;
}


#ifdef HAVE_LIBIPTCDATA


static CommentData *
load_comment_from_iptc (const char *filename)
{
	CommentData *data;
	IptcData    *d;
	struct tm    t;
	int          i;
	int          got_date = 0, got_time = 0;

	if (filename == NULL)
		return NULL;

	d = iptc_data_new_from_jpeg (filename);
	if (!d)
		return NULL;

	data = comment_data_new ();

	bzero (&t, sizeof (t));
	t.tm_isdst = -1;

	for (i = 0; i < d->count; i++) {
		IptcDataSet * ds = d->datasets[i];

		if ((ds->record == IPTC_RECORD_APP_2) 
		    && (ds->tag == IPTC_TAG_CAPTION)) {
			if (data->comment)
				continue;
			data->comment = g_new (char, ds->size + 1);
			if (data->comment)
				iptc_dataset_get_data (ds, (guchar*)data->comment, ds->size + 1);
		}
		else if ((ds->record == IPTC_RECORD_APP_2) 
			 && (ds->tag == IPTC_TAG_CONTENT_LOC_NAME)) {
			if (data->place)
				continue;
			data->place = g_new (char, ds->size + 1);
			if (data->place)
				iptc_dataset_get_data (ds, (guchar*)data->place, ds->size + 1);
		}
		else if ((ds->record == IPTC_RECORD_APP_2)
			 && (ds->tag == IPTC_TAG_KEYWORDS)) {
			char keyword[64];
			if (iptc_dataset_get_data (ds, (guchar*)keyword, sizeof(keyword)) < 0)
				continue;
			comment_data_add_keyword (data, keyword);
		}
		else if ((ds->record == IPTC_RECORD_APP_2) 
			 && (ds->tag == IPTC_TAG_DATE_CREATED)) {
			int year, month;
			iptc_dataset_get_date (ds, &year, &month, &t.tm_mday);
			t.tm_year = year - 1900;
			t.tm_mon = month - 1;
			got_date = 1;
		}
		else if ((ds->record == IPTC_RECORD_APP_2) 
			 && (ds->tag == IPTC_TAG_TIME_CREATED)) {
			iptc_dataset_get_time (ds, &t.tm_hour, &t.tm_min, &t.tm_sec, NULL);
			got_time = 1;
		}
	}

	if (got_date && got_time)
		data->time = mktime (&t);

	data->iptc_data = d;

	return data;
}


static void
clear_iptc_comment (IptcData *d)
{
	IptcTag deletetag[] = {
		IPTC_TAG_DATE_CREATED, IPTC_TAG_TIME_CREATED, IPTC_TAG_KEYWORDS,
		IPTC_TAG_CAPTION, IPTC_TAG_CONTENT_LOC_NAME, 0
	};
	int          i;

	for (i = 0; deletetag[i] != 0; i++) {
		IptcDataSet *ds;
		while ((ds = iptc_data_get_dataset (d, 
						    IPTC_RECORD_APP_2,
						    deletetag[i]))) {
			iptc_data_remove_dataset (d, ds);
			iptc_dataset_unref (ds);
		}
	}
}


static void
save_iptc_data (const char *filename, 
		IptcData   *d)
{
	guint        buf_len = 256 * 256;
	FILE        *infile, *outfile;
	guchar      *ps3_buf;
	guchar      *ps3_out_buf;
	guchar      *iptc_buf;
	guint        ps3_len, iptc_len;
	gchar       *tmpfile;
	struct stat  statinfo;

	if (filename == NULL)
		return;

	ps3_buf = g_malloc (buf_len);
	if (!ps3_buf)
		return;

	ps3_out_buf = g_malloc (buf_len);
	if (!ps3_out_buf)
		goto abort1;

	infile = fopen (filename, "r");
	if (!infile)
		goto abort2;

	ps3_len = iptc_jpeg_read_ps3 (infile, ps3_buf, buf_len);
	if (ps3_len < 0)
		goto abort3;

	if (iptc_data_save (d, &iptc_buf, &iptc_len) < 0)
		goto abort3;

	ps3_len = iptc_jpeg_ps3_save_iptc (ps3_buf, ps3_len, 
					   iptc_buf, iptc_len, 
					   ps3_out_buf, buf_len);
	iptc_data_free_buf (d, iptc_buf);
	if (ps3_len < 0)
		goto abort3;

	rewind (infile);

	tmpfile = g_strdup_printf ("%s.%d", filename, getpid());
	if (!tmpfile)
		goto abort3;

	outfile = fopen (tmpfile, "w");
	if (!outfile)
		goto abort4;
	
	if (iptc_jpeg_save_with_ps3 (infile, outfile, (guchar*)ps3_out_buf, ps3_len) < 0)
		goto abort5;

	fclose (outfile);
	fclose (infile);

	stat (filename, &statinfo);

	if (rename (tmpfile, filename) < 0)
		file_unlink (tmpfile);
	else {
		chown (filename, -1, statinfo.st_gid);
		chmod (filename, statinfo.st_mode);
	}

	g_free (tmpfile);
	g_free (ps3_out_buf);
	g_free (ps3_buf);

	return;
abort5:
	fclose (outfile);
	file_unlink (tmpfile);
abort4:
	g_free (tmpfile);
abort3:
	fclose (infile);
abort2:
	g_free (ps3_out_buf);
abort1:
	g_free (ps3_buf);
}


static void
save_comment_iptc (const char  *filename,
		   CommentData *data)
{
	IptcData    *d;
	IptcDataSet *ds;
	time_t       mtime;
	int          i;

	mtime = get_file_mtime (filename);

	d = iptc_data_new_from_jpeg (filename);
	if (d) {
		clear_iptc_comment (d);
	}
	else {
		d = iptc_data_new ();
		if (!d)
			return;
	}

	if (data->time > 0) {
		struct tm t;
		localtime_r (&data->time, &t);
		
		ds = iptc_dataset_new ();
		if (ds) {
			iptc_dataset_set_tag (ds, IPTC_RECORD_APP_2,
					IPTC_TAG_DATE_CREATED);
			iptc_dataset_set_date (ds, t.tm_year + 1900, t.tm_mon + 1,
					t.tm_mday, IPTC_DONT_VALIDATE);
			iptc_data_add_dataset (d, ds);
			iptc_dataset_unref (ds);
		}

		ds = iptc_dataset_new ();
		if (ds) {
			iptc_dataset_set_tag (ds, IPTC_RECORD_APP_2,
					IPTC_TAG_TIME_CREATED);
			iptc_dataset_set_time (ds, t.tm_hour, t.tm_min, t.tm_sec,
					0, IPTC_DONT_VALIDATE);
			iptc_data_add_dataset (d, ds);
			iptc_dataset_unref (ds);
		}
	}

	for (i = 0; i < data->keywords_n; i++) {
		ds = iptc_dataset_new ();
		if (ds) {
			iptc_dataset_set_tag (ds, IPTC_RECORD_APP_2,
					      IPTC_TAG_KEYWORDS);
			iptc_dataset_set_data (ds, (guchar*)data->keywords[i],
					       strlen (data->keywords[i]), IPTC_DONT_VALIDATE);
			iptc_data_add_dataset (d, ds);
			iptc_dataset_unref (ds);
		}
	}

	if (data->comment != NULL && data->comment[0]) {
		ds = iptc_dataset_new ();
		if (ds) {
			iptc_dataset_set_tag (ds, IPTC_RECORD_APP_2,
					IPTC_TAG_CAPTION);
			iptc_dataset_set_data (ds, (guchar*)data->comment,
					       strlen (data->comment), IPTC_DONT_VALIDATE);
			iptc_data_add_dataset (d, ds);
			iptc_dataset_unref (ds);
		}
	}

	if (data->place != NULL && data->place[0]) {
		ds = iptc_dataset_new ();
		if (ds) {
			iptc_dataset_set_tag (ds, IPTC_RECORD_APP_2,
					      IPTC_TAG_CONTENT_LOC_NAME);
			iptc_dataset_set_data (ds, (guchar*)data->place,
					       strlen (data->place), IPTC_DONT_VALIDATE);
			iptc_data_add_dataset (d, ds);
			iptc_dataset_unref (ds);
		}
	}

	iptc_data_set_version (d, IPTC_IIM_VERSION);
	iptc_data_set_encoding_utf8 (d);
	iptc_data_sort (d);

	save_iptc_data (filename, d);
	set_file_mtime (filename, mtime);

	iptc_data_unref (d);
}


static void
delete_comment_iptc (const char  *filename)
{
	IptcData *d;
	time_t    mtime;

	mtime = get_file_mtime (filename);

	d = iptc_data_new_from_jpeg (filename);
	if (!d)
		return;

	clear_iptc_comment (d);
	save_iptc_data (filename, d);
	set_file_mtime (filename, mtime);

	iptc_data_unref (d);
}


#endif /* HAVE_LIBIPTCDATA */


void
comment_copy (const char *src,
	      const char *dest)
{
	char *comment_src = NULL;
	char *comment_dest = NULL;

	comment_src = comments_get_comment_filename (src, TRUE, TRUE);
	if (! path_is_file (comment_src)) {
		g_free (comment_src);
		return;
	}

	comment_dest = comments_get_comment_filename (dest, TRUE, TRUE);
	if (path_is_file (comment_dest)) 
		file_unlink (comment_dest);

	file_copy (comment_src, comment_dest);

	g_free (comment_src);
	g_free (comment_dest);
}


void
comment_move (const char *src,
	      const char *dest)
{
	char *comment_src = NULL;
	char *comment_dest = NULL;

	comment_src = comments_get_comment_filename (src, TRUE, TRUE);
	if (! path_is_file (comment_src)) {
		g_free (comment_src);
		return;
	}

	comment_dest = comments_get_comment_filename (dest, TRUE, TRUE);
	if (path_is_file (comment_dest)) 
		file_unlink (comment_dest);

	file_move (comment_src, comment_dest);

	g_free (comment_src);
	g_free (comment_dest);
}


void
comment_delete (const char *filename)
{
	char *comment_name;

	comment_name = comments_get_comment_filename (filename, TRUE, TRUE);
	file_unlink (comment_name);
	g_free (comment_name);

#ifdef HAVE_LIBIPTCDATA
	if (image_is_jpeg (filename)) 
		delete_comment_iptc (filename);
#endif /* HAVE_LIBIPTCDATA */
}


static char *
get_utf8_text (CommentData *data,
	       char        *value)
{
	if (value == NULL)
		return NULL;

	if (data->utf8_format) 
		return g_strdup (value);
	else
		return g_locale_to_utf8 (value, -1, NULL, NULL, NULL);
}


static void
get_keywords (CommentData *data,
	      char        *utf8_value)
              
{
	char     *value;
        int       n;
        char     *p, *keyword;
        gboolean  done;

	if ((utf8_value == NULL) || (*utf8_value == 0)) {
		data->keywords_n = 0;
		data->keywords = NULL;
		return;
	}

	value = get_utf8_text (data, utf8_value);

	if (value == NULL) {
		data->keywords_n = 0;
		data->keywords = NULL;
		return;
	}

	n = 1;
	for (p = value; *p != 0; p = g_utf8_next_char (p)) {
		gunichar ch = g_utf8_get_char (p);
		if (ch == ',')
			n++;
	}

	data->keywords_n = n;
	data->keywords = g_new0 (char*, n + 1);
	data->keywords[n] = NULL;

        n = 0;
	keyword = p = value;
	do {
		gunichar ch = g_utf8_get_char (p);

		done = (*p == 0); 

		if ((ch == ',') || (*p == 0)) {
			data->keywords[n++] = g_strndup (keyword, p - keyword);
			keyword = p = g_utf8_next_char (p);
		} else
			p = g_utf8_next_char (p);
	} while (! done);

	g_free (value);
}


static CommentData *
load_comment_from_xml (const char *filename)
{
	CommentData *data;
	char        *comment_file;
	xmlDocPtr    doc;
        xmlNodePtr   root, node;
        xmlChar     *value;
	xmlChar     *format;

	if (filename == NULL)
		return NULL;

	comment_file = comments_get_comment_filename (filename, TRUE, TRUE);
	if (! path_is_file (comment_file)) {
		g_free (comment_file);
		return NULL;
	}

        doc = xmlParseFile (comment_file);
	if (doc == NULL) {
		g_free (comment_file);
		return NULL;
	}

	data = comment_data_new ();

        root = xmlDocGetRootElement (doc);
        node = root->xmlChildrenNode;

	format = xmlGetProp (root, FORMAT_TAG);
	if (strcmp ((char*)format, "1.0") == 0)
		data->utf8_format = FALSE;
	else
		data->utf8_format = TRUE;

	for (; node; node = node->next) {
		const char *name = (char*) node->name;

                value = xmlNodeListGetString (doc, node->xmlChildrenNode, 1);

                if (strcmp (name, (char*)PLACE_TAG) == 0) 
			data->place = get_utf8_text (data, (char*)value);
		else if (strcmp (name, (char*)NOTE_TAG) == 0) 
			data->comment = get_utf8_text (data, (char*)value);
		else if (strcmp (name, (char*)KEYWORDS_TAG) == 0) 
			get_keywords (data, (char*)value);
		else if (strcmp (name, (char*)TIME_TAG) == 0) {
			if (value != NULL) 
				data->time = atol ((char*)value);
		}
		
		if (value)
			xmlFree (value);
        }

	xmlFree (format);

        xmlFreeDoc (doc);
	g_free (comment_file);

	return data;	
}


void
save_comment (const char  *filename,
	      CommentData *data,
	      gboolean     save_embedded)
{
	xmlDocPtr    doc;
        xmlNodePtr   tree, subtree;
	char        *comment_file = NULL;
	char        *time_str = NULL;
	char        *keywords_str = NULL;
	char        *dest_dir = NULL;
	char        *e_comment = NULL, *e_place = NULL, *e_keywords = NULL;

	if (save_embedded) {
#ifdef HAVE_LIBIPTCDATA
		if (image_is_jpeg (filename)) 
			save_comment_iptc (get_file_path_from_uri (filename), data);
#endif /* HAVE_LIBIPTCDATA */
	}

	if (comment_data_is_void (data)) {
		comment_delete (filename);
		return;
	}

	/* Convert data to strings. */

	time_str = g_strdup_printf ("%ld", data->time);
	if (data->keywords_n > 0) {
		if (data->keywords_n == 1)
			keywords_str = g_strdup (data->keywords[0]);
		else
			keywords_str = g_strjoinv (",", data->keywords);
	} else
		keywords_str = g_strdup ("");

	/* Escape text */

	if (data->comment != NULL)
		e_comment = g_markup_escape_text (data->comment, -1);

	if (data->place != NULL)
		e_place = g_markup_escape_text (data->place, -1);

	if (keywords_str != NULL)
		e_keywords = g_markup_escape_text (keywords_str, -1);
	g_free (keywords_str);

	/* Create the xml tree. */

	doc = xmlNewDoc ((xmlChar*)"1.0");

	doc->xmlRootNode = xmlNewDocNode (doc, NULL, COMMENT_TAG, NULL); 
	xmlSetProp (doc->xmlRootNode, FORMAT_TAG, FORMAT_VER);

	tree = doc->xmlRootNode;
	subtree = xmlNewChild (tree, NULL, PLACE_TAG,    (xmlChar*)e_place);
	subtree = xmlNewChild (tree, NULL, TIME_TAG,     (xmlChar*)time_str);
	subtree = xmlNewChild (tree, NULL, NOTE_TAG,     (xmlChar*)e_comment);
	subtree = xmlNewChild (tree, NULL, KEYWORDS_TAG, (xmlChar*)e_keywords);

	g_free (e_place);
	g_free (time_str);
	g_free (e_comment);
	g_free (e_keywords);

	/* Write to disk. */

	comment_file = comments_get_comment_filename (filename, TRUE, TRUE);

	dest_dir = remove_level_from_path (comment_file);

	if (ensure_dir_exists (dest_dir, 0700)) {
		xmlSetDocCompressMode (doc, 3);
		xmlSaveFile (comment_file, doc);
	}
	g_free (dest_dir);
	g_free (comment_file);

        xmlFreeDoc (doc);
}


CommentData *
comments_load_comment (const char *filename,
		       gboolean    try_embedded)
{
	CommentData *xml_comment = NULL, *img_comment = NULL;

	if (filename == NULL)
		return NULL;

	xml_comment = load_comment_from_xml (filename);

	if (try_embedded) {
#ifdef HAVE_LIBIPTCDATA
		if (image_is_jpeg (filename)) 
			img_comment = load_comment_from_iptc (get_file_path_from_uri (filename));
		if (img_comment != NULL) {
			if (xml_comment == NULL)
				xml_comment = comment_data_new ();
			xml_comment->iptc_data = img_comment->iptc_data;
			if (xml_comment->iptc_data != NULL)
				iptc_data_ref (xml_comment->iptc_data);
		}
#endif /* HAVE_LIBIPTCDATA */
		if ((img_comment != NULL)
		    && (! comment_data_equal (xml_comment, img_comment))) {
			/* Consider the image comment more up-to-date and 
			 * sync the xml comment with it. */
			save_comment (filename, img_comment, FALSE);
			comment_data_free (xml_comment);
			xml_comment = img_comment;
			xml_comment->changed = TRUE;
		} else 
			comment_data_free (img_comment);
	}

	return xml_comment;
}


void
comments_save_comment (const char  *filename,
		       CommentData *data)
{
	CommentData *new_data;

	new_data = comments_load_comment (filename, FALSE);

	if ((new_data == NULL) && (data != NULL)) {
		CommentData *data_without_categories;

		data_without_categories = comment_data_dup (data);
		comment_data_free_keywords (data_without_categories);
		save_comment (filename, data_without_categories, TRUE);
		comment_data_free (data_without_categories);

		return;
	}
	comment_data_free_comment (new_data);

	if (data != NULL) {
		if (data->place != NULL) 
			new_data->place = g_strdup (data->place);
		if (data->time >= 0)
			new_data->time = data->time;
		if (data->comment != NULL) 
			new_data->comment = g_strdup (data->comment);
	}

	save_comment (filename, new_data, TRUE);
	comment_data_free (new_data);
}


void
comments_save_comment_non_null (const char  *filename,
				CommentData *data)
{
	CommentData *new_data;

	new_data = comments_load_comment (filename, TRUE);
	if (new_data == NULL) {
		comments_save_comment (filename, data);
		return;
	}

	/* Set non null fields. */

	if (data->place != NULL) {
		if (new_data->place != NULL) 
			g_free (new_data->place);
		new_data->place = g_strdup (data->place);
	}
	
	if (data->time >= 0)
		new_data->time = data->time;

	if (data->comment != NULL) {
		if (new_data->comment != NULL) 
			g_free (new_data->comment);
		new_data->comment = g_strdup (data->comment);
	}

	comments_save_comment (filename, new_data);
	comment_data_free (new_data);
}


void
comments_save_categories (const char  *filename,
			  CommentData *data)
{
	CommentData *new_data;

	new_data = comments_load_comment (filename, TRUE);

	if (new_data == NULL) {
		CommentData *data_without_comment;

		data_without_comment = comment_data_dup (data);
		comment_data_free_comment (data_without_comment);
		save_comment (filename, data_without_comment, TRUE);
		comment_data_free (data_without_comment);

		return;
	}

	comment_data_free_keywords (new_data);
	if (data->keywords != NULL) {
		int i;

		new_data->keywords = g_new0 (char*, data->keywords_n + 1);
		new_data->keywords_n = data->keywords_n;

		for (i = 0; i < data->keywords_n; i++) {
			char *k = g_strdup (data->keywords[i]);
			new_data->keywords[i] = k;
		}
		new_data->keywords[i] = NULL;
	}

	save_comment (filename, new_data, TRUE);
	comment_data_free (new_data);
}


void
comment_data_remove_keyword (CommentData *data,
			     const char  *keyword)
{
	gboolean  found = FALSE;
	int       i;

	if ((data->keywords == NULL) 
	    || (data->keywords_n == 0)
	    || (keyword == NULL))
		return;

	for (i = 0; i < data->keywords_n; i++) 
		if (g_utf8_collate (data->keywords[i], keyword) == 0) {
			found = TRUE;
			break;
		}

	if (!found)
		return;

	g_free (data->keywords[i]);
	for (; i < data->keywords_n - 1; i++)
		data->keywords[i] = data->keywords[i + 1];
	data->keywords[i] = NULL;

	data->keywords_n--;
	data->keywords = g_realloc (data->keywords, 
				    sizeof (char*) * (data->keywords_n + 1));

	if (data->keywords_n == 0) {
		g_free (data->keywords);
		data->keywords = NULL;
	}
}


void
comment_data_add_keyword (CommentData *data,
			  const char  *keyword)
{
	int i;

	if (keyword == NULL)
		return;

 	for (i = 0; i < data->keywords_n; i++) 
		if (g_utf8_collate (data->keywords[i], keyword) == 0) 
			return;
	
	data->keywords_n++;
	data->keywords = g_realloc (data->keywords, 
				    sizeof (char*) * (data->keywords_n + 1));
	data->keywords[data->keywords_n - 1] = g_strdup (keyword);
	data->keywords[data->keywords_n] = NULL;
}


gboolean
comment_data_is_void (CommentData *data)
{
	if (data == NULL)
		return TRUE;

	if ((data->place != NULL) && (*data->place != 0))
		return FALSE;
	if (data->time > 0)
		return FALSE;
	if ((data->comment != NULL) && (*data->comment != 0))
		return FALSE;
	if (data->keywords_n > 0)
		return FALSE;

	return TRUE;
}


gboolean
comment_text_is_void (CommentData *data)
{
	if (data == NULL)
		return TRUE;

	if ((data->place != NULL) && (*data->place != 0))
		return FALSE;
	if (data->time > 0)
		return FALSE;
	if ((data->comment != NULL) && (*data->comment != 0))
		return FALSE;

	return TRUE;
}


/* based on glib/glib/gmarkup.c (Copyright 2000, 2003 Red Hat, Inc.)  
 * This version does not escape ' and ''. Needed  because IE does not recognize
 * &apos; and &quot; */
static void
_append_escaped_text_for_html (GString     *str,
			       const gchar *text,
			       gssize       length)
{
	const gchar *p;
	const gchar *end;
	gunichar     ch;
	int          state = 0;

	p = text;
	end = text + length;
	
	while (p != end) {
		const gchar *next;
		next = g_utf8_next_char (p);
		ch = g_utf8_get_char (p);
		
		switch (state) {
		    case 1: /* escaped */
			if ((ch > 127) ||  !isprint((char)ch)) 
				g_string_append_printf (str, "\\&#%d;", ch);
			else
				g_string_append_unichar (str, ch);			
			state = 0;
			break;
			
		    default: /* not escaped */
			switch (*p) {
			    case '\\':
				state = 1; /* next character is escaped */
				break;
			
			    case '&':
				g_string_append (str, "&amp;");
				break;
			
			    case '<':
				g_string_append (str, "&lt;");
				break;
			
			    case '>':
				g_string_append (str, "&gt;");
				break;
			
			    case '\n':
				g_string_append (str, "<br />");
				break;

			    default:
				if ((ch > 127) ||  !isprint((char)ch)) 
					g_string_append_printf (str, "&#%d;", ch);
				else
					g_string_append_unichar (str, ch);			
				state = 0;
				break;
			}
		}
		
		p = next;
	}
}


char*
_g_escape_text_for_html (const gchar *text,
			 gssize       length)
{
	GString *str;

	g_return_val_if_fail (text != NULL, NULL);
	
	if (length < 0)
		length = strlen (text);

	/* prealloc at least as long as original text */
	str = g_string_sized_new (length);
	_append_escaped_text_for_html (str, text, length);

	return g_string_free (str, FALSE);
}


static void
_string_append (GString    *str,
		const char *a,
		gboolean    markup_escape) 
{
	if (a == NULL)
		return;

 	if (markup_escape)
		_append_escaped_text_for_html (str, a, strlen (a));
	else 
		g_string_append (str, a);
}


/* Note: separators are not escaped */
static char *
_get_comment_as_string_common (CommentData *data, 
			       char        *sep1, 
			       char        *sep2,
			       gboolean     markup_escape)
{
	char      *as_string = NULL;
	char       time_txt[50] = "";
	char      *utf8_time_txt = NULL;
	struct tm *tm;

	if (data == NULL)
		return NULL;
 
	if (data->time != 0) {
		tm = localtime (& data->time);
		if (tm->tm_hour + tm->tm_min + tm->tm_sec == 0)
			strftime (time_txt, 50, _("%d %B %Y"), tm);
		else
			strftime (time_txt, 50, _("%d %B %Y, %H:%M"), tm);
		utf8_time_txt = g_locale_to_utf8 (time_txt, -1, 0, 0, 0);
	} 

	if ((data->comment == NULL) 
	    && (data->place == NULL) 
	    && (data->time == 0)) {
		if (data->keywords_n > 0) 
			as_string = NULL;
		else if (markup_escape)
			as_string = g_markup_escape_text (_("(No Comment)"), -1);
		else
			as_string = g_strdup (_("(No Comment)"));
		
	} else {
		GString *comment;

		comment = g_string_new ("");

		if (data->comment != NULL) 
			_string_append (comment, data->comment, markup_escape);

		if ((data->comment != NULL)
		    && ((data->place != NULL) || (*time_txt != 0)))
			g_string_append (comment, sep1);
		
		if (data->place != NULL) 
			_string_append (comment, data->place, markup_escape);

		if ((data->place != NULL) && (*time_txt != 0))
			g_string_append (comment, sep2);
		
		if (utf8_time_txt != NULL) 
			_string_append (comment, utf8_time_txt, markup_escape);
		
		as_string = comment->str;
		g_string_free (comment, FALSE);
	}
	
	g_free (utf8_time_txt);
	
	return as_string;
}


char *
comments_get_comment_as_string (CommentData *data, 
				char        *sep1, 
				char        *sep2)
{
	return _get_comment_as_string_common (data, sep1, sep2, FALSE);
}


char *
comments_get_comment_as_xml_string (CommentData *data, 
				    char        *sep1, 
				    char        *sep2)
{
	return _get_comment_as_string_common (data, sep1, sep2, TRUE);
}
