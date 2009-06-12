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

#ifndef GTH_FULLSCREEN_ACTION_ENTRIES_H
#define GTH_FULLSCREEN_ACTION_ENTRIES_H


#include <config.h>
#include <glib/gi18n.h>
#include "gth-window-actions-callbacks.h"
#include "gth-fullscreen-actions-callbacks.h"
#include "gthumb-stock.h"
#include "typedefs.h"


static GtkActionEntry gth_fullscreen_action_entries[] = {
	{ "ExitFullscreen", GTK_STOCK_LEAVE_FULLSCREEN,
	  NULL, NULL,
	  NULL,
	  G_CALLBACK (gth_fullscreen_activate_action_close) },

	{ "StopSlideShow", GTK_STOCK_MEDIA_STOP,
	  N_("Stop"), NULL,
	  N_("Stop slideshow"),
	  G_CALLBACK (gth_fullscreen_activate_action_close) },

	{ "View_NextImage", GTK_STOCK_MEDIA_NEXT,
	  N_("Next"), NULL,
	  N_("View next image"),
	  G_CALLBACK (gth_fullscreen_activate_action_view_next_image) },

	{ "View_PrevImage", GTK_STOCK_MEDIA_PREVIOUS,
	  N_("Previous"), NULL,
	  N_("View previous image"),
	  G_CALLBACK (gth_fullscreen_activate_action_view_prev_image) },

};
static guint gth_fullscreen_action_entries_size = G_N_ELEMENTS (gth_fullscreen_action_entries);


static GtkToggleActionEntry gth_fullscreen_action_toggle_entries[] = {
	{ "View_Comment", GTK_STOCK_PROPERTIES,
	  N_("Proper_ties"), NULL,
	  NULL,
	  G_CALLBACK (gth_fullscreen_activate_action_toggle_comment), 
	  FALSE },
	{ "View_PauseSlideshow", GTK_STOCK_MEDIA_PAUSE,
	  NULL, NULL,
	  N_("Pause slideshow"),
	  G_CALLBACK (gth_fullscreen_activate_action_toggle_slideshow), 
	  FALSE }
};
static guint gth_fullscreen_action_toggle_entries_size = G_N_ELEMENTS (gth_fullscreen_action_toggle_entries);

#endif /* GTH_FULLSCREEN_ACTION_ENTRIES_H */
