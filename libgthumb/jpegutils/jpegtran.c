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

#include <libexif/exif-data.h>

static void
jpegtran_internal (struct jpeg_decompress_struct *srcinfo,
						struct jpeg_compress_struct *dstinfo,
						JXFORM_CODE transformation,
						gboolean trim);

/* error handler data */
struct error_handler_data {
	struct jpeg_error_mgr pub;
	sigjmp_buf setjmp_buffer;
        GError **error;
	const char *filename;
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
			     GTHUMB_ERROR,
			     0,
			     "Error interpreting JPEG image file: %s\n\n%s",
			     file_name_from_path (errmgr->filename),
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


static void
update_exif_orientation(ExifData *edata)
{
	unsigned int i;
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
update_exif_dimensions (ExifData *edata, JXFORM_CODE transform)
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


#define container_of(ptr, type, member) ({			\
        const typeof( ((type *)0)->member ) *__mptr = (ptr);	\
        (type *)( (char *)__mptr - offsetof(type,member) );})

struct th {
    struct jpeg_decompress_struct src;
    struct jpeg_compress_struct   dst;
    struct jpeg_error_mgr jsrcerr, jdsterr;
    unsigned char *in;
    unsigned char *out;
    int isize, osize;
};

static void thumbnail_src_init(struct jpeg_decompress_struct *cinfo)
{
    struct th *h  = container_of(cinfo, struct th, src);
    cinfo->src->next_input_byte = h->in;
    cinfo->src->bytes_in_buffer = h->isize;
}

static int thumbnail_src_fill(struct jpeg_decompress_struct *cinfo)
{
    fprintf(stderr,"jpeg: panic: no more thumbnail input data\n");
    exit(1);
}

static void thumbnail_src_skip(struct jpeg_decompress_struct *cinfo,
			       long num_bytes)
{
    cinfo->src->next_input_byte += num_bytes;
}

static void thumbnail_src_term(struct jpeg_decompress_struct *cinfo)
{
    /* nothing */
}

static void thumbnail_dest_init(struct jpeg_compress_struct *cinfo)
{
    struct th *h  = container_of(cinfo, struct th, dst);
    h->osize = h->isize * 2;
    h->out   = malloc(h->osize);
    cinfo->dest->next_output_byte = h->out;
    cinfo->dest->free_in_buffer   = h->osize;
}

static boolean thumbnail_dest_flush(struct jpeg_compress_struct *cinfo)
{
    fprintf(stderr,"jpeg: panic: output buffer full\n");
    exit(1);
}

static void thumbnail_dest_term(struct jpeg_compress_struct *cinfo)
{
    struct th *h  = container_of(cinfo, struct th, dst);
    h->osize -= cinfo->dest->free_in_buffer;
}

static struct jpeg_source_mgr thumbnail_src = {
	.init_source         = thumbnail_src_init,
	.fill_input_buffer   = thumbnail_src_fill,
	.skip_input_data     = thumbnail_src_skip,
	.resync_to_restart   = jpeg_resync_to_restart,
	.term_source         = thumbnail_src_term,
};

static struct jpeg_destination_mgr thumbnail_dst = {
	.init_destination    = thumbnail_dest_init,
	.empty_output_buffer = thumbnail_dest_flush,
	.term_destination    = thumbnail_dest_term,
};

static void
update_exif_thumbnail (ExifData *edata, JXFORM_CODE transform)
{
	struct th th;
	
	if (edata == NULL)
		return;

	if (transform == JXFORM_NONE)
		return;

	if (edata->data && edata->data[0] == 0xff && edata->data[1] == 0xd8) {
		memset(&th, 0, sizeof(th));
		th.in    = edata->data;
		th.isize = edata->size;
		   
		/* setup src */
		th.src.err = jpeg_std_error(&th.jsrcerr);
		jpeg_create_decompress(&th.src);
		th.src.src = &thumbnail_src;

		/* setup dst */
		th.dst.err = jpeg_std_error(&th.jdsterr);
		jpeg_create_compress(&th.dst);
		th.dst.dest = &thumbnail_dst;

		/* transform image */
		jpegtran_internal(&th.src, &th.dst, transform, FALSE);

		/* cleanup */
		jpeg_destroy_decompress(&th.src);
		jpeg_destroy_compress(&th.dst);

		/* replace thumbnail */
		free(edata->data);
		edata->data = th.out;
		edata->size = th.osize;
	}
}


static void
update_exif_data(struct jpeg_decompress_struct *src, JXFORM_CODE transform)
{
	jpeg_saved_marker_ptr mark = NULL;
	ExifData *edata = NULL;
	unsigned char *data = NULL;
	unsigned int size;
   
   if (src == NULL)
		return;
   
   // Find exif data.
	for (mark = src->marker_list; mark != NULL; mark = mark->next) {
		if (mark->marker != JPEG_APP0 +1)
			continue;
		edata = exif_data_new_from_data(mark->data, mark->data_length);
		break;
	}
   if (edata == NULL)
		return;

	// Adjust exif orientation (set to top-left)
	update_exif_orientation(edata);

	// Adjust exif dimensions (swap values if necessary)
	update_exif_dimensions(edata, transform);

	// Adjust thumbnail (transform)
	update_exif_thumbnail(edata, transform);

	// Build new exif data block
	exif_data_save_data(edata, &data, &size);
	exif_data_unref(edata);

	// Update jpeg APP1 (EXIF) marker
	mark->data = src->mem->alloc_large((j_common_ptr)src, JPOOL_IMAGE, size);
	mark->original_length = size;
	mark->data_length = size;
	memcpy (mark->data, data, size);
	free(data);
}


static void
jpegtran_internal (struct jpeg_decompress_struct *srcinfo,
						struct jpeg_compress_struct *dstinfo,
						JXFORM_CODE transformation,
						gboolean trim)
{
	jpeg_transform_info            transformoption; 
	jvirt_barray_ptr              *src_coef_arrays;
	jvirt_barray_ptr              *dst_coef_arrays;

	transformoption.transform = transformation;
	transformoption.trim = trim;
	transformoption.force_grayscale = FALSE;
	
	/* Enable saving of extra markers that we want to copy */
	jcopy_markers_setup (srcinfo, JCOPYOPT_ALL);

	/* Read file header */
	(void) jpeg_read_header (srcinfo, TRUE);

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
	jcopy_markers_execute (srcinfo, dstinfo, JCOPYOPT_ALL);

	/* Execute image transformation, if any */
	jtransform_execute_transformation (srcinfo, 
					   dstinfo,
					   src_coef_arrays,
					   &transformoption);

	/* Finish compression and release memory */
	jpeg_finish_compress (dstinfo);
	jpeg_finish_decompress (srcinfo);
}


int
jpegtran (char         *input_filename,
	  char         *output_filename,
	  JXFORM_CODE   transformation,
	  gboolean	trim,
	  GError      **error)
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

	if (sigsetjmp (jsrcerr.setjmp_buffer, 1)) {
		/* Release memory and close files */
		jpeg_destroy_compress (&dstinfo);
		jpeg_destroy_decompress (&srcinfo);
		fclose (input_file);
		fclose (output_file);
		return 1;
	}

	if (sigsetjmp (jdsterr.setjmp_buffer, 1)) {
		/* Release memory and close files */
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
	jpegtran_internal(&srcinfo, &dstinfo, transformation, trim);

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
