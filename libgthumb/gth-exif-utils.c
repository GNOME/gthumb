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
#include <stdio.h>
#include <locale.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <glib.h>

#include "file-utils.h"
#include "gfile-utils.h"
#include "gth-exif-utils.h"
#include "glib-utils.h"
#include "gth-gstreamer-utils.h"


/* Some bits of information may be contained in more than one metadata tag.
   The arrays below define the valid tags for a particular piece of 
   information, in decreasing order of preference (best one first) */

/* When reading / writing comment dates, we use a slightly different
   set of tags than for date sorting. */
const char *_COMMENT_DATE_TAG_NAMES[] = {
        "Exif.Image.DateTime",
        "Xmp.exif.DateTime",
        "Exif.Photo.DateTimeOriginal",
        "Xmp.exif.DateTimeOriginal",
        "Exif.Photo.DateTimeDigitized",
        "Xmp.exif.DateTimeDigitized",
	"Xmp.xmp.CreateDate",
        "Xmp.photoshop.DateCreated",
	"Xmp.xmp.ModifyDate",
	"Xmp.xmp.MetadataDate",
	NULL };

const char *_SORTING_DATE_TAG_NAMES[] = {
        "Exif.Photo.DateTimeOriginal",
        "Xmp.exif.DateTimeOriginal",
        "Exif.Photo.DateTimeDigitized",
        "Xmp.exif.DateTimeDigitized",
        "Exif.Image.DateTime",
        "Xmp.exif.DateTime",
        "Xmp.xmp.CreateDate",
        "Xmp.photoshop.DateCreated",
        "Xmp.xmp.ModifyDate",
        "Xmp.xmp.MetadataDate",
        NULL };

const char *_EXPTIME_TAG_NAMES[] = {
        "Exif.Photo.ExposureTime",
	"Xmp.exif.ExposureTime",
	"Exif.Photo.ShutterSpeedValue",
	"Xmp.exif.ShutterSpeedValue",
	NULL };

const char *_EXPMODE_TAG_NAMES[] = {
	"Exif.Photo.ExposureMode",
	"Xmp.exif.ExposureMode",
	NULL };

const char *_ISOSPEED_TAG_NAMES[] = {
        "Exif.Photo.ISOSpeedRatings",
	"Xmp.exif.ISOSpeedRatings", 
	NULL };

const char *_APERTURE_TAG_NAMES[] = {
        "Exif.Photo.ApertureValue",
	"Xmp.exif.ApertureValue", 
	"Exif.Photo.FNumber", 
	"Xmp.exif.FNumber",
	NULL };

const char *_FOCAL_TAG_NAMES[] = {
        "Exif.Photo.FocalLength",
	"Xmp.exif.FocalLength",
	NULL };

const char *_SHUTTERSPEED_TAG_NAMES[] = {
	"Exif.Photo.ShutterSpeedValue",
	"Xmp.exif.ShutterSpeedValue",
	NULL };

const char *_MAKE_TAG_NAMES[] = {
	"Exif.Image.Make",
	"Xmp.tiff.Make",
	NULL };

const char *_MODEL_TAG_NAMES[] = {
	"Exif.Image.Model",
	"Xmp.tiff.Model", 
	NULL };

const char *_FLASH_TAG_NAMES[] = {
	"Exif.Photo.Flash",
	"Xmp.exif.Flash", 
	NULL };

const char *_ORIENTATION_TAG_NAMES[] = {
	"Exif.Image.Orientation",
	"Xmp.tiff.Orientation",
	NULL };

const char *_COMMENT_TAG_NAMES[] = {
        "Exif.Photo.UserComment",
        "Exif.Image.ImageDescription",
	"Xmp.tiff.ImageDescription",
	"Xmp.dc.description",
	"Iptc.Application2.Caption",
        NULL };

const char *_LOCATION_TAG_NAMES[] = {
        "Xmp.iptc.Location",
        "Iptc.Application2.LocationName",
        NULL };

const char *_KEYWORD_TAG_NAMES[] = {
        "Xmp.dc.subject",
        "Xmp.iptc.Keywords",
        "Iptc.Application2.Keywords",
        NULL };


/* if you add something here, also update the matching enum in gth-exif-utils.h */
const char **TAG_NAME_SETS[] = {
        _COMMENT_DATE_TAG_NAMES,
	_SORTING_DATE_TAG_NAMES,
        _EXPTIME_TAG_NAMES,
	_EXPMODE_TAG_NAMES,
        _ISOSPEED_TAG_NAMES,
        _APERTURE_TAG_NAMES,
        _FOCAL_TAG_NAMES,
	_SHUTTERSPEED_TAG_NAMES,
	_MAKE_TAG_NAMES,
	_MODEL_TAG_NAMES,
	_FLASH_TAG_NAMES,
	_ORIENTATION_TAG_NAMES,
	_COMMENT_TAG_NAMES,
	_LOCATION_TAG_NAMES,
	_KEYWORD_TAG_NAMES
};


GList * read_exiv2_file (const char *uri, GList *metadata);
GList * read_exiv2_sidecar (const char *uri, GList *metadata);
void    write_metadata (const char *from_file, const char *to_file, GList *metadata);


static void
free_metadata_entry (GthMetadata *entry)
{
        if (entry != NULL) {
                g_free (entry->full_name);
                g_free (entry->display_name);
                g_free (entry->formatted_value);
                g_free (entry->raw_value);
                g_free (entry);
        }
}


void
free_metadata (GList *metadata)
{
        if (metadata != NULL) {
                g_list_foreach (metadata, (GFunc) free_metadata_entry, NULL);
                g_list_free (metadata);
        }
}


time_t
exif_string_to_time_t (char *string) 
{
        char       *data;
        struct tm   tm = { 0, };

	if (	(string == NULL) 	/* check for non-existent field */
	     || (strlen (string) < 10)	/* check for too-small field */
	     || (string[0] == 0)	/* check for empty field */
	     				/* check for year 1xxx or 2xxx */
	     || !((string[0] == '1') || (string[0] == '2')) )
		return (time_t) 0;

        data = g_strdup (string);

        data[4] = data[7] = data[10] = '\0';

        tm.tm_year = atoi (data) - 1900;
        tm.tm_mon  = atoi (data + 5) - 1;
        tm.tm_mday = atoi (data + 8);
        tm.tm_hour = 0;
        tm.tm_min  = 0;
        tm.tm_sec  = 0;
        tm.tm_isdst = -1;

        if (strlen (string) > 10) {
                data[13] = data[16] = '\0';
                tm.tm_hour = atoi (data + 11);
                tm.tm_min  = atoi (data + 14);
                tm.tm_sec  = atoi (data + 17);
        }

        g_free (data);
        return mktime (&tm);
}

	
gint
metadata_search (GthMetadata *a,
			char *b)
{
	return strcmp (a->full_name, b);
}


time_t
get_exif_time (FileData *fd)
{
	/* we cache the exif_time in fd for quicker sorting */
	update_metadata (fd);
	return fd->exif_time;
}


time_t
get_exif_time_or_mtime (FileData *fd) {
	time_t result;

	update_metadata (fd);
	result = fd->exif_time;

	if (result == (time_t) 0)
		result = fd->mtime;

	return result;
}


time_t
get_metadata_time_from_fd (FileData *fd, const char *tagnames[])
{
	char   *date = NULL;
	time_t  result = 0;

	date = get_metadata_tagset_string (fd, tagnames);
	if (date != NULL)
		result = exif_string_to_time_t (date);
	
	g_free (date);
	return result;
}


GthTransform
get_orientation_from_fd (FileData *fd)
{
        char         *orientation_string = NULL;
	GthTransform  result = GTH_TRANSFORM_NONE;

        orientation_string = get_metadata_tagset_string (fd, TAG_NAME_SETS[ORIENTATION_TAG_NAMES]);
	
	if (orientation_string == NULL)
		result = GTH_TRANSFORM_NONE;
	else if (!strcmp (orientation_string, "top, left"))
		result = GTH_TRANSFORM_NONE;
	else if (!strcmp (orientation_string, "top, right"))
                result = GTH_TRANSFORM_FLIP_H;
	else if (!strcmp (orientation_string, "bottom, right"))
                result = GTH_TRANSFORM_ROTATE_180;
	else if (!strcmp (orientation_string, "bottom, left"))
                result = GTH_TRANSFORM_FLIP_V;
	else if (!strcmp (orientation_string, "left, top"))
                result = GTH_TRANSFORM_TRANSPOSE;
	else if (!strcmp (orientation_string, "right, top"))
                result = GTH_TRANSFORM_ROTATE_90;
	else if (!strcmp (orientation_string, "right, bottom"))
                result = GTH_TRANSFORM_TRANSVERSE;
	else if (!strcmp (orientation_string, "left, bottom"))
                result = GTH_TRANSFORM_ROTATE_270;

        g_free (orientation_string);

        return result;
}


char *
get_metadata_string (FileData *fd, const char *tagname)
{
        char   *string = NULL;

	/* Searches for one (and only one) tag name */

        update_metadata (fd);

        GList *search_result = g_list_find_custom (fd->metadata, tagname, (GCompareFunc) metadata_search);
        if (search_result != NULL) {
		GthMetadata *md_entry = search_result->data;
                string = g_strdup (md_entry->formatted_value);
        }

        return string;
}


char *
get_metadata_tagset_string (FileData *fd, const char *tagnames[])
{
	int     i;
	char   *string = NULL;

	/* Searches for the best tag from a list of acceptable tag names */

	update_metadata (fd);

	for (i = 0; (tagnames[i] != NULL) && (string == NULL); i++) {		
		GList *search_result = g_list_find_custom (fd->metadata, tagnames[i], (GCompareFunc) metadata_search);
		if (search_result != NULL) {
			GthMetadata *md_entry = search_result->data;
			string = g_strdup (md_entry->formatted_value);
		}
	}
		
	return string;
}


GSList *
get_metadata_tagset_list (FileData *fd, const char *tagnames[])
{
	int     i;
        GSList *list = NULL;
        GList  *tmp;

	/* Searches for the best tag from a list of acceptable tag names */

	update_metadata (fd);

	for (i = 0; tagnames[i] != NULL; i++) {
                for (tmp = fd->metadata; tmp; tmp = g_list_next (tmp)) {
                        GthMetadata *md_entry = tmp->data;
                        if (!strcmp (md_entry->full_name, tagnames[i])) {
                                list = g_slist_append (list, g_strdup (md_entry->formatted_value));
                        }
                }
        }

	return list;
}


GList *
simple_add_metadata (GList       *metadata,
                     const gchar *key,
                     const gchar *value)
{
	/* This function is only used when we want to pack several
	   metadata items into a single structure, to supply to
	   update_and_save_metadata. */

        GthMetadata *entry;

        GList *search_result = g_list_find_custom (metadata, key, (GCompareFunc) metadata_search);
        if (search_result != NULL) {
		/* Update value of existing tag */
		entry = search_result->data;
		g_free (entry->formatted_value);
		g_free (entry->raw_value);
		entry->formatted_value = g_strdup (value);
		entry->raw_value = g_strdup (value);
	}
	else {
		/* Add a new tag entry */
                entry = g_new (GthMetadata, 1);
                entry->category = GTH_METADATA_CATEGORY_OTHER;
                entry->full_name = g_strdup (key);
                entry->display_name = g_strdup (key);
                entry->formatted_value = g_strdup (value);
                entry->raw_value = g_strdup (value);
                entry->position = 0;
                entry->writeable = TRUE;
                metadata = g_list_prepend (metadata, entry);
        }

        return metadata;
}


void 
update_and_save_metadatum (const char *uri_src,
                           const char *uri_dest,
                           char       *tag_name,
                           char       *tag_value)
{
	/* This is a convenience function, to simplify the API */
	GList *metadata = NULL;
	
	metadata = simple_add_metadata (metadata, tag_name, tag_value);
	update_and_save_metadata (uri_src, uri_dest, metadata);

	free_metadata (metadata);
}


void 
update_and_save_metadata (const char *uri_src,
			  const char *uri_dest,
			  GList      *metadata)
{
	FileData	 *from_fd;
	FileData	 *to_fd;

	from_fd = file_data_new (uri_src);
	to_fd = file_data_new (uri_dest);

	if ((from_fd->local_path == NULL) || (to_fd->local_path == NULL)) {
		file_data_unref (from_fd);
		file_data_unref (to_fd);
		g_warning ("Can't write metadata if the remote files are not mounted locally.");
		return;
	}

	write_metadata (from_fd->local_path, to_fd->local_path, metadata);

	file_data_unref (from_fd);
	file_data_unref (to_fd);
}


void
write_orientation_field (const char   *local_file,
			 GthTransform  transform)
{
	char *string_tf;
	int tf;
       
	tf = (int) transform;
	if ((tf < 0) || (tf > 8))
		tf = 1;
	string_tf = g_strdup_printf ("%d", tf);

	update_and_save_metadatum (local_file, local_file, "Exif.Image.Orientation", string_tf);

	g_free (string_tf);
}


GList *
gth_read_exiv2 (const char *local_file, GList *metadata)
{
	char *without_ext;
        char *sidecar;

	g_assert (! uri_has_scheme (local_file));

        if (local_file == NULL)
                return metadata;

	/* Because prepending is faster than appending */
	metadata = g_list_reverse (metadata);

	/* Read image file */
	metadata = read_exiv2_file (local_file, metadata);

	/* Read sidecar, if present */
	without_ext = remove_extension_from_path (local_file);
	sidecar = g_strconcat (without_ext, ".xmp", NULL);

	if ((sidecar != NULL) && path_exists (sidecar))
	       	metadata = read_exiv2_sidecar (sidecar, metadata);

       	g_free (without_ext);
        g_free (sidecar);

	/* Undo the initial reverse */
	metadata = g_list_reverse (metadata);

	return metadata;
}


static gint
sort_by_tag_name (GthMetadata *entry1, GthMetadata *entry2)
{
	return strcmp_null_tolerant (entry1->display_name, entry2->display_name);
}


void
update_metadata (FileData *fd) 
{ 
	/* Have we already read in the metadata? */
	if (fd->exif_data_loaded == TRUE)
		return;

	if (fd->local_path == NULL) {
                g_warning ("Can't read metadata if the remote files are not mounted locally.");
                return;	
	}
	
	if (fd->mime_type == NULL)
		file_data_update_mime_type (fd, FALSE);

	g_assert (fd->mime_type != NULL);

        if (mime_type_is_image (fd->mime_type)) 
                fd->metadata = gth_read_exiv2 (fd->local_path, fd->metadata); 
	else if (mime_type_is_video (fd->mime_type))
 		fd->metadata = gth_read_gstreamer (fd->local_path, fd->metadata);
 
        /* Sort alphabetically by tag name. The "position" value will 
           override this sorting, if position is non-zero. */ 
        fd->metadata = g_list_sort (fd->metadata, (GCompareFunc) sort_by_tag_name); 
 	fd->exif_data_loaded = TRUE;
	fd->exif_time = get_metadata_time_from_fd (fd, TAG_NAME_SETS[SORTING_DATE_TAG_NAMES]);

        return; 
}


void
swap_fields (GList *metadata, const char *tag1, const char *tag2)
{
	char        *tmp;
	GthMetadata *entry1;
	GthMetadata *entry2;

        GList *search_result1 = g_list_find_custom (metadata, tag1, (GCompareFunc) metadata_search);
        if (search_result1 == NULL)
		return;
	else
                entry1 = search_result1->data;

        GList *search_result2 = g_list_find_custom (metadata, tag2, (GCompareFunc) metadata_search);
        if (search_result2 == NULL) 
                return;
        else 
                entry2 = search_result2->data;
	
	tmp = entry1->formatted_value;	
	entry1->formatted_value = entry2->formatted_value;
	entry2->formatted_value = tmp;

        tmp = entry1->raw_value;
        entry1->raw_value = entry2->raw_value;
        entry2->raw_value = tmp;
}

GList *
clear_metadata_tagset (GList        *metadata,
                       const char   *tagnames[])
{
        int i;
        GList *tmp_metadata = metadata;

        for (i = 0; tagnames[i]; ++i) {
                tmp_metadata = simple_add_metadata (tmp_metadata, tagnames[i], "");
        }

        return tmp_metadata;
}

