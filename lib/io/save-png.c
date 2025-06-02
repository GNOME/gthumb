#include <config.h>
#include <strings.h>
#include <png.h>
#include "lib/pixel.h"
#include "save-png.h"


typedef struct {
	GError **error;
	png_struct *png_ptr;
	png_info *png_info_ptr;
	png_text *text_ptr;
	GByteArray *buffer_data;
	guchar *line_buffer;
} SaverData;


static void saver_data_destroy (SaverData *saver_data) {
	png_destroy_write_struct (&saver_data->png_ptr, &saver_data->png_info_ptr);
	if (saver_data->buffer_data != NULL) {
		g_byte_array_free (saver_data->buffer_data, TRUE);
		saver_data->buffer_data = NULL;
	}
	if (saver_data->text_ptr != NULL) {
		g_free (saver_data->text_ptr);
		saver_data->text_ptr = NULL;
	}
	if (saver_data->line_buffer != NULL) {
		g_free (saver_data->line_buffer);
		saver_data->line_buffer = NULL;
	}
}


static void error_func (png_structp png_ptr, png_const_charp message) {
	GError ***error_p = png_get_error_ptr (png_ptr);
	GError  **error = *error_p;

	if ((error != NULL) && (*error == NULL)) {
		g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_INVALID_DATA, message);
	}
	longjmp (png_jmpbuf(png_ptr), 1);
}


static void
warning_func (png_structp png_ptr, png_const_charp message) {
	// Ignored
}


static void write_data_func (png_structp png_ptr, png_bytep buffer, png_size_t size) {
	SaverData *saver_data = png_get_io_ptr (png_ptr);
	g_byte_array_append (saver_data->buffer_data, (guchar*) buffer, (guint) size);
}


static void
flush_data_func (png_structp png_ptr) {
	// Not required.
}


GBytes* save_png (GthImage *image, GCancellable *cancellable, GError **error) {
	SaverData saver_data;
	saver_data.error = error;
	saver_data.png_info_ptr = NULL;
	saver_data.text_ptr = NULL;
	saver_data.buffer_data = g_byte_array_new ();
	saver_data.line_buffer = NULL;

	saver_data.png_ptr = png_create_write_struct (
		PNG_LIBPNG_VER_STRING,
		saver_data.error,
		error_func,
		warning_func);

	if (saver_data.png_ptr == NULL) {
		saver_data_destroy (&saver_data);
		return NULL;
	}

	saver_data.png_info_ptr = png_create_info_struct (saver_data.png_ptr);
	if (saver_data.png_info_ptr == NULL) {
		saver_data_destroy (&saver_data);
		return NULL;
	}

	if (setjmp (png_jmpbuf (saver_data.png_ptr))) {
		saver_data_destroy (&saver_data);
		return NULL;
	}

	png_set_write_fn (saver_data.png_ptr, &saver_data,
		write_data_func,
		flush_data_func);

	volatile int compression_level = 6; // TODO

	// Image information

	int width = (int) gth_image_get_width (image);
	int height = (int) gth_image_get_height (image);
	gboolean alpha = FALSE;
	gth_image_get_has_alpha (image, &alpha);
	png_set_IHDR (saver_data.png_ptr, saver_data.png_info_ptr,
		width,
		height,
		8,
		(alpha ? PNG_COLOR_TYPE_RGBA : PNG_COLOR_TYPE_RGB),
		PNG_INTERLACE_NONE,
		PNG_COMPRESSION_TYPE_BASE,
		PNG_FILTER_TYPE_BASE);

	// Options

	png_color_8 sig_bit;
	sig_bit.red = 8;
	sig_bit.green = 8;
	sig_bit.blue = 8;
	if (alpha)
		sig_bit.alpha = 8;
	png_set_sBIT (saver_data.png_ptr, saver_data.png_info_ptr, &sig_bit);

	png_set_compression_level (saver_data.png_ptr, compression_level);

	// Attributes

	GHashTable *attributes = gth_image_get_attributes (image);
	if (attributes != NULL) {
		int num_text = 0;
		GList *keys = NULL;
		GHashTableIter iter;
		gpointer key, value;
		g_hash_table_iter_init (&iter, attributes);
		while (g_hash_table_iter_next (&iter, &key, &value)) {
			if (g_str_has_prefix (key, "private::"))
				continue;
			keys = g_list_prepend (keys, key);
			num_text++;
		}

		if (num_text > 0) {
			saver_data.text_ptr = (png_textp) g_new (png_text, num_text);
			int idx = 0;
			for (GList *scan = keys; scan; scan = scan->next) {
				char *key = scan->data;
				char *value = g_hash_table_lookup (attributes, key);
				png_text text = {
					.compression = PNG_TEXT_COMPRESSION_NONE,
					.key = key,
					.text = value,
					.text_length = 0,
					.itxt_length = 0,
					.lang = NULL,
					.lang_key = NULL
				};
				saver_data.text_ptr[idx++] = text;
			}
			png_set_text (saver_data.png_ptr,
				saver_data.png_info_ptr,
				saver_data.text_ptr,
				num_text);
		}
	}

	// Write the file header information.

	png_write_info (saver_data.png_ptr, saver_data.png_info_ptr);

	// Write the image.

	int bpp = alpha ? 4 : 3;
	saver_data.line_buffer = g_new (guchar, width * bpp);
	int row_stride;
	guchar *pixels = gth_image_get_pixels (image, NULL, &row_stride);
	guchar *row_ptr = pixels;
	if (alpha) {
		for (int row = 0; row < height; row++) {
			pixel_line_to_rgba_big_endian (saver_data.line_buffer, row_ptr, width);
			png_write_rows (saver_data.png_ptr, &(saver_data.line_buffer), 1);
			row_ptr += row_stride;
		}
	}
	else {
		for (int row = 0; row < height; row++) {
			pixel_line_to_rgb_big_endian (saver_data.line_buffer, row_ptr, width);
			png_write_rows (saver_data.png_ptr, &(saver_data.line_buffer), 1);
			row_ptr += row_stride;
		}
	}
	png_write_end (saver_data.png_ptr, saver_data.png_info_ptr);

	GBytes *bytes = g_byte_array_free_to_bytes (saver_data.buffer_data);
	saver_data.buffer_data = NULL;
	saver_data_destroy (&saver_data);
	return bytes;
}
