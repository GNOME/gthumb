#include <config.h>
#include <webp/encode.h>
#include "lib/pixel.h"
#include "save-webp.h"

static int writer_func (const uint8_t *data, size_t data_size, const WebPPicture *picture) {
	GByteArray *byte_array = picture->custom_ptr;
	g_byte_array_append (byte_array, data, (guint) data_size);
	return TRUE;
}

static gboolean import_image (WebPPicture *const picture, GthImage *image) {
	gboolean has_alpha = FALSE;
	gth_image_get_has_alpha (image, &has_alpha);
	if (has_alpha) {
		picture->colorspace |= WEBP_CSP_ALPHA_BIT;
	}
	else {
		picture->colorspace &= ~WEBP_CSP_ALPHA_BIT;
	}

	if (!WebPPictureAlloc (picture)) {
		return FALSE;
	}

	int src_stride, width, height;
	guchar *src_row = gth_image_prepare_edit (image, &src_stride, &width, &height);
	uint32_t *dest_row = picture->argb;
	int temp;
	guchar r, g, b, a;
	for (guint y = 0; y < height; y++) {
		guchar *src_pixel = src_row;
		for (guint x = 0; x < width; x++) {
			GET_PIXEL_RGBA (src_pixel, r, g, b, a);
			dest_row[x] = ((a << 24) | (r << 16) | (g << 8) | b);
			src_pixel += 4;
		}
		src_row += src_stride;
		dest_row += picture->argb_stride;
	}
	return TRUE;
}

GBytes* save_webp (GthImage *image, GthOption **options, GCancellable *cancellable, GError **error) {
	int quality = 75;
	int method = 4;
	gboolean lossless = TRUE;

	if (options != NULL) {
		for (int i = 0; options[i] != NULL; i++) {
			GthOption *option = options[i];
			switch (gth_option_get_id (option)) {
			case GTH_WEBP_OPTION_QUALITY:
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

			case GTH_WEBP_OPTION_METHOD:
				gth_option_get_int (option, &method);
				if ((quality < 0) || (quality > 6)) {
					g_set_error (error,
						G_IO_ERROR,
						G_IO_ERROR_INVALID_DATA,
						"Method must be between 0 and 6, '%d' is not allowed.",
						method);
					return NULL;
				}
				break;

			case GTH_WEBP_OPTION_LOSSLESS:
				gth_option_get_bool (option, &lossless);
				break;
			}
		}
	}

	WebPConfig config;

	if (!WebPConfigInit (&config)) {
		g_set_error (error,
			G_IO_ERROR,
			G_IO_ERROR_INVALID_DATA,
			"WebPConfigInit error");
		return NULL;
	}

	config.lossless = lossless;
	config.quality = quality;
	config.method = method;

	if (!WebPValidateConfig (&config)) {
		g_set_error (error,
			G_IO_ERROR,
			G_IO_ERROR_INVALID_DATA,
			"WebPValidateConfig error");
		return FALSE;
	}

	GByteArray *byte_array = g_byte_array_new ();

	WebPPicture picture;
	WebPPictureInit (&picture);
	picture.width = (int) gth_image_get_width (image);
	picture.height = (int) gth_image_get_height (image);
	picture.writer = writer_func;
	picture.custom_ptr = byte_array;
	picture.use_argb = TRUE;

	if (!import_image (&picture, image)) {
		WebPPictureFree (&picture);
		g_byte_array_free (byte_array, TRUE);
		g_set_error (error,
			G_IO_ERROR,
			G_IO_ERROR_INVALID_DATA,
			"Memory error");
		return NULL;
	}

	if (!WebPEncode (&config, &picture)) {
		WebPPictureFree (&picture);
		g_byte_array_free (byte_array, TRUE);
		g_set_error (error,
			G_IO_ERROR,
			G_IO_ERROR_INVALID_DATA,
			"Encoding error: %d",
			picture.error_code);
		return NULL;
	}

	WebPPictureFree (&picture);

	return g_byte_array_free_to_bytes (byte_array);
}
