/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2001-2008 The Free Software Foundation, Inc.
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

#ifndef TYPEDEFS_H
#define TYPEDEFS_H

#include <gtk/gtk.h>

G_BEGIN_DECLS


#define BOOKMARKS_FILE "bookmarks.xbel"
#define FILTERS_FILE   "filters.xml"
#define TAGS_FILE      "tags.xml"
#define FILE_CACHE     "cache"
#define SHORTCUTS_FILE "shortcuts.xml"


typedef enum {
	GTH_CLICK_POLICY_NAUTILUS,
	GTH_CLICK_POLICY_SINGLE,
	GTH_CLICK_POLICY_DOUBLE
} GthClickPolicy;


typedef enum {
	GTH_DIRECTION_FORWARD,
	GTH_DIRECTION_REVERSE,
	GTH_DIRECTION_RANDOM
} GthDirection;


/* The GthTransform numeric values range from 1 to 8, corresponding to
 * the valid range of Exif orientation tags.  The name associated with each
 * numeric valid describes the data transformation required that will allow
 * the orientation value to be reset to "1" without changing the displayed
 * image.
 * GthTransform and ExifShort values are interchangeably in a number of
 * places.  The main difference is that ExifShort can have a value of zero,
 * corresponding to an error or an absence of an Exif orientation tag.
 * See bug 361913 for additional details. */
typedef enum {
	GTH_TRANSFORM_NONE = 1,         /* no transformation */
	GTH_TRANSFORM_FLIP_H,           /* horizontal flip */
	GTH_TRANSFORM_ROTATE_180,       /* 180-degree rotation */
	GTH_TRANSFORM_FLIP_V,           /* vertical flip */
	GTH_TRANSFORM_TRANSPOSE,        /* transpose across UL-to-LR axis (= rotate_90 + flip_h) */
	GTH_TRANSFORM_ROTATE_90,        /* 90-degree clockwise rotation */
	GTH_TRANSFORM_TRANSVERSE,       /* transpose across UR-to-LL axis (= rotate_90 + flip_v) */
	GTH_TRANSFORM_ROTATE_270        /* 270-degree clockwise */
} GthTransform;


typedef enum {
	GTH_UNIT_PIXELS,
        GTH_UNIT_PERCENTAGE
} GthUnit;


typedef enum {
	GTH_METRIC_PIXELS,
        GTH_METRIC_MILLIMETERS,
        GTH_METRIC_INCHES
} GthMetric;


typedef enum {
	GTH_ASPECT_RATIO_NONE = 0,
	GTH_ASPECT_RATIO_SQUARE,
	GTH_ASPECT_RATIO_IMAGE,
	GTH_ASPECT_RATIO_DISPLAY,
	GTH_ASPECT_RATIO_5x4,
	GTH_ASPECT_RATIO_4x3,
	GTH_ASPECT_RATIO_7x5,
	GTH_ASPECT_RATIO_3x2,
	GTH_ASPECT_RATIO_16x10,
	GTH_ASPECT_RATIO_16x9,
	GTH_ASPECT_RATIO_185x100,
	GTH_ASPECT_RATIO_191x100,
	GTH_ASPECT_RATIO_239x100,
	GTH_ASPECT_RATIO_CUSTOM
} GthAspectRatio;


typedef enum {
	GTH_OVERWRITE_SKIP,
	GTH_OVERWRITE_RENAME,
	GTH_OVERWRITE_ASK,
	GTH_OVERWRITE_OVERWRITE
} GthOverwriteMode;


typedef enum {
	GTH_GRID_NONE = 0,
	GTH_GRID_THIRDS,
	GTH_GRID_GOLDEN,
	GTH_GRID_CENTER,
	GTH_GRID_UNIFORM
} GthGridType;


typedef enum {
	GTH_THUMBNAIL_STATE_DEFAULT = 0,
	GTH_THUMBNAIL_STATE_CREATING,
	GTH_THUMBNAIL_STATE_CREATED,
	GTH_THUMBNAIL_STATE_LOADING,
	GTH_THUMBNAIL_STATE_ERROR,
	GTH_THUMBNAIL_STATE_LOADED
} GthThumbnailState;


typedef enum /*< skip >*/ {
        GTH_CHANNEL_RED   = 0,
        GTH_CHANNEL_GREEN = 1,
        GTH_CHANNEL_BLUE  = 2,
        GTH_CHANNEL_ALPHA = 3
} GthChannel;


typedef enum /*< skip >*/ {
	GTH_COLOR_SPACE_UNKNOWN,
	GTH_COLOR_SPACE_SRGB,
	GTH_COLOR_SPACE_UNCALIBRATED
} GthColorSpace;


typedef enum /*< skip >*/ {
	GTH_ZOOM_IN,
	GTH_ZOOM_OUT
} GthZoomType;


typedef enum /*< skip >*/ {
	/* Shortcut handled by Gtk, not customizable, specified for
	 * documentation. */
	GTH_SHORTCUT_CONTEXT_INTERNAL = 1 << 1,

	/* Shortcut handled in gth_window_activate_shortcut, not customizable,
	 * specified for documentation. */
	GTH_SHORTCUT_CONTEXT_FIXED = 1 << 2,

	/* Shortcut available when the window is in browser mode. */
	GTH_SHORTCUT_CONTEXT_BROWSER = 1 << 3,

	/* Shortcut available when the window is in viewer mode. */
	GTH_SHORTCUT_CONTEXT_VIEWER = 1 << 4,

	/* Shortcut available when the window is in editor mode. */
	GTH_SHORTCUT_CONTEXT_EDITOR = 1 << 5,

	/* Shortcut available in slideshows. */
	GTH_SHORTCUT_CONTEXT_SLIDESHOW = 1 << 6,

	/* Entry used for documentation only. */
	GTH_SHORTCUT_CONTEXT_DOC = 1 << 7,

	/* Aggregated values: */

	GTH_SHORTCUT_CONTEXT_BROWSER_VIEWER = (GTH_SHORTCUT_CONTEXT_BROWSER | GTH_SHORTCUT_CONTEXT_VIEWER),
	GTH_SHORTCUT_CONTEXT_ANY = (GTH_SHORTCUT_CONTEXT_BROWSER | GTH_SHORTCUT_CONTEXT_VIEWER | GTH_SHORTCUT_CONTEXT_EDITOR | GTH_SHORTCUT_CONTEXT_SLIDESHOW),
} GthShortcutContext;


typedef void (*DataFunc)         (gpointer    user_data);
typedef void (*ReadyFunc)        (GError     *error,
			 	  gpointer    user_data);
typedef void (*ReadyCallback)    (GObject    *object,
				  GError     *error,
			   	  gpointer    user_data);
typedef void (*ProgressCallback) (GObject    *object,
				  const char *description,
				  const char *details,
			          gboolean    pulse,
			          double      fraction,
			   	  gpointer    user_data);
typedef void (*DialogCallback)   (gboolean    opened,
				  GtkWidget  *dialog,
				  gpointer    user_data);

G_END_DECLS

#endif /* TYPEDEFS_H */
