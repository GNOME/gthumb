#include <config.h>
#include <glib.h>
#include <webp/decode.h>
#include <webp/demux.h>
#include <lcms2.h>
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

	guint natural_width = WebPDemuxGetI (demux, WEBP_FF_CANVAS_WIDTH);
	guint natural_height = WebPDemuxGetI (demux, WEBP_FF_CANVAS_HEIGHT);
	guint frames = WebPDemuxGetI (demux, WEBP_FF_FRAME_COUNT);
	uint32_t background_color = WebPDemuxGetI (demux, WEBP_FF_BACKGROUND_COLOR);
	gboolean is_animated = frames > 1;
	//guint loops = WebPDemuxGetI (demux, WEBP_FF_LOOP_COUNT);

	WebPIterator iter;
	if (!WebPDemuxGetFrame (demux, 1, &iter)) {
		WebPDemuxDelete (demux);
		return NULL;
	}

	guint canvas_width = natural_width;
	guint canvas_height = natural_height;
#if SCALING_WORKS
	double scale_factor = 1.0;
	if (requested_size > 0) {
		scale_if_larger (&canvas_width, &canvas_height, requested_size);
		scale_factor = (double) canvas_width / natural_width;
	}
#endif

	GthImage *image = is_animated ? (GthImage *) g_object_new (GTH_TYPE_IMAGE, NULL) : gth_image_new (canvas_width, canvas_height);
	gth_image_set_has_alpha (image, iter.has_alpha);
	guint frame_idx = 0;
	GthImage *background = NULL;
	do {
		guint width = iter.width;
		guint height = iter.height;
#if SCALING_WORKS
		if (scale_factor < 1.0) {
			width = (guint) (scale_factor * width);
			height = (guint) (scale_factor * height);
		}
#endif
		if (!is_animated && ((width != canvas_width) || (height != canvas_height))) {
			g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_FAILED, "Wrong image size");
			WebPDemuxReleaseIterator (&iter);
			WebPFreeDecBuffer (&config.output);
			WebPDemuxDelete (demux);
			g_object_unref (image);
			return NULL;
		}

		GthImage *foreground = (frames > 1) ? gth_image_new (width, height) : g_object_ref (image);

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
		gsize foreground_size;
		config.output.u.RGBA.rgba = (uint8_t *) gth_image_get_pixels (foreground, &foreground_size);
		config.output.u.RGBA.stride = (int) gth_image_get_row_stride (foreground);
		config.output.u.RGBA.size = (size_t) foreground_size;
		config.output.is_external_memory = 1;
		config.output.width = width;
		config.output.height = height;

		if (WebPDecode (iter.fragment.bytes, iter.fragment.size, &config) != VP8_STATUS_OK) {
			g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_FAILED, "WebPDecode failed");
			g_object_unref (foreground);
			WebPDemuxReleaseIterator (&iter);
			WebPFreeDecBuffer (&config.output);
			WebPDemuxDelete (demux);
			g_object_unref (image);
			return NULL;
		}

		if (is_animated) {
			GthImage *canvas = gth_image_new (canvas_width, canvas_height);
			if (canvas == NULL) {
				g_set_error (error, G_IO_ERROR, G_IO_ERROR_FAILED,
					"Cannot allocate memory for frame %d.", frame_idx);
				g_object_unref (foreground);
				WebPDemuxReleaseIterator (&iter);
				WebPFreeDecBuffer (&config.output);
				WebPDemuxDelete (demux);
				g_object_unref (image);
				return NULL;
			}
			guint x_offset = iter.x_offset;
			guint y_offset = iter.y_offset;
#if SCALING_WORKS
			if (scale_factor < 1.0) {
				x_offset = (guint) (scale_factor * x_offset);
				y_offset = (guint) (scale_factor * y_offset);
			}
#endif
			gth_image_render_frame (canvas, background, background_color,
				foreground, x_offset, y_offset,
				(iter.blend_method == WEBP_MUX_BLEND));
			gth_image_add_frame (image, canvas, iter.duration);

			if (background != NULL) {
				g_object_unref (background);
				background = NULL;
			}
			if (iter.dispose_method == WEBP_MUX_DISPOSE_NONE) {
				background = g_object_ref (canvas);
			}

			g_object_unref (canvas);
		}
		g_object_unref (foreground);
		frame_idx++;
		if (!is_animated) {
			break;
		}
	}
	while (WebPDemuxNextFrame (&iter));

	if (background != NULL) {
		g_object_unref (background);
		background = NULL;
	}
	WebPDemuxReleaseIterator (&iter);
	WebPFreeDecBuffer (&config.output);

	// Read orientation and ICC profile.

	uint32_t flags = WebPDemuxGetI (demux, WEBP_FF_FORMAT_FLAGS);
	GthTransform orientation = GTH_TRANSFORM_NONE;
	GthIccProfile *profile = NULL;

	if (flags & ICCP_FLAG) {
		WebPChunkIterator chunk_iter;
		WebPDemuxGetChunk (demux, "ICCP", 1, &chunk_iter);
		GBytes *bytes = g_bytes_new (chunk_iter.chunk.bytes, chunk_iter.chunk.size);
		profile = gth_icc_profile_new_from_bytes (bytes, NULL);
		g_bytes_unref (bytes);
		WebPDemuxReleaseChunkIterator (&chunk_iter);
	}

	if (flags & EXIF_FLAG) {
		WebPChunkIterator chunk_iter;
		WebPDemuxGetChunk (demux, "EXIF", 1, &chunk_iter);
		JpegInfoData jpeg_info;
		_jpeg_info_data_init (&jpeg_info);
		if (_read_exif_data_tags ((guchar*) chunk_iter.chunk.bytes, chunk_iter.chunk.size,
			_JPEG_INFO_EXIF_ORIENTATION | _JPEG_INFO_ICC_PROFILE | _JPEG_INFO_EXIF_COLOR_SPACE,
			&jpeg_info))
		{
			if ((profile == NULL) && (jpeg_info.valid & _JPEG_INFO_EXIF_COLOR_SPACE)) {
				if (jpeg_info.color_space == GTH_COLOR_SPACE_SRGB) {
					profile = gth_icc_profile_new_srgb ();
				}
				else if (jpeg_info.color_space == GTH_COLOR_SPACE_ADOBERGB) {
					profile = gth_icc_profile_new_adobergb ();
				}
			}
			if ((profile == NULL) && (jpeg_info.valid & _JPEG_INFO_ICC_PROFILE)) {
				GBytes *bytes = g_bytes_new (jpeg_info.icc_data, jpeg_info.icc_data_size);
				profile = gth_icc_profile_new_from_bytes (bytes, NULL);
				g_bytes_unref (bytes);
			}
			if (jpeg_info.valid & _JPEG_INFO_EXIF_ORIENTATION) {
				orientation = jpeg_info.orientation;
			}
		}
		WebPDemuxReleaseChunkIterator (&chunk_iter);
	}

	if (profile != NULL) {
		gth_image_set_icc_profile (image, profile);
		g_object_unref (profile);
	}

	WebPDemuxDelete (demux);

	if (orientation != GTH_TRANSFORM_NONE) {
		GthImage *rotated = gth_image_apply_transform (image, orientation, cancellable);
		g_object_unref (image);
		image = rotated;
		if (image == NULL) {
			return NULL;
		}
	}

	return image;
}
