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

#ifndef IMPORTER_PREFERENCES_H
#define IMPORTER_PREFERENCES_H

#include <gthumb.h>

G_BEGIN_DECLS

#define PREF_IMPORT_DESTINATION             "/apps/gthumb/ext/importer/destination"
#define PREF_IMPORT_SUBFOLDER_TYPE          "/apps/gthumb/ext/importer/subfolder_type"
#define PREF_IMPORT_SUBFOLDER_FORMAT        "/apps/gthumb/ext/importer/subfolder_format"
#define PREF_IMPORT_SUBFOLDER_SINGLE        "/apps/gthumb/ext/importer/subfolder_single"
#define PREF_IMPORT_SUBFOLDER_CUSTOM_FORMAT "/apps/gthumb/ext/importer/subfolder_custom_format"
#define PREF_IMPORT_OVERWRITE               "/apps/gthumb/ext/importer/overwrite_files"
#define PREF_IMPORT_ADJUST_ORIENTATION      "/apps/gthumb/ext/importer/adjust_orientation"

G_END_DECLS

#endif /* IMPORTER_PREFERENCES_H */
