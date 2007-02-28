/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2003 Free Software Foundation, Inc.
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

#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <glib.h>
#include <libgnomeui/gnome-thumbnail.h>
#include <libgnomevfs/gnome-vfs.h>
#include "file-utils.h"
#include "gth-exif-utils.h"
#include <libexif/exif-data.h>
#include "jpegutils/jpeg-data.h"
#include "jpeg-utils.h"

ExifData *
gth_exif_data_new_from_uri (const char *path)
{
        char        *local_file = NULL;
	ExifData    *new_exif_data;

	if (path==NULL)
		return NULL;

        /* libexif does not support VFS URIs directly, so make a temporary local
           copy of remote jpeg files. */

        if (is_local_file (path) || image_is_jpeg (path)) 
                local_file = obtain_local_file (path);
	else
		return NULL;

        if (local_file == NULL) 
                return NULL;

	new_exif_data = exif_data_new_from_file (local_file);

	g_free (local_file);

	return new_exif_data;
}


char *
get_exif_tag (const char *filename,
	      ExifTag     etag)
{
	ExifData     *edata;
	unsigned int  i, j;

	if (filename == NULL)
		return g_strdup ("-");

	edata = gth_exif_data_new_from_uri (filename);

	if (edata == NULL) 
		return g_strdup ("-");

	for (i = 0; i < EXIF_IFD_COUNT; i++) {
		ExifContent *content = edata->ifd[i];

		if (! edata->ifd[i] || ! edata->ifd[i]->count) 
			continue;

		for (j = 0; j < content->count; j++) {
			ExifEntry *e = content->entries[j];

			if (! content->entries[j]) 
				continue;

			if (e->tag == etag) {
				const char *value;
				char *retval = NULL;

				value = get_exif_entry_value (e);
				if (value != NULL)
					retval = g_locale_to_utf8 (value, -1, 0, 0, 0);
				else
					retval = g_strdup ("-");
				exif_data_unref (edata);

				return retval;
			}
		}
	}

	exif_data_unref (edata);

	return g_strdup ("-");
}


ExifShort
get_exif_tag_short (const char *filename,
		    ExifTag     etag)
{
	ExifData     *edata;
	unsigned int  i, j;

	if (filename == NULL)
		return 0;

	edata = gth_exif_data_new_from_uri (filename);

	if (edata == NULL) 
		return 0;

	for (i = 0; i < EXIF_IFD_COUNT; i++) {
		ExifContent *content = edata->ifd[i];

		if (! edata->ifd[i] || ! edata->ifd[i]->count) 
			continue;

		for (j = 0; j < content->count; j++) {
			ExifEntry *e = content->entries[j];

			if (! content->entries[j]) 
				continue;

			if (e->tag == etag) {
				ExifByteOrder o;
				ExifShort retval = 0;

				o = exif_data_get_byte_order (e->parent->parent);
				if (e->data != NULL)
					retval = exif_get_short (e->data, o);
				exif_data_unref (edata);

				return retval;
			}
		}
	}

	exif_data_unref (edata);

	return 0;
}


time_t
get_exif_time (const char *filename)
{
	ExifData     *edata;
	unsigned int  i, j;
	time_t        time = 0;
	struct tm     tm = { 0, };

	if (filename == NULL)
		return (time_t)0;

	edata = gth_exif_data_new_from_uri (filename);

	if (edata == NULL) 
                return (time_t)0;

	for (i = 0; i < EXIF_IFD_COUNT; i++) {
		ExifContent *content = edata->ifd[i];

		if (! edata->ifd[i] || ! edata->ifd[i]->count) 
			continue;

		for (j = 0; j < content->count; j++) {
			ExifEntry   *e = content->entries[j];
			char        *data;

			if (! content->entries[j]) 
				continue;

			if ((e->tag != EXIF_TAG_DATE_TIME) &&
			    (e->tag != EXIF_TAG_DATE_TIME_ORIGINAL) &&
			    (e->tag != EXIF_TAG_DATE_TIME_DIGITIZED))
				continue;

			if (e->data == NULL)
				continue;

			if (strlen ((char*)e->data) < 10)
				continue;

			data = g_strdup ((char*)e->data);

			data[4] = data[7] = data[10] = '\0';

			tm.tm_year = atoi (data) - 1900;
			tm.tm_mon  = atoi (data + 5) - 1;
			tm.tm_mday = atoi (data + 8);
			tm.tm_hour = 0;
			tm.tm_min  = 0;
			tm.tm_sec  = 0;
			tm.tm_isdst = -1;

			if (strlen ((char*)e->data) > 10) {
				data[13] = data[16] = '\0';
				tm.tm_hour = atoi (data + 11);
				tm.tm_min  = atoi (data + 14);
				tm.tm_sec  = atoi (data + 17);
			}

			time = mktime (&tm);

			g_free (data);

			break;
		}
	}

	exif_data_unref (edata);

	return time;
}


char *
get_exif_aperture_value (const char *filename)
{
	ExifData     *edata;
	unsigned int  i, j;

	if (filename == NULL)
		return g_strdup ("-");

	edata = gth_exif_data_new_from_uri (filename);

	if (edata == NULL) 
		return g_strdup ("-");

	for (i = 0; i < EXIF_IFD_COUNT; i++) {
		ExifContent *content = edata->ifd[i];

		if (! edata->ifd[i] || ! edata->ifd[i]->count) 
			continue;

		for (j = 0; j < content->count; j++) {
			ExifEntry   *e = content->entries[j];
			const char  *value = NULL;
			char        *retval = NULL;

			if (! content->entries[j]) 
				continue;

			if ((e->tag != EXIF_TAG_APERTURE_VALUE) &&
			    (e->tag != EXIF_TAG_FNUMBER))
				continue;

			value = get_exif_entry_value (e);
			if (value)
				retval = g_locale_to_utf8 (value, -1, 0, 0, 0);
			else
				retval = g_strdup ("-");
			exif_data_unref (edata);

			return retval;
		}
	}

	exif_data_unref (edata);

	return g_strdup ("-");
}


gboolean 
have_exif_time (const char *filename)
{
	return get_exif_time (filename) != (time_t)0;
}


#define VALUE_LEN 1024


const char *
get_exif_entry_value (ExifEntry *entry)
{
	char value[VALUE_LEN + 1];
	return exif_entry_get_value (entry, value, VALUE_LEN);
}


void 
save_exif_data_to_uri (const char *filename,
		       ExifData   *edata)
{
	JPEGData  *jdata;
        gboolean   is_local;
        gboolean   remote_copy_ok = TRUE;
        char      *local_file_to_modify = NULL;

        is_local = is_local_file (filename);

        /* If the original file is stored on a remote VFS location, copy it to a local
           temp file, modify it, then copy it back. This is easier than modifying the
           underlying jpeg code (and other code) to handle VFS URIs. */

        local_file_to_modify = obtain_local_file (filename);
	
	if (local_file_to_modify == NULL)
		return;

	jdata = jpeg_data_new_from_file (local_file_to_modify);
	if (jdata == NULL) 
		return;

	if (edata != NULL)
		jpeg_data_set_exif_data (jdata, edata);

	jpeg_data_save_file (jdata, local_file_to_modify);
	jpeg_data_unref (jdata);

        if (!is_local)
                remote_copy_ok = copy_cache_file_to_remote_uri (local_file_to_modify, filename);

        g_free (local_file_to_modify);
}


void 
copy_exif_data (const char *src,
		const char *dest)
{
	ExifData *edata;

	if (!image_is_jpeg (src) || !image_is_jpeg (dest))
		return;

	edata = gth_exif_data_new_from_uri (src);
	if (edata == NULL) 
		return;
	save_exif_data_to_uri (dest, edata);

	exif_data_unref (edata);
}


void
set_orientation_in_exif_data (GthTransform  transform,
			      ExifData     *edata)
{
	unsigned int  i;

	for (i = 0; i < EXIF_IFD_COUNT; i++) {
		ExifContent *content = edata->ifd[i];
		ExifEntry   *entry;

		if ((content == NULL) || (content->count == 0)) 
			continue;

		entry = exif_content_get_entry (content, EXIF_TAG_ORIENTATION);
		if (entry != NULL) {
			ExifByteOrder byte_order;
			byte_order = exif_data_get_byte_order (edata);
			exif_set_short (entry->data, byte_order, transform);
		}
	}
}


static void
write_line_to_cache (gpointer key, gpointer value, gpointer out_file)
{
	char  group[256];
	char  tag_name[256];
	char  tag_value[65536];
	int   group_count, tag_count;

	if (sscanf ((char *) key, "%255[^:]:%255[^\n]", group, tag_name) != 2)
		return;

	if (sscanf ((char *) value, "%d:%d:%65535[^\n]", &group_count, &tag_count, tag_value) != 3)
		return;

	fprintf ((FILE *) out_file, "[%s] %s: %s\n", group, tag_name, tag_value);
}


static void
hash_to_file (GHashTable* metadata_hash, const char *cache_file)
{
	/* This functions writes metadata generated by libexif to
	   a cache file, in the exiftool format. */
	FILE *out_file;

	if (g_hash_table_size (metadata_hash) == 0)
		return;

	out_file = fopen (cache_file, "w");
	g_hash_table_foreach (metadata_hash, write_line_to_cache, out_file);
	fclose (out_file);
}


static void
cache_to_hash (const char *cache_file, GHashTable* metadata_hash)
{
	GHashTable *tag_grouping_hash;
	int         group_count = 1;
	int	    tag_count = 1;
	FILE       *in_file;
	char        buf[65535];

	in_file = fopen(cache_file, "r");
	if (in_file == NULL)
		return;

	tag_grouping_hash = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_free);

	/* read, file line by line */
	while(fgets(buf, sizeof (buf), in_file)) {
        	char  group[256];
	        char  tag_name[256];
		char  value[65536];
		char *key;
		char *group_count_string;

		if (sscanf (buf, "[%255[^]]] %255[^:]: %65535[^\n]", group, tag_name, value) != 3)
			continue;

		tag_count++;

		/* New group identified? */
		if (g_hash_table_lookup (tag_grouping_hash, group) == NULL)
			g_hash_table_insert (tag_grouping_hash, 
					     g_strdup (group), 
					     g_strdup_printf ("%d", group_count++));

		group_count_string = g_hash_table_lookup (tag_grouping_hash, group);

		/* key format - group:tag. For example - IFD0:Orientation
		   value format - groupcount:tagcount:value. For example: 7:45:Horizontal (Normal)

		   The groupcount and tagcount values determine the sorting of the groups and
		   tags in the displayed list. */

		key = g_strconcat (group, ":", tag_name, NULL);
		g_hash_table_insert (metadata_hash, 
				     g_strdup (key), 
				     g_strdup_printf ("%s:%d:%s",
					     	      group_count_string,
						      tag_count,
						      value));
  	}			

	fclose(in_file);

	g_hash_table_destroy (tag_grouping_hash);
}

void
get_metadata_for_file (const char *uri, GHashTable* metadata_hash)
{
        char *path;
        char *cache_file;
        char *md5_file;
        char *cache_file_esc;
        char *local_file_esc;
        char *command;
	char *local_file;
	
	if (uri == NULL)
		return;

        local_file = obtain_local_file (uri);
        if (local_file == NULL)
                return;

        path = gnome_vfs_unescape_string (uri, NULL);
        md5_file = gnome_thumbnail_md5 (path);
	g_free (path);

	local_file_esc = shell_escape (local_file);

	cache_file = get_metadata_cache_full_path (md5_file, ".metadata");
	g_free (md5_file);

        if (cache_file == NULL) {
                g_free (local_file_esc);
                return;
        }

        g_assert (is_local_file (cache_file));
        cache_file_esc = shell_escape (cache_file);

	/* Is an up-to-date cache file present? */
	if (path_is_file (cache_file) && (get_file_mtime (uri) < get_file_mtime (cache_file))) {
                cache_to_hash (cache_file, metadata_hash);
	}

	/* Ignore the cache file if it was generated from libexif
	   and exiftool is now available */
	if ( (g_hash_table_lookup (metadata_hash, "ExifTool:ExifToolVersion") == NULL) &&
	      gnome_vfs_is_executable_command_string ("exiftool")) {
		g_hash_table_remove_all (metadata_hash);
		file_unlink (cache_file);
	}

	/* Generate a cache file using exiftool, if needed and if possible */
	if ( (g_hash_table_size (metadata_hash) == 0) &&
	      gnome_vfs_is_executable_command_string ("exiftool")) {
		command = g_strconcat ( "exiftool -S -a -e -G1 ",
       	                                local_file_esc,
               	                        " > ",
                       	                cache_file_esc,
                               	        NULL );
                system (command);
       	        g_free (command);

		cache_to_hash (cache_file, metadata_hash);
	}

	/* If that didn't work, use exiftool */
	if ((g_hash_table_size (metadata_hash) == 0) && image_is_jpeg (local_file)) {
		ExifData *exif_data = NULL;
		JPEGData *jdata = NULL;

		jdata = jpeg_data_new_from_file (local_file);
		if (jdata != NULL) {
			exif_data = jpeg_data_get_exif_data (jdata);
			if (exif_data != NULL) {
				libexif_to_hash (exif_data, metadata_hash);
				hash_to_file (metadata_hash, cache_file);
				exif_data_unref (exif_data);
			}
	                jpeg_data_unref (jdata);
		}
	}

	g_free (local_file);
        g_free (local_file_esc);
        g_free (cache_file);
        g_free (cache_file_esc);
}


gboolean
write_metadata_tag_to_file (const char *path, 
		            const char *tag_name,
			    const char *value)
{
        char             *local_file_to_modify = NULL;
        gboolean          is_local;
        gboolean          remote_copy_ok = TRUE;
        char 	         *local_file_esc;
        char             *command;
	GnomeVFSFileInfo *info;

        is_local = is_local_file (path);
        local_file_to_modify = obtain_local_file (path);

        if (local_file_to_modify == NULL) {
                return FALSE;
        }
	
        info = gnome_vfs_file_info_new ();
        gnome_vfs_get_file_info (path, info, GNOME_VFS_FILE_INFO_GET_ACCESS_RIGHTS|GNOME_VFS_FILE_INFO_FOLLOW_LINKS);

	local_file_esc = shell_escape (local_file_to_modify);

	command = g_strconcat ("exiftool -q -",
			       tag_name,
			       "=\'",
			       value,
			       "\' ",
			       local_file_esc,
			       NULL);
	g_free (local_file_esc);

	/* This isn't really usable yet. */
	/* check for _original file, delete it */
	/* add error reporting! */
	system (command);
	g_free (command);

        if (!is_local)
                remote_copy_ok = copy_cache_file_to_remote_uri (local_file_to_modify, path);

        g_free (local_file_to_modify);

        gnome_vfs_set_file_info (path, info, GNOME_VFS_SET_FILE_INFO_PERMISSIONS|GNOME_VFS_SET_FILE_INFO_OWNER);
        gnome_vfs_file_info_unref (info);

	return remote_copy_ok;
}
