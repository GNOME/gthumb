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
#include "gth-delete-task.h"


struct _GthDeleteTaskPrivate {
	GCancellable *cancellable;
	GList        *file_list;
};


static gpointer parent_class = NULL;


static void
gth_delete_task_finalize (GObject *object)
{
	GthDeleteTask *self;

	self = GTH_DELETE_TASK (object);

	_g_object_list_unref (self->priv->file_list);
	_g_object_unref (self->priv->cancellable);

	G_OBJECT_CLASS (parent_class)->finalize (object);
}


static void
delete_ready_cb (GError   *error,
		 gpointer  user_data)
{
	gth_task_completed (GTH_TASK (user_data), error);
}


static void
gth_delete_task_exec (GthTask *task)
{
	GthDeleteTask *self;

	self = GTH_DELETE_TASK (task);

	gth_task_progress (task, _("Deleting files"), NULL, TRUE, 0.0);

	_g_delete_files_async (self->priv->file_list,
			       TRUE,
			       TRUE,
			       self->priv->cancellable,
			       delete_ready_cb,
			       self);
}


static void
gth_delete_task_cancel (GthTask *task)
{
	GthDeleteTask *self;

	g_return_if_fail (GTH_IS_DELETE_TASK (task));

	self = GTH_DELETE_TASK (task);

	g_cancellable_cancel (self->priv->cancellable);
}


static void
gth_delete_task_class_init (GthDeleteTaskClass *klass)
{
	GObjectClass *object_class;
	GthTaskClass *task_class;

	parent_class = g_type_class_peek_parent (klass);
	g_type_class_add_private (klass, sizeof (GthDeleteTaskPrivate));

	object_class = G_OBJECT_CLASS (klass);
	object_class->finalize = gth_delete_task_finalize;

	task_class = GTH_TASK_CLASS (klass);
	task_class->exec = gth_delete_task_exec;
	task_class->cancel = gth_delete_task_cancel;
}


static void
gth_delete_task_init (GthDeleteTask *self)
{
	self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, GTH_TYPE_DELETE_TASK, GthDeleteTaskPrivate);
	self->priv->cancellable = g_cancellable_new ();
}


GType
gth_delete_task_get_type (void)
{
	static GType type = 0;

	if (! type) {
		GTypeInfo type_info = {
			sizeof (GthDeleteTaskClass),
			NULL,
			NULL,
			(GClassInitFunc) gth_delete_task_class_init,
			NULL,
			NULL,
			sizeof (GthDeleteTask),
			0,
			(GInstanceInitFunc) gth_delete_task_init
		};

		type = g_type_register_static (GTH_TYPE_TASK,
					       "GthDeleteTask",
					       &type_info,
					       0);
	}

	return type;
}


GthTask *
gth_delete_task_new (GList *file_list)
{
	GthDeleteTask *self;

	self = GTH_DELETE_TASK (g_object_new (GTH_TYPE_DELETE_TASK, NULL));
	self->priv->file_list = _g_object_list_ref (file_list);

	return (GthTask *) self;
}
