/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2008 Free Software Foundation, Inc.
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

#ifndef GTH_CELL_RENDERER_THUMBNAIL_H
#define GTH_CELL_RENDERER_THUMBNAIL_H

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define GTH_TYPE_CELL_RENDERER_THUMBNAIL            (gth_cell_renderer_thumbnail_get_type ())
#define GTH_CELL_RENDERER_THUMBNAIL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTH_TYPE_CELL_RENDERER_THUMBNAIL, GthCellRendererThumbnail))
#define GTH_CELL_RENDERER_THUMBNAIL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GTH_TYPE_CELL_RENDERER_THUMBNAIL, GthCellRendererThumbnailClass))
#define GTH_IS_CELL_RENDERER_THUMBNAIL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTH_TYPE_CELL_RENDERER_THUMBNAIL))
#define GTH_IS_CELL_RENDERER_THUMBNAIL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GTH_TYPE_CELL_RENDERER_THUMBNAIL))
#define GTH_CELL_RENDERER_THUMBNAIL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GTH_TYPE_CELL_RENDERER_THUMBNAIL, GthCellRendererThumbnailClass))

typedef struct _GthCellRendererThumbnail        GthCellRendererThumbnail;
typedef struct _GthCellRendererThumbnailClass   GthCellRendererThumbnailClass;
typedef struct _GthCellRendererThumbnailPrivate GthCellRendererThumbnailPrivate;

struct _GthCellRendererThumbnail {
	GtkCellRenderer parent_instance;
	GthCellRendererThumbnailPrivate * priv;
};

struct _GthCellRendererThumbnailClass {
	GtkCellRendererClass parent_class;
};

GType              gth_cell_renderer_thumbnail_get_type (void);
GtkCellRenderer *  gth_cell_renderer_thumbnail_new      (void);

G_END_DECLS

#endif /* GTH_CELL_RENDERER_THUMBNAIL_H */
