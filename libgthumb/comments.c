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

#include <libgnome/libgnome.h>
#include <libgnomevfs/gnome-vfs-types.h>
#include <libgnomevfs/gnome-vfs-uri.h>
#include <libgnomevfs/gnome-vfs-utils.h>
#include <libxml/tree.h>
#include <libxml/parser.h>
#include <libxml/xmlmemory.h>
#include <gtk/gtk.h>

#include "typedefs.h"
#include "comments.h"
#include "file-utils.h"
#include "gtk-utils.h"

#define COMMENT_TAG  "Comment"
#define PLACE_TAG    "Place"
#define TIME_TAG     "Time"
#define NOTE_TAG     "Note"
#define KEYWORDS_TAG "Keywords"
#define FORMAT_TAG   "format"
#define FORMAT_VER   "2.0"


CommentData *
comment_data_new ()
{
	CommentData *data;

	data = g_new (CommentData, 1);

	data->place = NULL;
	data->time = 0;
	data->comment = NULL;
	data->keywords_n = 0;
	data->keywords = NULL;
	data->utf8_format = TRUE;

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

        g_free (data);
}


CommentData *
comment_data_dup (CommentData *data)
{
	CommentData *new_data;

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

	return new_data;
}


char *
comments_get_comment_filename (const char *source,
			       gboolean    resolve_symlinks,
			       gboolean    unescape) 
{
	char        *source_real = NULL;
	char        *directory;
	const char  *filename;
	char        *path;

	if (source == NULL) 
		return NULL;

	source_real = g_strdup (source);

	if (resolve_symlinks) {
		char           *parent;
		char           *parent_escaped;
		char           *parent_resolved;
		GnomeVFSResult  result;

		parent = remove_level_from_path (source);
		parent_escaped = gnome_vfs_escape_path_string (parent);
		g_free (parent);

		result = resolve_all_symlinks (parent_escaped, 
					       &parent_resolved);
		g_free (parent_escaped);

		if (result == GNOME_VFS_OK) { 
			g_free (source_real);
			source_real = g_strconcat (parent_resolved, 
						   "/",
						   file_name_from_path (source),
						   NULL);
			g_free (parent_resolved);
		} 
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

	g_free (directory);
	g_free (source_real);

	if (unescape) {
		char *unesc_path = gnome_vfs_unescape_string (path, NULL);
		g_free (path);
		path = unesc_path;
	}

	return path;
}


char *
comments_get_comment_dir (const char *directory,
			  gboolean    resolve_symlinks,
			  gboolean    unescape) 
{
	char        *directory_resolved = NULL;
	const char  *directory_real = directory;
	const char  *separator;
	char        *path;

	if (resolve_symlinks && (directory != NULL)) {
		char           *directory_escaped;
		GnomeVFSResult  result;

		directory_escaped = gnome_vfs_escape_path_string (directory);
		result = resolve_all_symlinks (directory_escaped, 
					       &directory_resolved);
		g_free (directory_escaped);

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


void
comment_copy (const char *src,
	      const char *dest)
{
	char *comment_src;
	char *comment_dest;

	comment_src = comments_get_comment_filename (src, TRUE, TRUE);
	if (! path_is_file (comment_src)) {
		g_free (comment_src);
		comment_src = comments_get_comment_filename (src, FALSE, TRUE);
		if (! path_is_file (comment_src)) {
			g_free (comment_src);
			return;
		}
	}
	comment_dest = comments_get_comment_filename (dest, TRUE, TRUE);

	if (path_is_file (comment_dest)) 
		unlink (comment_dest);
	file_copy (comment_src, comment_dest);

	g_free (comment_src);
	g_free (comment_dest);
}


void
comment_move (const char *src,
	      const char *dest)
{
	char *comment_src;
	char *comment_dest;

	comment_src = comments_get_comment_filename (src, TRUE, TRUE);
	if (! path_is_file (comment_src)) {
		g_free (comment_src);
		comment_src = comments_get_comment_filename (src, FALSE, TRUE);
		if (! path_is_file (comment_src)) {
			g_free (comment_src);
			return;
		}
	}

	comment_dest = comments_get_comment_filename (dest, TRUE, TRUE);

	if (path_is_file (comment_dest)) 
		unlink (comment_dest);
	file_move (comment_src, comment_dest);

	g_free (comment_src);
	g_free (comment_dest);
}


void
comment_delete (const char *filename)
{
	char *comment_name;

	comment_name = comments_get_comment_filename (filename, FALSE, TRUE);
	unlink (comment_name);
	g_free (comment_name);

	comment_name = comments_get_comment_filename (filename, TRUE, TRUE);
	unlink (comment_name);
	g_free (comment_name);
}


/* This checks all files in ~/.gqview/comments/DIR and
 * if CLEAR_ALL is TRUE removes them all otherwise removes only those who 
 * have no source counterpart.
 */
void
comments_remove_old_comments (const char *dir,
			      gboolean    recursive,
			      gboolean    clear_all)
{
	visit_rc_directory (RC_COMMENTS_DIR,
			    COMMENT_EXT,
			    dir,
			    recursive,
			    clear_all);
}


/* ----- comments_remove_old_comments_async implememtation. ------ */


typedef struct {
	gboolean   recursive;
	gboolean   clear_all;
	GtkWidget *dialog;
} CommentsRemoveData;


static void 
check_comment_file (char     *real_file, 
		    char     *rc_file, 
		    gpointer  data)
{
	CommentsRemoveData *crd = data;

	if (crd->clear_all || ! path_is_file (real_file)) {
		if ((unlink (rc_file) < 0)) 
			g_warning ("Cannot delete %s\n", rc_file);
	}
}


void
remove_comments_done (const GList *dir_list, 
		      gpointer     data)
{
	CommentsRemoveData *crd = data;
	const GList        *scan;
	
	if (! crd->clear_all) {
		gtk_widget_destroy (crd->dialog);
		g_free (crd);

		return;
	}

	for (scan = dir_list; scan; scan = scan->next) {
		char *dir = scan->data;
		rmdir (dir);
	}
}


void
comments_remove_old_comments_async (const char *dir,
				    gboolean    recursive,
				    gboolean    clear_all)
{
	CommentsRemoveData *crd;
	const char         *message;

	if (clear_all)
		message = _("Deleting all comments, wait please...");
	else
		message = _("Deleting old comments, wait please...");

	crd = g_new (CommentsRemoveData, 1);
	crd->recursive = recursive;
	crd->clear_all = clear_all;
	crd->dialog = _gtk_message_dialog_new (NULL,
					       GTK_DIALOG_MODAL,
					       GTK_MESSAGE_INFO,
					       message,
					       NULL,
					       GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE,
					       NULL);
	g_signal_connect_swapped (G_OBJECT (crd->dialog),
				  "response",
				  G_CALLBACK (gtk_widget_hide),
				  crd->dialog);
	gtk_widget_show (crd->dialog);

	visit_rc_directory_async (RC_COMMENTS_DIR,
				  COMMENT_EXT,
				  dir,
				  recursive,
				  check_comment_file,
				  remove_comments_done,
				  crd);
}


static char *
get_utf8_text (CommentData *data,
	       xmlChar     *value)
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
	      xmlChar     *utf8_value)
              
{
	char     *value;
        int       n;
        xmlChar  *p, *keyword;
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


CommentData *
comments_load_comment (const char *filename)
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

		comment_file = comments_get_comment_filename (filename, FALSE, TRUE);
		if (! path_is_file (comment_file)) {
			g_free (comment_file);

			return NULL;
		}
	}

        doc = xmlParseFile (comment_file);
	if (doc == NULL) {
		g_free (comment_file);
		return NULL;
	}

	data = g_new (CommentData, 1);
        data->place = NULL;
        data->time = 0;
        data->comment = NULL;
        data->keywords = NULL;
        data->keywords_n = 0;

        root = xmlDocGetRootElement (doc);
        node = root->xmlChildrenNode;

	format = xmlGetProp (root, FORMAT_TAG);
	if (strcmp (format, "1.0") == 0)
		data->utf8_format = FALSE;
	else
		data->utf8_format = TRUE;
	xmlFree (format);

	for (; node; node = node->next) {
                value = xmlNodeListGetString (doc, node->xmlChildrenNode, 1);

                if (strcmp (node->name, PLACE_TAG) == 0) 
			data->place = get_utf8_text (data, value);
		else if (strcmp (node->name, NOTE_TAG) == 0) 
			data->comment = get_utf8_text (data, value);
		else if (strcmp (node->name, KEYWORDS_TAG) == 0) 
			get_keywords (data, value);
		else if (strcmp (node->name, TIME_TAG) == 0) {
			if (value != NULL)
				data->time = atol (value);
		}
		
		if (value)
			xmlFree (value);
        }

        xmlFreeDoc (doc);
	g_free (comment_file);

	return data;
}


void
save_comment (const char  *filename,
	      CommentData *data)
{
	xmlDocPtr    doc;
        xmlNodePtr   tree, subtree;
	char        *comment_file;
	char        *time_str;
	char        *keywords_str;
	char        *dest_dir;

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

	/* Create the xml tree. */

	doc = xmlNewDoc ("1.0");

	doc->xmlRootNode = xmlNewDocNode (doc, NULL, COMMENT_TAG, NULL); 
	xmlSetProp (doc->xmlRootNode, FORMAT_TAG, FORMAT_VER);

	tree = doc->xmlRootNode;
	subtree = xmlNewChild (tree, NULL, PLACE_TAG,    data->place);
	subtree = xmlNewChild (tree, NULL, TIME_TAG,     time_str);
	subtree = xmlNewChild (tree, NULL, NOTE_TAG,     data->comment);
	subtree = xmlNewChild (tree, NULL, KEYWORDS_TAG, keywords_str);

	g_free (time_str);
	g_free (keywords_str);

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


void
comments_save_comment (const char  *filename,
		       CommentData *data)
{
	CommentData *new_data;

	new_data = comments_load_comment (filename);

	if (new_data == NULL) {
		CommentData *data_without_categories;

		data_without_categories = comment_data_dup (data);
		comment_data_free_keywords (data_without_categories);
		save_comment (filename, data_without_categories);
		comment_data_free (data_without_categories);

		return;
	}

	comment_data_free_comment (new_data);
	if (data->place != NULL) 
		new_data->place = g_strdup (data->place);
	if (data->time >= 0)
		new_data->time = data->time;
	if (data->comment != NULL) 
		new_data->comment = g_strdup (data->comment);

	save_comment (filename, new_data);
	comment_data_free (new_data);
}


void
comments_save_comment_non_null (const char  *filename,
				CommentData *data)
{
	CommentData *new_data;

	new_data = comments_load_comment (filename);
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

	new_data = comments_load_comment (filename);

	if (new_data == NULL) {
		CommentData *data_without_comment;

		data_without_comment = comment_data_dup (data);
		comment_data_free_comment (data_without_comment);
		save_comment (filename, data_without_comment);
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

	save_comment (filename, new_data);
	comment_data_free (new_data);
}


char *
comments_get_comment_as_string (CommentData *data, 
				char        *sep1, 
				char        *sep2)
{
	char      *as_string = NULL;
	char       time_txt[50];
	struct tm *tm;

	if (data == NULL) return NULL;
 
	if (data->time != 0) {
		tm = localtime (& data->time);
		if (tm->tm_hour + tm->tm_min + tm->tm_sec == 0)
			strftime (time_txt, 50, _("%d %B %Y"), tm);
		else
			strftime (time_txt, 50, _("%d %B %Y, %H:%M"), tm);
	} else
		time_txt[0] = 0;

	if ((data->comment == NULL) 
	    && (data->place == NULL) 
	    && (data->time == 0)) {
		if (data->keywords_n > 0) 
			as_string = NULL;
		else
			as_string = g_strdup (_("(No Comment)"));
	} else {
		GString *comment;

		comment = g_string_new ("");

		if (data->comment != NULL) 
			g_string_append (comment, data->comment);

		if ((data->comment != NULL)
		    && ((data->place != NULL) || (*time_txt != 0)))
			g_string_append (comment, sep1);
		
		if (data->place != NULL) 
			g_string_append (comment, data->place);

		if ((data->place != NULL) && (*time_txt != 0))
			g_string_append (comment, sep2);

		if (*time_txt != 0) 
			g_string_append (comment, time_txt);

		as_string = comment->str;
		g_string_free (comment, FALSE);
	}

	return as_string;
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
