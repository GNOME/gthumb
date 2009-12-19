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
#include <extensions/catalogs/gth-catalog.h>
#include <extensions/image_rotation/rotation-utils.h>
#include "gth-import-task.h"


#define IMPORTED_KEY "imported"


struct _GthImportTaskPrivate {
	GthBrowser          *browser;
	GList               *files;
	GFile               *destination;
	GthSubfolderType     subfolder_type;
	GthSubfolderFormat   subfolder_format;
	gboolean             single_subfolder;
	char               **tags;
	gboolean             delete_imported;
	gboolean             overwrite_files;
	gboolean             adjust_orientation;

	GHashTable          *catalogs;
	gsize                tot_size;
	gsize                copied_size;
	gsize                current_file_size;
	GList               *current;
	GthFileData         *destination_file;
	GFile               *imported_catalog;
};


static gpointer parent_class = NULL;


static void
gth_import_task_finalize (GObject *object)
{
	GthImportTask *self;

	self = GTH_IMPORT_TASK (object);

	if (ImportPhotos)
		gtk_window_present (GTK_WINDOW (self->priv->browser));

	_g_object_list_unref (self->priv->files);
	g_object_unref (self->priv->destination);
	_g_object_unref (self->priv->destination_file);
	g_strfreev (self->priv->tags);
	g_hash_table_destroy (self->priv->catalogs);
	g_object_unref (self->priv->imported_catalog);
	g_object_unref (self->priv->browser);

	G_OBJECT_CLASS (parent_class)->finalize (object);
}


static void import_current_file (GthImportTask *self);


static void
import_next_file (GthImportTask *self)
{
	self->priv->copied_size += self->priv->current_file_size;
	self->priv->current = self->priv->current->next;
	import_current_file (self);
}


static void
save_catalog (gpointer key,
	      gpointer value,
	      gpointer user_data)
{
	GthCatalog *catalog = value;

	gth_catalog_save (catalog);
}


static void
save_catalogs (GthImportTask *self)
{
	g_hash_table_foreach (self->priv->catalogs, save_catalog, self);
}


static void
catalog_imported_file (GthImportTask *self)
{
	char       *key;
	GObject    *metadata;
	GTimeVal    timeval;
	GthCatalog *catalog;

	if (! gth_main_extension_is_active ("catalogs")) {
		import_next_file (self);
		return;
	}

	key = NULL;
	metadata = g_file_info_get_attribute_object (self->priv->destination_file->info, "Embedded::Photo::DateTimeOriginal");
	if (metadata != NULL) {
		if (_g_time_val_from_exif_date (gth_metadata_get_raw (GTH_METADATA (metadata)), &timeval))
			key = _g_time_val_strftime (&timeval, "%Y.%m.%d");
	}

	if (key == NULL) {
		g_free (key);
		import_next_file (self);
		return;
	}

	catalog = g_hash_table_lookup (self->priv->catalogs, key);
	if (catalog == NULL) {
		GthDateTime *date_time;
		GFile       *catalog_file;

		date_time = gth_datetime_new ();
		gth_datetime_from_timeval (date_time, &timeval);

		catalog_file = gth_catalog_get_file_for_date (date_time);
		catalog = gth_catalog_load_from_file (catalog_file);
		if (catalog == NULL)
			catalog = gth_catalog_new ();
		gth_catalog_set_for_date (catalog, date_time);

		g_hash_table_insert (self->priv->catalogs, g_strdup (key), catalog);

		g_object_unref (catalog_file);
		gth_datetime_free (date_time);
	}
	gth_catalog_append_file (catalog, self->priv->destination_file->file);

	catalog = g_hash_table_lookup (self->priv->catalogs, IMPORTED_KEY);
	if (catalog == NULL) {
		GthDateTime *date_time;
		char        *name;
		char        *display_name;

		date_time = gth_datetime_new ();
		gth_datetime_from_timeval (date_time, &timeval);

		name = gth_datetime_strftime (date_time, "%Y.%m.%d-%H.%M.%S");
		self->priv->imported_catalog = _g_file_new_for_display_name ("catalog://", name, ".catalog");
		catalog = gth_catalog_load_from_file (self->priv->imported_catalog);
		if (catalog == NULL)
			catalog = gth_catalog_new ();

		gth_catalog_set_file (catalog, self->priv->imported_catalog);
		gth_catalog_set_date (catalog, date_time);
		display_name = gth_datetime_strftime (date_time, _("Imported %x %X"));
		gth_catalog_set_name (catalog, display_name);

		g_hash_table_insert (self->priv->catalogs, g_strdup (IMPORTED_KEY), catalog);

		g_free (display_name);
		g_free (name);
		gth_datetime_free (date_time);
	}
	gth_catalog_append_file (catalog, self->priv->destination_file->file);

	import_next_file (self);

	g_free (key);
}


static void
write_metadata_ready_func (GError   *error,
			   gpointer  user_data)
{
	GthImportTask *self = user_data;

	if (error != NULL)
		g_clear_error (&error);

	catalog_imported_file (self);
}


static void
transformation_ready_cb (GError   *error,
			 gpointer  user_data)
{
	GthImportTask *self = user_data;
	GthStringList *tag_list;
	GList         *file_list;

	if (self->priv->tags[0] == NULL) {
		catalog_imported_file (self);
		return;
	}

	tag_list = gth_string_list_new_from_strv (self->priv->tags);
	g_file_info_set_attribute_object (self->priv->destination_file->info, "comment::categories", G_OBJECT (tag_list));
	file_list = g_list_prepend (NULL, self->priv->destination_file);
	_g_write_metadata_async (file_list,
				 "comment::categories",
				 gth_task_get_cancellable (GTH_TASK (self)),
				 write_metadata_ready_func,
				 self);

	g_list_free (file_list);
	g_object_unref (tag_list);
}


static void
copy_ready_cb (GError   *error,
	       gpointer  user_data)
{
	GthImportTask *self = user_data;
	gboolean       appling_tranformation = FALSE;

	if (g_error_matches (error, G_IO_ERROR, G_IO_ERROR_NOT_SUPPORTED)) {
		self->priv->delete_imported = FALSE;
		error = NULL;
	}

	if (error != NULL) {
		gth_task_completed (GTH_TASK (self), error);
		return;
	}

	if (self->priv->adjust_orientation) {
		GthMetadata *metadata;

		metadata = (GthMetadata *) g_file_info_get_attribute_object (self->priv->destination_file->info, "Embedded::Image::Orientation");
		if (metadata != NULL) {
			const char *value;

			value = gth_metadata_get_raw (metadata);
			if (value != NULL) {
				int transform;

				sscanf (value, "%d", &transform);
				if (transform != 1) {
					apply_transformation_async (self->priv->destination_file,
								    (GthTransform) transform,
								    JPEG_MCU_ACTION_ABORT,
								    gth_task_get_cancellable (GTH_TASK (self)),
								    transformation_ready_cb,
								    self);
					appling_tranformation = TRUE;
				}
			}

			g_object_unref (metadata);
		}
	}

	if (! appling_tranformation)
		transformation_ready_cb (NULL, self);
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
	char          *local_details = NULL;

	if (! pulse) {
		char *s1;
		char *s2;

		s1 = g_format_size_for_display (((double) self->priv->current_file_size * fraction) + self->priv->copied_size);
		s2 = g_format_size_for_display (self->priv->tot_size);
		/* translators: this a copy progress message, for example: 1.2MB of 12MB */
		local_details = g_strdup_printf (_("%s of %s"), s1, s2);
		details = local_details;

		fraction = (((double) self->priv->current_file_size * fraction) + self->priv->copied_size) / self->priv->tot_size;
	}

	gth_task_progress (GTH_TASK (self), description, details, pulse, fraction);

	g_free (local_details);
}


static void
copy_dialog_cb (gboolean  opened,
		gpointer  user_data)
{
	GthImportTask *self = user_data;

	gth_task_dialog (GTH_TASK (self), opened);
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
							    self->priv->subfolder_format,
							    self->priv->single_subfolder);
	if (! g_file_make_directory_with_parents (destination, gth_task_get_cancellable (GTH_TASK (self)), &error)) {
		if (! g_error_matches (error, G_IO_ERROR, G_IO_ERROR_EXISTS)) {
			gth_task_completed (GTH_TASK (self), error);
			return;
		}
	}

	destination_file = _g_file_get_destination (file_data->file, NULL, destination);
	if (self->priv->overwrite_files || ! g_file_query_exists (destination_file, NULL)) {
		GFileCopyFlags copy_flags;

		_g_object_unref (self->priv->destination_file);
		self->priv->destination_file = gth_file_data_new (destination_file, file_data->info);

		copy_flags = G_FILE_COPY_ALL_METADATA | G_FILE_COPY_TARGET_DEFAULT_PERMS;
		if (self->priv->overwrite_files)
			copy_flags |= G_FILE_COPY_OVERWRITE;

		_g_copy_file_async (file_data,
				    destination_file,
				    self->priv->delete_imported,
				    copy_flags,
				    G_PRIORITY_DEFAULT,
				    gth_task_get_cancellable (GTH_TASK (self)),
				    copy_progress_cb,
				    self,
				    copy_dialog_cb,
				    self,
				    copy_ready_cb,
				    self);
	}
	else
		call_when_idle ((DataFunc) import_next_file, self);

	g_object_unref (destination_file);
	g_object_unref (destination);
}


static void
import_current_file (GthImportTask *self)
{
	GthFileData *file_data;
	GList       *list;

	if (self->priv->current == NULL) {
		save_catalogs (self);
		if (self->priv->imported_catalog != NULL)
			gth_browser_go_to (self->priv->browser, self->priv->imported_catalog, NULL);
		else
			gth_browser_go_to (self->priv->browser, self->priv->destination, NULL);
		gth_task_completed (GTH_TASK (self), NULL);
		return;
	}

	file_data = self->priv->current->data;
	list = g_list_prepend (NULL, file_data);
	_g_query_metadata_async (list,
				 "Embedded::Photo::DateTimeOriginal,Embedded::Image::Orientation",
				 gth_task_get_cancellable (GTH_TASK (self)),
				 file_info_ready_cb,
				 self);

	g_list_free (list);
}


static void
gth_import_task_exec (GthTask *base)
{
	GthImportTask *self = (GthImportTask *) base;
	GList         *scan;

	self->priv->tot_size = 0;
	for (scan = self->priv->files; scan; scan = scan->next) {
		GthFileData *file_data = scan->data;
		self->priv->tot_size += g_file_info_get_size (file_data->info);
	}

	self->priv->current = self->priv->files;
	import_current_file (self);
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
}


static void
gth_import_task_init (GthImportTask *self)
{
	self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, GTH_TYPE_IMPORT_TASK, GthImportTaskPrivate);
	self->priv->catalogs = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_object_unref);
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
gth_import_task_new (GthBrowser         *browser,
		     GList              *files,
		     GFile              *destination,
		     GthSubfolderType    subfolder_type,
		     GthSubfolderFormat  subfolder_format,
		     gboolean            single_subfolder,
		     char              **tags,
		     gboolean            delete_imported,
		     gboolean            overwrite_files,
		     gboolean            adjust_orientation)
{
	GthImportTask *self;

	self = GTH_IMPORT_TASK (g_object_new (GTH_TYPE_IMPORT_TASK, NULL));
	self->priv->browser = g_object_ref (browser);
	self->priv->files = _g_object_list_ref (files);
	self->priv->destination = g_file_dup (destination);
	self->priv->subfolder_type = subfolder_type;
	self->priv->subfolder_format = subfolder_format;
	self->priv->single_subfolder = single_subfolder;
	self->priv->tags = g_strdupv (tags);
	self->priv->delete_imported = delete_imported;
	self->priv->overwrite_files = overwrite_files;
	self->priv->adjust_orientation = adjust_orientation;

	return (GthTask *) self;
}


GFile *
gth_import_task_get_file_destination (GthFileData        *file_data,
				      GFile              *destination,
				      GthSubfolderType    subfolder_type,
				      GthSubfolderFormat  subfolder_format,
				      gboolean            single_subfolder)
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

		metadata = (GthMetadata *) g_file_info_get_attribute_object (file_data->info, "Embedded::Photo::DateTimeOriginal");
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
		if (subfolder_format != GTH_SUBFOLDER_FORMAT_YYYY) {
			parts[1] = g_strdup_printf ("%02d", g_date_get_month (date));
			if (subfolder_format != GTH_SUBFOLDER_FORMAT_YYYYMM)
				parts[2] = g_strdup_printf ("%02d", g_date_get_day (date));
		}
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
