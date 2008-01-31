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

#ifndef EXIF_UTILS_H
#define EXIF_UTILS_H


#include <config.h>

#include <libexif/exif-data.h>
#include <libexif/exif-content.h>
#include <libexif/exif-entry.h>
#include <libexif/exif-utils.h>
#include "jpegutils/jpeg-data.h"
#include "typedefs.h"
#include "file-data.h"

// Return values from gth_minimal_exif_tag_writes() 
#define PATCH_EXIF_OK               0
#define PATCH_EXIF_TOO_MANY_IFDS    1
#define PATCH_EXIF_TOO_FEW_IFDS     2
#define PATCH_EXIF_FILE_ERROR       3
#define PATCH_EXIF_NO_TIFF          4
#define PATCH_EXIF_NO_TAGS          5
#define PATCH_EXIF_UNSUPPORTED_TYPE 6
#define PATCH_EXIF_TRASHED_IFD      7
#define PATCH_EXIF_TAGVAL_OVERFLOW  8


typedef enum { 
        GTH_METADATA_CATEGORY_FILE = 0, 
        GTH_METADATA_CATEGORY_EXIF_CAMERA, 
        GTH_METADATA_CATEGORY_EXIF_CONDITIONS, 
        GTH_METADATA_CATEGORY_EXIF_IMAGE, 
        GTH_METADATA_CATEGORY_EXIF_THUMBNAIL, 
        GTH_METADATA_CATEGORY_GPS, 
        GTH_METADATA_CATEGORY_MAKERNOTE, 
        GTH_METADATA_CATEGORY_VERSIONS, 
	GTH_METADATA_CATEGORY_IPTC,
        GTH_METADATA_CATEGORY_XMP_EMBEDDED, 
	GTH_METADATA_CATEGORY_XMP_SIDECAR,
	GTH_METADATA_CATEGORY_GSTREAMER,
        GTH_METADATA_CATEGORY_OTHER, 
        GTH_METADATA_CATEGORIES 
} GthMetadataCategory; 
 
 
typedef struct {
        GthMetadataCategory category;
	char    *full_name;
        char    *display_name;
        char    *formatted_value;
	char    *raw_value;
        int      position;
	gboolean writeable;
} GthMetadata;


extern const char **TAG_NAME_SETS[];

enum {
	DATE_TAG_NAMES,
	EXPTIME_TAG_NAMES,
	EXPMODE_TAG_NAMES,
	ISOSPEED_TAG_NAMES,
	APERTURE_TAG_NAMES,
	FOCAL_TAG_NAMES,
	SHUTTERSPEED_TAG_NAMES,
	MAKE_TAG_NAMES,
	MODEL_TAG_NAMES,
	FLASH_TAG_NAMES,
	ORIENTATION_TAG_NAMES
};


ExifData     *gth_exif_data_new_from_uri  (const char   *path);
time_t        get_metadata_time_from_fd   (FileData     *fd);
time_t        get_metadata_time           (const char   *mime_type,
					   const char   *uri,
					   GList        *md);
char *	      get_metadata_string_from_fd (FileData	*fd,
					   const char   *tagnames[]);
void          save_exif_data_to_uri       (const char   *filename,
				           ExifData     *edata);
void          update_and_save_metadata    (const char   *uri_src,
                                           const char   *uri_dest,
                                           const char   *tag_name,
                                           const char   *tag_value);
int           gth_minimal_exif_tag_write  (const char   *filename,
                                           ExifTag       etag,
                                           void         *data,
                                           int           size);
void	      write_orientation_field     (const char   *filename, 
				  	   GthTransform  transform);
GList *       gth_read_exiv2		  (const char   *filename,
					   GList        *metadata);
void          update_metadata		  (FileData	*fd);
#endif /* EXIF_UTILS_H */
