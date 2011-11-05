/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2011 Free Software Foundation, Inc.
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

#ifndef GTH_CELL_RENDERER_CAPTION_H
#define GTH_CELL_RENDERER_CAPTION_H

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define GTH_TYPE_CELL_RENDERER_CAPTION            (gth_cell_renderer_caption_get_type ())
#define GTH_CELL_RENDERER_CAPTION(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTH_TYPE_CELL_RENDERER_CAPTION, GthCellRendererCaption))
#define GTH_CELL_RENDERER_CAPTION_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GTH_TYPE_CELL_RENDERER_CAPTION, GthCellRendererCaptionClass))
#define GTH_IS_CELL_RENDERER_CAPTION(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTH_TYPE_CELL_RENDERER_CAPTION))
#define GTH_IS_CELL_RENDERER_CAPTION_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GTH_TYPE_CELL_RENDERER_CAPTION))
#define GTH_CELL_RENDERER_CAPTION_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GTH_TYPE_CELL_RENDERER_CAPTION, GthCellRendererCaptionClass))

typedef struct _GthCellRendererCaption        GthCellRendererCaption;
typedef struct _GthCellRendererCaptionClass   GthCellRendererCaptionClass;
typedef struct _GthCellRendererCaptionPrivate GthCellRendererCaptionPrivate;

struct _GthCellRendererCaption {
	GtkCellRenderer parent_instance;
	GthCellRendererCaptionPrivate * priv;
};

struct _GthCellRendererCaptionClass {
	GtkCellRendererClass parent_class;
};

GType              gth_cell_renderer_caption_get_type     (void);
GtkCellRenderer *  gth_cell_renderer_caption_new          (void);
void               gth_cell_renderer_caption_clear_cache  (GthCellRendererCaption *self);

G_END_DECLS

#endif /* GTH_CELL_RENDERER_CAPTION_H */
