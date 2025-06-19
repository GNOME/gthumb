#include "image-info.h"
#include "lib/jpeg/jpeg-info.h"
#if HAVE_LIBWEBP
#include <webp/decode.h>
#endif
#if HAVE_LIBJXL
#include <jxl/decode.h>
#include <jxl/thread_parallel_runner.h>
#endif
#if HAVE_LIBHEIF
#include <libheif/heif.h>
#endif

#define BUFFER_SIZE 4096

#if HAVE_LIBHEIF

struct _heif_reader_data {
	int64_t position;
	GInputStream *stream;
	GCancellable *cancellable;
};

static int64_t _heif_get_position (void* userdata) {
	struct _heif_reader_data *reader = userdata;
	//g_print ("  GET POSITION %ld\n", reader->position);
	return reader->position;
}

int _heif_read (void* data, size_t size, void* userdata) {
	//g_print ("  READ %ld, data: %p\n", size, data);
	if ((data == NULL) || (size == 0))
		return 0;
	struct _heif_reader_data *reader = userdata;
	gboolean success = g_input_stream_read_all (G_INPUT_STREAM (reader->stream), data, size, NULL, reader->cancellable, NULL);
	if (success) {
		reader->position += size;
	}
	return success ? 0 : 1;
}

int _heif_seek (int64_t position, void* userdata) {
	struct _heif_reader_data *reader = userdata;
	//g_print ("  SEEK %ld -> %ld\n", reader->position, position);
	gboolean success = g_seekable_seek (G_SEEKABLE (reader->stream), position, G_SEEK_SET, reader->cancellable, NULL);
	if (success) {
		reader->position = position;
	}
	return success ? 0 : 1;
}

enum heif_reader_grow_status _heif_wait_for_file_size (int64_t target_size, void* userdata) {
	//g_print ("  WAIT %ld\n", target_size);
	struct _heif_reader_data *reader = userdata;
	gboolean success = g_seekable_seek (G_SEEKABLE (reader->stream), target_size, G_SEEK_SET, reader->cancellable, NULL);
	if (success) {
		_heif_seek (reader->position, userdata);
	}
	return success ? heif_reader_grow_status_size_reached : heif_reader_grow_status_size_beyond_eof;
}

#endif /* HAVE_LIBHEIF */


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

	gboolean format_recognized = FALSE;
	int local_width = 0;
	int local_height = 0;

	if ((size >= 24)
		// PNG signature

		&& (buffer[0] == 0x89)
		&& (buffer[1] == 0x50)
		&& (buffer[2] == 0x4E)
		&& (buffer[3] == 0x47)
		&& (buffer[4] == 0x0D)
		&& (buffer[5] == 0x0A)
		&& (buffer[6] == 0x1A)
		&& (buffer[7] == 0x0A)

		// IHDR Image header

		&& (buffer[12] == 0x49)
		&& (buffer[13] == 0x48)
		&& (buffer[14] == 0x44)
		&& (buffer[15] == 0x52))
	{
		// PNG

		local_width  = (buffer[16] << 24) + (buffer[17] << 16) + (buffer[18] << 8) + buffer[19];
		local_height = (buffer[20] << 24) + (buffer[21] << 16) + (buffer[22] << 8) + buffer[23];
		format_recognized = TRUE;
	}

	else if ((size >= 4)
		&& (buffer[0] == 0xff)
		&& (buffer[1] == 0xd8)
		&& (buffer[2] == 0xff))
	{
		// JPEG

		JpegInfoData jpeg_info;

		_jpeg_info_data_init (&jpeg_info);

		if (g_seekable_can_seek (G_SEEKABLE (stream))) {
			g_seekable_seek (G_SEEKABLE (stream), 0, G_SEEK_SET, cancellable, NULL);
		}
		else {
			g_object_unref (stream);
			stream = g_file_read (file, cancellable, NULL);
		}

		_jpeg_info_get_from_stream (
			G_INPUT_STREAM (stream),
			_JPEG_INFO_IMAGE_SIZE | _JPEG_INFO_EXIF_ORIENTATION,
			&jpeg_info,
			cancellable,
			NULL);

		if (jpeg_info.valid & _JPEG_INFO_IMAGE_SIZE) {
			local_width = jpeg_info.width;
			local_height = jpeg_info.height;
			format_recognized = TRUE;

			if (jpeg_info.valid & _JPEG_INFO_EXIF_ORIENTATION) {
				if ((jpeg_info.orientation == GTH_TRANSFORM_ROTATE_90)
					|| (jpeg_info.orientation == GTH_TRANSFORM_ROTATE_270)
					|| (jpeg_info.orientation == GTH_TRANSFORM_TRANSPOSE)
					|| (jpeg_info.orientation == GTH_TRANSFORM_TRANSVERSE))
				{
					int tmp = local_width;
					local_width = local_height;
					local_height = tmp;
				}
			}
		}

		_jpeg_info_data_dispose (&jpeg_info);
	}
#if HAVE_LIBWEBP
	else if ((size > 15) && (memcmp (buffer + 8, "WEBPVP8", 7) == 0)) {
		WebPDecoderConfig config;

		if (WebPInitDecoderConfig (&config)) {
			if (WebPGetFeatures (buffer, buffer_size, &config.input) == VP8_STATUS_OK) {
				local_width = config.input.width;
				local_height = config.input.height;
				format_recognized = TRUE;
			}
			WebPFreeDecBuffer (&config.output);
		}
	}
#endif /* HAVE_LIBWEBP */

#if HAVE_LIBJXL
	else if (((size >= 12) && (memcmp (buffer + 4, "JXL", 3) == 0))
		|| ((size >= 3)
			&& (buffer[0] == 0xFF)
			&& (buffer[1] == 0x0A)))
	{
		JxlSignature sig = JxlSignatureCheck (buffer, size);
		if (sig > JXL_SIG_INVALID) {
			JxlDecoder *dec = JxlDecoderCreate (NULL);
			if (dec != NULL) {
				if (JxlDecoderSubscribeEvents (dec, JXL_DEC_BASIC_INFO) == JXL_DEC_SUCCESS) {
					if (JxlDecoderSetInput (dec, buffer, size) == JXL_DEC_SUCCESS) {
						JxlDecoderStatus status = JXL_DEC_NEED_MORE_INPUT;
						while (status != JXL_DEC_SUCCESS) {
							status = JxlDecoderProcessInput (dec);
							if (status == JXL_DEC_ERROR) {
								break;
							}
							else if (status == JXL_DEC_NEED_MORE_INPUT) {
								JxlDecoderReleaseInput (dec);
								if (!g_input_stream_read_all (G_INPUT_STREAM (stream), buffer, buffer_size, &size, cancellable,	NULL)) {
									break;
								}
								if (JxlDecoderSetInput (dec, buffer, size) != JXL_DEC_SUCCESS) {
									break;
								}
							}
							else if (status == JXL_DEC_BASIC_INFO) {
								JxlBasicInfo info;
								if (JxlDecoderGetBasicInfo (dec, &info) == JXL_DEC_SUCCESS) {
									local_width = info.xsize;
									local_height = info.ysize;
									format_recognized = TRUE;
								}
								break;
							}
						}
					}
				}
				JxlDecoderDestroy (dec);
			}
		}
	}
#endif /* HAVE_LIBJXL */

#if HAVE_LIBHEIF
	else if ((size >= 8) && (memcmp (buffer + 4, "ftyp", 4) == 0)) {
		// HEIF
		struct heif_reader reader = {
			.reader_api_version = 1,
			.get_position = _heif_get_position,
			.read = _heif_read,
			.seek = _heif_seek,
			.wait_for_file_size = _heif_wait_for_file_size,
		};
		struct _heif_reader_data data = {
			.position = 0,
			.stream = g_buffered_input_stream_new (G_INPUT_STREAM (stream)),
			.cancellable = cancellable,
		};
		struct heif_context *ctx = heif_context_alloc ();
		struct heif_error err = heif_context_read_from_reader (ctx, &reader, &data, NULL);
		if (err.code == heif_error_Ok) {
			struct heif_image_handle *handle;
			err = heif_context_get_primary_image_handle (ctx, &handle);
			if (err.code == heif_error_Ok) {
				local_width = heif_image_handle_get_width (handle);
				local_height = heif_image_handle_get_height (handle);
				format_recognized = TRUE;
			}
			if (handle != NULL) {
				heif_image_handle_release (handle);
			}
		}
		if (ctx != NULL) {
			heif_context_free (ctx);
		}
		g_object_unref (data.stream);
	}
#endif /* HAVE_LIBHEIF */

	g_free (buffer);
	g_object_unref (stream);

	if (width != NULL)
		*width = local_width;
	if (height != NULL)
		*height = local_height;

	return format_recognized;
}
