#include <config.h>
#include <png.h>
#if HAVE_LCMS2
#include <lcms2.h>
#endif
#include "lib/jpeg/jpeg-info.h" // For reading the color profile in EXIF data
#include "lib/gth-icc-profile.h"
#include "load-png.h"

// Starting from libpng version 1.5 it is not possible
// to access inside the PNG struct directly
#define PNG_SETJMP(ptr) setjmp(png_jmpbuf(ptr))

#ifdef PNG_LIBPNG_VER
#if PNG_LIBPNG_VER < 10400
#ifdef PNG_SETJMP
#undef PNG_SETJMP
#endif
#define PNG_SETJMP(ptr) setjmp(ptr->jmpbuf)
#endif
#endif

typedef struct {
	GBytes *bytes;
	GCancellable *cancellable;
	size_t bytes_offset;
	GError **error;
	png_struct *png_ptr;
	png_info *png_info_ptr;
	GthImage *image;
} LoaderData;


static void
loader_data_destroy (LoaderData *loader_data)
{
	png_destroy_read_struct (&loader_data->png_ptr, &loader_data->png_info_ptr, NULL);
	g_bytes_unref (loader_data->bytes);
	if (loader_data->image != NULL)
		g_object_unref (loader_data->image);
	g_free (loader_data);
}


static void
gerror_error_func (png_structp     png_ptr,
		   png_const_charp message)
{
	GError ***error_p = png_get_error_ptr (png_ptr);
	GError  **error = *error_p;

	if (error != NULL) {
		*error = g_error_new (G_IO_ERROR, G_IO_ERROR_INVALID_DATA, "%s", message);
	}
}


static void
gerror_warning_func (png_structp     png_ptr,
		     png_const_charp message)
{
	// We don't care about warnings.
}


static void
read_buffer_func (png_structp png_ptr,
		  png_bytep   buffer,
		  png_size_t  size)
{
	LoaderData *loader_data = png_get_io_ptr (png_ptr);
	gsize max_size;
	gconstpointer bytes = g_bytes_get_data (loader_data->bytes, &max_size);
	if (loader_data->bytes_offset + size > max_size) {
		png_error (png_ptr, "Wrong size");
		return;
	}
	memcpy (buffer, bytes + loader_data->bytes_offset, size);
	loader_data->bytes_offset += size;
}


static void
transform_to_argb32_format_func (png_structp   png,
				 png_row_infop row_info,
				 png_bytep     data)
{
	for (guint i = 0; i < row_info->rowbytes; i += 4) {
		guchar *p_iter = data + i;
		guint32 pixel;
		guchar a = p_iter[3];
		if (a == 0xFF) {
			pixel = RGBA_TO_PIXEL (p_iter[0], p_iter[1], p_iter[2], 0xFF);
		}
		else if (a == 0) {
			pixel = 0;
		}
		else {
			pixel = pixel_from_rgba_multiply_alpha (p_iter[0], p_iter[1], p_iter[2], p_iter[3]);
		}
		memcpy (p_iter, &pixel, sizeof (guint32));
	}
}


GthImage *
load_png (GBytes	 *bytes,
	  guint		  requested_size,
	  GCancellable	 *cancellable,
	  GError	**error)
{
	if (bytes == NULL) {
		g_set_error_literal (error,
			G_IO_ERROR,
			G_IO_ERROR_FAILED,
			"Buffer is null");
		return NULL;
	}

	LoaderData *loader_data = g_new0 (LoaderData, 1);
	loader_data->bytes = g_bytes_ref (bytes);
	loader_data->cancellable = cancellable;
	loader_data->bytes_offset = 0;
	loader_data->image = NULL;
	loader_data->error = error;
	loader_data->png_ptr = png_create_read_struct (
		PNG_LIBPNG_VER_STRING,
		&loader_data->error,
		gerror_error_func,
		gerror_warning_func);

	if (loader_data->png_ptr == NULL) {
		loader_data_destroy (loader_data);
		g_set_error_literal (error,
			G_IO_ERROR,
			G_IO_ERROR_FAILED,
			"Could not create the png struct");
		return NULL;
	}

	loader_data->png_info_ptr = png_create_info_struct (loader_data->png_ptr);
	if (loader_data->png_info_ptr == NULL) {
		loader_data_destroy (loader_data);
		g_set_error_literal (error,
			G_IO_ERROR,
			G_IO_ERROR_FAILED,
			"Could not create the png info struct");
		return NULL;
	}

	if (PNG_SETJMP (loader_data->png_ptr)) {
		loader_data_destroy (loader_data);
		return NULL;
	}

	png_set_read_fn (loader_data->png_ptr, loader_data, read_buffer_func);
#ifndef HAVE_LCMS2
	png_set_gamma (loader_data->png_ptr, PNG_DEFAULT_sRGB, PNG_DEFAULT_sRGB);
#endif
	png_read_info (loader_data->png_ptr, loader_data->png_info_ptr);

	png_uint_32 width, height;
	int bit_depth, color_type, interlace_type;

	png_get_IHDR (
		loader_data->png_ptr,
		loader_data->png_info_ptr,
		&width,
		&height,
		&bit_depth,
		&color_type,
		&interlace_type,
		NULL,
		NULL);

	loader_data->image = gth_image_new (width, height);
	if (loader_data->image == NULL) {
		g_set_error_literal (error,
			G_IO_ERROR,
			G_IO_ERROR_FAILED,
			"Not enough memory?");
		loader_data_destroy (loader_data);
		return NULL;
	}
	gth_image_set_has_alpha (loader_data->image, (color_type & PNG_COLOR_MASK_ALPHA) || (color_type & PNG_COLOR_MASK_PALETTE));
	gth_image_set_original_size (loader_data->image, width, height);

	// Set the data transformations.

	png_set_strip_16 (loader_data->png_ptr);

	png_set_packing (loader_data->png_ptr);

	if (color_type == PNG_COLOR_TYPE_PALETTE) {
		png_set_palette_to_rgb (loader_data->png_ptr);
	}

	if ((color_type == PNG_COLOR_TYPE_GRAY) && (bit_depth < 8)) {
		png_set_expand_gray_1_2_4_to_8 (loader_data->png_ptr);
	}

	if (png_get_valid (loader_data->png_ptr, loader_data->png_info_ptr, PNG_INFO_tRNS)) {
	      png_set_tRNS_to_alpha (loader_data->png_ptr);
	}

	png_set_filler (loader_data->png_ptr, 0xff, PNG_FILLER_AFTER);

	if ((color_type == PNG_COLOR_TYPE_GRAY) || (color_type == PNG_COLOR_TYPE_GRAY_ALPHA)) {
		png_set_gray_to_rgb (loader_data->png_ptr);
	}

	if (interlace_type != PNG_INTERLACE_NONE) {
		png_set_interlace_handling (loader_data->png_ptr);
	}

	png_set_read_user_transform_fn (loader_data->png_ptr, transform_to_argb32_format_func);

	png_read_update_info (loader_data->png_ptr, loader_data->png_info_ptr);

	// Read the image.

	int row_stride;
	guchar *surface_row = gth_image_get_pixels (loader_data->image, NULL, &row_stride);
	png_bytep *row_pointers = g_new (png_bytep, height);
	for (int row = 0; row < height; row++) {
		row_pointers[row] = surface_row;
		surface_row += row_stride;
	}
	png_read_image (loader_data->png_ptr, row_pointers);
	png_read_end (loader_data->png_ptr, loader_data->png_info_ptr);

	png_textp text_ptr;
	int num_texts;

	if (png_get_text (loader_data->png_ptr,
		loader_data->png_info_ptr,
		&text_ptr,
		&num_texts))
	{
		for (int i = 0; i < num_texts; i++) {
			int original_image_width;
			int original_image_height;
			if (strcmp (text_ptr[i].key, "Thumb::Image::Width") == 0) {
				original_image_width = atoi (text_ptr[i].text);
			}
			else if (strcmp (text_ptr[i].key, "Thumb::Image::Height") == 0) {
				original_image_height = atoi (text_ptr[i].text);
			}
			gth_image_set_original_image_size (loader_data->image,
				original_image_width,
				original_image_height);
		}
	}

	g_free (row_pointers);

#if HAVE_LCMS2
	{
		GthICCProfile *profile = NULL;
		int            intent;
		png_charp      name;
		int            compression_type;
		png_bytep      icc_data;
		png_uint_32    icc_data_size;
		double         gamma;
		png_bytep      exif_data;
		png_uint_32    exif_data_size;

		if (png_get_sRGB (loader_data->png_ptr,
				  loader_data->png_info_ptr,
				  &intent) == PNG_INFO_sRGB)
		{
			profile = gth_icc_profile_new_srgb ();
		}
		else if (png_get_iCCP (loader_data->png_ptr,
				       loader_data->png_info_ptr,
				       &name,
				       &compression_type,
				       &icc_data,
				       &icc_data_size) == PNG_INFO_iCCP)
		{
			if ((icc_data_size > 0) && (icc_data != NULL)) {
				profile = gth_icc_profile_new (GTH_ICC_PROFILE_ID_UNKNOWN,
					cmsOpenProfileFromMem (icc_data, icc_data_size));
			}
		}
		else if (png_get_gAMA (loader_data->png_ptr,
				       loader_data->png_info_ptr,
				       &gamma))
		{
			profile = gth_icc_profile_new_srgb_with_gamma (1.0 / gamma);
		}
#ifdef PNG_eXIf_SUPPORTED
		else if (png_get_eXIf_1 (loader_data->png_ptr,
					 loader_data->png_info_ptr,
					 &exif_data_size,
					 &exif_data))
		{
			JpegInfoData jpeg_info;
			_jpeg_info_data_init (&jpeg_info);
			if (_read_exif_data_tags (exif_data, exif_data_size, _JPEG_INFO_EXIF_COLOR_SPACE, &jpeg_info)) {
				if (jpeg_info.valid & _JPEG_INFO_EXIF_COLOR_SPACE) {
					if (jpeg_info.color_space == GTH_COLOR_SPACE_SRGB) {
						profile = gth_icc_profile_new_srgb ();
					}
					else if (jpeg_info.color_space == GTH_COLOR_SPACE_ADOBERGB) {
						profile = gth_icc_profile_new_adobergb ();
					}
				}
			}
		}
#endif
		if (profile != NULL) {
			gth_image_set_icc_profile (loader_data->image, profile);
			g_object_unref (profile);
		}
	}
#endif

	GthImage *result = g_object_ref (loader_data->image);
	loader_data_destroy (loader_data);
	return result;
}
