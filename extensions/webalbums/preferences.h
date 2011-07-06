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
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef PREFERENCES_H
#define PREFERENCES_H

#include <gthumb.h>

G_BEGIN_DECLS

#define PREF_WEBALBUMS_INDEX_FILE               "/apps/gthumb/ext/webalbums/index_file"
#define PREF_WEBALBUMS_DIR_PREVIEWS             "/apps/gthumb/ext/webalbums/directories/previews"
#define PREF_WEBALBUMS_DIR_THUMBNAILS           "/apps/gthumb/ext/webalbums/directories/thumbnails"
#define PREF_WEBALBUMS_DIR_IMAGES               "/apps/gthumb/ext/webalbums/directories/images"
#define PREF_WEBALBUMS_DIR_HTML_IMAGES          "/apps/gthumb/ext/webalbums/directories/html_images"
#define PREF_WEBALBUMS_DIR_HTML_INDEXES         "/apps/gthumb/ext/webalbums/directories/html_indexes"
#define PREF_WEBALBUMS_DIR_THEME_FILES          "/apps/gthumb/ext/webalbums/directories/theme_files"
#define PREF_WEBALBUMS_DESTINATION              "/apps/gthumb/ext/webalbums/destination"
#define PREF_WEBALBUMS_COPY_IMAGES              "/apps/gthumb/ext/webalbums/copy_images"
#define PREF_WEBALBUMS_RESIZE_IMAGES            "/apps/gthumb/ext/webalbums/resize_images"
#define PREF_WEBALBUMS_RESIZE_WIDTH             "/apps/gthumb/ext/webalbums/resize_width"
#define PREF_WEBALBUMS_RESIZE_HEIGHT            "/apps/gthumb/ext/webalbums/resize_height"
#define PREF_WEBALBUMS_IMAGES_PER_INDEX         "/apps/gthumb/ext/webalbums/images_per_index"
#define PREF_WEBALBUMS_SINGLE_INDEX             "/apps/gthumb/ext/webalbums/single_index"
#define PREF_WEBALBUMS_COLUMNS                  "/apps/gthumb/ext/webalbums/columns"
#define PREF_WEBALBUMS_ADAPT_TO_WIDTH           "/apps/gthumb/ext/webalbums/adapt_to_width"
#define PREF_WEBALBUMS_SORT_TYPE                "/apps/gthumb/ext/webalbums/sort_type"
#define PREF_WEBALBUMS_SORT_INVERSE             "/apps/gthumb/ext/webalbums/sort_inverse"
#define PREF_WEBALBUMS_HEADER                   "/apps/gthumb/ext/webalbums/header"
#define PREF_WEBALBUMS_FOOTER                   "/apps/gthumb/ext/webalbums/footer"
#define PREF_WEBALBUMS_IMAGE_PAGE_HEADER        "/apps/gthumb/ext/webalbums/image_page_header"
#define PREF_WEBALBUMS_IMAGE_PAGE_FOOTER        "/apps/gthumb/ext/webalbums/image_page_footer"
#define PREF_WEBALBUMS_THEME                    "/apps/gthumb/ext/webalbums/theme"
#define PREF_WEBALBUMS_ENABLE_THUMBNAIL_CAPTION "/apps/gthumb/ext/webalbums/enable_thumbnail_caption"
#define PREF_WEBALBUMS_THUMBNAIL_CAPTION        "/apps/gthumb/ext/webalbums/thumbnail_caption"
#define PREF_WEBALBUMS_ENABLE_IMAGE_DESCRIPTION "/apps/gthumb/ext/webalbums/enable_image_description"
#define PREF_WEBALBUMS_ENABLE_IMAGE_ATTRIBUTES  "/apps/gthumb/ext/webalbums/enable_image_attributes"
#define PREF_WEBALBUMS_IMAGE_ATTRIBUTES         "/apps/gthumb/ext/webalbums/image_attributes"

G_END_DECLS

#endif /* PREFERENCES_H */
