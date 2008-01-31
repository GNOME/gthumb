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
#include <libgnomevfs/gnome-vfs.h>

#include "file-utils.h"
#include "gth-exif-utils.h"
#include "glib-utils.h"
#include "gth-gstreamer-utils.h"


/* Some bits of information may be contained in more than one metadata tag.
   The arrays below define the valid tags for a particular piece of 
   information, in decreasing order of preference (best one first) */

const char *_DATE_TAG_NAMES[] = {
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

/* if you add something here, also update the matching enum in gth-exif-utils.h */
const char **TAG_NAME_SETS[] = {
        _DATE_TAG_NAMES,
        _EXPTIME_TAG_NAMES,
	_EXPMODE_TAG_NAMES,
        _ISOSPEED_TAG_NAMES,
        _APERTURE_TAG_NAMES,
        _FOCAL_TAG_NAMES,
	_SHUTTERSPEED_TAG_NAMES,
	_MAKE_TAG_NAMES,
	_MODEL_TAG_NAMES,
	_FLASH_TAG_NAMES,
	_ORIENTATION_TAG_NAMES
};


ExifData *
gth_exif_data_new_from_uri (const char *uri)
{
        char     *local_file = NULL;
	ExifData *new_exif_data;

	if (uri == NULL)
		return NULL;

	local_file = get_cache_filename_from_uri (uri);
	new_exif_data = exif_data_new_from_file (local_file);
	g_free (local_file);

	return new_exif_data;
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
get_metadata_time_from_fd (FileData *fd)
{
	char   *date = NULL;
	time_t  result = 0;

	date = get_metadata_string_from_fd (fd, TAG_NAME_SETS[DATE_TAG_NAMES]);
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

        orientation_string = get_metadata_string_from_fd (fd, TAG_NAME_SETS[ORIENTATION_TAG_NAMES]);
	
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
get_metadata_string_from_fd (FileData *fd, const char *tagnames[])
{
	int     i;
	char   *string = NULL;

	update_metadata (fd);

	for (i = 0; (tagnames[i] != NULL) && (string == NULL); i++) {
		GList *search_result = g_list_find_custom (fd->metadata, tagnames[i], (GCompareFunc) metadata_search);
		if (search_result != NULL) {
			GthMetadata *md_entry = search_result->data;
			g_free (string);
			string = g_strdup (md_entry->formatted_value);
		}
	}
		
	return string;
}


void 
save_exif_data_to_uri (const char *uri,
		       ExifData   *edata)
{
	char      *local_file = NULL;
	JPEGData  *jdata;

	local_file = get_cache_filename_from_uri (uri);
	jdata = jpeg_data_new_from_file (local_file);
	if (jdata == NULL) 
		return;

	if (edata != NULL)
		jpeg_data_set_exif_data (jdata, edata);

	jpeg_data_save_file (jdata, local_file);
	jpeg_data_unref (jdata);

        g_free (local_file);
}


void 
update_and_save_metadata (const char *uri_src,
			  const char *uri_dest,
			  const char *tag_name,
			  const char *tag_value)
{
	char *from_local_file;
	char *to_local_file;

	from_local_file = get_cache_filename_from_uri (uri_src);
	to_local_file = get_cache_filename_from_uri (uri_dest);

	write_metadata (from_local_file, to_local_file, tag_name, tag_value);

	/* to do: update non-local uri_dest */

	g_free (from_local_file);
	g_free (to_local_file);
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

	update_and_save_metadata (local_file, local_file, "Exif.Image.Orientation", string_tf);

	g_free (string_tf);
}


GList * read_exiv2_file (const char *uri, GList *metadata);
GList * read_exiv2_sidecar (const char *uri, GList *metadata);


GList *
gth_read_exiv2 (const char *uri, GList *metadata)
{
	char *local_file;
	char *uri_wo_ext;
        char *sidecar_uri;

        local_file = get_cache_filename_from_uri (uri);
        if (local_file == NULL)
                return metadata;

	/* Because prepending is faster than appending */
	metadata = g_list_reverse (metadata);

	/* Read image file */
	metadata = read_exiv2_file (local_file, metadata);
	g_free (local_file);

	/* Read sidecar, if present */
	/* FIXME: add remote cache support (use copy_remote_file_to_cache) */
	uri_wo_ext = remove_extension_from_path (uri);
	sidecar_uri = g_strconcat (uri_wo_ext, ".xmp", NULL);
	local_file = get_cache_filename_from_uri (sidecar_uri);

	if ((local_file != NULL) && path_exists (local_file))
	       	metadata = read_exiv2_sidecar (local_file, metadata);

	g_free (local_file);
       	g_free (uri_wo_ext);
        g_free (sidecar_uri);

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
	char *local_file;

	/* Have we already read in the metadata? */
	if (fd->exif_data_loaded == TRUE)
		return;

	local_file = get_cache_filename_from_uri (fd->path); 

	if (fd->mime_type == NULL)
		file_data_update_mime_type (fd, FALSE);

	g_assert (fd->mime_type != NULL);

        if (mime_type_is_image (fd->mime_type)) 
                fd->metadata = gth_read_exiv2 (local_file, fd->metadata); 
	else if (mime_type_is_video (fd->mime_type))
 		fd->metadata = gth_read_gstreamer (local_file, fd->metadata);
 
        /* Sort alphabetically by tag name. The "position" value will 
           override this sorting, if position is non-zero. */ 
        fd->metadata = g_list_sort (fd->metadata, (GCompareFunc) sort_by_tag_name); 
 	fd->exif_data_loaded = TRUE;
	fd->exif_time = get_metadata_time_from_fd (fd);

	g_free (local_file);

        return; 
}
