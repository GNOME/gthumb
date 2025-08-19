/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2010 Free Software Foundation, Inc.
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
#include "gth-reset-orientation-task.h"
#include "rotation-utils.h"


struct _GthResetOrientationTaskPrivate {
	GthBrowser    *browser;
	GList         *file_list;
	GList         *current;
	GthFileData   *file_data;
	int            n_image;
	int            n_images;
};


G_DEFINE_TYPE_WITH_CODE (GthResetOrientationTask,
			 gth_reset_orientation_task,
			 GTH_TYPE_TASK,
			 G_ADD_PRIVATE (GthResetOrientationTask))


static void
gth_reset_orientation_task_finalize (GObject *object)
{
	GthResetOrientationTask *self;

	self = GTH_RESET_ORIENTATION_TASK (object);

	_g_object_unref (self->priv->file_data);
	_g_object_list_unref (self->priv->file_list);

	G_OBJECT_CLASS (gth_reset_orientation_task_parent_class)->finalize (object);
}


static void transform_current_file (GthResetOrientationTask *self);


static void
transform_next_file (GthResetOrientationTask *self)
{
	self->priv->n_image++;
	self->priv->current = self->priv->current->next;
	transform_current_file (self);
}


/*static void
write_metadata_ready_cb (GObject      *source_object,
	 	 	 GAsyncResult *result,
	 	 	 gpointer      user_data)
{
	GthResetOrientationTask *self = user_data;
	GFile                   *parent;
	GList                   *file_list;
	GError                  *error = NULL;

	if (! _g_write_metadata_finish (result, &error)) {
		gth_task_completed (GTH_TASK (self), error);
		return;
	}

	parent = g_file_get_parent (self->priv->file_data->file);
	file_list = g_list_append (NULL, self->priv->file_data->file);
	gth_monitor_folder_changed (gth_main_get_default_monitor (),
				    parent,
				    file_list,
				    GTH_MONITOR_EVENT_CHANGED);

	g_list_free (file_list);
	g_object_unref (parent);

	transform_next_file (self);
}*/


static void
transform_file_ready_cb (GObject *source_object,
			 GAsyncResult *res,
			 gpointer user_data)
{
	GthResetOrientationTask *self = user_data;
	GError *error = NULL;

	if (!apply_transformation_finish (res, &error)) {
		gth_task_completed (GTH_TASK (self), error);
		return;
	}

	GFile *parent = g_file_get_parent (self->priv->file_data->file);
	GList *file_list = g_list_append (NULL, self->priv->file_data->file);
	gth_monitor_folder_changed (gth_main_get_default_monitor (),
				    parent,
				    file_list,
				    GTH_MONITOR_EVENT_CHANGED);

	g_list_free (file_list);
	g_object_unref (parent);

	transform_next_file (self);
}


static void
file_info_ready_cb (GList    *files,
		    GError   *error,
		    gpointer  user_data)
{
	GthResetOrientationTask *self = user_data;

	if (error != NULL) {
		gth_task_completed (GTH_TASK (self), error);
		return;
	}

	_g_object_unref (self->priv->file_data);
	self->priv->file_data = g_object_ref ((GthFileData *) files->data);

	gth_task_progress (GTH_TASK (self),
			   _("Saving images"),
			   g_file_info_get_display_name (self->priv->file_data->info),
			   FALSE,
			   (double) (self->priv->n_image + 1) / (self->priv->n_images + 1));

	apply_transformation_async (self->priv->file_data,
		GTH_TRANSFORM_NONE,
		GTH_TRANSFORM_FLAG_LOAD_METADATA | GTH_TRANSFORM_FLAG_RESET,
		gth_task_get_cancellable (GTH_TASK (self)),
		transform_file_ready_cb,
		self);
}


static void
transform_current_file (GthResetOrientationTask *self)
{
	GFile *file;
	GList *singleton;

	if (self->priv->current == NULL) {
		gth_task_completed (GTH_TASK (self), NULL);
		return;
	}

	file = self->priv->current->data;
	singleton = g_list_append (NULL, g_object_ref (file));
	_g_query_all_metadata_async (singleton,
				     GTH_LIST_DEFAULT,
				     (G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE ","
				      G_FILE_ATTRIBUTE_STANDARD_DISPLAY_NAME),
				     gth_task_get_cancellable (GTH_TASK (self)),
				     file_info_ready_cb,
				     self);

	_g_object_list_unref (singleton);
}


static void
gth_reset_orientation_task_exec (GthTask *task)
{
	GthResetOrientationTask *self;

	g_return_if_fail (GTH_IS_RESET_ORIENTATION_TASK (task));

	self = GTH_RESET_ORIENTATION_TASK (task);

	self->priv->n_images = g_list_length (self->priv->file_list);
	self->priv->n_image = 0;
	self->priv->current = self->priv->file_list;
	transform_current_file (self);
}


static void
gth_reset_orientation_task_class_init (GthResetOrientationTaskClass *klass)
{
	GObjectClass *object_class;
	GthTaskClass *task_class;

	object_class = G_OBJECT_CLASS (klass);
	object_class->finalize = gth_reset_orientation_task_finalize;

	task_class = GTH_TASK_CLASS (klass);
	task_class->exec = gth_reset_orientation_task_exec;
}


static void
gth_reset_orientation_task_init (GthResetOrientationTask *self)
{
	self->priv = gth_reset_orientation_task_get_instance_private (self);
	self->priv->file_data = NULL;
}


GthTask *
gth_reset_orientation_task_new (GthBrowser *browser,
				GList      *file_list)
{
	GthResetOrientationTask *self;

	self = GTH_RESET_ORIENTATION_TASK (g_object_new (GTH_TYPE_RESET_ORIENTATION_TASK, NULL));
	self->priv->browser = browser;
	self->priv->file_list = _g_object_list_ref (file_list);

	return (GthTask *) self;
}
