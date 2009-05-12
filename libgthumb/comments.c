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
#include <strings.h>
#include <ctype.h>
#include <time.h>

#include <glib/gi18n.h>
#include <libgnomevfs/gnome-vfs-result.h>
#include <libxml/tree.h>
#include <libxml/parser.h>
#include <libxml/xmlmemory.h>
#include <gtk/gtk.h>

#include "typedefs.h"
#include "comments.h"
#include "file-utils.h"
#include "glib-utils.h"
#include "gtk-utils.h"
#include "gth-exif-utils.h"

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
	data->keywords = NULL;
	data->utf8_format = TRUE;
	data->changed = FALSE;

	return data;
}


void
comment_data_free_keywords (CommentData *data)
{
        g_slist_foreach (data->keywords, (GFunc) g_free, NULL);
        g_slist_free (data->keywords);
        data->keywords = NULL;
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
        g_free (data);
}


CommentData *
comment_data_dup (CommentData *data)
{
	CommentData *new_data;
        GSList *tmp;

	if (data == NULL)
		return NULL;

	new_data = comment_data_new ();

	if (data->place != NULL)
		new_data->place = g_strdup (data->place);
	new_data->time = data->time;
	if (data->comment != NULL)
		new_data->comment = g_strdup (data->comment);

        for (tmp = data->keywords; tmp; tmp = g_slist_next (tmp)) {
                new_data->keywords = g_slist_append (new_data->keywords, g_strdup (tmp->data));
	}
	new_data->utf8_format = data->utf8_format;

	return new_data;
}


gboolean
comment_data_equal (CommentData *data1,
		    CommentData *data2)
{
        GSList *tmp1, *tmp2;

	if ((data1 == NULL) && (data2 == NULL))
		return TRUE;
	if ((data1 == NULL) || (data2 == NULL))
		return FALSE;

	if (strcmp_null_tolerant (data1->place, data2->place) != 0)
		return FALSE;
	if (data1->time != data2->time)
		return FALSE;
	if (strcmp_null_tolerant (data1->comment, data2->comment) != 0)
		return FALSE;
	if (g_slist_length (data1->keywords) != g_slist_length (data2->keywords))
		return FALSE;
        for (tmp1 = data1->keywords, tmp2 = data2->keywords;
             tmp1 && tmp2;
             tmp1 = g_slist_next (tmp1), tmp2 = g_slist_next (tmp2)) {
		if (strcmp_null_tolerant (tmp1->data,
					  tmp2->data) != 0)
			return FALSE;
        }

	return TRUE;
}


char *
comments_get_comment_filename (const char *uri,
			       gboolean    resolve_symlinks)
{
	char *source_real = NULL;
	char *directory = NULL;
	char *filename = NULL;
	char *comment_uri = NULL;

	if (uri == NULL) 
		return NULL;

	source_real = g_strdup (uri);

	if (resolve_symlinks) {
		char           *resolved = NULL;
		GnomeVFSResult  result;

		result = resolve_all_symlinks (source_real, &resolved);

		if (result == GNOME_VFS_OK) {
			g_free (source_real);
			source_real = resolved;
		} 
		else
			g_free (resolved);
	}

	directory = remove_level_from_path (source_real);
	filename = g_strconcat (file_name_from_path (source_real), COMMENT_EXT, NULL);

	comment_uri = g_strconcat (directory, "/.comments/", filename, NULL);

	g_free (directory);
	g_free (filename);
	g_free (source_real);

	return comment_uri;
}


void
comment_copy (const char *src,
	      const char *dest)
{
	char *comment_src = NULL;
	char *comment_dest = NULL;

	comment_src = comments_get_comment_filename (src, TRUE);
	if (! path_is_file (comment_src)) {
		g_free (comment_src);
		return;
	}

	comment_dest = comments_get_comment_filename (dest, TRUE);
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

	comment_src = comments_get_comment_filename (src, TRUE);
	if (! path_is_file (comment_src)) {
		g_free (comment_src);
		return;
	}

	comment_dest = comments_get_comment_filename (dest, TRUE);
	if (path_is_file (comment_dest))
		file_unlink (comment_dest);

	file_move (comment_src, comment_dest);

	g_free (comment_src);
	g_free (comment_dest);
}


void
comment_delete (const char *uri)
{
	char *comment_uri;

	comment_uri = comments_get_comment_filename (uri, TRUE);
	file_unlink (comment_uri);
	g_free (comment_uri);

	/* TODO: delete comment metadata fields */
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
        char    **keywords_v;
        int       i;

        comment_data_free_keywords (data);

	if ((utf8_value == NULL) || (*utf8_value == 0)) {
		return;
	}

	value = get_utf8_text (data, utf8_value);
	if (value == NULL) {
		return;
	}

        keywords_v = g_strsplit (value, ",", 0);

        for (i = 0; keywords_v[i]; ++i) {
                data->keywords = g_slist_append (data->keywords, g_strdup (keywords_v[i]));
        }
        g_strfreev (keywords_v);

	g_free (value);
}


static gboolean
has_non_whitespace_comment (const char *text_in)
{
        const char *pos;

        if (text_in == NULL)
                return FALSE;

        for (pos = text_in; *pos != 0; pos = g_utf8_next_char (pos)) {
                gunichar ch = g_utf8_get_char (pos);
                if (!g_unichar_isspace (ch))
                        return TRUE;
        }

        return FALSE;
}


static CommentData *
load_comment_from_metadata (const char *uri)
{
	CommentData *data;
	FileData    *file;
	char        *metadata_string = NULL;
	time_t	     metadata_time = 0;
        GSList      *metadata_list, *tmp;
        char        **keywords_v;

	file = file_data_new (uri);
	file_data_update_all (file, FALSE);

	data = comment_data_new ();

	metadata_string = get_metadata_tagset_string (file, TAG_NAME_SETS[COMMENT_TAG_NAMES]);
	if (has_non_whitespace_comment (metadata_string))
		data->comment = g_strdup (metadata_string);
	g_free (metadata_string);

        metadata_string = get_metadata_tagset_string (file, TAG_NAME_SETS[LOCATION_TAG_NAMES]);
	if (has_non_whitespace_comment (metadata_string))
                data->place = g_strdup (metadata_string);
	g_free (metadata_string);

        metadata_list = get_metadata_tagset_list (file, TAG_NAME_SETS[KEYWORD_TAG_NAMES]);
	comment_data_free_keywords (data);
        for (tmp = metadata_list; tmp; tmp = g_slist_next (tmp)) {
                if (has_non_whitespace_comment (tmp->data)) {
                        int i;
                        keywords_v = g_strsplit (tmp->data, ",", 0);

                        for (i = 0; keywords_v[i]; ++i) {
                                comment_data_add_keyword (data, keywords_v[i]);
                        }
                        g_strfreev (keywords_v);
                }
        }
        g_slist_foreach (metadata_list, (GFunc) g_free, NULL);
        g_slist_free (metadata_list);

	/* Only load the metadata time if the metadata contained useful comments,
	   location data, or keywords. Almost all photos contain date metadata, so
	   we do not want to crowd the comment display with date metadata unless
	   it seems to be intentionally linked with other metadata. This may
	   be a matter for further consideration. */
	if ((data->comment != NULL) ||
            (data->place != NULL) ||
	    (data->keywords)) {
	        metadata_time = get_metadata_time_from_fd (file, TAG_NAME_SETS[COMMENT_DATE_TAG_NAMES]);
        	if (metadata_time > (time_t) 0)
                	data->time = metadata_time;
	} else {
		/* Nothing useful was found. Return NULL. */
		comment_data_free (data);	
		data = NULL;
	}

        file_data_unref (file);

	return data;
}

static char*
get_keywords_str (CommentData *data)
{
        GString   *keywords_str;
        GSList    *tmp;

        keywords_str = g_string_new (NULL);
        for (tmp = data->keywords; tmp; tmp = g_slist_next (tmp)) {
                keywords_str = g_string_append (keywords_str, g_strdup (tmp->data));
                if (g_slist_next (tmp))
                        keywords_str = g_string_append_c (keywords_str, ',');
        }
        return g_string_free (keywords_str, FALSE);
}

static void
save_comment_to_metadata (const char  *uri,
   	                  CommentData *data)
{
        char      *keywords_str;
	GList     *add_metadata = NULL;
        FileData  *file;
        char      *buf;
        struct tm  tm;

        file = file_data_new (uri);
        file_data_update_all (file, FALSE);

	add_metadata = simple_add_metadata (add_metadata, TAG_NAME_SETS[COMMENT_TAG_NAMES][0], data->comment);
	add_metadata = simple_add_metadata (add_metadata, TAG_NAME_SETS[LOCATION_TAG_NAMES][0], data->place);

        localtime_r (&data->time, &tm);
        buf = g_strdup_printf ("%04d:%02d:%02d %02d:%02d:%02d ",
                               tm.tm_year + 1900,
                               tm.tm_mon + 1,
                               tm.tm_mday,
                               tm.tm_hour,
                               tm.tm_min,
                               tm.tm_sec );
        add_metadata = simple_add_metadata (add_metadata, TAG_NAME_SETS[COMMENT_DATE_TAG_NAMES][0], buf);

        add_metadata = clear_metadata_tagset (add_metadata, TAG_NAME_SETS[KEYWORD_TAG_NAMES]);
        keywords_str = get_keywords_str (data);
	add_metadata = simple_add_metadata (add_metadata, TAG_NAME_SETS[KEYWORD_TAG_NAMES][0], keywords_str);
        g_free (keywords_str);

	update_and_save_metadata (file->path, file->path, add_metadata);
	free_metadata (add_metadata);
	file_data_unref (file);
}


static CommentData *
load_comment_from_xml (const char *uri)
{
	CommentData *data;
	char        *comment_uri;
	xmlDocPtr    doc;
        xmlNodePtr   root, node;
        xmlChar     *value;
	xmlChar     *format;
	FileData    *fd;

	if (uri == NULL)
		return NULL;

	comment_uri = comments_get_comment_filename (uri, TRUE);
	fd = file_data_new (comment_uri);
	g_free (comment_uri);

	if (! path_is_file (fd->utf8_path) || ! file_data_has_local_path (fd, NULL)) {
		file_data_unref (fd);
		return NULL;
	}

        doc = xmlParseFile (fd->local_path);
	file_data_unref (fd);
        
	if (doc == NULL) 
		return NULL;
	
	data = comment_data_new ();
        root = xmlDocGetRootElement (doc);
	format = xmlGetProp (root, FORMAT_TAG);
	if (strcmp ((char*)format, "1.0") == 0)
		data->utf8_format = FALSE;
	else
		data->utf8_format = TRUE;
	for (node = root->xmlChildrenNode; node; node = node->next) {
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

	return data;
}


void
save_comment (const char  *uri,
	      CommentData *data,
	      gboolean     save_embedded)
{
	xmlDocPtr    doc;
        xmlNodePtr   tree, subtree;
	char        *comment_uri = NULL;
	char        *time_str = NULL;
	char        *keywords_str = NULL;
	char        *dest_dir = NULL;
	char        *e_comment = NULL, *e_place = NULL, *e_keywords = NULL;
	FileData    *fd;

	if ((data == NULL) || (uri == NULL))
		return;

	if (save_embedded)
		save_comment_to_metadata (uri, data);

	if (comment_data_is_void (data)) {
		comment_delete (uri);
		return;
	}

	/* Convert data to strings. */

	time_str = g_strdup_printf ("%ld", data->time);
        keywords_str = get_keywords_str (data);

	/* Escape text */

	if (data->comment != NULL && g_utf8_validate (data->comment, -1, NULL))
		e_comment = g_markup_escape_text (data->comment, -1);

	if (data->place != NULL && g_utf8_validate (data->place, -1, NULL))
		e_place = g_markup_escape_text (data->place, -1);

	if (keywords_str != NULL && g_utf8_validate (keywords_str, -1, NULL))
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

	comment_uri = comments_get_comment_filename (uri, TRUE);
	fd = file_data_new (comment_uri);

	if (file_data_has_local_path (fd, NULL)) {
		dest_dir = remove_level_from_path (fd->local_path);
		if (ensure_dir_exists (dest_dir)) {
			xmlSetDocCompressMode (doc, 3);
			xmlSaveFile (fd->local_path, doc);
		}
	}
	
	file_data_unref (fd);	
	g_free (dest_dir);
	g_free (comment_uri);
        xmlFreeDoc (doc);
}


CommentData *
comments_load_comment (const char *uri,
		       gboolean    try_embedded)
{
	CommentData *xml_comment = NULL;
	CommentData *img_comment = NULL;

	if (uri == NULL)
		return NULL;

	xml_comment = load_comment_from_xml (uri);

	if (try_embedded) {
		img_comment = load_comment_from_metadata (uri);
		if (img_comment != NULL) {
			if (xml_comment == NULL)
				xml_comment = comment_data_new ();

			if (! comment_data_equal (xml_comment, img_comment)) {
				/* Consider the image comment more up-to-date and
				 * sync the xml comment with it. */
				save_comment (uri, img_comment, FALSE);
				comment_data_free (xml_comment);
				xml_comment = img_comment;
				xml_comment->changed = TRUE;
			}
		} 
		else
			comment_data_free (img_comment);
	}

	return xml_comment;
}


void
comments_save_comment (const char  *uri,
		       CommentData *data)
{
	CommentData *new_data;

	if (uri == NULL)
		return;

	new_data = comments_load_comment (uri, FALSE);
	
	if ((new_data == NULL) && (data != NULL)) {
		CommentData *data_without_categories;

		data_without_categories = comment_data_dup (data);
		comment_data_free_keywords (data_without_categories);
		save_comment (uri, data_without_categories, TRUE);
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

	save_comment (uri, new_data, TRUE);
	comment_data_free (new_data);
}


void
comments_save_comment_non_null (const char  *uri,
				CommentData *data)
{
	CommentData *new_data;

	if (uri == NULL)
		return;

	new_data = comments_load_comment (uri, TRUE);
	if (new_data == NULL) {
		comments_save_comment (uri, data);
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

	comments_save_comment (uri, new_data);
	comment_data_free (new_data);
}


void
comments_save_categories (const char  *uri,
			  CommentData *data)
{
	CommentData *new_data;
        GSList *tmp;

	if (uri == NULL)
		return;

	new_data = comments_load_comment (uri, TRUE);

	if (new_data == NULL) {
		CommentData *data_without_comment;

		data_without_comment = comment_data_dup (data);
		comment_data_free_comment (data_without_comment);
		save_comment (uri, data_without_comment, TRUE);
		comment_data_free (data_without_comment);

		return;
	}

	comment_data_free_keywords (new_data);

        for (tmp = data->keywords; tmp; tmp = g_slist_next (tmp)) {
                new_data->keywords = g_slist_append (new_data->keywords, g_strdup (tmp->data));
	}

	save_comment (uri, new_data, TRUE);
	comment_data_free (new_data);
}


void
comment_data_remove_keyword (CommentData *data,
			     const char  *keyword)
{
        GSList *found;

	if ((data->keywords == NULL)
	    || (keyword == NULL))
		return;

        found = g_slist_find_custom (data->keywords, keyword, (GCompareFunc) g_utf8_collate);
	if (!found)
		return;

        data->keywords = g_slist_remove_link (data->keywords, found);
        g_free (found->data);
        g_slist_free (found);
}


void
comment_data_add_keyword (CommentData *data,
			  const char  *keyword)
{
        GSList *found;
        gchar *formated_keyword;

	if (keyword == NULL)
		return;

        if (!has_non_whitespace_comment (keyword))
                return;

        /* Removes leading and trailing whitespaces */
        formated_keyword = g_strstrip (g_strdup (keyword));

        found = g_slist_find_custom (data->keywords, formated_keyword, (GCompareFunc) g_utf8_collate);
        if (!found)
                data->keywords = g_slist_append (data->keywords, g_strdup (formated_keyword));

        g_free (formated_keyword);
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
	if (data->keywords)
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
		if (data->keywords)
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


/* Note: separators are not escaped */
static char *
_get_categories_as_string_common (CommentData *data,
                                  char        *sep)
{
        GString *categories;
        GSList  *tmp;

	if (data == NULL)
		return NULL;

        if (data->keywords == NULL)
                return NULL;

        categories = g_string_new ("");

        for (tmp = data->keywords; tmp; tmp = g_slist_next (tmp)) {
                g_string_append (categories, tmp->data);
                if (g_slist_next (tmp))
                        g_string_append (categories, sep);
        }

        return g_string_free (categories, FALSE);
}


char *
comments_get_comment_as_string (CommentData *data,
				char        *sep1,
				char        *sep2)
{
	return _get_comment_as_string_common (data, sep1, sep2, FALSE);
}

char *
comments_get_categories_as_string (CommentData *data,
                                   char        *sep)
{
	return _get_categories_as_string_common (data, sep);
}

