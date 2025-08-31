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


struct _GthMetadataProviderExiv2Private {
	GSettings *general_settings;
};


G_DEFINE_TYPE_WITH_CODE (GthMetadataProviderExiv2,
			 gth_metadata_provider_exiv2,
			 GTH_TYPE_METADATA_PROVIDER,
			 G_ADD_PRIVATE (GthMetadataProviderExiv2))


static void
gth_metadata_provider_exiv2_finalize (GObject *object)
{
	GthMetadataProviderExiv2 *self;

	self = GTH_METADATA_PROVIDER_EXIV2 (object);

	_g_object_unref (self->priv->general_settings);

	G_OBJECT_CLASS (gth_metadata_provider_exiv2_parent_class)->finalize (object);
}


static gboolean
gth_metadata_provider_exiv2_can_read (GthMetadataProvider  *self,
				      GthFileData          *file_data,
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
	if (! exiv2_can_write (mime_type))
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
gth_metadata_provider_exiv2_read (GthMetadataProvider *base,
				  GthFileData         *file_data,
				  const char          *attributes,
				  GCancellable        *cancellable)
{
	GFile *sidecar;
	GthFileData *sidecar_file_data;

	if (! g_content_type_is_a (gth_file_data_get_mime_type (file_data), "image/*"))
		return;

	/* This function is executed in a secondary thread, so calling
	 * slow sync functions is not a problem. */

	guint8 *buffer;
	gsize size;
	if (! _g_file_load_in_buffer (file_data->file, (void **) &buffer, &size, cancellable, NULL)) {
		return;
	}

	exiv2_read_metadata_from_buffer (
		buffer,
		size,
		file_data->info,
		TRUE,
		NULL);

	g_free (buffer);

	/* Sidecar data */

	sidecar = exiv2_get_sidecar (file_data->file);
	sidecar_file_data = gth_file_data_new (sidecar, NULL);
	if (g_file_query_exists (sidecar_file_data->file, cancellable)) {
		if (g_file_query_exists (sidecar_file_data->file, cancellable)) {
			exiv2_read_sidecar (sidecar_file_data->file,
					    file_data->info,
					    TRUE);
		}
	}

	g_object_unref (sidecar_file_data);
	g_object_unref (sidecar);
}


static void
gth_metadata_provider_exiv2_write (GthMetadataProvider   *base,
				   GthMetadataWriteFlags  flags,
				   GthFileData           *file_data,
				   const char            *attributes,
				   GCancellable          *cancellable)
{
	GthMetadataProviderExiv2 *self = GTH_METADATA_PROVIDER_EXIV2 (base);
	void                     *buffer = NULL;
	gsize                     size;
	GError                   *error = NULL;

	if (self->priv->general_settings == NULL)
		self->priv->general_settings = g_settings_new (GTHUMB_GENERAL_SCHEMA);

	if (! (flags & GTH_METADATA_WRITE_FORCE_EMBEDDED)
	    && ! g_settings_get_boolean (self->priv->general_settings, PREF_GENERAL_STORE_METADATA_IN_FILES))
		return;

	if (! exiv2_can_write (gth_file_data_get_mime_type (file_data)))
		return;

	if (! _g_file_load_in_buffer (file_data->file, &buffer, &size, cancellable, &error))
		return;

	if (exiv2_write_metadata_to_buffer (&buffer,
					    &size,
					    file_data->info,
					    NULL,
					    &error))
	{
		_g_file_write (file_data->file,
			       FALSE,
			       G_FILE_CREATE_NONE,
			       buffer,
			       size,
			       cancellable,
			       &error);
	}

	if (buffer != NULL)
		g_free (buffer);
	g_clear_error (&error);
}


static void
gth_metadata_provider_exiv2_class_init (GthMetadataProviderExiv2Class *klass)
{
	GObjectClass             *object_class;
	GthMetadataProviderClass *mp_class;

	object_class = G_OBJECT_CLASS (klass);
	object_class->finalize = gth_metadata_provider_exiv2_finalize;

	mp_class = GTH_METADATA_PROVIDER_CLASS (klass);
	mp_class->can_read = gth_metadata_provider_exiv2_can_read;
	mp_class->can_write = gth_metadata_provider_exiv2_can_write;
	mp_class->read = gth_metadata_provider_exiv2_read;
	mp_class->write = gth_metadata_provider_exiv2_write;
}


static void
gth_metadata_provider_exiv2_init (GthMetadataProviderExiv2 *self)
{
	self->priv = gth_metadata_provider_exiv2_get_instance_private (self);
	self->priv->general_settings = NULL;
}
