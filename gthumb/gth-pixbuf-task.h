/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2001-2009 The Free Software Foundation, Inc.
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

#ifndef _GTH_PIXBUF_TASK_H
#define _GTH_PIXBUF_TASK_H

#include <glib.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include "gth-task.h"

G_BEGIN_DECLS

#define GTH_TYPE_PIXBUF_TASK            (gth_pixbuf_task_get_type ())
#define GTH_PIXBUF_TASK(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTH_TYPE_PIXBUF_TASK, GthPixbufTask))
#define GTH_PIXBUF_TASK_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GTH_TYPE_PIXBUF_TASK, GthPixbufTaskClass))
#define GTH_IS_PIXBUF_TASK(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTH_TYPE_PIXBUF_TASK))
#define GTH_IS_PIXBUF_TASK_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GTH_TYPE_PIXBUF_TASK))
#define GTH_PIXBUF_TASK_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), GTH_TYPE_PIXBUF_TASK, GthPixbufTaskClass))

typedef struct _GthPixbufTask        GthPixbufTask;
typedef struct _GthPixbufTaskClass   GthPixbufTaskClass;

typedef void (*PixbufOpFunc) (GthPixbufTask *pixbuf_task);

enum {
	RED_PIX   = 0,
	GREEN_PIX = 1,
	BLUE_PIX  = 2,
	ALPHA_PIX = 3
};

struct _GthPixbufTask {
	GthTask __parent;

	GdkPixbuf    *src;
	GdkPixbuf    *dest;
	gpointer      data;

	PixbufOpFunc  init_func;
	PixbufOpFunc  step_func;
	PixbufOpFunc  release_func;
	PixbufOpFunc  free_data_func;

	gboolean      single_step;

	gboolean      has_alpha;
	int           bytes_per_pixel;
	int           width, height;
	int           rowstride;
	guchar       *src_line, *src_pixel;
	guchar       *dest_line, *dest_pixel;

	gboolean      ltr, first_step, last_step;
	guint         timeout_id;
	int           line;
	int           line_step;
	int           column;
	gboolean      interrupt;

	const char   *description;
};

struct _GthPixbufTaskClass {
	GthTaskClass __parent;
};

GType         gth_pixbuf_task_get_type        (void);
GthTask *     gth_pixbuf_task_new             (const char     *description,
					       GdkPixbuf      *src,
					       GdkPixbuf      *dest,
					       PixbufOpFunc    init_func,
					       PixbufOpFunc    step_func,
					       PixbufOpFunc    release_func,
					       gpointer        data);
void          gth_pixbuf_task_set_single_step (GthPixbufTask  *pixbuf_task,
					       gboolean        single_step);
void          gth_pixbuf_task_set_pixbufs     (GthPixbufTask  *pixbuf_task,
					       GdkPixbuf      *src,
					       GdkPixbuf      *dest);

G_END_DECLS

#endif /* _GTH_PIXBUF_TASK_H */
