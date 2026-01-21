#include <config.h>
#include <tiffio.h>
#include "lib/pixel.h"
#include "save-tiff.h"

#define TILE_HEIGHT 40

static tsize_t tiff_read (thandle_t handle, tdata_t buffer, tsize_t size) {
	return -1;
}

static tsize_t tiff_write (thandle_t handle, tdata_t buffer, tsize_t size) {
	GOutputStream *output = (GOutputStream *) handle;
	gsize bytes_written;
	g_output_stream_write_all (output, buffer, (gsize) size, &bytes_written, NULL, NULL);
	return (tsize_t) bytes_written;
}

static toff_t tiff_seek (thandle_t handle, toff_t offset, int whence) {
	GOutputStream *output = (GOutputStream *) handle;
	GSeekType seek_type = G_SEEK_SET;
	if (whence == SEEK_CUR) {
		seek_type = G_SEEK_CUR;
	}
	else if (whence == SEEK_END) {
		seek_type = G_SEEK_END;
	}
	g_seekable_seek (G_SEEKABLE (output), offset, seek_type, NULL, NULL);
	return (toff_t) g_seekable_tell (G_SEEKABLE (output));
}

static int tiff_close (thandle_t handle) {
	return 0;
}

static toff_t tiff_size (thandle_t handle) {
	GOutputStream *output = (GOutputStream *) handle;
	return (toff_t) g_memory_output_stream_get_size (G_MEMORY_OUTPUT_STREAM (output));
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
		COMPRESSION_ADOBE_DEFLATE,
		COMPRESSION_JPEG,
	};
	gushort compression = TIFF_COMPRESSION[tiff_compression];

	GOutputStream *output = g_memory_output_stream_new_resizable  ();
	TIFF *tif = TIFFClientOpen ("gth-tiff-writer", "w",
		output,
		tiff_read,
		tiff_write,
		tiff_seek,
		tiff_close,
		tiff_size,
		NULL,
		NULL);

	if (tif == NULL) {
		g_object_unref (output);
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
		g_object_unref (output);
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
			g_object_unref (output);
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

	g_output_stream_close (output, NULL, NULL);
	GBytes *bytes = g_memory_output_stream_steal_as_bytes (G_MEMORY_OUTPUT_STREAM (output));
	g_object_unref (output);
	return bytes;
}
