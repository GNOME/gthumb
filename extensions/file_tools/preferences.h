/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2010 Free Software Foundation, Inc.
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

#define PREF_CROP_GRID_TYPE             "/apps/gthumb/ext/crop/grid_type"
#define PREF_CROP_ASPECT_RATIO          "/apps/gthumb/ext/crop/aspect_ratio"
#define PREF_CROP_ASPECT_RATIO_INVERT   "/apps/gthumb/ext/crop/aspect_ratio_invert"
#define PREF_CROP_ASPECT_RATIO_WIDTH    "/apps/gthumb/ext/crop/aspect_ratio_width"
#define PREF_CROP_ASPECT_RATIO_HEIGHT   "/apps/gthumb/ext/crop/aspect_ratio_height"
#define PREF_CROP_BIND_DIMENSIONS       "/apps/gthumb/ext/crop/bind_dimensions"
#define PREF_CROP_BIND_FACTOR           "/apps/gthumb/ext/crop/bind_factor"

#define PREF_RESIZE_UNIT                "/apps/gthumb/ext/resize/unit"
#define PREF_RESIZE_WIDTH               "/apps/gthumb/ext/resize/width"
#define PREF_RESIZE_HEIGHT              "/apps/gthumb/ext/resize/height"
#define PREF_RESIZE_ASPECT_RATIO_WIDTH  "/apps/gthumb/ext/resize/aspect_ratio_width"
#define PREF_RESIZE_ASPECT_RATIO_HEIGHT "/apps/gthumb/ext/resize/aspect_ratio_height"
#define PREF_RESIZE_ASPECT_RATIO        "/apps/gthumb/ext/resize/aspect_ratio"
#define PREF_RESIZE_ASPECT_RATIO_INVERT "/apps/gthumb/ext/resize/aspect_ratio_invert"
#define PREF_RESIZE_HIGH_QUALITY        "/apps/gthumb/ext/resize/high_quality"

G_END_DECLS

#endif /* PREFERENCES_H */
