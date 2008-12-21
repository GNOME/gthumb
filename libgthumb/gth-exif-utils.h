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
#include "typedefs.h"
#include "file-data.h"


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
	COMMENT_DATE_TAG_NAMES,
	SORTING_DATE_TAG_NAMES,
	EXPTIME_TAG_NAMES,
	EXPMODE_TAG_NAMES,
	ISOSPEED_TAG_NAMES,
	APERTURE_TAG_NAMES,
	FOCAL_TAG_NAMES,
	SHUTTERSPEED_TAG_NAMES,
	MAKE_TAG_NAMES,
	MODEL_TAG_NAMES,
	FLASH_TAG_NAMES,
	ORIENTATION_TAG_NAMES,
	COMMENT_TAG_NAMES,
	LOCATION_TAG_NAMES,
	KEYWORD_TAG_NAMES
};


void	      free_metadata               (GList        *metadata);
GthTransform  get_orientation_from_fd     (FileData     *fd);
time_t        get_exif_time               (FileData     *fd);
time_t        get_exif_time_or_mtime      (FileData     *fd);
time_t        get_metadata_time_from_fd   (FileData     *fd,
					   const char   *tagnames[]);
time_t        get_metadata_time           (const char   *mime_type,
					   const char   *uri,
					   GList        *md);
char *        get_metadata_string         (FileData     *fd,
					   const char *tagname);
char *	      get_metadata_tagset_string  (FileData     *fd,
					   const char   *tagnames[]);
GList *       simple_add_metadata         (GList        *metadata,
			                   const gchar  *key,
					   const gchar  *value);
void          update_and_save_metadatum   (const char   *uri_src,
                                           const char   *uri_dest,
                                           char         *tag_name,
                                           char         *tag_value);
void          update_and_save_metadata    (const char   *uri_src,
                                           const char   *uri_dest,
                                           GList        *metdata);
void	      write_orientation_field     (const char   *filename, 
				  	   GthTransform  transform);
GList *       gth_read_exiv2		  (const char   *filename,
					   GList        *metadata);
void          update_metadata		  (FileData	*fd);
void          swap_fields                 (GList        *metadata,
					   const char   *tag1,
				           const char   *tag2);
#endif /* EXIF_UTILS_H */
