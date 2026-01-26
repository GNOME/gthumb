#include <config.h>
#include <jxl/encode.h>
#include <jxl/resizable_parallel_runner.h>
#include "save-jxl.h"

GBytes* save_jxl (GthImage *image, GthOption **options, GCancellable *cancellable, GError **error) {
	JxlEncoder *enc = JxlEncoderCreate (NULL);
	if (enc == NULL) {
		g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_FAILED, "Could not create JXL encoder.");
		return NULL;
	}

	void *runner = JxlResizableParallelRunnerCreate (NULL);
	if (runner == NULL) {
		g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_FAILED, "Could not create threaded parallel runner.");
		JxlEncoderDestroy (enc);
		return NULL;
	}

	guint width = gth_image_get_width (image);
	guint height = gth_image_get_height (image);

	JxlResizableParallelRunnerSetThreads (runner,
		JxlResizableParallelRunnerSuggestThreads (width, height));

	if (JxlEncoderSetParallelRunner (enc, JxlResizableParallelRunner, runner) != JXL_ENC_SUCCESS) {
		g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_FAILED, "Could not set parallel runner.");
		JxlResizableParallelRunnerDestroy (runner);
		JxlEncoderDestroy (enc);
		return NULL;
	}

	gboolean has_alpha = gth_image_get_has_alpha_if_valid (image);

	JxlBasicInfo basic_info;
	JxlEncoderInitBasicInfo (&basic_info);
	basic_info.have_container = JXL_TRUE;
	basic_info.xsize = width;
	basic_info.ysize = height;
	basic_info.bits_per_sample = 8;
	basic_info.orientation = JXL_ORIENT_IDENTITY;
	basic_info.num_color_channels = 3;
	if (has_alpha) {
		basic_info.num_extra_channels = 1;
		basic_info.alpha_bits = 8;
	}
	else {
		basic_info.num_extra_channels = 0;
		basic_info.alpha_bits = 0;
	}

	if (JxlEncoderSetBasicInfo (enc, &basic_info) != JXL_ENC_SUCCESS) {
		g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_FAILED, "JxlEncoderSetBasicInfo failed");
		JxlResizableParallelRunnerDestroy (runner);
		JxlEncoderDestroy (enc);
		return NULL;
	}

#if HAVE_LCMS2

	GthIccProfile *icc_profile = gth_image_get_icc_profile (image);
	if (icc_profile != NULL) {
		if (gth_icc_profile_get_known_type (icc_profile) == GTH_ICC_TYPE_SRGB) {
			JxlColorEncoding color_encoding = {};
			JxlColorEncodingSetToSRGB (&color_encoding, FALSE);
			JxlEncoderSetColorEncoding (enc, &color_encoding);
		}
		else {
			GBytes *bytes = gth_icc_profile_get_bytes (icc_profile);
			if (bytes != NULL) {
				gsize profile_size;
				gconstpointer profile_data = g_bytes_get_data (bytes, &profile_size);
				if ((profile_data != NULL) && (profile_size > 0)) {
					JxlEncoderSetICCProfile (enc, profile_data, profile_size);
				}
				g_bytes_unref (bytes);
			}
		}
	}

#endif /* HAVE_LCMS2 */

	JxlEncoderFrameSettings *frame_settings = JxlEncoderFrameSettingsCreate (enc, NULL);
	if (frame_settings == NULL) {
		g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_FAILED, "JxlEncoderFrameSettingsCreate failed");
		JxlResizableParallelRunnerDestroy (runner);
		JxlEncoderDestroy (enc);
		return NULL;
	}

	// Convert to RGB(A) big endian.
	GthImage *tmp_image = gth_image_new (width, height);
	gth_image_copy_to_rgba_big_endian (image, tmp_image);

	gsize tmp_surface_size = 0;
	guchar *tmp_surface_data = gth_image_get_pixels (tmp_image, &tmp_surface_size);

	JxlPixelFormat pixel_format;
	pixel_format.num_channels = has_alpha ? 4 : 3;
	pixel_format.data_type = JXL_TYPE_UINT8;
	pixel_format.endianness = JXL_BIG_ENDIAN;
	pixel_format.align = gth_image_get_row_stride (tmp_image);

	if (JxlEncoderAddImageFrame (frame_settings, &pixel_format,
		(void*) tmp_surface_data, tmp_surface_size) != JXL_ENC_SUCCESS)
	{
		g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_FAILED, "JxlEncoderAddImageFrame failed");
		g_object_unref (tmp_image);
		JxlResizableParallelRunnerDestroy (runner);
		JxlEncoderDestroy (enc);
		return NULL;
	}
	JxlEncoderCloseInput (enc);

	size_t buffer_size = 4096;
	uint8_t *buffer = g_new (uint8_t, buffer_size);
	uint8_t *next_out = buffer;
	size_t avail_size = buffer_size;
	while (TRUE) {
		JxlEncoderStatus result = JxlEncoderProcessOutput (enc, &next_out, &avail_size);
		if (result == JXL_ENC_SUCCESS) {
			buffer_size = next_out - buffer;
			buffer = g_realloc (buffer, buffer_size);
			break;
		}
		if (result != JXL_ENC_NEED_MORE_OUTPUT) {
			g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_FAILED, "JxlEncoderProcessOutput failed");
			g_free (buffer);
			buffer = NULL;
			break;
		}
		size_t offset = next_out - buffer;
		buffer_size = buffer_size * 2;
		buffer = g_realloc (buffer, buffer_size);
		next_out = buffer + offset;
		avail_size = buffer_size - offset;
	}
	GBytes *bytes = NULL;
	if (buffer != NULL) {
		bytes = g_bytes_new_take (buffer, buffer_size);
	}
	g_object_unref (tmp_image);
	JxlResizableParallelRunnerDestroy (runner);
	JxlEncoderDestroy (enc);
	return bytes;
}
