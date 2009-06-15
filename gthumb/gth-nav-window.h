/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2006-2009 The Free Software Foundation, Inc.
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

#ifndef GTH_NAV_WINDOW_H
#define GTH_NAV_WINDOW_H

#include <gtk/gtk.h>
#include "gth-image-viewer.h"

G_BEGIN_DECLS

#define GTH_TYPE_NAV_WINDOW            (gth_nav_window_get_type ())
#define GTH_NAV_WINDOW(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTH_TYPE_NAV_WINDOW, GthNavWindow))
#define GTH_NAV_WINDOW_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GTH_TYPE_NAV_WINDOW, GthNavWindowClass))
#define GTH_IS_NAV_WINDOW(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTH_TYPE_NAV_WINDOW))
#define GTH_IS_NAV_WINDOW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GTH_TYPE_NAV_WINDOW))
#define GTH_NAV_WINDOW_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), GTH_TYPE_NAV_WINDOW, GthNavWindowClass))

typedef struct _GthNavWindow        GthNavWindow;
typedef struct _GthNavWindowClass   GthNavWindowClass;
typedef struct _GthNavWindowPrivate GthNavWindowPrivate;

struct _GthNavWindow {
	GtkHBox __parent;
	GthNavWindowPrivate *priv;
};

struct _GthNavWindowClass {
	GtkHBoxClass __parent;
};

GType        gth_nav_window_get_type               (void);
GtkWidget *  gth_nav_window_new                    (GthImageViewer *viewer);
void         gth_nav_window_set_scrollbars_visible (GthNavWindow   *window,
						    gboolean        visible);

G_END_DECLS

#endif /* GTH_NAV_WINDOW_H */
