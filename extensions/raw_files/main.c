/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2009 Free Software Foundation, Inc.
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
#include <glib.h>
#include "main.h"


const char *raw_mime_types[] = {
	"image/x-adobe-dng",
	"image/x-canon-cr2",
	"image/x-canon-crw",
	"image/x-epson-erf",
	"image/x-minolta-mrw",
	"image/x-nikon-nef",
	"image/x-olympus-orf",
	"image/x-pentax-pef",
	"image/x-sony-arw",
	NULL };


#ifdef HAVE_LIBRAW


#include <cairo.h>
#include <gtk/gtk.h>
#include <gthumb.h>
#include <libraw.h>
#include "gth-metadata-provider-raw.h"


#define RAW_USE_EMBEDDED_THUMBNAIL 1


typedef enum {
	RAW_OUTPUT_COLOR_RAW = 0,
	RAW_OUTPUT_COLOR_SRGB = 1,
	RAW_OUTPUT_COLOR_ADOBE = 2,
	RAW_OUTPUT_COLOR_WIDE = 3,
	RAW_OUTPUT_COLOR_PROPHOTO = 4,
	RAW_OUTPUT_COLOR_XYZ = 5
} RawOutputColor;


static void
_libraw_set_gerror (GError **error,
		    int      error_code)
{
	g_set_error_literal (error,
			     G_IO_ERROR,
			     G_IO_ERROR_FAILED,
			     libraw_strerror (error_code));
}


static GthImage *
_libraw_read_jpeg_data (void           *buffer,
			gsize           buffer_size,
			int             requested_size,
			GCancellable   *cancellable,
			GError        **error)
{
	GthImageLoaderFunc  loader_func;
	GInputStream       *istream;
	GthImage           *image;

	loader_func = gth_main_get_image_loader_func ("image/jpeg", GTH_IMAGE_FORMAT_CAIRO_SURFACE);
	if (loader_func == NULL)
		return NULL;

	istream = g_memory_input_stream_new_from_data (buffer, buffer_size, NULL);
	if (istream == NULL)
		return NULL;

	image = loader_func (istream,
			     NULL,
			     requested_size,
			     NULL,
			     NULL,
			     NULL,
			     NULL,
			     cancellable,
			     error);

	g_object_unref (istream);

	return image;
}


static cairo_surface_t *
_cairo_surface_create_from_ppm (int     width,
				int     height,
				int     colors,
				int     bits,
				guchar *buffer,
				gsize   buffer_size)
{
	cairo_surface_t *surface;
	int              stride;
	guchar          *buffer_p;
	int              r, c;
	guchar          *row;
	guchar          *column;
	guint32          pixel;

	if (bits != 8)
		return NULL;

	surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, width, height);
	stride = cairo_image_surface_get_stride (surface);

	buffer_p = buffer;
	row = _cairo_image_surface_flush_and_get_data (surface);
	for (r = 0; r < height; r++) {
		column = row;
		for (c = 0; c < width; c++) {
			switch (colors) {
			case 4:
				pixel = CAIRO_RGBA_TO_UINT32 (buffer_p[0], buffer_p[1], buffer_p[2], buffer_p[3]);
				break;
			case 3:
				pixel = CAIRO_RGBA_TO_UINT32 (buffer_p[0], buffer_p[1], buffer_p[2], 0xff);
				break;
			case 1:
				pixel = CAIRO_RGBA_TO_UINT32 (buffer_p[0], buffer_p[0], buffer_p[0], 0xff);
				break;
			default:
				g_assert_not_reached ();
			}

			memcpy (column, &pixel, sizeof (guint32));

			column += 4;
			buffer_p += colors;
		}
		row += stride;
	}

	cairo_surface_mark_dirty (surface);

	return surface;
}


static GthImage *
_libraw_read_bitmap_data (int     width,
			  int     height,
			  int     colors,
			  int     bits,
			  guchar *buffer,
			  gsize   buffer_size)
{
	GthImage        *image = NULL;
	cairo_surface_t *surface = NULL;

	surface = _cairo_surface_create_from_ppm (width, height, colors, bits, buffer, buffer_size);
	if (surface != NULL) {
		image = gth_image_new_for_surface (surface);
		cairo_surface_destroy (surface);
	}

	return image;
}


#ifdef RAW_USE_EMBEDDED_THUMBNAIL


static GthTransform
_libraw_get_tranform (libraw_data_t *raw_data)
{
	GthTransform transform;

	switch (raw_data->sizes.flip) {
	case 3:
		transform = GTH_TRANSFORM_ROTATE_180;
		break;
	case 5:
		transform = GTH_TRANSFORM_ROTATE_270;
		break;
	case 6:
		transform = GTH_TRANSFORM_ROTATE_90;
		break;
	default:
		transform = GTH_TRANSFORM_NONE;
		break;
	}

	return transform;
}


#endif


static int
_libraw_progress_cb (void                 *callback_data,
		     enum LibRaw_progress  stage,
		     int                   iteration,
		     int                   expected)
{
	return g_cancellable_is_cancelled ((GCancellable *) callback_data) ? 1 : 0;
}


static GthImage *
_cairo_image_surface_create_from_raw (GInputStream  *istream,
				      GthFileData   *file_data,
				      int            requested_size,
				      int           *original_width,
				      int           *original_height,
				      gboolean      *loaded_original,
				      gpointer       user_data,
				      GCancellable  *cancellable,
				      GError       **error)
{
	libraw_data_t *raw_data;
	int            result;
	void          *buffer = NULL;
	size_t         size;
	GthImage      *image = NULL;

	raw_data = libraw_init (LIBRAW_OPIONS_NO_MEMERR_CALLBACK | LIBRAW_OPIONS_NO_DATAERR_CALLBACK);
	if (raw_data == NULL) {
		_libraw_set_gerror (error, errno);
		goto fatal_error;
	}

	libraw_set_progress_handler (raw_data, _libraw_progress_cb, cancellable);

	if (! _g_input_stream_read_all (istream, &buffer, &size, cancellable, error))
		goto fatal_error;

	raw_data->params.output_tiff = FALSE;
	raw_data->params.use_camera_wb = TRUE;
	raw_data->params.use_rawspeed = TRUE;
	raw_data->params.highlight = FALSE;
	raw_data->params.use_camera_matrix = TRUE;
	raw_data->params.output_color = RAW_OUTPUT_COLOR_SRGB;
	raw_data->params.output_bps = 8;
	raw_data->params.half_size = (requested_size > 0);

	result = libraw_open_buffer (raw_data, buffer, size);
	if (LIBRAW_FATAL_ERROR (result)) {
		_libraw_set_gerror (error, result);
		goto fatal_error;
	}

	/*  */

#if RAW_USE_EMBEDDED_THUMBNAIL

	if (requested_size > 0) {

		if (loaded_original != NULL)
			*loaded_original = FALSE;

		/* read the thumbnail */

		result = libraw_unpack_thumb (raw_data);
		if (result != LIBRAW_SUCCESS) {
			_libraw_set_gerror (error, result);
			goto fatal_error;
		}

		switch (raw_data->thumbnail.tformat) {
		case LIBRAW_THUMBNAIL_JPEG:
			image = _libraw_read_jpeg_data (raw_data->thumbnail.thumb,
							raw_data->thumbnail.tlength,
							requested_size,
							cancellable,
							error);
			break;
		case LIBRAW_THUMBNAIL_BITMAP:
			image = _libraw_read_bitmap_data (raw_data->thumbnail.twidth,
							  raw_data->thumbnail.theight,
							  raw_data->thumbnail.tcolors,
							  8,
							  (guchar *) raw_data->thumbnail.thumb,
							  raw_data->thumbnail.tlength);
			break;
		default:
			g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_INVALID_DATA, "Unsupported data format");
			break;
		}

		if ((image != NULL) && (_libraw_get_tranform (raw_data) != GTH_TRANSFORM_NONE)) {
			cairo_surface_t *surface;
			cairo_surface_t *rotated;

			surface = gth_image_get_cairo_surface (image);
			rotated = _cairo_image_surface_transform (surface, _libraw_get_tranform (raw_data));
			gth_image_set_cairo_surface (image, rotated);

			cairo_surface_destroy (rotated);
			cairo_surface_destroy (surface);
		}

		/* get the original size */

		if ((original_width != NULL) && (original_height != NULL)) {
			libraw_close (raw_data);

			raw_data = libraw_init (LIBRAW_OPIONS_NO_MEMERR_CALLBACK | LIBRAW_OPIONS_NO_DATAERR_CALLBACK);
			if (raw_data == NULL)
				goto fatal_error;

			result = libraw_open_buffer (raw_data, buffer, size);
			if (LIBRAW_FATAL_ERROR (result))
				goto fatal_error;

			result = libraw_unpack (raw_data);
			if (result != LIBRAW_SUCCESS) {
				_libraw_set_gerror (error, result);
				goto fatal_error;
			}

			result = libraw_adjust_sizes_info_only (raw_data);
			if (result != LIBRAW_SUCCESS) {
				_libraw_set_gerror (error, result);
				goto fatal_error;
			}

			*original_width = raw_data->sizes.iwidth;
			*original_height = raw_data->sizes.iheight;
		}
	}
	else

#endif

	{
		/* read the image */

		libraw_processed_image_t *processed_image;

		result = libraw_unpack (raw_data);
		if (result != LIBRAW_SUCCESS) {
			_libraw_set_gerror (error, result);
			goto fatal_error;
		}

		result = libraw_dcraw_process (raw_data);
		if (result != LIBRAW_SUCCESS) {
			_libraw_set_gerror (error, result);
			goto fatal_error;
		}

		processed_image = libraw_dcraw_make_mem_image (raw_data, &result);
		if (result != LIBRAW_SUCCESS) {
			_libraw_set_gerror (error, result);
			goto fatal_error;
		}

		switch (processed_image->type) {
		case LIBRAW_IMAGE_JPEG:
			image = _libraw_read_jpeg_data (processed_image->data,
							processed_image->data_size,
							-1,
							cancellable,
							error);
			break;
		case LIBRAW_IMAGE_BITMAP:
			image = _libraw_read_bitmap_data (processed_image->width,
							  processed_image->height,
							  processed_image->colors,
							  processed_image->bits,
							  processed_image->data,
							  processed_image->data_size);
			break;
		default:
			g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_INVALID_DATA, "Unsupported data format");
			break;
		}

		libraw_dcraw_clear_mem (processed_image);

		/* get the original size */

		if ((original_width != NULL) && (original_height != NULL)) {
			result = libraw_adjust_sizes_info_only (raw_data);
			if (result != LIBRAW_SUCCESS) {
				_libraw_set_gerror (error, result);
				goto fatal_error;
			}

			*original_width = raw_data->sizes.iwidth;
			*original_height = raw_data->sizes.iheight;
		}
	}

	fatal_error:

	if (raw_data != NULL)
		libraw_close (raw_data);
	g_free (buffer);

	return image;
}


G_MODULE_EXPORT void
gthumb_extension_activate (void)
{
	gth_main_register_metadata_provider (GTH_TYPE_METADATA_PROVIDER_RAW);
	gth_main_register_image_loader_func_v (_cairo_image_surface_create_from_raw,
					       GTH_IMAGE_FORMAT_CAIRO_SURFACE,
					       raw_mime_types);
}


#else /* ! HAVE_LIBRAW */


#define GDK_PIXBUF_ENABLE_BACKEND
#include <gtk/gtk.h>
#include <gthumb.h>


static gboolean
_g_mime_type_is_raw (const char *mime_type)
{
	return (g_content_type_is_a (mime_type, "application/x-crw")	/* ? */
		|| g_content_type_is_a (mime_type, "image/x-raw")       /* mimelnk */
		|| g_content_type_is_a (mime_type, "image/x-dcraw"));	/* freedesktop.org.xml - this should
									   catch most RAW formats, which are
									   registered as sub-classes of
									   image/x-dcraw */
}


static gboolean
_g_mime_type_is_hdr (const char *mime_type)
{
	/* Note that some HDR file extensions have been hard-coded into
	   the get_file_mime_type function above. */
	return g_content_type_is_a (mime_type, "image/x-hdr");
}


static char *
get_cache_full_path (const char *filename,
		     const char *extension)
{
	char  *name;
	GFile *file;
	char  *cache_filename;

	if (extension == NULL)
		name = g_strdup (filename);
	else
		name = g_strconcat (filename, ".", extension, NULL);
	file = gth_user_dir_get_file_for_write (GTH_DIR_CACHE, GTHUMB_DIR, name, NULL);
	cache_filename = g_file_get_path (file);

	g_object_unref (file);
	g_free (name);

	return cache_filename;
}


static time_t
get_file_mtime (const char *path)
{
	GFile  *file;
	time_t  t;

	file = g_file_new_for_path (path);
	t = _g_file_get_mtime (file);
	g_object_unref (file);

	return t;
}


static GthImage *
dcraw_pixbuf_animation_new_from_file (GInputStream  *istream,
					GthFileData   *file_data,
					int            requested_size,
					int           *original_width,
					int           *original_height,
					gboolean      *loaded_original,
					gpointer       user_data,
					GCancellable  *cancellable,
					GError       **error)
{
	GthImage    *image = NULL;
	GdkPixbuf   *pixbuf;
	gboolean     is_thumbnail;
	gboolean     is_raw;
	gboolean     is_hdr;
	char        *local_file;
	char         *local_file_md5;
	char	     *cache_file;
	char	     *cache_file_esc;
	char	     *local_file_esc;
	char	     *command = NULL;

	if (file_data == NULL) {
		if (error != NULL)
			*error = g_error_new_literal (G_IO_ERROR, G_IO_ERROR_INVALID_FILENAME, "Could not load file");
		return NULL;
	}

	is_thumbnail = requested_size > 0;
	is_raw = _g_mime_type_is_raw (gth_file_data_get_mime_type (file_data));
	is_hdr = _g_mime_type_is_hdr (gth_file_data_get_mime_type (file_data));

	/* The output filename, and its persistence, depend on the input file
	 * type, and whether or not a thumbnail has been requested. */

	local_file = g_file_get_path (file_data->file);
	local_file_md5 = gnome_desktop_thumbnail_md5 (local_file);

	if (is_raw && !is_thumbnail)
		/* Full-sized converted RAW file */
		cache_file = get_cache_full_path (local_file_md5, "conv.pnm");
	else if (is_raw && is_thumbnail)
		/* RAW: thumbnails generated in pnm format. The converted file is later removed. */
		cache_file = get_cache_full_path (local_file_md5, "conv-thumb.pnm");
	else if (is_hdr && is_thumbnail)
		/* HDR: thumbnails generated in tiff format. The converted file is later removed. */
		cache_file = get_cache_full_path (local_file_md5, "conv-thumb.tiff");
	else
		/* Full-sized converted HDR files */
		cache_file = get_cache_full_path (local_file_md5, "conv.tiff");

	g_free (local_file_md5);

	if (cache_file == NULL) {
		g_free (local_file);
		return NULL;
	}

	local_file_esc = g_shell_quote (local_file);
	cache_file_esc = g_shell_quote (cache_file);

	/* Do nothing if an up-to-date converted file is already in the cache */
	if (! g_file_test (cache_file, G_FILE_TEST_EXISTS)
	    || (gth_file_data_get_mtime (file_data) > get_file_mtime (cache_file)))
	{
		if (is_raw) {
			if (is_thumbnail) {
				char *first_part;
				char *jpg_thumbnail;
				char *tiff_thumbnail;
				char *ppm_thumbnail;
				char *thumb_command;

				thumb_command = g_strdup_printf ("dcraw -e %s", local_file_esc);
				g_spawn_command_line_sync (thumb_command, NULL, NULL, NULL, NULL);
				g_free (thumb_command);

				first_part = _g_uri_remove_extension (local_file);
				jpg_thumbnail = g_strdup_printf ("%s.thumb.jpg", first_part);
				tiff_thumbnail = g_strdup_printf ("%s.thumb.tiff", first_part);
				ppm_thumbnail = g_strdup_printf ("%s.thumb.ppm", first_part);

				if (g_file_test (jpg_thumbnail, G_FILE_TEST_EXISTS)) {
					g_free (cache_file);
					cache_file = g_strdup (jpg_thumbnail);
				}
				else if (g_file_test (tiff_thumbnail, G_FILE_TEST_EXISTS)) {
					g_free (cache_file);
					cache_file = g_strdup (tiff_thumbnail);
				}
				else if (g_file_test (ppm_thumbnail, G_FILE_TEST_EXISTS)) {
					g_free (cache_file);
					cache_file = g_strdup (ppm_thumbnail);
				}
				else {
					/* No embedded thumbnail. Read the whole file. */
					/* Add -h option to speed up thumbnail generation. */
					command = g_strdup_printf ("dcraw -w -c -h %s > %s",
								   local_file_esc,
								   cache_file_esc);
				}

				g_free (first_part);
				g_free (jpg_thumbnail);
				g_free (tiff_thumbnail);
				g_free (ppm_thumbnail);
			}
			else {
				/* -w option = camera-specified white balance */
				command = g_strdup_printf ("dcraw -w -c %s > %s",
							   local_file_esc,
							   cache_file_esc);
			}
		}

		if (is_hdr) {
			/* HDR files. We can use the pfssize tool to speed up
			   thumbnail generation considerably, so we treat
			   thumbnailing as a special case. */
			char *resize_command;

			if (is_thumbnail)
				resize_command = g_strdup_printf (" | pfssize --maxx %d --maxy %d",
								  requested_size,
								  requested_size);
			else
				resize_command = g_strdup_printf (" ");

			command = g_strconcat ( "pfsin ",
						local_file_esc,
						resize_command,
						" |  pfsclamp  --rgb  | pfstmo_drago03 | pfsout ",
						cache_file_esc,
						NULL );
			g_free (resize_command);
		}

		if (command != NULL) {
			if (system (command) == -1) {
				g_free (command);
				g_free (cache_file_esc);
				g_free (local_file_esc);
				g_free (cache_file);
				g_free (local_file);

				return NULL;
			}
			g_free (command);
		}
	}

	pixbuf = gdk_pixbuf_new_from_file (cache_file, NULL);

	/* Thumbnail files are already cached, so delete the conversion cache copies */
	if (is_thumbnail) {
		GFile *file;

		file = g_file_new_for_path (cache_file);
		g_file_delete (file, NULL, NULL);
		g_object_unref (file);
	}

	if (pixbuf != NULL) {
		image = gth_image_new_for_pixbuf (pixbuf);
		g_object_unref (pixbuf);
	}

	g_free (cache_file_esc);
	g_free (local_file_esc);
	g_free (cache_file);
	g_free (local_file);

	return image;
}


G_MODULE_EXPORT void
gthumb_extension_activate (void)
{
	gth_main_register_image_loader_func_v (dcraw_pixbuf_animation_new_from_file,
					       GTH_IMAGE_FORMAT_GDK_PIXBUF,
					       raw_mime_types);
}


#endif


G_MODULE_EXPORT void
gthumb_extension_deactivate (void)
{
}


G_MODULE_EXPORT gboolean
gthumb_extension_is_configurable (void)
{
	return FALSE;
}


G_MODULE_EXPORT void
gthumb_extension_configure (GtkWindow *parent)
{
}
