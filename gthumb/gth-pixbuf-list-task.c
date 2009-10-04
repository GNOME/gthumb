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
#include "glib-utils.h"
#include "gth-main.h"
#include "gth-pixbuf-list-task.h"
#include "pixbuf-io.h"


struct _GthPixbufListTaskPrivate {
	GthBrowser *browser;
	GList      *file_list;
	GthTask    *task;
	gulong      task_completed;
	gulong      task_progress;
	gulong      task_dialog;
	GList      *current;
	int         n_current;
	int         n_files;
	GdkPixbuf  *original_pixbuf;
	GdkPixbuf  *new_pixbuf;
};


static gpointer parent_class = NULL;


static void
gth_pixbuf_list_task_finalize (GObject *object)
{
	GthPixbufListTask *self;

	self = GTH_PIXBUF_LIST_TASK (object);

	_g_object_unref (self->priv->original_pixbuf);
	_g_object_unref (self->priv->new_pixbuf);
	g_signal_handler_disconnect (self->priv->task, self->priv->task_completed);
	g_signal_handler_disconnect (self->priv->task, self->priv->task_progress);
	g_signal_handler_disconnect (self->priv->task, self->priv->task_dialog);
	g_object_unref (self->priv->task);
	_g_object_list_unref (self->priv->file_list);

	G_OBJECT_CLASS (parent_class)->finalize (object);
}


static void process_current_file (GthPixbufListTask *self);


static void
process_next_file (GthPixbufListTask *self)
{
	self->priv->current = self->priv->current->next;
	process_current_file (self);
}


static void
pixbuf_saved_cb (GthFileData *file_data,
		 GError      *error,
		 gpointer     user_data)
{
	GthPixbufListTask *self = user_data;
	GFile             *parent;
	GList             *file_list;

	if (error != NULL) {
		gth_task_completed (GTH_TASK (self), error);
		return;
	}

	parent = g_file_get_parent (file_data->file);
	file_list = g_list_append (NULL, file_data->file);
	gth_monitor_folder_changed (gth_main_get_default_monitor (),
				    parent,
				    file_list,
				    GTH_MONITOR_EVENT_CHANGED);

	g_list_free (file_list);
	g_object_unref (parent);

	process_next_file (self);
}


static void
pixbuf_task_dialog_cb (GthTask  *task,
		       gboolean  opened,
		       gpointer  user_data)
{
	gth_task_dialog (GTH_TASK (user_data), opened);
}


static void
pixbuf_task_progress_cb (GthTask    *task,
		         const char *description,
		         const char *details,
		         gboolean    pulse,
		         double      fraction,
		         gpointer    user_data)
{
	/*GthPixbufListTask *self = user_data;*/

	/* FIXME */
}


static void
pixbuf_task_completed_cb (GthTask  *task,
			  GError   *error,
			  gpointer  user_data)
{
	GthPixbufListTask *self = user_data;
	GthFileData       *file_data;
	char              *pixbuf_type;

	if (g_error_matches (error, GTH_TASK_ERROR, GTH_TASK_ERROR_SKIP_TO_NEXT_FILE)) {
		process_next_file (self);
		return;
	}

	if (error != NULL) {
		gth_task_completed (GTH_TASK (self), error);
		return;
	}

	file_data = self->priv->current->data;
	pixbuf_type = get_pixbuf_type_from_mime_type (gth_file_data_get_mime_type (file_data));
	self->priv->new_pixbuf = g_object_ref (GTH_PIXBUF_TASK (task)->dest);
	_gdk_pixbuf_save_async (self->priv->new_pixbuf,
				file_data,
				pixbuf_type,
				NULL,
				NULL,
				pixbuf_saved_cb,
				self);

	g_free (pixbuf_type);
}


static void
file_buffer_ready_cb (void     *buffer,
		      gsize     count,
		      GError   *error,
		      gpointer  user_data)
{
	GthPixbufListTask *self = user_data;
	GInputStream      *istream;

	if (error != NULL) {
		gth_task_completed (GTH_TASK (self), error);
		return;
	}

	istream = g_memory_input_stream_new_from_data (buffer, count, NULL);
	self->priv->original_pixbuf = gdk_pixbuf_new_from_stream (istream, gth_task_get_cancellable (GTH_TASK (self)), &error);
	g_object_unref (istream);

	if (self->priv->original_pixbuf == NULL) {
		gth_task_completed (GTH_TASK (self), error);
		return;
	}

	gth_pixbuf_task_set_source (GTH_PIXBUF_TASK (self->priv->task), self->priv->original_pixbuf);
	gth_task_exec (self->priv->task, gth_task_get_cancellable (GTH_TASK (self)));
}


static void
process_current_file (GthPixbufListTask *self)
{
	GthFileData *file_data;

	if (self->priv->current == NULL) {
		gth_task_completed (GTH_TASK (self), NULL);
		return;
	}

	_g_object_unref (self->priv->original_pixbuf);
	self->priv->original_pixbuf = NULL;
	_g_object_unref (self->priv->new_pixbuf);
	self->priv->new_pixbuf = NULL;

	gth_task_progress (GTH_TASK (self),
			   NULL,
			   NULL,
			   FALSE,
			   ((double) self->priv->n_current + 1) / (self->priv->n_files + 1));

	file_data = self->priv->current->data;
	g_load_file_async (file_data->file,
			   G_PRIORITY_DEFAULT,
			   gth_task_get_cancellable (GTH_TASK (self)),
			   file_buffer_ready_cb,
			   self);
}


static void
gth_pixbuf_list_task_exec (GthTask *task)
{
	GthPixbufListTask *self;

	g_return_if_fail (GTH_IS_PIXBUF_LIST_TASK (task));

	self = GTH_PIXBUF_LIST_TASK (task);

	self->priv->current = self->priv->file_list;
	self->priv->n_current = 0;
	self->priv->n_files = g_list_length (self->priv->file_list);
	process_current_file (self);
}


static void
gth_pixbuf_list_task_class_init (GthPixbufListTaskClass *klass)
{
	GObjectClass *object_class;
	GthTaskClass *task_class;

	parent_class = g_type_class_peek_parent (klass);
	g_type_class_add_private (klass, sizeof (GthPixbufListTaskPrivate));

	object_class = G_OBJECT_CLASS (klass);
	object_class->finalize = gth_pixbuf_list_task_finalize;

	task_class = GTH_TASK_CLASS (klass);
	task_class->exec = gth_pixbuf_list_task_exec;
}


static void
gth_pixbuf_list_task_init (GthPixbufListTask *self)
{
	self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, GTH_TYPE_PIXBUF_LIST_TASK, GthPixbufListTaskPrivate);
	self->priv->original_pixbuf = NULL;
}


GType
gth_pixbuf_list_task_get_type (void)
{
	static GType type = 0;

	if (! type) {
		GTypeInfo type_info = {
			sizeof (GthPixbufListTaskClass),
			NULL,
			NULL,
			(GClassInitFunc) gth_pixbuf_list_task_class_init,
			NULL,
			NULL,
			sizeof (GthPixbufListTask),
			0,
			(GInstanceInitFunc) gth_pixbuf_list_task_init
		};

		type = g_type_register_static (GTH_TYPE_TASK,
					       "GthPixbufListTask",
					       &type_info,
					       0);
	}

	return type;
}


GthTask *
gth_pixbuf_list_task_new (GthBrowser    *browser,
			  GList         *file_list,
			  GthPixbufTask *task)
{
	GthPixbufListTask *self;

	g_return_val_if_fail (task != NULL, NULL);
	g_return_val_if_fail (GTH_IS_PIXBUF_TASK (task), NULL);

	self = GTH_PIXBUF_LIST_TASK (g_object_new (GTH_TYPE_PIXBUF_LIST_TASK, NULL));
	self->priv->browser = browser;
	self->priv->file_list = _g_object_list_ref (file_list);
	self->priv->task = g_object_ref (task);
	self->priv->task_completed = g_signal_connect (self->priv->task,
						       "completed",
						       G_CALLBACK (pixbuf_task_completed_cb),
						       self);
	self->priv->task_progress = g_signal_connect (self->priv->task,
						      "progress",
						      G_CALLBACK (pixbuf_task_progress_cb),
						      self);
	self->priv->task_dialog = g_signal_connect (self->priv->task,
						    "dialog",
						    G_CALLBACK (pixbuf_task_dialog_cb),
						    self);

	return (GthTask *) self;
}
