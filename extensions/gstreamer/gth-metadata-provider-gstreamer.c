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
#include "gstreamer-utils.h"
#include "gth-metadata-provider-gstreamer.h"


#define GTH_METADATA_PROVIDER_GSTREAMER_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), GTH_TYPE_METADATA_PROVIDER_GSTREAMER, GthMetadataProviderGstreamerPrivate))


struct _GthMetadataProviderGstreamerPrivate {
	int dummy;
};


static GthMetadataProviderClass *parent_class = NULL;


static void
gth_metadata_provider_gstreamer_read (GthMetadataProvider *self,
				      GthFileData         *file_data,
				      const char          *attributes)
{
	if (! g_content_type_is_a (gth_file_data_get_mime_type (file_data), "audio/*")
	    && ! g_content_type_is_a (gth_file_data_get_mime_type (file_data), "video/*"))
	{
		return;
	}

	/* this function is executed in a secondary thread, so calling
	 * slow sync functions is not a problem. */

	gstreamer_read_metadata_from_file (file_data->file, file_data->info, NULL);
}


static void
gth_metadata_provider_gstreamer_finalize (GObject *object)
{
	/*GthMetadataProviderGstreamer *comment = GTH_METADATA_PROVIDER_GSTREAMER (object);*/

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

	g_object_set (self, "readable-attributes", "audio-video::*", NULL);

	return obj;
}


static void
gth_metadata_provider_gstreamer_class_init (GthMetadataProviderGstreamerClass *klass)
{
	parent_class = g_type_class_peek_parent (klass);
	g_type_class_add_private (klass, sizeof (GthMetadataProviderGstreamerPrivate));

	G_OBJECT_CLASS (klass)->finalize = gth_metadata_provider_gstreamer_finalize;
	G_OBJECT_CLASS (klass)->constructor = gth_metadata_provider_constructor;

	GTH_METADATA_PROVIDER_CLASS (klass)->read = gth_metadata_provider_gstreamer_read;
}


static void
gth_metadata_provider_gstreamer_init (GthMetadataProviderGstreamer *catalogs)
{
}


GType
gth_metadata_provider_gstreamer_get_type (void)
{
	static GType type = 0;

	if (! type) {
		GTypeInfo type_info = {
			sizeof (GthMetadataProviderGstreamerClass),
			NULL,
			NULL,
			(GClassInitFunc) gth_metadata_provider_gstreamer_class_init,
			NULL,
			NULL,
			sizeof (GthMetadataProviderGstreamer),
			0,
			(GInstanceInitFunc) gth_metadata_provider_gstreamer_init
		};

		type = g_type_register_static (GTH_TYPE_METADATA_PROVIDER,
					       "GthMetadataProviderGstreamer",
					       &type_info,
					       0);
	}

	return type;
}
