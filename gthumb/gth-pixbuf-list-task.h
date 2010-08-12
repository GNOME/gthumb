/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2009 The Free Software Foundation, Inc.
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

#ifndef GTH_PIXBUF_LIST_TASK_H
#define GTH_PIXBUF_LIST_TASK_H

#include <glib.h>
#include "gth-browser.h"
#include "gth-pixbuf-task.h"
#include "typedefs.h"

G_BEGIN_DECLS

#define GTH_TYPE_PIXBUF_LIST_TASK            (gth_pixbuf_list_task_get_type ())
#define GTH_PIXBUF_LIST_TASK(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTH_TYPE_PIXBUF_LIST_TASK, GthPixbufListTask))
#define GTH_PIXBUF_LIST_TASK_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GTH_TYPE_PIXBUF_LIST_TASK, GthPixbufListTaskClass))
#define GTH_IS_PIXBUF_LIST_TASK(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTH_TYPE_PIXBUF_LIST_TASK))
#define GTH_IS_PIXBUF_LIST_TASK_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GTH_TYPE_PIXBUF_LIST_TASK))
#define GTH_PIXBUF_LIST_TASK_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), GTH_TYPE_PIXBUF_LIST_TASK, GthPixbufListTaskClass))

typedef struct _GthPixbufListTask        GthPixbufListTask;
typedef struct _GthPixbufListTaskClass   GthPixbufListTaskClass;
typedef struct _GthPixbufListTaskPrivate GthPixbufListTaskPrivate;

struct _GthPixbufListTask {
	GthTask __parent;
	GthPixbufListTaskPrivate *priv;
};

struct _GthPixbufListTaskClass {
	GthTaskClass __parent;
};

GType      gth_pixbuf_list_task_get_type             (void);
GthTask *  gth_pixbuf_list_task_new                  (GthBrowser           *browser,
						      GList                *file_list, /* GthFileData list */
						      GthPixbufTask        *task);
void       gth_pixbuf_list_task_set_destination      (GthPixbufListTask    *self,
						      GFile                *folder);
void       gth_pixbuf_list_task_set_overwrite_mode   (GthPixbufListTask    *self,
						      GthOverwriteMode      overwrite_mode);
void       gth_pixbuf_list_task_set_output_mime_type (GthPixbufListTask    *self,
						      const char           *mime_type);
G_END_DECLS

#endif /* GTH_PIXBUF_LIST_TASK_H */
