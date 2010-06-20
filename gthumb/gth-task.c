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

	if (task->priv->cancellable != NULL) {
		g_signal_handler_disconnect (task->priv->cancellable, task->priv->cancellable_cancelled);
		g_object_unref (task->priv->cancellable);
	}

	G_OBJECT_CLASS (parent_class)->finalize (object);
}


static void
base_exec (GthTask *task)
{
	/*gth_task_completed (task, NULL);*/
}


static void
base_cancelled (GthTask *task)
{
	/*gth_task_completed (task, g_error_new_literal (GTH_TASK_ERROR, GTH_TASK_ERROR_CANCELLED, ""));*/
}


static void
gth_task_class_init (GthTaskClass *class)
{
	GObjectClass *object_class;

	parent_class = g_type_class_peek_parent (class);
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
		g_signal_handler_disconnect (task->priv->cancellable, task->priv->cancellable_cancelled);
		g_object_unref (task->priv->cancellable);
	}

	if (cancellable != NULL)
		task->priv->cancellable = _g_object_ref (cancellable);
	else
		task->priv->cancellable = g_cancellable_new ();
	task->priv->cancellable_cancelled = g_signal_connect (task->priv->cancellable,
							      "cancelled",
							      G_CALLBACK (cancellable_cancelled_cb),
							      task);
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
