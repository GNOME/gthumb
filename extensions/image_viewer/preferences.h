/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2009 Free Software Foundation, Inc.
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

#ifndef PREFERENCES_H
#define PREFERENCES_H

#include <gthumb.h>

#define  PREF_VIEWER_ZOOM_QUALITY           "/apps/gthumb/viewer/zoom_quality"
#define  PREF_VIEWER_ZOOM_CHANGE            "/apps/gthumb/viewer/zoom_change"
#define  PREF_VIEWER_TRANSP_TYPE            "/apps/gthumb/viewer/transparency_type"
#define  PREF_VIEWER_RESET_SCROLLBARS       "/apps/gthumb/viewer/reset_scrollbars"
#define  PREF_VIEWER_CHECK_TYPE             "/apps/gthumb/viewer/check_type"
#define  PREF_VIEWER_CHECK_SIZE             "/apps/gthumb/viewer/check_size"
#define  PREF_VIEWER_BLACK_BACKGROUND       "/apps/gthumb/viewer/black_background"
#define  PREF_VIEWER_SHRINK_WRAP            "/apps/gthumb/viewer/shrink_wrap"

void image_viewer__dlg_preferences_construct_cb (GtkWidget  *dialog,
						 GthBrowser *browser,
						 GtkBuilder *builder);

#endif /* PREFERENCES_H */
