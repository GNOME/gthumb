/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2005 The Free Software Foundation, Inc.
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

#ifndef GTH_BATCH_OP_H
#define GTH_BATCH_OP_H

#include <gtk/gtkwindow.h>
#include "gth-pixbuf-op.h"

#define GTH_TYPE_BATCH_OP            (gth_batch_op_get_type ())
#define GTH_BATCH_OP(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTH_TYPE_BATCH_OP, GthBatchOp))
#define GTH_BATCH_OP_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GTH_TYPE_BATCH_OP, GthBatchOpClass))
#define GTH_IS_BATCH_OP(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTH_TYPE_BATCH_OP))
#define GTH_IS_BATCH_OP_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GTH_TYPE_BATCH_OP))
#define GTH_BATCH_OP_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), GTH_TYPE_BATCH_OP, GthBatchOpClass))

typedef struct _GthBatchOp            GthBatchOp;
typedef struct _GthBatchOpClass       GthBatchOpClass;
typedef struct _GthBatchOpPrivateData GthBatchOpPrivateData;

struct _GthBatchOp {
	GObject __parent;
	GthBatchOpPrivateData *priv;
};

struct _GthBatchOpClass {
	GObjectClass __parent;

	/* -- signals -- */

	void (*batch_op_progress) (GthBatchOp *pixbuf,
				   float       percentage);
	void (*batch_op_done)     (GthBatchOp *pixbuf_op,
				   gboolean    completed);
};

GType         gth_batch_op_get_type   (void);
GthBatchOp *  gth_batch_op_new        (GthPixbufOp      *pixbuf_op,
				       gpointer          extra_data);
void          gth_batch_op_set_op     (GthBatchOp       *batch_op,
				       GthPixbufOp      *pixbuf_op,
				       gpointer          extra_data);
void          gth_batch_op_start      (GthBatchOp       *batch_op,
				       const char       *image_type,
				       GList            *file_list,
				       const char       *destination,
				       GthOverwriteMode  overwrite_mode,
				       gboolean          remove_original,
				       GtkWindow        *window);
void          gth_batch_op_stop       (GthBatchOp       *batch_op);

#endif /* GTH_BATCH_OP_H */
