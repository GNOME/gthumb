#include <config.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <setjmp.h>
#include <jpeglib.h>
#if HAVE_LCMS2
#include <lcms2.h>
#endif
#include "lib/jpeg/jmemorysrc.h"
#include "lib/jpeg/jpeg-info.h"
#include "lib/gth-icc-profile.h"
#include "load-jpeg.h"


struct error_handler_data {
	struct jpeg_error_mgr pub;
	sigjmp_buf setjmp_buffer;
	GError **error;
};


static void fatal_error_handler (j_common_ptr cinfo) {
	struct error_handler_data *errmgr;

	errmgr = (struct error_handler_data *) cinfo->err;
	if ((errmgr->error != NULL) && (*errmgr->error == NULL)) {
		char buffer[JMSG_LENGTH_MAX];

		// Create the message.
		(*cinfo->err->format_message) (cinfo, buffer);

		g_set_error (errmgr->error,
			     G_IO_ERROR,
			     G_IO_ERROR_INVALID_DATA,
			     "Error interpreting JPEG image file: %s",
			     buffer);
		siglongjmp (errmgr->setjmp_buffer, 1);
	}
}


static void output_message_handler (j_common_ptr cinfo) {
	// This method keeps libjpeg from dumping text to stderr.
}


// Tables with pre-multiplied values.


static unsigned char *CMYK_Tab = NULL;
static int *YCbCr_R_Cr_Tab = NULL;
static int *YCbCr_G_Cb_Tab = NULL;
static int *YCbCr_G_Cr_Tab = NULL;
static int *YCbCr_B_Cb_Tab = NULL;
static GMutex Tables_Mutex;


#define SCALE_FACTOR 16
#define SCALE_UP(x) ((gint32) ((x) * (1L << SCALE_FACTOR) + 0.5))
#define SCALE_DOWN(x) ((x) >> SCALE_FACTOR)
#define ONE_HALF ((gint32) (1 << (SCALE_FACTOR - 1)))


static void CMYK_table_init (void) {
	g_mutex_lock (&Tables_Mutex);

	if (CMYK_Tab == NULL) {
		int    v, k, i;
		double k1;

		// tab[k * 256 + v] = v * k / 255.0

		CMYK_Tab = g_new (unsigned char, 256 * 256);
		i = 0;
		for (k = 0; k <= 255; k++) {
			k1 = (double) k / 255.0;
			for (v = 0; v <= 255; v++)
				CMYK_Tab[i++] = (double) v * k1;
		}
	}

	g_mutex_unlock (&Tables_Mutex);
}


static void YCbCr_tables_init (void) {
	g_mutex_lock (&Tables_Mutex);

	if (YCbCr_R_Cr_Tab == NULL) {
		int i, v;

		YCbCr_R_Cr_Tab = g_new (int, 256);
		YCbCr_G_Cb_Tab = g_new (int, 256);
		YCbCr_G_Cr_Tab = g_new (int, 256);
		YCbCr_B_Cb_Tab = g_new (int, 256);

		for (i = 0, v = -128; i <= 255; i++, v++) {
			YCbCr_R_Cr_Tab[i] = SCALE_DOWN (SCALE_UP (1.402) * v + ONE_HALF);
			YCbCr_G_Cb_Tab[i] = - SCALE_UP (0.34414) * v;
			YCbCr_G_Cr_Tab[i] = - SCALE_UP (0.71414) * v + ONE_HALF;
			YCbCr_B_Cb_Tab[i] = SCALE_DOWN (SCALE_UP (1.77200) * v + ONE_HALF);
		}
	}

	g_mutex_unlock (&Tables_Mutex);
}


GthImage * load_jpeg (GBytes *bytes, guint requested_size, GCancellable *cancellable, GError **error) {
	gsize in_buffer_size;
	const void *in_buffer = g_bytes_get_data (bytes, &in_buffer_size);
	if (in_buffer_size == 0) {
		g_set_error_literal (error,
			G_IO_ERROR,
			G_IO_ERROR_INVALID_DATA,
			"No data");
		return NULL;
	}

	// Read orientation and color profile.

	JpegInfoData jpeg_info;
	_jpeg_info_data_init (&jpeg_info);

	JpegInfoFlags info_flags = _JPEG_INFO_EXIF_ORIENTATION;
#if HAVE_LCMS2
	info_flags |= _JPEG_INFO_EXIF_COLOR_SPACE | _JPEG_INFO_ICC_PROFILE;
#endif
	_jpeg_info_get_from_buffer (in_buffer, in_buffer_size, info_flags, &jpeg_info);

	GthTransform orientation = GTH_TRANSFORM_NONE;
	if (jpeg_info.valid & _JPEG_INFO_EXIF_ORIENTATION) {
		orientation = jpeg_info.orientation;
	}

	GthIccProfile *profile = NULL;

#if HAVE_LCMS2
	if (jpeg_info.valid & _JPEG_INFO_ICC_PROFILE) {
		GBytes *bytes = g_bytes_new_take (jpeg_info.icc_data, jpeg_info.icc_data_size);
		profile = gth_icc_profile_new_from_bytes (bytes, NULL);
		g_bytes_unref (bytes);
		jpeg_info.icc_data = NULL;
	}
	else if (jpeg_info.valid & _JPEG_INFO_EXIF_COLOR_SPACE) {
		if (jpeg_info.color_space == GTH_COLOR_SPACE_SRGB) {
			profile = gth_icc_profile_new_srgb ();
		}
		else if (jpeg_info.color_space == GTH_COLOR_SPACE_ADOBERGB) {
			profile = gth_icc_profile_new_adobergb ();
		}
	}
#endif

	_jpeg_info_data_dispose (&jpeg_info);

	// Decompress the image

	GthImage *image = NULL;

	struct jpeg_decompress_struct srcinfo;
	struct error_handler_data jsrcerr;
	srcinfo.err = jpeg_std_error (&(jsrcerr.pub));
	jsrcerr.pub.error_exit = fatal_error_handler;
	jsrcerr.pub.output_message = output_message_handler;
	jsrcerr.error = error;

	jpeg_create_decompress (&srcinfo);

	if (sigsetjmp (jsrcerr.setjmp_buffer, 1)) {
		goto stop_loading;
	}

	_jpeg_memory_src (&srcinfo, in_buffer, in_buffer_size);

	jpeg_read_header (&srcinfo, TRUE);

	srcinfo.out_color_space = srcinfo.jpeg_color_space; // Make all the color space conversions manually.

	gboolean load_scaled = (requested_size > 0) && (requested_size < srcinfo.image_width) && (requested_size < srcinfo.image_height);
	if (load_scaled) {
		for (srcinfo.scale_denom = 1; srcinfo.scale_denom <= 16; srcinfo.scale_denom++) {
			jpeg_calc_output_dimensions (&srcinfo);
			if ((srcinfo.output_width < requested_size) || (srcinfo.output_height < requested_size)) {
				srcinfo.scale_denom -= 1;
				break;
			}
		}

		if (srcinfo.scale_denom <= 0) {
			srcinfo.scale_denom = srcinfo.scale_num;
			load_scaled = FALSE;
		}
	}

	jpeg_calc_output_dimensions (&srcinfo);

	int buffer_stride = srcinfo.output_width * srcinfo.output_components;
	JSAMPARRAY buffer = (*srcinfo.mem->alloc_sarray) ((j_common_ptr) &srcinfo, JPOOL_IMAGE, buffer_stride, srcinfo.rec_outbuf_height);

	jpeg_start_decompress (&srcinfo);

	int output_width = MIN (srcinfo.output_width, GTH_MAX_IMAGE_SIZE);
	int output_height = MIN (srcinfo.output_height, GTH_MAX_IMAGE_SIZE);
	int destination_width;
	int destination_height;
	int line_start;
	int line_step;
	int pixel_step;
	get_transformation_steps (
		orientation,
		output_width,
		output_height,
		&destination_width,
		&destination_height,
		&line_start,
		&line_step,
		&pixel_step);

#if 0
	g_print ("requested: %d, original [%d, %d] ==> load at [%d, %d]\n",
		requested_size,
		srcinfo.image_width,
		srcinfo.image_height,
		destination_width,
		destination_height);
#endif

	image = gth_image_new (destination_width, destination_height);
	if (image == NULL) {
		jpeg_destroy_decompress (&srcinfo);
		return NULL;
	}

	gth_image_set_has_alpha (image, FALSE);
	if (profile != NULL) {
		gth_image_set_icc_profile (image, profile);
		g_object_unref (profile);
	}

	unsigned char *surface_data = gth_image_get_pixels (image, NULL, NULL);
	unsigned char *surface_row = surface_data + line_start;
	int scanned_lines = 0;
	JDIMENSION n_lines;
	JSAMPARRAY buffer_row;
	int l;
	unsigned char *p_surface;
	guchar r, g, b;
	guint32 pixel;
	unsigned char *p_buffer;
	int x;
	volatile gboolean read_all_scanlines = FALSE;
	volatile gboolean finished = FALSE;

	switch (srcinfo.out_color_space) {
	case JCS_CMYK:
		{
			register unsigned char *cmyk_tab;
			int c, m, y, k, ki;

			CMYK_table_init ();
			cmyk_tab = CMYK_Tab;

			while (srcinfo.output_scanline < output_height) {
				if (g_cancellable_is_cancelled (cancellable))
					goto stop_loading;

				n_lines = jpeg_read_scanlines (&srcinfo, buffer, srcinfo.rec_outbuf_height);
				if (scanned_lines + n_lines > output_height)
					n_lines = output_height - scanned_lines;

				buffer_row = buffer;
				for (l = 0; l < n_lines; l++) {
					p_surface = surface_row;
					p_buffer = buffer_row[l];

					if (g_cancellable_is_cancelled (cancellable))
						goto stop_loading;

					for (x = 0; x < output_width; x++) {
						if (srcinfo.saw_Adobe_marker) {
							c = p_buffer[0];
							m = p_buffer[1];
							y = p_buffer[2];
							k = p_buffer[3];
						}
						else {
							c = 255 - p_buffer[0];
							m = 255 - p_buffer[1];
							y = 255 - p_buffer[2];
							k = 255 - p_buffer[3];
						}

						ki = k << 8; // ki = k * 256
						r = cmyk_tab[ki + c];
						g = cmyk_tab[ki + m];
						b = cmyk_tab[ki + y];
						pixel = RGBA_TO_PIXEL (r, g, b, 0xff);
						memcpy (p_surface, &pixel, sizeof (guint32));

						p_surface += pixel_step;
						p_buffer += 4; // srcinfo.output_components
					}

					surface_row += line_step;
					buffer_row += buffer_stride;
					scanned_lines += 1;
				}
			}
		}
		break;

	case JCS_GRAYSCALE:
		{
			while (srcinfo.output_scanline < output_height) {
				if (g_cancellable_is_cancelled (cancellable))
					goto stop_loading;

				n_lines = jpeg_read_scanlines (&srcinfo, buffer, srcinfo.rec_outbuf_height);
				if (scanned_lines + n_lines > output_height)
					n_lines = output_height - scanned_lines;

				buffer_row = buffer;
				for (l = 0; l < n_lines; l++) {
					p_surface = surface_row;
					p_buffer = buffer_row[l];

					if (g_cancellable_is_cancelled (cancellable))
						goto stop_loading;

					for (x = 0; x < output_width; x++) {
						r = g = b = p_buffer[0];
						pixel = RGBA_TO_PIXEL (r, g, b, 0xff);
						memcpy (p_surface, &pixel, sizeof (guint32));

						p_surface += pixel_step;
						p_buffer += 1; // srcinfo.output_components
					}

					surface_row += line_step;
					buffer_row += buffer_stride;
					scanned_lines += 1;
				}
			}
		}
		break;

	case JCS_RGB:
		{
			while (srcinfo.output_scanline < output_height) {
				if (g_cancellable_is_cancelled (cancellable))
					goto stop_loading;

				n_lines = jpeg_read_scanlines (&srcinfo, buffer, srcinfo.rec_outbuf_height);
				if (scanned_lines + n_lines > output_height)
					n_lines = output_height - scanned_lines;

				buffer_row = buffer;
				for (l = 0; l < n_lines; l++) {
					p_surface = surface_row;
					p_buffer = buffer_row[l];

					if (g_cancellable_is_cancelled (cancellable))
						goto stop_loading;

					for (x = 0; x < output_width; x++) {
						r = p_buffer[0];
						g = p_buffer[1];
						b = p_buffer[2];
						pixel = RGBA_TO_PIXEL (r, g, b, 0xff);
						memcpy (p_surface, &pixel, sizeof (guint32));

						p_surface += pixel_step;
						p_buffer += 3; // srcinfo.output_components
					}

					surface_row += line_step;
					buffer_row += buffer_stride;
					scanned_lines += 1;
				}
			}
		}
		break;

	case JCS_YCbCr:
		{
			register JSAMPLE *range_limit = srcinfo.sample_range_limit;
			register int *r_cr_tab;
			register int *g_cb_tab;
			register int *g_cr_tab;
			register int *b_cb_tab;
			int Y, Cb, Cr;

			YCbCr_tables_init ();
			r_cr_tab = YCbCr_R_Cr_Tab;
			g_cb_tab = YCbCr_G_Cb_Tab;
			g_cr_tab = YCbCr_G_Cr_Tab;
			b_cb_tab = YCbCr_B_Cb_Tab;

			while (srcinfo.output_scanline < output_height) {
				if (g_cancellable_is_cancelled (cancellable))
					goto stop_loading;

				n_lines = jpeg_read_scanlines (&srcinfo, buffer, srcinfo.rec_outbuf_height);
				if (scanned_lines + n_lines > output_height)
					n_lines = output_height - scanned_lines;

				buffer_row = buffer;
				for (l = 0; l < n_lines; l++) {
					p_surface = surface_row;
					p_buffer = buffer_row[l];

					if (g_cancellable_is_cancelled (cancellable))
						goto stop_loading;

					for (x = 0; x < output_width; x++) {
						Y = p_buffer[0];
						Cb = p_buffer[1];
						Cr = p_buffer[2];

						r = range_limit[Y + r_cr_tab[Cr]];
						g = range_limit[Y + SCALE_DOWN (g_cb_tab[Cb] + g_cr_tab[Cr])];
						b = range_limit[Y + b_cb_tab[Cb]];
						pixel = RGBA_TO_PIXEL (r, g, b, 0xff);
						memcpy (p_surface, &pixel, sizeof (guint32));

						p_surface += pixel_step;
						p_buffer += 3; // srcinfo.output_components
					}

					surface_row += line_step;
					buffer_row += buffer_stride;
					scanned_lines += 1;
				}
			}
		}
		break;

	case JCS_YCCK:
		{
			register JSAMPLE *range_limit = srcinfo.sample_range_limit;
			register int *r_cr_tab;
			register int *g_cb_tab;
			register int *g_cr_tab;
			register int *b_cb_tab;
			register guchar *cmyk_tab;
			int Y, Cb, Cr, K, Ki, c, m , y;

			YCbCr_tables_init ();
			r_cr_tab = YCbCr_R_Cr_Tab;
			g_cb_tab = YCbCr_G_Cb_Tab;
			g_cr_tab = YCbCr_G_Cr_Tab;
			b_cb_tab = YCbCr_B_Cb_Tab;

			CMYK_table_init ();
			cmyk_tab = CMYK_Tab;

			while (srcinfo.output_scanline < output_height) {
				if (g_cancellable_is_cancelled (cancellable))
					goto stop_loading;

				n_lines = jpeg_read_scanlines (&srcinfo, buffer, srcinfo.rec_outbuf_height);
				if (scanned_lines + n_lines > output_height)
					n_lines = output_height - scanned_lines;

				buffer_row = buffer;
				for (l = 0; l < n_lines; l++) {
					p_surface = surface_row;
					p_buffer = buffer_row[l];

					if (g_cancellable_is_cancelled (cancellable))
						goto stop_loading;

					for (x = 0; x < output_width; x++) {
						Y = p_buffer[0];
						Cb = p_buffer[1];
						Cr = p_buffer[2];
						K = p_buffer[3];

						c = range_limit[255 - (Y + r_cr_tab[Cr])];
						m = range_limit[255 - (Y + SCALE_DOWN (g_cb_tab[Cb] + g_cr_tab[Cr]))];
						y = range_limit[255 - (Y + b_cb_tab[Cb])];

						Ki = K << 8; // ki = K * 256

						r = cmyk_tab[Ki + c];
						g = cmyk_tab[Ki + m];
						b = cmyk_tab[Ki + y];
						pixel = RGBA_TO_PIXEL (r, g, b, 0xff);
						memcpy (p_surface, &pixel, sizeof (guint32));

						p_surface += pixel_step;
						p_buffer += 4; // srcinfo.output_components
					}

					surface_row += line_step;
					buffer_row += buffer_stride;
					scanned_lines += 1;
				}
			}
		}
		break;

	case JCS_UNKNOWN:
	default:
		g_set_error (error,
			G_IO_ERROR,
			G_IO_ERROR_INVALID_DATA,
			"Unknown JPEG color space (%d)",
			srcinfo.out_color_space);
		break;
	}

	read_all_scanlines = TRUE;

stop_loading:

	if (!finished) {
		finished = TRUE;
		gboolean success = FALSE;
		if (image != NULL) {
			if (read_all_scanlines && !g_cancellable_is_cancelled (cancellable)) {
				int natural_width;
				int natural_height;

				// Set the original dimensions.
				if ((orientation == GTH_TRANSFORM_ROTATE_90)
					 || (orientation == GTH_TRANSFORM_ROTATE_270)
					 || (orientation == GTH_TRANSFORM_TRANSPOSE)
					 || (orientation == GTH_TRANSFORM_TRANSVERSE))
				{
					natural_width = srcinfo.image_height;
					natural_height = srcinfo.image_width;
				}
				else {
					natural_width = srcinfo.image_width;
					natural_height = srcinfo.image_height;
				}

				gth_image_set_natural_size (image, (guint) natural_width, (guint) natural_height);
				success = TRUE;
			}
			else {
				g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_CANCELLED, "");
			}
		}

		if (read_all_scanlines) {
			jpeg_finish_decompress (&srcinfo);
		}
		else {
			jpeg_abort_decompress (&srcinfo);
		}
		jpeg_destroy_decompress (&srcinfo);

		if (!success && (image != NULL)) {
			g_object_unref (image);
			image = NULL; // Ignore other jpeg errors.
		}
	}

	return image;
}

#undef SCALE_FACTOR
#undef SCALE_UP
#undef SCALE_DOWN
#undef ONE_HALF

gboolean load_jpeg_info (GInputStream *stream, GthImageInfo *image_info, GCancellable *cancellable) {
	if (!g_seekable_seek (G_SEEKABLE (stream), 0, G_SEEK_SET, cancellable, NULL))
		return FALSE;

	JpegInfoData jpeg_info;
	_jpeg_info_data_init (&jpeg_info);
	_jpeg_info_get_from_stream (
		G_INPUT_STREAM (stream),
		_JPEG_INFO_IMAGE_SIZE | _JPEG_INFO_EXIF_ORIENTATION,
		&jpeg_info,
		cancellable,
		NULL);

	gboolean format_recognized = FALSE;
	if (jpeg_info.valid & _JPEG_INFO_IMAGE_SIZE) {
		image_info->width = jpeg_info.width;
		image_info->height = jpeg_info.height;
		format_recognized = TRUE;
		if (jpeg_info.valid & _JPEG_INFO_EXIF_ORIENTATION) {
			if ((jpeg_info.orientation == GTH_TRANSFORM_ROTATE_90)
				|| (jpeg_info.orientation == GTH_TRANSFORM_ROTATE_270)
				|| (jpeg_info.orientation == GTH_TRANSFORM_TRANSPOSE)
				|| (jpeg_info.orientation == GTH_TRANSFORM_TRANSVERSE))
			{
				int tmp = image_info->width;
				image_info->width = image_info->height;
				image_info->height = tmp;
			}
		}
	}
	_jpeg_info_data_dispose (&jpeg_info);
	return format_recognized;
}
