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

#ifndef GTH_WINDOW_ACTIONS_CALLBACKS_H
#define GTH_WINDOW_ACTIONS_CALLBACKS_H

#include <gtk/gtk.h>

#define DEFINE_ACTION(x) void x (GtkAction *action, gpointer data);


DEFINE_ACTION(gth_window_activate_action_file_close_window)
DEFINE_ACTION(gth_window_activate_action_file_open_with)
DEFINE_ACTION(gth_window_activate_action_image_open_with)
DEFINE_ACTION(gth_window_activate_action_file_save)
DEFINE_ACTION(gth_window_activate_action_file_save_as)
DEFINE_ACTION(gth_window_activate_action_file_revert)
DEFINE_ACTION(gth_window_activate_action_file_print)
DEFINE_ACTION(gth_window_activate_action_edit_edit_comment)
DEFINE_ACTION(gth_window_activate_action_edit_edit_categories)
DEFINE_ACTION(gth_window_activate_action_edit_undo)
DEFINE_ACTION(gth_window_activate_action_edit_redo)
DEFINE_ACTION(gth_window_activate_action_alter_image_rotate90)
DEFINE_ACTION(gth_window_activate_action_alter_image_rotate90cc)
DEFINE_ACTION(gth_window_activate_action_alter_image_flip)
DEFINE_ACTION(gth_window_activate_action_alter_image_mirror)
DEFINE_ACTION(gth_window_activate_action_alter_image_desaturate)
DEFINE_ACTION(gth_window_activate_action_alter_image_invert)
DEFINE_ACTION(gth_window_activate_action_alter_image_adjust_levels)
DEFINE_ACTION(gth_window_activate_action_alter_image_equalize)
DEFINE_ACTION(gth_window_activate_action_alter_image_posterize)
DEFINE_ACTION(gth_window_activate_action_alter_image_brightness_contrast)
DEFINE_ACTION(gth_window_activate_action_alter_image_hue_saturation)
DEFINE_ACTION(gth_window_activate_action_alter_image_redeye_removal)
DEFINE_ACTION(gth_window_activate_action_alter_image_color_balance)
DEFINE_ACTION(gth_window_activate_action_alter_image_resize)
DEFINE_ACTION(gth_window_activate_action_alter_image_crop)
DEFINE_ACTION(gth_window_activate_action_alter_image_dither_bw)
DEFINE_ACTION(gth_window_activate_action_alter_image_dither_web)
DEFINE_ACTION(gth_window_activate_action_view_zoom_in)
DEFINE_ACTION(gth_window_activate_action_view_zoom_out)
DEFINE_ACTION(gth_window_activate_action_view_zoom_100)
DEFINE_ACTION(gth_window_activate_action_view_zoom_fit)
DEFINE_ACTION(gth_window_activate_action_view_zoom_fit_if_larger)
DEFINE_ACTION(gth_window_activate_action_view_zoom_to_width)
DEFINE_ACTION(gth_window_activate_action_view_fullscreen)
DEFINE_ACTION(gth_window_activate_action_view_exit_fullscreen)
DEFINE_ACTION(gth_window_activate_action_wallpaper_centered)
DEFINE_ACTION(gth_window_activate_action_wallpaper_tiled)
DEFINE_ACTION(gth_window_activate_action_wallpaper_scaled)
DEFINE_ACTION(gth_window_activate_action_wallpaper_stretched)
DEFINE_ACTION(gth_window_activate_action_wallpaper_restore)
DEFINE_ACTION(gth_window_activate_action_help_about)
DEFINE_ACTION(gth_window_activate_action_help_help)
DEFINE_ACTION(gth_window_activate_action_help_shortcuts)
DEFINE_ACTION(gth_window_activate_action_view_toggle_animation)
DEFINE_ACTION(gth_window_activate_action_view_step_animation)
DEFINE_ACTION(gth_window_activate_action_tools_change_date)
DEFINE_ACTION(gth_window_activate_action_tools_jpeg_rotate)
DEFINE_ACTION(gth_window_activate_action_tools_jpeg_rotate_right)
DEFINE_ACTION(gth_window_activate_action_tools_jpeg_rotate_left)
DEFINE_ACTION(gth_window_activate_action_tools_reset_exif)

#endif /* GTH_WINDOW_ACTIONS_CALLBACK_H */
