#include <config.h>
#include <tiffio.h>
#include "lib/pixel.h"
#include "save-tiff.h"

#define TILE_HEIGHT 40

static tsize_t tiff_read (thandle_t handle, tdata_t buffer, tsize_t size) {
	return -1;
}

static tsize_t tiff_write (thandle_t handle, tdata_t buffer, tsize_t size) {
	GByteArray *byte_array = (GByteArray *) handle;
	g_byte_array_append (byte_array, buffer, (guint) size);
	return size;
}

static toff_t tiff_seek (thandle_t handle, toff_t offset, int whence) {
	return -1;
}

static int tiff_close (thandle_t context) {
	return 0;
}

static toff_t tiff_size (thandle_t handle) {
	return -1;
}

GBytes* save_tiff (GthImage *image, GthOption **options, GCancellable *cancellable, GError **error) {
	GthTiffCompression tiff_compression = GTH_TIFF_COMPRESSION_LOSSLESS;
	int horizontal_dpi = 72;
	int vertical_dpi = 72;
	gboolean save_resolution = FALSE;

	if (options != NULL) {
		for (int i = 0; options[i] != NULL; i++) {
			GthOption *option = options[i];
			switch (gth_option_get_id (option)) {
			case GTH_TIFF_OPTION_COMPRESSION:
				gth_option_get_enum (option, (int*) &tiff_compression);
				break;

			case GTH_TIFF_OPTION_HRESOLUTION:
				if (gth_option_get_int (option, &horizontal_dpi)) {
					save_resolution = TRUE;
				}
				if (horizontal_dpi < 0) {
					g_set_error (error,
						G_IO_ERROR,
						G_IO_ERROR_INVALID_DATA,
						"Horizontal DPI must be a positive integer, '%d' is not allowed.",
						horizontal_dpi);
					return NULL;
				}
				break;

			case GTH_TIFF_OPTION_VRESOLUTION:
				if (gth_option_get_int (option, &vertical_dpi)) {
					save_resolution = TRUE;
				}
				if (vertical_dpi < 0) {
					g_set_error (error,
						G_IO_ERROR,
						G_IO_ERROR_INVALID_DATA,
						"Vertical DPI must be a positive integer, '%d' is not allowed.",
						vertical_dpi);
					return NULL;
				}
				break;
			}
		}
	}

	gushort TIFF_COMPRESSION[] = {
		COMPRESSION_NONE,
		COMPRESSION_DEFLATE,
		COMPRESSION_JPEG,
	};
	gushort compression = TIFF_COMPRESSION[tiff_compression];

	GByteArray *byte_array = g_byte_array_new  ();
	TIFF *tif = TIFFClientOpen ("gth-tiff-writer", "w",
		byte_array,
		tiff_read,
		tiff_write,
		tiff_seek,
		tiff_close,
		tiff_size,
		NULL,
		NULL);

	if (tif == NULL) {
		g_byte_array_free (byte_array, TRUE);
		g_set_error_literal (error,
			GDK_PIXBUF_ERROR,
			GDK_PIXBUF_ERROR_INSUFFICIENT_MEMORY,
			"Couldn't allocate memory for writing TIFF file");
		return NULL;
	}

	int rowstride, cols, rows;
	guchar *pixels = gth_image_prepare_edit (image, &rowstride, &cols, &rows);

	int alpha = FALSE;
	gth_image_get_has_alpha (image, &alpha);

	gshort predictor = 2;
	gshort bitspersample = 8;
	gshort photometric = PHOTOMETRIC_RGB;
	gshort samplesperpixel = alpha ? 4 : 3;
	glong rowsperstrip = TILE_HEIGHT;

	// Set TIFF parameters.

	TIFFSetField (tif, TIFFTAG_SUBFILETYPE, 0);
	TIFFSetField (tif, TIFFTAG_IMAGEWIDTH, cols);
	TIFFSetField (tif, TIFFTAG_IMAGELENGTH, rows);
	TIFFSetField (tif, TIFFTAG_BITSPERSAMPLE, bitspersample);
	TIFFSetField (tif, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);
	TIFFSetField (tif, TIFFTAG_COMPRESSION, compression);

	if (((compression == COMPRESSION_LZW) || (compression == COMPRESSION_DEFLATE))
	    && (predictor != 0))
	{
		TIFFSetField (tif, TIFFTAG_PREDICTOR, predictor);
	}

	if (alpha) {
		gushort extra_samples[1];
		extra_samples [0] = EXTRASAMPLE_ASSOCALPHA;
		TIFFSetField (tif, TIFFTAG_EXTRASAMPLES, 1, extra_samples);
	}

	TIFFSetField (tif, TIFFTAG_PHOTOMETRIC, photometric);
	//TIFFSetField (tif, TIFFTAG_DOCUMENTNAME, filename);
	TIFFSetField (tif, TIFFTAG_SAMPLESPERPIXEL, samplesperpixel);
	TIFFSetField (tif, TIFFTAG_ROWSPERSTRIP, rowsperstrip);
	TIFFSetField (tif, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);

	if (save_resolution) {
		TIFFSetField (tif, TIFFTAG_XRESOLUTION, (double) horizontal_dpi);
		TIFFSetField (tif, TIFFTAG_YRESOLUTION, (double) vertical_dpi);
		TIFFSetField (tif, TIFFTAG_RESOLUTIONUNIT, RESUNIT_INCH);
	}

	guchar *line_buffer = g_try_malloc (cols * samplesperpixel * sizeof (guchar));
	if (!line_buffer) {
		g_byte_array_free (byte_array, TRUE);
		TIFFClose (tif);
		g_set_error_literal (error,
				     GDK_PIXBUF_ERROR,
				     GDK_PIXBUF_ERROR_INSUFFICIENT_MEMORY,
				     _("Insufficient memory"));
		return FALSE;
	}

	// Write the TIFF data.
	guchar *ptr = pixels;
	for (int row = 0; row < rows; row++) {
		if (alpha) {
			pixel_line_to_rgba_big_endian (line_buffer, ptr, cols);
		}
		else {
			pixel_line_to_rgb_big_endian (line_buffer, ptr, cols);
		}
		if (TIFFWriteScanline (tif, line_buffer, row, 0) < 0) {
			g_free (line_buffer);
			g_byte_array_free (byte_array, TRUE);
			TIFFClose (tif);
			g_set_error (error,
				     GDK_PIXBUF_ERROR,
				     GDK_PIXBUF_ERROR_FAILED,
				     "TIFF Failed a scanline write on row %d",
				     row);
			return FALSE;
		}
		ptr += rowstride;
	}

	g_free (line_buffer);

	TIFFFlushData (tif);
	TIFFClose (tif);

	return g_byte_array_free_to_bytes (byte_array);
}
