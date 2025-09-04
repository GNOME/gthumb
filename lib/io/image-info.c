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

static gboolean load_image_info_from_stream (GInputStream *stream, int *width, int *height, GCancellable *cancellable) {
	int buffer_size = BUFFER_SIZE;
	guchar *buffer = g_new (guchar, buffer_size);
	gsize size;
	if (!g_input_stream_read_all (stream, buffer, buffer_size,
		&size, cancellable, NULL))
	{
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
		if (load_jpeg_info (stream, &image_info, cancellable)) {
			format_recognized = TRUE;
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
		if (load_jxl_info (stream, &image_info, buffer, size, cancellable)) {
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
		if (load_heif_info (stream, &image_info, buffer, size, cancellable)) {
			format_recognized = TRUE;
		}
	}
#endif /* HAVE_LIBHEIF */

#if HAVE_LIBTIFF
	if (!format_recognized && (g_strcmp0 (mime_type, "image/tiff") == 0)) {
		if (load_tiff_info (stream, &image_info, cancellable)) {
			format_recognized = TRUE;
		}
	}
#endif /* HAVE_LIBTIFF */

	g_free (buffer);

	if (width != NULL)
		*width = image_info.width;
	if (height != NULL)
		*height = image_info.height;

	return format_recognized;
}

gboolean load_image_info_from_bytes (GBytes *bytes, int *width, int *height, GCancellable *cancellable) {
	GInputStream *stream = g_memory_input_stream_new_from_bytes (bytes);
	gboolean result = load_image_info_from_stream (stream, width, height, cancellable);
	g_object_unref (stream);
	return result;
}

gboolean load_image_info (GFile *file, int *width, int *height, GCancellable *cancellable) {
	GFileInputStream *file_stream = g_file_read (file, cancellable, NULL);
	if (file_stream == NULL) {
		return FALSE;
	}
	GInputStream *buffered = g_buffered_input_stream_new_sized (G_INPUT_STREAM (file_stream), BUFFER_SIZE);
	gboolean result = load_image_info_from_stream (buffered, width, height, cancellable);
	g_object_unref (buffered);
	g_object_unref (file_stream);
	return result;
}
