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
#include "glib-utils.h"


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
exif_string_to_time_t (char *string) 
{
        char       *data;
        struct tm   tm = { 0, };

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


time_t
get_exif_time (const char *filename)
{
        char    date_string[21]={0};
        time_t  time = 0;

        if (filename == NULL)
                return (time_t)0;

        gth_minimal_exif_tag_read (filename, EXIF_TAG_DATE_TIME, date_string, 20);

        time = exif_string_to_time_t (date_string);

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

int static
gth_minimal_exif_tag_action (const char *filename,
			     ExifTag     etag,
			     void       *data,
			     int         size,
			     int         access)
{
	/* This function updates/reads ONLY the affected tag. Unlike libexif, it does
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
 
 	debug (DEBUG_INFO, "gth_minimal_exif_tag_access (%s, %04x, %08x, %d)\n", filename, etag, data, size);
 
        // Init IFD stack
 	IFD_OFFSET_PUSH(0);
 	IFD_NAME_PUSH("START");
 
	g_assert (is_local_file (filename));

 	// get path to file
 	if ((filename = get_file_path_from_uri (filename)) == NULL )
 		return PATCH_EXIF_FILE_ERROR;
 
 	// open file r (read (r))
 	if ((jf = fopen (filename, "r")) == NULL)
 		return PATCH_EXIF_FILE_ERROR;
 
        // Fill buffer
        readsize = fread (buf, 1, sizeof (buf), jf);
 
 	// close file during prcessing
 	fclose (jf);
 
        // Check for TIFF header and catch endianess
 	i = 0;
 	while (i < readsize){
 
 		// Little endian TIFF header
 		if (bcmp (&buf[i], leth, 4) == 0){ 
 			endian = G_LITTLE_ENDIAN;
                }
 
 		// Big endian TIFF header
 		else if (bcmp (&buf[i], beth, 4) == 0){ 
 			endian = G_BIG_ENDIAN;
                }
 
 		// Keep looking through buffer
 		else {
 			i++;
 			continue;
 		}
 		// We have found either big or little endian TIFF header
 		tiff = i;
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
 			if (bcmp (&buf[i], (char *)&tag, 2) == 0){ 
 
				// Write TAG value
				if (access == 1){ 

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
					
					if (offset + tiff + count * types[type] > sizeof (buf)) return PATCH_EXIF_TRASHED_IFD;

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
				// Read a TAG value
				else{ 

					// Local value that can be read directly in TAG table 
					if ((types[type] * count) <= 4 && size >= (types[type] * count)){
 
						// Fake TIFF offset
						offset = i + 8 - tiff; 
						patches++;
					}
					// Offseted value of less or equal size than buffer
					else if (types[type] * count <= size){
						patches++;
					}
					// Otherwise we are not able to read the value
					else {
						fprintf(stderr, "gth_minimal_exif_tag_read: TAG value does not fit, Can't read value\n");
						continue;
					}
					
					stitch = 0;
					while (stitch < count){
						switch (types[type]){
						case 1:
							*(char *) (data + stitch) = buf[tiff + offset + stitch];
							break;
						case 2:
							*(guint16 *) (data + stitch * 2) = 
								DE_ENDIAN16(*((unsigned short*)(&buf[tiff + offset + stitch * 2])));
							
							break;
						case 4:
							*(guint32 *) (data + stitch * 4) =
								DE_ENDIAN32(*((unsigned long*) (&buf[tiff + offset + stitch * 4])));
							
							break;
						default:
							fprintf(stderr, "gth_minimal_exif_tag_read:unsupported element size\n");
							return PATCH_EXIF_UNSUPPORTED_TYPE;
						}
						stitch++;
					}					
					return PATCH_EXIF_OK;
				}
			}
 			// EXIF pointer tag?
 			else if (bcmp (&buf[i], (char *) &exififd, 2) == 0){ 
 				IFD_OFFSET_PUSH(offset + tiff);
 				IFD_NAME_PUSH("EXIF");
 			}
 			// GPS pointer tag?
 			else if (bcmp (&buf[i], (char *) &gpsifd, 2) == 0){ 
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


int
gth_minimal_exif_tag_read (const char *filename,
 	                    ExifTag     etag,
 			    void       *data,
 			    int         size)
{
 	debug (DEBUG_INFO, "gth_minimal_exif_tag_read(%s, %04x, %08x, %d)\n", filename, etag, data, size); 
	return gth_minimal_exif_tag_action (filename, etag, data, size, 0);
}


int
gth_minimal_exif_tag_write (const char *filename,
 	                    ExifTag     etag,
 			    void       *data,
 			    int         size)
{
 	debug (DEBUG_INFO, "gth_minimal_exif_tag_write(%s, %04x, %08x, %d)\n", filename, etag, data, size); 
	return gth_minimal_exif_tag_action (filename, etag, data, size, 1);
}


GthTransform
read_orientation_field (const char *path)
{
	ExifShort orientation;
	guint16   tf;

	if (path == NULL)
		return GTH_TRANSFORM_NONE;

	/* old libexif method
	orientation = get_exif_tag_short (path, EXIF_TAG_ORIENTATION); */

	/* new internal method */
	gth_minimal_exif_tag_read (path, EXIF_TAG_ORIENTATION, &tf, 2);

	orientation = (GthTransform) tf;

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

	gth_minimal_exif_tag_write (path, EXIF_TAG_ORIENTATION, &tf, 2);
}
