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

#ifdef HAVE_LIBEXIF

#include <libexif/exif-data.h>
#include <libexif/exif-content.h>
#include <libexif/exif-entry.h>
#include <libexif/exif-utils.h>
#include "jpegutils/jpeg-data.h"


typedef enum { /*< skip >*/
	GTH_EXIF_ORIENTATION_NONE = 0,
	GTH_EXIF_ORIENTATION_TOP_LEFT,
	GTH_EXIF_ORIENTATION_TOP_RIGHT,
	GTH_EXIF_ORIENTATION_BOTTOM_RIGHT,
	GTH_EXIF_ORIENTATION_BOTTOM_LEFT,
	GTH_EXIF_ORIENTATION_LEFT_TOP,
	GTH_EXIF_ORIENTATION_RIGHT_TOP,
	GTH_EXIF_ORIENTATION_RIGHT_BOTTOM,
	GTH_EXIF_ORIENTATION_LEFT_BOTTOM
} GthExifOrientation;


char *      get_exif_tag            (const char *filename,
				     ExifTag     etag);
ExifShort   get_exif_tag_short      (const char *filename,
				     ExifTag     etag);
time_t      get_exif_time           (const char *filename);
char *      get_exif_aperture_value (const char *filename);
gboolean    have_exif_time          (const char *filename);
const char *get_exif_entry_value    (ExifEntry  *entry);
ExifData *  load_exif_data          (const char *filename);
void        save_exif_data          (const char *filename,
				     ExifData   *edata);
void        copy_exif_data          (const char *src,
				     const char *dest);

#endif /* HAVE_LIBEXIF */

#endif /* EXIF_UTILS_H */
