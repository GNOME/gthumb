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

#include <config.h>

#include "gth-fullscreen.h"


void
gth_fullscreen_activate_action_view_next_image (GtkAction     *action,
						GthFullscreen *fullscreen)
{
	gth_fullscreen_show_next_image (fullscreen);	
}


void
gth_fullscreen_activate_action_view_prev_image (GtkAction     *action,
						GthFullscreen *fullscreen)
{
	gth_fullscreen_show_prev_image (fullscreen);	
}


void
gth_fullscreen_activate_action_toggle_slideshow (GtkAction     *action,
						 GthFullscreen *fullscreen)
{
	gth_fullscreen_pause_slideshow (fullscreen, gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action)));
}


void
gth_fullscreen_activate_action_toggle_comment (GtkAction     *action,
					       GthFullscreen *fullscreen)
{
	gth_fullscreen_show_comment (fullscreen, gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action)));
}


void
gth_fullscreen_activate_action_close (GtkAction     *action,
				      GthFullscreen *fullscreen)
{
	gtk_widget_destroy ((GtkWidget*) fullscreen);
}
