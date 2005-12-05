/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2005 Free Software Foundation, Inc.
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

#ifndef GTH_BROWSER_ACTION_ENTRIES_H
#define GTH_BROWSER_ACTION_ENTRIES_H


#include <config.h>
#include <glib/gi18n.h>
#include "gth-browser-actions-callbacks.h"
#include "gthumb-stock.h"
#include "typedefs.h"
#include "image-viewer.h"


static GtkActionEntry gth_browser_action_entries[] = {
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
	{ "BookmarksMenu", NULL, N_("_Bookmarks") },
	{ "ToolsMenu", NULL, N_("_Tools") },
	{ "ToolsWallpaperMenu", NULL, N_("Set Image as _Wallpaper") },
	{ "HelpMenu", NULL, N_("_Help") },

	{ "File_NewWindow", GTK_STOCK_NEW,
	  N_("New _Window"), NULL,
	  N_("Create a new window"),
	  G_CALLBACK (gth_browser_activate_action_file_new_window) },
	
	{ "File_ViewImage", GTK_STOCK_OPEN,
	  NULL, NULL,
	  N_("Open the selected image in a new window"),
	  G_CALLBACK (gth_browser_activate_action_file_view_image) },

	{ "File_ImageProp", GTK_STOCK_PROPERTIES,
	  NULL, NULL,
	  N_("View image properties"),
	  G_CALLBACK (gth_browser_activate_action_file_image_properties) },

	{ "File_CameraImport", GTHUMB_STOCK_CAMERA,
	  N_("_Import Photos..."), NULL,
	  N_("Import photos from a digital camera"),
	  G_CALLBACK (gth_browser_activate_action_file_camera_import) },

	{ "File_WriteToCD", GTK_STOCK_CDROM,
	  N_("_Write To CD"), NULL,
	  N_("Write selection to CD"),
	  G_CALLBACK (gth_browser_activate_action_file_write_to_cd) },

	{ "Image_Rename", NULL,
	  N_("_Rename..."), NULL,
	  N_("Rename this image"),
	  G_CALLBACK (gth_browser_activate_action_image_rename) },

	{ "Image_Delete", GTK_STOCK_DELETE,
	  N_("Move to _Trash"), NULL,
	  N_("Move this image to the Trash"),
	  G_CALLBACK (gth_browser_activate_action_image_delete) },

	{ "Image_Copy", NULL,
	  N_("_Copy..."), NULL,
	  N_("Copy this image to another location"),
	  G_CALLBACK (gth_browser_activate_action_image_copy) },

	{ "Image_Move", NULL,
	  N_("_Move..."), NULL,
	  N_("Move this image to another location"),
	  G_CALLBACK (gth_browser_activate_action_image_move) },

	{ "Edit_RenameFile", NULL,
	  N_("_Rename..."), "F2",
	  N_("Rename selected images"),
	  G_CALLBACK (gth_browser_activate_action_edit_rename_file) },

	{ "Edit_DuplicateFile", NULL,
	  N_("D_uplicate"), NULL,
	  N_("Duplicate selected images"),
	  G_CALLBACK (gth_browser_activate_action_edit_duplicate_file) },

	{ "Edit_DeleteFiles", GTK_STOCK_DELETE,
	  N_("Move to _Trash"), NULL,
	  N_("Move the selected images to the Trash"),
	  G_CALLBACK (gth_browser_activate_action_edit_delete_files) },

	{ "Edit_CopyFiles", NULL,
	  N_("_Copy..."), NULL,
	  N_("Copy selected images to another location"),
	  G_CALLBACK (gth_browser_activate_action_edit_copy_files) },

	{ "Edit_MoveFiles", NULL,
	  N_("_Move..."), NULL,
	  N_("Move selected images to another location"),
	  G_CALLBACK (gth_browser_activate_action_edit_move_files) },

	{ "Edit_SelectAll", NULL,
	  N_("_Select All"), NULL,
	  N_("Select all images"),
	  G_CALLBACK (gth_browser_activate_action_edit_select_all) },

	{ "Edit_AddToCatalog", GTHUMB_STOCK_ADD_TO_CATALOG,
	  N_("_Add to Catalog..."), NULL,
	  N_("Add selected images to a catalog"),
	  G_CALLBACK (gth_browser_activate_action_edit_add_to_catalog) },

	{ "Edit_RemoveFromCatalog", NULL,
	  N_("Remo_ve from Catalog"), NULL,
	  N_("Remove selected images from the catalog"),
	  G_CALLBACK (gth_browser_activate_action_edit_remove_from_catalog) },

	{ "EditCatalog_View", NULL,
	  N_("Open"), NULL,
	  N_("Open the selected catalog"),
	  G_CALLBACK (gth_browser_activate_action_edit_catalog_view) },

	{ "EditCatalog_View_NewWindow", NULL,
	  N_("Open in New Window"), NULL,
	  N_("Open the selected catalog in a new window"),
	  G_CALLBACK (gth_browser_activate_action_edit_catalog_view_new_window) },

	{ "EditCatalog_Rename", NULL,
	  N_("_Rename..."), NULL,
	  N_("Rename selected catalog"),
	  G_CALLBACK (gth_browser_activate_action_edit_catalog_rename) },

	{ "EditCatalog_Delete", NULL,
	  N_("Rem_ove"), NULL,
	  N_("Remove selected catalog"),
	  G_CALLBACK (gth_browser_activate_action_edit_catalog_delete) },

	{ "EditCatalog_Move", NULL,
	  N_("_Move..."), NULL,
	  N_("Move selected catalog to another location"),
	  G_CALLBACK (gth_browser_activate_action_edit_catalog_move) },

	{ "EditCatalog_EditSearch", NULL,
	  N_("_Edit Search"), NULL,
	  N_("Modify search criteria"),
	  G_CALLBACK (gth_browser_activate_action_edit_catalog_edit_search) },

	{ "EditCatalog_RedoSearch", GTK_STOCK_REFRESH,
	  N_("Redo _Search"), NULL,
	  N_("Redo the search"),
	  G_CALLBACK (gth_browser_activate_action_edit_catalog_redo_search) },

	{ "EditCurrentCatalog_New", NULL,
	  N_("_New Catalog..."), NULL,
	  N_("Create a new catalog"),
	  G_CALLBACK (gth_browser_activate_action_edit_current_catalog_new) },

	{ "EditCurrentCatalog_NewLibrary", NULL,
	  N_("New _Library..."), NULL,
	  N_("Create a new catalog library"),
	  G_CALLBACK (gth_browser_activate_action_edit_current_catalog_new_library) },

	{ "EditCurrentCatalog_Rename", NULL,
	  N_("_Rename..."), NULL,
	  N_("Rename current catalog"),
	  G_CALLBACK (gth_browser_activate_action_edit_current_catalog_rename) },

	{ "EditCurrentCatalog_Delete", NULL,
	  N_("Rem_ove"), NULL,
	  N_("Remove current catalog"),
	  G_CALLBACK (gth_browser_activate_action_edit_current_catalog_delete) },

	{ "EditCurrentCatalog_Move", NULL,
	  N_("_Move..."), NULL,
	  N_("Move current catalog to another location"),
	  G_CALLBACK (gth_browser_activate_action_edit_current_catalog_move) },

	{ "EditCurrentCatalog_EditSearch", NULL,
	  N_("_Edit Search"), NULL,
	  N_("Modify search criteria"),
	  G_CALLBACK (gth_browser_activate_action_edit_current_catalog_edit_search) },

	{ "EditCurrentCatalog_RedoSearch", GTK_STOCK_REFRESH,
	  N_("Redo _Search"), NULL,
	  N_("Redo the search"),
	  G_CALLBACK (gth_browser_activate_action_edit_current_catalog_redo_search) },

	{ "EditDir_View", NULL,
	  N_("Open"), NULL,
	  N_("Open the selected folder"),
	  G_CALLBACK (gth_browser_activate_action_edit_dir_view) },

	{ "EditDir_View_NewWindow", NULL,
	  N_("Open in New Window"), NULL,
	  N_("Open the selected folder in a new window"),
	  G_CALLBACK (gth_browser_activate_action_edit_dir_view_new_window) },

	{ "EditDir_Open", NULL,
	  N_("Open with the _File Manager"), NULL,
	  N_("Open the selected folder with the Nautilus file manager"),
	  G_CALLBACK (gth_browser_activate_action_edit_dir_open) },

	{ "EditDir_Rename", NULL,
	  N_("_Rename..."), NULL,
	  N_("Rename selected folder"),
	  G_CALLBACK (gth_browser_activate_action_edit_dir_rename) },

	{ "EditDir_Delete", GTK_STOCK_DELETE,
	  N_("Move to _Trash"), NULL,
	  N_("Move the selected folder to the Trash"),
	  G_CALLBACK (gth_browser_activate_action_edit_dir_delete) },

	{ "EditDir_Copy", NULL,
	  N_("_Copy..."), NULL,
	  N_("Copy selected folder"),
	  G_CALLBACK (gth_browser_activate_action_edit_dir_copy) },

	{ "EditDir_Move", NULL,
	  N_("_Move..."), NULL,
	  N_("Move selected folder"),
	  G_CALLBACK (gth_browser_activate_action_edit_dir_move) },

	{ "EditDir_Categories", NULL,
	  N_("Ca_tegories"), NULL,
	  N_("Assign categories to the selected folder"),
	  G_CALLBACK (gth_browser_activate_action_edit_dir_categories) },

	{ "EditCurrentDir_Open", NULL,
	  N_("Open with the _File Manager"), NULL,
	  N_("Open current folder with the Nautilus file manager"),
	  G_CALLBACK (gth_browser_activate_action_edit_current_dir_open) },

	{ "EditCurrentDir_Rename", NULL,
	  N_("_Rename..."), NULL,
	  N_("Rename current folder"),
	  G_CALLBACK (gth_browser_activate_action_edit_current_dir_rename) },

	{ "EditCurrentDir_Delete", GTK_STOCK_DELETE,
	  N_("Move to _Trash"), NULL,
	  N_("Move the current folder to the Trash"),
	  G_CALLBACK (gth_browser_activate_action_edit_current_dir_delete) },

	{ "EditCurrentDir_Copy", NULL,
	  N_("_Copy..."), NULL,
	  N_("Copy current folder"),
	  G_CALLBACK (gth_browser_activate_action_edit_current_dir_copy) },

	{ "EditCurrentDir_Move", NULL,
	  N_("_Move..."), NULL,
	  N_("Move current folder"),
	  G_CALLBACK (gth_browser_activate_action_edit_current_dir_move) },

	{ "EditCurrentDir_Categories", NULL,
	  N_("Ca_tegories"), NULL,
	  N_("Assign categories to the current folder"),
	  G_CALLBACK (gth_browser_activate_action_edit_current_dir_categories) },

	{ "EditCurrentDir_New", NULL,
	  N_("_New Folder..."), NULL,
	  N_("Create a new folder"),
	  G_CALLBACK (gth_browser_activate_action_edit_current_dir_new) },

	{ "View_NextImage", GTHUMB_STOCK_NEXT_IMAGE,
	  N_("Next"), NULL,
	  N_("View next image"),
	  G_CALLBACK (gth_browser_activate_action_view_next_image) },

	{ "View_PrevImage", GTHUMB_STOCK_PREVIOUS_IMAGE,
	  N_("Previous"), NULL,
	  N_("View previous image"),
	  G_CALLBACK (gth_browser_activate_action_view_prev_image) },

	{ "Go_Back", GTK_STOCK_GO_BACK,
	  NULL, "<alt>Left",
	  N_("Go to the previous visited location"),
	  G_CALLBACK (gth_browser_activate_action_go_back) },

	{ "Go_Forward", GTK_STOCK_GO_FORWARD,
	  NULL, "<alt>Right",
	  N_("Go to the next visited location"),
	  G_CALLBACK (gth_browser_activate_action_go_forward) },

	{ "Go_Up", GTK_STOCK_GO_UP,
	  NULL, "<alt>Up",
	  N_("Go up one level"),
	  G_CALLBACK (gth_browser_activate_action_go_up) },

	{ "Go_Refresh", GTK_STOCK_REFRESH,
	  N_("_Reload"), "<control>R",
	  N_("Reload the current location"),
	  G_CALLBACK (gth_browser_activate_action_go_refresh) },

	{ "Go_Stop", GTK_STOCK_STOP,
	  NULL, "Escape",
	  N_("Stop loading current location"),
	  G_CALLBACK (gth_browser_activate_action_go_stop) },

	{ "Go_Home", GTK_STOCK_HOME,
	  NULL, "<alt>Home",
	  N_("Go to the home folder"),
	  G_CALLBACK (gth_browser_activate_action_go_home) },

	{ "Go_ToContainer", GTK_STOCK_JUMP_TO,
	  N_("_Go to the Image Folder"), "<alt>End",
	  N_("Go to the folder that contains the selected image"),
	  G_CALLBACK (gth_browser_activate_action_go_to_container) },

	{ "Go_DeleteHistory", NULL,
	  N_("_Delete History"), NULL,
	  N_("Delete the list of visited locations"),
	  G_CALLBACK (gth_browser_activate_action_go_delete_history) },

	{ "Go_Location", NULL,
	  N_("_Location..."), "<control>L",
	  N_("Specify a location to visit"),
	  G_CALLBACK (gth_browser_activate_action_go_location) },

	{ "Bookmarks_Add", GTK_STOCK_ADD,
	  N_("_Add Bookmark"), "<control>D",
	  N_("Add current location to bookmarks"),
	  G_CALLBACK (gth_browser_activate_action_bookmarks_add) },

	{ "Bookmarks_Edit", NULL,
	  N_("_Edit Bookmarks"), "<control>B",
	  N_("Edit bookmarks"),
	  G_CALLBACK (gth_browser_activate_action_bookmarks_edit) },

	{ "Tools_Slideshow", GTHUMB_STOCK_SLIDESHOW,
	  N_("_Slide Show"), "F12",
	  N_("View as a slide show"),
	  G_CALLBACK (gth_browser_activate_action_tools_slideshow) },

	{ "Tools_FindImages", GTK_STOCK_FIND,
	  NULL, NULL, 
	  NULL,
	  G_CALLBACK (gth_browser_activate_action_tools_find_images) },

	{ "Tools_IndexImage", GTHUMB_STOCK_INDEX_IMAGE,
	  N_("Create _Index Image"), NULL,
	  " ",
	  G_CALLBACK (gth_browser_activate_action_tools_index_image) },

	{ "Tools_WebExporter", NULL,
	  N_("Create _Web Album"), NULL,
	  " ",
	  G_CALLBACK (gth_browser_activate_action_tools_web_exporter) },

	{ "Tools_ConvertFormat", GTK_STOCK_CONVERT,
	  N_("Convert F_ormat"), NULL,
	  "Convert image format",
	  G_CALLBACK (gth_browser_activate_action_tools_convert_format) },

	{ "Tools_FindDuplicates", NULL,
	  N_("Fi_nd Duplicates..."), NULL,
	  " ",
	  G_CALLBACK (gth_browser_activate_action_tools_find_duplicates) },

	{ "Tools_Preferences", GTK_STOCK_PREFERENCES,
	  NULL, NULL, 
	  N_("Edit various preferences"),
	  G_CALLBACK (gth_browser_activate_action_tools_preferences) },

	{ "Tools_ScaleSeries", GTHUMB_STOCK_RESIZE,
	  N_("Scale Images"), NULL,
	  N_("Scale Images"),
	  G_CALLBACK (gth_browser_activate_action_tools_resize_images) }
};
static guint gth_browser_action_entries_size = G_N_ELEMENTS (gth_browser_action_entries);


static GtkToggleActionEntry gth_browser_action_toggle_entries[] = {
	{ "View_Toolbar", NULL,
	  N_("_Toolbar"), NULL,
	  N_("View or hide the toolbar of this window"),
	  G_CALLBACK (gth_browser_activate_action_view_toolbar), 
	  TRUE },
	{ "View_Statusbar", NULL,
	  N_("_Statusbar"), NULL,
	  N_("View or hide the statusbar of this window"),
	  G_CALLBACK (gth_browser_activate_action_view_statusbar), 
	  TRUE },
	{ "View_Thumbnails", NULL,
	  N_("_Thumbnails"), "<control>T",
	  N_("View thumbnails"),
	  G_CALLBACK (gth_browser_activate_action_view_thumbnails), 
	  TRUE },
	{ "View_ShowPreview", NULL,
	  N_("_Image Preview"), NULL,
	  N_("View the image"),
	  G_CALLBACK (gth_browser_activate_action_view_show_preview), 
	  TRUE },
	{ "View_ShowInfo", NULL,
	  N_("Image _Comment"), NULL,
	  N_("View image comment"),
	  G_CALLBACK (gth_browser_activate_action_view_show_info), 
	  TRUE },
	{ "SortReversed", NULL,
	  N_("_Reversed Order"), NULL,
	  N_("Reverse images order"),
	  G_CALLBACK (gth_browser_activate_action_sort_reversed), 
	  FALSE },
};
static guint gth_browser_action_toggle_entries_size = G_N_ELEMENTS (gth_browser_action_toggle_entries);


static GtkRadioActionEntry gth_browser_sort_by_entries[] = {
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
	  N_("Sort images by modification time"), GTH_SORT_METHOD_BY_TIME },
	{ "SortManual", NULL,
	  N_("_Manual Order"), NULL,
	  N_("Sort images manually"), GTH_SORT_METHOD_MANUAL }
};
static guint gth_browser_sort_by_entries_size = G_N_ELEMENTS (gth_browser_sort_by_entries);


static GtkRadioActionEntry gth_browser_content_entries[] = {
	{ "View_ShowFolders", GTK_STOCK_OPEN,
	  N_("_Folders"), "<alt>1",
	  N_("View the folders"), GTH_SIDEBAR_DIR_LIST },
	{ "View_ShowCatalogs", GTHUMB_STOCK_SHOW_CATALOGS,
	  N_("_Catalogs"), "<alt>2",
	  N_("View the catalogs"), GTH_SIDEBAR_CATALOG_LIST },
	{ "View_ShowImage", GTHUMB_STOCK_SHOW_IMAGE,
	  N_("_Image"), "<alt>3",
	  N_("View the image"), GTH_SIDEBAR_NO_LIST },
};
static guint gth_browser_content_entries_size = G_N_ELEMENTS (gth_browser_content_entries);


#endif /* GTH_BROWSER_ACTION_ENTRIES_H */
