#include <config.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <setjmp.h>
#include <string.h>
#include <jpeglib.h>
#include <lib/jpeg/jmemorydest.h>
#include "lib/pixel.h"
#include "save-jpeg.h"

struct error_handler_data {
	struct jpeg_error_mgr pub;
	sigjmp_buf setjmp_buffer;
	GError **error;
};

static void fatal_error_handler (j_common_ptr cinfo) {
	struct error_handler_data *errmgr;
	errmgr = (struct error_handler_data *) cinfo->err;

	// Create the message
	char buffer[JMSG_LENGTH_MAX];
	(* cinfo->err->format_message) (cinfo, buffer);

	// Broken check for *error == NULL for robustness.
	if (errmgr->error && (*errmgr->error == NULL)) {
		g_set_error (errmgr->error,
			GDK_PIXBUF_ERROR,
			GDK_PIXBUF_ERROR_CORRUPT_IMAGE,
			"Error interpreting JPEG image file (%s)",
			buffer);
	}

	siglongjmp (errmgr->setjmp_buffer, 1);

	g_assert_not_reached ();
}

static void output_message_handler (j_common_ptr cinfo) {
	// This method keeps libjpeg from writing messages to stderr.
}

GBytes* save_jpeg (GthImage *image, GthOption **options, GCancellable *cancellable, GError **error) {
	volatile int quality = 85;
	volatile int smoothing = 0;
	volatile gboolean optimize = FALSE;
#ifdef HAVE_PROGRESSIVE_JPEG
	volatile gboolean progressive = FALSE;
#endif

	if (options != NULL) {
		for (int i = 0; options[i] != NULL; i++) {
			GthOption *option = options[i];
			switch (gth_option_get_id (option)) {
			case GTH_JPEG_OPTION_QUALITY:
				gth_option_get_int (option, &quality);
				if ((quality < 0) || (quality > 100)) {
					g_set_error (error,
						G_IO_ERROR,
						G_IO_ERROR_INVALID_DATA,
						"JPEG quality must be a value between 0 and 100; value '%d' is not allowed.",
						quality);
					return NULL;
				}
				break;

			case GTH_JPEG_OPTION_SMOOTHING:
				gth_option_get_int (option, &smoothing);
				if ((smoothing < 0) || (smoothing > 100)) {
					g_set_error (error,
						G_IO_ERROR,
						G_IO_ERROR_INVALID_DATA,
						"JPEG smoothing must be a value between 0 and 100; value '%d' is not allowed.",
						smoothing);
					return NULL;
				}
				break;

			case GTH_JPEG_OPTION_OPTIMIZE:
				gth_option_get_bool (option, &optimize);
				break;

			case GTH_JPEG_OPTION_PROGRESSIVE:
#ifdef HAVE_PROGRESSIVE_JPEG
				gth_option_get_bool (option, &progressive);
#endif
				break;
			}
		}
	}

	int width = (int) gth_image_get_width (image);
	int height = (int) gth_image_get_height (image);
	int row_stride;
	guchar *pixels = gth_image_get_pixels (image, NULL, &row_stride);

	if (pixels == NULL) {
		g_set_error_literal (error,
			G_IO_ERROR,
			G_IO_ERROR_INVALID_DATA,
			"Image data is NULL.");
		return NULL;
	}

	// Allocate a buffer to convert lines.
	guchar *line_buffer = g_try_malloc (width * 3 * sizeof (guchar));
	if (!line_buffer) {
		g_set_error_literal (error,
			G_IO_ERROR,
			G_IO_ERROR_FAILED,
			"Couldn't allocate memory for loading JPEG file");
		return FALSE;
	}

	// Set up error handling

	struct jpeg_compress_struct cinfo;
	struct error_handler_data jerr;
	cinfo.err = jpeg_std_error (&(jerr.pub));
	jerr.pub.error_exit = fatal_error_handler;
	jerr.pub.output_message = output_message_handler;
	jerr.error = error;
	if (sigsetjmp (jerr.setjmp_buffer, 1)) {
		jpeg_destroy_compress (&cinfo);
		g_free (line_buffer);
		return FALSE;
	}

	// Setup compress params.

	jpeg_create_compress (&cinfo);

	char *buffer;
	gsize buffer_size;
	_jpeg_memory_dest (&cinfo, (void**) &buffer, &buffer_size);

	cinfo.image_width = width;
	cinfo.image_height = height;
	cinfo.input_components = 3;
	cinfo.in_color_space = JCS_RGB;

	// Set up jpeg compression parameters.

	jpeg_set_defaults (&cinfo);
	jpeg_set_quality (&cinfo, quality, TRUE);
	cinfo.smoothing_factor = smoothing;
	cinfo.optimize_coding = optimize;

#ifdef HAVE_PROGRESSIVE_JPEG
	if (progressive) {
		jpeg_simple_progression (&cinfo);
	}
#endif /* HAVE_PROGRESSIVE_JPEG */

	jpeg_start_compress (&cinfo, TRUE);

	// Save one scanline at a time
	guchar *row_ptr = pixels;
	while (cinfo.next_scanline < cinfo.image_height) {
		// Convert scanline to RGB
		pixel_line_to_rgb_big_endian (line_buffer, row_ptr, width);

		// Write scanline
		JSAMPROW *jrow = (JSAMPROW *)(&line_buffer);
		jpeg_write_scanlines (&cinfo, jrow, 1);

		row_ptr += row_stride;
	}

	// Finish off
	jpeg_finish_compress (&cinfo);
	jpeg_destroy_compress (&cinfo);
	g_free (line_buffer);

	return g_bytes_new_take (buffer, buffer_size);
}
