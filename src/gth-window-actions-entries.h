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

#ifndef GTH_WINDOW_ACTION_ENTRIES_H
#define GTH_WINDOW_ACTION_ENTRIES_H


#include <config.h>
#include <glib/gi18n.h>
#include "gth-window-actions-callbacks.h"
#include "gthumb-stock.h"
#include "typedefs.h"


static GtkActionEntry gth_window_action_entries[] = {
	{ "ImageTransformMenu", NULL, N_("_Transform") },
	{ "ImageAutoMenu", NULL, N_("Auto") },
	{ "DitherMenu", GTHUMB_STOCK_REDUCE_COLORS, N_("Reduce Colors") },

	{ "File_CloseWindow", GTK_STOCK_CLOSE,
	  NULL, NULL,
	  N_("Close this window"),
	  G_CALLBACK (gth_window_activate_action_file_close_window) },

	{ "File_OpenWith", GTK_STOCK_OPEN,
	  N_("_Open With"), "O",
	  N_("Open selected images with an application"),
	  G_CALLBACK (gth_window_activate_action_file_open_with) },

	{ "File_Save", GTK_STOCK_SAVE,
	  NULL, NULL,
	  N_("Save current image"),
	  G_CALLBACK (gth_window_activate_action_file_save) },

	{ "File_SaveAs", GTK_STOCK_SAVE_AS,
	  NULL, NULL,
	  N_("Save current image"),
	  G_CALLBACK (gth_window_activate_action_file_save_as) },

	{ "File_Revert", GTK_STOCK_REVERT_TO_SAVED,
	  NULL, "F4",
	  N_("Revert to saved image"),
	  G_CALLBACK (gth_window_activate_action_file_revert) },

	{ "File_Print", GTK_STOCK_PRINT,
	  NULL, NULL,
	  N_("Print the current image"),
	  G_CALLBACK (gth_window_activate_action_file_print) },

	{ "Image_OpenWith", GTK_STOCK_OPEN,
	  N_("_Open With"), "O",
	  N_("Open this image with an application"),
	  G_CALLBACK (gth_window_activate_action_image_open_with) },

	{ "Edit_EditComment", GTHUMB_STOCK_ADD_COMMENT,
	  N_("Comm_ent"), "C",
	  N_("Add a comment to selected images"),
	  G_CALLBACK (gth_window_activate_action_edit_edit_comment) },

	{ "Edit_DeleteComment", NULL,
	  N_("Rem_ove Comment"), NULL,
	  N_("Remove comments of selected images"),
	  G_CALLBACK (gth_window_activate_action_edit_delete_comment) },

	{ "Edit_EditCategories", GTK_STOCK_INDEX,
	  N_("Ca_tegories"), "K",
	  N_("Assign categories to selected images"),
	  G_CALLBACK (gth_window_activate_action_edit_edit_categories) },

	{ "Edit_Undo", GTK_STOCK_UNDO,
	  NULL, "<control>Z",
	  NULL,
	  G_CALLBACK (gth_window_activate_action_edit_undo) },

	{ "Edit_Redo", GTK_STOCK_REDO,
	  NULL, "<shift><control>Z",
	  NULL,
	  G_CALLBACK (gth_window_activate_action_edit_redo) },

	{ "AlterImage_Rotate90", GTHUMB_STOCK_ROTATE_90,
	  N_("Rotate Ri_ght"), NULL,
	  N_("View the image rotated clockwise"),
	  G_CALLBACK (gth_window_activate_action_alter_image_rotate90) },

	{ "AlterImage_Rotate90CC", GTHUMB_STOCK_ROTATE_90_CC,
	  N_("Rotate _Left"), NULL,
	  N_("View the image rotated counter-clockwise"),
	  G_CALLBACK (gth_window_activate_action_alter_image_rotate90cc) },

	{ "AlterImage_Flip", GTHUMB_STOCK_FLIP,
	  N_("_Flip"), NULL,
	  N_("View the image flipped"),
	  G_CALLBACK (gth_window_activate_action_alter_image_flip) },

	{ "AlterImage_Mirror", GTHUMB_STOCK_MIRROR,
	  N_("_Mirror"), NULL,
	  N_("View the image mirrored"),
	  G_CALLBACK (gth_window_activate_action_alter_image_mirror) },

	{ "AlterImage_Desaturate", GTHUMB_STOCK_DESATURATE,
	  N_("_Desaturate"), NULL,
	  N_("View the image in black and white"),
	  G_CALLBACK (gth_window_activate_action_alter_image_desaturate) },

	{ "AlterImage_Invert", GTHUMB_STOCK_INVERT,
	  N_("_Negative"), NULL,
	  N_("View the image with negative colors"),
	  G_CALLBACK (gth_window_activate_action_alter_image_invert) },

	{ "AlterImage_AdjustLevels", GTHUMB_STOCK_ENHANCE,
	  N_("_Enhance"), "<shift><control>E",
	  N_("Automatically adjust the color levels"),
	  G_CALLBACK (gth_window_activate_action_alter_image_adjust_levels) },

	{ "AlterImage_Equalize", GTHUMB_STOCK_HISTOGRAM,
	  N_("_Equalize"), NULL,
	  N_("Automatically equalize the image histogram"),
	  G_CALLBACK (gth_window_activate_action_alter_image_equalize) },

	{ "AlterImage_Posterize", GTHUMB_STOCK_POSTERIZE,
	  N_("_Posterize"), NULL,
	  N_("Reduce the number of colors"),
	  G_CALLBACK (gth_window_activate_action_alter_image_posterize) },

	{ "AlterImage_BrightnessContrast", GTHUMB_STOCK_BRIGHTNESS_CONTRAST,
	  N_("_Brightness-Contrast"), NULL,
	  N_("Adjust brightness and contrast"),
	  G_CALLBACK (gth_window_activate_action_alter_image_brightness_contrast) },

	{ "AlterImage_HueSaturation", GTHUMB_STOCK_HUE_SATURATION,
	  N_("_Hue-Saturation"), NULL,
	  N_("Adjust hue and saturation"),
	  G_CALLBACK (gth_window_activate_action_alter_image_hue_saturation) },

	{ "AlterImage_ColorBalance", GTHUMB_STOCK_COLOR_BALANCE,
	  N_("_Color Balance"), NULL,
	  N_("Adjust color balance"),
	  G_CALLBACK (gth_window_activate_action_alter_image_color_balance) },

	{ "AlterImage_Threshold", GTHUMB_STOCK_THRESHOLD,
	  N_("_Threshold"), NULL,
	  N_("Apply threshold"),
	  G_CALLBACK (gth_window_activate_action_alter_image_threshold) },

	{ "AlterImage_Resize", GTHUMB_STOCK_RESIZE,
	  N_("_Resize"), NULL,
	  N_("Resize image"),
	  G_CALLBACK (gth_window_activate_action_alter_image_resize) },

	{ "AlterImage_Rotate", GTHUMB_STOCK_ROTATE,
	  N_("_Rotate"), NULL,
	  N_("Rotate image"),
	  G_CALLBACK (gth_window_activate_action_alter_image_rotate) },

	{ "AlterImage_Crop", GTHUMB_STOCK_CROP,
	  N_("_Crop"), NULL,
	  N_("Crop image"),
	  G_CALLBACK (gth_window_activate_action_alter_image_crop) },

	{ "AlterImage_Dither_BW", NULL,
	  N_("Black and White"), NULL,
	  N_("Reduce the number of colors"),
	  G_CALLBACK (gth_window_activate_action_alter_image_dither_bw) },

	{ "AlterImage_Dither_Web", NULL,
	  N_("Web Palette"), NULL,
	  N_("Reduce the number of colors"),
	  G_CALLBACK (gth_window_activate_action_alter_image_dither_web) },

	{ "View_ZoomIn", GTK_STOCK_ZOOM_IN,
	  N_("In"), "<control>plus",
	  N_("Zoom in"),
	  G_CALLBACK (gth_window_activate_action_view_zoom_in) },

	{ "View_ZoomOut", GTK_STOCK_ZOOM_OUT,
	  N_("Out"), "<control>minus",
	  N_("Zoom out"),
	  G_CALLBACK (gth_window_activate_action_view_zoom_out) },

	{ "View_Zoom100", GTK_STOCK_ZOOM_100,
	  N_("1:1"), "<control>0",
	  N_("Actual size"),
	  G_CALLBACK (gth_window_activate_action_view_zoom_100) },

	{ "View_ZoomFit", GTK_STOCK_ZOOM_FIT,
	  N_("Fit"), NULL,
	  N_("Zoom to fit window"),
	  G_CALLBACK (gth_window_activate_action_view_zoom_fit) },

	{ "View_StepAnimation", NULL,
	  N_("Step A_nimation"), "J",
	  N_("View next animation frame"),
	  G_CALLBACK (gth_window_activate_action_view_step_animation) },

	{ "View_Fullscreen", GTHUMB_STOCK_FULLSCREEN,
	  N_("_Full Screen"), "F11",
	  N_("View image in fullscreen mode"),
	  G_CALLBACK (gth_window_activate_action_view_fullscreen) },

	{ "View_ExitFullscreen", GTHUMB_STOCK_NORMAL_VIEW,
	  N_("Restore Normal View"), NULL,
	  N_("View image in fullscreen mode"),
	  G_CALLBACK (gth_window_activate_action_view_exit_fullscreen) },

	{ "Wallpaper_Centered", NULL,
	  N_("_Centered"), NULL,
	  N_("Set the image as desktop background (centered)"),
	  G_CALLBACK (gth_window_activate_action_wallpaper_centered) },

	{ "Wallpaper_Tiled", NULL,
	  N_("_Tiled"), NULL,
	  N_("Set the image as desktop background (tiled)"),
	  G_CALLBACK (gth_window_activate_action_wallpaper_tiled) },

	{ "Wallpaper_Scaled", NULL,
	  N_("_Scaled"), NULL,
	  N_("Set the image as desktop background (scaled keeping aspect ratio)"),
	  G_CALLBACK (gth_window_activate_action_wallpaper_scaled) },

	{ "Wallpaper_Stretched", NULL,
	  N_("Str_etched"), NULL,
	  N_("Set the image as desktop background (stretched)"),
	  G_CALLBACK (gth_window_activate_action_wallpaper_stretched) },

	{ "Wallpaper_Restore", NULL,
	  N_("_Restore"), NULL,
	  N_("Restore the original desktop wallpaper"),
	  G_CALLBACK (gth_window_activate_action_wallpaper_restore) },

	{ "Tools_ChangeDate", GTHUMB_STOCK_CHANGE_DATE,
	  N_("Change _Date"), NULL,
	  N_("Change images last modified date"),
	  G_CALLBACK (gth_window_activate_action_tools_change_date) },

	{ "Tools_JPEGRotate", GTHUMB_STOCK_TRANSFORM,
	  N_("Ro_tate Images"), NULL,
	  N_("Rotate images without loss of quality"),
	  G_CALLBACK (gth_window_activate_action_tools_jpeg_rotate) },

	{ "Tools_JPEGRotate_Left", GTHUMB_STOCK_ROTATE_90_CC,
	  N_("Rotate _Left"), NULL,
	  N_("Rotate images without loss of quality"),
	  G_CALLBACK (gth_window_activate_action_tools_jpeg_rotate_left) },

	{ "Tools_JPEGRotate_Right", GTHUMB_STOCK_ROTATE_90,
	  N_("Rotate Ri_ght"), NULL,
	  N_("Rotate images without loss of quality"),
	  G_CALLBACK (gth_window_activate_action_tools_jpeg_rotate_right) },

	{ "Tools_JPEGRotate_Auto", NULL,
	  N_("Adjust photo _orientation"), NULL,
	  N_("Rotate images without loss of quality"),
	  G_CALLBACK (gth_window_activate_action_tools_jpeg_rotate_auto) },

	{ "Help_About", GTK_STOCK_ABOUT,
	  NULL, NULL,
	  N_("Show information about gThumb"),
	  G_CALLBACK (gth_window_activate_action_help_about) },

	{ "Help_Help", GTK_STOCK_HELP,
	  NULL, NULL,
	  " ",
	  G_CALLBACK (gth_window_activate_action_help_help) },

	{ "Help_Shortcuts", NULL,
	  N_("_Keyboard Shortcuts"), NULL,
	  " ",
	  G_CALLBACK (gth_window_activate_action_help_shortcuts) },

	/* Accelerators */

	{ "ControlEqual", NULL,
	  N_("In"), "<control>equal",
	  NULL,
	  G_CALLBACK (gth_window_activate_action_view_zoom_in) }
};
static guint gth_window_action_entries_size = G_N_ELEMENTS (gth_window_action_entries);


static GtkToggleActionEntry gth_window_action_toggle_entries[] = {
	{ "View_PlayAnimation", NULL,
	  N_("Play _Animation"), "G",
	  N_("Start or stop current animation"),
	  G_CALLBACK (gth_window_activate_action_view_toggle_animation), 
	  TRUE }
};
static guint gth_window_action_toggle_entries_size = G_N_ELEMENTS (gth_window_action_toggle_entries);


#ifndef NO_ZOOM_QUALITY
static GtkRadioActionEntry gth_window_zoom_quality_entries[] = {
	{ "View_ZoomQualityHigh", NULL,
	  N_("_High Quality"), NULL,
	  N_("Use high quality zoom"), GTH_ZOOM_QUALITY_HIGH },
	{ "View_ZoomQualityLow", NULL,
	  N_("_Low Quality"), NULL,
	  N_("Use low quality zoom"), GTH_ZOOM_QUALITY_LOW }
};
static guint gth_window_zoom_quality_entries_size = G_N_ELEMENTS (gth_window_zoom_quality_entries);
#endif

#endif /* GTH_WINDOW_ACTION_ENTRIES_H */
