/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2009 Free Software Foundation, Inc.
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

#ifndef GTH_TASK_H
#define GTH_TASK_H

#include <glib-object.h>

G_BEGIN_DECLS

#define GTH_TASK_ERROR gth_task_error_quark ()

typedef enum {
	GTH_TASK_ERROR_FAILED,
	GTH_TASK_ERROR_CANCELLED
} GthTaskErrorEnum;

#define GTH_TYPE_TASK         (gth_task_get_type ())
#define GTH_TASK(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), GTH_TYPE_TASK, GthTask))
#define GTH_TASK_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST ((k), GTH_TYPE_TASK, GthTaskClass))
#define GTH_IS_TASK(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), GTH_TYPE_TASK))
#define GTH_IS_TASK_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), GTH_TYPE_TASK))
#define GTH_TASK_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS((o), GTH_TYPE_TASK, GthTaskClass))

typedef struct _GthTask         GthTask;
typedef struct _GthTaskPrivate  GthTaskPrivate;
typedef struct _GthTaskClass    GthTaskClass;

struct _GthTask
{
	GObject __parent;
	GthTaskPrivate *priv;
};

struct _GthTaskClass
{
	GObjectClass __parent_class;

	/*< signals >*/

	void  (*completed)    (GthTask    *task,
			       GError     *error);
	void  (*progress)     (GthTask    *task,
			       const char *text,
			       gboolean    pulse,
			       double      fraction);

	/*< virtual functions >*/

	void  (*exec)         (GthTask    *task);
	void  (*cancel)       (GthTask    *task);
};

GQuark      gth_task_error_quark (void);

GType       gth_task_get_type    (void) G_GNUC_CONST;
GthTask *   gth_task_new         (void);
void        gth_task_exec        (GthTask    *task);
gboolean    gth_task_is_running  (GthTask    *task);
void        gth_task_cancel      (GthTask    *task);
void        gth_task_completed   (GthTask    *task,
				  GError     *error);
void        gth_task_progress    (GthTask    *task,
				  const char *text,
			          gboolean    pulse,
			          double      fraction);

G_END_DECLS

#endif /* GTH_TASK_H */
