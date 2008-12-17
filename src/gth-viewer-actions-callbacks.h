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

#ifndef GTH_VIEWER_ACTIONS_CALLBACKS_H
#define GTH_VIEWER_ACTIONS_CALLBACKS_H

#include <gtk/gtk.h>

#define DEFINE_ACTION(x) void x (GtkAction *action, gpointer data);

DEFINE_ACTION(gth_viewer_activate_action_file_new_window)
DEFINE_ACTION(gth_viewer_activate_action_file_open_folder)
DEFINE_ACTION(gth_viewer_activate_action_file_revert)
DEFINE_ACTION(gth_viewer_activate_action_edit_add_to_catalog)
DEFINE_ACTION(gth_viewer_activate_action_view_toolbar)
DEFINE_ACTION(gth_viewer_activate_action_view_statusbar)
DEFINE_ACTION(gth_viewer_activate_action_view_show_info)
DEFINE_ACTION(gth_viewer_activate_action_go_refresh)
DEFINE_ACTION(gth_viewer_activate_action_single_window)
DEFINE_ACTION(gth_viewer_activate_action_scripts)
DEFINE_ACTION(gth_viewer_activate_action_upload_flickr)

#endif /* GTH_VIEWER_ACTIONS_CALLBACK_H */
