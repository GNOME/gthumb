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
#include <gdk-pixbuf/gdk-pixbuf-io.h>
#include <gthumb.h>
#include "gth-metadata-provider-image.h"


#define GTH_METADATA_PROVIDER_IMAGE_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), GTH_TYPE_METADATA_PROVIDER_IMAGE, GthMetadataProviderImagePrivate))


struct _GthMetadataProviderImagePrivate {
	int dummy;
};


static GthMetadataProviderClass *parent_class = NULL;


static void
gth_metadata_provider_image_read (GthMetadataProvider *self,
				  GthFileData         *file_data,
				  const char          *attributes)
{
	if (_g_file_attributes_matches (attributes, "image::*")) {
		GdkPixbufFormat *format;
		char            *filename;
		int              width, height;

		filename = g_file_get_path (file_data->file);
		format = gdk_pixbuf_get_file_info (filename, &width, &height);
		if (format != NULL) {
			char *size;

			g_file_info_set_attribute_string (file_data->info, "image::format", gdk_pixbuf_format_get_description (format));

			g_file_info_set_attribute_int32 (file_data->info, "image::width", width);
			g_file_info_set_attribute_int32 (file_data->info, "image::height", height);

			size = g_strdup_printf ("%d x %d", width, height);
			g_file_info_set_attribute_string (file_data->info, "image::size", size);

			g_free (size);
		}

		g_free (filename);
	}
}


static void
gth_metadata_provider_image_finalize (GObject *object)
{
	/*GthMetadataProviderImage *image = GTH_METADATA_PROVIDER_IMAGE (object);*/

	G_OBJECT_CLASS (parent_class)->finalize (object);
}


static GObject *
gth_metadata_provider_constructor (GType                  type,
				   guint                  n_construct_properties,
				   GObjectConstructParam *construct_properties)
{
	GthMetadataProviderClass *klass;
	GObjectClass             *parent_class;
	GObject                  *obj;
	GthMetadataProvider      *self;

	klass = GTH_METADATA_PROVIDER_CLASS (g_type_class_peek (GTH_TYPE_METADATA_PROVIDER));
	parent_class = G_OBJECT_CLASS (g_type_class_peek_parent (klass));
	obj = parent_class->constructor (type, n_construct_properties, construct_properties);
	self = GTH_METADATA_PROVIDER (obj);

	g_object_set (self, "readable-attributes", "image::format,image::size,image::width,image::height", NULL);

	return obj;
}


static void
gth_metadata_provider_image_class_init (GthMetadataProviderImageClass *klass)
{
	parent_class = g_type_class_peek_parent (klass);
	g_type_class_add_private (klass, sizeof (GthMetadataProviderImagePrivate));

	G_OBJECT_CLASS (klass)->finalize = gth_metadata_provider_image_finalize;
	G_OBJECT_CLASS (klass)->constructor = gth_metadata_provider_constructor;

	GTH_METADATA_PROVIDER_CLASS (klass)->read = gth_metadata_provider_image_read;
}


static void
gth_metadata_provider_image_init (GthMetadataProviderImage *catalogs)
{
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
			(GInstanceInitFunc) gth_metadata_provider_image_init
		};

		type = g_type_register_static (GTH_TYPE_METADATA_PROVIDER,
					       "GthMetadataProviderImage",
					       &type_info,
					       0);
	}

	return type;
}
