#include "image-info.h"
#include "lib/jpeg/jpeg-info.h"

#define BUFFER_SIZE 1024

gboolean load_image_info (GFile *file, int *width, int *height, GCancellable *cancellable) {
	GFileInputStream *stream = g_file_read (file, cancellable, NULL);
	if (stream == NULL) {
		return FALSE;
	}

	int buffer_size = BUFFER_SIZE;
	guchar *buffer = g_new (guchar, buffer_size);
	gsize size;
	if (!g_input_stream_read_all (
		G_INPUT_STREAM (stream),
		buffer,
		buffer_size,
		&size,
		cancellable,
		NULL))
	{
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

	g_free (buffer);
	g_object_unref (stream);

	if (width != NULL)
		*width = local_width;
	if (height != NULL)
		*height = local_height;

	return format_recognized;
}
