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

#include <config.h>
#include <glib.h>
#include "gth-marshal.h"
#include "gth-task.h"


/* Signals */
enum {
	COMPLETED,
	PROGRESS,
	LAST_SIGNAL
};

struct _GthTaskPrivate
{
	gboolean running;
};


static GObjectClass *parent_class = NULL;
static guint gth_task_signals[LAST_SIGNAL] = { 0 };


GQuark
gth_task_error_quark (void)
{
	return g_quark_from_static_string ("gth-task-error-quark");
}


static void
gth_task_finalize (GObject *object)
{
	GthTask *task;

	task = GTH_TASK (object);

	if (task->priv != NULL) {
		g_free (task->priv);
		task->priv = NULL;
	}

	G_OBJECT_CLASS (parent_class)->finalize (object);
}


static void
base_exec (GthTask *task)
{
	gth_task_completed (task, NULL);
}


static void
base_cancel (GthTask *task)
{
	gth_task_completed (task, g_error_new_literal (GTH_TASK_ERROR, GTH_TASK_ERROR_CANCELLED, NULL));
}


static void
gth_task_class_init (GthTaskClass *class)
{
	GObjectClass *object_class;

	parent_class = g_type_class_peek_parent (class);
	object_class = (GObjectClass*) class;

	object_class->finalize = gth_task_finalize;

	class->exec = base_exec;
	class->cancel = base_cancel;

	/* signals */

	gth_task_signals[COMPLETED] =
		g_signal_new ("completed",
			      G_TYPE_FROM_CLASS (class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (GthTaskClass, completed),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__POINTER,
			      G_TYPE_NONE,
			      1,
			      G_TYPE_POINTER);

	gth_task_signals[PROGRESS] =
		g_signal_new ("progress",
			      G_TYPE_FROM_CLASS (class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (GthTaskClass, progress),
			      NULL, NULL,
			      gth_marshal_VOID__STRING_BOOLEAN_DOUBLE,
			      G_TYPE_NONE,
			      3,
			      G_TYPE_STRING,
			      G_TYPE_BOOLEAN,
			      G_TYPE_DOUBLE);
}


static void
gth_task_init (GthTask *task)
{
	task->priv = g_new0 (GthTaskPrivate, 1);
	task->priv->running = FALSE;
}


GType
gth_task_get_type (void)
{
	static GType type = 0;

	if (! type) {
		GTypeInfo type_info = {
			sizeof (GthTaskClass),
			NULL,
			NULL,
			(GClassInitFunc) gth_task_class_init,
			NULL,
			NULL,
			sizeof (GthTask),
			0,
			(GInstanceInitFunc) gth_task_init
		};

		type = g_type_register_static (G_TYPE_OBJECT,
					       "GthTask",
					       &type_info,
					       0);
	}

	return type;
}


GthTask *
gth_task_new (void)
{
	return (GthTask*) g_object_new (GTH_TYPE_TASK, NULL);
}


void
gth_task_exec (GthTask *task)
{
	task->priv->running = TRUE;
	GTH_TASK_GET_CLASS (task)->exec (task);
}


gboolean
gth_task_is_running (GthTask *task)
{
	return task->priv->running;
}


void
gth_task_cancel (GthTask *task)
{
	if (task->priv->running)
		GTH_TASK_GET_CLASS (task)->cancel (task);
}


void
gth_task_completed (GthTask *task,
		    GError  *error)
{
	task->priv->running = FALSE;
	g_signal_emit (task, gth_task_signals[COMPLETED], 0, error);
}


void
gth_task_progress (GthTask    *task,
		   const char *text,
		   gboolean    pulse,
		   double      fraction)
{
	g_signal_emit (task, gth_task_signals[PROGRESS], 0, text, pulse, fraction);
}
