/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2004 Free Software Foundation, Inc.
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

#ifndef UI_H
#define UI_H


#include <config.h>
#include <gnome.h>
#include "actions.h"
#include "gthumb-stock.h"


static GtkActionEntry action_entries[] = {
	{ "FileMenu", NULL, N_("_File") },
	{ "FileFolderMenu", NULL, N_("_Folder") },
	{ "FileCatalogMenu", NULL, N_("Ca_talog") },
	{ "FileSearchMenu", NULL, N_("Ca_talog") },
	{ "EditMenu", NULL, N_("_Edit") },
	{ "ViewMenu", NULL, N_("_View") },
	{ "ViewShowHideMenu", NULL, N_("Show/_Hide") },
	{ "ViewSortMenu", NULL, N_("S_ort Images") },
	{ "ViewZoomMenu", NULL, N_("_Zoom") },
	{ "GoMenu", NULL, N_("_Go") },
	{ "ImageMenu", NULL, N_("_Image") },
	{ "ImageTransformMenu", NULL, N_("_Transform") },
	{ "ImageAutoMenu", NULL, N_("Auto") },
	{ "BookmarksMenu", NULL, N_("_Bookmarks") },
	{ "ToolsMenu", NULL, N_("_Tools") },
	{ "ToolsWallpaperMenu", NULL, N_("Set Image as _Wallpaper") },
	{ "HelpMenu", NULL, N_("_Help") },

	{ "File_NewWindow", GTK_STOCK_NEW,
	  N_("New _Window"), NULL,
	  N_("Create a new window"),
	  G_CALLBACK (activate_action_file_new_window) },
	
	{ "File_CloseWindow", GTK_STOCK_CLOSE,
	  N_("_Close"), "<control>W",
	  N_("Close this window"),
	  G_CALLBACK (activate_action_file_close_window) },

	{ "File_OpenWith", GTK_STOCK_OPEN,
	  N_("_Open With"), "<control>O",
	  N_("Open selected images with an application"),
	  G_CALLBACK (activate_action_file_open_with) },

	{ "File_Save", GTK_STOCK_SAVE_AS,
	  N_("Save _As..."), "<shift><control>S",
	  N_("Save current image"),
	  G_CALLBACK (activate_action_file_save) },

	{ "File_Revert", GTK_STOCK_REVERT_TO_SAVED,
	  N_("_Revert"), NULL,
	  N_("Revert to saved image"),
	  G_CALLBACK (activate_action_file_revert) },

	{ "File_Print", GTK_STOCK_PRINT,
	  N_("_Print..."), "<control>P",
	  N_("Print the current image"),
	  G_CALLBACK (activate_action_file_print) },

	{ "File_ImageProp", GTHUMB_STOCK_PROPERTIES,
	  N_("Proper_ties"), NULL,
	  N_("View image properties"),
	  G_CALLBACK (activate_action_file_image_properties) },

	{ "File_CameraImport", GTHUMB_STOCK_CAMERA,
	  N_("_Import Photos..."), NULL,
	  N_("Import photos from a digital camera"),
	  G_CALLBACK (activate_action_file_camera_import) },

	{ "File_WriteToCD", GTHUMB_STOCK_CDROM,
	  N_("_Write To CD"), NULL,
	  N_("Write selection to CD"),
	  G_CALLBACK (activate_action_file_write_to_cd) },

	{ "Image_OpenWith", GTK_STOCK_OPEN,
	  N_("_Open With"), NULL,
	  N_("Open this image with an application"),
	  G_CALLBACK (activate_action_image_open_with) },

	{ "Image_Rename", NULL,
	  N_("_Rename..."), NULL,
	  N_("Rename this image"),
	  G_CALLBACK (activate_action_image_rename) },

	{ "Image_Duplicate", NULL,
	  N_("D_uplicate"), NULL,
	  N_("Duplicate this image"),
	  G_CALLBACK (activate_action_image_duplicate) },

	{ "Image_Delete", GTK_STOCK_DELETE,
	  N_("Move to _Trash"), NULL,
	  N_("Move this image to the Trash"),
	  G_CALLBACK (activate_action_image_delete) },

	{ "Image_Copy", NULL,
	  N_("_Copy..."), NULL,
	  N_("Copy this image to another location"),
	  G_CALLBACK (activate_action_image_copy) },

	{ "Image_Move", NULL,
	  N_("_Move..."), NULL,
	  N_("Move this image to another location"),
	  G_CALLBACK (activate_action_image_move) },

	{ "Edit_RenameFile", NULL,
	  N_("_Rename..."), "F2",
	  N_("Rename selected image"),
	  G_CALLBACK (activate_action_edit_rename_file) },

	{ "Edit_DuplicateFile", NULL,
	  N_("D_uplicate"), NULL,
	  N_("Duplicate selected image"),
	  G_CALLBACK (activate_action_edit_duplicate_file) },

	{ "Edit_DeleteFiles", GTK_STOCK_DELETE,
	  N_("Move to _Trash"), NULL,
	  N_("Move the selected images to the Trash"),
	  G_CALLBACK (activate_action_edit_delete_files) },

	{ "Edit_CopyFiles", NULL,
	  N_("_Copy..."), NULL,
	  N_("Copy selected images to another location"),
	  G_CALLBACK (activate_action_edit_copy_files) },

	{ "Edit_MoveFiles", NULL,
	  N_("_Move..."), NULL,
	  N_("Move selected images to another location"),
	  G_CALLBACK (activate_action_edit_move_files) },

	{ "Edit_SelectAll", NULL,
	  N_("_Select All"), NULL,
	  N_("Select all images"),
	  G_CALLBACK (activate_action_edit_select_all) },

	{ "Edit_EditComment", GTHUMB_STOCK_ADD_COMMENT,
	  N_("Comm_ent"), NULL,
	  N_("Add a comment to selected images"),
	  G_CALLBACK (activate_action_edit_edit_comment) },

	{ "Edit_DeleteComment", NULL,
	  N_("Rem_ove Comment"), NULL,
	  N_("Remove comments of selected images"),
	  G_CALLBACK (activate_action_edit_delete_comment) },

	{ "Edit_EditCategories", GTK_STOCK_INDEX,
	  N_("Ca_tegories"), NULL,
	  N_("Assign categories to selected images"),
	  G_CALLBACK (activate_action_edit_edit_categories) },

	{ "Edit_AddToCatalog", GTHUMB_STOCK_ADD_TO_CATALOG,
	  N_("_Add to Catalog..."), NULL,
	  N_("Add selected images to a catalog"),
	  G_CALLBACK (activate_action_edit_add_to_catalog) },

	{ "Edit_RemoveFromCatalog", NULL,
	  N_("Remo_ve from Catalog"), NULL,
	  N_("Remove selected images from the catalog"),
	  G_CALLBACK (activate_action_edit_remove_from_catalog) },

	{ "EditCatalog_Rename", NULL,
	  N_("_Rename..."), NULL,
	  N_("Rename selected catalog"),
	  G_CALLBACK (activate_action_edit_catalog_rename) },

	{ "EditCatalog_Delete", NULL,
	  N_("Rem_ove"), NULL,
	  N_("Remove selected catalog"),
	  G_CALLBACK (activate_action_edit_catalog_delete) },

	{ "EditCatalog_Move", NULL,
	  N_("_Move..."), NULL,
	  N_("Move selected catalog to another location"),
	  G_CALLBACK (activate_action_edit_catalog_move) },

	{ "EditCatalog_EditSearch", NULL,
	  N_("_Edit Search"), NULL,
	  N_("Modify search criteria"),
	  G_CALLBACK (activate_action_edit_catalog_edit_search) },

	{ "EditCatalog_RedoSearch", NULL,
	  N_("Redo _Search"), NULL,
	  N_("Redo the search"),
	  G_CALLBACK (activate_action_edit_catalog_redo_search) },

	{ "EditCurrentCatalog_New", NULL,
	  N_("_New Catalog..."), NULL,
	  N_("Create a new catalog"),
	  G_CALLBACK (activate_action_edit_current_catalog_new) },

	{ "EditCurrentCatalog_NewLibrary", NULL,
	  N_("New _Library..."), NULL,
	  N_("Create a new catalog library"),
	  G_CALLBACK (activate_action_edit_current_catalog_new_library) },

	{ "EditCurrentCatalog_Rename", NULL,
	  N_("_Rename..."), NULL,
	  N_("Rename current catalog"),
	  G_CALLBACK (activate_action_edit_current_catalog_rename) },

	{ "EditCurrentCatalog_Delete", NULL,
	  N_("Rem_ove"), NULL,
	  N_("Remove current catalog"),
	  G_CALLBACK (activate_action_edit_current_catalog_delete) },

	{ "EditCurrentCatalog_Move", NULL,
	  N_("_Move..."), NULL,
	  N_("Move current catalog to another location"),
	  G_CALLBACK (activate_action_edit_current_catalog_move) },

	{ "EditCurrentCatalog_EditSearch", NULL,
	  N_("_Edit Search"), NULL,
	  N_("Modify search criteria"),
	  G_CALLBACK (activate_action_edit_current_catalog_edit_search) },

	{ "EditCurrentCatalog_RedoSearch", NULL,
	  N_("Redo _Search"), NULL,
	  N_("Redo the search"),
	  G_CALLBACK (activate_action_edit_current_catalog_redo_search) },

	{ "EditDir_View", NULL,
	  N_("Open"), NULL,
	  N_("Open the selected folder"),
	  G_CALLBACK (activate_action_edit_dir_view) },

	{ "EditDir_View_NewWindow", NULL,
	  N_("Open in New Window"), NULL,
	  N_("Open the selected folder in a new window"),
	  G_CALLBACK (activate_action_edit_dir_view_new_window) },

	{ "EditDir_Open", NULL,
	  N_("Open with the _File Manager"), NULL,
	  N_("Open the selected folder with the Nautilus file manager"),
	  G_CALLBACK (activate_action_edit_dir_open) },

	{ "EditDir_Rename", NULL,
	  N_("_Rename..."), NULL,
	  N_("Rename selected folder"),
	  G_CALLBACK (activate_action_edit_dir_rename) },

	{ "EditDir_Delete", GTK_STOCK_DELETE,
	  N_("Move to _Trash"), NULL,
	  N_("Move the selected folder to the Trash"),
	  G_CALLBACK (activate_action_edit_dir_delete) },

	{ "EditDir_Copy", NULL,
	  N_("_Copy..."), NULL,
	  N_("Copy selected folder"),
	  G_CALLBACK (activate_action_edit_dir_copy) },

	{ "EditDir_Move", NULL,
	  N_("_Move..."), NULL,
	  N_("Move selected folder"),
	  G_CALLBACK (activate_action_edit_dir_move) },

	{ "EditDir_Categories", NULL,
	  N_("Ca_tegories"), NULL,
	  N_("Assign categories to the selected folder"),
	  G_CALLBACK (activate_action_edit_dir_categories) },

	{ "EditCurrentDir_Open", NULL,
	  N_("Open with the _File Manager"), NULL,
	  N_("Open current folder with the Nautilus file manager"),
	  G_CALLBACK (activate_action_edit_current_dir_open) },

	{ "EditCurrentDir_Rename", NULL,
	  N_("_Rename..."), NULL,
	  N_("Rename current folder"),
	  G_CALLBACK (activate_action_edit_current_dir_rename) },

	{ "EditCurrentDir_Delete", GTK_STOCK_DELETE,
	  N_("Move to _Trash"), NULL,
	  N_("Move the current folder to the Trash"),
	  G_CALLBACK (activate_action_edit_current_dir_delete) },

	{ "EditCurrentDir_Copy", NULL,
	  N_("_Copy..."), NULL,
	  N_("Copy current folder"),
	  G_CALLBACK (activate_action_edit_current_dir_copy) },

	{ "EditCurrentDir_Move", NULL,
	  N_("_Move..."), NULL,
	  N_("Move current folder"),
	  G_CALLBACK (activate_action_edit_current_dir_move) },

	{ "EditCurrentDir_Categories", NULL,
	  N_("Ca_tegories"), NULL,
	  N_("Assign categories to the current folder"),
	  G_CALLBACK (activate_action_edit_current_dir_categories) },

	{ "EditCurrentDir_New", NULL,
	  N_("_New Folder..."), NULL,
	  N_("Create a new folder"),
	  G_CALLBACK (activate_action_edit_current_dir_new) },

	{ "AlterImage_Rotate90", GTHUMB_STOCK_ROTATE_90,
	  N_("Rotate Ri_ght"), NULL,
	  N_("View the image rotated clockwise"),
	  G_CALLBACK (activate_action_alter_image_rotate90) },

	{ "AlterImage_Rotate90CC", GTHUMB_STOCK_ROTATE_90_CC,
	  N_("Rotate _Left"), NULL,
	  N_("View the image rotated counter-clockwise"),
	  G_CALLBACK (activate_action_alter_image_rotate90cc) },

	{ "AlterImage_Flip", GTHUMB_STOCK_FLIP,
	  N_("_Flip"), NULL,
	  N_("View the image flipped"),
	  G_CALLBACK (activate_action_alter_image_flip) },

	{ "AlterImage_Mirror", GTHUMB_STOCK_MIRROR,
	  N_("_Mirror"), NULL,
	  N_("View the image mirrored"),
	  G_CALLBACK (activate_action_alter_image_mirror) },

	{ "AlterImage_Desaturate", GTHUMB_STOCK_DESATURATE,
	  N_("_Desaturate"), NULL,
	  N_("View the image in black and white"),
	  G_CALLBACK (activate_action_alter_image_desaturate) },

	{ "AlterImage_Invert", GTHUMB_STOCK_INVERT,
	  N_("_Negative"), NULL,
	  N_("View the image with negative colors"),
	  G_CALLBACK (activate_action_alter_image_invert) },

	{ "AlterImage_AdjustLevels", GTHUMB_STOCK_ENHANCE,
	  N_("_Enhance"), "<shift><control>E",
	  N_("Automatically adjust the color levels"),
	  G_CALLBACK (activate_action_alter_image_adjust_levels) },

	{ "AlterImage_Equalize", GTHUMB_STOCK_HISTOGRAM,
	  N_("_Equalize"), NULL,
	  N_("Automatically equalize the image histogram"),
	  G_CALLBACK (activate_action_alter_image_equalize) },

	{ "AlterImage_Normalize", NULL,
	  N_("_Normalize"), NULL,
	  N_("Automatically normalize the contrast"),
	  G_CALLBACK (activate_action_alter_image_normalize) },

	{ "AlterImage_StretchContrast", NULL,
	  N_("_Stretch Contrast"), NULL,
	  N_("Automatically stretch the contrast"),
	  G_CALLBACK (activate_action_alter_image_stretch_contrast) },

	{ "AlterImage_Posterize", GTHUMB_STOCK_POSTERIZE,
	  N_("_Posterize"), NULL,
	  N_("Reduce the number of colors"),
	  G_CALLBACK (activate_action_alter_image_posterize) },

	{ "AlterImage_BrightnessContrast", GTHUMB_STOCK_BRIGHTNESS_CONTRAST,
	  N_("_Brightness-Contrast"), NULL,
	  N_("Adjust brightness and contrast"),
	  G_CALLBACK (activate_action_alter_image_brightness_contrast) },

	{ "AlterImage_HueSaturation", GTHUMB_STOCK_HUE_SATURATION,
	  N_("_Hue-Saturation"), NULL,
	  N_("Adjust hue and saturation"),
	  G_CALLBACK (activate_action_alter_image_hue_saturation) },

	{ "AlterImage_ColorBalance", GTHUMB_STOCK_COLOR_BALANCE,
	  N_("_Color Balance"), NULL,
	  N_("Adjust color balance"),
	  G_CALLBACK (activate_action_alter_image_color_balance) },

	{ "AlterImage_Threshold", GTHUMB_STOCK_THRESHOLD,
	  N_("_Threshold"), NULL,
	  N_("Apply threshold"),
	  G_CALLBACK (activate_action_alter_image_threshold) },

	{ "AlterImage_Resize", GTHUMB_STOCK_RESIZE,
	  N_("_Resize"), NULL,
	  N_("Resize image"),
	  G_CALLBACK (activate_action_alter_image_resize) },

	{ "AlterImage_Rotate", GTHUMB_STOCK_ROTATE,
	  N_("_Rotate"), NULL,
	  N_("Rotate image"),
	  G_CALLBACK (activate_action_alter_image_rotate) },

	{ "AlterImage_Crop", GTHUMB_STOCK_CROP,
	  N_("_Crop"), NULL,
	  N_("Crop image"),
	  G_CALLBACK (activate_action_alter_image_crop) },

	{ "View_NextImage", GTHUMB_STOCK_NEXT_IMAGE,
	  N_("Next"), NULL,
	  N_("View next image"),
	  G_CALLBACK (activate_action_view_next_image) },

	{ "View_PrevImage", GTHUMB_STOCK_PREVIOUS_IMAGE,
	  N_("Previous"), NULL,
	  N_("View previous image"),
	  G_CALLBACK (activate_action_view_prev_image) },

	{ "View_ZoomIn", GTK_STOCK_ZOOM_IN,
	  N_("In"), NULL,
	  N_("Zoom in"),
	  G_CALLBACK (activate_action_view_zoom_in) },

	{ "View_ZoomOut", GTK_STOCK_ZOOM_OUT,
	  N_("Out"), NULL,
	  N_("Zoom out"),
	  G_CALLBACK (activate_action_view_zoom_out) },

	{ "View_Zoom100", GTK_STOCK_ZOOM_100,
	  N_("1:1"), NULL,
	  N_("Actual size"),
	  G_CALLBACK (activate_action_view_zoom_100) },

	{ "View_ZoomFit", GTK_STOCK_ZOOM_FIT,
	  N_("Fit"), NULL,
	  N_("Zoom to fit window"),
	  G_CALLBACK (activate_action_view_zoom_fit) },

	{ "View_StepAnimation", NULL,
	  N_("Step A_nimation"), NULL,
	  N_("View next animation frame"),
	  G_CALLBACK (activate_action_view_step_animation) },

	{ "View_Fullscreen", GTHUMB_STOCK_FULLSCREEN,
	  N_("_Full Screen"), "<control>V",
	  N_("View image in fullscreen mode"),
	  G_CALLBACK (activate_action_view_fullscreen) },

	{ "View_ExitFullscreen", GTHUMB_STOCK_NORMAL_VIEW,
	  N_("Restore Normal View"), NULL,
	  N_("View image in fullscreen mode"),
	  G_CALLBACK (activate_action_view_exit_fullscreen) },

	{ "View_ImageProp", GTHUMB_STOCK_PROPERTIES,
	  N_("Proper_ties"), NULL,
	  N_("View image properties"),
	  G_CALLBACK (activate_action_view_image_prop) },

	{ "Go_Back", GTK_STOCK_GO_BACK,
	  N_("_Back"), "<alt>Left",
	  N_("Go to the previous visited location"),
	  G_CALLBACK (activate_action_go_back) },

	{ "Go_Forward", GTK_STOCK_GO_FORWARD,
	  N_("_Forward"), "<alt>Right",
	  N_("Go to the next visited location"),
	  G_CALLBACK (activate_action_go_forward) },

	{ "Go_Up", GTK_STOCK_GO_UP,
	  N_("_Up"), "<alt>Up",
	  N_("Go up one level"),
	  G_CALLBACK (activate_action_go_up) },

	{ "Go_Refresh", GTK_STOCK_REFRESH,
	  N_("_Reload"), "<control>R",
	  N_("Reload the current location"),
	  G_CALLBACK (activate_action_go_refresh) },

	{ "Go_Stop", GTK_STOCK_STOP,
	  N_("_Stop"), "Escape",
	  N_("Stop loading current location"),
	  G_CALLBACK (activate_action_go_stop) },

	{ "Go_Home", GTK_STOCK_HOME,
	  N_("_Home"), "<alt>Home",
	  N_("Go to the home folder"),
	  G_CALLBACK (activate_action_go_home) },

	{ "Go_ToContainer", GTK_STOCK_JUMP_TO,
	  N_("_Go to the Image Folder"), "<alt>End",
	  N_("Go to the folder that contains the selected image"),
	  G_CALLBACK (activate_action_go_to_container) },

	{ "Go_DeleteHistory", NULL,
	  N_("_Delete History"), NULL,
	  N_("Delete the list of visited locations"),
	  G_CALLBACK (activate_action_go_delete_history) },

	{ "Go_Location", NULL,
	  N_("_Location..."), "<control>L",
	  N_("Specify a location to visit"),
	  G_CALLBACK (activate_action_go_location) },

	{ "Bookmarks_Add", GTK_STOCK_ADD,
	  N_("_Add Bookmark"), "<control>D",
	  N_("Add current location to bookmarks"),
	  G_CALLBACK (activate_action_bookmarks_add) },

	{ "Bookmarks_Edit", NULL,
	  N_("_Edit Bookmarks"), "<control>B",
	  N_("Edit bookmarks"),
	  G_CALLBACK (activate_action_bookmarks_edit) },

	{ "Wallpaper_Centered", NULL,
	  N_("_Centered"), NULL,
	  N_("Set the image as desktop background (centered)"),
	  G_CALLBACK (activate_action_wallpaper_centered) },

	{ "Wallpaper_Tiled", NULL,
	  N_("_Tiled"), NULL,
	  N_("Set the image as desktop background (tiled)"),
	  G_CALLBACK (activate_action_wallpaper_tiled) },

	{ "Wallpaper_Scaled", NULL,
	  N_("_Scaled"), NULL,
	  N_("Set the image as desktop background (scaled keeping aspect ratio)"),
	  G_CALLBACK (activate_action_wallpaper_scaled) },

	{ "Wallpaper_Stretched", NULL,
	  N_("Str_etched"), NULL,
	  N_("Set the image as desktop background (stretched)"),
	  G_CALLBACK (activate_action_wallpaper_stretched) },

	{ "Wallpaper_Restore", NULL,
	  N_("_Restore"), NULL,
	  N_("Restore the original desktop wallpaper"),
	  G_CALLBACK (activate_action_wallpaper_restore) },

	{ "Tools_Slideshow", GTHUMB_STOCK_SLIDESHOW,
	  N_("_Slide Show"), "<control>S",
	  N_("View as a slide show"),
	  G_CALLBACK (activate_action_tools_slideshow) },

	{ "Tools_FindImages", GTK_STOCK_FIND,
	  N_("_Find"), "<control>F",
	  N_("Find images"),
	  G_CALLBACK (activate_action_tools_find_images) },

	{ "Tools_IndexImage", GTHUMB_STOCK_INDEX_IMAGE,
	  N_("Create _Index Image"), NULL,
	  " ",
	  G_CALLBACK (activate_action_tools_index_image) },

	{ "Tools_WebExporter", NULL,
	  N_("Create _Web Album"), NULL,
	  " ",
	  G_CALLBACK (activate_action_tools_web_exporter) },

	{ "Tools_RenameSeries", NULL,
	  N_("_Rename Series"), NULL,
	  " ",
	  G_CALLBACK (activate_action_tools_rename_series) },

	{ "Tools_ConvertFormat", GTK_STOCK_CONVERT,
	  N_("Convert F_ormat"), NULL,
	  "Convert image format",
	  G_CALLBACK (activate_action_tools_convert_format) },

	{ "Tools_FindDuplicates", NULL,
	  N_("Fi_nd Duplicates..."), NULL,
	  " ",
	  G_CALLBACK (activate_action_tools_find_duplicates) },

	{ "Tools_JPEGRotate", GTHUMB_STOCK_TRANSFORM,
	  N_("Ro_tate Images"), NULL,
	  N_("Rotate images without loss of quality"),
	  G_CALLBACK (activate_action_tools_jpeg_rotate) },

	{ "Tools_Preferences", GTK_STOCK_PREFERENCES,
	  N_("_Preferences"), NULL,
	  N_("Edit various preferences"),
	  G_CALLBACK (activate_action_tools_preferences) },

	{ "Tools_ChangeDate", GTHUMB_STOCK_CHANGE_DATE,
	  N_("Change _Date"), NULL,
	  N_("Change images last modified date"),
	  G_CALLBACK (activate_action_tools_change_date) },

	{ "Help_About", GNOME_STOCK_ABOUT,
	  N_("_About"), NULL,
	  N_("Show information about gThumb"),
	  G_CALLBACK (activate_action_help_about) },

	{ "Help_Help", GTK_STOCK_HELP,
	  N_("_Contents"), "F1",
	  " ",
	  G_CALLBACK (activate_action_help_help) },

	{ "Help_Shortcuts", NULL,
	  N_("_Keyboard Shortcuts"), NULL,
	  " ",
	  G_CALLBACK (activate_action_help_shortcuts) }
};
static guint n_action_entries = G_N_ELEMENTS (action_entries);


static GtkToggleActionEntry action_toggle_entries[] = {
	{ "View_Toolbar", NULL,
	  N_("_Toolbar"), NULL,
	  N_("View or hide the toolbar of this window"),
	  G_CALLBACK (activate_action_view_toolbar), 
	  TRUE },
	{ "View_Statusbar", NULL,
	  N_("_Statusbar"), NULL,
	  N_("View or hide the statusbar of this window"),
	  G_CALLBACK (activate_action_view_statusbar), 
	  TRUE },
	{ "View_Thumbnails", NULL,
	  N_("_Thumbnails"), "<control>T",
	  N_("View thumbnails"),
	  G_CALLBACK (activate_action_view_thumbnails), 
	  TRUE },
	{ "View_PlayAnimation", NULL,
	  N_("Play _Animation"), NULL,
	  N_("Start or stop current animation"),
	  G_CALLBACK (activate_action_view_play_animation), 
	  TRUE },
	{ "View_ShowPreview", NULL,
	  N_("_Image Preview"), NULL,
	  N_("View the image"),
	  G_CALLBACK (activate_action_view_show_preview), 
	  TRUE },
	{ "View_ShowInfo", NULL,
	  N_("Image _Comment"), NULL,
	  N_("View image comment"),
	  G_CALLBACK (activate_action_view_show_info), 
	  TRUE },
	{ "SortReversed", NULL,
	  N_("_Reversed Order"), NULL,
	  N_("Reverse images order"),
	  G_CALLBACK (activate_action_sort_reversed), 
	  FALSE },
};
static guint n_action_toggle_entries = G_N_ELEMENTS (action_toggle_entries);


static GtkRadioActionEntry sort_by_entries[] = {
	{ "SortByName", NULL,
	  N_("by _Name"), NULL,
	  N_("Sort images by name"), GTH_SORT_METHOD_BY_NAME },
	{ "SortByPath", NULL,
	  N_("by _Path"), NULL,
	  N_("Sort images by path"), GTH_SORT_METHOD_BY_PATH },
	{ "SortBySize", NULL,
	  N_("by _Size"), NULL,
	  N_("Sort images by file size"), GTH_SORT_METHOD_BY_SIZE },
	{ "SortByTime", NULL,
	  N_("by _Time"), NULL,
	  N_("Sort images by modification time"), GTH_SORT_METHOD_BY_TIME }
};
static guint n_sort_by_entries = G_N_ELEMENTS (sort_by_entries);


static GtkRadioActionEntry zoom_quality_entries[] = {
	{ "View_ZoomQualityHigh", NULL,
	  N_("_High Quality"), NULL,
	  N_("Use high quality zoom"), GTH_ZOOM_QUALITY_HIGH },
	{ "View_ZoomQualityLow", NULL,
	  N_("_Low Quality"), NULL,
	  N_("Use low quality zoom"), GTH_ZOOM_QUALITY_LOW }
};
static guint n_zoom_quality_entries = G_N_ELEMENTS (zoom_quality_entries);


static GtkRadioActionEntry content_entries[] = {
	{ "View_ShowFolders", GTHUMB_STOCK_SHOW_FOLDERS,
	  N_("_Folders"), "<alt>1",
	  N_("View the folders"), GTH_SIDEBAR_DIR_LIST },
	{ "View_ShowCatalogs", GTHUMB_STOCK_SHOW_CATALOGS,
	  N_("_Catalogs"), "<alt>2",
	  N_("View the catalogs"), GTH_SIDEBAR_CATALOG_LIST },
	{ "View_ShowImage", GTHUMB_STOCK_SHOW_IMAGE,
	  N_("_Image"), "<alt>3",
	  N_("View the image"), GTH_SIDEBAR_NO_LIST },
};
static guint n_content_entries = G_N_ELEMENTS (content_entries);


static const gchar *main_ui_info = 
"<ui>"
"  <menubar name='MenuBar'>"
"    <menu name='File' action='FileMenu'>"
"      <menuitem action='File_NewWindow'/>"
"      <separator name='sep01'/>"
"      <menuitem action='File_OpenWith'/>"
"      <menuitem action='File_Save'/>"
"      <menuitem action='File_Revert'/>"
"      <menuitem action='File_Print'/>"
"      <menuitem action='View_ImageProp'/>"
"      <separator name='sep02'/>"
"      <placeholder name='ContentPlaceholder'/>"
"      <separator name='sep03'/>"
"      <menuitem action='File_CameraImport'/>"
"      <menuitem action='File_WriteToCD'/>"
"      <separator name='sep04'/>"
"      <menuitem action='File_CloseWindow'/>"
"    </menu>"
"    <menu name='Edit' action='EditMenu'>"
"      <menuitem action='Edit_RenameFile'/>"
"      <menuitem action='Edit_CopyFiles'/>"
"      <menuitem action='Edit_MoveFiles'/>"
"      <menuitem action='Edit_DuplicateFile'/>"
"      <menuitem action='Edit_DeleteFiles'/>"
"      <separator/>"
"      <menuitem action='Edit_SelectAll'/>"
"      <separator/>"
"      <menuitem action='Tools_FindImages'/>"
"      <menuitem action='Tools_FindDuplicates'/>"
"      <separator/>"
"      <menuitem action='Edit_EditComment'/>"
"      <menuitem action='Edit_EditCategories'/>"
"      <menuitem action='Edit_DeleteComment'/>"
"      <separator/>"
"      <menuitem action='Edit_AddToCatalog'/>"
"      <menuitem action='Edit_RemoveFromCatalog'/>"
"      <separator/>"
"      <menuitem action='Tools_Preferences'/>"
"    </menu>"
"    <menu name='View' action='ViewMenu'>"
"      <menu name='ShowHide' action='ViewShowHideMenu'>"
"        <menuitem action='View_ShowPreview'/>"
"        <menuitem action='View_ShowInfo'/>"
"        <separator/>"
"        <menuitem action='View_Toolbar'/>"
"        <menuitem action='View_Statusbar'/>"
"      </menu>"
"      <separator/>"
"      <menuitem action='View_Fullscreen'/>"
"      <menuitem action='Tools_Slideshow'/>"
"      <separator/>"
"      <menuitem action='View_ShowFolders'/>"
"      <menuitem action='View_ShowCatalogs'/>"
"      <menuitem action='View_ShowImage'/>"
"      <separator/>"
"      <menu name='ViewSort' action='ViewSortMenu'>"
"        <menuitem action='SortByName'/>"
"        <menuitem action='SortByPath'/>"
"        <menuitem action='SortBySize'/>"
"        <menuitem action='SortByTime'/>"
"        <separator/>"
"        <menuitem action='SortReversed'/>"
"      </menu>"
"      <separator/>"
"      <menu name='ZoomType' action='ViewZoomMenu'>"
"        <menuitem action='View_ZoomIn'/>"
"        <menuitem action='View_ZoomOut'/>"
"        <menuitem action='View_Zoom100'/>"
"        <menuitem action='View_ZoomFit'/>"
"        <separator/>"
"        <menuitem action='View_ZoomQualityHigh'/>"
"        <menuitem action='View_ZoomQualityLow'/>"
"      </menu>"
"      <menuitem action='View_PlayAnimation'/>"
"      <menuitem action='View_StepAnimation'/>"
"      <separator/>"
"      <menu name='Go' action='GoMenu'>"
"        <menuitem action='Go_Back'/>"
"        <menuitem action='Go_Forward'/>"
"        <menuitem action='Go_Up'/>"
"        <separator/>"
"        <menuitem action='Go_Home'/>"
"        <menuitem action='Go_ToContainer'/>"
"        <menuitem action='Go_Location'/>"
"        <separator/>"
"        <menuitem action='Go_DeleteHistory'/>"
"        <separator/>"
"        <placeholder name='HistoryList'/>"
"      </menu>"
"      <menuitem action='Go_Stop'/>"
"      <menuitem action='Go_Refresh'/>"
"    </menu>"
"    <menu name='Image' action='ImageMenu'>"
"      <menuitem action='AlterImage_AdjustLevels'/>"
"      <menuitem action='AlterImage_Resize'/>"
"      <menuitem action='AlterImage_Crop'/>"
"      <menu name='Transform' action='ImageTransformMenu'>"
"        <menuitem action='AlterImage_Rotate90'/>"
"        <menuitem action='AlterImage_Rotate90CC'/>"
"        <menuitem action='AlterImage_Flip'/>"
"        <menuitem action='AlterImage_Mirror'/>"
"      </menu>"
"      <separator/>"
"      <menuitem action='AlterImage_Desaturate'/>"
"      <menuitem action='AlterImage_Invert'/>"
"      <menuitem action='AlterImage_ColorBalance'/>"
"      <menuitem action='AlterImage_HueSaturation'/>"
"      <menuitem action='AlterImage_BrightnessContrast'/>"
"      <menuitem action='AlterImage_Posterize'/>"
"      <menu name='Auto' action='ImageAutoMenu'>"
"        <menuitem action='AlterImage_Equalize'/>"
"        <menuitem action='AlterImage_Normalize'/>"
"        <menuitem action='AlterImage_StretchContrast'/>"
"      </menu>"
"    </menu>"
"    <menu name='Bookmarks' action='BookmarksMenu'>"
"      <menuitem action='Bookmarks_Add'/>"
"      <menuitem action='Bookmarks_Edit'/>"
"      <separator/>"
"      <placeholder name='BookmarkList'/>"
"    </menu>"
"    <menu name='Tools' action='ToolsMenu'>"
"      <menu name='Wallpaper' action='ToolsWallpaperMenu'>"
"        <menuitem action='Wallpaper_Centered'/>"
"        <menuitem action='Wallpaper_Tiled'/>"
"        <menuitem action='Wallpaper_Scaled'/>"
"        <menuitem action='Wallpaper_Stretched'/>"
"        <separator/>"
"        <menuitem action='Wallpaper_Restore'/>"
"      </menu>"
"      <separator/>"
"      <menuitem action='Tools_JPEGRotate'/>"
"      <menuitem action='Tools_ConvertFormat'/>"
"      <menuitem action='Tools_RenameSeries'/>"
"      <menuitem action='Tools_ChangeDate'/>"
"      <separator/>"
"      <menuitem action='Tools_IndexImage'/>"
"      <menuitem action='Tools_WebExporter'/>"
"    </menu>"
"    <menu name='Help' action='HelpMenu'>"
"      <menuitem action='Help_Help'/>"
"      <menuitem action='Help_Shortcuts'/>"
"      <separator/>"
"      <menuitem action='Help_About'/>"
"    </menu>"
"  </menubar>"
"  <toolbar name='ToolBar'>"
"    <toolitem action='View_ShowFolders'/>"
"    <toolitem action='View_ShowCatalogs'/>"
"    <toolitem action='View_ShowImage'/>"
"    <separator/>"
"    <placeholder name='ContentPlaceholder1'/>"
"    <placeholder name='ContentPlaceholder2'/>"
"  </toolbar>"
"  <popup name='FilePopup'>"
"    <menuitem action='File_OpenWith'/>"
"    <separator/>"
"    <menu action='ToolsWallpaperMenu'>"
"      <menuitem action='Wallpaper_Centered'/>"
"      <menuitem action='Wallpaper_Tiled'/>"
"      <menuitem action='Wallpaper_Scaled'/>"
"      <menuitem action='Wallpaper_Stretched'/>"
"      <separator/>"
"      <menuitem action='Wallpaper_Restore'/>"
"    </menu>"
"    <menuitem action='Go_ToContainer'/>"
"    <separator/>"
"    <menuitem action='Edit_RenameFile'/>"
"    <menuitem action='Edit_CopyFiles'/>"
"    <menuitem action='Edit_MoveFiles'/>"
"    <menuitem action='Edit_DuplicateFile'/>"
"    <menuitem action='Edit_DeleteFiles'/>"
"    <separator/>"
"    <menuitem action='Edit_EditComment'/>"
"    <menuitem action='Edit_EditCategories'/>"
"    <menuitem action='Edit_DeleteComment'/>"
"    <separator/>"
"    <menuitem action='Edit_AddToCatalog'/>"
"    <menuitem action='Edit_RemoveFromCatalog'/>"
"  </popup>"
"  <popup name='ImagePopup'>"
"    <menuitem action='View_Fullscreen'/>"
"    <separator/>"
"    <menuitem action='Image_OpenWith'/>"
"    <menuitem action='File_Save'/>"
"    <menuitem action='File_Print'/>"
"    <separator/>"
"    <menu action='ToolsWallpaperMenu'>"
"      <menuitem action='Wallpaper_Centered'/>"
"      <menuitem action='Wallpaper_Tiled'/>"
"      <menuitem action='Wallpaper_Scaled'/>"
"      <menuitem action='Wallpaper_Stretched'/>"
"      <separator/>"
"      <menuitem action='Wallpaper_Restore'/>"
"    </menu>"
"    <menuitem action='Go_ToContainer'/>"
"    <separator/>"
"    <menuitem action='Image_Rename'/>"
"    <menuitem action='Image_Copy'/>"
"    <menuitem action='Image_Move'/>"
"    <menuitem action='Image_Duplicate'/>"
"    <menuitem action='Image_Delete'/>"
"    <separator/>"
"    <menuitem action='View_ImageProp'/>"
"  </popup>"
"  <popup name='FullscreenImagePopup'>"
"    <menuitem action='View_ExitFullscreen'/>"
"    <separator/>"
"    <menuitem action='Edit_EditComment'/>"
"    <menuitem action='Edit_EditCategories'/>"
"    <menuitem action='Edit_DeleteComment'/>"
"  </popup>"
"  <popup name='CatalogPopup'>"
"    <menuitem action='EditCatalog_RedoSearch'/>"
"    <menuitem action='EditCatalog_EditSearch'/>"
"    <separator/>"
"    <menuitem action='EditCatalog_Rename'/>"
"    <menuitem action='EditCatalog_Delete'/>"
"    <menuitem action='EditCatalog_Move'/>"
"  </popup>"
"  <popup name='LibraryPopup'>"
"    <menuitem action='EditCatalog_Rename'/>"
"    <menuitem action='EditCatalog_Delete'/>"
"    <menuitem action='EditCatalog_Move'/>"
"  </popup>"
"  <popup name='DirPopup'>"
"    <menuitem action='EditDir_View'/>"
"    <menuitem action='EditDir_View_NewWindow'/>"
"    <menuitem action='EditDir_Open'/>"
"    <separator/>"
"    <menuitem action='EditDir_Rename'/>"
"    <menuitem action='EditDir_Delete'/>"
"    <menuitem action='EditDir_Copy'/>"
"    <menuitem action='EditDir_Move'/>"
"    <separator/>"
"    <menuitem action='EditDir_Categories'/>"
"  </popup>"
"  <popup name='DirListPopup'>"
"    <menuitem action='EditCurrentDir_New'/>"
"  </popup>"
"  <popup name='CatalogListPopup'>"
"    <menuitem action='EditCurrentCatalog_New'/>"
"  </popup>"
"  <popup name='HistoryListPopup'>"
"  </popup>"
"</ui>";


static const gchar *folder_ui_info = 
"<ui>"
"  <menubar name='MenuBar'>"
"    <menu name='File' action='FileMenu'>"
"      <placeholder name='ContentPlaceholder'>"
"        <menuitem action='EditCurrentDir_New'/>"
"        <menu name='Folder' action='FileFolderMenu'>"
"          <menuitem action='EditCurrentDir_Open'/>"
"          <separator name='sep01'/>"
"          <menuitem action='EditCurrentDir_Rename'/>"
"          <menuitem action='EditCurrentDir_Delete'/>"
"          <menuitem action='EditCurrentDir_Copy'/>"
"          <menuitem action='EditCurrentDir_Move'/>"
"          <separator name='sep02'/>"
"          <menuitem action='EditCurrentDir_Categories'/>"
"        </menu>"
"      </placeholder>"
"    </menu>"
"  </menubar>"
"</ui>";


static const gchar *catalog_ui_info = 
"<ui>"
"  <menubar name='MenuBar'>"
"    <menu name='File' action='FileMenu'>"
"      <placeholder name='ContentPlaceholder'>"
"        <menuitem action='EditCurrentCatalog_New'/>"
"        <menuitem action='EditCurrentCatalog_NewLibrary'/>"
"        <menu name='Catalog' action='FileCatalogMenu'>"
"          <menuitem action='EditCurrentCatalog_Rename'/>"
"          <menuitem action='EditCurrentCatalog_Delete'/>"
"          <menuitem action='EditCurrentCatalog_Move'/>"
"        </menu>"
"      </placeholder>"
"    </menu>"
"  </menubar>"
"</ui>";


static const gchar *search_ui_info =
"<ui>"
"  <menubar name='MenuBar'>"
"    <menu name='File' action='FileMenu'>"
"      <placeholder name='ContentPlaceholder'>"
"        <menuitem action='EditCurrentCatalog_New'/>"
"        <menuitem action='EditCurrentCatalog_NewLibrary'/>"
"        <menu name='Search' action='FileSearchMenu'>"
"          <menuitem action='EditCurrentCatalog_RedoSearch'/>"
"          <menuitem action='EditCurrentCatalog_EditSearch'/>"
"          <separator name='EditCurrentCatalog_Sep'/>"
"          <menuitem action='EditCurrentCatalog_Rename'/>"
"          <menuitem action='EditCurrentCatalog_Delete'/>"
"          <menuitem action='EditCurrentCatalog_Move'/>"
"        </menu>"
"      </placeholder>"
"    </menu>"
"  </menubar>"
"</ui>";


static const gchar *browser_ui_info =
"<ui>"
"  <toolbar name='ToolBar'>"
"    <placeholder name='ContentPlaceholder1'>"
"      <toolitem action='Go_Back'/>"
"      <toolitem action='Go_Forward'/>"
"      <toolitem action='Go_Home'/>"
"      <separator name='sep01'/>"
"      <toolitem action='Tools_Slideshow'/>"
"      <separator name='sep02'/>"
"      <toolitem action='Tools_FindImages'/>"
"      <toolitem action='Edit_EditComment'/>"
"      <toolitem action='Edit_EditCategories'/>"
"      <separator name='sep03'/>"
"      <toolitem action='Tools_JPEGRotate'/>"
"    </placeholder>"
"  </toolbar>"
"</ui>";


static const gchar *viewer_ui_info =
"<ui>"
"  <toolbar name='ToolBar'>"
"    <placeholder name='ContentPlaceholder2'>"
"      <toolitem action='View_PrevImage'/>"
"      <toolitem action='View_NextImage'/>"
"      <separator name='sep01'/>"
"      <toolitem action='View_Fullscreen'/>"
"      <separator name='sep02'/>"
"      <toolitem action='Edit_EditComment'/>"
"      <toolitem action='Edit_EditCategories'/>"
"      <separator name='sep03'/>"
"      <toolitem action='View_ZoomIn'/>"
"      <toolitem action='View_ZoomOut'/>"
"      <toolitem action='View_Zoom100'/>"
"      <toolitem action='View_ZoomFit'/>"
"    </placeholder>"
"  </toolbar>"
"</ui>";


#endif /* UI_H */
