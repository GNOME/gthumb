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

#ifndef GTH_FULLSCREEN_ACTIONS_CALLBACKS_H
#define GTH_FULLSCREEN_ACTIONS_CALLBACKS_H

#include <gtk/gtk.h>

#define DEFINE_ACTION(x) void x (GtkAction *action, gpointer data);

DEFINE_ACTION(gth_fullscreen_activate_action_view_next_image)
DEFINE_ACTION(gth_fullscreen_activate_action_view_prev_image)
DEFINE_ACTION(gth_fullscreen_activate_action_toggle_slideshow)
DEFINE_ACTION(gth_fullscreen_activate_action_toggle_comment)
DEFINE_ACTION(gth_fullscreen_activate_action_close)

#endif /* GTH_FULLSCREEN_ACTIONS_CALLBACKS_H */
