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

#ifdef HAVE_LIBJPEG


#include <stdio.h>
#include <unistd.h>
#include <jpeglib.h>
#include "transupp.h"		/* Support routines for jpegtran */


int
jpegtran (char        *input_filename,
	  char        *output_filename,
	  JXFORM_CODE  transformation)
{
	struct jpeg_decompress_struct  srcinfo;
	struct jpeg_compress_struct    dstinfo;
	struct jpeg_error_mgr          jsrcerr, jdsterr;
	jpeg_transform_info            transformoption; 
	jvirt_barray_ptr              *src_coef_arrays;
	jvirt_barray_ptr              *dst_coef_arrays;
	FILE                          *input_file;
	FILE                          *output_file;

	transformoption.transform = transformation;
	transformoption.trim = FALSE;
	transformoption.force_grayscale = FALSE;
	
	/* Initialize the JPEG decompression object with default error 
	 * handling. */
	srcinfo.err = jpeg_std_error (&jsrcerr);
	jpeg_create_decompress (&srcinfo);

	/* Initialize the JPEG compression object with default error 
	 * handling. */
	dstinfo.err = jpeg_std_error (&jdsterr);
	jpeg_create_compress (&dstinfo);
	
	dstinfo.err->trace_level = 0;
	dstinfo.arith_code = FALSE;
	dstinfo.optimize_coding = FALSE;

	jsrcerr.trace_level = jdsterr.trace_level;
	srcinfo.mem->max_memory_to_use = dstinfo.mem->max_memory_to_use;

	/* Open the input file. */
	input_file = fopen (input_filename, "rb");
	if (input_file == NULL) 
		return 1;

	output_file = fopen (output_filename, "wb");
	if (output_file == NULL) {
		fclose (input_file);
		return 1;
	}

	/* Specify data source for decompression */
	jpeg_stdio_src (&srcinfo, input_file);

	/* Enable saving of extra markers that we want to copy */
	jcopy_markers_setup (&srcinfo, JCOPYOPT_ALL);

	/* Read file header */
	(void) jpeg_read_header (&srcinfo, TRUE);

	/* Any space needed by a transform option must be requested before
	 * jpeg_read_coefficients so that memory allocation will be done right.
	 */
	jtransform_request_workspace (&srcinfo, &transformoption);

	/* Read source file as DCT coefficients */
	src_coef_arrays = jpeg_read_coefficients (&srcinfo);

	/* Initialize destination compression parameters from source values */
	jpeg_copy_critical_parameters (&srcinfo, &dstinfo);

	/* Adjust destination parameters if required by transform options;
	 * also find out which set of coefficient arrays will hold the output.
	 */
	dst_coef_arrays = jtransform_adjust_parameters (&srcinfo, 
							&dstinfo,
							src_coef_arrays,
							&transformoption);

	/* Specify data destination for compression */
	jpeg_stdio_dest (&dstinfo, output_file);

	/* Start compressor (note no image data is actually written here) */
	jpeg_write_coefficients (&dstinfo, dst_coef_arrays);

	/* Copy to the output file any extra markers that we want to 
	 * preserve */
	jcopy_markers_execute (&srcinfo, &dstinfo, JCOPYOPT_ALL);

	/* Execute image transformation, if any */
	jtransform_execute_transformation (&srcinfo, 
					   &dstinfo,
					   src_coef_arrays,
					   &transformoption);

	/* Finish compression and release memory */
	jpeg_finish_compress (&dstinfo);
	jpeg_destroy_compress (&dstinfo);
	(void) jpeg_finish_decompress (&srcinfo);
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

#endif /* HAVE_LIBJPEG */
