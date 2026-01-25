#include <config.h>
#include <jxl/decode.h>
#include <jxl/thread_parallel_runner.h>
#if HAVE_LCMS2
#include <lcms2.h>
#endif
#include "lib/gth-icc-profile.h"
#include "image-info.h"
#include "load-jxl.h"

static void premultiply_alpha (int width, int height, guchar *buffer) {
	guchar *p = buffer;
	guchar r, g, b; // used in RGBA_TO_PIXEL
	guint temp; // used in RGBA_TO_PIXEL
	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			RGBA_TO_PIXEL (p, p[0], p[1], p[2], p[3]);
			p += 4;
		}
	}
}

GthImage* load_jxl (GBytes *bytes, guint requested_size, GCancellable *cancellable, GError **error) {
	JxlDecoder *dec = JxlDecoderCreate (NULL);
	if (dec == NULL) {
		g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_FAILED, "Could not create JXL decoder.");
		return NULL;
	}

	gsize buffer_size;
	gconstpointer buffer = g_bytes_get_data (bytes, &buffer_size);

	if (JxlSignatureCheck (buffer, buffer_size) < JXL_SIG_CODESTREAM) {
		g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_FAILED, "Signature does not match for JPEG XL codestream or container.");
		JxlDecoderDestroy (dec);
		return NULL;
	}

	void *runner = JxlThreadParallelRunnerCreate (NULL, JxlThreadParallelRunnerDefaultNumWorkerThreads ());
	if (runner == NULL) {
		g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_FAILED, "Could not create threaded parallel runner.");
		JxlDecoderDestroy (dec);
		return NULL;
	}

	if (JxlDecoderSetParallelRunner (dec, JxlThreadParallelRunner, runner) != JXL_DEC_SUCCESS) {
		g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_FAILED, "Could not set parallel runner.");
		JxlThreadParallelRunnerDestroy (runner);
		JxlDecoderDestroy (dec);
		return NULL;
	}

	if (JxlDecoderSubscribeEvents (dec, JXL_DEC_BASIC_INFO
		| JXL_DEC_COLOR_ENCODING
		| JXL_DEC_FRAME
		| JXL_DEC_FULL_IMAGE) != JXL_DEC_SUCCESS)
	{
		g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_FAILED, "Could not subscribe to decoder events.");
		JxlThreadParallelRunnerDestroy (runner);
		JxlDecoderDestroy (dec);
		return NULL;
	}

	if (JxlDecoderSetInput (dec, buffer, buffer_size) != JXL_DEC_SUCCESS) {
		g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_FAILED, "Could not set decoder input.");
		JxlThreadParallelRunnerDestroy (runner);
		JxlDecoderDestroy (dec);
		return NULL;
	}

	GthImage *image = NULL;
	int width = 0;
	int height = 0;
	unsigned char *surface_data = NULL;
	gsize surface_size = 0;

	JxlBasicInfo info;
	JxlFrameHeader frame_header;

	JxlPixelFormat pixel_format;
	pixel_format.num_channels = 4;
	pixel_format.data_type = JXL_TYPE_UINT8;
	pixel_format.endianness = JXL_NATIVE_ENDIAN;
	pixel_format.align = 0;

	JxlDecoderStatus status;
	do {
		status = JxlDecoderProcessInput (dec);

		switch (status) {
		case JXL_DEC_ERROR:
			g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_FAILED, "jxl: decoder error.");
			break;

		case JXL_DEC_NEED_MORE_INPUT:
			g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_FAILED, "jxl: reached end of file but decoder still wants more.\n");
			status = JXL_DEC_ERROR;
			break;

		case JXL_DEC_BASIC_INFO:
			if (JxlDecoderGetBasicInfo (dec, &info) != JXL_DEC_SUCCESS) {
				g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_FAILED, "jxl: could not get basic info from decoder.");
				status = JXL_DEC_ERROR;
				break;
			}
			width = info.xsize;
			height = info.ysize;
			// TODO: read info.orientation
			break;

#if HAVE_LCMS2
		case JXL_DEC_COLOR_ENCODING:
			gsize profile_size;
			if (JxlDecoderGetICCProfileSize (dec, JXL_COLOR_PROFILE_TARGET_DATA, &profile_size) != JXL_DEC_SUCCESS) {
				// g_message ("jxl: could not get ICC profile size.\n");
				break;
			}

			guchar *profile_data = g_new (guchar, profile_size);
			if (JxlDecoderGetColorAsICCProfile (dec, JXL_COLOR_PROFILE_TARGET_DATA, profile_data, profile_size) != JXL_DEC_SUCCESS) {
				// g_message ("jxl: could not get ICC profile.\n");
				g_free (profile_data);
				break;
			}

			GBytes *bytes = g_bytes_new_take (profile_data, profile_size);
			GthIccProfile *profile = gth_icc_profile_new_from_bytes (bytes, NULL);
			if (profile != NULL) {
				if (image != NULL) {
					gth_image_set_icc_profile (image, profile);
				}
				g_object_unref (profile);
			}
			g_bytes_unref (bytes);
			break;
#endif

		case JXL_DEC_NEED_IMAGE_OUT_BUFFER:
			// g_print ("> JXL_DEC_NEED_IMAGE_OUT_BUFFER\n");
			if (image == NULL) {
				image = gth_image_new ((guint) width, (guint) height);
				gth_image_set_has_alpha (image, info.num_extra_channels > 0);
				surface_data = gth_image_get_pixels (image, &surface_size);
			}
			else {
				if (gth_image_get_frames (image) == 0) {
					// Use the static image as first frame.
					GthImage *first_frame = gth_image_new_as_frame (image);
					gth_image_add_frame (image, first_frame, 0);
					g_object_unref (first_frame);
				}
				GthImage *frame = gth_image_new ((guint) width, (guint) height);
				gth_image_set_has_alpha (frame, info.num_extra_channels > 0);
				guint delay = (guint) ((double) frame_header.duration * info.animation.tps_numerator / info.animation.tps_denominator / 10);
				gth_image_add_frame (image, frame, delay);
				surface_data = gth_image_get_pixels (frame, &surface_size);
				g_object_unref (frame);
			}
			if (JxlDecoderSetImageOutBuffer (dec, &pixel_format, surface_data, surface_size) != JXL_DEC_SUCCESS) {
				g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_FAILED, "jxl: could not set image out-buffer.");
				status = JXL_DEC_ERROR;
			}
			break;

		case JXL_DEC_FRAME:
			if (JxlDecoderGetFrameHeader (dec, &frame_header) != JXL_DEC_SUCCESS) {
				g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_FAILED, "jxl: could not get the frame header.");
				status = JXL_DEC_ERROR;
			}
			break;

		case JXL_DEC_FULL_IMAGE:
			// g_print ("> JXL_DEC_FULL_IMAGE\n");
			if (surface_data != NULL) {
				premultiply_alpha (width, height, surface_data);
			}
			break;

		default:
			break;
		}
	}
	while ((status != JXL_DEC_ERROR) && (status != JXL_DEC_SUCCESS));

	if (status == JXL_DEC_ERROR) {
		if (image != NULL) {
			g_object_unref (image);
			image = NULL;
		}
	}
	JxlThreadParallelRunnerDestroy (runner);
	JxlDecoderDestroy (dec);
	return image;
}

gboolean load_jxl_info (GInputStream *stream, GthImageInfo *image_info, guchar *buffer, gsize buffer_size, GCancellable *cancellable) {
	JxlSignature sig = JxlSignatureCheck (buffer, buffer_size);
	if (sig <= JXL_SIG_INVALID) {
		return FALSE;
	}

	JxlDecoder *dec = JxlDecoderCreate (NULL);
	if (dec == NULL) {
		return FALSE;
	}

	if (JxlDecoderSubscribeEvents (dec, JXL_DEC_BASIC_INFO) != JXL_DEC_SUCCESS) {
		JxlDecoderDestroy (dec);
		return FALSE;
	}

	if (JxlDecoderSetInput (dec, buffer, buffer_size) != JXL_DEC_SUCCESS) {
		JxlDecoderDestroy (dec);
		return FALSE;
	}

	gboolean success = FALSE;
	JxlDecoderStatus status = JXL_DEC_NEED_MORE_INPUT;
	guchar *bigger_buffer = NULL;
	size_t bigger_size;
	while (status != JXL_DEC_SUCCESS) {
		status = JxlDecoderProcessInput (dec);
		if (status == JXL_DEC_ERROR) {
			break;
		}
		else if (status == JXL_DEC_NEED_MORE_INPUT) {
			size_t unprocessed = JxlDecoderReleaseInput (dec);
			if (unprocessed > 0) {
				guchar *prev_buffer;
				size_t prev_size;
				if (bigger_buffer != NULL) {
					prev_buffer = bigger_buffer;
					prev_size = bigger_size;
				}
				else {
					prev_buffer = buffer;
					prev_size = buffer_size;
				}
				bigger_size = unprocessed + prev_size;
				bigger_buffer = g_new (guchar, bigger_size);
				size_t processed = prev_size - unprocessed;
				memcpy (bigger_buffer, prev_buffer + processed, unprocessed);
				if (prev_buffer != buffer) {
					g_free (prev_buffer);
				}
			}
			else {
				if (bigger_buffer == NULL) {
					bigger_size = buffer_size;
					bigger_buffer = g_new (guchar, bigger_size);
				}
			}
			gsize read_bytes;
			if (!g_input_stream_read_all (G_INPUT_STREAM (stream),
				bigger_buffer + unprocessed,
				bigger_size - unprocessed,
				&read_bytes,
				cancellable, NULL))
			{
				break;
			}
			read_bytes += unprocessed;
			if (JxlDecoderSetInput (dec, bigger_buffer, read_bytes) != JXL_DEC_SUCCESS) {
				break;
			}
		}
		else if (status == JXL_DEC_BASIC_INFO) {
			JxlBasicInfo info;
			if (JxlDecoderGetBasicInfo (dec, &info) == JXL_DEC_SUCCESS) {
				image_info->width = info.xsize;
				image_info->height = info.ysize;
				success = TRUE;
			}
			break;
		}
	}
	if (bigger_buffer != NULL) {
		g_free (bigger_buffer);
	}
	JxlDecoderDestroy (dec);

	return success;
}
