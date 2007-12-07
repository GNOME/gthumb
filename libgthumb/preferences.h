/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2001, 2003 Free Software Foundation, Inc.
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
#include <gtk/gtkwindow.h>
#include "bookmarks.h"
#include "typedefs.h"
#include "image-viewer.h"


#define PREF_DESKTOP_ICON_THEME         "/desktop/gnome/file_views/icon_theme"
#define PREF_DESKTOP_MENUS_HAVE_TEAROFF "/desktop/gnome/interface/menus_have_tearoff"
#define PREF_DESKTOP_MENUBAR_DETACHABLE "/desktop/gnome/interface/menubar_detachable"
#define PREF_DESKTOP_TOOLBAR_DETACHABLE "/desktop/gnome/interface/toolbar_detachable"

#define  PREF_GO_TO_LAST_LOCATION    "/apps/gthumb/general/go_to_last_location"
#define  PREF_USE_STARTUP_LOCATION   "/apps/gthumb/general/use_startup_location"
#define  PREF_STARTUP_LOCATION       "/apps/gthumb/general/startup_location"
#define  PREF_MAX_HISTORY_LENGTH     "/apps/gthumb/general/max_history_length"
#define  PREF_EDITORS                "/apps/gthumb/general/editors"
#define  PREF_MIGRATE_DIRECTORIES    "/apps/gthumb/general/migrate_directories"
#define  PREF_MIGRATE_COMMENT_SYSTEM "/apps/gthumb/general/migrate_comment_system"

#define  PREF_SHOW_HIDDEN_FILES      "/apps/gthumb/browser/show_hidden_files"
#define  PREF_SHOW_FILENAMES         "/apps/gthumb/browser/show_filenames"
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
#define  PREF_VIEW_AS                "/apps/gthumb/browser/view_as"
#define  PREF_SHOW_PREVIEW           "/apps/gthumb/browser/show_preview"
#define  PREF_PREVIEW_CONTENT        "/apps/gthumb/browser/preview_content"
#define  PREF_SHOW_IMAGE_DATA        "/apps/gthumb/browser/show_image_data"

#define  PREF_ZOOM_QUALITY           "/apps/gthumb/viewer/zoom_quality"
#define  PREF_ZOOM_CHANGE            "/apps/gthumb/viewer/zoom_change"
#define  PREF_TRANSP_TYPE            "/apps/gthumb/viewer/transparency_type"
#define  PREF_RESET_SCROLLBARS       "/apps/gthumb/viewer/reset_scrollbars"
#define  PREF_CHECK_TYPE             "/apps/gthumb/viewer/check_type"
#define  PREF_CHECK_SIZE             "/apps/gthumb/viewer/check_size"
#define  PREF_BLACK_BACKGROUND       "/apps/gthumb/viewer/black_background"
#define  PREF_SINGLE_WINDOW          "/apps/gthumb/viewer/single_window"

#define  PREF_SLIDESHOW_DIR          "/apps/gthumb/slideshow/direction"
#define  PREF_SLIDESHOW_DELAY        "/apps/gthumb/slideshow/change_delay"
#define  PREF_SLIDESHOW_WRAP_AROUND  "/apps/gthumb/slideshow/wrap_around"
#define  PREF_SLIDESHOW_FADING       "/apps/gthumb/slideshow/fading"

#define  PREF_HOTKEY0                "/apps/gthumb/hotkeys/hotkey0"
#define  PREF_HOTKEY1                "/apps/gthumb/hotkeys/hotkey1"
#define  PREF_HOTKEY2                "/apps/gthumb/hotkeys/hotkey2"
#define  PREF_HOTKEY3                "/apps/gthumb/hotkeys/hotkey3"
#define  PREF_HOTKEY4                "/apps/gthumb/hotkeys/hotkey4"
#define  PREF_HOTKEY5                "/apps/gthumb/hotkeys/hotkey5"
#define  PREF_HOTKEY6                "/apps/gthumb/hotkeys/hotkey6"
#define  PREF_HOTKEY7                "/apps/gthumb/hotkeys/hotkey7"
#define  PREF_HOTKEY8                "/apps/gthumb/hotkeys/hotkey8"
#define  PREF_HOTKEY9                "/apps/gthumb/hotkeys/hotkey9"

#define  PREF_UI_LAYOUT              "/apps/gthumb/ui/layout"
#define  PREF_UI_TOOLBAR_STYLE       "/apps/gthumb/ui/toolbar_style"
#define  PREF_UI_WINDOW_WIDTH        "/apps/gthumb/ui/window_width"
#define  PREF_UI_WINDOW_HEIGHT       "/apps/gthumb/ui/window_height"
#define  PREF_UI_VIEWER_WIDTH        "/apps/gthumb/ui/viewer_width"
#define  PREF_UI_VIEWER_HEIGHT       "/apps/gthumb/ui/viewer_height"
#define  PREF_UI_IMAGE_PANE_VISIBLE  "/apps/gthumb/ui/image_pane_visible"
#define  PREF_UI_TOOLBAR_VISIBLE     "/apps/gthumb/ui/toolbar_visible"
#define  PREF_UI_STATUSBAR_VISIBLE   "/apps/gthumb/ui/statusbar_visible"
#define  PREF_UI_FILTERBAR_VISIBLE   "/apps/gthumb/ui/filterbar_visible"
#define  PREF_UI_SIDEBAR_SIZE        "/apps/gthumb/ui/sidebar_size"
#define  PREF_UI_SIDEBAR_CONTENT_SIZE "/apps/gthumb/ui/sidebar_content_size"
#define  PREF_UI_COMMENT_PANE_SIZE   "/apps/gthumb/ui/comment_pane_size"

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

#define  PREF_EXP_WEB_INDEX_FILE       "/apps/gthumb/exporter/web_album/index_file"
#define  PREF_EXP_WEB_DIR_PREVIEWS     "/apps/gthumb/exporter/web_album/directories/previews"
#define  PREF_EXP_WEB_DIR_THUMBNAILS   "/apps/gthumb/exporter/web_album/directories/thumbnails"
#define  PREF_EXP_WEB_DIR_IMAGES       "/apps/gthumb/exporter/web_album/directories/images"
#define  PREF_EXP_WEB_DIR_HTML_IMAGES  "/apps/gthumb/exporter/web_album/directories/html_images"
#define  PREF_EXP_WEB_DIR_HTML_INDEXES "/apps/gthumb/exporter/web_album/directories/html_indexes"
#define  PREF_EXP_WEB_DIR_THEME_FILES  "/apps/gthumb/exporter/web_album/directories/theme_files"

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
#define  PREF_RENAME_SERIES_CHANGE_CASE "/apps/gthumb/dialogs/rename_series/change_case"

#define  PREF_SCALE_UNIT              "/apps/gthumb/dialogs/scale_image/unit"
#define  PREF_SCALE_KEEP_RATIO        "/apps/gthumb/dialogs/scale_image/keep_aspect_ratio"
#define  PREF_SCALE_HIGH_QUALITY      "/apps/gthumb/dialogs/scale_image/high_quality"

#define  PREF_SCALE_SERIES_WIDTH      "/apps/gthumb/dialogs/scale_series/width"
#define  PREF_SCALE_SERIES_HEIGHT     "/apps/gthumb/dialogs/scale_series/height"

#define  PREF_WEB_ALBUM_DESTINATION    "/apps/gthumb/dialogs/web_album/destination"
#define  PREF_WEB_ALBUM_DESTINATIONS   "/apps/gthumb/dialogs/web_album/destinations"
#define  PREF_WEB_ALBUM_USE_SUBFOLDERS "/apps/gthumb/dialogs/web_album/use_subfolders"
#define  PREF_WEB_ALBUM_COPY_IMAGES    "/apps/gthumb/dialogs/web_album/copy_images"
#define  PREF_WEB_ALBUM_RESIZE_IMAGES  "/apps/gthumb/dialogs/web_album/resize_images"
#define  PREF_WEB_ALBUM_RESIZE_WIDTH   "/apps/gthumb/dialogs/web_album/resize_width"
#define  PREF_WEB_ALBUM_RESIZE_HEIGHT  "/apps/gthumb/dialogs/web_album/resize_height"
#define  PREF_WEB_ALBUM_ROWS           "/apps/gthumb/dialogs/web_album/rows"
#define  PREF_WEB_ALBUM_COLUMNS        "/apps/gthumb/dialogs/web_album/columns"
#define  PREF_WEB_ALBUM_SINGLE_INDEX   "/apps/gthumb/dialogs/web_album/single_index"
#define  PREF_WEB_ALBUM_SORT           "/apps/gthumb/dialogs/web_album/sort_by"
#define  PREF_WEB_ALBUM_REVERSE        "/apps/gthumb/dialogs/web_album/reverse_order"
#define  PREF_WEB_ALBUM_HEADER         "/apps/gthumb/dialogs/web_album/header"
#define  PREF_WEB_ALBUM_FOOTER         "/apps/gthumb/dialogs/web_album/footer"
#define  PREF_WEB_ALBUM_THEME          "/apps/gthumb/dialogs/web_album/theme"
#define  PREF_WEB_ALBUM_INDEX_CAPTION  "/apps/gthumb/dialogs/web_album/index_caption"
#define  PREF_WEB_ALBUM_IMAGE_CAPTION  "/apps/gthumb/dialogs/web_album/image_caption"

#define  PREF_SEARCH_RECURSIVE        "/apps/gthumb/dialogs/search/recursive"

#define  PREF_PRINT_PAPER_SIZE        "/apps/gthumb/dialogs/print/paper_size"
#define  PREF_PRINT_PAPER_ORIENTATION "/apps/gthumb/dialogs/print/print_paper_orientation"
#define  PREF_PRINT_INCLUDE_COMMENT   "/apps/gthumb/dialogs/print/include_comment"
#define  PREF_PRINT_INCLUDE_FILENAME  "/apps/gthumb/dialogs/print/include_filename"
#define  PREF_PRINT_COMMENT_FONT      "/apps/gthumb/dialogs/print/comment_font"
#define  PREF_PRINT_PAPER_WIDTH       "/apps/gthumb/dialogs/print/paper_width"
#define  PREF_PRINT_PAPER_HEIGHT      "/apps/gthumb/dialogs/print/paper_height"
#define  PREF_PRINT_MARGIN_LEFT       "/apps/gthumb/dialogs/print/margin_left"
#define  PREF_PRINT_MARGIN_RIGHT      "/apps/gthumb/dialogs/print/margin_right"
#define  PREF_PRINT_MARGIN_TOP        "/apps/gthumb/dialogs/print/margin_top"
#define  PREF_PRINT_MARGIN_BOTTOM     "/apps/gthumb/dialogs/print/margin_bottom"
#define  PREF_PRINT_PAPER_UNIT        "/apps/gthumb/dialogs/print/paper_unit"
#define  PREF_PRINT_IMAGES_PER_PAGE   "/apps/gthumb/dialogs/print/images_per_page"
#define  PREF_PRINT_IMAGE_UNIT	      "/apps/gthumb/dialogs/print/image_unit"
#define  PREF_PRINT_IMAGE_SIZING      "/apps/gthumb/dialogs/print/image_sizing"
#define  PREF_PRINT_IMAGE_WIDTH	      "/apps/gthumb/dialogs/print/image_width"
#define  PREF_PRINT_IMAGE_HEIGHT      "/apps/gthumb/dialogs/print/image_height"
#define  PREF_PRINT_IMAGE_RESOLUTION  "/apps/gthumb/dialogs/print/image_resolution"


#define  PREF_MSG_CANNOT_MOVE_TO_TRASH "/apps/gthumb/dialogs/messages/cannot_move_to_trash"
#define  PREF_MSG_SAVE_MODIFIED_IMAGE  "/apps/gthumb/dialogs/messages/save_modified_image"
#define  PREF_MSG_JPEG_MCU_WARNING     "/apps/gthumb/dialogs/messages/jpeg_mcu_warning"

#define  PREF_CHOOSE_DESTINATION_VIEW  "/apps/gthumb/dialogs/choose_destination/view"

#define  PREF_PHOTO_IMPORT_DESTINATION    "/apps/gthumb/dialogs/photo_importer/destination"
#define  PREF_PHOTO_IMPORT_SUBFOLDER      "/apps/gthumb/dialogs/photo_importer/subfolder"
#define  PREF_PHOTO_IMPORT_CUSTOM_FORMAT  "/apps/gthumb/dialogs/photo_importer/custom_format_code"
#define  PREF_PHOTO_IMPORT_DELETE         "/apps/gthumb/dialogs/photo_importer/delete_from_camera"
#define  PREF_PHOTO_IMPORT_KEEP_FILENAMES "/apps/gthumb/dialogs/photo_importer/keep_original_filenames"
#define  PREF_PHOTO_IMPORT_RESET_EXIF_ORIENTATION	"/apps/gthumb/dialogs/photo_importer/reset_exif_orientation_on_import"
#define  PREF_PHOTO_IMPORT_MODEL          "/apps/gthumb/dialogs/photo_importer/model"
#define  PREF_PHOTO_IMPORT_PORT           "/apps/gthumb/dialogs/photo_importer/port"

#define  PREF_CROP_ASPECT_RATIO           "/apps/gthumb/dialogs/crop/aspect_ratio"
#define  PREF_CROP_ASPECT_RATIO_WIDTH     "/apps/gthumb/dialogs/crop/aspect_ratio_width"
#define  PREF_CROP_ASPECT_RATIO_HEIGHT    "/apps/gthumb/dialogs/crop/aspect_ratio_height"
#define  PREF_ADD_TO_CATALOG_LAST_CATALOG "/apps/gthumb/dialogs/add_to_catalog/last_catalog"
#define  PREF_ADD_TO_CATALOG_VIEW         "/apps/gthumb/dialogs/add_to_catalog/view"

#define  PREF_ROTATE_RESET_EXIF_ORIENTATION     "/apps/gthumb/dialogs/rotate/reset_exif_orientation_on_rotate"

typedef struct {
	Bookmarks   *bookmarks;

	/* Desktop options. */

	gboolean     menus_have_tearoff;
	gboolean     menus_have_icons;
	gboolean     toolbar_detachable;
	GthClickPolicy  nautilus_click_policy;
	char        *nautilus_theme;
	char        *startup_location;

	/* Wallpaper options. */

	char        *wallpaper;
	char        *wallpaperAlign;
} Preferences;


void            preferences_init                      (void);
void            preferences_release                   (void);
void            preferences_set_startup_location      (const char *location);
const char *    preferences_get_startup_location      (void);
void            pref_util_get_rgb_values              (const char *hex,
						       guint16    *r,
						       guint16    *g,
						       guint16    *b);
void            pref_util_get_color_from_hex          (const char *hex,
						       GdkColor   *color);
const char     *pref_util_get_hex_value               (guint16     r,
						       guint16     g,
						       guint16     b);
guint32         pref_util_get_int_value               (const char *hex);
guint32         pref_util_get_ui32_from_color         (GdkColor   *color);
GthClickPolicy  pref_get_real_click_policy            (void);
GthToolbarStyle pref_get_real_toolbar_style           (void);
void            pref_util_save_window_geometry        (GtkWindow  *window,
						       const char *dialog);
void            pref_util_restore_window_geometry     (GtkWindow  *window,
						       const char *dialog);

/* ------- */

GthClickPolicy     pref_get_click_policy           (void);
void               pref_set_click_policy           (GthClickPolicy value);
GthSortMethod      pref_get_arrange_type           (void);
void               pref_set_arrange_type           (GthSortMethod value);
GtkSortType        pref_get_sort_order             (void);
void               pref_set_sort_order             (GtkSortType value);
GthZoomQuality     pref_get_zoom_quality           (void);
void               pref_set_zoom_quality           (GthZoomQuality value);
GthZoomChange      pref_get_zoom_change            (void);
void               pref_set_zoom_change            (GthZoomChange value);
GthTranspType      pref_get_transp_type            (void);
void               pref_set_transp_type            (GthTranspType value);
GthCheckType       pref_get_check_type             (void);
void               pref_set_check_type             (GthCheckType value);
GthCheckSize       pref_get_check_size             (void);
void               pref_set_check_size             (GthCheckSize value);
GthDirectionType   pref_get_slideshow_direction    (void);
void               pref_set_slideshow_direction    (GthDirectionType value);
GthToolbarStyle    pref_get_toolbar_style          (void);
void               pref_set_toolbar_style          (GthToolbarStyle value);
GthSortMethod      pref_get_exp_arrange_type       (void);
void               pref_set_exp_arrange_type       (GthSortMethod value);
GtkSortType        pref_get_exp_sort_order         (void);
void               pref_set_exp_sort_order         (GtkSortType value);
GthFrameStyle      pref_get_exporter_frame_style   (void);
void               pref_set_exporter_frame_style   (GthFrameStyle value);
GthOverwriteMode   pref_get_convert_overwrite_mode (void);
void               pref_set_convert_overwrite_mode (GthOverwriteMode value);
GthChangeCase      pref_get_change_case		   (void);
void               pref_set_change_case            (GthChangeCase value);
GthSortMethod      pref_get_rename_sort_order      (void);
void               pref_set_rename_sort_order      (GthSortMethod value);
GthSortMethod      pref_get_web_album_sort_order   (void);
void               pref_set_web_album_sort_order   (GthSortMethod value);
GthViewAs          pref_get_view_as                (void);
void               pref_set_view_as                (GthViewAs value);
void               pref_set_preview_content        (GthPreviewContent value);
GthPreviewContent  pref_get_preview_content        (void);
GthViewMode        pref_get_view_mode              (void);
void               pref_set_print_unit             (GthPrintUnit value);
GthPrintUnit       pref_get_print_unit             (void);
void               pref_set_image_unit             (GthPrintUnit value);
GthPrintUnit       pref_get_image_unit             (void);
void               pref_set_image_sizing           (GthImageSizing value);
GthImageSizing     pref_get_image_sizing           (void);
void               pref_set_image_resolution       (GthImageResolution value);
GthImageResolution pref_get_image_resolution       (void);
void               pref_set_crop_ratio             (GthCropRatio value);
GthCropRatio       pref_get_crop_ratio             (void);
GthSubFolder       pref_get_import_subfolder       (void);
void               pref_set_import_subfolder       (GthSubFolder value);

#endif /* PREFERENCES_H */
