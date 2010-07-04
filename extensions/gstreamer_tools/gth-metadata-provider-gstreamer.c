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
#include <gthumb.h>
#include <extensions/gstreamer_utils/gstreamer-utils.h>
#include "gth-metadata-provider-gstreamer.h"


static GthMetadataProviderClass *parent_class = NULL;


static gboolean
gth_metadata_provider_gstreamer_can_read (GthMetadataProvider  *self,
				          const char           *mime_type,
				          char                **attribute_v)
{
	if (! _g_content_type_is_a (mime_type, "audio/*")
	    && ! _g_content_type_is_a (mime_type, "video/*"))
	{
		return FALSE;
	}

	return _g_file_attributes_matches_any_v ("general::title,"
						 "general::format,"
						 "general::dimensions,"
						 "frame::width,"
			 	 	 	 "frame::height,"
						 "audio-video::*",
					         attribute_v);
}


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
gth_metadata_provider_gstreamer_class_init (GthMetadataProviderGstreamerClass *klass)
{
	parent_class = g_type_class_peek_parent (klass);

	GTH_METADATA_PROVIDER_CLASS (klass)->can_read = gth_metadata_provider_gstreamer_can_read;
	GTH_METADATA_PROVIDER_CLASS (klass)->read = gth_metadata_provider_gstreamer_read;
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
			(GInstanceInitFunc) NULL
		};

		type = g_type_register_static (GTH_TYPE_METADATA_PROVIDER,
					       "GthMetadataProviderGstreamer",
					       &type_info,
					       0);
	}

	return type;
}
