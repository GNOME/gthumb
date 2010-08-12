/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2009 Free Software Foundation, Inc.
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
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef PREFERENCES_H
#define PREFERENCES_H

#include <gthumb.h>


typedef enum {
	GTH_TIFF_COMPRESSION_NONE,
	GTH_TIFF_COMPRESSION_DEFLATE,
	GTH_TIFF_COMPRESSION_JPEG
} GthTiffCompression;


#define  PREF_JPEG_QUALITY              "/apps/gthumb/save_options/jpeg/quality"
#define  PREF_JPEG_SMOOTHING            "/apps/gthumb/save_options/jpeg/smoothing"
#define  PREF_JPEG_OPTIMIZE             "/apps/gthumb/save_options/jpeg/optimize"
#define  PREF_JPEG_PROGRESSIVE          "/apps/gthumb/save_options/jpeg/progressive"
#define  PREF_PNG_COMPRESSION_LEVEL     "/apps/gthumb/save_options/png/compression_level"
#define  PREF_TGA_RLE_COMPRESSION       "/apps/gthumb/save_options/tga/rle_compression"
#define  PREF_TIFF_COMPRESSION          "/apps/gthumb/save_options/tiff/compression"
#define  PREF_TIFF_HORIZONTAL_RES       "/apps/gthumb/save_options/tiff/horizontal_resolution"
#define  PREF_TIFF_VERTICAL_RES         "/apps/gthumb/save_options/tiff/vertical_resolution"


void so__dlg_preferences_construct_cb (GtkWidget  *dialog,
				       GthBrowser *browser,
				       GtkBuilder *builder);
void so__dlg_preferences_apply_cb     (GtkWidget  *dialog,
				       GthBrowser *browser,
				       GtkBuilder *builder);

#endif /* PREFERENCES_H */
