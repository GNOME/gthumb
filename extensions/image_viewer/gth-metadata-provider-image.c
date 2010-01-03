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
#include <glib.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gthumb.h>
#include "gth-metadata-provider-image.h"


static GthMetadataProviderClass *parent_class = NULL;


static gboolean
gth_metadata_provider_image_can_read (GthMetadataProvider  *self,
				      const char           *mime_type,
				      char                **attribute_v)
{
	if (! _g_content_type_is_a (mime_type, "image/*"))
		return FALSE;

	return _g_file_attributes_matches_any_v ("general::format,"
			                         "general::dimensions,"
						 "image::width,"
						 "image::height",
					         attribute_v);
}


static void
gth_metadata_provider_image_read (GthMetadataProvider *self,
				  GthFileData         *file_data,
				  const char          *attributes)
{
	GdkPixbufFormat *format;
	char            *filename;
	int              width, height;

	if (! g_content_type_is_a (gth_file_data_get_mime_type (file_data), "image/*"))
		return;

	filename = g_file_get_path (file_data->file);
	format = gdk_pixbuf_get_file_info (filename, &width, &height);
	if (format != NULL) {
		char *size;

		g_file_info_set_attribute_string (file_data->info, "general::format", gdk_pixbuf_format_get_description (format));

		g_file_info_set_attribute_int32 (file_data->info, "image::width", width);
		g_file_info_set_attribute_int32 (file_data->info, "image::height", height);

		size = g_strdup_printf ("%d x %d", width, height);
		g_file_info_set_attribute_string (file_data->info, "general::dimensions", size);

		g_free (size);
	}

	g_free (filename);
}


static void
gth_metadata_provider_image_class_init (GthMetadataProviderImageClass *klass)
{
	parent_class = g_type_class_peek_parent (klass);

	GTH_METADATA_PROVIDER_CLASS (klass)->can_read = gth_metadata_provider_image_can_read;
	GTH_METADATA_PROVIDER_CLASS (klass)->read = gth_metadata_provider_image_read;
}


GType
gth_metadata_provider_image_get_type (void)
{
	static GType type = 0;

	if (! type) {
		GTypeInfo type_info = {
			sizeof (GthMetadataProviderImageClass),
			NULL,
			NULL,
			(GClassInitFunc) gth_metadata_provider_image_class_init,
			NULL,
			NULL,
			sizeof (GthMetadataProviderImage),
			0,
			(GInstanceInitFunc) NULL
		};

		type = g_type_register_static (GTH_TYPE_METADATA_PROVIDER,
					       "GthMetadataProviderImage",
					       &type_info,
					       0);
	}

	return type;
}
