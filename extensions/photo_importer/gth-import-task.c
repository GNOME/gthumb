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
	GthBrowser       *browser;
	GList            *files;
	GFile            *destination;
	GthSubfolderType  subfolder_type;
	gboolean          single_subfolder;
	char             *tags;
	gboolean          delete_imported;
	GCancellable     *cancellable;

	gsize             tot_size;
	gsize             copied_size;
	gsize             current_file_size;
	GList            *current;
};


static gpointer parent_class = NULL;


static void
gth_import_task_finalize (GObject *object)
{
	GthImportTask *self;

	self = GTH_IMPORT_TASK (object);

	_g_object_list_unref (self->priv->files);
	g_object_unref (self->priv->destination);
	g_free (self->priv->tags);
	g_object_unref (self->priv->cancellable);
	g_object_unref (self->priv->browser);

	G_OBJECT_CLASS (parent_class)->finalize (object);
}


static void import_current_file (GthImportTask *self);


static void
copy_ready_cb (GError   *error,
	       gpointer  user_data)
{
	GthImportTask *self = user_data;
	GthFileData   *file_data;

	if (error != NULL) {
		gth_task_completed (GTH_TASK (self), error);
		return;
	}

	/*
	 * adjust orientation
	 * add tags
	 */

	file_data = self->priv->current->data;
	self->priv->copied_size += self->priv->current_file_size;
	self->priv->current = self->priv->current->next;
	import_current_file (self);
}


static void
copy_progress_cb (GObject    *object,
		  const char *description,
		  const char *details,
		  gboolean    pulse,
		  double      fraction,
		  gpointer    user_data)
{
	GthImportTask *self = user_data;

	fraction = (((double) self->priv->current_file_size * fraction) + self->priv->copied_size) / self->priv->tot_size;
	gth_task_progress (GTH_TASK (self), description, details, pulse, fraction);
}


static void
file_info_ready_cb (GList    *files,
		    GError   *error,
		    gpointer  user_data)
{
	GthImportTask *self = user_data;
	GthFileData   *file_data;
	GFile         *destination;
	GFile         *destination_file;

	if (error != NULL) {
		gth_task_completed (GTH_TASK (self), error);
		return;
	}

	file_data = self->priv->current->data;
	self->priv->current_file_size = g_file_info_get_size (file_data->info);

	destination = gth_import_task_get_file_destination (file_data,
							    self->priv->destination,
							    self->priv->subfolder_type,
							    self->priv->single_subfolder);
	if (! g_file_make_directory_with_parents (destination, self->priv->cancellable, &error)) {
		if (! g_error_matches (error, G_IO_ERROR, G_IO_ERROR_EXISTS)) {
			gth_task_completed (GTH_TASK (self), error);
			return;
		}
	}

	destination_file = _g_file_get_destination (file_data->file, NULL, destination);
	_g_copy_file_async (file_data,
			    destination_file,
			    FALSE /*self->priv->delete_imported*/,
			    G_FILE_COPY_ALL_METADATA | G_FILE_COPY_TARGET_DEFAULT_PERMS,
			    G_PRIORITY_DEFAULT,
			    self->priv->cancellable,
			    copy_progress_cb,
			    self,
			    copy_ready_cb,
			    self);

	g_object_unref (destination_file);
	g_object_unref (destination);
}


static void
import_current_file (GthImportTask *self)
{
	GthFileData *file_data;
	GList       *list;

	if (self->priv->current == NULL) {
		gth_browser_go_to (self->priv->browser, self->priv->destination);
		gth_task_completed (GTH_TASK (self), NULL);
		return;
	}

	file_data = self->priv->current->data;
	list = g_list_prepend (NULL, file_data);
	_g_query_metadata_async (list,
				 "Exif::Image::DateTime",
				 self->priv->cancellable,
				 file_info_ready_cb,
				 self);

	g_list_free (list);
}


static void
gth_import_task_exec (GthTask *base)
{
	GthImportTask *self = (GthImportTask *) base;

	self->priv->current = self->priv->files;
	import_current_file (self);
}


static void
gth_import_task_cancel (GthTask *base)
{
	g_cancellable_cancel (GTH_IMPORT_TASK (base)->priv->cancellable);
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
gth_import_task_new (GthBrowser       *browser,
		     GList            *files,
		     GFile            *destination,
		     GthSubfolderType  subfolder_type,
		     gboolean          single_subfolder,
		     const char       *tags,
		     gboolean          delete_imported)
{
	GthImportTask *self;

	self = GTH_IMPORT_TASK (g_object_new (GTH_TYPE_IMPORT_TASK, NULL));
	self->priv->browser = g_object_ref (browser);
	self->priv->files = _g_object_list_ref (files);
	self->priv->destination = g_file_dup (destination);
	self->priv->subfolder_type = subfolder_type;
	self->priv->single_subfolder = single_subfolder;
	self->priv->tags = g_strdup (tags);
	self->priv->delete_imported = delete_imported;

	return (GthTask *) self;
}


GFile *
gth_import_task_get_file_destination (GthFileData      *file_data,
				      GFile            *destination,
				      GthSubfolderType  subfolder_type,
				      gboolean          single_subfolder)
{
	GFile     *file_destination;
	GTimeVal   timeval;
	GDate     *date;
	char     **parts = NULL;
	char      *child;

	if (subfolder_type == GTH_SUBFOLDER_TYPE_CURRENT_DATE) {
		g_get_current_time (&timeval);
	}
	else if (subfolder_type == GTH_SUBFOLDER_TYPE_FILE_DATE) {
		GthMetadata *metadata;

		metadata = (GthMetadata *) g_file_info_get_attribute_object (file_data->info, "Embedded::Image::DateTime");
		if (metadata != NULL)
			_g_time_val_from_exif_date (gth_metadata_get_raw (metadata), &timeval);
		else
			subfolder_type = GTH_SUBFOLDER_TYPE_NONE;
	}

	date = g_date_new ();

	switch (subfolder_type) {
	case GTH_SUBFOLDER_TYPE_FILE_DATE:
	case GTH_SUBFOLDER_TYPE_CURRENT_DATE:
		g_date_set_time_val (date, &timeval);

		parts = g_new0 (char *, 4);
		parts[0] = g_strdup_printf ("%04d", g_date_get_year (date));
		parts[1] = g_strdup_printf ("%02d", g_date_get_month (date));
		parts[2] = g_strdup_printf ("%02d", g_date_get_day (date));
		break;

	case GTH_SUBFOLDER_TYPE_NONE:
		break;
	}

	if (parts == NULL)
		child = NULL;
	else if (single_subfolder)
		child = g_strjoinv ("-", parts);
	else
		child = g_strjoinv ("/", parts);

	file_destination = _g_file_append_path (destination, child);

	g_free (child);
	g_strfreev (parts);
	g_date_free (date);

	return file_destination;
}
