#include <config.h>
#include <glib.h>
#include <libraw/libraw.h>
#include "load-raw.h"

class GInputStream_datastream : public LibRaw_abstract_datastream {
	public:

	GInputStream_datastream (GInputStream *_stream) {
		stream = g_object_ref (_stream);
		data_stream = g_data_input_stream_new (stream);
		calc_size = TRUE;
	}

	virtual ~GInputStream_datastream () {
		g_object_unref (stream);
		g_object_unref (data_stream);
	}

	virtual int valid () {
		int result = G_IS_SEEKABLE (stream) ? 1 : 0;
		// g_print ("> valid: %d\n", result);
		return result;
	}

	// virtual int jpeg_src(void *jpegdata);

	virtual int read (void *ptr, size_t sz, size_t nmemb) {
		size_t count = sz * nmemb;
		// g_print ("> read: %ld\n", count);
		gsize bytes_read;
		if (!g_input_stream_read_all (stream, ptr, count, &bytes_read, NULL, NULL)) {
			return 0;
		}
		return (int) (bytes_read / (sz > 0 ? sz : 1));
	}

	virtual int eof () {
		int result = tell () >= size ();
		// g_print ("> eof: %d\n", result);
		return result;
	}

	virtual int seek (INT64 offset, int whence) {
		GSeekType seek_type = (whence == SEEK_CUR) ? G_SEEK_CUR : (whence == SEEK_SET) ? G_SEEK_SET : G_SEEK_END;
		// g_print ("> seek: %lld\n", offset);
		return g_seekable_seek (G_SEEKABLE (stream), offset, seek_type, NULL, NULL);
	}

	virtual INT64 tell () {
		// g_print ("> tell\n");
		return (INT64) g_seekable_tell (G_SEEKABLE (stream));
	}

	virtual INT64 size () {
		if (calc_size) {
			gint64 prev = g_seekable_tell (G_SEEKABLE (stream));
			if (!g_seekable_seek (G_SEEKABLE (stream), 0, G_SEEK_END, NULL, NULL)) {
				stream_size = 0;
			}
			else {
				stream_size = g_seekable_tell (G_SEEKABLE (stream));
				g_seekable_seek (G_SEEKABLE (stream), prev, G_SEEK_SET, NULL, NULL);
			}
			calc_size = FALSE;
		}
		// g_print ("> size: %ld\n", stream_size);
		return (INT64) stream_size;
	}

	virtual char *gets (char *s, int sz) {
		if (sz < 1) {
			return NULL;
		}
		int chunk_size = sz;
		if (chunk_size > BUFFER_SIZE) {
			chunk_size = BUFFER_SIZE;
		}
		gint64 s_start = g_seekable_tell (G_SEEKABLE (stream));
		int bytes_copied = 0;
		while (bytes_copied < sz - 1) {
			gsize bytes_read;
			if (!g_input_stream_read_all (stream, buffer, chunk_size, &bytes_read, NULL, NULL)) {
				break;
			}
			gsize i = 0;
			while ((i < bytes_read) && (bytes_copied < sz - 1)) {
				s[i] = buffer[i];
				bytes_copied++;
				if (buffer[i] == '\n') {
					g_seekable_seek (G_SEEKABLE (stream), s_start + bytes_copied, G_SEEK_SET, NULL, NULL);
					break;
				}
				i++;
			}
		}
		s[bytes_copied] = 0;
		// g_print ("> gets: %s\n", s);
		return s;
	}

	virtual int scanf_one (const char *fmt, void *val) {
		// g_print ("> scanf_one\n");
		gint64 s_start = g_seekable_tell (G_SEEKABLE (stream));
		gsize length;
		char *data = g_data_input_stream_read_upto (data_stream, "\0 \t\n", 4, &length, NULL, NULL);
		int result = sscanf (data, fmt, val);
		if (length > MAX_SCANF_LENGTH) {
			g_seekable_seek (G_SEEKABLE (stream), s_start + MAX_SCANF_LENGTH, G_SEEK_SET, NULL, NULL);
		}
		g_free (data);
		return result;
	}

	virtual int get_char () {
		// g_print ("> get_char\n");
		return (int) g_data_input_stream_read_byte (data_stream, NULL, NULL);
	}

	private:

	static const int BUFFER_SIZE = 1024;
	static const gsize MAX_SCANF_LENGTH = 24;

	GInputStream *stream;
	GDataInputStream *data_stream;
	gint64 stream_size;
	gboolean calc_size;
	char buffer[BUFFER_SIZE];
};

static int
_libraw_progress_cb (void *callback_data, enum LibRaw_progress stage,
	int iteration, int expected)
{
	return g_cancellable_is_cancelled ((GCancellable *) callback_data) ? 1 : 0;
}

static GthImage * _libraw_read_bitmap_data (int width, int height, int colors,
	int bits, guchar *buffer, gsize buffer_size)
{
	GthImage *image = gth_image_new (width, height);
	int row_stride;
	guchar *row = gth_image_prepare_edit (image, &row_stride, NULL, NULL);
	guchar *pixel;
	guchar *buffer_p = buffer;
	guchar r, g, b; // used in RGBA_TO_PIXEL
	guint temp; // used in RGBA_TO_PIXEL
	switch (colors) {
	case 4:
		for (int i = 0; i < height; i++) {
			pixel = row;
			for (int j = 0; j < width; j++) {
				RGBA_TO_PIXEL (pixel, buffer_p[0], buffer_p[1], buffer_p[2], buffer_p[3]);
				pixel += PIXEL_BYTES;
				buffer_p += colors;
			}
			row += row_stride;
		}
		break;

	case 3:
		for (int i = 0; i < height; i++) {
			pixel = row;
			for (int j = 0; j < width; j++) {
				RGBA_TO_PIXEL (pixel, buffer_p[0], buffer_p[1], buffer_p[2], 0xff);
				pixel += PIXEL_BYTES;
				buffer_p += colors;
			}
			row += row_stride;
		}
		break;

	case 1:
		for (int i = 0; i < height; i++) {
			pixel = row;
			for (int j = 0; j < width; j++) {
				RGBA_TO_PIXEL (pixel, buffer_p[0], buffer_p[0], buffer_p[0], 0xff);
				pixel += PIXEL_BYTES;
				buffer_p += colors;
			}
			row += row_stride;
		}
		break;

	default:
		g_assert_not_reached ();
	}
	return image;
}

typedef enum {
	RAW_OUTPUT_COLOR_RAW = 0,
	RAW_OUTPUT_COLOR_SRGB = 1,
	RAW_OUTPUT_COLOR_ADOBE = 2,
	RAW_OUTPUT_COLOR_WIDE = 3,
	RAW_OUTPUT_COLOR_PROPHOTO = 4,
	RAW_OUTPUT_COLOR_XYZ = 5
} RawOutputColor;

extern "C"
GthImage* load_raw (GBytes *bytes, guint requested_size, GCancellable *cancellable,
	GError **error)
{
	auto raw_proc = new LibRaw (LIBRAW_OPTIONS_NO_DATAERR_CALLBACK);
	raw_proc->imgdata.params.output_tiff = FALSE;
	raw_proc->imgdata.params.use_camera_wb = TRUE;
	raw_proc->imgdata.rawparams.use_rawspeed = TRUE;
	raw_proc->imgdata.params.highlight = FALSE;
	raw_proc->imgdata.params.use_camera_matrix = TRUE;
	raw_proc->imgdata.params.output_color = RAW_OUTPUT_COLOR_SRGB;
	raw_proc->imgdata.params.output_bps = 8;
	// raw_proc->imgdata.params.half_size = (requested_size > 0);
	if (cancellable != NULL) {
		raw_proc->set_progress_handler (_libraw_progress_cb, cancellable);
	}

	gsize buffer_size;
	gconstpointer buffer = g_bytes_get_data (bytes, &buffer_size);
	int result;

	result = raw_proc->open_buffer (buffer, buffer_size);
	if (result != LIBRAW_SUCCESS) {
		g_set_error (error, G_IO_ERROR, G_IO_ERROR_INVALID_DATA,
			"open_buffer failed: %s", libraw_strerror (result));
		return NULL;
	}

	result = raw_proc->unpack ();
	if (result != LIBRAW_SUCCESS) {
		g_set_error (error, G_IO_ERROR, G_IO_ERROR_INVALID_DATA,
			"unpack failed: %s", libraw_strerror (result));
		return NULL;
	}

	result = raw_proc->dcraw_process ();
	if (result != LIBRAW_SUCCESS) {
		g_set_error (error, G_IO_ERROR, G_IO_ERROR_INVALID_DATA,
			"dcraw_process failed: %s", libraw_strerror (result));
		return NULL;
	}

	auto processed_image = raw_proc->dcraw_make_mem_image (&result);
	if (result != LIBRAW_SUCCESS) {
		g_set_error (error, G_IO_ERROR, G_IO_ERROR_INVALID_DATA,
			"dcraw_make_mem_image failed: %s", libraw_strerror (result));
		return NULL;
	}

	GthImage *image = NULL;

	switch (processed_image->type) {
	case LIBRAW_IMAGE_BITMAP:
		image = _libraw_read_bitmap_data (
			processed_image->width,
			processed_image->height,
			processed_image->colors,
			processed_image->bits,
			processed_image->data,
			processed_image->data_size);
		break;

	default:
		g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_INVALID_DATA, "Wrong data format");
		break;
	}

	LibRaw::dcraw_clear_mem (processed_image);

	return image;
}

extern "C"
gboolean load_raw_info (GInputStream *stream, GthImageInfo *image_info,
	GCancellable *cancellable)
{
	if (!g_seekable_seek (G_SEEKABLE (stream), 0, G_SEEK_SET, cancellable, NULL)) {
		return FALSE;
	}
	auto raw_proc = new LibRaw (LIBRAW_OPTIONS_NO_DATAERR_CALLBACK);
	auto data_stream = new GInputStream_datastream (stream);
	if (cancellable != NULL) {
		raw_proc->set_progress_handler (_libraw_progress_cb, cancellable);
	}
	int result = raw_proc->open_datastream (data_stream);
	if (result != LIBRAW_SUCCESS) {
		return FALSE;
	}
	result = raw_proc->adjust_sizes_info_only ();
	if (result != LIBRAW_SUCCESS) {
		return FALSE;
	}
	image_info->width = raw_proc->imgdata.sizes.iwidth;
	image_info->height = raw_proc->imgdata.sizes.iheight;
	// g_print ("> load_raw_info: %dx%d\n", image_info->width, image_info->height);
	return TRUE;
}
