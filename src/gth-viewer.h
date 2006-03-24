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

#ifndef GTH_VIEWER_H
#define GTH_VIEWER_H

#include "gth-window.h"

#define GTH_TYPE_VIEWER              (gth_viewer_get_type ())
#define GTH_VIEWER(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTH_TYPE_VIEWER, GthViewer))
#define GTH_VIEWER_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), GTH_VIEWER_TYPE, GthViewerClass))
#define GTH_IS_VIEWER(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTH_TYPE_VIEWER))
#define GTH_IS_VIEWER_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), GTH_TYPE_VIEWER))
#define GTH_VIEWER_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS((obj), GTH_TYPE_VIEWER, GthViewerClass))

typedef struct _GthViewer            GthViewer;
typedef struct _GthViewerClass       GthViewerClass;
typedef struct _GthViewerPrivateData GthViewerPrivateData;

struct _GthViewer
{
	GthWindow __parent;
	GthViewerPrivateData *priv;
};

struct _GthViewerClass
{
	GthWindowClass __parent_class;
};

GType          gth_viewer_get_type                 (void);
GtkWidget *    gth_viewer_new                      (const char  *filename);
void           gth_viewer_load                     (GthViewer   *viewer,
						    const char  *filename);
void           gth_viewer_set_metadata_visible     (GthViewer   *viewer,
						    gboolean     visible);
void           gth_viewer_set_single_window        (GthViewer   *viewer,
						    gboolean     value);

GtkWidget *    gth_viewer_get_current_viewer       (void);

#endif /* GTH_VIEWER_H */
