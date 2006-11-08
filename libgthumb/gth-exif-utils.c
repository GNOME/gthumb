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

#include "file-utils.h"
#include "gth-exif-utils.h"


char *
get_exif_tag (const char *filename,
	      ExifTag     etag)
{
	ExifData     *edata;
	unsigned int  i, j;

	filename = get_file_path_from_uri (filename);
	if (filename == NULL)
		return g_strdup ("-");

	edata = exif_data_new_from_file (filename);

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

	filename = get_file_path_from_uri (filename);
	if (filename == NULL)
		return 0;

	edata = exif_data_new_from_file (filename);

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

	filename = get_file_path_from_uri (filename);
	if (filename == NULL)
		return (time_t)0;

	edata = exif_data_new_from_file (filename);

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

	filename = get_file_path_from_uri (filename);
	if (filename == NULL)
		return g_strdup ("-");

	edata = exif_data_new_from_file (filename);

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


ExifData *
load_exif_data (const char *filename)
{
	JPEGData *jdata = NULL;
	ExifData *exif_data = NULL;

	filename = get_file_path_from_uri (filename);
	if (filename == NULL)
		return NULL;

	jdata = jpeg_data_new_from_file (filename);
	if (jdata == NULL) 
		return NULL;

	exif_data = jpeg_data_get_exif_data (jdata);
	jpeg_data_unref (jdata);

	return exif_data;
}


void 
save_exif_data (const char *filename,
		ExifData   *edata)
{
	JPEGData *jdata;

	filename = get_file_path_from_uri (filename);
	if (filename == NULL)
		return;

	jdata = jpeg_data_new_from_file (filename);
	if (jdata == NULL) 
		return;

	if (edata != NULL)
		jpeg_data_set_exif_data (jdata, edata);

	jpeg_data_save_file (jdata, filename);
	jpeg_data_unref (jdata);
}


void 
copy_exif_data (const char *src,
		const char *dest)
{
	ExifData *edata;

	if (!image_is_jpeg (src) || !image_is_jpeg (dest))
		return;
	src = get_file_path_from_uri (src);
	if (src == NULL)
		return;
	dest = get_file_path_from_uri (dest);
	if (dest == NULL)
		return;

	edata = exif_data_new_from_file (src);
	if (edata == NULL) 
		return;
	save_exif_data (dest, edata);

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

