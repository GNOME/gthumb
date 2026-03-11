#include <config.h>
#include <libheif/heif.h>
#include "lib/pixel.h"
#include "save-avif.h"


typedef struct {
	GByteArray *byte_array;
	struct heif_error err;
} Context;


static struct heif_error write_fn (struct heif_context *ctx, const void *data, size_t size, void *userdata) {
	Context *context = userdata;

	g_byte_array_append (context->byte_array, data, (guint) size);
	context->err.code = heif_error_Ok;
	context->err.subcode = heif_suberror_Unspecified;
	context->err.message = "";

	return context->err;
}


GBytes* save_avif (GthImage *image, GthOption **options, GCancellable *cancellable, GError **error) {
	int quality = 75;
	gboolean lossless = TRUE;

	if (options != NULL) {
		for (int i = 0; options[i] != NULL; i++) {
			GthOption *option = options[i];
			switch (gth_option_get_id (option)) {
			case GTH_AVIF_OPTION_QUALITY:
				gth_option_get_int (option, &quality);
				if ((quality < 0) || (quality > 100)) {
					g_set_error (error,
						G_IO_ERROR,
						G_IO_ERROR_INVALID_DATA,
						"Quality must be between 0 and 100, '%d' is not allowed.",
						quality);
					return NULL;
				}
				break;

			case GTH_AVIF_OPTION_LOSSLESS:
				gth_option_get_bool (option, &lossless);
				break;
			}
		}
	}

	GBytes *bytes = NULL;
	struct heif_context *ctx = NULL;
	struct heif_encoder *encoder = NULL;
	struct heif_image *heif_image = NULL;
	struct heif_image_handle *handle = NULL;

	ctx = heif_context_alloc ();
	heif_context_get_encoder_for_format (ctx, heif_compression_AV1, &encoder);
	heif_encoder_set_lossless (encoder, lossless);
	heif_encoder_set_lossy_quality (encoder, quality);

	gboolean has_alpha = FALSE;
	gth_image_get_has_alpha (image, &has_alpha);
	int columns = (int) gth_image_get_width (image);
	int rows = (int) gth_image_get_height (image);

	struct heif_error err;

	err = heif_image_create (
		columns,
		rows,
		heif_colorspace_RGB,
		has_alpha ? heif_chroma_interleaved_RGBA : heif_chroma_interleaved_RGB,
		&heif_image);

	if (err.code != heif_error_Ok) {
		g_set_error (error,
			G_IO_ERROR,
			G_IO_ERROR_FAILED,
			"Could not create the image: %s",
			err.message);
		goto cleanup;
	}

	err = heif_image_add_plane (heif_image, heif_channel_interleaved, columns, rows, 8);
	if (err.code != heif_error_Ok) {
		g_set_error (error,
			     G_IO_ERROR,
			     G_IO_ERROR_FAILED,
			     "Could not add plane to the image: %s",
			     err.message);
		goto cleanup;
	}

	if (gth_image_has_icc_profile (image)) {
		GthIccProfile *icc_profile = gth_image_get_icc_profile (image);
		GBytes *bytes = gth_icc_profile_get_bytes (icc_profile);
		if (bytes != NULL) {
			gsize profile_size;
			gconstpointer profile_data = g_bytes_get_data (bytes, &profile_size);
			heif_image_set_raw_color_profile (heif_image, "prof", profile_data, profile_size);
			g_bytes_unref (bytes);
		}
	}

	int in_stride;
	guchar *pixels = gth_image_prepare_edit (image, &in_stride, NULL, NULL);
	int out_stride;
	uint8_t *plane = heif_image_get_plane (heif_image, heif_channel_interleaved, &out_stride);
	while (rows > 0) {
		if (has_alpha) {
			pixel_line_to_rgba_big_endian (plane, pixels, columns);
		}
		else {
			pixel_line_to_rgb_big_endian (plane, pixels, columns);
		}
		plane += out_stride;
		pixels += in_stride;
		rows -= 1;
	}

	err = heif_context_encode_image (ctx, heif_image, encoder, NULL, &handle);
	if (err.code != heif_error_Ok) {
		g_set_error (error,
			     G_IO_ERROR,
			     G_IO_ERROR_FAILED,
			     "Could not encode the image: %s",
			     err.message);
		goto cleanup;
	}

#if GENERATE_THUMBNAIL
	struct heif_image_handle *thumbnail_handle = NULL;

	err = heif_context_encode_thumbnail (ctx, heif_image, handle, encoder, NULL, THUMBNAIL_SIZE, &thumbnail_handle);
	if (thumbnail_handle != NULL) {
		heif_image_handle_release (thumbnail_handle);
	}

	if (err.code != heif_error_Ok) {
		g_set_error (error,
			     G_IO_ERROR,
			     G_IO_ERROR_FAILED,
			     "Could not generate thumbnail: %s",
			     err.message);
		goto cleanup;
	}
#endif

	heif_image_handle_release (handle);
	handle = NULL;

	heif_encoder_release (encoder);
	encoder = NULL;

	// Write

	struct heif_writer writer;
	writer.writer_api_version = 1;
	writer.write = write_fn;

	Context context;
	context.byte_array = g_byte_array_new ();
	err = heif_context_write (ctx, &writer, &context);

	gboolean success = (err.code == heif_error_Ok);
	if (success) {
		bytes = g_byte_array_free_to_bytes (context.byte_array);
	}
	else {
		g_byte_array_free (context.byte_array, TRUE);
	}

cleanup:

	if (handle != NULL)
		heif_image_handle_release (handle);
	if (heif_image != NULL)
		heif_image_release (heif_image);
	if (encoder != NULL)
		heif_encoder_release (encoder);
	if (ctx != NULL)
		heif_context_free (ctx);

	return bytes;
}
