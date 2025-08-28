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
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <config.h>
#include "glib-utils.h"
#include "gth-file-data.h"
#include "gth-main.h"
#include "gth-metadata-provider.h"


#define CHECK_THREAD_RATE 5


G_DEFINE_TYPE (GthMetadataProvider, gth_metadata_provider, G_TYPE_OBJECT)


static gboolean
gth_metadata_provider_real_can_read (GthMetadataProvider  *self,
				     GthFileData          *file_data,
				     const char           *mime_type,
				     char                **attribute_v)
{
	return FALSE;
}


static gboolean
gth_metadata_provider_real_can_write (GthMetadataProvider  *self,
				      const char           *mime_type,
				      char                **attribute_v)
{
	return FALSE;
}


static void
gth_metadata_provider_real_read (GthMetadataProvider *self,
				 GthFileData         *file_data,
				 const char          *attributes,
				 GCancellable        *cancellable)
{
	/* void */
}


static void
gth_metadata_provider_real_write (GthMetadataProvider   *self,
				  GthMetadataWriteFlags  flags,
				  GthFileData           *file_data,
				  const char            *attributes,
				  GCancellable          *cancellable)
{
	/* void */
}


static int
gth_metadata_provider_real_get_priority (GthMetadataProvider *self)
{
	return 10;
}


static void
gth_metadata_provider_class_init (GthMetadataProviderClass * klass)
{
	GTH_METADATA_PROVIDER_CLASS (klass)->can_read = gth_metadata_provider_real_can_read;
	GTH_METADATA_PROVIDER_CLASS (klass)->can_write = gth_metadata_provider_real_can_write;
	GTH_METADATA_PROVIDER_CLASS (klass)->read = gth_metadata_provider_real_read;
	GTH_METADATA_PROVIDER_CLASS (klass)->write = gth_metadata_provider_real_write;
	GTH_METADATA_PROVIDER_CLASS (klass)->get_priority = gth_metadata_provider_real_get_priority;
}


static void
gth_metadata_provider_init (GthMetadataProvider *self)
{
	/* void */
}


gboolean
gth_metadata_provider_can_read (GthMetadataProvider  *self,
				GthFileData          *file_data,
				const char           *mime_type,
				char                **attribute_v)
{
	return GTH_METADATA_PROVIDER_GET_CLASS (self)->can_read (self, file_data, mime_type, attribute_v);
}


gboolean
gth_metadata_provider_can_write (GthMetadataProvider  *self,
				 const char           *mime_type,
				 char                **attribute_v)
{
	return GTH_METADATA_PROVIDER_GET_CLASS (self)->can_write (self, mime_type, attribute_v);
}


void
gth_metadata_provider_read (GthMetadataProvider *self,
			    GthFileData         *file_data,
			    const char          *attributes,
			    GCancellable        *cancellable)
{
	GTH_METADATA_PROVIDER_GET_CLASS (self)->read (self, file_data, attributes, cancellable);
}


void
gth_metadata_provider_write (GthMetadataProvider   *self,
			     GthMetadataWriteFlags  flags,
			     GthFileData           *file_data,
			     const char            *attributes,
			     GCancellable          *cancellable)
{
	GTH_METADATA_PROVIDER_GET_CLASS (self)->write (self, flags, file_data, attributes, cancellable);
}


int
gth_metadata_provider_get_priority (GthMetadataProvider *self)
{
	return GTH_METADATA_PROVIDER_GET_CLASS (self)->get_priority (self);
}


/* -- _g_query_metadata_async -- */


typedef struct {
	GList   *files;
	char   *attributes;
	char  **attributes_v;
} QueryMetadataData;


static void
query_metadata_data_free (gpointer user_data)
{
	QueryMetadataData *qmd = user_data;

	g_strfreev (qmd->attributes_v);
	g_free (qmd->attributes);
	_g_object_list_unref (qmd->files);
	g_free (qmd);
}


static void
_g_query_metadata_async_thread (GTask        *task,
				gpointer      source_object,
				gpointer      task_data,
				GCancellable *cancellable)
{
	QueryMetadataData  *qmd;
	GList              *providers;
	GList              *scan;
	GError             *error = NULL;

	performance (DEBUG_INFO, "_g_query_metadata_async_thread start");

	qmd = g_task_get_task_data (task);

	providers = NULL;
	for (scan = gth_main_get_all_metadata_providers (); scan; scan = scan->next)
		providers = g_list_prepend (providers, g_object_new (G_OBJECT_TYPE (scan->data), NULL));
	providers = g_list_reverse (providers);

	for (scan = qmd->files; scan; scan = scan->next) {
		GthFileData *file_data = scan->data;
		GList       *scan_providers;

		if ((cancellable != NULL) && g_cancellable_is_cancelled (cancellable)) {
			error = g_error_new_literal (G_IO_ERROR, G_IO_ERROR_CANCELLED, "");
			break;
		}

#if WEBP_IS_UNKNOWN_TO_GLIB || JXL_IS_UNKNOWN_TO_GLIB
		if (_g_file_attributes_matches_any_v (G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE ","
						      G_FILE_ATTRIBUTE_STANDARD_FAST_CONTENT_TYPE,
						      qmd->attributes_v))
		{
			char *uri;
			char *ext;

			uri = g_file_get_uri (file_data->file);
			ext = _g_uri_get_extension (uri);
#if WEBP_IS_UNKNOWN_TO_GLIB
			if (g_strcmp0 (ext, ".webp") == 0)
				gth_file_data_set_mime_type (file_data, "image/webp");
#endif
#if JXL_IS_UNKNOWN_TO_GLIB
			if (g_strcmp0 (ext, ".jxl") == 0)
				gth_file_data_set_mime_type (file_data, "image/jxl");
#endif

			g_free (ext);
			g_free (uri);
		}
#endif

		for (scan_providers = providers; scan_providers; scan_providers = scan_providers->next) {
			GthMetadataProvider *metadata_provider = scan_providers->data;

			if (gth_metadata_provider_can_read (metadata_provider,
							    file_data,
							    gth_file_data_get_mime_type (file_data),
							    qmd->attributes_v))
			{
				gth_metadata_provider_read (metadata_provider, file_data, qmd->attributes, cancellable);
			}
		}
	}

	_g_object_list_unref (providers);

	performance (DEBUG_INFO, "_g_query_metadata_async_thread before read-metadata-ready");

	if (error == NULL)
		gth_hook_invoke ("read-metadata-ready", qmd->files, qmd->attributes);

	if (error != NULL)
		g_task_return_error (task, error);
	else
		g_task_return_boolean (task, TRUE);

	performance (DEBUG_INFO, "_g_query_metadata_async_thread end");
}


void
_g_query_metadata_async (GList               *files,       /* GthFileData * list */
			 const char          *attributes,
			 GCancellable        *cancellable,
			 GAsyncReadyCallback  callback,
			 gpointer             user_data)
{
	GTask             *task;
	QueryMetadataData *qmd;

	qmd = g_new0 (QueryMetadataData, 1);
	qmd->files = _g_object_list_ref (files);
	qmd->attributes = g_strdup (attributes);
	qmd->attributes_v = gth_main_get_metadata_attributes (attributes);

	task = g_task_new (NULL, cancellable, callback, user_data);
	g_task_set_task_data (task, qmd, query_metadata_data_free);
	g_task_run_in_thread (task, _g_query_metadata_async_thread);

	g_object_unref (task);
}


GList *
_g_query_metadata_finish (GAsyncResult  *result,
			  GError       **error)
{
	QueryMetadataData  *qmd;

	if (! g_task_propagate_boolean (G_TASK (result), error))
		return NULL;

	qmd = g_task_get_task_data (G_TASK (result));

	return qmd->files;
}


/* -- _g_write_metadata_async -- */


typedef struct {
	GList                  *files;
	GthMetadataWriteFlags   flags;
	char                   *attributes;
	char                  **attributes_v;
} WriteMetadataData;


static void
write_metadata_data_free (gpointer user_data)
{
	WriteMetadataData *wmd = user_data;

	g_strfreev (wmd->attributes_v);
	g_free (wmd->attributes);
	_g_object_list_unref (wmd->files);
	g_free (wmd);
}


static void
_g_write_metadata_async_thread (GTask        *task,
				gpointer      source_object,
				gpointer      task_data,
				GCancellable *cancellable)
{
	WriteMetadataData  *wmd;
	GList              *providers;
	GList              *scan;
	GError             *error = NULL;

	wmd = g_task_get_task_data (task);

	providers = NULL;
	for (scan = gth_main_get_all_metadata_providers (); scan; scan = scan->next)
		providers = g_list_prepend (providers, g_object_new (G_OBJECT_TYPE (scan->data), NULL));
	providers = g_list_reverse (providers);

	for (scan = wmd->files; scan; scan = scan->next) {
		GthFileData *file_data = scan->data;
		GList       *scan_providers;

		if ((cancellable != NULL) && g_cancellable_is_cancelled (cancellable)) {
			error = g_error_new_literal (G_IO_ERROR, G_IO_ERROR_CANCELLED, "");
			break;
		}

		for (scan_providers = providers; scan_providers; scan_providers = scan_providers->next) {
			GthMetadataProvider *metadata_provider = scan_providers->data;

			if (gth_metadata_provider_can_write (metadata_provider, gth_file_data_get_mime_type (file_data), wmd->attributes_v))
				gth_metadata_provider_write (metadata_provider, wmd->flags, file_data, wmd->attributes, cancellable);
		}
	}

	_g_object_list_unref (providers);

	if (error != NULL)
		g_task_return_error (task, error);
	else
		g_task_return_boolean (task, TRUE);
}


void
_g_write_metadata_async (GList                  *files, /* GthFileData * list */
			 GthMetadataWriteFlags   flags,
			 const char             *attributes,
			 GCancellable           *cancellable,
			 GAsyncReadyCallback     callback,
			 gpointer                user_data)
{
	GTask             *task;
	WriteMetadataData *wmd;

	wmd = g_new0 (WriteMetadataData, 1);
	wmd->files = _g_object_list_ref (files);
	wmd->attributes = g_strdup (attributes);
	wmd->attributes_v = gth_main_get_metadata_attributes (attributes);

	task = g_task_new (NULL, cancellable, callback, user_data);
	g_task_set_task_data (task, wmd, write_metadata_data_free);
	g_task_run_in_thread (task, _g_write_metadata_async_thread);

	g_object_unref (task);
}


gboolean
_g_write_metadata_finish (GAsyncResult  *result,
			  GError       **error)
{
	  return g_task_propagate_boolean (G_TASK (result), error);
}


/* -- _g_query_all_metadata_async -- */


typedef struct {
	GList             *files;
	char              *attributes;
	GCancellable      *cancellable;
	InfoReadyCallback  ready_func;
	gpointer           user_data;
} QueryAllMetadata;


static void
query_all_metadata_free (QueryAllMetadata *qam)
{
	_g_object_list_unref (qam->files);
	g_free (qam->attributes);
	_g_object_unref (qam->cancellable);
	g_free (qam);
}


static void
qam_metadata_ready_cb (GObject      *source_object,
                       GAsyncResult *result,
                       gpointer      user_data)
{
	QueryAllMetadata *qam = user_data;
	GList            *files;
	GError           *error = NULL;

	files = _g_query_metadata_finish (result, &error);
	if (files != NULL)
		qam->ready_func (files, NULL, qam->user_data);
	else
		qam->ready_func (NULL, error, qam->user_data);

	query_all_metadata_free (qam);
}


static void
qam_info_ready_cb (GList    *files,
		   GError   *error,
		   gpointer  user_data)
{
	QueryAllMetadata *qam = user_data;

	if (error != NULL) {
		qam->ready_func (NULL, error, qam->user_data);
		query_all_metadata_free (qam);
		return;
	}

	qam->files = _g_object_list_ref (files);
	_g_query_metadata_async (qam->files,
				 qam->attributes,
				 qam->cancellable,
				 qam_metadata_ready_cb,
				 qam);
}


void
_g_query_all_metadata_async (GList             *files, /* GFile * list */
			     GthListFlags       flags,
			     const char        *attributes,
			     GCancellable      *cancellable,
			     InfoReadyCallback  ready_func,
			     gpointer           user_data)
{
	QueryAllMetadata *qam;

	qam = g_new0 (QueryAllMetadata, 1);
	qam->attributes = g_strdup (attributes);
	qam->cancellable = _g_object_ref (cancellable);
	qam->ready_func = ready_func;
	qam->user_data = user_data;

	_g_file_list_query_info_async (files,
				       flags,
				       qam->attributes,
				       qam->cancellable,
				       qam_info_ready_cb,
				       qam);
}
