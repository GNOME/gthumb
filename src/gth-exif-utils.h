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

#include <exif-data.h>
#include <exif-content.h>
#include <exif-entry.h>


char *   get_exif_tag            (const char *filename,
				  ExifTag     etag);

time_t   get_exif_time           (const char *filename);

char *   get_exif_aperture_value (const char *filename);

gboolean have_exif_data          (const char *filename);

const char *get_exif_entry_value (ExifEntry *entry);

#endif /* HAVE_LIBEXIF */

#endif /* EXIF_UTILS_H */
