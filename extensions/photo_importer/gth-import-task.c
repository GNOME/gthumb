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
#include "gth-import-task.h"


struct _GthImportTaskPrivate {
	GCancellable *cancellable;
};


static gpointer parent_class = NULL;


static void
gth_import_task_finalize (GObject *object)
{
	GthImportTask *self;

	self = GTH_IMPORT_TASK (object);

	g_object_unref (self->priv->cancellable);

	G_OBJECT_CLASS (parent_class)->finalize (object);
}


static void
gth_import_task_exec (GthTask *task)
{
	/* FIXME */
}


static void
gth_import_task_cancel (GthTask *task)
{
	g_cancellable_cancel (GTH_IMPORT_TASK (task)->priv->cancellable);
}


static void
gth_import_task_class_init (GthImportTaskClass *klass)
{
	GObjectClass *object_class;
	GthTaskClass *task_class;

	parent_class = g_type_class_peek_parent (klass);
	g_type_class_add_private (klass, sizeof (GthImportTaskPrivate));

	object_class = G_OBJECT_CLASS (klass);
	object_class->finalize = gth_import_task_finalize;

	task_class = GTH_TASK_CLASS (klass);
	task_class->exec = gth_import_task_exec;
	task_class->cancel = gth_import_task_cancel;
}


static void
gth_import_task_init (GthImportTask *self)
{
	self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, GTH_TYPE_IMPORT_TASK, GthImportTaskPrivate);
	self->priv->cancellable = g_cancellable_new ();
}


GType
gth_import_task_get_type (void)
{
	static GType type = 0;

	if (! type) {
		GTypeInfo type_info = {
			sizeof (GthImportTaskClass),
			NULL,
			NULL,
			(GClassInitFunc) gth_import_task_class_init,
			NULL,
			NULL,
			sizeof (GthImportTask),
			0,
			(GInstanceInitFunc) gth_import_task_init
		};

		type = g_type_register_static (GTH_TYPE_TASK,
					       "GthImportTask",
					       &type_info,
					       0);
	}

	return type;
}


GthTask *
gth_import_task_new (GFile       *destination,
		     const char  *subfolder,
		     char       **tags,
		     gboolean     move)
{
	GthImportTask *self;

	self = GTH_IMPORT_TASK (g_object_new (GTH_TYPE_IMPORT_TASK, NULL));

	return (GthTask *) self;
}
