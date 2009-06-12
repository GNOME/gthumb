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

#ifndef GTH_FULLSCREEN_UI_H
#define GTH_FULLSCREEN_UI_H


#include <config.h>
#include "gth-window-actions-callbacks.h"
#define NO_ZOOM_QUALITY
#include "gth-window-actions-entries.h"
#include "gth-fullscreen-actions-callbacks.h"
#include "gth-fullscreen-actions-entries.h"


static const gchar *fullscreen_ui_info =
"<ui>"
"  <toolbar name='ToolBar'>"
"    <toolitem action='ExitFullscreen'/>"
"    <separator/>"
"    <toolitem action='View_PrevImage'/>"
"    <toolitem action='View_NextImage'/>"
"    <separator/>"
"    <toolitem action='View_ZoomIn'/>"
"    <toolitem action='View_ZoomOut'/>"
"    <toolitem action='View_Zoom100'/>"
"    <toolitem action='View_ZoomFit'/>"
"    <toolitem action='View_ZoomWidth'/>"
"    <separator/>"
"    <toolitem action='View_Comment'/>"
"  </toolbar>"
"</ui>";


static const gchar *slideshow_ui_info =
"<ui>"
"  <toolbar name='ToolBar'>"
"    <toolitem action='StopSlideShow'/>"
"    <toolitem action='View_PauseSlideshow'/>"
"    <separator/>"
"    <toolitem action='View_PrevImage'/>"
"    <toolitem action='View_NextImage'/>"
"    <separator/>"
"    <toolitem action='View_ZoomIn'/>"
"    <toolitem action='View_ZoomOut'/>"
"    <toolitem action='View_Zoom100'/>"
"    <toolitem action='View_ZoomFit'/>"
"    <toolitem action='View_ZoomWidth'/>"
"    <separator/>"
"    <toolitem action='View_Comment'/>"
"  </toolbar>"
"</ui>";


#endif /* GTH_FULLSCREEN_UI_H */
