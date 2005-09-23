/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2001, 2003 Free Software Foundation, Inc.
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

#include <glib.h>
#include "gthumb-marshal.h"
#include "gth-pixbuf-op.h"


#define N_STEPS          20   /* number of lines to process in a single 
			       * timeout handler. */
#define PROGRESS_STEP    5    /* notify progress each PROGRESS_STEP lines. */ 


enum {
	PIXBUF_OP_PROGRESS,
	PIXBUF_OP_DONE,
	LAST_SIGNAL
};


static GObjectClass *parent_class = NULL;
static guint         gth_pixbuf_op_signals[LAST_SIGNAL] = { 0 };


static void
release_pixbufs (GthPixbufOp *pixbuf_op)
{
	if (pixbuf_op->src != NULL) {
		g_object_unref (pixbuf_op->src);
		pixbuf_op->src = NULL;
	}

	if (pixbuf_op->dest != NULL) {
		g_object_unref (pixbuf_op->dest);
		pixbuf_op->dest = NULL;
	}
}


static void 
gth_pixbuf_op_finalize (GObject *object)
{
	GthPixbufOp *pixbuf_op;

	g_return_if_fail (GTH_IS_PIXBUF_OP (object));
	pixbuf_op = GTH_PIXBUF_OP (object);

	if (pixbuf_op->timeout_id != 0) {
		g_source_remove (pixbuf_op->timeout_id);
		pixbuf_op->timeout_id = 0;
	}
	release_pixbufs (pixbuf_op);
	if (pixbuf_op->free_data_func != NULL)
		(*pixbuf_op->free_data_func)(pixbuf_op);

	G_OBJECT_CLASS (parent_class)->finalize (object);
}


static void
gth_pixbuf_op_class_init (GthPixbufOpClass *class)
{
	GObjectClass *object_class;

	parent_class = g_type_class_peek_parent (class);

	gth_pixbuf_op_signals[PIXBUF_OP_PROGRESS] =
		g_signal_new ("pixbuf_op_progress",
			      G_TYPE_FROM_CLASS (class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (GthPixbufOpClass, pixbuf_op_progress),
			      NULL, NULL,
			      gthumb_marshal_VOID__FLOAT,
			      G_TYPE_NONE, 
			      1,
			      G_TYPE_FLOAT);
	gth_pixbuf_op_signals[PIXBUF_OP_DONE] =
		g_signal_new ("pixbuf_op_done",
			      G_TYPE_FROM_CLASS (class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (GthPixbufOpClass, pixbuf_op_done),
			      NULL, NULL,
			      gthumb_marshal_VOID__BOOLEAN,
			      G_TYPE_NONE, 
			      1,
			      G_TYPE_BOOLEAN);

	object_class = G_OBJECT_CLASS (class);
	object_class->finalize = gth_pixbuf_op_finalize;
}


static void
gth_pixbuf_op_init (GthPixbufOp *pixbuf_op)
{
	pixbuf_op->src = NULL;
	pixbuf_op->dest = NULL;
	pixbuf_op->data = NULL;

	pixbuf_op->init_func = NULL;
	pixbuf_op->step_func = NULL;
	pixbuf_op->release_func = NULL;
	pixbuf_op->free_data_func = NULL;

	pixbuf_op->src_line = NULL;
	pixbuf_op->src_pixel = NULL;
	pixbuf_op->dest_line = NULL;
	pixbuf_op->dest_pixel = NULL;

	pixbuf_op->ltr = TRUE;

	pixbuf_op->timeout_id = 0;
	pixbuf_op->line = 0;
	pixbuf_op->interrupt = FALSE;
}


GType
gth_pixbuf_op_get_type ()
{
	static GType type = 0;

	if (! type) {
		GTypeInfo type_info = {
			sizeof (GthPixbufOpClass),
                        NULL,
                        NULL,
                        (GClassInitFunc) gth_pixbuf_op_class_init,
                        NULL,
                        NULL,
                        sizeof (GthPixbufOp),
                        0,
                        (GInstanceInitFunc) gth_pixbuf_op_init
                };

                type = g_type_register_static (G_TYPE_OBJECT,
                                               "GthPixbufOp",
                                               &type_info,
                                               0);
        }

        return type;
}


GthPixbufOp *
gth_pixbuf_op_new (GdkPixbuf    *src,
		   GdkPixbuf    *dest,
		   PixbufOpFunc  init_func,
		   PixbufOpFunc  step_func,
		   PixbufOpFunc  release_func,
		   gpointer      data)
{
	GthPixbufOp *pixbuf_op;

	pixbuf_op = GTH_PIXBUF_OP (g_object_new (GTH_TYPE_PIXBUF_OP, NULL));

	pixbuf_op->init_func = init_func;
	pixbuf_op->step_func = step_func;
	pixbuf_op->release_func = release_func;
	pixbuf_op->data = data;

	gth_pixbuf_op_set_pixbufs (pixbuf_op, src, dest);

	return pixbuf_op;
}


static gboolean
one_step (gpointer data)
{
	GthPixbufOp *pixbuf_op = data;
	int          dir = 1;

	if (pixbuf_op->single_step)
		(*pixbuf_op->step_func) (pixbuf_op);	

	if ((pixbuf_op->line >= pixbuf_op->height)
	    || pixbuf_op->single_step
	    || pixbuf_op->interrupt) {
		if (pixbuf_op->release_func != NULL)
			(*pixbuf_op->release_func) (pixbuf_op);
		g_signal_emit (G_OBJECT (pixbuf_op),
			       gth_pixbuf_op_signals[PIXBUF_OP_DONE],
			       0,
			       ! pixbuf_op->interrupt);
		return FALSE;
	}

	pixbuf_op->src_pixel = pixbuf_op->src_line;
	pixbuf_op->src_line += pixbuf_op->rowstride;
	
	pixbuf_op->dest_pixel = pixbuf_op->dest_line;
	pixbuf_op->dest_line += pixbuf_op->rowstride;
	
	if (pixbuf_op->line % PROGRESS_STEP == 0) 
		g_signal_emit (G_OBJECT (pixbuf_op),
			       gth_pixbuf_op_signals[PIXBUF_OP_PROGRESS],
			       0,
			       (float) pixbuf_op->line / pixbuf_op->height);

	if (! pixbuf_op->ltr) { /* right to left */
		int ofs = (pixbuf_op->width - 1) * pixbuf_op->bytes_per_pixel;
		pixbuf_op->src_pixel += ofs;
		pixbuf_op->dest_pixel += ofs;
		dir = -1;
		pixbuf_op->column = pixbuf_op->width - 1;
	} else
		pixbuf_op->column = 0;

	pixbuf_op->line_step = 0;
	while (pixbuf_op->line_step < pixbuf_op->width) {
		(*pixbuf_op->step_func) (pixbuf_op);
		pixbuf_op->src_pixel += dir * pixbuf_op->bytes_per_pixel;
		pixbuf_op->dest_pixel += dir * pixbuf_op->bytes_per_pixel;
		pixbuf_op->column += dir;
		pixbuf_op->line_step++;
	}

	pixbuf_op->line++;

	return TRUE;
}


static gboolean
step (gpointer data)
{
	GthPixbufOp *pixbuf_op = data;
	int          i;

	if (pixbuf_op->timeout_id != 0) {
		g_source_remove (pixbuf_op->timeout_id);
		pixbuf_op->timeout_id = 0;
	}
	
	for (i = 0; i < N_STEPS; i++)
		if (! one_step (data))
			return FALSE;

	pixbuf_op->timeout_id = g_idle_add (step, pixbuf_op);

	return FALSE;
}


void
gth_pixbuf_op_set_single_step (GthPixbufOp *pixbuf_op,
			       gboolean     single_step)
{
	pixbuf_op->single_step = single_step;
}


void
gth_pixbuf_op_set_pixbufs (GthPixbufOp  *pixbuf_op,
			   GdkPixbuf    *src,
			   GdkPixbuf    *dest)
{
	if (src == NULL)
		return;

	/* NOTE that src and dest MAY be the same pixbuf! */
  
        g_return_if_fail (GDK_IS_PIXBUF (src));
	if (dest != NULL) {
		g_return_if_fail (GDK_IS_PIXBUF (dest));
		g_return_if_fail (gdk_pixbuf_get_has_alpha (src) == gdk_pixbuf_get_has_alpha (dest));
		g_return_if_fail (gdk_pixbuf_get_width (src) == gdk_pixbuf_get_width (dest));
		g_return_if_fail (gdk_pixbuf_get_height (src) == gdk_pixbuf_get_height (dest));
		g_return_if_fail (gdk_pixbuf_get_colorspace (src) == gdk_pixbuf_get_colorspace (dest));
	}

	release_pixbufs (pixbuf_op);

	g_object_ref (src);
	pixbuf_op->src = src;

	pixbuf_op->has_alpha       = gdk_pixbuf_get_has_alpha (src);
	pixbuf_op->bytes_per_pixel = pixbuf_op->has_alpha ? 4 : 3;
	pixbuf_op->width           = gdk_pixbuf_get_width (src);
	pixbuf_op->height          = gdk_pixbuf_get_height (src);
	pixbuf_op->rowstride       = gdk_pixbuf_get_rowstride (src);
	pixbuf_op->src_line        = gdk_pixbuf_get_pixels (src);

	if (dest != NULL) {
		g_object_ref (dest);
		pixbuf_op->dest = dest;
		pixbuf_op->dest_line = gdk_pixbuf_get_pixels (dest);
	}
}


void
gth_pixbuf_op_start (GthPixbufOp  *pixbuf_op)
{
	g_return_if_fail (GTH_IS_PIXBUF_OP (pixbuf_op));
	g_return_if_fail (pixbuf_op->src != NULL);

	pixbuf_op->line = 0;
	if (pixbuf_op->init_func != NULL)
		(*pixbuf_op->init_func) (pixbuf_op);

	step (pixbuf_op);
}


void
gth_pixbuf_op_stop (GthPixbufOp  *pixbuf_op)
{
	g_return_if_fail (GTH_IS_PIXBUF_OP (pixbuf_op));
	pixbuf_op->interrupt = TRUE;
}
