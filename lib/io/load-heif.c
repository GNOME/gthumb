#include <config.h>
#include <libheif/heif.h>
#include <libheif/heif_sequences.h>
#include <lcms2.h>
#include "lib/gth-icc-profile.h"
#include "image-info.h"
#include "load-heif.h"

typedef struct {
	GCancellable *cancellable;
	GError **error;
} ProgressData;

static int cancel_decoding_func (void* progress_user_data) {
	ProgressData *progress_data = progress_user_data;
	if (g_cancellable_is_cancelled (progress_data->cancellable)) {
		g_set_error_literal (progress_data->error, G_IO_ERROR, G_IO_ERROR_CANCELLED, "Cancelled");
		return TRUE;
	}
	return FALSE;
}

GthImage* load_heif (GBytes *bytes, guint requested_size, GCancellable *cancellable, GError **error) {
	GthImage *image = NULL;

	ProgressData progress_data = {
		.cancellable = cancellable,
		.error = error,
	};
	heif_decoding_options *options = heif_decoding_options_alloc ();
	options->convert_hdr_to_8bit = TRUE;
	if (cancellable != NULL) {
		options->progress_user_data = &progress_data;
		options->cancel_decoding = cancel_decoding_func;
	}

	struct heif_context *ctx = heif_context_alloc ();
	gsize buffer_size;
	const void *buffer = g_bytes_get_data (bytes, &buffer_size);
	struct heif_error err = heif_context_read_from_memory_without_copy (ctx, buffer, buffer_size, NULL);
	if (err.code != heif_error_Ok) {
		g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_FAILED, err.message);
		goto stop_loading;
	}

	int tracks = heif_context_number_of_sequence_tracks (ctx);
	//g_print ("=> TRACKS: %d\n", tracks);
	if (tracks > 0) {
		heif_track *track = heif_context_get_track (ctx, 0);
		if (track != NULL) {
			GthImage *image = (GthImage *) g_object_new (GTH_TYPE_IMAGE, NULL);
			double tick_milliseconds = (double) 1000.0 / heif_track_get_timescale (track);
			while (TRUE) {
				struct heif_image *img = NULL;
				err = heif_track_decode_next_image (track,
					&img, heif_colorspace_RGB,
					heif_chroma_interleaved_RGB, options);
				if (err.code != heif_error_Ok) {
					//g_print ("  err.code != heif_error_Ok\n");
					break;
				}

				int width = heif_image_get_primary_width (img);
				int height = heif_image_get_primary_height (img);
				guint delay = (uint32_t) (tick_milliseconds * heif_image_get_duration (img));
				//g_print ("> FRAME SIZE: %u, %u\n", width, height);
				//g_print ("  DELAY: %u\n", delay);

				GthImage *frame = gth_image_new (width, height);
				if (frame == NULL) {
					//g_print ("  FRAME == NULL\n");
					heif_image_release (img);
					err.code = heif_error_Memory_allocation_error;
					break;
				}
				int data_stride;
				const uint8_t *data = heif_image_get_plane_readonly (img, heif_channel_interleaved, &data_stride);
				if (data == NULL) {
					//g_print ("  DATA == NULL\n");
					g_object_unref (frame);
					heif_image_release (img);
					err.code = heif_error_Decoder_plugin_error;
					break;
				}
				gth_image_copy_from_rgba_big_endian (frame, data, FALSE, data_stride);
				gth_image_add_frame (image, frame, delay);

				g_object_unref (frame);
				heif_image_release (img);
			}
			if (err.code != heif_error_End_of_sequence) {
				//g_print ("  err.code != heif_error_End_of_sequence\n");
				g_object_unref (image);
				image = NULL;
			}
			heif_track_release (track);
		}
	}

	if (image == NULL) {
		struct heif_image_handle *handle = NULL;
		err = heif_context_get_primary_image_handle (ctx, &handle);
		if (err.code != heif_error_Ok) {
			g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_FAILED, err.message);
			goto stop_loading;
		}

		gboolean has_alpha = heif_image_handle_has_alpha_channel (handle);

		struct heif_image *img = NULL;
		err = heif_decode_image (handle, &img, heif_colorspace_RGB,
			has_alpha ? heif_chroma_interleaved_RGBA : heif_chroma_interleaved_RGB,
			options);
		if (err.code != heif_error_Ok) {
			g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_FAILED, err.message);
			heif_image_handle_release (handle);
			goto stop_loading;
		}

		int image_width = heif_image_get_primary_width (img);
		int image_height = heif_image_get_primary_height (img);
		int data_stride;
		const uint8_t *data = heif_image_get_plane_readonly (img, heif_channel_interleaved, &data_stride);
		if (data == NULL) {
			g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_FAILED, "heif_image_get_plane_readonly failed");
			heif_image_release (img);
			heif_image_handle_release (handle);
			goto stop_loading;
		}

		image = gth_image_new (image_width, image_height);
		gth_image_set_has_alpha (image, has_alpha);
		gth_image_copy_from_rgba_big_endian (image, data, has_alpha, data_stride);

		if (heif_image_get_color_profile_type (img) != heif_color_profile_type_not_present) {
			size_t icc_data_size = heif_image_get_raw_color_profile_size (img);
			gpointer icc_data = g_malloc (icc_data_size);
			err = heif_image_get_raw_color_profile (img, icc_data);
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

		heif_image_release (img);
		heif_image_handle_release (handle);
	}

stop_loading:

	heif_decoding_options_free (options);
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
