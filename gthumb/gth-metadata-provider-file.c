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
#include <string.h>
#include <glib/gi18n.h>
#include <glib.h>
#include "glib-utils.h"
#include "gth-metadata-provider-file.h"


#define GTH_METADATA_PROVIDER_FILE_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), GTH_TYPE_METADATA_PROVIDER_COMMENT, GthMetadataProviderFilePrivate))


struct _GthMetadataProviderFilePrivate {
	int dummy;
};


static GthMetadataProviderClass *parent_class = NULL;


static void
gth_metadata_provider_file_read (GthMetadataProvider *self,
				 GthFileData         *file_data,
				 const char          *attributes)
{
	GFileAttributeMatcher *matcher;

	matcher = g_file_attribute_matcher_new (attributes);

	if (g_file_attribute_matcher_matches (matcher, "file::display-size")) {
		char *value;

		value = g_format_size_for_display (g_file_info_get_size (file_data->info));
		g_file_info_set_attribute_string (file_data->info, "file::display-size", value);

		g_free (value);
	}

	if (g_file_attribute_matcher_matches (matcher, "file::display-ctime")) {
		GTimeVal  timeval;
		char     *value;

		timeval.tv_sec = g_file_info_get_attribute_uint64 (file_data->info, "time::created");
		timeval.tv_usec = g_file_info_get_attribute_uint32 (file_data->info, "time::created-usec");

		value = _g_time_val_to_exif_date (&timeval);
		g_file_info_set_attribute_string (file_data->info, "file::display-ctime", value);

		g_free (value);
	}

	if (g_file_attribute_matcher_matches (matcher, "file::display-mtime")) {
		GTimeVal *timeval;
		char     *value;

		timeval = gth_file_data_get_modification_time (file_data);
		value = _g_time_val_to_exif_date (timeval);
		g_file_info_set_attribute_string (file_data->info, "file::display-mtime", value);

		g_free (value);
	}

	if (g_file_attribute_matcher_matches (matcher, "file::content-type")) {
		const char *value;

		value = get_static_string (g_file_info_get_content_type (file_data->info));
		if (value != NULL)
			g_file_info_set_attribute_string (file_data->info, "file::content-type", value);
	}

	g_file_attribute_matcher_unref (matcher);
}


static void
gth_metadata_provider_file_finalize (GObject *object)
{
	/*GthMetadataProviderFile *file = GTH_METADATA_PROVIDER_FILE (object);*/

	G_OBJECT_CLASS (parent_class)->finalize (object);
}


static GObject *
gth_metadata_provider_constructor (GType                  type,
				   guint                  n_construct_properties,
				   GObjectConstructParam *construct_properties)
{
	GthMetadataProviderClass *klass;
	GObjectClass     *parent_class;
	GObject          *obj;
	GthMetadataProvider      *self;

	klass = GTH_METADATA_PROVIDER_CLASS (g_type_class_peek (GTH_TYPE_METADATA_PROVIDER));
	parent_class = G_OBJECT_CLASS (g_type_class_peek_parent (klass));
	obj = parent_class->constructor (type, n_construct_properties, construct_properties);
	self = GTH_METADATA_PROVIDER (obj);

	g_object_set (self, "readable-attributes", "file::display-size,file::display-mtime,file::content-type,file::is-modified", NULL);

	return obj;
}


static void
gth_metadata_provider_file_class_init (GthMetadataProviderFileClass *klass)
{
	parent_class = g_type_class_peek_parent (klass);
	g_type_class_add_private (klass, sizeof (GthMetadataProviderFilePrivate));

	G_OBJECT_CLASS (klass)->finalize = gth_metadata_provider_file_finalize;
	G_OBJECT_CLASS (klass)->constructor = gth_metadata_provider_constructor;

	GTH_METADATA_PROVIDER_CLASS (klass)->read = gth_metadata_provider_file_read;
}


static void
gth_metadata_provider_file_init (GthMetadataProviderFile *catalogs)
{
}


GType
gth_metadata_provider_file_get_type (void)
{
	static GType type = 0;

	if (! type) {
		GTypeInfo type_info = {
			sizeof (GthMetadataProviderFileClass),
			NULL,
			NULL,
			(GClassInitFunc) gth_metadata_provider_file_class_init,
			NULL,
			NULL,
			sizeof (GthMetadataProviderFile),
			0,
			(GInstanceInitFunc) gth_metadata_provider_file_init
		};

		type = g_type_register_static (GTH_TYPE_METADATA_PROVIDER,
					       "GthMetadataProviderFile",
					       &type_info,
					       0);
	}

	return type;
}
