#include <config.h>
#include <tiff.h>
#include <tiffio.h>
#include "load-tiff.h"

#define HANDLE(x) ((Handle *) (x))

typedef struct {
	GInputStream *istream;
	GCancellable *cancellable;
	goffset size;
} Handle;


static tsize_t tiff_read (thandle_t handle, tdata_t buf, tsize_t size) {
	Handle *h = HANDLE (handle);
	return g_input_stream_read (G_INPUT_STREAM (h->istream), buf, size, h->cancellable, NULL);
}


static tsize_t tiff_write (thandle_t handle, tdata_t buf, tsize_t size) {
	return -1;
}


static toff_t tiff_seek (thandle_t handle, toff_t offset, int whence) {
	Handle *h = HANDLE (handle);
	GSeekType seek_type;

	seek_type = G_SEEK_SET;
	switch (whence) {
	case SEEK_SET:
		seek_type = G_SEEK_SET;
		break;
	case SEEK_CUR:
		seek_type = G_SEEK_CUR;
		break;
	case SEEK_END:
		seek_type = G_SEEK_END;
		break;
	}

	if (! g_seekable_seek (G_SEEKABLE (h->istream), offset, seek_type, h->cancellable, NULL))
		return -1;

	return g_seekable_tell (G_SEEKABLE (h->istream));
}


static int tiff_close (thandle_t context) {
	return 0;
}


static toff_t tiff_size (thandle_t handle) {
	return (toff_t) HANDLE (handle)->size;
}


static void tiff_error_handler (const char *module, const char *fmt, va_list ap) {
	// Do not print warnings and errors.
}


GthImage * load_tiff (GBytes *bytes, guint requested_size, GCancellable *cancellable, GError **error) {
	gsize in_buffer_size;
	const void *in_buffer = g_bytes_get_data (bytes, &in_buffer_size);
	if (in_buffer_size == 0) {
		g_set_error_literal (error,
			G_IO_ERROR,
			G_IO_ERROR_INVALID_DATA,
			"No data");
		return NULL;
	}

	Handle handle;
	handle.cancellable = cancellable;
	handle.istream = g_memory_input_stream_new_from_data (in_buffer, in_buffer_size, NULL);
	handle.size = in_buffer_size;

	TIFFSetErrorHandler (tiff_error_handler);
	TIFFSetWarningHandler (tiff_error_handler);

	TIFF *tif = TIFFClientOpen ("gth-tiff-reader", "r",
		&handle,
		tiff_read, tiff_write,
		tiff_seek, tiff_close,
		tiff_size,
		NULL, NULL);
	if (tif == NULL) {
		g_set_error_literal (error,
			G_IO_ERROR,
			G_IO_ERROR_INVALID_DATA,
			"Could not allocate memory to load the image");
		return NULL;
	}

	// Find the best image to load.

	gboolean first_directory = TRUE;
	int best_directory = -1;
	int max_width = -1;
	int max_height = -1;
	int min_diff = 0;
	char emsg[1024];
	do {
		int width;
		int height;

		if (TIFFGetField (tif, TIFFTAG_IMAGEWIDTH, &width) != 1) {
			continue;
		}
		if (TIFFGetField (tif, TIFFTAG_IMAGELENGTH, &height) != 1) {
			continue;
		}
		if (!TIFFRGBAImageOK (tif, emsg)) {
			continue;
		}

		if (width > max_width) {
			max_width = width;
			max_height = height;
			if (requested_size == 0) {
				best_directory = TIFFCurrentDirectory (tif);
			}
		}

		if (requested_size > 0) {
			int diff = abs ((int) requested_size - width);

			if (first_directory) {
				min_diff = diff;
				best_directory = TIFFCurrentDirectory (tif);
			}
			else if (diff < min_diff) {
				min_diff = diff;
				best_directory = TIFFCurrentDirectory (tif);
			}
		}

		first_directory = FALSE;
	}
	while (TIFFReadDirectory (tif));

	if (best_directory == -1) {
		TIFFClose (tif);
		g_object_unref (handle.istream);
		g_set_error_literal (error,
			G_IO_ERROR,
			G_IO_ERROR_INVALID_DATA,
			"Invalid TIFF format");
		return NULL;
	}

	// Read the image.

	uint32 image_width;
	uint32 image_height;
	uint32 spp;
	uint16 extrasamples;
	uint16 *sampleinfo;
	uint16 orientation;

	TIFFSetDirectory (tif, best_directory);
	TIFFGetField (tif, TIFFTAG_IMAGEWIDTH, &image_width);
	TIFFGetField (tif, TIFFTAG_IMAGELENGTH, &image_height);
	TIFFGetField (tif, TIFFTAG_SAMPLESPERPIXEL, &spp);
	TIFFGetFieldDefaulted (tif, TIFFTAG_EXTRASAMPLES, &extrasamples, &sampleinfo);
	if (TIFFGetFieldDefaulted (tif, TIFFTAG_ORIENTATION, &orientation) != 1) {
		orientation = ORIENTATION_TOPLEFT;
	}

	uint natural_width = (uint) max_width;
	uint natural_height = (uint) max_height;
	GthImage *image = gth_image_new (image_width, image_height);
	if (image == NULL) {
		TIFFClose (tif);
		g_object_unref (handle.istream);
		g_set_error_literal (error,
			G_IO_ERROR,
			G_IO_ERROR_INVALID_DATA,
			"Could not allocate memory to load the image");
		return NULL;
	}

	gth_image_set_has_alpha (image, (extrasamples == 1) || (spp == 4));
	gth_image_set_natural_size (image, natural_width, natural_height);

	uint32 *raster = (uint32*) _TIFFmalloc (image_width * image_height * sizeof (uint32));
	if (raster == NULL) {
		g_object_unref (image);
		TIFFClose (tif);
		g_object_unref (handle.istream);
		g_set_error_literal (error,
			G_IO_ERROR,
			G_IO_ERROR_INVALID_DATA,
			"Could not allocate memory to load the image");
		return NULL;
	}

	if (!TIFFReadRGBAImageOriented (tif, image_width, image_height, raster, orientation, 0)) {
		_TIFFfree (raster);
		g_object_unref (image);
		TIFFClose (tif);
		g_object_unref (handle.istream);
		g_set_error_literal (error,
			G_IO_ERROR,
			G_IO_ERROR_INVALID_DATA,
			"Could not read the image");
		return NULL;
	}

	int dest_row_stride;
	guchar *dest_row = gth_image_get_pixels (image, NULL, &dest_row_stride);
	int src_row_stride = image_width * 4;
	guchar *src_row = (guchar *) raster;
	for (int y = 0; y < image_height; y++) {
		if (g_cancellable_is_cancelled (cancellable)) {
			g_object_unref (image);
			image = NULL;
			break;
		}
		abgr_line_to_pixel (dest_row, src_row, image_width);
		dest_row += dest_row_stride;
		src_row += src_row_stride;
	}

	_TIFFfree (raster);
	TIFFClose (tif);
	g_object_unref (handle.istream);

	return image;
}
