/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2019 Free Software Foundation, Inc.
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
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */


#include <config.h>
#include <glib/gi18n.h>
#include <glib-object.h>
#include <gthumb.h>
#include "callbacks.h"
#include "shortcuts.h"


static const GthShortcut shortcuts[] = {
	{ "video-screenshot", N_("Screenshot"), GTH_SHORTCUT_CONTEXT_VIEWER, GTH_SHORTCUT_CATEGORY_MEDIA_VIEWER, "<Alt>s" },
	{ "toggle-play", N_("Play/Pause"), GTH_SHORTCUT_CONTEXT_VIEWER, GTH_SHORTCUT_CATEGORY_MEDIA_VIEWER, "space" },
};


void
media_viewer__gth_browser_construct_cb (GthBrowser *browser)
{
	g_return_if_fail (GTH_IS_BROWSER (browser));

	gth_window_add_viewer_shortcuts (GTH_WINDOW (browser),
					 GTH_SHORTCUT_VIEWER_CONTEXT_MEDIA,
					 shortcuts,
					 G_N_ELEMENTS (shortcuts));
}
