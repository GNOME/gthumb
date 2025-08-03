#include <config.h>
#if HAVE_LCMS2
#include <lcms2.h>
#endif
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

	// ICC Profile

#if HAVE_LCMS2

	uint32 icc_profile_size;
	void *icc_profile_data;
	if (TIFFGetField (tif, TIFFTAG_ICCPROFILE, &icc_profile_size, &icc_profile_data) == 1) {
		GBytes *bytes = g_bytes_new (icc_profile_data, icc_profile_size);
		GthIccProfile *profile = gth_icc_profile_new_from_bytes (bytes, NULL);
		if (profile != NULL) {
			gth_image_set_icc_profile (image, profile);
			g_object_unref (profile);
		}
		g_bytes_unref (bytes);
	}

#endif

	_TIFFfree (raster);
	TIFFClose (tif);
	g_object_unref (handle.istream);

	return image;
}


typedef struct {
	gboolean big_endian;
	GInputStream *stream;
	GCancellable *cancellable;
} Reader;


static bool _read_byte (Reader *reader, guchar *value) {
	return g_input_stream_read (reader->stream, value, 1, reader->cancellable, NULL) == 1;
}


static bool _read_uint16 (Reader *reader, guint16 *value) {
	guchar byte1, byte2;
	if (!_read_byte (reader, &byte1)) {
		return FALSE;
	}
	if (!_read_byte (reader, &byte2)) {
		return FALSE;
	}
	if (reader->big_endian) {
		*value = (byte1 << 8) + byte2;
	}
	else {
		*value = (byte2 << 8) + byte1;
	}
	return TRUE;
}


static bool _read_uint32 (Reader *reader, guint32 *value) {
	guchar byte1, byte2, byte3, byte4;
	if (!_read_byte (reader, &byte1)) {
		return FALSE;
	}
	if (!_read_byte (reader, &byte2)) {
		return FALSE;
	}
	if (!_read_byte (reader, &byte3)) {
		return FALSE;
	}
	if (!_read_byte (reader, &byte4)) {
		return FALSE;
	}
	if (reader->big_endian) {
		*value = (byte1 << 24) + (byte2 << 16) + (byte3 << 8) + byte4;
	}
	else {
		*value = (byte4 << 24) + (byte3 << 16) + (byte2 << 8) + byte1;
	}
	return TRUE;
}


static bool _seek (Reader *reader, guint32 offset) {
	return g_seekable_seek (G_SEEKABLE (reader->stream),
		(goffset) offset,
		G_SEEK_SET,
		reader->cancellable,
		NULL);
}


static bool _skip (Reader *reader, guint32 offset) {
	return g_seekable_seek (G_SEEKABLE (reader->stream),
		(goffset) offset,
		G_SEEK_CUR,
		reader->cancellable,
		NULL);
}


gboolean load_tiff_info (GInputStream *stream, GthImageInfo *image_info, GCancellable *cancellable) {
	Reader reader = {
		.stream = stream,
		.cancellable = cancellable
	};

	if (!_seek (&reader, 0)) {
		return FALSE;
	}

	// Byte order

	guchar byte1, byte2;
	if (!_read_byte (&reader, &byte1)) {
		return FALSE;
	}
	if (!_read_byte (&reader, &byte2)) {
		return FALSE;
	}

	if ((byte1 == 0x49) && (byte2 == 0x49)) {
		reader.big_endian = FALSE;
	}
	else if ((byte1 == 0x4D) && (byte2 == 0x4D)) {
		reader.big_endian = TRUE;
	}
	else {
		return FALSE;
	}

	// Type signature

	guint16 word;
	if (!_read_uint16 (&reader, &word)) {
		return FALSE;
	}
	if (word != 42) {
		return FALSE;
	}

	int remaining_tags = 3;
	int orientation = ORIENTATION_TOPLEFT;

	// IFD offset

next_ifd:

	guint32 offset;
	if (!_read_uint32 (&reader, &offset)) {
		return FALSE;
	}
	if (offset == 0) {
		return FALSE;
	}

	// Move to the next IFD

	if (!_seek (&reader, (goffset) offset)) {
		return FALSE;
	}

	// Number of entries

	guint16 entries;
	if (!_read_uint16 (&reader, &entries)) {
		return FALSE;
	}
	if (entries == 0) {
		return FALSE;
	}

	for (int i = 0; (remaining_tags > 0) && (i < entries); i++) {
		guint16 entry_tag;
		if (!_read_uint16 (&reader, &entry_tag)) {
			return FALSE;
		}

		if ((entry_tag == TIFFTAG_IMAGEWIDTH)
			|| (entry_tag == TIFFTAG_IMAGELENGTH)
			|| (entry_tag == TIFFTAG_ORIENTATION))
		{
			guint16 entry_type;
			if (!_read_uint16 (&reader, &entry_type)) {
				return FALSE;
			}

			guint32 count;
			if (!_read_uint32 (&reader, &count)) {
				return FALSE;
			}

			guint32 offset;
			if (!_read_uint32 (&reader, &offset)) {
				return FALSE;
			}

			if (count != 1) {
				return FALSE;
			}

			// Values shorter than 4 bytes are left-justified.

			guint32 value;
			if (entry_type == 1) { // BYTE
				if (reader.big_endian) {
					value = (offset >> 32);
				}
				else {
					value = offset;
				}
			}
			else if (entry_type == 3) { // SHORT (2 BYTES)
				if (reader.big_endian) {
					value = (offset >> 16);
				}
				else {
					value = offset;
				}
			}
			else if (entry_type == 4) { // LONG (4 BYTES)
				value = offset;
			}
			else {
				return FALSE;
			}

			if (entry_tag == TIFFTAG_IMAGEWIDTH) {
				image_info->width = value;
				remaining_tags--;
			}

			if (entry_tag == TIFFTAG_IMAGELENGTH) {
				image_info->height = value;
				remaining_tags--;
			}

			if (entry_tag == TIFFTAG_ORIENTATION) {
				orientation = value;
				remaining_tags--;
			}
		}
		else {
			// Skip the rest of the entry.
			_skip (&reader, 12 - 2);
		}
	}

	if (remaining_tags > 0) {
		goto next_ifd;
	}

	if (orientation >= ORIENTATION_LEFTTOP) {
		uint tmp = image_info->width;
		image_info->width = image_info->height;
		image_info->height = tmp;
	}

	return TRUE;
}
