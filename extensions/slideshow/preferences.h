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
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Street #330, Boston, MA 02111-1307, USA.
 */

#ifndef PREFERENCES_H
#define PREFERENCES_H

#include <gthumb.h>

#define  PREF_SLIDESHOW_CHANGE_DELAY     "/apps/gthumb/ext/slideshow/change_delay"
#define  PREF_SLIDESHOW_WRAP_AROUND      "/apps/gthumb/ext/slideshow/wrap_around"
#define  PREF_SLIDESHOW_AUTOMATIC        "/apps/gthumb/ext/slideshow/automatic"
#define  PREF_SLIDESHOW_TRANSITION       "/apps/gthumb/ext/slideshow/transition"

#define  DEFAULT_TRANSITION "random"

void ss__dlg_preferences_construct_cb (GtkWidget  *dialog,
				       GthBrowser *browser,
				       GtkBuilder *builder);

#endif /* CALLBACKS_H */
