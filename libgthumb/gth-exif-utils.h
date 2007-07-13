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

// Return values from gth_minimal_exif_tag_writes() 
#define PATCH_EXIF_OK               0
#define PATCH_EXIF_TOO_MANY_IFDS    1
#define PATCH_EXIF_TOO_FEW_IFDS     2
#define PATCH_EXIF_FILE_ERROR       3
#define PATCH_EXIF_NO_TIFF          4
#define PATCH_EXIF_NO_TAGS          5
#define PATCH_EXIF_UNSUPPORTED_TYPE 6
#define PATCH_EXIF_TRASHED_IFD      7

ExifData     *gth_exif_data_new_from_uri  (const char   *path);
char *        get_exif_tag                (const char   *filename,
				           ExifTag       etag);
ExifShort     get_exif_tag_short          (const char   *filename,
				           ExifTag       etag);
time_t        get_exif_time               (const char   *filename);
char *        get_exif_aperture_value     (const char   *filename);
gboolean      have_exif_time              (const char   *filename);
const char   *get_exif_entry_value        (ExifEntry    *entry);
void          save_exif_data_to_uri       (const char   *filename,
				           ExifData     *edata);
void          copy_exif_data              (const char   *src,
				           const char   *dest);
int           gth_minimal_exif_tag_write  (const char   *filename,
                                           ExifTag       etag,
                                           void         *data,
                                           int           size);
GthTransform  read_orientation_field      (const char   *path);
void	      write_orientation_field     (const char   *filename, 
				  	   GthTransform  transform);

#endif /* EXIF_UTILS_H */
