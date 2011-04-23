/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2011 Free Software Foundation, Inc.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <config.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <setjmp.h>
#include <jpeglib.h>
#include <gthumb.h>
#include <extensions/jpeg_utils/jmemorysrc.h>
#include "cairo-io-jpeg.h"


/* error handler data */

struct error_handler_data {
	struct jpeg_error_mgr   pub;
	sigjmp_buf              setjmp_buffer;
        GError                **error;
};


static void
fatal_error_handler (j_common_ptr cinfo)
{
	struct error_handler_data *errmgr;
        char buffer[JMSG_LENGTH_MAX];

	errmgr = (struct error_handler_data *) cinfo->err;

        /* Create the message */
        (* cinfo->err->format_message) (cinfo, buffer);

        /* broken check for *error == NULL for robustness against
         * crappy JPEG library
         */
        if (errmgr->error && *errmgr->error == NULL) {
                g_set_error (errmgr->error,
                             GDK_PIXBUF_ERROR,
                             GDK_PIXBUF_ERROR_CORRUPT_IMAGE,
			     "Error interpreting JPEG image file (%s)",
                             buffer);
        }

	siglongjmp (errmgr->setjmp_buffer, 1);

        g_assert_not_reached ();
}


static void
output_message_handler (j_common_ptr cinfo)
{
	/* This method keeps libjpeg from dumping crap to stderr */
	/* do nothing */
}


GthImage *
_cairo_image_surface_create_from_jpeg (GthFileData   *file_data,
				       int            requested_size,
				       int           *original_width,
				       int           *original_height,
				       gpointer       user_data,
				       GCancellable  *cancellable,
				       GError       **error)
{
	GthImage                      *image;
	void                          *in_buffer;
	gsize                          in_buffer_size;
	struct error_handler_data      jsrcerr;
	struct jpeg_decompress_struct  srcinfo;
	cairo_surface_t               *surface;
	int                            surface_stride;
	unsigned char                 *surface_row;
	JSAMPARRAY                     buffer;
	int                            buffer_stride;
	JDIMENSION                     n_lines;
	JSAMPARRAY                     buffer_row;
	int                            l;
	unsigned char                 *p_surface;
	unsigned char                 *p_buffer;
	int                            x;

	image = gth_image_new ();

	if (! g_load_file_in_buffer (file_data->file,
			      	     &in_buffer,
			      	     &in_buffer_size,
			      	     cancellable,
			      	     error))
	{
		return image;
	}

	srcinfo.err = jpeg_std_error (&(jsrcerr.pub));
	jsrcerr.pub.error_exit = fatal_error_handler;
	jsrcerr.pub.output_message = output_message_handler;
	jsrcerr.error = error;

	jpeg_create_decompress (&srcinfo);

	if (sigsetjmp (jsrcerr.setjmp_buffer, 1)) {
		jpeg_destroy_decompress (&srcinfo);
		return image;
	}

	_jpeg_memory_src (&srcinfo, in_buffer, in_buffer_size);

	jpeg_read_header (&srcinfo, TRUE);

	/* FIXME: read the orientation flag and rotate the image */

	if (original_width != NULL)
		*original_width = srcinfo.image_width;
	if (original_height != NULL)
		*original_height = srcinfo.image_height;

	/* FIXME
	if (requested_size > 0) {
		for (srcinfo.scale_denom = 16; srcinfo.scale_denom >= 1; srcinfo.scale_denom--) {
			jpeg_calc_output_dimensions (&srcinfo);
			if ((srcinfo.output_width < requested_size) || (srcinfo.output_height < requested_size)) {
				srcinfo.scale_denom += 1;
				break;
			}
		}

		if (srcinfo.scale_denom == 0)
			srcinfo.scale_denom = srcinfo.scale_num;
	}
	*/

	jpeg_start_decompress (&srcinfo);

	surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, srcinfo.output_width, srcinfo.output_height);
	surface_stride = cairo_image_surface_get_stride (surface);
	surface_row = cairo_image_surface_get_data (surface);

	buffer_stride = srcinfo.output_width * srcinfo.output_components;
	buffer = (*srcinfo.mem->alloc_sarray) ((j_common_ptr) &srcinfo, JPOOL_IMAGE, buffer_stride, srcinfo.rec_outbuf_height);

	switch (srcinfo.out_color_space) {
	case JCS_CMYK:
		{
			static unsigned char *tab = NULL; /* table with pre-multiplied values */
			int                   ki;

			while (srcinfo.output_scanline < srcinfo.output_height) {
				if (g_cancellable_is_cancelled (cancellable))
					break;

				n_lines = jpeg_read_scanlines (&srcinfo, buffer, srcinfo.rec_outbuf_height);

				if (tab == NULL) {
					int    v, k, i;
					double k1;

					/* tab[k * 256 + v] = v * k / 255.0 */

					tab = g_new (unsigned char, 256 * 256);
					i = 0;
					for (k = 0; k <= 255; k++) {
						k1 = (double) k / 255.0;
						for (v = 0; v <= 255; v++)
							tab[i++] = (double) v * k1;
					}
				}

				buffer_row = buffer;
				for (l = 0; l < n_lines; l++) {
					p_surface = surface_row;
					p_buffer = buffer_row[l];

					for (x = 0; x < srcinfo.output_width; x++) {
						ki = p_buffer[3] << 8; /* ki = k * 256 */
						p_surface[CAIRO_RED]   = tab[ki + p_buffer[0]];
						p_surface[CAIRO_GREEN] = tab[ki + p_buffer[1]];
						p_surface[CAIRO_BLUE]  = tab[ki + p_buffer[2]];
						p_surface[CAIRO_ALPHA] = 0xff;

						p_surface += 4;
						p_buffer += 4 /*srcinfo.output_components*/;
					}

					surface_row += surface_stride;
					buffer_row += buffer_stride;
				}
			}
		}
		break;

	case JCS_GRAYSCALE:
		{
			while (srcinfo.output_scanline < srcinfo.output_height) {
				if (g_cancellable_is_cancelled (cancellable))
					break;

				n_lines = jpeg_read_scanlines (&srcinfo, buffer, srcinfo.rec_outbuf_height);

				buffer_row = buffer;
				for (l = 0; l < n_lines; l++) {
					p_surface = surface_row;
					p_buffer = buffer_row[l];

					for (x = 0; x < srcinfo.output_width; x++) {
						p_surface[CAIRO_RED]   = p_buffer[0];
						p_surface[CAIRO_GREEN] = p_buffer[0];
						p_surface[CAIRO_BLUE]  = p_buffer[0];
						p_surface[CAIRO_ALPHA] = 0xff;

						p_surface += 4;
						p_buffer += 1 /*srcinfo.output_components*/;
					}

					surface_row += surface_stride;
					buffer_row += buffer_stride;
				}
			}
		}
		break;

	case JCS_RGB:
		{
			while (srcinfo.output_scanline < srcinfo.output_height) {
				if (g_cancellable_is_cancelled (cancellable))
					break;

				n_lines = jpeg_read_scanlines (&srcinfo, buffer, srcinfo.rec_outbuf_height);

				buffer_row = buffer;
				for (l = 0; l < n_lines; l++) {
					p_surface = surface_row;
					p_buffer = buffer_row[l];

					for (x = 0; x < srcinfo.output_width; x++) {
						p_surface[CAIRO_RED]   = p_buffer[0];
						p_surface[CAIRO_GREEN] = p_buffer[1];
						p_surface[CAIRO_BLUE]  = p_buffer[2];
						p_surface[CAIRO_ALPHA] = 0xff;

						p_surface += 4;
						p_buffer += 3 /*srcinfo.output_components*/;
					}

					surface_row += surface_stride;
					buffer_row += buffer_stride;
				}
			}
		}
		break;

	case JCS_YCbCr:
		{
			double Y, Cb, Cr;

			while (srcinfo.output_scanline < srcinfo.output_height) {
				if (g_cancellable_is_cancelled (cancellable))
					break;

				n_lines = jpeg_read_scanlines (&srcinfo, buffer, srcinfo.rec_outbuf_height);

				buffer_row = buffer;
				for (l = 0; l < n_lines; l++) {
					p_surface = surface_row;
					p_buffer = buffer_row[l];

					for (x = 0; x < srcinfo.output_width; x++) {
						Y = (double) p_buffer[0];
						Cb = (double) p_buffer[1];
						Cr = (double) p_buffer[2];

						p_surface[CAIRO_RED]   = Y + 1.402 * (Cr - 128);
						p_surface[CAIRO_GREEN] = Y - 0.34414 * (Cb - 128) - 0.71414 * (Cr - 128);
						p_surface[CAIRO_BLUE]  = Y + 1.772 * (Cb - 128);
						p_surface[CAIRO_ALPHA] = 0xff;

						p_surface += 4;
						p_buffer += 3 /*srcinfo.output_components*/;
					}

					surface_row += surface_stride;
					buffer_row += buffer_stride;
				}
			}
		}
		break;

	case JCS_YCCK:
	case JCS_UNKNOWN:
	default:
		/* FIXME: ? */
		break;
	}

	if (! g_cancellable_is_cancelled (cancellable)) {
		jpeg_finish_decompress (&srcinfo);
		jpeg_destroy_decompress (&srcinfo);

		/* FIXME: scale to the requested size */

		gth_image_set_cairo_surface (image, surface);
	}
	else
		jpeg_destroy ((j_common_ptr) &srcinfo);

	cairo_surface_destroy (surface);
	g_free (in_buffer);

	return image;
}
