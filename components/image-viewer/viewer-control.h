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

#ifndef VIEWER_CONTROL
#define VIEWER_CONTROL

#include <libbonoboui.h>
#include "image-viewer.h"

#define VIEWER_TYPE_CONTROL            (viewer_control_get_type ())
#define VIEWER_CONTROL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), VIEWER_TYPE_CONTROL, ViewerControl))
#define VIEWER_CONTROL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), VIEWER_TYPE_CONTROL, ViewerControlClass))
#define VIEWER_IS_CONTROL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), VIEWER_TYPE_CONTROL))
#define VIEWER_IS_CONTROL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), VIEWER_TYPE_CONTROL))
#define VIEWER_CONTROL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), VIEWER_TYPE_CONTROL, ViewerControlClass))

typedef struct _ViewerControl         ViewerControl;
typedef struct _ViewerControlClass    ViewerControlClass;
typedef struct _ViewerControlPrivate  ViewerControlPrivate;

struct _ViewerControlPrivate {
	BonoboZoomable *zoomable;
	ImageViewer    *viewer;
	BonoboObject   *nautilus_view;

	GtkWidget      *viewer_vscr;
	GtkWidget      *viewer_hscr;
	GtkWidget      *viewer_nav_btn;
	GtkWidget      *viewer_table;
};

struct _ViewerControl {
	BonoboControl         control;
	ViewerControlPrivate *priv;
};

struct _ViewerControlClass {
	BonoboControlClass parent_class;
};

GType           viewer_control_get_type   (void);
BonoboControl  *viewer_control_new        (ImageViewer *viewer);

#endif /* VIEWER_CONTROL */
