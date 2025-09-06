#include <config.h>
#include <glib.h>
#include <webp/decode.h>
#include <webp/demux.h>
#if HAVE_LCMS2
#include <lcms2.h>
#endif
#include "lib/jpeg/jpeg-info.h"
#include "lib/gth-icc-profile.h"
#include "load-webp.h"

GthImage* load_webp (GBytes *bytes, guint requested_size, GCancellable *cancellable, GError **error) {
	WebPDecoderConfig config;
	if (!WebPInitDecoderConfig (&config)) {
		g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_FAILED, "WebPInitDecoderConfig failed");
		return NULL;
	}

	gsize buffer_size;
	gconstpointer buffer = g_bytes_get_data (bytes, &buffer_size);
	WebPData webp_data = { .bytes = (uint8_t*) buffer, .size = (size_t) buffer_size };
	WebPDemuxer *demux = WebPDemux (&webp_data);

	WebPIterator iter;
	if (!WebPDemuxGetFrame (demux, 1, &iter)) {
		WebPDemuxDelete (demux);
		return NULL;
	}

	guint natural_width = (guint) iter.width;
	guint natural_height = (guint) iter.height;
	guint width = natural_width;
	guint height = natural_height;

#if SCALING_WORKS
	if (requested_size > 0) {
		scale_if_larger (&width, &height, requested_size);
	}
#endif

	GthImage *image = gth_image_new (width, height);
	gth_image_set_has_alpha (image, iter.has_alpha);

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
	gsize image_size;
	config.output.u.RGBA.rgba = (uint8_t *) gth_image_get_pixels (image, &image_size);
	config.output.u.RGBA.stride = (int) gth_image_get_row_stride (image);
	config.output.u.RGBA.size = (size_t) image_size;
	config.output.is_external_memory = 1;
	config.output.width = width;
	config.output.height = height;

	if (WebPDecode (iter.fragment.bytes, iter.fragment.size, &config) != VP8_STATUS_OK) {
		g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_FAILED, "WebPDecode failed");
		g_object_unref (image);
		image = NULL;
	};
	WebPDemuxReleaseIterator (&iter);
	WebPFreeDecBuffer (&config.output);

	if (image == NULL) {
		WebPDemuxDelete (demux);
		return NULL;
	}

	// Read orientation and ICC profile.

	uint32_t flags = WebPDemuxGetI (demux, WEBP_FF_FORMAT_FLAGS);
	GthTransform orientation = GTH_TRANSFORM_NONE;

#if HAVE_LCMS2
	GthIccProfile *profile = NULL;

	if (flags & ICCP_FLAG) {
		WebPChunkIterator chunk_iter;
		WebPDemuxGetChunk (demux, "ICCP", 1, &chunk_iter);
		GBytes *bytes = g_bytes_new (chunk_iter.chunk.bytes, chunk_iter.chunk.size);
		profile = gth_icc_profile_new_from_bytes (bytes, NULL);
		g_bytes_unref (bytes);
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

	WebPDemuxDelete (demux);

	if (orientation != GTH_TRANSFORM_NONE) {
		GthImage *rotated = gth_image_apply_transform (image, orientation, cancellable);
		g_object_unref (image);
		image = rotated;
		if (image == NULL) {
			return NULL;
		}

		if (transformation_changes_size (orientation)) {
			guint tmp = natural_width;
			natural_width = natural_height;
			natural_height = tmp;
		}
	}
	gth_image_set_natural_size (image, natural_width, natural_height);

	return image;
}
