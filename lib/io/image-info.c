#include "image-info.h"
#include "lib/jpeg/jpeg-info.h"
#if HAVE_LIBWEBP
#include <webp/decode.h>
#endif
#if HAVE_LIBJXL
#include <lib/io/load-jxl.h>
#endif
#if HAVE_LIBHEIF
#include <lib/io/load-heif.h>
#endif

#define BUFFER_SIZE 4096

gboolean load_image_info (GFile *file, int *width, int *height, GCancellable *cancellable) {
	GFileInputStream *stream = g_file_read (file, cancellable, NULL);
	if (stream == NULL) {
		return FALSE;
	}

	int buffer_size = BUFFER_SIZE;
	guchar *buffer = g_new (guchar, buffer_size);
	gsize size;
	if (!g_input_stream_read_all (G_INPUT_STREAM (stream), buffer, buffer_size, &size, cancellable,	NULL)) {
		g_free (buffer);
		g_object_unref (stream);
		return FALSE;
	}

	GthImageInfo image_info;
	gboolean format_recognized = FALSE;
	const char *mime_type = guess_mime_type (buffer, buffer_size);

	if ((size >= 24)
		&& (g_strcmp0 (mime_type, "image/png") == 0)

		// IHDR Image header

		&& (buffer[12] == 0x49)
		&& (buffer[13] == 0x48)
		&& (buffer[14] == 0x44)
		&& (buffer[15] == 0x52))
	{
		// PNG
		image_info.width  = (buffer[16] << 24) + (buffer[17] << 16) + (buffer[18] << 8) + buffer[19];
		image_info.height = (buffer[20] << 24) + (buffer[21] << 16) + (buffer[22] << 8) + buffer[23];
		format_recognized = TRUE;
	}

	if (!format_recognized
		&& (size >= 4)
		&& (g_strcmp0 (mime_type, "image/jpeg") == 0))
	{
		// JPEG
		if (g_seekable_seek (G_SEEKABLE (stream), 0, G_SEEK_SET, cancellable, NULL)) {
			JpegInfoData jpeg_info;

			_jpeg_info_data_init (&jpeg_info);
			_jpeg_info_get_from_stream (
				G_INPUT_STREAM (stream),
				_JPEG_INFO_IMAGE_SIZE | _JPEG_INFO_EXIF_ORIENTATION,
				&jpeg_info,
				cancellable,
				NULL);

			if (jpeg_info.valid & _JPEG_INFO_IMAGE_SIZE) {
				image_info.width = jpeg_info.width;
				image_info.height = jpeg_info.height;
				format_recognized = TRUE;

				if (jpeg_info.valid & _JPEG_INFO_EXIF_ORIENTATION) {
					if ((jpeg_info.orientation == GTH_TRANSFORM_ROTATE_90)
						|| (jpeg_info.orientation == GTH_TRANSFORM_ROTATE_270)
						|| (jpeg_info.orientation == GTH_TRANSFORM_TRANSPOSE)
						|| (jpeg_info.orientation == GTH_TRANSFORM_TRANSVERSE))
					{
						int tmp = image_info.width;
						image_info.width = image_info.height;
						image_info.height = tmp;
					}
				}
			}
			_jpeg_info_data_dispose (&jpeg_info);
		}
	}

#if HAVE_LIBWEBP
	if (!format_recognized
		&& (size > 15)
		&& (g_strcmp0 (mime_type, "image/webp") == 0))
	{
		WebPDecoderConfig config;

		if (WebPInitDecoderConfig (&config)) {
			if (WebPGetFeatures (buffer, buffer_size, &config.input) == VP8_STATUS_OK) {
				image_info.width = config.input.width;
				image_info.height = config.input.height;
				format_recognized = TRUE;
			}
			WebPFreeDecBuffer (&config.output);
		}
	}
#endif /* HAVE_LIBWEBP */

#if HAVE_LIBJXL
	if (!format_recognized && (size >= 12)) {
		if (load_jxl_info (G_INPUT_STREAM (stream), &image_info, buffer, size, cancellable)) {
			format_recognized = TRUE;
		}
	}
#endif /* HAVE_LIBJXL */

#if HAVE_LIBHEIF
	if (!format_recognized
		&& (size >= 8)
		&& ((g_strcmp0 (mime_type, "image/heic") == 0)
			|| (g_strcmp0 (mime_type, "image/avif") == 0)))
	{
		if (load_heif_info (G_INPUT_STREAM (stream), &image_info, buffer, size, cancellable)) {
			format_recognized = TRUE;
		}
	}
#endif /* HAVE_LIBHEIF */

	g_free (buffer);
	g_object_unref (stream);

	if (width != NULL)
		*width = image_info.width;
	if (height != NULL)
		*height = image_info.height;

	return format_recognized;
}
