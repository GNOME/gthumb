#include <libheif/heif.h>
#include "image-info.h"
#include "load-jxl.h"

GthImage* load_heif (GBytes *buffer, guint requested_size, GCancellable *cancellable, GError **error) {
	return NULL;
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
