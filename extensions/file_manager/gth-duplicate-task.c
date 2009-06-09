/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2001-2009 Free Software Foundation, Inc.
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
#include "gth-duplicate-task.h"


struct _GthDuplicateTaskPrivate {
	GCancellable *cancellable;
	GList        *file_list;
	GList        *current;
	int           attempt;
};


static gpointer parent_class = NULL;


static void
gth_duplicate_task_finalize (GObject *object)
{
	GthDuplicateTask *self;

	self = GTH_DUPLICATE_TASK (object);

	_g_object_list_unref (self->priv->file_list);
	_g_object_unref (self->priv->cancellable);

	G_OBJECT_CLASS (parent_class)->finalize (object);
}


static GFile *
get_destination (GthFileData *file_data,
	         int          n)
{
	char       *uri;
	char       *uri_no_ext;
	const char *ext;
	char       *new_uri;
	GFile      *new_file;

	uri = g_file_get_uri (file_data->file);
	uri_no_ext = _g_uri_remove_extension (uri);
	ext = _g_uri_get_file_extension (uri);
	new_uri = g_strdup_printf ("%s%%20(%d)%s",
				   uri_no_ext,
				   n + 1,
				   (ext == NULL) ? "" : ext);
	new_file = g_file_new_for_uri (new_uri);

	g_free (new_uri);
	g_free (uri_no_ext);
	g_free (uri);

	return new_file;
}


static void
copy_progress_cb (goffset      current_file,
                  goffset      total_files,
                  GFile       *source,
                  GFile       *destination,
                  goffset      current_num_bytes,
                  goffset      total_num_bytes,
                  gpointer     user_data)
{
	GthDuplicateTask *self = user_data;

	gth_task_progress (GTH_TASK (self), (float) current_num_bytes / total_num_bytes);
}


static void duplicate_current_file (GthDuplicateTask *self);


static void
copy_ready_cb (GError    *error,
               gpointer   user_data)
{
	GthDuplicateTask *self = user_data;

	if (error != NULL) {
		if (g_error_matches (error, G_IO_ERROR, G_IO_ERROR_EXISTS)) {
			g_clear_error (&error);

			self->priv->attempt++;
			duplicate_current_file (self);
			return;
		}

		gth_task_completed (GTH_TASK (self), error);
		return;
	}

	self->priv->current = self->priv->current->next;
	self->priv->attempt = 1;
	duplicate_current_file (self);
}


static void
duplicate_current_file (GthDuplicateTask *self)
{
	GthFileData *file_data;
	GFile       *destination;

	if (self->priv->current == NULL) {
		gth_task_completed (GTH_TASK (self), NULL);
		return;
	}

	file_data = self->priv->current->data;
	destination = get_destination (file_data, self->priv->attempt);

	_g_copy_file_async (file_data->file,
			    destination,
			    G_FILE_COPY_ALL_METADATA,
			    G_PRIORITY_DEFAULT,
			    self->priv->cancellable,
			    copy_progress_cb,
			    self,
			    copy_ready_cb,
			    self);

	g_object_unref (destination);
}


static void
gth_duplicate_task_exec (GthTask *task)
{
	GthDuplicateTask *self;

	g_return_if_fail (GTH_IS_DUPLICATE_TASK (task));

	self = GTH_DUPLICATE_TASK (task);

	self->priv->current = self->priv->file_list;
	self->priv->attempt = 1;
	duplicate_current_file (self);
}


static void
gth_duplicate_task_cancel (GthTask *task)
{
	GthDuplicateTask *self;

	g_return_if_fail (GTH_IS_DUPLICATE_TASK (task));

	self = GTH_DUPLICATE_TASK (task);

	g_cancellable_cancel (self->priv->cancellable);
}


static void
gth_duplicate_task_class_init (GthDuplicateTaskClass *klass)
{
	GObjectClass *object_class;
	GthTaskClass *task_class;

	parent_class = g_type_class_peek_parent (klass);
	g_type_class_add_private (klass, sizeof (GthDuplicateTaskPrivate));

	object_class = G_OBJECT_CLASS (klass);
	object_class->finalize = gth_duplicate_task_finalize;

	task_class = GTH_TASK_CLASS (klass);
	task_class->exec = gth_duplicate_task_exec;
	task_class->cancel = gth_duplicate_task_cancel;
}


static void
gth_duplicate_task_init (GthDuplicateTask *self)
{
	self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, GTH_TYPE_DUPLICATE_TASK, GthDuplicateTaskPrivate);
	self->priv->cancellable = g_cancellable_new ();
}


GType
gth_duplicate_task_get_type (void)
{
	static GType type = 0;

	if (! type) {
		GTypeInfo type_info = {
			sizeof (GthDuplicateTaskClass),
			NULL,
			NULL,
			(GClassInitFunc) gth_duplicate_task_class_init,
			NULL,
			NULL,
			sizeof (GthDuplicateTask),
			0,
			(GInstanceInitFunc) gth_duplicate_task_init
		};

		type = g_type_register_static (GTH_TYPE_TASK,
					       "GthDuplicateTask",
					       &type_info,
					       0);
	}

	return type;
}


GthTask *
gth_duplicate_task_new (GList *file_list)
{
	GthDuplicateTask *self;

	self = GTH_DUPLICATE_TASK (g_object_new (GTH_TYPE_DUPLICATE_TASK, NULL));
	self->priv->file_list = _g_object_list_ref (file_list);

	return (GthTask *) self;
}
