/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2008 Free Software Foundation, Inc.
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
#include <string.h>
#include <glib/gi18n.h>
#include <glib.h>
#include <gthumb.h>
#include "gth-catalog.h"
#include "gth-file-source-catalogs.h"


struct _GthFileSourceCatalogsPrivate
{
	GList        *files;
	GthCatalog   *catalog;
	ListReady     ready_func;
	gpointer      ready_data;
};


static gpointer parent_class = NULL;


static GList *
get_entry_points (GthFileSource *file_source)
{
	GList       *list = NULL;
	GFile       *file;
	GFileInfo   *info;

	file = g_file_new_for_uri ("catalog:///");
	info = gth_file_source_get_file_info (file_source, file, GFILE_BASIC_ATTRIBUTES);
	list = g_list_append (list, gth_file_data_new (file, info));

	g_object_unref (info);
	g_object_unref (file);

	return list;
}


static GFile *
gth_file_source_catalogs_to_gio_file (GthFileSource *file_source,
				      GFile         *file)
{
	return gth_catalog_file_to_gio_file (file);
}


static void
update_file_info (GthFileSource *file_source,
		  GFile         *catalog_file,
		  GFileInfo     *info)
{
	char *uri;

	uri = g_file_get_uri (catalog_file);

	if (g_str_has_suffix (uri, ".gqv") || g_str_has_suffix (uri, ".catalog")) {
		g_file_info_set_file_type (info, G_FILE_TYPE_DIRECTORY);
		g_file_info_set_content_type (info, "gthumb/catalog");
		g_file_info_set_icon (info, g_themed_icon_new ("file-catalog"));
		g_file_info_set_sort_order (info, 1);
		g_file_info_set_attribute_boolean (info, "gthumb::no-child", TRUE);
		gth_catalog_update_standard_attributes (catalog_file, info);
	}
	else if (g_str_has_suffix (uri, ".search")) {
		g_file_info_set_file_type (info, G_FILE_TYPE_DIRECTORY);
		g_file_info_set_content_type (info, "gthumb/search");
		g_file_info_set_icon (info, g_themed_icon_new ("file-search"));
		g_file_info_set_sort_order (info, 1);
		g_file_info_set_attribute_boolean (info, "gthumb::no-child", TRUE);
		gth_catalog_update_standard_attributes (catalog_file, info);
	}
	else {
		g_file_info_set_file_type (info, G_FILE_TYPE_DIRECTORY);
		g_file_info_set_content_type (info, "gthumb/library");
		g_file_info_set_icon (info, g_themed_icon_new ("file-library"));
		g_file_info_set_sort_order (info, 0);
		g_file_info_set_attribute_boolean (info, "gthumb::no-child", FALSE);
		gth_catalog_update_standard_attributes (catalog_file, info);
	}

	g_free (uri);
}


static GFileInfo *
gth_file_source_catalogs_get_file_info (GthFileSource *file_source,
					GFile         *file,
					const char    *attributes)
{
	GFile     *gio_file;
	GFileInfo *file_info;

	gio_file = gth_catalog_file_to_gio_file (file);
	file_info = g_file_query_info (gio_file,
				       attributes,
				       G_FILE_QUERY_INFO_NONE,
				       NULL,
				       NULL);
	if (file_info == NULL)
		file_info = g_file_info_new ();
	update_file_info (file_source, file, file_info);

	g_object_unref (gio_file);

	return file_info;
}


static GthFileData *
gth_file_source_catalogs_get_file_data (GthFileSource *file_source,
					GFile         *file,
					GFileInfo     *info)
{
	GthFileData *file_data = NULL;
	char        *uri;
	GFile       *catalog_file;

	uri = g_file_get_uri (file);

	switch (g_file_info_get_file_type (info)) {
	case G_FILE_TYPE_REGULAR:
		if (! g_str_has_suffix (uri, ".gqv")
		    && ! g_str_has_suffix (uri, ".catalog")
		    && ! g_str_has_suffix (uri, ".search"))
		{
			file_data = gth_file_data_new (file, info);
			break;
		}

		catalog_file = gth_catalog_file_from_gio_file (file, NULL);
		update_file_info (file_source, catalog_file, info);
		file_data = gth_file_data_new (catalog_file, info);

		g_object_unref (catalog_file);
		break;

	case G_FILE_TYPE_DIRECTORY:
		catalog_file = gth_catalog_file_from_gio_file (file, NULL);
		update_file_info (file_source, catalog_file, info);
		file_data = gth_file_data_new (catalog_file, info);

		g_object_unref (catalog_file);
		break;

	default:
		break;
	}

	g_free (uri);

	return file_data;
}


/* -- gth_file_source_catalogs_write_metadata -- */


typedef struct {
	GthFileSourceCatalogs *catalogs;
	GthFileData           *file_data;
	char                  *attributes;
	ReadyCallback          ready_callback;
	gpointer               user_data;
	GthCatalog            *catalog;
} MetadataOpData;


static void
metadata_op_free (MetadataOpData *metadata_op)
{
	gth_file_source_set_active (GTH_FILE_SOURCE (metadata_op->catalogs), FALSE);
	g_object_unref (metadata_op->file_data);
	g_free (metadata_op->attributes);
	g_object_unref (metadata_op->catalog);
	g_object_unref (metadata_op->catalogs);
	g_free (metadata_op);
}


static void
write_metadata_write_buffer_ready_cb (void     **buffer,
				      gsize      count,
				      GError    *error,
				      gpointer   user_data)
{
	MetadataOpData        *metadata_op = user_data;
	GthFileSourceCatalogs *catalogs = metadata_op->catalogs;

	metadata_op->ready_callback (G_OBJECT (catalogs), error, metadata_op->user_data);
	metadata_op_free (metadata_op);
}


static void
write_metadata_load_buffer_ready_cb (void     **buffer,
				     gsize      count,
				     GError    *error,
				     gpointer   user_data)
{
	MetadataOpData        *metadata_op = user_data;
	GthFileSourceCatalogs *catalogs = metadata_op->catalogs;
	GFile                 *gio_file;
	void                  *catalog_buffer;
	gsize                  catalog_size;

	if (error != NULL) {
		metadata_op->ready_callback (G_OBJECT (catalogs), error, metadata_op->user_data);
		metadata_op_free (metadata_op);
		return;
	}

	gth_catalog_load_from_data (metadata_op->catalog, *buffer, count, &error);

	if (error != NULL) {
		metadata_op->ready_callback (G_OBJECT (catalogs), error, metadata_op->user_data);
		metadata_op_free (metadata_op);
		return;
	}

	if (_g_file_attributes_matches_any (metadata_op->attributes, "sort::*"))
		gth_catalog_set_order (metadata_op->catalog,
				       g_file_info_get_attribute_string (metadata_op->file_data->info, "sort::type"),
				       g_file_info_get_attribute_boolean (metadata_op->file_data->info, "sort::inverse"));

	gth_hook_invoke ("gth-catalog-read-metadata", metadata_op->catalog, metadata_op->file_data);

	catalog_buffer = gth_catalog_to_data (metadata_op->catalog, &catalog_size);
	gio_file = gth_catalog_file_to_gio_file (metadata_op->file_data->file);
	g_write_file_async (gio_file,
			    catalog_buffer,
			    catalog_size,
			    TRUE,
			    G_PRIORITY_DEFAULT,
			    gth_file_source_get_cancellable (GTH_FILE_SOURCE (metadata_op->catalogs)),
			    write_metadata_write_buffer_ready_cb,
			    metadata_op);

	g_object_unref (gio_file);
}


static void
gth_file_source_catalogs_write_metadata (GthFileSource *file_source,
					 GthFileData   *file_data,
					 const char    *attributes,
					 ReadyCallback  callback,
					 gpointer       user_data)
{
	GthFileSourceCatalogs *catalogs = (GthFileSourceCatalogs *) file_source;
	char                  *uri;
	MetadataOpData        *metadata_op;
	GFile                 *gio_file;

	uri = g_file_get_uri (file_data->file);
	if (! g_str_has_suffix (uri, ".gqv")
	    && ! g_str_has_suffix (uri, ".catalog")
	    && ! g_str_has_suffix (uri, ".search"))
	{
		g_free (uri);
		object_ready_with_error (file_source, callback, user_data, NULL);
		return;
	}

	metadata_op = g_new0 (MetadataOpData, 1);
	metadata_op->catalogs = g_object_ref (catalogs);
	metadata_op->file_data = g_object_ref (file_data);
	metadata_op->attributes = g_strdup (attributes);
	metadata_op->ready_callback = callback;
	metadata_op->user_data = user_data;

	gth_file_source_set_active (GTH_FILE_SOURCE (catalogs), TRUE);
	g_cancellable_reset (gth_file_source_get_cancellable (file_source));

	metadata_op->catalog = gth_catalog_new ();
	gio_file = gth_file_source_to_gio_file (file_source, file_data->file);
	gth_catalog_set_file (metadata_op->catalog, gio_file);
	g_load_file_async (gio_file,
			   G_PRIORITY_DEFAULT,
			   gth_file_source_get_cancellable (file_source),
			   write_metadata_load_buffer_ready_cb,
			   metadata_op);

	g_object_unref (gio_file);
	g_free (uri);
}


/* -- gth_file_source_catalogs_read_metadata -- */


typedef struct {
	GthFileSource *file_source;
	GthFileData   *file_data;
	char          *attributes;
	ReadyCallback  callback;
	gpointer       data;
} ReadMetadataOpData;


static void
read_metadata_free (ReadMetadataOpData *read_metadata)
{
	g_object_unref (read_metadata->file_source);
	g_object_unref (read_metadata->file_data);
	g_free (read_metadata->attributes);
	g_free (read_metadata);
}


static void
read_metadata_catalog_ready_cb (GObject  *object,
				GError   *error,
				gpointer  user_data)
{
	ReadMetadataOpData *read_metadata = user_data;

	/* ignore errors */
	if (error != NULL)
		g_clear_error (&error);

	if (object != NULL) {
		gth_catalog_update_metadata (GTH_CATALOG (object), read_metadata->file_data);
		g_object_unref (object);
	}

	read_metadata->callback (G_OBJECT (read_metadata->file_source), error, read_metadata->data);
	read_metadata_free (read_metadata);
}


static void
read_metadata_info_ready_cb (GList    *files,
			     GError   *error,
			     gpointer  user_data)
{
	ReadMetadataOpData *read_metadata = user_data;
	GthFileData        *result;
	GFile              *gio_file;

	if (error != NULL) {
		read_metadata->callback (G_OBJECT (read_metadata->file_source), error, read_metadata->data);
		read_metadata_free (read_metadata);
		return;
	}

	result = files->data;
	g_file_info_copy_into (result->info, read_metadata->file_data->info);
	update_file_info (read_metadata->file_source, read_metadata->file_data->file, read_metadata->file_data->info);

	gio_file = gth_catalog_file_to_gio_file (read_metadata->file_data->file);
	gth_catalog_load_from_file_async (gio_file,
					  gth_file_source_get_cancellable (read_metadata->file_source),
					  read_metadata_catalog_ready_cb,
					  read_metadata);

	g_object_unref (gio_file);
}


static void
gth_file_source_catalogs_read_metadata (GthFileSource *file_source,
					GthFileData   *file_data,
					const char    *attributes,
					ReadyCallback  callback,
					gpointer       data)
{
	ReadMetadataOpData *read_metadata;
	GFile              *gio_file;
	GList              *files;

	read_metadata = g_new0 (ReadMetadataOpData, 1);
	read_metadata->file_source = g_object_ref (file_source);
	read_metadata->file_data = g_object_ref (file_data);
	read_metadata->attributes = g_strdup (attributes);
	read_metadata->callback = callback;
	read_metadata->data = data;

	gio_file = gth_catalog_file_to_gio_file (file_data->file);
	files = g_list_prepend (NULL, gio_file);
	_g_query_all_metadata_async (files,
				     GTH_LIST_DEFAULT,
				     attributes,
				     gth_file_source_get_cancellable (file_source),
				     read_metadata_info_ready_cb,
				     read_metadata);

	_g_object_list_unref (files);
}


/* -- gth_file_source_catalogs_for_each_child -- */


typedef struct {
	GthFileSource         *file_source;
	gboolean               recursive;
	char                  *attributes;
	StartDirCallback       start_dir_func;
	ForEachChildCallback   for_each_file_func;
	ReadyCallback          ready_func;
	gpointer               user_data;
	GthCatalog            *catalog;
	GList                 *to_visit;
} ForEachChildData;


static void
for_each_child_data_free (ForEachChildData *data)
{
	_g_object_list_unref (data->to_visit);
	g_object_ref (data->catalog);
	g_free (data->attributes);
	g_object_ref (data->file_source);
}


static void
for_each_child_data_done (ForEachChildData *data,
			  GError           *error)
{
	gth_file_source_set_active (data->file_source, FALSE);
	data->ready_func (G_OBJECT (data->file_source), error, data->user_data);

	for_each_child_data_free (data);
}


static void for_each_child__visit_file (ForEachChildData *data,
				        GthFileData      *library);


static void
for_each_child__continue (ForEachChildData *data)
{
	GthFileData *file;
	GList       *tmp;

	if (! data->recursive || (data->to_visit == NULL)) {
		for_each_child_data_done (data, NULL);
		return;
	}

	file = data->to_visit->data;
	tmp = data->to_visit;
	data->to_visit = g_list_remove_link (data->to_visit, tmp);
	g_list_free (tmp);

	for_each_child__visit_file (data, file);

	g_object_unref (file);
}


static void
for_each_child__done_func (GError   *error,
			   gpointer  user_data)
{
	for_each_child__continue ((ForEachChildData *) user_data);
}


static void
for_each_child__for_each_file_func (GFile     *file,
				    GFileInfo *info,
				    gpointer   user_data)
{
	ForEachChildData *data = user_data;
	GthFileData      *file_data;

	file_data = gth_file_source_get_file_data (data->file_source, file, info);
	if (file_data == NULL)
		return;

	data->for_each_file_func (file_data->file, file_data->info, data->user_data);

	if (data->recursive && (g_file_info_get_file_type (file_data->info) == G_FILE_TYPE_DIRECTORY))
		data->to_visit = g_list_append (data->to_visit, g_object_ref (file_data));

	g_object_unref (file_data);
}


static DirOp
for_each_child__start_dir_func (GFile       *directory,
				GFileInfo   *info,
				GError     **error,
				gpointer     user_data)
{
	if (g_file_info_get_is_hidden (info))
		return DIR_OP_SKIP;
	else
		return DIR_OP_CONTINUE;
}


static void
for_each_child__catalog_list_ready_cb (GthCatalog *catalog,
				       GList      *files,
				       GError     *error,
				       gpointer    user_data)
{
	ForEachChildData *data = user_data;
	GList            *scan;

	for (scan = files; scan; scan = scan->next) {
		GthFileData *file_data = scan->data;

		if (g_file_info_get_is_hidden (file_data->info))
			continue;

		data->for_each_file_func (file_data->file,
					  file_data->info,
					  data->user_data);
	}

	for_each_child__continue (data);
}


static void
for_each_child__visit_file (ForEachChildData *data,
			    GthFileData      *file_data)
{
	GFile *gio_file;
	char  *uri;

	if (data->start_dir_func != NULL) {
		GError *error = NULL;

		switch (data->start_dir_func (file_data->file, file_data->info, &error, data->user_data)) {
		case DIR_OP_CONTINUE:
			break;
		case DIR_OP_SKIP:
			for_each_child__continue (data);
			return;
		case DIR_OP_STOP:
			for_each_child_data_done (data, NULL);
			return;
		}
	}

	gio_file = gth_file_source_to_gio_file (data->file_source, file_data->file);
	uri = g_file_get_uri (file_data->file);
	if (g_str_has_suffix (uri, ".gqv")
	    || g_str_has_suffix (uri, ".catalog")
	    || g_str_has_suffix (uri, ".search"))
	{
		gth_catalog_set_file (data->catalog, gio_file);
		gth_catalog_list_async (data->catalog,
					data->attributes,
					gth_file_source_get_cancellable (data->file_source),
					for_each_child__catalog_list_ready_cb,
					data);
	}
	else
		g_directory_foreach_child (gio_file,
					   FALSE,
					   TRUE,
					   GFILE_STANDARD_ATTRIBUTES_WITH_FAST_CONTENT_TYPE,
					   gth_file_source_get_cancellable (data->file_source),
					   for_each_child__start_dir_func,
					   for_each_child__for_each_file_func,
					   for_each_child__done_func,
					   data);

	g_object_unref (gio_file);
	g_free (uri);
}


static void
for_each_child__parent_info_ready_cb (GObject      *source_object,
				      GAsyncResult *result,
				      gpointer      user_data)
{
	ForEachChildData *data = user_data;
	GFile            *file;
	GFileInfo        *info;
	GError           *error = NULL;
	GthFileData      *file_data;

	file = G_FILE (source_object);
	info = g_file_query_info_finish (file, result, &error);
	if (info == NULL) {
		for_each_child_data_done (data, error);
		return;
	}

	file_data = gth_file_source_get_file_data (data->file_source, file, info);
	for_each_child__visit_file (data, file_data);

	g_object_unref (file_data);
}


static void
gth_file_source_catalogs_for_each_child (GthFileSource        *file_source,
					 GFile                *parent,
					 gboolean              recursive,
					 const char           *attributes,
					 StartDirCallback      start_dir_func,
					 ForEachChildCallback  for_each_file_func,
					 ReadyCallback         ready_func,
					 gpointer              user_data)
{
	ForEachChildData *data;
	GFile            *gio_parent;

	data = g_new0 (ForEachChildData, 1);
	data->file_source = g_object_ref (file_source);
	data->recursive = recursive;
	data->attributes = g_strdup (attributes);
	data->start_dir_func = start_dir_func;
	data->for_each_file_func = for_each_file_func;
	data->ready_func = ready_func;
	data->user_data = user_data;
	data->catalog = gth_catalog_new ();

	gio_parent = gth_file_source_to_gio_file (file_source, parent);
	g_file_query_info_async (gio_parent,
			         GFILE_STANDARD_ATTRIBUTES_WITH_FAST_CONTENT_TYPE,
				 G_FILE_QUERY_INFO_NONE,
				 G_PRIORITY_DEFAULT,
				 gth_file_source_get_cancellable (data->file_source),
				 for_each_child__parent_info_ready_cb,
				 data);

	g_object_unref (gio_parent);
}


/* -- gth_file_source_catalogs_copy -- */


typedef struct {
	GthFileSource    *file_source;
	GthFileData      *destination;
	GList            *file_list;
	ProgressCallback  progress_callback;
	DialogCallback    dialog_callback;
	ReadyCallback     ready_callback;
	gpointer          user_data;
	GList            *files;
	GthCatalog       *catalog;
} CopyOpData;


static void
copy_op_data_free (CopyOpData *cod)
{
	_g_object_unref (cod->catalog);
	_g_object_list_unref (cod->files);
	_g_object_list_unref (cod->file_list);
	g_object_unref (cod->destination);
	g_object_unref (cod->file_source);
	g_free (cod);
}


static void
catalog_save_done_cb (void     **buffer,
		      gsize      count,
		      GError    *error,
		      gpointer   user_data)
{
	CopyOpData *cod = user_data;

	if (error == NULL)
		gth_monitor_folder_changed (gth_main_get_default_monitor (),
				            cod->destination->file,
				            cod->files,
					    GTH_MONITOR_EVENT_CREATED);

	cod->ready_callback (G_OBJECT (cod->file_source), error, cod->user_data);
	copy_op_data_free (cod);
}


static void
catalog_ready_cb (GObject  *catalog,
		  GError   *error,
		  gpointer  user_data)
{
	CopyOpData *cod = user_data;
	GList      *scan;
	char       *buffer;
	gsize       size;
	GFile      *gio_file;

	if (error != NULL) {
		cod->ready_callback (G_OBJECT (cod->file_source), error, cod->user_data);
		copy_op_data_free (cod);
		return;
	}

	cod->catalog = (GthCatalog *) catalog;

	for (scan = cod->files; scan; scan = scan->next)
		gth_catalog_insert_file (cod->catalog, (GFile *) scan->data, -1);

	buffer = gth_catalog_to_data (cod->catalog, &size);
	gio_file = gth_catalog_file_to_gio_file (cod->destination->file);
	g_write_file_async (gio_file,
			    buffer,
			    size,
			    TRUE,
			    G_PRIORITY_DEFAULT,
			    NULL,
			    catalog_save_done_cb,
			    cod);

	g_object_unref (gio_file);
}


static void
copy__file_list_info_ready_cb (GList    *files,
			       GError   *error,
			       gpointer  user_data)
{
	CopyOpData *cod = user_data;
	GList      *scan;

	for (scan = files; scan; scan = scan->next) {
		GthFileData *file_data = scan->data;

		switch (g_file_info_get_file_type (file_data->info)) {
		case G_FILE_TYPE_REGULAR:
		case G_FILE_TYPE_SYMBOLIC_LINK:
			cod->files = g_list_prepend (cod->files, g_object_ref (file_data->file));
			break;
		default:
			break;
		}
	}
	cod->files = g_list_reverse (cod->files);

	gth_catalog_load_from_file_async (cod->destination->file,
					  gth_file_source_get_cancellable (cod->file_source),
					  catalog_ready_cb,
					  cod);
}


static void
gth_file_source_catalogs_copy (GthFileSource    *file_source,
			       GthFileData      *destination,
			       GList            *file_list, /* GFile * list */
			       gboolean          move,
			       ProgressCallback  progress_callback,
			       DialogCallback    dialog_callback,
			       ReadyCallback     ready_callback,
			       gpointer          data)
{
	CopyOpData *cod;

	cod = g_new0 (CopyOpData, 1);
	cod->file_source = g_object_ref (file_source);
	cod->destination = g_object_ref (destination);
	cod->file_list = _g_object_list_ref (file_list);
	cod->progress_callback = progress_callback;
	cod->dialog_callback = dialog_callback;
	cod->ready_callback = ready_callback;
	cod->user_data = data;

	if (cod->progress_callback != NULL) {
		char *message;

		message = g_strdup_printf (_("Copying files to '%s'"), g_file_info_get_display_name (destination->info));
		(cod->progress_callback) (G_OBJECT (file_source), message, NULL, TRUE, 0.0, cod->user_data);

		g_free (message);
	}

	_g_query_info_async (cod->file_list,
			     GTH_LIST_DEFAULT,
			     GFILE_NAME_TYPE_ATTRIBUTES,
			     gth_file_source_get_cancellable (file_source),
			     copy__file_list_info_ready_cb,
			     cod);
}


static gboolean
gth_file_source_catalogs_is_reorderable (GthFileSource *file_source)
{
	return TRUE;

}


typedef struct {
	GthFileSource *file_source;
	GthFileData   *destination;
	GList         *visible_files; /* GFile list */
	GList         *files_to_move; /* GFile list */
	int            dest_pos;
	ReadyCallback  callback;
	gpointer       data;
	int           *new_order;
} ReorderData;


static void
reorder_data_free (ReorderData *reorder_data)
{
	gth_file_source_set_active (reorder_data->file_source, FALSE);
	_g_object_list_unref (reorder_data->visible_files);
	_g_object_list_unref (reorder_data->files_to_move);
	_g_object_unref (reorder_data->destination);
	_g_object_unref (reorder_data->file_source);
	g_free (reorder_data->new_order);
	g_free (reorder_data);
}


static void
reorder_buffer_ready_cb (void     **buffer,
		         gsize      count,
		         GError    *error,
		         gpointer   user_data)
{
	ReorderData *reorder_data = user_data;

	gth_monitor_order_changed (gth_main_get_default_monitor (),
				   reorder_data->destination->file,
				   reorder_data->new_order);
	reorder_data->callback (G_OBJECT (reorder_data->file_source), error, reorder_data->data);

	reorder_data_free (reorder_data);
}


static int
remove_from_file_list_and_get_position (GList **file_list,
					GFile  *file)
{
	GList *scan;
	int    i = 0;

	for (scan = *file_list; scan; scan = scan->next, i++)
		if (g_file_equal ((GFile *) scan->data, file))
			break;

	if (scan == NULL)
		return -1;

	*file_list = g_list_remove_link (*file_list, scan);

	return i;
}


static int *
reorder_catalog_list (GthCatalog *catalog,
		      GList      *visible_files,
		      GList      *files_to_move,
		      int         dest_pos)
{
	GHashTable *positions;
	GList      *new_visible_files;
	GList      *scan;
	int        *new_order;
	int         pos;
	GList      *new_file_list;
	GHashTable *visibles;

	/* save the original positions */

	positions = g_hash_table_new_full (g_file_hash, (GEqualFunc) g_file_equal, (GDestroyNotify) g_object_unref, NULL);
	for (scan = visible_files, pos = 0; scan; scan = scan->next, pos++)
		g_hash_table_insert (positions, g_object_ref ((GFile *) scan->data), GINT_TO_POINTER (pos));

	/* create the new visible list */

	new_visible_files = g_list_copy (visible_files);

	for (scan = files_to_move; scan; scan = scan->next) {
		int file_pos = remove_from_file_list_and_get_position (&new_visible_files, (GFile *) scan->data);
		if (file_pos < dest_pos)
			dest_pos--;
	}

	for (scan = files_to_move; scan; scan = scan->next) {
		new_visible_files = g_list_insert (new_visible_files, (GFile *) scan->data, dest_pos);
		dest_pos++;
	}

	/* compute the new order */

	new_order = g_new0 (int, g_list_length (new_visible_files));
	for (scan = new_visible_files, pos = 0; scan; scan = scan->next, pos++)
		new_order[pos] = GPOINTER_TO_INT (g_hash_table_lookup (positions, (GFile *) scan->data));

	/* save the new order in the catalog, appending the hidden files at
	 * the end. */

	new_file_list = _g_object_list_ref (new_visible_files);

	visibles = g_hash_table_new_full (g_file_hash, (GEqualFunc) g_file_equal, (GDestroyNotify) g_object_unref, NULL);
	for (scan = new_visible_files; scan; scan = scan->next)
		g_hash_table_insert (visibles, g_object_ref ((GFile *) scan->data), GINT_TO_POINTER (1));

	new_file_list = g_list_reverse (new_file_list);
	for (scan = gth_catalog_get_file_list (catalog); scan; scan = scan->next) {
		GFile *file = scan->data;

		if (g_hash_table_lookup (visibles, file) == NULL)
			new_file_list = g_list_prepend (new_file_list, g_object_ref (file));
	}
	new_file_list = g_list_reverse (new_file_list);
	gth_catalog_set_file_list (catalog, new_file_list);

	_g_object_list_unref (new_file_list);
	g_hash_table_destroy (visibles);
	g_list_free (new_visible_files);
	g_hash_table_destroy (positions);

	return new_order;
}


static void
reorder_catalog_ready_cb (GObject  *object,
			  GError   *error,
			  gpointer  user_data)
{
	ReorderData *reorder_data = user_data;
	GthCatalog  *catalog;
	char        *buffer;
	gsize        size;
	GFile       *gio_file;

	if (error != NULL) {
		reorder_data->callback (G_OBJECT (reorder_data->file_source), error, reorder_data->data);
		reorder_data_free (reorder_data);
		return;
	}

	catalog = (GthCatalog *) object;
	reorder_data->new_order = reorder_catalog_list (catalog,
							reorder_data->visible_files,
							reorder_data->files_to_move,
							reorder_data->dest_pos);
	gth_catalog_set_order (catalog, "general::unsorted", FALSE);

	buffer = gth_catalog_to_data (catalog, &size);
	gio_file = gth_file_source_to_gio_file (reorder_data->file_source, reorder_data->destination->file);
	g_write_file_async (gio_file,
			    buffer,
			    size,
			    TRUE,
			    G_PRIORITY_DEFAULT,
			    gth_file_source_get_cancellable (reorder_data->file_source),
			    reorder_buffer_ready_cb,
			    reorder_data);

	g_object_unref (gio_file);
}


static void
gth_file_source_catalogs_reorder (GthFileSource *file_source,
				  GthFileData   *destination,
				  GList         *visible_files, /* GFile list */
				  GList         *files_to_move, /* GFile list */
				  int            dest_pos,
				  ReadyCallback  callback,
				  gpointer       data)
{
	GthFileSourceCatalogs *catalogs = (GthFileSourceCatalogs *) file_source;
	ReorderData           *reorder_data;
	GFile                 *gio_file;

	gth_file_source_set_active (GTH_FILE_SOURCE (catalogs), TRUE);
	g_cancellable_reset (gth_file_source_get_cancellable (file_source));

	reorder_data = g_new0 (ReorderData, 1);
	reorder_data->file_source = g_object_ref (file_source);
	reorder_data->destination = g_object_ref (destination);
	reorder_data->visible_files = _g_object_list_ref (visible_files);
	reorder_data->files_to_move = _g_object_list_ref (files_to_move);
	reorder_data->dest_pos = dest_pos;
	reorder_data->callback = callback;
	reorder_data->data = data;

	gio_file = gth_file_source_to_gio_file (file_source, destination->file);
	gth_catalog_load_from_file_async (gio_file,
					  gth_file_source_get_cancellable (file_source),
					  reorder_catalog_ready_cb,
					  reorder_data);

	g_object_unref (gio_file);
}



static void
gth_file_source_catalogs_finalize (GObject *object)
{
	GthFileSourceCatalogs *catalogs = GTH_FILE_SOURCE_CATALOGS (object);

	if (catalogs->priv != NULL) {
		g_object_unref (catalogs->priv->catalog);
		_g_object_list_unref (catalogs->priv->files);
		catalogs->priv->files = NULL;

		g_free (catalogs->priv);
		catalogs->priv = NULL;
	}

	G_OBJECT_CLASS (parent_class)->finalize (object);
}


static void
gth_file_source_catalogs_class_init (GthFileSourceCatalogsClass *class)
{
	GObjectClass       *object_class;
	GthFileSourceClass *file_source_class;

	parent_class = g_type_class_peek_parent (class);
	object_class = (GObjectClass*) class;
	file_source_class = (GthFileSourceClass*) class;

	object_class->finalize = gth_file_source_catalogs_finalize;
	file_source_class->get_entry_points = get_entry_points;
	file_source_class->to_gio_file = gth_file_source_catalogs_to_gio_file;
	file_source_class->get_file_info = gth_file_source_catalogs_get_file_info;
	file_source_class->get_file_data = gth_file_source_catalogs_get_file_data;
	file_source_class->write_metadata = gth_file_source_catalogs_write_metadata;
	file_source_class->read_metadata = gth_file_source_catalogs_read_metadata;
	file_source_class->for_each_child = gth_file_source_catalogs_for_each_child;
	file_source_class->copy = gth_file_source_catalogs_copy;
	file_source_class->is_reorderable  = gth_file_source_catalogs_is_reorderable;
	file_source_class->reorder = gth_file_source_catalogs_reorder;

}


static void
gth_file_source_catalogs_init (GthFileSourceCatalogs *catalogs)
{
	gth_file_source_add_scheme (GTH_FILE_SOURCE (catalogs), "catalog://");

	catalogs->priv = g_new0 (GthFileSourceCatalogsPrivate, 1);
	catalogs->priv->catalog = gth_catalog_new ();
}


GType
gth_file_source_catalogs_get_type (void)
{
	static GType type = 0;

	if (! type) {
		GTypeInfo type_info = {
			sizeof (GthFileSourceCatalogsClass),
			NULL,
			NULL,
			(GClassInitFunc) gth_file_source_catalogs_class_init,
			NULL,
			NULL,
			sizeof (GthFileSourceCatalogs),
			0,
			(GInstanceInitFunc) gth_file_source_catalogs_init
		};

		type = g_type_register_static (GTH_TYPE_FILE_SOURCE,
					       "GthFileSourceCatalogs",
					       &type_info,
					       0);
	}

	return type;
}
