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
#include <extensions/jpeg_utils/jpeg-info.h> // For reading EXIF data
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
	gsize                      buffer_size;
	int                        width, height;
	cairo_surface_t           *surface;
	cairo_surface_metadata_t  *metadata;

	image = gth_image_new ();

	if (!WebPInitDecoderConfig (&config)) {
		return image;
	}

	if (!_g_input_stream_read_all (istream,
		(void **) &buffer,
		&buffer_size,
		cancellable,
		error))
	{
		return image;
	}

	if (WebPGetFeatures (buffer, buffer_size, &config.input) != VP8_STATUS_OK) {
		g_free (buffer);
		return image;
	}

	WebPData webp_data = { .bytes = (uint8_t*) buffer, .size = (size_t) buffer_size };
	WebPDemuxer* demux = WebPDemux (&webp_data);
	uint32_t flags = WebPDemuxGetI (demux, WEBP_FF_FORMAT_FLAGS);

	GthTransform orientation = GTH_TRANSFORM_NONE;

#if HAVE_LCMS2
	GthICCProfile *profile = NULL;

	// Read the ICC profile.
	if (flags & ICCP_FLAG) {
		WebPChunkIterator chunk_iter;
		WebPDemuxGetChunk (demux, "ICCP", 1, &chunk_iter);
		profile = gth_icc_profile_new (GTH_ICC_PROFILE_ID_UNKNOWN,
			cmsOpenProfileFromMem (chunk_iter.chunk.bytes, chunk_iter.chunk.size));
		WebPDemuxReleaseChunkIterator (&chunk_iter);
	}
#endif

	if (flags & EXIF_FLAG) {
		WebPChunkIterator chunk_iter;
		WebPDemuxGetChunk (demux, "EXIF", 1, &chunk_iter);
		JpegInfoData jpeg_info;
		_jpeg_info_data_init (&jpeg_info);
		if (_read_exif_data_tags ((guchar*) chunk_iter.chunk.bytes, chunk_iter.chunk.size,
			_JPEG_INFO_EXIF_ORIENTATION | _JPEG_INFO_ICC_PROFILE,
			&jpeg_info))
		{
#if HAVE_LCMS2
			if ((profile == NULL) && (jpeg_info.valid & _JPEG_INFO_EXIF_COLOR_SPACE)) {
				if (jpeg_info.color_space == GTH_COLOR_SPACE_SRGB) {
					profile = gth_icc_profile_new_srgb ();
				}
				else if (jpeg_info.color_space == GTH_COLOR_SPACE_ADOBERGB) {
					profile = gth_icc_profile_new_adobergb ();
				}
			}
#endif
			if (jpeg_info.valid & _JPEG_INFO_EXIF_ORIENTATION) {
				orientation = jpeg_info.orientation;
			}
		}
		WebPDemuxReleaseChunkIterator (&chunk_iter);
	}
#if HAVE_LCMS2
	if (profile != NULL) {
		gth_image_set_icc_profile (image, profile);
		g_object_unref (profile);
	}
#endif

	WebPIterator iter;
	if (!WebPDemuxGetFrame (demux, 1, &iter)) {
		g_free (buffer);
		WebPDemuxDelete (demux);
		return image;
	}

	width = iter.width;
	height = iter.height;

	surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, width, height);
	metadata = _cairo_image_surface_get_metadata (surface);
	_cairo_metadata_set_has_alpha (metadata, iter.has_alpha);

	config.options.no_fancy_upsampling = 1;
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

	WebPDecode (iter.fragment.bytes, iter.fragment.size, &config);

	WebPDemuxReleaseIterator (&iter);
	WebPDemuxDelete (demux);

	cairo_surface_mark_dirty (surface);
	if (orientation != GTH_TRANSFORM_NONE) {
		cairo_surface_t *rotated = _cairo_image_surface_transform (surface, orientation);
		cairo_surface_destroy (surface);
		surface = rotated;
		if ((orientation == GTH_TRANSFORM_ROTATE_90)
			|| (orientation == GTH_TRANSFORM_ROTATE_270)
			|| (orientation == GTH_TRANSFORM_TRANSPOSE)
			|| (orientation == GTH_TRANSFORM_TRANSVERSE))
		{
			int tmp = width;
			width = height;
			height = tmp;
		}
	}
	if (cairo_surface_status (surface) == CAIRO_STATUS_SUCCESS) {
		gth_image_set_cairo_surface (image, surface);
	}
	cairo_surface_destroy (surface);

	if (original_width != NULL)
		*original_width = width;
	if (original_height != NULL)
		*original_height = height;
	if (loaded_original != NULL)
		*loaded_original = TRUE;

	g_free (buffer);

	return image;
}
