/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2009 The Free Software Foundation, Inc.
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
#include <string.h>
#include "glib-utils.h"
#include "gth-duplicable.h"
#include "gth-file-data.h"


#define GTH_FILE_DATA_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), GTH_TYPE_FILE_DATA, GthFileDataPrivate))

struct _GthFileDataPrivate {
	GTimeVal  mtime;
	char     *sort_key;
};

static gpointer gth_file_data_parent_class = NULL;


static void
gth_file_data_finalize (GObject *obj)
{
	GthFileData *self = GTH_FILE_DATA (obj);

	if (self->file != NULL)
		g_object_unref (self->file);
	if (self->info != NULL)
		g_object_unref (self->info);
	g_free (self->priv->sort_key);

	G_OBJECT_CLASS (gth_file_data_parent_class)->finalize (obj);
}


static void
gth_file_data_class_init (GthFileDataClass *klass)
{
	gth_file_data_parent_class = g_type_class_peek_parent (klass);
	g_type_class_add_private (klass, sizeof (GthFileDataPrivate));

	G_OBJECT_CLASS (klass)->finalize = gth_file_data_finalize;
}


static void
gth_file_data_instance_init (GthFileData *self)
{
	self->priv = GTH_FILE_DATA_GET_PRIVATE (self);
}


static GObject *
gth_file_data_real_duplicate (GthDuplicable *base)
{
	return (GObject *) gth_file_data_dup ((GthFileData*) base);
}


static void
gth_file_data_gth_duplicable_interface_init (GthDuplicableIface *iface)
{
	iface->duplicate = gth_file_data_real_duplicate;
}


GType
gth_file_data_get_type (void)
{
	static GType gth_file_data_type_id = 0;

	if (gth_file_data_type_id == 0) {
		static const GTypeInfo g_define_type_info = {
			sizeof (GthFileDataClass),
			(GBaseInitFunc) NULL,
			(GBaseFinalizeFunc) NULL,
			(GClassInitFunc) gth_file_data_class_init,
			(GClassFinalizeFunc) NULL,
			NULL,
			sizeof (GthFileData),
			0,
			(GInstanceInitFunc) gth_file_data_instance_init,
			NULL
		};
		static const GInterfaceInfo gth_duplicable_info = {
			(GInterfaceInitFunc) gth_file_data_gth_duplicable_interface_init,
			(GInterfaceFinalizeFunc) NULL,
			NULL
		};
		gth_file_data_type_id = g_type_register_static (G_TYPE_OBJECT, "GthFileData", &g_define_type_info, 0);
		g_type_add_interface_static (gth_file_data_type_id, GTH_TYPE_DUPLICABLE, &gth_duplicable_info);
	}

	return gth_file_data_type_id;
}


GthFileData *
gth_file_data_new (GFile     *file,
		   GFileInfo *info)
{
	GthFileData *self;

	self = g_object_new (GTH_TYPE_FILE_DATA, NULL);
	gth_file_data_set_file (self, file);
	gth_file_data_set_info (self, info);

	return self;
}


GthFileData *
gth_file_data_new_for_uri (const char *uri,
			   const char *mime_type)
{
	GFile       *file;
	GthFileData *file_data;

	file = g_file_new_for_uri (uri);
	file_data = gth_file_data_new (file, NULL);
	gth_file_data_set_mime_type (file_data, mime_type);

	g_object_unref (file);

	return file_data;
}


GthFileData *
gth_file_data_dup (GthFileData *self)
{
	GthFileData *file;

	if (self == NULL)
		return NULL;

	file = g_object_new (GTH_TYPE_FILE_DATA, NULL);
	file->file = g_file_dup (self->file);
	file->info = g_file_info_dup (self->info);
	file->error = self->error;
	file->thumb_created = self->thumb_created;
	file->thumb_loaded = self->thumb_loaded;

	return file;
}


void
gth_file_data_set_file (GthFileData *self,
			GFile       *file)
{
	if (file != NULL)
		g_object_ref (file);

	if (self->file != NULL) {
		g_object_unref (self->file);
		self->file = NULL;
	}

	if (file != NULL)
		self->file = file;
}


void
gth_file_data_set_info (GthFileData *self,
			GFileInfo   *info)
{
	if (info != NULL)
		g_object_ref (info);

	if (self->info != NULL)
		g_object_unref (self->info);

	if (info != NULL)
		self->info = info;
	else
		self->info = g_file_info_new ();
}


void
gth_file_data_set_mime_type (GthFileData *self,
			     const char  *mime_type)
{
	if (mime_type != NULL)
		g_file_info_set_content_type (self->info, get_static_string (mime_type));
}


const char *
gth_file_data_get_mime_type (GthFileData *self)
{
	const char *content_type;

	if (self->info == NULL)
		return NULL;

	content_type = g_file_info_get_attribute_string (self->info, G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE);
	if (content_type == NULL)
		content_type = g_file_info_get_attribute_string (self->info, G_FILE_ATTRIBUTE_STANDARD_FAST_CONTENT_TYPE);

	return get_static_string (content_type);
}


const char *
gth_file_data_get_filename_sort_key (GthFileData *self)
{
	if (self->info == NULL)
		return NULL;

	if (self->priv->sort_key == NULL)
		self->priv->sort_key = g_utf8_collate_key_for_filename (g_file_info_get_display_name (self->info), -1);

	return self->priv->sort_key;
}


time_t
gth_file_data_get_mtime (GthFileData *self)
{
	g_file_info_get_modification_time (self->info, &self->priv->mtime);
	return (time_t) self->priv->mtime.tv_sec;
}


GTimeVal *
gth_file_data_get_modification_time (GthFileData *self)
{
	g_file_info_get_modification_time (self->info, &self->priv->mtime);
	return &self->priv->mtime;
}


gboolean
gth_file_data_is_readable (GthFileData *self)
{
	return g_file_info_get_attribute_boolean (self->info, G_FILE_ATTRIBUTE_ACCESS_CAN_READ);
}


void
gth_file_data_update_info (GthFileData *fd,
			   const char  *attributes)
{
	if (attributes == NULL)
		attributes = GFILE_STANDARD_ATTRIBUTES;
	if (fd->info != NULL)
		g_object_unref (fd->info);

	fd->info = g_file_query_info (fd->file, attributes, G_FILE_QUERY_INFO_NONE, NULL, NULL);

	if (fd->info == NULL)
		fd->info = g_file_info_new ();
}


void
gth_file_data_update_mime_type (GthFileData *fd,
				gboolean     fast)
{
	gth_file_data_set_mime_type (fd, _g_file_get_mime_type (fd->file, fast || ! g_file_is_native (fd->file)));
}


void
gth_file_data_update_all (GthFileData *fd,
			  gboolean     fast)
{
	gth_file_data_update_info (fd, NULL);
	gth_file_data_update_mime_type (fd, fast);
}


GList*
gth_file_data_list_from_uri_list (GList *list)
{
	GList *result = NULL;
	GList *scan;

	for (scan = list; scan; scan = scan->next) {
		char  *uri = scan->data;
		GFile *file;

		file = g_file_new_for_uri (uri);
		result = g_list_prepend (result, gth_file_data_new (file, NULL));
		g_object_unref (file);
	}

	return g_list_reverse (result);
}


GList*
gth_file_data_list_to_uri_list (GList *list)
{
	GList *result = NULL;
	GList *scan;

	for (scan = list; scan; scan = scan->next) {
		GthFileData *file = scan->data;
		result = g_list_prepend (result, g_file_get_uri (file->file));
	}

	return g_list_reverse (result);
}


GList *
gth_file_data_list_to_file_list (GList *list)
{
	GList *result = NULL;
	GList *scan;

	for (scan = list; scan; scan = scan->next) {
		GthFileData *file = scan->data;
		result = g_list_prepend (result, g_file_dup (file->file));
	}

	return g_list_reverse (result);
}


GList *
gth_file_data_list_find_file (GList *list,
			      GFile *file)
{
	GList *scan;

	for (scan = list; scan; scan = scan->next) {
		GthFileData *file_data = scan->data;

		if (g_file_equal (file_data->file, file))
			return scan;
	}

	return NULL;
}


GList *
gth_file_data_list_find_uri (GList      *list,
			     const char *uri)
{
	GList *scan;

	for (scan = list; scan; scan = scan->next) {
		GthFileData *file = scan->data;
		char    *file_uri;

		file_uri = g_file_get_uri (file->file);
		if (strcmp (file_uri, uri) == 0) {
			g_free (file_uri);
			return scan;
		}
		g_free (file_uri);
	}

	return NULL;
}


typedef struct {
	GthFileData     *file_data;
	GthFileDataFunc  ready_func;
	gpointer         user_data;
	GError          *error;
	guint            id;
} ReadyData;


static gboolean
exec_ready_func (gpointer user_data)
{
	ReadyData *data = user_data;

	g_source_remove (data->id);
	data->ready_func (data->file_data, data->error, data->user_data);

	_g_object_unref (data->file_data);
	g_free (data);

	return FALSE;
}


void
gth_file_data_ready_with_error (GthFileData     *file_data,
				GthFileDataFunc  ready_func,
				gpointer         user_data,
				GError          *error)
{
	ReadyData *data;

	data = g_new0 (ReadyData, 1);
	if (file_data != NULL)
		data->file_data = g_object_ref (file_data);
	data->ready_func = ready_func;
	data->user_data = user_data;
	data->error = error;
	data->id = g_idle_add (exec_ready_func, data);
}
