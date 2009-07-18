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
#include "gth-copy-task.h"


struct _GthCopyTaskPrivate {
	GthFileData   *destination;
	GthFileSource *file_source;
	GList         *files;
	gboolean       move;
};


static gpointer parent_class = NULL;


static void
gth_copy_task_finalize (GObject *object)
{
	GthCopyTask *self;

	self = GTH_COPY_TASK (object);

	_g_object_list_unref (self->priv->files);
	_g_object_unref (self->priv->file_source);
	_g_object_unref (self->priv->destination);

	G_OBJECT_CLASS (parent_class)->finalize (object);
}


static void
copy_done_cb (GObject    *object,
	      GError     *error,
	      gpointer    user_data)
{
	gth_task_completed (GTH_TASK (user_data), error);
}


static void
copy_progress_cb (GObject    *object,
		  const char *description,
		  const char *details,
		  gboolean    pulse,
		  double      fraction,
	   	  gpointer    user_data)
{
	GthCopyTask *self = user_data;

	gth_task_progress (GTH_TASK (self), description, details, pulse, fraction);
}


static void
gth_copy_task_exec (GthTask *task)
{
	GthCopyTask *self;

	g_return_if_fail (GTH_IS_COPY_TASK (task));

	self = GTH_COPY_TASK (task);

	gth_file_source_copy (self->priv->file_source,
			      self->priv->destination,
			      self->priv->files,
			      copy_progress_cb,
			      copy_done_cb,
			      self);
}


static void
gth_copy_task_cancel (GthTask *task)
{
	gth_file_source_cancel (GTH_COPY_TASK (task)->priv->file_source);
}


static void
gth_copy_task_class_init (GthCopyTaskClass *klass)
{
	GObjectClass *object_class;
	GthTaskClass *task_class;

	parent_class = g_type_class_peek_parent (klass);
	g_type_class_add_private (klass, sizeof (GthCopyTaskPrivate));

	object_class = G_OBJECT_CLASS (klass);
	object_class->finalize = gth_copy_task_finalize;

	task_class = GTH_TASK_CLASS (klass);
	task_class->exec = gth_copy_task_exec;
	task_class->cancel = gth_copy_task_cancel;
}


static void
gth_copy_task_init (GthCopyTask *self)
{
	self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, GTH_TYPE_COPY_TASK, GthCopyTaskPrivate);
}


GType
gth_copy_task_get_type (void)
{
	static GType type = 0;

	if (! type) {
		GTypeInfo type_info = {
			sizeof (GthCopyTaskClass),
			NULL,
			NULL,
			(GClassInitFunc) gth_copy_task_class_init,
			NULL,
			NULL,
			sizeof (GthCopyTask),
			0,
			(GInstanceInitFunc) gth_copy_task_init
		};

		type = g_type_register_static (GTH_TYPE_TASK,
					       "GthCopyTask",
					       &type_info,
					       0);
	}

	return type;
}


GthTask *
gth_copy_task_new (GthFileSource *file_source,
		   GthFileData   *destination,
		   gboolean       move,
		   GList         *files)
{
	GthCopyTask *self;

	self = GTH_COPY_TASK (g_object_new (GTH_TYPE_COPY_TASK, NULL));

	self->priv->file_source = g_object_ref (file_source);
	self->priv->destination = g_object_ref (destination);
	self->priv->move = move;
	self->priv->files = _g_object_list_ref (files);

	return (GthTask *) self;
}
