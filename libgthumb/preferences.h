/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2001 The Free Software Foundation, Inc.
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


#include <glib.h>
#include "bookmarks.h"
#include "catalog-png-exporter.h"
#include "typedefs.h"
#include "image-viewer.h"

#define  PREF_GO_TO_LAST_LOCATION    "/apps/gthumb/general/go_to_last_location"
#define  PREF_USE_STARTUP_LOCATION   "/apps/gthumb/general/use_startup_location"
#define  PREF_STARTUP_LOCATION       "/apps/gthumb/general/startup_location"
#define  PREF_MAX_HISTORY_LENGTH     "/apps/gthumb/general/max_history_length"
#define  PREF_EDITORS                "/apps/gthumb/general/editors"
#define  PREF_MIGRATE_DIRECTORIES    "/apps/gthumb/general/migrate_directories"

#define  PREF_SHOW_HIDDEN_FILES      "/apps/gthumb/browser/show_hidden_files"
#define  PREF_SHOW_COMMENTS          "/apps/gthumb/browser/show_comments"
#define  PREF_SHOW_THUMBNAILS        "/apps/gthumb/browser/show_thumbnails"
#define  PREF_FAST_FILE_TYPE         "/apps/gthumb/browser/fast_file_type"
#define  PREF_SAVE_THUMBNAILS        "/apps/gthumb/browser/save_thumbnails"
#define  PREF_THUMBNAIL_SIZE         "/apps/gthumb/browser/thumbnail_size"
#define  PREF_THUMBNAIL_LIMIT        "/apps/gthumb/browser/thumbnail_limit"
#define  PREF_CLICK_POLICY           "/apps/gthumb/browser/click_policy"
#define  PREF_CONFIRM_DELETION       "/apps/gthumb/browser/confirm_deletion"
#define  PREF_ARRANGE_IMAGES         "/apps/gthumb/browser/arrange_images"
#define  PREF_SORT_IMAGES            "/apps/gthumb/browser/sort_images"

#define  PREF_ZOOM_QUALITY           "/apps/gthumb/viewer/zoom_quality"
#define  PREF_ZOOM_CHANGE            "/apps/gthumb/viewer/zoom_change"
#define  PREF_TRANSP_TYPE            "/apps/gthumb/viewer/transparency_type"
#define  PREF_CHECK_TYPE             "/apps/gthumb/viewer/check_type"
#define  PREF_CHECK_SIZE             "/apps/gthumb/viewer/check_size"

#define  PREF_SLIDESHOW_DIR          "/apps/gthumb/slideshow/direction"
#define  PREF_SLIDESHOW_DELAY        "/apps/gthumb/slideshow/delay"
#define  PREF_SLIDESHOW_WRAP_AROUND  "/apps/gthumb/slideshow/wrap_around"
#define  PREF_SLIDESHOW_FULLSCREEN   "/apps/gthumb/slideshow/fullscreen"

#define  PREF_UI_LAYOUT              "/apps/gthumb/ui/layout"
#define  PREF_UI_TOOLBAR_STYLE       "/apps/gthumb/ui/toolbar_style"
#define  PREF_UI_WINDOW_WIDTH        "/apps/gthumb/ui/window_width"
#define  PREF_UI_WINDOW_HEIGHT       "/apps/gthumb/ui/window_height"
#define  PREF_UI_IMAGE_PANE_VISIBLE  "/apps/gthumb/ui/image_pane_visible"
#define  PREF_UI_TOOLBAR_VISIBLE     "/apps/gthumb/ui/toolbar_visible"
#define  PREF_UI_STATUSBAR_VISIBLE   "/apps/gthumb/ui/statusbar_visible"
#define  PREF_UI_SIDEBAR_SIZE        "/apps/gthumb/ui/sidebar_size"
#define  PREF_UI_SIDEBAR_CONTENT_SIZE "/apps/gthumb/ui/sidebar_content_size"

#define  PREF_EXP_NAME_TEMPLATE      "/apps/gthumb/exporter/general/name_template"
#define  PREF_EXP_START_FROM         "/apps/gthumb/exporter/general/start_from"
#define  PREF_EXP_FILE_TYPE          "/apps/gthumb/exporter/general/file_type"
#define  PREF_EXP_WRITE_IMAGE_MAP    "/apps/gthumb/exporter/general/write_image_map"
#define  PREF_EXP_ARRANGE_IMAGES     "/apps/gthumb/exporter/general/arrange_images"
#define  PREF_EXP_SORT_IMAGES        "/apps/gthumb/exporter/general/sort_images"

#define  PREF_EXP_PAGE_WIDTH         "/apps/gthumb/exporter/page/width"
#define  PREF_EXP_PAGE_HEIGHT        "/apps/gthumb/exporter/page/height"
#define  PREF_EXP_PAGE_SIZE_USE_RC   "/apps/gthumb/exporter/page/size_use_row_col"
#define  PREF_EXP_PAGE_ROWS          "/apps/gthumb/exporter/page/rows"
#define  PREF_EXP_PAGE_COLS          "/apps/gthumb/exporter/page/cols"
#define  PREF_EXP_PAGE_SAME_SIZE     "/apps/gthumb/exporter/page/all_pages_same_size"
#define  PREF_EXP_PAGE_BGCOLOR       "/apps/gthumb/exporter/page/background_color"
#define  PREF_EXP_PAGE_HGRAD_COLOR1  "/apps/gthumb/exporter/page/hgrad_color1"
#define  PREF_EXP_PAGE_HGRAD_COLOR2  "/apps/gthumb/exporter/page/hgrad_color2"
#define  PREF_EXP_PAGE_VGRAD_COLOR1  "/apps/gthumb/exporter/page/vgrad_color1"
#define  PREF_EXP_PAGE_VGRAD_COLOR2  "/apps/gthumb/exporter/page/vgrad_color2"
#define  PREF_EXP_PAGE_USE_SOLID_COLOR "/apps/gthumb/exporter/page/use_solid_color"
#define  PREF_EXP_PAGE_USE_HGRADIENT "/apps/gthumb/exporter/page/use_hgradient"
#define  PREF_EXP_PAGE_USE_VGRADIENT "/apps/gthumb/exporter/page/use_vgradient"
#define  PREF_EXP_PAGE_HEADER_TEXT   "/apps/gthumb/exporter/page/header_text"
#define  PREF_EXP_PAGE_HEADER_COLOR  "/apps/gthumb/exporter/page/header_color"
#define  PREF_EXP_PAGE_HEADER_FONT   "/apps/gthumb/exporter/page/header_font"
#define  PREF_EXP_PAGE_FOOTER_TEXT   "/apps/gthumb/exporter/page/footer_text"
#define  PREF_EXP_PAGE_FOOTER_COLOR  "/apps/gthumb/exporter/page/footer_color"
#define  PREF_EXP_PAGE_FOOTER_FONT   "/apps/gthumb/exporter/page/footer_font"

#define  PREF_EXP_SHOW_COMMENT       "/apps/gthumb/exporter/thumbnail/show_comment"
#define  PREF_EXP_SHOW_PATH          "/apps/gthumb/exporter/thumbnail/show_path"
#define  PREF_EXP_SHOW_NAME          "/apps/gthumb/exporter/thumbnail/show_name"
#define  PREF_EXP_SHOW_SIZE          "/apps/gthumb/exporter/thumbnail/show_size"
#define  PREF_EXP_SHOW_IMAGE_DIM     "/apps/gthumb/exporter/thumbnail/show_image_dim"
#define  PREF_EXP_FRAME_STYLE        "/apps/gthumb/exporter/thumbnail/frame_style"
#define  PREF_EXP_FRAME_COLOR        "/apps/gthumb/exporter/thumbnail/frame_color"
#define  PREF_EXP_THUMB_SIZE         "/apps/gthumb/exporter/thumbnail/thumb_size"
#define  PREF_EXP_TEXT_COLOR         "/apps/gthumb/exporter/thumbnail/text_color"
#define  PREF_EXP_TEXT_FONT          "/apps/gthumb/exporter/thumbnail/text_font"

#define  PREF_PNG_COMPRESSION_LEVEL  "/apps/gthumb/dialogs/png_saver/compression_level"

#define  PREF_JPEG_QUALITY           "/apps/gthumb/dialogs/jpeg_saver/quality"
#define  PREF_JPEG_SMOOTHING         "/apps/gthumb/dialogs/jpeg_saver/smoothing"
#define  PREF_JPEG_OPTIMIZE          "/apps/gthumb/dialogs/jpeg_saver/optimize"
#define  PREF_JPEG_PROGRESSIVE       "/apps/gthumb/dialogs/jpeg_saver/progressive"

#define  PREF_TGA_RLE_COMPRESSION    "/apps/gthumb/dialogs/tga_saver/rle_compression"

#define  PREF_TIFF_COMPRESSION       "/apps/gthumb/dialogs/tiff_saver/compression"
#define  PREF_TIFF_HORIZONTAL_RES    "/apps/gthumb/dialogs/tiff_saver/horizontal_resolution"
#define  PREF_TIFF_VERTICAL_RES      "/apps/gthumb/dialogs/tiff_saver/vertical_resolution"

#define  PREF_CONVERT_IMAGE_TYPE      "/apps/gthumb/dialogs/convert_format/image_type"
#define  PREF_CONVERT_OVERWRITE       "/apps/gthumb/dialogs/convert_format/overwrite_mode"
#define  PREF_CONVERT_REMOVE_ORIGINAL "/apps/gthumb/dialogs/convert_format/remove_original"

#define  PREF_RENAME_SERIES_TEMPLATE  "/apps/gthumb/dialogs/rename_series/template"
#define  PREF_RENAME_SERIES_START_AT  "/apps/gthumb/dialogs/rename_series/start_at"
#define  PREF_RENAME_SERIES_SORT      "/apps/gthumb/dialogs/rename_series/sort_by"
#define  PREF_RENAME_SERIES_REVERSE   "/apps/gthumb/dialogs/rename_series/reverse_order"

#define  PREF_SCALE_UNIT              "/apps/gthumb/dialogs/scale_image/unit"
#define  PREF_SCALE_KEEP_RATIO        "/apps/gthumb/dialogs/scale_image/keep_aspect_ratio"
#define  PREF_SCALE_HIGH_QUALITY      "/apps/gthumb/dialogs/scale_image/high_quality"


typedef struct {
	Bookmarks   *bookmarks;

	/* Desktop options. */

	gboolean     menus_have_tearoff;
	gboolean     menus_have_icons;
	gboolean     toolbar_detachable;
	ClickPolicy  nautilus_click_policy;
	char        *nautilus_theme;

	/* Wallpaper options. */

	char        *wallpaper;
	char        *wallpaperAlign;
} Preferences;


void            preferences_init                      ();

void            preferences_release                   ();

const  char    *pref_util_get_file_location           (const char *location);

const  char    *pref_util_get_catalog_location        (const char *location);

const  char    *pref_util_get_search_location         (const char *location);

gboolean        pref_util_location_is_file            (const char *location);

gboolean        pref_util_location_is_catalog         (const char *location);

gboolean        pref_util_location_is_search          (const char *location);

const  char    *pref_util_remove_prefix               (const char *location);

void            pref_util_get_rgb_values              (const char *hex,
						       guint16    *r,
						       guint16    *g,
						       guint16    *b);

const char     *pref_util_get_hex_value               (guint16     r,
						       guint16     g,
						       guint16     b);

guint32         pref_util_get_int_value               (const char *hex);

ClickPolicy     pref_get_real_click_policy            ();

/* ------- */

ClickPolicy     pref_get_click_policy         ();

void            pref_set_click_policy         (ClickPolicy value);

SortMethod      pref_get_arrange_type         ();

void            pref_set_arrange_type         (SortMethod value);

GtkSortType     pref_get_sort_order           ();

void            pref_set_sort_order           (GtkSortType value);

ZoomQuality     pref_get_zoom_quality         ();

void            pref_set_zoom_quality         (ZoomQuality value);

ZoomChange      pref_get_zoom_change          ();

void            pref_set_zoom_change          (ZoomChange value);

TranspType      pref_get_transp_type          ();

void            pref_set_transp_type          (TranspType value);

CheckType       pref_get_check_type           ();

void            pref_set_check_type           (CheckType value);

CheckSize       pref_get_check_size           ();

void            pref_set_check_size           (CheckSize value);

DirectionType   pref_get_slideshow_direction  ();

void            pref_set_slideshow_direction  (DirectionType value);

ToolbarStyle    pref_get_toolbar_style        ();

void            pref_set_toolbar_style        (ToolbarStyle value);

SortMethod      pref_get_exp_arrange_type     ();

void            pref_set_exp_arrange_type     (SortMethod value);

GtkSortType     pref_get_exp_sort_order       ();

void            pref_set_exp_sort_order       (GtkSortType value);

FrameStyle      pref_get_exporter_frame_style ();

void            pref_set_exporter_frame_style (FrameStyle value);

OverwriteMode   pref_get_convert_overwrite_mode ();

void            pref_set_convert_overwrite_mode (OverwriteMode value);

GtkSortType     pref_get_rename_sort_order      ();

void            pref_set_rename_sort_order      (GtkSortType value);


#endif /* PREFERENCES_H */
