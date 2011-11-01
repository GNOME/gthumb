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
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <config.h>
#include <glib.h>
#include "glib-utils.h"
#include "gth-marshal.h"
#include "gth-task.h"


/* Signals */
enum {
	COMPLETED,
	PROGRESS,
	DIALOG,
	LAST_SIGNAL
};

struct _GthTaskPrivate
{
	gboolean      running;
	GCancellable *cancellable;
	gulong        cancellable_cancelled;
};


static guint gth_task_signals[LAST_SIGNAL] = { 0 };


GQuark
gth_task_error_quark (void)
{
	return g_quark_from_static_string ("gth-task-error-quark");
}


G_DEFINE_TYPE (GthTask, gth_task, G_TYPE_OBJECT)


static void
gth_task_finalize (GObject *object)
{
	GthTask *task;

	task = GTH_TASK (object);

	if (task->priv->cancellable != NULL) {
		g_cancellable_disconnect (task->priv->cancellable, task->priv->cancellable_cancelled);
		g_object_unref (task->priv->cancellable);
	}

	G_OBJECT_CLASS (gth_task_parent_class)->finalize (object);
}


static void
base_exec (GthTask *task)
{
	/* void */
}


static void
base_cancelled (GthTask *task)
{
	/* void */
}


static void
gth_task_class_init (GthTaskClass *class)
{
	GObjectClass *object_class;

	g_type_class_add_private (class, sizeof (GthTaskPrivate));

	object_class = (GObjectClass*) class;
	object_class->finalize = gth_task_finalize;

	class->exec = base_exec;
	class->cancelled = base_cancelled;

	/* signals */

	gth_task_signals[COMPLETED] =
		g_signal_new ("completed",
			      G_TYPE_FROM_CLASS (class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (GthTaskClass, completed),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__BOXED,
			      G_TYPE_NONE,
			      1,
			      G_TYPE_ERROR);

	gth_task_signals[PROGRESS] =
		g_signal_new ("progress",
			      G_TYPE_FROM_CLASS (class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (GthTaskClass, progress),
			      NULL, NULL,
			      gth_marshal_VOID__STRING_STRING_BOOLEAN_DOUBLE,
			      G_TYPE_NONE,
			      4,
			      G_TYPE_STRING,
			      G_TYPE_STRING,
			      G_TYPE_BOOLEAN,
			      G_TYPE_DOUBLE);

	gth_task_signals[DIALOG] =
		g_signal_new ("dialog",
			      G_TYPE_FROM_CLASS (class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (GthTaskClass, dialog),
			      NULL, NULL,
			      gth_marshal_VOID__BOOLEAN_POINTER,
			      G_TYPE_NONE,
			      2,
			      G_TYPE_BOOLEAN,
			      G_TYPE_POINTER);
}


static void
gth_task_init (GthTask *self)
{
	self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, GTH_TYPE_TASK, GthTaskPrivate);
	self->priv->running = FALSE;
	self->priv->cancellable = NULL;
	self->priv->cancellable_cancelled = 0;
}


static void
cancellable_cancelled_cb (GCancellable *cancellable,
			  gpointer      user_data)
{
	GthTask *task = user_data;

	GTH_TASK_GET_CLASS (task)->cancelled (task);
}


void
gth_task_exec (GthTask      *task,
	       GCancellable *cancellable)
{
	if (task->priv->running)
		return;

	if (task->priv->cancellable != NULL) {
		g_cancellable_disconnect (task->priv->cancellable, task->priv->cancellable_cancelled);
		g_object_unref (task->priv->cancellable);
	}

	if (cancellable != NULL)
		task->priv->cancellable = _g_object_ref (cancellable);
	else
		task->priv->cancellable = g_cancellable_new ();
	task->priv->cancellable_cancelled = g_cancellable_connect (task->priv->cancellable,
								   G_CALLBACK (cancellable_cancelled_cb),
							           task,
							           NULL);
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
	if (task->priv->cancellable != NULL)
		g_cancellable_cancel (task->priv->cancellable);
	else
		cancellable_cancelled_cb (NULL, task);
}


GCancellable *
gth_task_get_cancellable (GthTask *task)
{
	return task->priv->cancellable;
}


void
gth_task_completed (GthTask *task,
		    GError  *error)
{
	task->priv->running = FALSE;
	g_signal_emit (task, gth_task_signals[COMPLETED], 0, error);
}


void
gth_task_dialog (GthTask   *task,
		 gboolean   opened,
		 GtkWidget *dialog)
{
	g_signal_emit (task, gth_task_signals[DIALOG], 0, opened, dialog);
}


void
gth_task_progress (GthTask    *task,
		   const char *description,
		   const char *details,
		   gboolean    pulse,
		   double      fraction)
{
	g_signal_emit (task, gth_task_signals[PROGRESS], 0, description, details, pulse, fraction);
}
