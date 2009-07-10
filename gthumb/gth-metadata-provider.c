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
#include "gth-file-data.h"
#include "gth-main.h"
#include "gth-metadata-provider.h"


#define GTH_METADATA_PROVIDER_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), GTH_TYPE_METADATA_PROVIDER, GthMetadataProviderPrivate))
#define CHECK_THREAD_RATE 5


enum  {
	GTH_METADATA_PROVIDER_DUMMY_PROPERTY,
	GTH_METADATA_PROVIDER_READABLE_ATTRIBUTES,
	GTH_METADATA_PROVIDER_WRITABLE_ATTRIBUTES
};


struct _GthMetadataProviderPrivate {
	char *_readable_attributes;
	char *_writable_attributes;
};

static gpointer gth_metadata_provider_parent_class = NULL;


static void
gth_metadata_provider_real_read (GthMetadataProvider *self,
				 GthFileData         *file_data,
				 const char          *attributes)
{
}


static void
gth_metadata_provider_real_write (GthMetadataProvider *self,
				  GthFileData         *file_data,
				  const char          *attributes)
{
}


static void
gth_metadata_provider_set_readable_attributes (GthMetadataProvider *self,
					       const char          *value)
{
	if (self->priv->_readable_attributes != NULL) {
		g_free (self->priv->_readable_attributes);
		self->priv->_readable_attributes = NULL;
	}

	if (value != NULL)
		self->priv->_readable_attributes = g_strdup (value);
}


static void
gth_metadata_provider_set_writable_attributes (GthMetadataProvider *self,
					       const char          *value)
{
	if (self->priv->_writable_attributes != NULL) {
		g_free (self->priv->_writable_attributes);
		self->priv->_writable_attributes = NULL;
	}

	if (value != NULL)
		self->priv->_writable_attributes = g_strdup (value);
}


static void
gth_metadata_provider_set_property (GObject      *object,
				    guint         property_id,
				    const GValue *value,
				    GParamSpec   *pspec)
{
	GthMetadataProvider *self;

	self = GTH_METADATA_PROVIDER (object);

	switch (property_id) {
	case GTH_METADATA_PROVIDER_READABLE_ATTRIBUTES:
		gth_metadata_provider_set_readable_attributes (self, g_value_get_string (value));
		break;
	case GTH_METADATA_PROVIDER_WRITABLE_ATTRIBUTES:
		gth_metadata_provider_set_writable_attributes (self, g_value_get_string (value));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
		break;
	}
}


static void
gth_metadata_provider_finalize (GObject *obj)
{
	GthMetadataProvider *self;

	self = GTH_METADATA_PROVIDER (obj);

	g_free (self->priv->_readable_attributes);
	g_free (self->priv->_writable_attributes);

	G_OBJECT_CLASS (gth_metadata_provider_parent_class)->finalize (obj);
}


static void
gth_metadata_provider_class_init (GthMetadataProviderClass * klass)
{
	gth_metadata_provider_parent_class = g_type_class_peek_parent (klass);
	g_type_class_add_private (klass, sizeof (GthMetadataProviderPrivate));

	G_OBJECT_CLASS (klass)->set_property = gth_metadata_provider_set_property;
	G_OBJECT_CLASS (klass)->finalize = gth_metadata_provider_finalize;

	GTH_METADATA_PROVIDER_CLASS (klass)->read = gth_metadata_provider_real_read;
	GTH_METADATA_PROVIDER_CLASS (klass)->write = gth_metadata_provider_real_write;

	g_object_class_install_property (G_OBJECT_CLASS (klass),
					 GTH_METADATA_PROVIDER_READABLE_ATTRIBUTES,
					 g_param_spec_string ("readable-attributes",
					 		      "readable-attributes",
					 		      "readable-attributes",
					 		      NULL,
					 		      G_PARAM_STATIC_NAME | G_PARAM_STATIC_NICK | G_PARAM_STATIC_BLURB | G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY));
	g_object_class_install_property (G_OBJECT_CLASS (klass),
					 GTH_METADATA_PROVIDER_WRITABLE_ATTRIBUTES,
					 g_param_spec_string ("writable-attributes",
					 		      "writable-attributes",
					 		      "writable-attributes",
					 		      NULL,
					 		      G_PARAM_STATIC_NAME | G_PARAM_STATIC_NICK | G_PARAM_STATIC_BLURB | G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY));
}


static void
gth_metadata_provider_instance_init (GthMetadataProvider *self)
{
	self->priv = GTH_METADATA_PROVIDER_GET_PRIVATE (self);
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
			(GInstanceInitFunc) gth_metadata_provider_instance_init,
			NULL
		};
		gth_metadata_provider_type_id = g_type_register_static (G_TYPE_OBJECT, "GthMetadataProvider", &g_define_type_info, 0);
	}
	return gth_metadata_provider_type_id;
}


GthMetadataProvider *
gth_metadata_provider_new (void)
{

	return g_object_new (GTH_TYPE_METADATA_PROVIDER, NULL);
}


static gboolean
attribute_matches_attributes (const char  *attributes,
			      char       **attribute_v)
{
	gboolean matches;
	int      i;

	if (attributes == NULL)
		return FALSE;

	matches = FALSE;
	for (i = 0; ! matches && (attribute_v[i] != NULL); i++)
		matches = _g_file_attributes_matches (attributes, attribute_v[i]);

	return matches;
}


gboolean
gth_metadata_provider_can_read (GthMetadataProvider  *self,
				char                **attribute_v)
{
	g_return_val_if_fail (self != NULL, FALSE);
	g_return_val_if_fail (attribute_v != NULL, FALSE);

	return attribute_matches_attributes (self->priv->_readable_attributes, attribute_v);
}


gboolean
gth_metadata_provider_can_write (GthMetadataProvider  *self,
				 char                **attribute_v)
{
	g_return_val_if_fail (self != NULL, FALSE);
	g_return_val_if_fail (attribute_v != NULL, FALSE);

	return attribute_matches_attributes (self->priv->_writable_attributes, attribute_v);
}


void
gth_metadata_provider_read (GthMetadataProvider *self,
			    GthFileData         *file_data,
			    const char          *attributes)
{
	GTH_METADATA_PROVIDER_GET_CLASS (self)->read (self, file_data, attributes);
}


void
gth_metadata_provider_write (GthMetadataProvider *self,
			     GthFileData         *file_data,
			     const char          *attributes)
{
	GTH_METADATA_PROVIDER_GET_CLASS (self)->write (self, file_data, attributes);
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
	g_free (rmd);
}


static gpointer
read_metadata_thread (gpointer data)
{
	QueryMetadataThreadData *rmtd = data;
	GList                   *scan;

	for (scan = gth_main_get_all_metadata_providers (); scan; scan = scan->next) {
		GthMetadataProvider *metadata_provider;

		metadata_provider = g_object_new (G_OBJECT_TYPE (scan->data), NULL);

		if (gth_metadata_provider_can_read (metadata_provider, rmtd->attributes_v)) {
			GList *scan_files;

			for (scan_files = rmtd->files; scan_files; scan_files = scan_files->next) {
				GthFileData *file_data = scan_files->data;
				gth_metadata_provider_read (metadata_provider, file_data, rmtd->attributes);
			}
		}

		g_object_unref (metadata_provider);
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
	gboolean                thread_done;

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
	rmd->cancellable = cancellable;
	rmd->ready_func = ready_func;
	rmd->user_data = user_data;
	rmd->rmtd = rmtd;
	rmd->thread = g_thread_create (read_metadata_thread, rmtd, FALSE, NULL);
	rmd->check_id = g_timeout_add (CHECK_THREAD_RATE, check_read_metadata_thread, rmd);
}


/* -- _g_write_metadata_async -- */


typedef struct {
	GList        *files;
	char         *attributes;
	char        **attributes_v;
	GMutex       *mutex;
	gboolean      thread_done;
	GError       *error;
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
	GList                   *scan;

	for (scan = gth_main_get_all_metadata_providers (); scan; scan = scan->next) {
		GthMetadataProvider *metadata_provider;

		metadata_provider = g_object_new (G_OBJECT_TYPE (scan->data), NULL);

		if (gth_metadata_provider_can_write (metadata_provider, wmtd->attributes_v)) {
			GList *scan_files;

			for (scan_files = wmtd->files; scan_files; scan_files = scan_files->next) {
				GthFileData *file_data = scan_files->data;
				gth_metadata_provider_write (metadata_provider, file_data, wmtd->attributes);
			}
		}

		g_object_unref (metadata_provider);
	}

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
_g_write_metadata_async (GList         *files, /* GthFileData * list */
			 const char    *attributes,
			 GCancellable  *cancellable,
			 ReadyFunc      ready_func,
			 gpointer       user_data)
{
	WriteMetadataData       *wmd;
	WriteMetadataThreadData *wmtd;

	wmtd = g_new0 (WriteMetadataThreadData, 1);
	wmtd->files = _g_object_list_ref (files);
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
