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
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Street #330, Boston, MA 02111-1307, USA.
 */

#ifndef PREFERENCES_H
#define PREFERENCES_H

#include <gthumb.h>

G_BEGIN_DECLS

typedef enum {
	GTH_SUBFOLDER_TYPE_NONE = 0,
        GTH_SUBFOLDER_TYPE_FILE_DATE,
	GTH_SUBFOLDER_TYPE_CURRENT_DATE,
} GthSubfolderType;


typedef enum {
	GTH_SUBFOLDER_FORMAT_YYYYMMDD,
	GTH_SUBFOLDER_FORMAT_YYYYMM,
	GTH_SUBFOLDER_FORMAT_YYYY,
	GTH_SUBFOLDER_FORMAT_CUSTOM
} GthSubfolderFormat;


#define PREF_PHOTO_IMPORT_DESTINATION             "/apps/gthumb/ext/photo_importer/destination"
#define PREF_PHOTO_IMPORT_SUBFOLDER_TYPE          "/apps/gthumb/ext/photo_importer/subfolder_type"
#define PREF_PHOTO_IMPORT_SUBFOLDER_FORMAT        "/apps/gthumb/ext/photo_importer/subfolder_format"
#define PREF_PHOTO_IMPORT_SUBFOLDER_SINGLE        "/apps/gthumb/ext/photo_importer/subfolder_single"
#define PREF_PHOTO_IMPORT_SUBFOLDER_CUSTOM_FORMAT "/apps/gthumb/ext/photo_importer/subfolder_custom_format"
#define PREF_PHOTO_IMPORT_DELETE                  "/apps/gthumb/ext/photo_importer/delete_from_camera"
#define PREF_PHOTO_IMPORT_OVERWRITE               "/apps/gthumb/ext/photo_importer/overwrite_files"
#define PREF_PHOTO_IMPORT_ADJUST_ORIENTATION      "/apps/gthumb/ext/photo_importer/adjust_orientation"


G_END_DECLS

#endif /* PREFERENCES_H */
