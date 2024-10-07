/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2012 Free Software Foundation, Inc.
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
#include <glib.h>
#include <webp/decode.h>
#include <webp/demux.h>
#if HAVE_LCMS2
#include <lcms2.h>
#endif
#include <gthumb.h>
#include "cairo-image-surface-webp.h"


#define BUFFER_SIZE (16*1024)


GthImage *
_cairo_image_surface_create_from_webp (GInputStream  *istream,
				       GthFileData   *file_data,
				       int            requested_size,
				       int           *original_width,
				       int           *original_height,
				       gboolean      *loaded_original,
				       gpointer       user_data,
				       GCancellable  *cancellable,
				       GError       **error)
{
	GthImage                  *image;
	WebPDecoderConfig          config;
	guchar                    *buffer;
	gsize                      bytes_read;
	int                        width, height;
	cairo_surface_t           *surface;
	cairo_surface_metadata_t  *metadata;
	WebPIDecoder              *idec;

	image = gth_image_new ();

	if (! WebPInitDecoderConfig (&config))
		return image;

	buffer = g_new (guchar, BUFFER_SIZE);
	if (! g_input_stream_read_all (istream,
				       buffer,
				       BUFFER_SIZE,
				       &bytes_read,
				       cancellable,
				       error))
	{
		g_free (buffer);
		return image;
	}

	if (WebPGetFeatures (buffer, bytes_read, &config.input) != VP8_STATUS_OK) {
		g_free (buffer);
		return image;
	}

#if HAVE_LCMS2
	// Read the ICC profile.
	WebPData webp_data = { .bytes = (uint8_t*) buffer, .size = (size_t) bytes_read };
	WebPDemuxState demux_state;
	WebPDemuxer* demux = WebPDemuxPartial (&webp_data, &demux_state);
	if ((demux_state == WEBP_DEMUX_PARSED_HEADER) || (demux_state == WEBP_DEMUX_DONE)) {
		uint32_t flags = WebPDemuxGetI (demux, WEBP_FF_FORMAT_FLAGS);
		if (flags & ICCP_FLAG) {
			WebPChunkIterator chunk_iter;
			WebPDemuxGetChunk (demux, "ICCP", 1, &chunk_iter);
			GthICCProfile *profile = gth_icc_profile_new (GTH_ICC_PROFILE_ID_UNKNOWN,
				cmsOpenProfileFromMem (chunk_iter.chunk.bytes,chunk_iter.chunk.size));
			if (profile != NULL) {
				gth_image_set_icc_profile (image, profile);
				g_object_unref (profile);
			}
			WebPDemuxReleaseChunkIterator (&chunk_iter);
		}
	}
	WebPDemuxDelete (demux);
#endif

	width = config.input.width;
	height = config.input.height;

	if (original_width != NULL)
		*original_width = width;
	if (original_height != NULL)
		*original_height = height;
	if (loaded_original != NULL)
		*loaded_original = TRUE;

#if SCALING_WORKS
	if (requested_size > 0)
		scale_keeping_ratio (&width, &height, requested_size, requested_size, FALSE);
#endif

	surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, width, height);
	metadata = _cairo_image_surface_get_metadata (surface);
	_cairo_metadata_set_has_alpha (metadata, config.input.has_alpha);

	config.options.no_fancy_upsampling = 1;

#if SCALING_WORKS
	if (requested_size > 0) {
		config.options.use_scaling = 1;
		config.options.scaled_width = width;
		config.options.scaled_height = height;
	}
#endif

#if G_BYTE_ORDER == G_LITTLE_ENDIAN
	config.output.colorspace = MODE_bgrA;
#elif G_BYTE_ORDER == G_BIG_ENDIAN
	config.output.colorspace = MODE_Argb;
#endif
	config.output.u.RGBA.rgba = (uint8_t *) _cairo_image_surface_flush_and_get_data (surface);
	config.output.u.RGBA.stride = cairo_image_surface_get_stride (surface);
	config.output.u.RGBA.size = cairo_image_surface_get_stride (surface) * height;
	config.output.is_external_memory = 1;
	config.output.width = width;
	config.output.height = height;

	idec = WebPINewDecoder (&config.output);
	if (idec == NULL) {
		cairo_surface_destroy (surface);
		g_free (buffer);
		return image;
	}

	while (TRUE) {
		VP8StatusCode status = WebPIAppend (idec, buffer, bytes_read);
		if ((status != VP8_STATUS_OK) && (status != VP8_STATUS_SUSPENDED))
			break;

		gssize signed_bytes_read = g_input_stream_read (istream,
							       buffer,
							       BUFFER_SIZE,
							       cancellable,
							       error);
		if (signed_bytes_read <= 0)
			break;
		bytes_read = signed_bytes_read;
	}

	cairo_surface_mark_dirty (surface);
	if (cairo_surface_status (surface) == CAIRO_STATUS_SUCCESS)
		gth_image_set_cairo_surface (image, surface);
	cairo_surface_destroy (surface);

	WebPIDelete (idec);
	WebPFreeDecBuffer (&config.output);

	g_free (buffer);

	return image;
}
