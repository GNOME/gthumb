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
#include <gthumb.h>
#include "exiv2-utils.h"
#include "gth-metadata-provider-exiv2.h"


static GthMetadataProviderClass *parent_class = NULL;


static gboolean
gth_metadata_provider_exiv2_can_read (GthMetadataProvider  *self,
				      const char           *mime_type,
				      char                **attribute_v)
{
	if (! _g_content_type_is_a (mime_type, "image/*"))
		return FALSE;

	return _g_file_attributes_matches_any_v ("Exif::*,"
						 "Xmp::*,"
						 "Iptc::*,"
						 "Embedded::Image::*,"
						 "Embedded::Photo::*,"
						 "general::datetime,"
						 "general::title,"
						 "general::description,"
						 "general::location,"
						 "general::tags",
					         attribute_v);
}


static gboolean
gth_metadata_provider_exiv2_can_write (GthMetadataProvider  *self,
				       const char           *mime_type,
				       char                **attribute_v)
{
	if (! exiv2_supports_writes (mime_type))
		return FALSE;

	return _g_file_attributes_matches_any_v ("Exif::*,"
						 "Xmp::*,"
						 "Iptc::*,"
						 "Embedded::Image::*,"
						 "Embedded::Photo::*,"
						 "general::datetime,"
						 "general::title,"
						 "general::description,"
						 "general::location,"
						 "general::tags",
					         attribute_v);
}


static void
gth_metadata_provider_exiv2_read (GthMetadataProvider *self,
				  GthFileData         *file_data,
				  const char          *attributes)
{
	char        *uri;
	char        *uri_wo_ext;
	char        *sidecar_uri;
	GthFileData *sidecar_file_data;

	if (! g_content_type_is_a (gth_file_data_get_mime_type (file_data), "image/*"))
		return;

	/* this function is executed in a secondary thread, so calling
	 * slow sync functions is not a problem. */

	exiv2_read_metadata_from_file (file_data->file, file_data->info, NULL);

	/* sidecar data */

	uri = g_file_get_uri (file_data->file);
	uri_wo_ext = _g_uri_remove_extension (uri);
	sidecar_uri = g_strconcat (uri_wo_ext, ".xmp", NULL);
	sidecar_file_data = gth_file_data_new_for_uri (sidecar_uri, NULL);
	if (g_file_query_exists (sidecar_file_data->file, NULL)) {
		gth_file_data_update_info (sidecar_file_data, "time::*");
		if (g_file_query_exists (sidecar_file_data->file, NULL))
			exiv2_read_sidecar (sidecar_file_data->file, file_data->info);
	}

	g_object_unref (sidecar_file_data);
	g_free (sidecar_uri);
	g_free (uri_wo_ext);
	g_free (uri);
}


static void
gth_metadata_provider_exiv2_write (GthMetadataProvider *self,
				   GthFileData         *file_data,
				   const char          *attributes)
{
	void    *buffer = NULL;
	gsize    size;
	GError  *error = NULL;
	GObject *metadata;

	if (! eel_gconf_get_boolean (PREF_STORE_METADATA_IN_FILES, TRUE))
		return;

	if (! exiv2_supports_writes (gth_file_data_get_mime_type (file_data)))
		return;

	if (! g_load_file_in_buffer (file_data->file, &buffer, &size, &error))
		return;

	metadata = g_file_info_get_attribute_object (file_data->info, "general::description");
	if (metadata != NULL) {
		g_file_info_set_attribute_object (file_data->info, "Exif::Photo::UserComment", metadata);
		g_file_info_set_attribute_object (file_data->info, "Xmp::dc::description", metadata);
		g_file_info_set_attribute_object (file_data->info, "Iptc::Application2::Headline", metadata);
	}

	metadata = g_file_info_get_attribute_object (file_data->info, "general::location");
	if (metadata != NULL) {
		g_file_info_set_attribute_object (file_data->info, "Xmp::iptc::Location", metadata);
		g_file_info_set_attribute_object (file_data->info, "Iptc::Application2::LocationName", metadata);
	}

	metadata = g_file_info_get_attribute_object (file_data->info, "general::tags");
	if (metadata != NULL) {
		GthMetadata *meta;
		char        *raw;

		meta = gth_metadata_new ();
		raw = gth_string_list_join (GTH_STRING_LIST (metadata), ", ");
		g_object_set (meta, "id", "general::tags", "raw", raw, NULL);

		g_file_info_set_attribute_object (file_data->info, "Xmp::iptc::Keywords", G_OBJECT (meta));
		g_file_info_set_attribute_object (file_data->info, "Iptc::Application2::Keywords", G_OBJECT (meta));

		g_free (raw);
		g_object_unref (meta);
	}

	metadata = g_file_info_get_attribute_object (file_data->info, "general::datetime");
	if (metadata != NULL)
		g_file_info_set_attribute_object (file_data->info, "Exif::Image::DateTime", metadata);

	if (exiv2_write_metadata_to_buffer (&buffer,
					    &size,
					    file_data->info,
					    NULL,
					    &error))
	{
		GFileInfo *tmp_info;

		g_write_file (file_data->file,
			      FALSE,
			      G_FILE_CREATE_NONE,
			      buffer,
			      size,
			      NULL,
			      &error);

		tmp_info = g_file_info_new ();
		g_file_info_set_attribute_uint64 (tmp_info,
						  G_FILE_ATTRIBUTE_TIME_MODIFIED,
						  g_file_info_get_attribute_uint64 (file_data->info, G_FILE_ATTRIBUTE_TIME_MODIFIED));
		g_file_info_set_attribute_uint32 (tmp_info,
						  G_FILE_ATTRIBUTE_TIME_MODIFIED_USEC,
						  g_file_info_get_attribute_uint32 (file_data->info, G_FILE_ATTRIBUTE_TIME_MODIFIED_USEC));
		g_file_set_attributes_from_info (file_data->file,
						 tmp_info,
						 G_FILE_QUERY_INFO_NONE,
						 NULL,
						 NULL);

		g_object_unref (tmp_info);
	}

	if (buffer != NULL)
		g_free (buffer);
	g_clear_error (&error);
}


static void
gth_metadata_provider_exiv2_class_init (GthMetadataProviderExiv2Class *klass)
{
	parent_class = g_type_class_peek_parent (klass);

	GTH_METADATA_PROVIDER_CLASS (klass)->can_read = gth_metadata_provider_exiv2_can_read;
	GTH_METADATA_PROVIDER_CLASS (klass)->can_write = gth_metadata_provider_exiv2_can_write;
	GTH_METADATA_PROVIDER_CLASS (klass)->read = gth_metadata_provider_exiv2_read;
	GTH_METADATA_PROVIDER_CLASS (klass)->write = gth_metadata_provider_exiv2_write;
}


GType
gth_metadata_provider_exiv2_get_type (void)
{
	static GType type = 0;

	if (! type) {
		GTypeInfo type_info = {
			sizeof (GthMetadataProviderExiv2Class),
			NULL,
			NULL,
			(GClassInitFunc) gth_metadata_provider_exiv2_class_init,
			NULL,
			NULL,
			sizeof (GthMetadataProviderExiv2),
			0,
			(GInstanceInitFunc) NULL
		};

		type = g_type_register_static (GTH_TYPE_METADATA_PROVIDER,
					       "GthMetadataProviderExiv2",
					       &type_info,
					       0);
	}

	return type;
}
