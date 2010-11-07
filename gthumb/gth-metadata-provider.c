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


static gpointer gth_metadata_provider_parent_class = NULL;


static gboolean
gth_metadata_provider_real_can_read (GthMetadataProvider  *self,
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
				 const char          *attributes)
{
	/* void */
}


static void
gth_metadata_provider_real_write (GthMetadataProvider   *self,
				  GthMetadataWriteFlags  flags,
				  GthFileData           *file_data,
				  const char            *attributes)
{
	/* void */
}


static void
gth_metadata_provider_class_init (GthMetadataProviderClass * klass)
{
	gth_metadata_provider_parent_class = g_type_class_peek_parent (klass);

	GTH_METADATA_PROVIDER_CLASS (klass)->can_read = gth_metadata_provider_real_can_read;
	GTH_METADATA_PROVIDER_CLASS (klass)->can_write = gth_metadata_provider_real_can_write;
	GTH_METADATA_PROVIDER_CLASS (klass)->read = gth_metadata_provider_real_read;
	GTH_METADATA_PROVIDER_CLASS (klass)->write = gth_metadata_provider_real_write;
}


GType
gth_metadata_provider_get_type (void)
{
	static GType gth_metadata_provider_type_id = 0;
	if (gth_metadata_provider_type_id == 0) {
		static const GTypeInfo g_define_type_info = {
			sizeof (GthMetadataProviderClass),
			(GBaseInitFunc) NULL,
			(GBaseFinalizeFunc) NULL,
			(GClassInitFunc) gth_metadata_provider_class_init,
			(GClassFinalizeFunc) NULL,
			NULL,
			sizeof (GthMetadataProvider),
			0,
			(GInstanceInitFunc) NULL,
			NULL
		};
		gth_metadata_provider_type_id = g_type_register_static (G_TYPE_OBJECT, "GthMetadataProvider", &g_define_type_info, 0);
	}
	return gth_metadata_provider_type_id;
}


gboolean
gth_metadata_provider_can_read (GthMetadataProvider  *self,
				const char           *mime_type,
				char                **attribute_v)
{
	return GTH_METADATA_PROVIDER_GET_CLASS (self)->can_read (self, mime_type, attribute_v);
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
			    const char          *attributes)
{
	GTH_METADATA_PROVIDER_GET_CLASS (self)->read (self, file_data, attributes);
}


void
gth_metadata_provider_write (GthMetadataProvider   *self,
			     GthMetadataWriteFlags  flags,
			     GthFileData           *file_data,
			     const char            *attributes)
{
	GTH_METADATA_PROVIDER_GET_CLASS (self)->write (self, flags, file_data, attributes);
}


/* -- _g_query_metadata_async -- */


typedef struct {
	GList        *files;
	char         *attributes;
	char        **attributes_v;
	GMutex       *mutex;
	gboolean      thread_done;
	GError       *error;
} QueryMetadataThreadData;


typedef struct {
	GCancellable            *cancellable;
	gulong                   cancel_signal;
	InfoReadyCallback        ready_func;
	gpointer                 user_data;
	guint                    check_id;
	GThread                 *thread;
	QueryMetadataThreadData *rmtd;
} QueryMetadataData;


static void
query_metadata_done (QueryMetadataData *rmd)
{
	QueryMetadataThreadData *rmtd = rmd->rmtd;

	if (rmd->ready_func != NULL)
		(*rmd->ready_func) (rmtd->files, rmtd->error, rmd->user_data);

	g_mutex_free (rmtd->mutex);
	g_strfreev (rmtd->attributes_v);
	g_free (rmtd->attributes);
	_g_object_list_unref (rmtd->files);
	g_free (rmtd);
	if (rmd->cancel_signal != 0)
		g_cancellable_disconnect (rmd->cancellable, rmd->cancel_signal);
	_g_object_unref (rmd->cancellable);
	g_free (rmd);
}


static gpointer
read_metadata_thread (gpointer data)
{
	QueryMetadataThreadData *rmtd = data;
	GList                   *providers;
	GList                   *scan;
	gboolean                 cancelled = FALSE;

	providers = NULL;
	for (scan = gth_main_get_all_metadata_providers (); scan; scan = scan->next)
		providers = g_list_prepend (providers, g_object_new (G_OBJECT_TYPE (scan->data), NULL));
	providers = g_list_reverse (providers);

	for (scan = rmtd->files; scan; scan = scan->next) {
		GthFileData *file_data = scan->data;
		GList       *scan_providers;

		g_mutex_lock (rmtd->mutex);
		cancelled = rmtd->thread_done;
		g_mutex_unlock (rmtd->mutex);

		if (cancelled)
			break;

		for (scan_providers = providers; scan_providers; scan_providers = scan_providers->next) {
			GthMetadataProvider *metadata_provider = scan_providers->data;

			if (gth_metadata_provider_can_read (metadata_provider, gth_file_data_get_mime_type (file_data), rmtd->attributes_v))
				gth_metadata_provider_read (metadata_provider, file_data, rmtd->attributes);
		}
	}

	_g_object_list_unref (providers);

	if (! cancelled)
		for (scan = rmtd->files; scan; scan = scan->next) {
			GthFileData *file_data = scan->data;
			gth_hook_invoke ("read-metadata-ready", file_data, rmtd->attributes);
		}

	g_mutex_lock (rmtd->mutex);
	rmtd->thread_done = TRUE;
	g_mutex_unlock (rmtd->mutex);

	return rmtd;
}


static gboolean
check_read_metadata_thread (gpointer data)
{
	QueryMetadataData       *rmd = data;
	QueryMetadataThreadData *rmtd = rmd->rmtd;
	gboolean                 thread_done;

	g_source_remove (rmd->check_id);
	rmd->check_id = 0;

	g_mutex_lock (rmtd->mutex);
	thread_done = rmtd->thread_done;
	g_mutex_unlock (rmtd->mutex);

	if (thread_done)
		query_metadata_done (rmd);
	else
		rmd->check_id = g_timeout_add (CHECK_THREAD_RATE, check_read_metadata_thread, rmd);

	return FALSE;
}


static void
query_metadata_cancelled_cb (GCancellable *cancellable,
                             gpointer      user_data)
{
	QueryMetadataData       *rmd = user_data;
	QueryMetadataThreadData *rmtd = rmd->rmtd;

	g_mutex_lock (rmtd->mutex);
	rmtd->thread_done = TRUE;
	rmtd->error = g_error_new_literal (G_IO_ERROR, G_IO_ERROR_CANCELLED, "");
	g_mutex_unlock (rmtd->mutex);
}


void
_g_query_metadata_async (GList             *files,       /* GthFileData * list */
			 const char        *attributes,
			 GCancellable      *cancellable,
			 InfoReadyCallback  ready_func,
			 gpointer           user_data)
{
	QueryMetadataData       *rmd;
	QueryMetadataThreadData *rmtd;

	rmtd = g_new0 (QueryMetadataThreadData, 1);
	rmtd->files = _g_object_list_ref (files);
	rmtd->attributes = g_strdup (attributes);
	rmtd->attributes_v = gth_main_get_metadata_attributes (attributes);
	rmtd->mutex = g_mutex_new ();

	rmd = g_new0 (QueryMetadataData, 1);
	rmd->cancellable = _g_object_ref (cancellable);
	if (rmd->cancellable != NULL)
		rmd->cancel_signal = g_cancellable_connect (rmd->cancellable,
							    G_CALLBACK (query_metadata_cancelled_cb),
							    rmd,
							    NULL);

	rmd->ready_func = ready_func;
	rmd->user_data = user_data;
	rmd->rmtd = rmtd;
	rmd->thread = g_thread_create (read_metadata_thread, rmtd, FALSE, NULL);
	rmd->check_id = g_timeout_add (CHECK_THREAD_RATE, check_read_metadata_thread, rmd);
}


/* -- _g_write_metadata_async -- */


typedef struct {
	GList                  *files;
	GthMetadataWriteFlags   flags;
	char                   *attributes;
	char                  **attributes_v;
	GMutex                 *mutex;
	gboolean                thread_done;
	GError                 *error;
} WriteMetadataThreadData;


typedef struct {
	GCancellable            *cancellable;
	ReadyFunc                ready_func;
	gpointer                 user_data;
	guint                    check_id;
	GThread                 *thread;
	WriteMetadataThreadData *wmtd;
} WriteMetadataData;


static void
write_metadata_done (WriteMetadataData *wmd)
{
	WriteMetadataThreadData *wmtd = wmd->wmtd;

	g_thread_join (wmd->thread);

	if (wmd->ready_func != NULL)
		(*wmd->ready_func) (wmtd->error, wmd->user_data);

	g_mutex_free (wmtd->mutex);
	g_strfreev (wmtd->attributes_v);
	g_free (wmtd->attributes);
	_g_object_list_unref (wmtd->files);
	g_free (wmtd);
	g_free (wmd);
}


static gpointer
write_metadata_thread (gpointer data)
{
	WriteMetadataThreadData *wmtd = data;
	GList                   *providers;
	GList                   *scan;
	gboolean                 cancelled = FALSE;

	providers = NULL;
	for (scan = gth_main_get_all_metadata_providers (); scan; scan = scan->next)
		providers = g_list_prepend (providers, g_object_new (G_OBJECT_TYPE (scan->data), NULL));
	providers = g_list_reverse (providers);

	for (scan = wmtd->files; scan; scan = scan->next) {
		GthFileData *file_data = scan->data;
		GList       *scan_providers;

		g_mutex_lock (wmtd->mutex);
		cancelled = wmtd->thread_done;
		g_mutex_unlock (wmtd->mutex);

		if (cancelled)
			break;

		for (scan_providers = providers; scan_providers; scan_providers = scan_providers->next) {
			GthMetadataProvider *metadata_provider = scan_providers->data;

			if (gth_metadata_provider_can_write (metadata_provider, gth_file_data_get_mime_type (file_data), wmtd->attributes_v))
				gth_metadata_provider_write (metadata_provider, wmtd->flags, file_data, wmtd->attributes);
		}
	}

	_g_object_list_unref (providers);

	g_mutex_lock (wmtd->mutex);
	wmtd->thread_done = TRUE;
	g_mutex_unlock (wmtd->mutex);

	return wmtd;
}


static gboolean
check_write_metadata_thread (gpointer data)
{
	WriteMetadataData       *wmd = data;
	WriteMetadataThreadData *wmtd = wmd->wmtd;
	gboolean                 thread_done;

	g_source_remove (wmd->check_id);
	wmd->check_id = 0;

	g_mutex_lock (wmtd->mutex);
	thread_done = wmtd->thread_done;
	g_mutex_unlock (wmtd->mutex);

	if (thread_done)
		write_metadata_done (wmd);
	else
		wmd->check_id = g_timeout_add (CHECK_THREAD_RATE, check_write_metadata_thread, wmd);

	return FALSE;
}


void
_g_write_metadata_async (GList                 *files, /* GthFileData * list */
			 GthMetadataWriteFlags  flags,
			 const char            *attributes,
			 GCancellable          *cancellable,
			 ReadyFunc              ready_func,
			 gpointer               user_data)
{
	WriteMetadataData       *wmd;
	WriteMetadataThreadData *wmtd;

	wmtd = g_new0 (WriteMetadataThreadData, 1);
	wmtd->files = _g_object_list_ref (files);
	wmtd->flags = flags;
	wmtd->attributes = g_strdup (attributes);
	wmtd->attributes_v = gth_main_get_metadata_attributes (attributes);
	wmtd->mutex = g_mutex_new ();

	wmd = g_new0 (WriteMetadataData, 1);
	wmd->cancellable = cancellable;
	wmd->ready_func = ready_func;
	wmd->user_data = user_data;
	wmd->wmtd = wmtd;
	wmd->thread = g_thread_create (write_metadata_thread, wmtd, TRUE, NULL);
	wmd->check_id = g_timeout_add (CHECK_THREAD_RATE, check_write_metadata_thread, wmd);
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
qam_metadata_ready_cb (GList    *files,
		       GError   *error,
		       gpointer  user_data)
{
	QueryAllMetadata *qam = user_data;

	qam->ready_func (qam->files, error, qam->user_data);
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

	_g_query_info_async (files,
			     flags,
			     qam->attributes,
			     qam->cancellable,
			     qam_info_ready_cb,
			     qam);
}
