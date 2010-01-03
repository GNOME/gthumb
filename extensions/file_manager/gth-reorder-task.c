/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Reorderright (C) 2009 Free Software Foundation, Inc.
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
 *  You should have received a reorder of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Street #330, Boston, MA 02111-1307, USA.
 */

#include <config.h>
#include "gth-reorder-task.h"


struct _GthReorderTaskPrivate {
	GthFileSource *file_source;
	GthFileData   *destination;
	GList         *files;
	int            new_pos;
};


static gpointer parent_class = NULL;


static void
gth_reorder_task_finalize (GObject *object)
{
	GthReorderTask *self;

	self = GTH_REORDER_TASK (object);

	_g_object_list_unref (self->priv->files);
	_g_object_unref (self->priv->destination);
	_g_object_unref (self->priv->file_source);

	G_OBJECT_CLASS (parent_class)->finalize (object);
}


static void
reorder_done_cb (GObject  *object,
	         GError   *error,
	         gpointer  user_data)
{
	gth_task_completed (GTH_TASK (user_data), error);
}


static void
gth_reorder_task_exec (GthTask *task)
{
	GthReorderTask *self;

	g_return_if_fail (GTH_IS_REORDER_TASK (task));

	self = GTH_REORDER_TASK (task);

	gth_file_source_reorder (self->priv->file_source,
				 self->priv->destination,
			         self->priv->files,
			         self->priv->new_pos,
			         reorder_done_cb,
			         self);
}


static void
gth_reorder_task_cancelled (GthTask *task)
{
	gth_file_source_cancel (GTH_REORDER_TASK (task)->priv->file_source);
}


static void
gth_reorder_task_class_init (GthReorderTaskClass *klass)
{
	GObjectClass *object_class;
	GthTaskClass *task_class;

	parent_class = g_type_class_peek_parent (klass);
	g_type_class_add_private (klass, sizeof (GthReorderTaskPrivate));

	object_class = G_OBJECT_CLASS (klass);
	object_class->finalize = gth_reorder_task_finalize;

	task_class = GTH_TASK_CLASS (klass);
	task_class->exec = gth_reorder_task_exec;
	task_class->cancelled = gth_reorder_task_cancelled;
}


static void
gth_reorder_task_init (GthReorderTask *self)
{
	self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, GTH_TYPE_REORDER_TASK, GthReorderTaskPrivate);
}


GType
gth_reorder_task_get_type (void)
{
	static GType type = 0;

	if (! type) {
		GTypeInfo type_info = {
			sizeof (GthReorderTaskClass),
			NULL,
			NULL,
			(GClassInitFunc) gth_reorder_task_class_init,
			NULL,
			NULL,
			sizeof (GthReorderTask),
			0,
			(GInstanceInitFunc) gth_reorder_task_init
		};

		type = g_type_register_static (GTH_TYPE_TASK,
					       "GthReorderTask",
					       &type_info,
					       0);
	}

	return type;
}


GthTask *
gth_reorder_task_new (GthFileSource *file_source,
		      GthFileData   *destination,
		      GList         *file_list,
		      int            new_pos)
{
	GthReorderTask *self;

	self = GTH_REORDER_TASK (g_object_new (GTH_TYPE_REORDER_TASK, NULL));

	self->priv->file_source = g_object_ref (file_source);
	self->priv->destination = g_object_ref (destination);
	self->priv->new_pos = new_pos;
	self->priv->files = _g_object_list_ref (file_list);

	return (GthTask *) self;
}
