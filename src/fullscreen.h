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

#ifndef FULLSCREEN_H
#define FULLSCREEN_H

#include "image-viewer.h"
#include "gthumb-window.h"

typedef struct {
	GtkWidget     *window;
	GtkWidget     *viewer;
	GtkWidget     *toolbar;
	GThumbWindow  *related_win;
	guint          motion_id;
	guint          mouse_hide_id;
	gboolean       wm_state_fullscreen_support;
} FullScreen;


FullScreen *   fullscreen_new    ();

void           fullscreen_close  (FullScreen *fullscreen);

void           fullscreen_start  (FullScreen *fullscreen,
				  GThumbWindow *window);

void           fullscreen_stop   (FullScreen *fullscreen);


#endif /* FULLSCREEN_H */
