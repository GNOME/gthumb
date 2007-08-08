/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2001, 2002 The Free Software Foundation, Inc.
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
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Street #330, Boston, MA 02111-1307, USA.
 */

/* based upon file jpegtran.c from the libjpeg package, original copyright
 * note follows:
 *
 * jpegtran.c
 *
 * Copyright (C) 1995-1997, Thomas G. Lane.
 * This file is part of the Independent JPEG Group's software.
 * For conditions of distribution and use, see the accompanying README file.
 *
 * This file contains a command-line user interface for JPEG transcoding.
 * It is very similar to cjpeg.c, but provides lossless transcoding between
 * different JPEG file formats.  It also provides some lossless and sort-of-
 * lossless transformations of JPEG data.
 */


#include <config.h>

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <setjmp.h>
#include <string.h>

#include <jpeglib.h>
#include <glib.h>
#include "gthumb-error.h"
#include "file-utils.h"
#include "transupp.h"		/* Support routines for jpegtran */
#include "jpeg-memory-mgr.h"
#include "jpegtran.h"

#include <libexif/exif-data.h>


static int
jpegtran_thumbnail (const void   *idata,
		    size_t        isize,
		    void        **odata,
		    size_t       *osize,
		    JXFORM_CODE   transformation);


/* error handler data */
struct error_handler_data {
	struct jpeg_error_mgr   pub;
	sigjmp_buf              setjmp_buffer;
        GError                **error;
	const char             *filename;
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
        if ((errmgr->error != NULL) && (*errmgr->error == NULL))
                g_set_error (errmgr->error,
			     GTHUMB_ERROR,
			     0,
			     "Error interpreting JPEG image file: %s\n\n%s",
			     file_name_from_path (errmgr->filename),
                             buffer);

	siglongjmp (errmgr->setjmp_buffer, 1);
        g_assert_not_reached ();
}


static void
output_message_handler (j_common_ptr cinfo)
{
	/* This method keeps libjpeg from dumping crap to stderr */
	/* do nothing */
}


void
set_exif_orientation_to_top_left (ExifData *edata)
{
	unsigned int  i;
	ExifByteOrder byte_order;

	if (edata == NULL)
		return;

	byte_order = exif_data_get_byte_order (edata);

	for (i = 0; i < EXIF_IFD_COUNT; i++) {
		ExifContent *content = edata->ifd[i];
		ExifEntry   *entry;

		if ((content == NULL) || (content->count == 0))
			continue;

		entry = exif_content_get_entry (content, EXIF_TAG_ORIENTATION);
		if (entry != NULL)
			exif_set_short (entry->data, byte_order, 1);
	}
}


static void
swap_fields (ExifContent *content,
	     ExifTag      tag1,
	     ExifTag      tag2)
{
	ExifEntry     *entry1 = NULL;
	ExifEntry     *entry2 = NULL;
	unsigned char *data;
	unsigned int   size;

	entry1 = exif_content_get_entry (content, tag1);
	if (entry1 == NULL)
		return;

	entry2 = exif_content_get_entry (content, tag2);
	if (entry2 == NULL)
		return;

	data = entry1->data;
	size = entry1->size;

	entry1->data = entry2->data;
	entry1->size = entry2->size;

	entry2->data = data;
	entry2->size = size;
}


static void
update_exif_dimensions (ExifData    *edata,
		        JXFORM_CODE  transform)
{
	unsigned int i;

	if (edata == NULL)
		return;

	switch (transform) {
	case JXFORM_ROT_90:
	case JXFORM_ROT_270:
	case JXFORM_TRANSPOSE:
	case JXFORM_TRANSVERSE:
	    break;
	default:
	    return;
	}

	for (i = 0; i < EXIF_IFD_COUNT; i++) {
		ExifContent *content = edata->ifd[i];

		if ((content == NULL) || (content->count == 0))
			continue;

		swap_fields (content,
			     EXIF_TAG_RELATED_IMAGE_WIDTH,
			     EXIF_TAG_RELATED_IMAGE_LENGTH);
		swap_fields (content,
			     EXIF_TAG_IMAGE_WIDTH,
			     EXIF_TAG_IMAGE_LENGTH);
		swap_fields (content,
			     EXIF_TAG_PIXEL_X_DIMENSION,
			     EXIF_TAG_PIXEL_Y_DIMENSION);
		swap_fields (content,
			     EXIF_TAG_X_RESOLUTION,
			     EXIF_TAG_Y_RESOLUTION);
		swap_fields (content,
			     EXIF_TAG_FOCAL_PLANE_X_RESOLUTION,
			     EXIF_TAG_FOCAL_PLANE_Y_RESOLUTION);
	}
}


static void
update_exif_thumbnail (ExifData    *edata,
		       JXFORM_CODE  transform)
{
	unsigned int osize;
	unsigned char *out;

	if (edata == NULL || edata->data == NULL)
		return;

	if (transform == JXFORM_NONE)
		return;

	/* Allocate a new thumbnail buffer (twice the size of the original thumbnail).
	 * WARNING: If this buffer is too small (very unlikely, but not impossible),
	 * jpegtran will return an error and the thumbnail will be discarded.
	 * To prevent this, the size of the buffer should be increased somehow.
	 */
	osize = edata->size * 2;
	out = g_malloc (osize);

	/* Transform thumbnail */
	if (jpegtran_thumbnail (edata->data, edata->size,
			(void**)&out, &osize, transform) != 0) {
		/* Failed: Discard thumbnail */
		g_free (out);
		g_free (edata->data);
		edata->data = NULL;
		edata->size = 0;
	} else {
		/* Success: Replace thumbnail */
		g_free (edata->data);
		edata->data = out;
		edata->size = osize;
	}
}


static void
update_exif_data (struct jpeg_decompress_struct *src,
                  JXFORM_CODE                    transform)
{
	jpeg_saved_marker_ptr  mark = NULL;
	ExifData              *edata = NULL;
	unsigned char         *data = NULL;
	unsigned int           size;

	if (src == NULL)
		return;

	/* Find exif data. */
	for (mark = src->marker_list; mark != NULL; mark = mark->next) {
		if (mark->marker != JPEG_APP0 +1)
			continue;
		edata = exif_data_new_from_data(mark->data, mark->data_length);
		break;
	}
	if (edata == NULL)
		return;

	/* Adjust exif orientation (set to top-left) */
	set_exif_orientation_to_top_left (edata);

	/* Adjust exif dimensions (swap values if necessary) */
	update_exif_dimensions(edata, transform);

	/* Adjust thumbnail (transform) */
	update_exif_thumbnail(edata, transform);

	/* Build new exif data block */
	exif_data_save_data(edata, &data, &size);
	exif_data_unref(edata);

	/* Update jpeg APP1 (EXIF) marker */
	mark->data = src->mem->alloc_large((j_common_ptr)src, JPOOL_IMAGE, size);
	mark->original_length = size;
	mark->data_length = size;
	memcpy (mark->data, data, size);
	free(data);
}


static boolean
jtransform_perfect_transform(JDIMENSION image_width, JDIMENSION image_height,
			     int MCU_width, int MCU_height,
			     JXFORM_CODE transform)
{
	/* This function determines if it is possible to perform a lossless
	   jpeg transformation without trimming, based on the image dimensions
	   and MCU size. Further details at http://jpegclub.org/jpegtran. */

	boolean result = TRUE;

	switch (transform) {
	case JXFORM_FLIP_H:
	case JXFORM_ROT_270:
		if (image_width % (JDIMENSION) MCU_width)
			result = FALSE;
		break;
	case JXFORM_FLIP_V:
	case JXFORM_ROT_90:
		if (image_height % (JDIMENSION) MCU_height)
			result = FALSE;
		break;
	case JXFORM_TRANSVERSE:
	case JXFORM_ROT_180:
		if (image_width % (JDIMENSION) MCU_width)
			result = FALSE;
		if (image_height % (JDIMENSION) MCU_height)
			result = FALSE;
		break;
	default:
		break;
	}

	return result;
}


static int
jpegtran_internal (struct jpeg_decompress_struct *srcinfo,
		   struct jpeg_compress_struct   *dstinfo,
		   JXFORM_CODE                    transformation,
		   JCOPY_OPTION                   option,
		   jpegtran_mcu_callback          callback,
		   void                          *userdata)
{
	jpeg_transform_info  transformoption;
	jvirt_barray_ptr    *src_coef_arrays;
	jvirt_barray_ptr    *dst_coef_arrays;

	transformoption.transform = transformation;
	transformoption.trim = FALSE;
	transformoption.force_grayscale = FALSE;

	/* Enable saving of extra markers that we want to copy */
	jcopy_markers_setup (srcinfo, option);

	/* Read file header */
	(void) jpeg_read_header (srcinfo, TRUE);

	/* Check JPEG Minimal Coding Unit (mcu) */
	if (callback &&	! jtransform_perfect_transform (
			srcinfo->image_width,
			srcinfo->image_height,
			srcinfo->max_h_samp_factor * DCTSIZE,
			srcinfo->max_v_samp_factor * DCTSIZE,
			transformation)) {

		if (callback == JPEGTRAN_MCU_TRIM) {
			// Continue transform and trim partial MCUs
			transformoption.trim = TRUE;
		}
		else if ((callback == JPEGTRAN_MCU_CANCEL) ||
			  ! callback (&transformoption.transform, &transformoption.trim, userdata)) {
			// Abort transform
			return 1;
		}
	}

	/* Update exif data */
	update_exif_data(srcinfo, transformation);

	/* Any space needed by a transform option must be requested before
	 * jpeg_read_coefficients so that memory allocation will be done right.
	 */
	jtransform_request_workspace (srcinfo, &transformoption);

	/* Read source file as DCT coefficients */
	src_coef_arrays = jpeg_read_coefficients (srcinfo);

	/* Initialize destination compression parameters from source values */
	jpeg_copy_critical_parameters (srcinfo, dstinfo);
	
	
	/* Do not output a JFIF marker for EXIF thumbnails. 
	 * This is not the optimal way to detect the difference 
	 * between a thumbnail and a normal image, but it works
	 * well for gThumb. */
	if (option == JCOPYOPT_NONE) {
		dstinfo->write_JFIF_header = FALSE;
	}

	/* Adjust the markers to create a standard EXIF file if an EXIF marker
	 * is present in the input. By default, libjpeg creates a JFIF file, 
	 * which is incompatible with the EXIF standard. */
	jcopy_markers_exif (srcinfo, dstinfo, option);

	/* Adjust destination parameters if required by transform options;
	 * also find out which set of coefficient arrays will hold the output.
	 */
	dst_coef_arrays = jtransform_adjust_parameters (srcinfo,
							dstinfo,
							src_coef_arrays,
							&transformoption);

	/* Start compressor (note no image data is actually written here) */
	jpeg_write_coefficients (dstinfo, dst_coef_arrays);

	/* Copy to the output file any extra markers that we want to
	 * preserve */
	jcopy_markers_execute (srcinfo, dstinfo, option);

	/* Execute image transformation, if any */
	jtransform_execute_transformation (srcinfo,
					   dstinfo,
					   src_coef_arrays,
					   &transformoption);

	/* Finish compression */
	jpeg_finish_compress (dstinfo);
	jpeg_finish_decompress (srcinfo);

	return 0;
}


static int
jpegtran_thumbnail (const void   *idata,
		    size_t        isize,
		    void        **odata,
		    size_t       *osize,
		    JXFORM_CODE   transformation)
{
	struct jpeg_decompress_struct  srcinfo;
	struct jpeg_compress_struct    dstinfo;
	struct error_handler_data      jsrcerr, jdsterr;

	/* Initialize the JPEG decompression object with default error
	 * handling. */
	srcinfo.err = jpeg_std_error (&(jsrcerr.pub));
	jsrcerr.pub.error_exit = fatal_error_handler;
	jsrcerr.pub.output_message = output_message_handler;
	jsrcerr.filename = NULL;
	jsrcerr.error = NULL;

	jpeg_create_decompress (&srcinfo);

	/* Initialize the JPEG compression object with default error
	 * handling. */
	dstinfo.err = jpeg_std_error (&(jdsterr.pub));
	jdsterr.pub.error_exit = fatal_error_handler;
	jdsterr.pub.output_message = output_message_handler;
	jdsterr.filename = NULL;
	jdsterr.error = NULL;

	jpeg_create_compress (&dstinfo);

	dstinfo.err->trace_level = 0;
	dstinfo.arith_code = FALSE;
	dstinfo.optimize_coding = FALSE;

	jsrcerr.pub.trace_level = jdsterr.pub.trace_level;
	srcinfo.mem->max_memory_to_use = dstinfo.mem->max_memory_to_use;

	/* Decompression error handler */
	if (sigsetjmp (jsrcerr.setjmp_buffer, 1)) {
		jpeg_destroy_compress (&dstinfo);
		jpeg_destroy_decompress (&srcinfo);
		return 1;
	}

	/* Compression error handler */
	if (sigsetjmp (jdsterr.setjmp_buffer, 1)) {
		jpeg_destroy_compress (&dstinfo);
		jpeg_destroy_decompress (&srcinfo);
		return 1;
	}

	/* Specify data source for decompression */
	jpeg_memory_src (&srcinfo, idata, isize);

	/* Specify data destination for compression */
	jpeg_memory_dest (&dstinfo, odata, osize);

	/* Apply transformation */
	if (jpegtran_internal (&srcinfo, &dstinfo, transformation, JCOPYOPT_NONE, NULL, NULL) != 0) {
		jpeg_destroy_compress (&dstinfo);
		jpeg_destroy_decompress (&srcinfo);
		return 1;
	}

	/* Release memory */
	jpeg_destroy_compress (&dstinfo);
	jpeg_destroy_decompress (&srcinfo);

	return 0;
}


int
jpegtran (const char             *input_filename,
	  const char             *output_filename,
	  JXFORM_CODE             transformation,
	  jpegtran_mcu_callback   callback,
	  void                   *userdata,
	  GError                **error)
{
	struct jpeg_decompress_struct  srcinfo;
	struct jpeg_compress_struct    dstinfo;
	struct error_handler_data      jsrcerr, jdsterr;
	FILE                          *input_file;
	FILE                          *output_file;

	/* Open the input file. */
	input_file = fopen (input_filename, "rb");
	if (input_file == NULL)
		return 1;

	/* Open the output file. */
	output_file = fopen (output_filename, "wb");
	if (output_file == NULL) {
		fclose (input_file);
		return 1;
	}

	/* Initialize the JPEG decompression object with default error
	 * handling. */
	srcinfo.err = jpeg_std_error (&(jsrcerr.pub));
	jsrcerr.pub.error_exit = fatal_error_handler;
	jsrcerr.pub.output_message = output_message_handler;
	jsrcerr.filename = input_filename;
	jsrcerr.error = error;

	jpeg_create_decompress (&srcinfo);

	/* Initialize the JPEG compression object with default error
	 * handling. */
	dstinfo.err = jpeg_std_error (&(jdsterr.pub));
	jdsterr.pub.error_exit = fatal_error_handler;
	jdsterr.pub.output_message = output_message_handler;
	jdsterr.filename = output_filename;
	jdsterr.error = error;

	jpeg_create_compress (&dstinfo);

	dstinfo.err->trace_level = 0;
	dstinfo.arith_code = FALSE;
	dstinfo.optimize_coding = FALSE;

	jsrcerr.pub.trace_level = jdsterr.pub.trace_level;
	srcinfo.mem->max_memory_to_use = dstinfo.mem->max_memory_to_use;

	/* Decompression error handler */
	if (sigsetjmp (jsrcerr.setjmp_buffer, 1)) {
		jpeg_destroy_compress (&dstinfo);
		jpeg_destroy_decompress (&srcinfo);
		fclose (input_file);
		fclose (output_file);
		return 1;
	}

	/* Compression error handler */
	if (sigsetjmp (jdsterr.setjmp_buffer, 1)) {
		jpeg_destroy_compress (&dstinfo);
		jpeg_destroy_decompress (&srcinfo);
		fclose (input_file);
		fclose (output_file);
		return 1;
	}

	/* Specify data source for decompression */
	jpeg_stdio_src (&srcinfo, input_file);

	/* Specify data destination for compression */
	jpeg_stdio_dest (&dstinfo, output_file);

	/* Apply transformation */
	if (jpegtran_internal (&srcinfo, &dstinfo, transformation, JCOPYOPT_ALL, callback, userdata) != 0) {
		jpeg_destroy_compress (&dstinfo);
		jpeg_destroy_decompress (&srcinfo);
		fclose (input_file);
		fclose (output_file);
		return 1;
	}

	/* Release memory */
	jpeg_destroy_compress (&dstinfo);
	jpeg_destroy_decompress (&srcinfo);

	/* Close files */
	fclose (input_file);
	fclose (output_file);

	return 0;
}


#ifdef TEST


int
main (int    argc,
      char **argv)
{
	char *input_filename;
	char *output_filename;

	if (argc != 3) {
		fprintf (stderr, "%s input_image output_image", argv[0]);
		return 1;
	}

	input_filename  = argv[1];
	output_filename = argv[2];

	/*
	  JXFORM_ROT_90
	  JXFORM_ROT_180
	  JXFORM_ROT_270
	  JXFORM_FLIP_H
	  JXFORM_FLIP_V
	*/

	return jpegtran (input_filename, output_filename, JXFORM_FLIP_H);
}

#endif /* TEST */
