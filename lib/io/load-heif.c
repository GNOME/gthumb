#include <config.h>
#include <libheif/heif.h>
#if HAVE_LCMS2
#include <lcms2.h>
#endif
#include "lib/gth-icc-profile.h"
#include "image-info.h"
#include "load-heif.h"

GthImage* load_heif (GBytes *bytes, guint requested_size, GCancellable *cancellable, GError **error) {
	gsize buffer_size;
	const void *buffer = g_bytes_get_data (bytes, &buffer_size);

	struct heif_context *ctx = heif_context_alloc ();
	struct heif_error err = heif_context_read_from_memory_without_copy (ctx, buffer, buffer_size, NULL);
	if (err.code != heif_error_Ok) {
		g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_FAILED, err.message);
		goto stop_loading;
	}

	struct heif_image_handle *handle = NULL;
	err = heif_context_get_primary_image_handle (ctx, &handle);
	if (err.code != heif_error_Ok) {
		g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_FAILED, err.message);
		goto stop_loading;
	}

	gboolean has_alpha = heif_image_handle_has_alpha_channel (handle);

	struct heif_image *img = NULL;
	err = heif_decode_image (handle, &img, heif_colorspace_RGB, has_alpha ? heif_chroma_interleaved_RGBA : heif_chroma_interleaved_RGB, NULL);
	if (err.code != heif_error_Ok) {
		g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_FAILED, err.message);
		goto stop_loading;
	}

	int image_width = heif_image_get_primary_width (img);
	int image_height = heif_image_get_primary_height (img);
	int data_stride;
	const uint8_t *data = heif_image_get_plane_readonly (img, heif_channel_interleaved, &data_stride);

	GthImage *image = gth_image_new (image_width, image_height);
	gth_image_set_has_alpha (image, has_alpha);

#if HAVE_LCMS2

	if (heif_image_handle_get_color_profile_type (handle) != heif_color_profile_type_not_present) {
		size_t icc_data_size = heif_image_handle_get_raw_color_profile_size (handle);
		gpointer icc_data = g_malloc (icc_data_size);
		err = heif_image_handle_get_raw_color_profile (handle, icc_data);
		if (err.code == heif_error_Ok) {
			GBytes *bytes = g_bytes_new_take (icc_data, icc_data_size);
			GthIccProfile *profile = gth_icc_profile_new_from_bytes (bytes, NULL);
			if (profile != NULL) {
				gth_image_set_icc_profile (image, profile);
				g_object_unref (profile);
			}
			icc_data = NULL;
			g_bytes_unref (bytes);
		}
		g_free (icc_data);
	}

#endif

	gth_image_copy_from_rgba_big_endian (image, data, has_alpha, data_stride);

stop_loading:

	if (img != NULL) {
		heif_image_release (img);
	}
	if (handle != NULL) {
		heif_image_handle_release (handle);
	}
	if (ctx != NULL) {
		heif_context_free (ctx);
	}

	return image;
}

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

static int _heif_read (void* data, size_t size, void* userdata) {
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

static int _heif_seek (int64_t position, void* userdata) {
	struct _heif_reader_data *reader = userdata;
	//g_print ("  SEEK %ld -> %ld\n", reader->position, position);
	gboolean success = g_seekable_seek (G_SEEKABLE (reader->stream), position, G_SEEK_SET, reader->cancellable, NULL);
	if (success) {
		reader->position = position;
	}
	return success ? 0 : 1;
}

static enum heif_reader_grow_status _heif_wait_for_file_size (int64_t target_size, void* userdata) {
	//g_print ("  WAIT %ld\n", target_size);
	struct _heif_reader_data *reader = userdata;
	gboolean success = g_seekable_seek (G_SEEKABLE (reader->stream), target_size, G_SEEK_SET, reader->cancellable, NULL);
	if (success) {
		_heif_seek (reader->position, userdata);
	}
	return success ? heif_reader_grow_status_size_reached : heif_reader_grow_status_size_beyond_eof;
}

gboolean load_heif_info (GInputStream *stream, GthImageInfo *image_info, guchar *buffer, gsize size, GCancellable *cancellable) {
	gboolean format_recognized = FALSE;
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
			image_info->width = heif_image_handle_get_width (handle);
			image_info->height = heif_image_handle_get_height (handle);
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
	return format_recognized;
}
