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

#ifndef _GTH_PIXBUF_OP_H
#define _GTH_PIXBUF_OP_H

#include <glib.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#define GTH_TYPE_PIXBUF_OP            (gth_pixbuf_op_get_type ())
#define GTH_PIXBUF_OP(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTH_TYPE_PIXBUF_OP, GthPixbufOp))
#define GTH_PIXBUF_OP_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GTH_TYPE_PIXBUF_OP, GthPixbufOpClass))
#define GTH_IS_PIXBUF_OP(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTH_TYPE_PIXBUF_OP))
#define GTH_IS_PIXBUF_OP_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GTH_TYPE_PIXBUF_OP))
#define GTH_PIXBUF_OP_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), GTH_TYPE_PIXBUF_OP, GthPixbufOpClass))

typedef struct _GthPixbufOp       GthPixbufOp;
typedef struct _GthPixbufOpClass  GthPixbufOpClass;

typedef void (*PixbufOpFunc) (GthPixbufOp *pixbuf_op);

struct _GthPixbufOp {
	GObject __parent;

	GdkPixbuf    *src;
	GdkPixbuf    *dest;
	gpointer      data;

	PixbufOpFunc  init_func;
	PixbufOpFunc  step_func;
	PixbufOpFunc  release_func;

	gboolean      has_alpha;
	int           bytes_per_pixel;
	int           width, height; 
	int           rowstride;
	guchar       *src_line, *src_pixel;
	guchar       *dest_line, *dest_pixel;

	guint         timeout_id;
	int           line;
	gboolean      interrupt;
};

struct _GthPixbufOpClass {
	GObjectClass __parent;

	/* -- signals -- */

	void (*progress) (GthPixbufOp *pixbuf,
			  float        percentage);
	void (*done)     (GthPixbufOp *pixbuf_op,
			  gboolean     completed);
};

GType         gth_pixbuf_op_get_type   (void);

GthPixbufOp * gth_pixbuf_op_new        (GdkPixbuf       *src,
					GdkPixbuf       *dest,
					PixbufOpFunc     init_func,
					PixbufOpFunc     step_func,
					PixbufOpFunc     release_func,
					gpointer         data);

void          gth_pixbuf_op_start      (GthPixbufOp     *pixbuf_op);

void          gth_pixbuf_op_stop       (GthPixbufOp     *pixbuf_op);

#endif /* _GTH_PIXBUF_OP_H */
