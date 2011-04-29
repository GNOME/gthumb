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
#include <setjmp.h>
#include <glib.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gthumb.h>
#if HAVE_LIBJPEG
#include <extensions/jpeg_utils/jpeg-info.h>
#endif /* HAVE_LIBJPEG */
#include "gth-metadata-provider-image.h"


static GthMetadataProviderClass *parent_class = NULL;


static gboolean
gth_metadata_provider_image_can_read (GthMetadataProvider  *self,
				      const char           *mime_type,
				      char                **attribute_v)
{
	return _g_file_attributes_matches_any_v ("general::format,"
			                         "general::dimensions,"
						 "image::width,"
						 "image::height,"
						 "frame::width,"
						 "frame::height",
					         attribute_v);
}


#define BUFFER_SIZE 32


static void
gth_metadata_provider_image_read (GthMetadataProvider *self,
				  GthFileData         *file_data,
				  const char          *attributes)
{
	gboolean          format_recognized;
	GFileInputStream *stream;
	char             *description;
	int               width;
	int               height;
	const char       *mime_type = NULL;

	format_recognized = FALSE;

	stream = g_file_read (file_data->file, NULL, NULL);
	if (stream != NULL) {
		int     buffer_size;
		guchar *buffer;
		gssize  size;

		buffer_size = BUFFER_SIZE;
		buffer = g_new (guchar, buffer_size);
		size = g_input_stream_read (G_INPUT_STREAM (stream),
					    buffer,
					    buffer_size,
					    NULL,
					    NULL);
		if (size >= 0) {
			if ((size >= 24)

			    /* PNG signature */

			    && (buffer[0] == 0x89)
			    && (buffer[1] == 0x50)
			    && (buffer[2] == 0x4E)
			    && (buffer[3] == 0x47)
			    && (buffer[4] == 0x0D)
			    && (buffer[5] == 0x0A)
			    && (buffer[6] == 0x1A)
			    && (buffer[7] == 0x0A)

			    /* IHDR Image header */

			    && (buffer[12] == 0x49)
    			    && (buffer[13] == 0x48)
    			    && (buffer[14] == 0x44)
    			    && (buffer[15] == 0x52))
			{
				/* PNG */

				width  = (buffer[16] << 24) + (buffer[17] << 16) + (buffer[18] << 8) + buffer[19];
				height = (buffer[20] << 24) + (buffer[21] << 16) + (buffer[22] << 8) + buffer[23];

				format_recognized = TRUE;
				description = "PNG";
				mime_type = "image/png";
			}

#if HAVE_LIBJPEG
			else if ((size >= 4)
				 && (buffer[0] == 0xff)
				 && (buffer[1] == 0xd8)
				 && (buffer[2] == 0xff))
			{
				/* JPEG */

				GthTransform orientation;

				if (g_seekable_can_seek (G_SEEKABLE (stream))) {
					g_seekable_seek (G_SEEKABLE (stream), 0, G_SEEK_SET, NULL, NULL);
				}
				else {
					g_object_unref (stream);
					stream = g_file_read (file_data->file, NULL, NULL);
				}

				if (_jpeg_get_image_info (G_INPUT_STREAM (stream),
							  &width,
							  &height,
							  &orientation,
							  NULL,
							  NULL))
				{
					format_recognized = TRUE;
					description = "JPEG";
					mime_type = "image/jpeg";

					if ((orientation == GTH_TRANSFORM_ROTATE_90)
					     ||	(orientation == GTH_TRANSFORM_ROTATE_270)
					     ||	(orientation == GTH_TRANSFORM_TRANSPOSE)
					     ||	(orientation == GTH_TRANSFORM_TRANSVERSE))
					{
						int tmp = width;
						width = height;
						height = tmp;
					}
				}
			}
#endif /* HAVE_LIBJPEG */
		}

		g_free (buffer);
		g_object_unref (stream);
	}

	if (! format_recognized) { /* use gdk_pixbuf_get_file_info */
		char *filename;

		filename = g_file_get_path (file_data->file);
		if (filename != NULL) {
			GdkPixbufFormat  *format;

			format = gdk_pixbuf_get_file_info (filename, &width, &height);
			if (format != NULL) {
				format_recognized = TRUE;
				description = gdk_pixbuf_format_get_description (format);
			}

			g_free (filename);
		}
	}

	if (format_recognized) {
		char *size;

		g_file_info_set_attribute_string (file_data->info, "general::format", description);

		g_file_info_set_attribute_int32 (file_data->info, "image::width", width);
		g_file_info_set_attribute_int32 (file_data->info, "image::height", height);
		g_file_info_set_attribute_int32 (file_data->info, "frame::width", width);
		g_file_info_set_attribute_int32 (file_data->info, "frame::height", height);

		if (mime_type != NULL)
			gth_file_data_set_mime_type (file_data, mime_type);

		size = g_strdup_printf (_("%d × %d"), width, height);
		g_file_info_set_attribute_string (file_data->info, "general::dimensions", size);

		g_free (size);
	}
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
