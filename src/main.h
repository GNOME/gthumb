/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2001-2005 The Free Software Foundation, Inc.
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

#ifndef __MAIN_H__
#define __MAIN_H__

#include "image-viewer.h"
#include "gth-monitor.h"
#include "gth-window.h"
#include "preferences.h"

extern GthWindow        *current_window;
extern Preferences       preferences;

extern gboolean          StartInFullscreen;
extern gboolean          StartSlideshow;
extern gboolean          ViewFirstImage;
extern gboolean          HideSidebar;
extern gboolean          ExitAll;
extern char             *ImageToDisplay;
extern gboolean          FirstStart;
extern gboolean          ImportPhotos;
extern gboolean          UseViewer;

#define MENU_ICON_SIZE 20.0
#define LIST_ICON_SIZE 20.0

int        get_folder_pixbuf_size_for_list       (GtkWidget  *widget);
int        get_folder_pixbuf_size_for_menu       (GtkWidget  *widget);
GdkPixbuf *get_folder_pixbuf                     (double      icon_size);
G_CONST_RETURN char *  get_stock_id_for_uri      (const char *uri);
GdkPixbuf *get_icon_for_uri                      (GtkWidget  *widget,
						  const char *uri);

#endif /* __MAIN_H__ */
