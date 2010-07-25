/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2009-2010 Free Software Foundation, Inc.
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
#include <extensions/exiv2_tools/exiv2-utils.h>
#include <extensions/image_rotation/rotation-utils.h>
#include "gth-import-task.h"
#include "preferences.h"
#include "utils.h"


#define IMPORTED_KEY "imported"


struct _GthImportTaskPrivate {
	GthBrowser          *browser;
	GList               *files;
	GFile               *destination;
	GHashTable          *destinations;
	GthSubfolderType     subfolder_type;
	GthSubfolderFormat   subfolder_format;
	gboolean             single_subfolder;
	char                *custom_format;
	char                *event_name;
	char               **tags;
	GTimeVal             import_start_time;
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
	gboolean             delete_not_supported;
	int                  n_imported;
};


static gpointer parent_class = NULL;


static void
gth_import_task_finalize (GObject *object)
{
	GthImportTask *self;

	self = GTH_IMPORT_TASK (object);

	if (ImportPhotos)
		gtk_window_present (GTK_WINDOW (self->priv->browser));

	g_object_unref (self->priv->destinations);
	_g_object_list_unref (self->priv->files);
	g_object_unref (self->priv->destination);
	_g_object_unref (self->priv->destination_file);
	g_free (self->priv->custom_format);
	g_free (self->priv->event_name);
	if (self->priv->tags != NULL)
		g_strfreev (self->priv->tags);
	g_hash_table_destroy (self->priv->catalogs);
	_g_object_unref (self->priv->imported_catalog);
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

	self->priv->n_imported++;

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
		g_get_current_time (&timeval);
		key = _g_time_val_strftime (&timeval, "%Y.%m.%d");
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
	gth_catalog_insert_file (catalog, self->priv->destination_file->file, -1);

	catalog = g_hash_table_lookup (self->priv->catalogs, IMPORTED_KEY);
	if (catalog != NULL)
		gth_catalog_insert_file (catalog, self->priv->destination_file->file, -1);

	import_next_file (self);

	g_free (key);
}


static void
write_metadata_ready_func (GError   *error,
			   gpointer  user_data)
{
	GthImportTask *self = user_data;

	if ((error != NULL) && g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED)) {
		gth_task_completed (GTH_TASK (self), error);
		return;
	}

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

	if ((error != NULL) && g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED)) {
		gth_task_completed (GTH_TASK (self), error);
		return;
	}

	if ((self->priv->tags == NULL) || (self->priv->tags[0] == NULL)) {
		catalog_imported_file (self);
		return;
	}

	tag_list = gth_string_list_new_from_strv (self->priv->tags);
	g_file_info_set_attribute_object (self->priv->destination_file->info, "comment::categories", G_OBJECT (tag_list));
	file_list = g_list_prepend (NULL, self->priv->destination_file);
	_g_write_metadata_async (file_list,
				 GTH_METADATA_WRITE_DEFAULT,
				 "comment::categories",
				 gth_task_get_cancellable (GTH_TASK (self)),
				 write_metadata_ready_func,
				 self);

	g_list_free (file_list);
	g_object_unref (tag_list);
}


static void
write_buffer_ready_cb (void     **buffer,
		       gsize      count,
		       GError    *error,
		       gpointer   user_data)
{
	GthImportTask *self = user_data;
	GthFileData   *file_data;
	gboolean       appling_tranformation = FALSE;

	if (error != NULL) {
		gth_task_completed (GTH_TASK (self), error);
		return;
	}

	file_data = self->priv->current->data;
	if (self->priv->delete_imported) {
		GError *local_error = NULL;

		if (! g_file_delete (file_data->file,
				     gth_task_get_cancellable (GTH_TASK (self)),
				     &local_error))
		{
			if (g_error_matches (local_error, G_IO_ERROR, G_IO_ERROR_NOT_SUPPORTED)) {
				self->priv->delete_imported = FALSE;
				self->priv->delete_not_supported = TRUE;
				local_error = NULL;
			}
			if (local_error != NULL) {
				gth_task_completed (GTH_TASK (self), local_error);
				return;
			}
		}
	}

	if (self->priv->adjust_orientation && gth_main_extension_is_active ("image_rotation")) {
		GthMetadata *metadata;

		metadata = (GthMetadata *) g_file_info_get_attribute_object (self->priv->destination_file->info, "Embedded::Image::Orientation");
		if (metadata != NULL) {
			const char *value;

			value = gth_metadata_get_raw (metadata);
			if (value != NULL) {
				GthTransform transform;

				transform = (GthTransform) strtol (value, (char **) NULL, 10);
				if (transform != 1) {
					apply_transformation_async (self->priv->destination_file,
								    transform,
								    JPEG_MCU_ACTION_ABORT,
								    gth_task_get_cancellable (GTH_TASK (self)),
								    transformation_ready_cb,
								    self);
					appling_tranformation = TRUE;
				}
			}
		}
	}

	if (! appling_tranformation)
		transformation_ready_cb (NULL, self);
}


static void
file_buffer_ready_cb (void     **buffer,
		      gsize      count,
		      GError    *error,
		      gpointer   user_data)
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
	if (gth_main_extension_is_active ("exiv2_tools"))
		exiv2_read_metadata_from_buffer (*buffer, count, file_data->info, NULL);

	destination = gth_import_utils_get_file_destination (file_data,
							     self->priv->destination,
							     self->priv->subfolder_type,
							     self->priv->subfolder_format,
							     self->priv->single_subfolder,
							     self->priv->custom_format,
							     self->priv->event_name,
							     self->priv->import_start_time);
	if (! g_file_make_directory_with_parents (destination, gth_task_get_cancellable (GTH_TASK (self)), &error)) {
		if (! g_error_matches (error, G_IO_ERROR, G_IO_ERROR_EXISTS)) {
			gth_task_completed (GTH_TASK (self), error);
			return;
		}
	}

	destination_file = _g_file_get_destination (file_data->file, NULL, destination);

	/* avoid to overwrite an already imported file */
	while (g_hash_table_lookup (self->priv->destinations, destination_file) != NULL) {
		GFile *tmp = destination_file;
		destination_file = _g_file_get_duplicated (tmp);
		g_object_unref (tmp);
	}
	g_hash_table_insert (self->priv->destinations, g_object_ref (destination_file), GINT_TO_POINTER (1));

	if (self->priv->overwrite_files || ! g_file_query_exists (destination_file, NULL)) {
		_g_object_unref (self->priv->destination_file);
		self->priv->destination_file = gth_file_data_new (destination_file, file_data->info);

		gth_task_progress (GTH_TASK (self),
				   _("Importing files"),
				   g_file_info_get_display_name (file_data->info),
				   FALSE,
				   (self->priv->copied_size + ((double) self->priv->current_file_size / 3.0 * 2.0)) / self->priv->tot_size);

		g_write_file_async (self->priv->destination_file->file,
				    *buffer,
				    count,
				    TRUE,
				    G_PRIORITY_DEFAULT,
				    gth_task_get_cancellable (GTH_TASK (self)),
				    write_buffer_ready_cb,
				    self);
		*buffer = NULL; /* g_write_file_async takes ownership of the buffer */
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

	if (self->priv->current == NULL) {
		save_catalogs (self);

		if (self->priv->n_imported == 0) {
			GtkWidget *d;

			d =  _gtk_message_dialog_new (GTK_WINDOW (self->priv->browser),
						      0,
						      GTK_STOCK_DIALOG_WARNING,
						      _("No file imported"),
						      _("The selected files are already present in the destination."),
						      GTK_STOCK_CLOSE, GTK_RESPONSE_CANCEL,
						      NULL);
			g_signal_connect (G_OBJECT (d), "response",
					  G_CALLBACK (gtk_widget_destroy),
					  NULL);
			gtk_widget_show (d);
		}
		else {
			if ((self->priv->subfolder_type != GTH_SUBFOLDER_TYPE_NONE) && (self->priv->imported_catalog != NULL))
				gth_browser_go_to (self->priv->browser, self->priv->imported_catalog, NULL);
			else
				gth_browser_go_to (self->priv->browser, self->priv->destination, NULL);

			if (self->priv->delete_not_supported && eel_gconf_get_boolean (PREF_IMPORT_WARN_DELETE_UNSUPPORTED, TRUE)) {
				GtkWidget *d;

				d =  _gtk_message_dialog_new (GTK_WINDOW (self->priv->browser),
							      0,
							      GTK_STOCK_DIALOG_WARNING,
							      _("Could not delete the files"),
							      _("Delete operation not supported."),
							      GTK_STOCK_CLOSE, GTK_RESPONSE_CANCEL,
							      NULL);
				g_signal_connect (G_OBJECT (d), "response",
						  G_CALLBACK (gtk_widget_destroy),
						  NULL);
				gtk_widget_show (d);

				eel_gconf_set_boolean (PREF_IMPORT_WARN_DELETE_UNSUPPORTED, FALSE);
			}
		}

		gth_task_completed (GTH_TASK (self), NULL);
		return;
	}

	file_data = self->priv->current->data;
	self->priv->current_file_size = g_file_info_get_size (file_data->info);

	gth_task_progress (GTH_TASK (self),
			   _("Importing files"),
			   g_file_info_get_display_name (file_data->info),
			   FALSE,
			   (self->priv->copied_size + ((double) self->priv->current_file_size / 3.0)) / self->priv->tot_size);

	g_load_file_async (file_data->file,
			   G_PRIORITY_DEFAULT,
			   gth_task_get_cancellable (GTH_TASK (self)),
			   file_buffer_ready_cb,
			   self);
}


static void
gth_import_task_exec (GthTask *base)
{
	GthImportTask *self = (GthImportTask *) base;
	GTimeVal       timeval;
	GList         *scan;

	self->priv->n_imported = 0;
	self->priv->tot_size = 0;
	for (scan = self->priv->files; scan; scan = scan->next) {
		GthFileData *file_data = scan->data;
		self->priv->tot_size += g_file_info_get_size (file_data->info);
	}
	g_get_current_time (&timeval);
	self->priv->import_start_time = timeval;

	/* create the imported files catalog */

	if (gth_main_extension_is_active ("catalogs")) {
		GthDateTime *date_time;
		char        *display_name;
		GthCatalog  *catalog = NULL;

		date_time = gth_datetime_new ();
		gth_datetime_from_timeval (date_time, &timeval);

		if ((self->priv->event_name != NULL) && ! _g_utf8_all_spaces (self->priv->event_name)) {
			display_name = g_strdup (self->priv->event_name);
			self->priv->imported_catalog = _g_file_new_for_display_name ("catalog://", display_name, ".catalog");
			/* append files to the catalog if an event name was given */
			catalog = gth_catalog_load_from_file (self->priv->imported_catalog);
		}
		else {
			display_name = g_strdup (_("Last imported"));
			self->priv->imported_catalog = _g_file_new_for_display_name ("catalog://", display_name, ".catalog");
			/* overwrite the catalog content if the generic "last imported" catalog is used. */
			catalog = NULL;
		}

		if (catalog == NULL)
			catalog = gth_catalog_new ();
		gth_catalog_set_file (catalog, self->priv->imported_catalog);
		gth_catalog_set_date (catalog, date_time);
		gth_catalog_set_name (catalog, display_name);

		g_hash_table_insert (self->priv->catalogs, g_strdup (IMPORTED_KEY), catalog);

		g_free (display_name);
		gth_datetime_free (date_time);
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
	self->priv->delete_not_supported = FALSE;
	self->priv->destinations = g_hash_table_new_full (g_file_hash,
							  (GEqualFunc) g_file_equal,
							  g_object_unref,
							  NULL);
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
		     const char         *custom_format,
		     const char         *event_name,
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
	if (custom_format != NULL)
		self->priv->custom_format = g_strdup (custom_format);
	else
		self->priv->custom_format = NULL;
	self->priv->event_name = g_strdup (event_name);
	self->priv->tags = g_strdupv (tags);
	self->priv->delete_imported = delete_imported;
	self->priv->overwrite_files = overwrite_files;
	self->priv->adjust_orientation = adjust_orientation;

	return (GthTask *) self;
}
