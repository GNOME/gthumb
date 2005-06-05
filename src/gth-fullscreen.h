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

#ifndef GTH_FULLSCREEN_H
#define GTH_FULLSCREEN_H

#include "gth-window.h"

#define GTH_TYPE_FULLSCREEN              (gth_fullscreen_get_type ())
#define GTH_FULLSCREEN(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTH_TYPE_FULLSCREEN, GthFullscreen))
#define GTH_FULLSCREEN_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), GTH_TYPE_FULLSCREEN, GthFullscreenClass))
#define GTH_IS_FULLSCREEN(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTH_TYPE_FULLSCREEN))
#define GTH_IS_FULLSCREEN_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), GTH_TYPE_FULLSCREEN))
#define GTH_FULLSCREEN_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS((obj), GTH_TYPE_FULLSCREEN, GthFullscreenClass))

typedef struct _GthFullscreen            GthFullscreen;
typedef struct _GthFullscreenClass       GthFullscreenClass;
typedef struct _GthFullscreenPrivateData GthFullscreenPrivateData;

struct _GthFullscreen
{
	GthWindow __parent;
	GthFullscreenPrivateData *priv;
};

struct _GthFullscreenClass
{
	GthWindowClass __parent_class;
};

GType          gth_fullscreen_get_type               (void);
GtkWidget *    gth_fullscreen_new                    (GdkPixbuf     *image,
						      GList         *file_list);
void           gth_fullscreen_set_metadata_visible   (GthFullscreen *window,
						      gboolean       visible);

#endif /* GTH_FULLSCREEN_H */
