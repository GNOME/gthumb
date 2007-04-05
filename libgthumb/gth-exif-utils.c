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
#include <stdio.h>
#include <time.h>
#include <glib.h>
#include <libgnomeui/gnome-thumbnail.h>
#include <libgnomevfs/gnome-vfs.h>
#include "file-utils.h"
#include "gth-exif-utils.h"
#include "glib-utils.h"
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


gboolean
use_exiftool_for_metadata ()
{
	return gnome_vfs_is_executable_command_string ("exiftool");
}


char *
strip_sort_codes (const char *value) {
	int      category_position, tag_position;
	char     scanned_string[65536];

	/* The value portion of the metadata hash is normally prefixed by
	   a category and tag sorting code, to determine the placement
	   in the display tree. This function removes those codes for
	   applications where they aren't needed. */

	if (sscanf (value, "%d:%d:%65535[^\n]", &category_position, &tag_position, scanned_string) == 3)
		return g_strdup (scanned_string);
	else
		return NULL;
}


time_t
get_metadata_for_file (const char *uri, GHashTable* metadata_hash)
{
        char       *path;
        char       *cache_file;
        char       *md5_file;
        char       *cache_file_esc;
        char       *local_file_esc;
        char       *command;
	char       *local_file;
	gboolean    just_update_cache;
	gboolean    cache_is_good;
	GHashTable *working_metadata_hash;


	if (uri == NULL)
		return;

	/* If no metadata_hash structure is supplied, that means we 
	   just want to update the disk metadata cache. */
	just_update_cache = (metadata_hash == NULL);

	if (just_update_cache)
		working_metadata_hash = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_free);
	else
		working_metadata_hash = metadata_hash;

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
	cache_is_good = path_is_file (cache_file) && (get_file_mtime (uri) < get_file_mtime (cache_file));
	debug (DEBUG_INFO, "Is metadata cache for %s good: %d\n", uri, cache_is_good);

	/* Load the cache file if it is good, and if it is requested. */
	if (cache_is_good && !just_update_cache) {
		debug (DEBUG_INFO, "Loaded metadata cache to a hash.\n");
                cache_to_hash (cache_file, working_metadata_hash);
	}

	/* Do nothing if the cache is good, and we just wanted to ensure
	   it existed. Otherwise, refresh the cache as required. */

	if ( !(cache_is_good && just_update_cache) ) {

		/* Ignore the cache file if it was generated from libexif
		   and exiftool is now available */
		if ( (g_hash_table_lookup (working_metadata_hash, "ExifTool:ExifToolVersion") == NULL) &&
		      use_exiftool_for_metadata ()) {
			g_hash_table_remove_all (working_metadata_hash);
			file_unlink (cache_file);
		}

		/* Generate a cache file using exiftool, if needed and if possible */
		if ( (g_hash_table_size (working_metadata_hash) == 0) &&
		      use_exiftool_for_metadata ()) {
			debug (DEBUG_INFO, "Get metadata with exiftool.\n");

			command = g_strconcat ( "exiftool -S -a -e -G1 ",
       	        	                        local_file_esc,
               	        	                " > ",
                       	        	        cache_file_esc,
                               	        	NULL );
	                system (command);
       		        g_free (command);

			cache_to_hash (cache_file, working_metadata_hash);
		}

		/* If that didn't work, use libexif */
		if ((g_hash_table_size (working_metadata_hash) == 0) && image_is_jpeg (local_file)) {
			ExifData *exif_data = NULL;
			JPEGData *jdata = NULL;

			debug (DEBUG_INFO, "Get metadata with libexif.\n");
			jdata = jpeg_data_new_from_file (local_file);
			if (jdata != NULL) {
				exif_data = jpeg_data_get_exif_data (jdata);
				if (exif_data != NULL) {
					libexif_to_hash (exif_data, working_metadata_hash);
					hash_to_file (working_metadata_hash, cache_file);
					exif_data_unref (exif_data);
				}
	        	        jpeg_data_unref (jdata);
			}
		}
	}

	if (just_update_cache)
		g_hash_table_destroy (working_metadata_hash);

	g_free (local_file);
        g_free (local_file_esc);
        g_free (cache_file);
        g_free (cache_file_esc);

	return get_file_mtime (cache_file);
}


static void
append_to_command (gpointer key, gpointer value, gpointer command_string)
{
	char *esc_value;
	char *str_value;

	if (value == NULL)
		esc_value = g_strdup (" ");
	else {
		str_value = g_strdup ((char *) value);
		g_strstrip (str_value);
		if (str_value[0] == 0)
			esc_value = g_strdup (" ");
		else
			esc_value = shell_escape ((char *) value);
		g_free (str_value);
	}

	g_string_append_printf ((GString *) command_string, 
                                "-%s=%s ", 
                                (char *) key, 
                                esc_value); 
	g_free (esc_value);
}


gboolean
write_metadata_tag_to_file (const char *path,
			    GHashTable *metadata_hash_to_write)
{
        char             *local_file_to_modify = NULL;
        gboolean          is_local;
        gboolean          remote_copy_ok = TRUE;
        char 	         *local_file_esc;
	GnomeVFSFileInfo *info;
	GString          *command;

        is_local = is_local_file (path);
        local_file_to_modify = obtain_local_file (path);

        if (local_file_to_modify == NULL) {
                return FALSE;
        }
	
        info = gnome_vfs_file_info_new ();
        gnome_vfs_get_file_info (path, info, GNOME_VFS_FILE_INFO_GET_ACCESS_RIGHTS|GNOME_VFS_FILE_INFO_FOLLOW_LINKS);

	local_file_esc = shell_escape (local_file_to_modify);

	command = g_string_new  ("exiftool -q -overwrite_original ");

	g_hash_table_foreach (metadata_hash_to_write, 
			      append_to_command, 
			      command);	

	command = g_string_append (command, local_file_esc);
	g_free (local_file_esc);

// printf ("command: %s\n\r",command->str);	
	system (command->str);
	g_string_free (command,TRUE);

        if (!is_local)
                remote_copy_ok = copy_cache_file_to_remote_uri (local_file_to_modify, path);

        g_free (local_file_to_modify);

        gnome_vfs_set_file_info (path, info, GNOME_VFS_SET_FILE_INFO_PERMISSIONS|GNOME_VFS_SET_FILE_INFO_OWNER);
        gnome_vfs_file_info_unref (info);

	return remote_copy_ok;
}


const char leth[]  = {0x49, 0x49, 0x2a, 0x00}; // Little endian TIFF header
const char beth[]  = {0x4d, 0x4d, 0x00, 0x2a}; // Big endian TIFF header
const char types[] = {0x00, 0x01, 0x01, 0x02, 0x04, 0x08, 0x00, 0x08, 0x00, 0x04, 0x08}; // size in bytes for EXIF types
 
#define DE_ENDIAN16(val) endian == G_BIG_ENDIAN ? GUINT16_FROM_BE(val) : GUINT16_FROM_LE(val)
#define DE_ENDIAN32(val) endian == G_BIG_ENDIAN ? GUINT32_FROM_BE(val) : GUINT32_FROM_LE(val)
 
#define ENDIAN16_IT(val) endian == G_BIG_ENDIAN ? GUINT16_TO_BE(val) : GUINT16_TO_LE(val)
#define ENDIAN32_IT(val) endian == G_BIG_ENDIAN ? GUINT32_TO_BE(val) : GUINT32_TO_LE(val)
 
#define IFD_OFFSET_PUSH(val) if (oi < sizeof(offsets) - 1) offsets[oi++] = val; else return PATCH_EXIF_TOO_MANY_IFDS;
#define IFD_OFFSET_PULL()    offsets[--oi]
 
#define IFD_NAME_PUSH(val) names[ni++] = val;
#define IFD_NAME_PULL()    names[--ni]
 
int
gth_minimal_exif_tag_write (const char *filename,
 	                    ExifTag     etag,
 			    void       *data,
 			    int         size,
 			    int         ifds)
{
	/* This function updates ONLY the affected tag. Unlike libexif, it does
	   not attempt to correct or re-format other data. This helps preserve
	   the integrity of certain MakerNote entries, by avoiding re-positioning
	   of the data whenever possible. Some MakerNotes incorporate absolute
	   offsets and others use relative offsets. The offset issue is 
	   avoided if tags are not moved or resized if not required. */

 	FILE *jf;            	// File descriptor to file to patch
 	char  buf[1024 * 64]; 	// Working buffer
 	int   i;               	// index into working buffer
 	int   tag;             	// endianed version of 'etag' in call
 	int   gpsifd;          	// endianed gps ifd pointer tag
 	int   exififd;         	// endianed exif ifd pointer tag
 	int   offset;          	// de-endianed offset in various situations
 	int   tags;            	// number of tags in current ifd
 	int   type;            	// de-endianed type of tag used as index into types[]
 	int   count;           	// de-endianed count of elements in a tag
 	int   stitch;          	// offset in data buffer in type size increments
        int   tiff = 0;   	// offset to active tiff header
        int   endian = 0;   	// detected endian of data
 	int   readsize = 0;   	// number of read bytes from file into buffer
 	int   writesize = 0;   	// number of written bytes from buffer to file
 	int   patches = 0;   	// number of values changed
 
 	// IFD stack variables
 	unsigned long offsets[32];  // Offsets in working buffer to start of IFDs
        char         *names[32];    // Printable names of IFD:s (for debug mostly)
        int           oi = 0;       // index into offsets
        int           ni = 0;       // iundex into names
 	int           cifdi = 0;    // curret ifd index
 
 	debug (DEBUG_INFO, "gth_minimal_exif_tag_write(%s, %04x, %08x, %d, %02x)\n", filename, etag, data, size, ifds);
 
        // Init IFD stack
 	IFD_OFFSET_PUSH(0);
 	IFD_NAME_PUSH("START");
 
	g_assert (is_local_file (filename));

 	// get path to file
 	if ((filename = get_file_path_from_uri (filename)) == NULL )
 		return PATCH_EXIF_FILE_ERROR;
 
 	// open file r (read (r))
 	if ((jf = fopen(filename, "r")) == NULL)
 		return PATCH_EXIF_FILE_ERROR;
 
        // Fill buffer
        readsize = fread(buf, 1, sizeof(buf), jf);
 
 	// close file during prcessing
 	fclose(jf);
 
        // Check for TIFF header and catch endianess
 	i = 0;
 	while (i < readsize){
 
 		// Little endian TIFF header
 		if (bcmp(&buf[i], leth, 4) == 0){ 
 			endian = G_LITTLE_ENDIAN;
                }
 
 		// Big endian TIFF header
 		else if (bcmp(&buf[i], beth, 4) == 0){ 
 			endian = G_BIG_ENDIAN;
                }
 
 		// Keep looking through buffer
 		else {
 			i++;
 			continue;
 		}
 		// We have found either big or little endian TIFF header
 		tiff    = i;
 		break;
        }
 
 	// So did we find a TIFF header or did we just hit end of buffer?
 	if (tiff == 0) return PATCH_EXIF_NO_TIFF;
 
        // Endian some tag values that we will look for
 	exififd = ENDIAN16_IT(0x8769);
 	gpsifd  = ENDIAN16_IT(0x8825);
        tag     = ENDIAN16_IT(etag);
 
        // Read out the offset
        offset  = DE_ENDIAN32(*((unsigned long*)(&buf[i] + 4)));
 	i       = i + offset;
 
        // Start out with IFD0 (and add more IFDs while we go)
 	IFD_OFFSET_PUSH(i);
 	IFD_NAME_PUSH("IFD0");
 
        // As long as we find more IFDs check out the tags in each
 	while ((oi >=0 && (i = IFD_OFFSET_PULL()) != 0 && i < readsize - 2)){
 	  
 		cifdi    = oi; // remember which ifd we are at
 
 		debug (DEBUG_INFO, "%s:\n", IFD_NAME_PULL());
 		tags    = DE_ENDIAN16(*((unsigned short*)(&buf[i])));
 		i       = i + 2;
 
 		// Check this IFD for tags of interest
 		while (tags-- && i < readsize - 12 ){
 			type   = DE_ENDIAN16(*((unsigned short*)(&buf[i + 2])));
 			count  = DE_ENDIAN32(*((unsigned long*) (&buf[i + 4])));
 			offset = DE_ENDIAN32(*((unsigned long*) (&buf[i + 8])));
 
 			debug (DEBUG_INFO, "TAG: %04x type:%02d count:%02d offset:%04lx ", 
 				     DE_ENDIAN16(*((unsigned short*)(&buf[i]))), type, count, offset);
 
 			// Our tag?
 			if (bcmp(&buf[i], (char *)&tag, 2) == 0){ 
 
 				debug (DEBUG_INFO, "*");

 				// Local value that can be patched directly in TAG table 
 				if ((types[type] * count) <= 4){
 
 					// Fake TIFF offset
 					offset = i + 8 - tiff; 
 					patches++;
 				}
 				// Offseted value of same or larger size that we can patch
 				else if (types[type] * count >= size){
 
 					// Adjust count to new length
 					count = size / types[type]; 
 					*((unsigned short*)(&buf[i + 4])) = ENDIAN32_IT(count);
 					patches++;
 				}
 				// Otherwise we are not able to patch the new value, at least not here
 				else {
 					fprintf(stderr, "gth_minimal_exif_tag_write: New TAG value does not fit, no patch applied\n");
 					continue;
 				}
				
				if (offset + tiff + count * types[type] > sizeof(buf)) return PATCH_EXIF_TRASHED_IFD;

 				// Copy the data
 				stitch = 0;			       
 				while (stitch < count){
 					switch (types[type]){
 					case 1:
 						buf[tiff + offset + stitch] = *(char *) (data + stitch);
 						break;
 					case 2:
 						*((unsigned short*)(&buf[tiff + offset + stitch * 2])) = 
 							ENDIAN16_IT(*(guint16 *) (data + stitch * 2));
 						break;
 					case 4:
 						*((unsigned long*) (&buf[tiff + offset + stitch * 4])) = 
 							ENDIAN32_IT(*(guint32 *) (data + stitch * 4));
 						break;
 					default:
 						fprintf(stderr, "gth_minimal_exif_tag_write:unsupported element size\n");
						return PATCH_EXIF_UNSUPPORTED_TYPE;
 					}
 					stitch++;
 				}
 			}
 			// EXIF pointer tag?
 			else if (bcmp(&buf[i], (char *)&exififd, 2) == 0){ 
 				IFD_OFFSET_PUSH(offset + tiff);
 				IFD_NAME_PUSH("EXIF");
 			}
 			// GPS pointer tag?
 			else if (bcmp(&buf[i], (char *)&gpsifd, 2) == 0){ 
 				IFD_OFFSET_PUSH(offset + tiff);
 				IFD_NAME_PUSH("GPS");
 			}
 
 			debug (DEBUG_INFO, "\n");
 					
 			i = i + 12;
 		}
 		// Check for a valid next pointer and assume that to be IFD1 if we just checked IFD0
 		if (cifdi == 1 && i < readsize - 2 && (i = DE_ENDIAN32((*((unsigned long*) (&buf[i]))))) != 0){ 
 			i = i + tiff;
 			IFD_OFFSET_PUSH(i);
 			IFD_NAME_PUSH("IFD1");
 			
 		}
        }

 	// Check if we need to save
 	if (patches == 0)
 		return PATCH_EXIF_NO_TAGS;
 
 	// open file rb+ (read (r) and modify (+), b indicates binary file accces on non-Unix systems
 	if ((jf = fopen(filename, "rb+")) == NULL)
 		return PATCH_EXIF_FILE_ERROR;
 
        // Save changes
 	writesize = fwrite(buf, 1, readsize, jf);
 	fclose(jf);
 
 	return readsize == writesize ? PATCH_EXIF_OK : PATCH_EXIF_FILE_ERROR; 
}


GthTransform
read_orientation_field (const char *path)
{
	ExifShort orientation;

	if (path == NULL)
		return GTH_TRANSFORM_NONE;

	orientation = get_exif_tag_short (path, EXIF_TAG_ORIENTATION);
	if (orientation >= 1 && orientation <= 8)
		return orientation;
	else
		return GTH_TRANSFORM_NONE;
}


void
write_orientation_field (const char   *path,
			 GthTransform  transform)
{
	guint16 tf = (guint16) transform;

	if (path == NULL)
		return;

	gth_minimal_exif_tag_write (path, EXIF_TAG_ORIENTATION, &tf, 2, 0);
}


void
copy_exif_data (const char *local_src_filename,
		const char *local_dest_filename)
{
	/* This exif-copy function avoids the uses of the libexif routines,
	   because they can re-write and corrupt some tags (MakerNotes
	   especially). This routine is a "bulk copy", which does not
	   modify the data. */

	const unsigned char soiapp1[] = {0xff, 0xd8, 0xff, 0xe1};
	const unsigned char app2[]    = {0xff, 0xe2};

	int           msize;            // generic placeholder for marker sizes
	int           appsize[3];       // Sizes for APP0 to APP2
	int          *asize;            // Pointer to current APP marker slot of appsize 
	char         *app[3];           // pointer into imagggge data pointing to APP0 to APP2 
	char         *ptr;              // working pointer through image data

	// vars for setting tag in target file
	guint16       tf = (guint16) 1; // top-left value of EXIF_TAG_ORIENTATION tag
	int           res;              // result from call gth_minimal_exif_tag_write()

	// vars for file copy
	char          *tmp_dir;
	char          *tmp_filename;     // temporary filename
	FILE          *src, *dest, *tfd; // file descriptors
	size_t        fsize;          // number of bytes read

	char          buf[1024 * 129];  // Read buffer for EXIF data (Should be first and within 1 + 2 x 64Kb)
	                               // re-used for copying the rest of the file to the temp destination file

	debug (DEBUG_INFO, "copy_exif_from_orig_and_reset_orientation: %s -> %s\n",local_src_filename, local_dest_filename);

	g_assert (!uri_has_scheme (local_src_filename));
	g_assert (!uri_has_scheme (local_dest_filename));

	// Open exif source file
	if ((src = fopen(local_src_filename, "r")) == (void *) 0L){
		fprintf(stderr, "catalog_web_exporter: Error opening file: %s\n", local_src_filename);
		return;
	}

	// Try to read a buffer from the beginning of the source file
	fsize = fread(buf, 1, sizeof(buf), src); 

        //
	// Scan for markers 
	//
	ptr = buf;                    // Get point for start of search
	memset(app, 0L, sizeof(app)); // Zero table for APP marker starts

	// Loop through buffer as long as we find a marker start (0xff)
	while (ptr - buf < fsize && (ptr = memchr(ptr, 0xff, fsize - (ptr - buf))) != NULL){

		debug (DEBUG_INFO, ".");
		ptr++; // Step over the marker start (0xff) to avoid it the next search

		// Lookup what kind of marker, if one, we found
		switch((int)ptr[0] & 0xff){
		case 0xd8: debug (DEBUG_INFO, "SOI found\n "); continue;
		case 0xe0: debug (DEBUG_INFO, "APP0 found "); app[0] = ptr + 1; asize = &appsize[0]; break;
		case 0xe1: debug (DEBUG_INFO, "APP1 found "); app[1] = ptr + 1; asize = &appsize[1]; break;
		case 0xe2: debug (DEBUG_INFO, "APP2 found "); app[2] = ptr + 1; asize = &appsize[2]; break;
		case 0xda: debug (DEBUG_INFO, "SOS found\n "); ptr = buf + fsize; continue; // We are done
		case 0xdb: // DQT
		case 0xc4: // DHT
		case 0xdd: // DRI
		case 0xc0: debug (DEBUG_INFO, "Valid marker found\n "); asize = &msize; continue; // SOF

		default: continue;   // False marker sync byte, continue to look for real ones
		}

		debug (DEBUG_INFO, "Sizebytes: %02x and %02x\n", ptr[1], ptr[2]);

		*asize = GUINT16_FROM_BE(*(guint16 *)&ptr[1]); // Get size of marker
		ptr = ptr + 1 + *asize;
	}

	// Open dest file which is already assumed to contain a JFIF (APP0 only) image
	if ((dest = fopen(local_dest_filename, "r")) == (void *) 0L){
		fprintf(stderr, "catalog_web_exporter: Error opening file: %s\n", local_dest_filename);
		fclose(src);
		return;
	}
	
	// Open temp file that will contain the merged files

        tmp_dir = get_temp_dir_name ();
        if (tmp_dir == NULL) {
                /* should we display an error message here? */
                return;
        }
        tmp_filename = get_temp_file_name (tmp_dir, NULL);
	g_free (tmp_dir);

	if ((tfd = fopen(tmp_filename, "w+")) == (void *) 0L){
		fprintf(stderr, "catalog_web_exporter: Error opening file: %s\n", tmp_filename);
		fclose (dest);
		fclose (src);
		remove_temp_file_and_dir (tmp_filename);
		g_free (tmp_filename);
		return;
	}
	    
	// Check that we found some EXIF data
	if (app[1] != 0L){
		// write SOI + APP1 marker
		fwrite(soiapp1, sizeof(soiapp1), 1, tfd);

		// write APP1 data
		fwrite(app[1],  appsize[1], 1, tfd);

		// Write APP2 if available
		if (app[2] != 0L){
			// write APP2 marker
			fwrite(app2, sizeof(app2), 1, tfd);

			// write APP2 data
			fwrite(app[2],  appsize[2], 1, tfd);
		}
		
		// write rest of dest file to the tmp file from APP0 onwards
		fseek(dest, 2L, SEEK_SET); // Skip SOI since we already got that written
	} else {
		debug (DEBUG_INFO, "catalog_web_exporter: No EXIF data found in %s\n", local_src_filename);

		// Just copy the whole file
		fseek(dest, 0L, SEEK_SET); 
	}


	while (!feof(dest)){
		fsize = fread(buf, 1, sizeof(buf), dest);
		fwrite(buf, 1, fsize, tfd);
        }

	// Close files
	fclose(src);
	fclose(dest);
	fclose(tfd);

	// Move tmp to dest
	file_move (tmp_filename, local_dest_filename);

	remove_temp_file_and_dir (tmp_filename);
	g_free (tmp_filename);
}

