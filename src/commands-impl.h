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

#ifndef COMMANDS_IMPL_H
#define COMMANDS_IMPL_H

#include <libbonoboui.h>

#define DEFINE_VERB(x) \
void (x) (BonoboUIComponent *uic, gpointer user_data, const gchar *verbname)

DEFINE_VERB(file_new_window_command_impl);
DEFINE_VERB(file_close_window_command_impl);
DEFINE_VERB(file_open_with_command_impl);
DEFINE_VERB(file_print_command_impl);
DEFINE_VERB(file_save_command_impl);
DEFINE_VERB(file_revert_command_impl);
DEFINE_VERB(file_exit_command_impl);
DEFINE_VERB(edit_rename_file_command_impl);
DEFINE_VERB(edit_duplicate_file_command_impl);
DEFINE_VERB(edit_delete_files_command_impl);
DEFINE_VERB(edit_copy_files_command_impl);
DEFINE_VERB(edit_move_files_command_impl);
DEFINE_VERB(image_open_with_command_impl);
DEFINE_VERB(image_rename_command_impl);
DEFINE_VERB(image_duplicate_command_impl);
DEFINE_VERB(image_delete_command_impl);
DEFINE_VERB(image_delete_from_catalog_command_impl);
DEFINE_VERB(image_edit_comment_command_impl);
DEFINE_VERB(image_edit_categories_command_impl);
DEFINE_VERB(image_copy_command_impl);
DEFINE_VERB(image_move_command_impl);

DEFINE_VERB(edit_select_all_command_impl);
DEFINE_VERB(edit_edit_comment_command_impl);
DEFINE_VERB(edit_delete_comment_command_impl);
DEFINE_VERB(edit_edit_categories_command_impl);
DEFINE_VERB(edit_add_to_catalog_command_impl);
DEFINE_VERB(edit_remove_from_catalog_command_impl);
DEFINE_VERB(edit_folder_open_nautilus_command_impl);
DEFINE_VERB(edit_folder_rename_command_impl);
DEFINE_VERB(edit_folder_delete_command_impl);
DEFINE_VERB(edit_folder_move_command_impl);
DEFINE_VERB(edit_current_folder_new_command_impl);
DEFINE_VERB(edit_current_folder_open_nautilus_command_impl);
DEFINE_VERB(edit_current_folder_rename_command_impl);
DEFINE_VERB(edit_current_folder_delete_command_impl);
DEFINE_VERB(edit_current_folder_move_command_impl);
DEFINE_VERB(edit_catalog_rename_command_impl);
DEFINE_VERB(edit_catalog_delete_command_impl);
DEFINE_VERB(edit_catalog_move_command_impl);
DEFINE_VERB(edit_catalog_edit_search_command_impl);
DEFINE_VERB(edit_catalog_redo_search_command_impl);
DEFINE_VERB(edit_current_catalog_new_command_impl);
DEFINE_VERB(edit_current_catalog_new_library_command_impl);
DEFINE_VERB(edit_current_catalog_rename_command_impl);
DEFINE_VERB(edit_current_catalog_delete_command_impl);
DEFINE_VERB(edit_current_catalog_move_command_impl);
DEFINE_VERB(edit_current_catalog_edit_search_command_impl);
DEFINE_VERB(edit_current_catalog_redo_search_command_impl);
DEFINE_VERB(alter_image_rotate_command_impl);
DEFINE_VERB(alter_image_rotate_cc_command_impl);
DEFINE_VERB(alter_image_flip_command_impl);
DEFINE_VERB(alter_image_mirror_command_impl);
DEFINE_VERB(alter_image_desaturate_command_impl);
DEFINE_VERB(alter_image_invert_command_impl);
DEFINE_VERB(alter_image_posterize_command_impl);
DEFINE_VERB(alter_image_brightness_contrast_command_impl);
DEFINE_VERB(alter_image_hue_saturation_command_impl);
DEFINE_VERB(alter_image_color_balance_command_impl);
DEFINE_VERB(alter_image_scale_command_impl);
DEFINE_VERB(view_zoom_in_command_impl);
DEFINE_VERB(view_zoom_out_command_impl);
DEFINE_VERB(view_zoom_100_command_impl);
DEFINE_VERB(view_zoom_fit_command_impl);
DEFINE_VERB(view_step_ani_command_impl);
DEFINE_VERB(view_show_folders_command_impl);
DEFINE_VERB(view_show_catalogs_command_impl);
DEFINE_VERB(view_show_image_command_impl);
DEFINE_VERB(view_fullscreen_command_impl);
DEFINE_VERB(view_prev_image_command_impl);
DEFINE_VERB(view_next_image_command_impl);
DEFINE_VERB(view_image_prop_command_impl);
DEFINE_VERB(view_sidebar_command_impl);
DEFINE_VERB(go_back_command_impl);
DEFINE_VERB(go_forward_command_impl);
DEFINE_VERB(go_up_command_impl);
DEFINE_VERB(go_stop_command_impl);
DEFINE_VERB(go_refresh_command_impl);
DEFINE_VERB(go_home_command_impl);
DEFINE_VERB(go_to_container_command_impl);
DEFINE_VERB(go_delete_history_command_impl);
DEFINE_VERB(go_location_command_impl);
DEFINE_VERB(bookmarks_add_command_impl);
DEFINE_VERB(bookmarks_edit_command_impl);
DEFINE_VERB(wallpaper_centered_command_impl);
DEFINE_VERB(wallpaper_tiled_command_impl);
DEFINE_VERB(wallpaper_scaled_command_impl);
DEFINE_VERB(wallpaper_stretched_command_impl);
DEFINE_VERB(wallpaper_restore_command_impl);
DEFINE_VERB(tools_slideshow_command_impl);
DEFINE_VERB(tools_find_images_command_impl);
DEFINE_VERB(tools_index_image_command_impl);
DEFINE_VERB(tools_maintenance_command_impl);
DEFINE_VERB(tools_rename_series_command_impl);
DEFINE_VERB(tools_preferences_command_impl);
DEFINE_VERB(tools_jpeg_rotate_command_impl);
DEFINE_VERB(tools_duplicates_command_impl);
DEFINE_VERB(tools_convert_format_command_impl);
DEFINE_VERB(tools_change_date_command_impl);
DEFINE_VERB(help_help_command_impl);
DEFINE_VERB(help_shortcuts_command_impl);
DEFINE_VERB(help_about_command_impl);

#endif /* COMMANDS_IMPL_H */
