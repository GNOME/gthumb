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
	if (! g_str_equal (mime_type, "*") && ! _g_content_type_is_a (mime_type, "image/*"))
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
gth_metadata_provider_exiv2_write (GthMetadataProvider   *self,
				   GthMetadataWriteFlags  flags,
				   GthFileData           *file_data,
				   const char            *attributes)
{
	void    *buffer = NULL;
	gsize    size;
	GError  *error = NULL;
	GObject *metadata;

	if (((flags & GTH_METADATA_WRITE_FORCE_EMBEDDED) != GTH_METADATA_WRITE_FORCE_EMBEDDED) && ! eel_gconf_get_boolean (PREF_STORE_METADATA_IN_FILES, TRUE))
		return;

	if (! exiv2_supports_writes (gth_file_data_get_mime_type (file_data)))
		return;

	if (! g_load_file_in_buffer (file_data->file, &buffer, &size, &error))
		return;

	metadata = g_file_info_get_attribute_object (file_data->info, "general::description");
	if (metadata != NULL) {
		const char *tags_to_remove[] = {
			"Exif::Image::ImageDescription",
			"Xmp::tiff::ImageDescription",
			"Iptc::Application2::Headline",
			NULL
		};
		const char *tags_to_update[] = {
			"Exif::Photo::UserComment",
			"Xmp::dc::description",
			"Iptc::Application2::Caption",
			NULL
		};
		int i;

		for (i = 0; tags_to_remove[i] != NULL; i++)
			g_file_info_remove_attribute (file_data->info, tags_to_remove[i]);

		/* Remove the value type to use the default type for each field
		 * as described in exiv2_tools/main.c */

		g_object_set (metadata, "value-type", NULL, NULL);

		for (i = 0; tags_to_update[i] != NULL; i++) {
			GObject *orig_metadata;

			orig_metadata = g_file_info_get_attribute_object (file_data->info, tags_to_update[i]);
			if (orig_metadata != NULL) {
				/* keep the original value type */

				g_object_set (orig_metadata,
					      "raw", gth_metadata_get_raw (GTH_METADATA (metadata)),
					      "formatted", gth_metadata_get_formatted (GTH_METADATA (metadata)),
					      NULL);
			}
			else
				g_file_info_set_attribute_object (file_data->info, tags_to_update[i], metadata);
		}
	}
	else {
		const char *tags_to_remove[] = {
			"Exif::Image::ImageDescription",
			"Xmp::tiff::ImageDescription",
			"Iptc::Application2::Headline",
			"Exif::Photo::UserComment",
			"Xmp::dc::description",
			"Iptc::Application2::Caption",
			NULL
		};
		int i;

		for (i = 0; tags_to_remove[i] != NULL; i++)
			g_file_info_remove_attribute (file_data->info, tags_to_remove[i]);
	}

	metadata = g_file_info_get_attribute_object (file_data->info, "general::title");
	if (metadata != NULL) {
		g_object_set (metadata, "value-type", NULL, NULL);
		g_file_info_set_attribute_object (file_data->info, "Xmp::dc::title", metadata);
		g_file_info_set_attribute_object (file_data->info, "Iptc::Application2::Headline", metadata);
	}
	else {
		g_file_info_remove_attribute (file_data->info, "Xmp::dc::title");
		g_file_info_remove_attribute (file_data->info, "Iptc::Application2::Headline");
	}

	metadata = g_file_info_get_attribute_object (file_data->info, "general::location");
	if (metadata != NULL) {
		g_object_set (metadata, "value-type", NULL, NULL);
		g_file_info_set_attribute_object (file_data->info, "Xmp::iptc::Location", metadata);
		g_file_info_set_attribute_object (file_data->info, "Iptc::Application2::LocationName", metadata);
	}
	else {
		g_file_info_remove_attribute (file_data->info, "Xmp::iptc::Location");
		g_file_info_remove_attribute (file_data->info, "Iptc::Application2::LocationName");
	}

	metadata = g_file_info_get_attribute_object (file_data->info, "general::tags");
	if (metadata != NULL) {
		if (GTH_IS_METADATA (metadata))
			g_object_set (metadata, "value-type", NULL, NULL);
		g_file_info_set_attribute_object (file_data->info, "Xmp::iptc::Keywords", metadata);
		g_file_info_set_attribute_object (file_data->info, "Iptc::Application2::Keywords", metadata);
	}
	else {
		g_file_info_remove_attribute (file_data->info, "Xmp::iptc::Keywords");
		g_file_info_remove_attribute (file_data->info, "Iptc::Application2::Keywords");
	}

	metadata = g_file_info_get_attribute_object (file_data->info, "general::datetime");
	if (metadata != NULL) {
		g_object_set (metadata, "value-type", NULL, NULL);
		g_file_info_set_attribute_object (file_data->info, "Exif::Image::DateTime", metadata);
	}
	else
		g_file_info_remove_attribute (file_data->info, "Exif::Image::DateTime");

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
