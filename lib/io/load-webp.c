#include <config.h>
#include <glib.h>
#include <webp/decode.h>
#include <webp/demux.h>
#if HAVE_LCMS2
#include <lcms2.h>
#endif
#include "lib/gth-icc-profile.h"
#include "load-webp.h"

#define CHUNK_SIZE (16*1024)

GthImage* load_webp (GBytes *bytes, guint requested_size, GCancellable *cancellable, GError **error) {
	WebPDecoderConfig config;
	if (!WebPInitDecoderConfig (&config)) {
		g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_FAILED, "WebPInitDecoderConfig failed");
		return NULL;
	}

	gsize buffer_size;
	gconstpointer buffer = g_bytes_get_data (bytes, &buffer_size);

	if (WebPGetFeatures (buffer, buffer_size, &config.input) != VP8_STATUS_OK) {
		g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_FAILED, "WebPGetFeatures failed");
		return NULL;
	}

	GthIccProfile *profile = NULL;

#if HAVE_LCMS2
	// Read the ICC profile.
	WebPData webp_data = { .bytes = (uint8_t*) buffer, .size = (size_t) buffer_size };
	WebPDemuxState demux_state;
	WebPDemuxer* demux = WebPDemuxPartial (&webp_data, &demux_state);
	if ((demux_state == WEBP_DEMUX_PARSED_HEADER) || (demux_state == WEBP_DEMUX_DONE)) {
		uint32_t flags = WebPDemuxGetI (demux, WEBP_FF_FORMAT_FLAGS);
		if (flags & ICCP_FLAG) {
			WebPChunkIterator chunk_iter;
			WebPDemuxGetChunk (demux, "ICCP", 1, &chunk_iter);
			GBytes *bytes = g_bytes_new (chunk_iter.chunk.bytes, chunk_iter.chunk.size);
			profile = gth_icc_profile_new_from_bytes (bytes, NULL);
			g_bytes_unref (bytes);
			WebPDemuxReleaseChunkIterator (&chunk_iter);
		}
	}
	WebPDemuxDelete (demux);
#endif

	guint natural_width = (guint) config.input.width;
	guint natural_height = (guint) config.input.height;
	guint width = natural_width;
	guint height = natural_height;

#if SCALING_WORKS
	if (requested_size > 0) {
		scale_if_larger (&width, &height, requested_size);
	}
#endif

	GthImage *image = gth_image_new (width, height);
	gth_image_set_has_alpha (image, config.input.has_alpha);
	if (profile != NULL) {
		gth_image_set_icc_profile (image, profile);
		g_object_unref (profile);
	}
	gth_image_set_natural_size (image, natural_width, natural_height);

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
	int row_stride;
	config.output.u.RGBA.rgba = (uint8_t *) gth_image_get_pixels (image, &image_size, &row_stride);
	config.output.u.RGBA.stride = row_stride;
	config.output.u.RGBA.size = (size_t) image_size;
	config.output.is_external_memory = 1;
	config.output.width = width;
	config.output.height = height;

	WebPIDecoder *idec = WebPINewDecoder (&config.output);
	if (idec == NULL) {
		g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_FAILED, "WebPINewDecoder failed");
		g_object_unref (image);
		return NULL;
	}

	size_t chunk_size = MIN (CHUNK_SIZE, buffer_size);
	size_t chunk_offset = 0;
	while (TRUE) {
		VP8StatusCode status = WebPIAppend (idec, buffer + chunk_offset, chunk_size);
		if ((status != VP8_STATUS_OK) && (status != VP8_STATUS_SUSPENDED)) {
			g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_FAILED, "WebPIAppend failed");
			g_object_unref (image);
			image = NULL;
			break;
		}
		if ((cancellable != NULL) && g_cancellable_is_cancelled (cancellable)) {
			g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_CANCELLED, "Cancelled");
			g_object_unref (image);
			image = NULL;
			break;
		}
		chunk_offset += chunk_size;
		if (chunk_offset >= buffer_size) {
			break;
		}
		if (chunk_offset + chunk_size > buffer_size) {
			chunk_size = buffer_size - chunk_offset;
		}
	}

	WebPIDelete (idec);
	WebPFreeDecBuffer (&config.output);

	return image;
}
