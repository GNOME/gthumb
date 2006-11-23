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
 *  along with this program; if not, wr/usr/include/jerror.hite to the Free Software
 *  Foundation, Inc., 59 Temple Street #330, Boston, MA 02111-1307, USA.
 */

#include "jpeg-memory-mgr.h"

#include <jerror.h>


/* Expanded data source object for memory input */

typedef struct {
	struct jpeg_source_mgr pub;	/* public fields */
	JOCTET *data;	/* source data buffer */
  	size_t size;	/* size of source data buffer */
} my_source_mgr;

typedef my_source_mgr * my_src_ptr;

/* Expanded data source object for memory output */

typedef struct {
	struct jpeg_destination_mgr pub;	/* public fields */
	JOCTET **data;	/* destination data buffer */
  	size_t *size;	/* size of destination data buffer */
} my_destination_mgr;

typedef my_destination_mgr * my_dest_ptr;


METHODDEF(void)
init_source (j_decompress_ptr cinfo)
{
	my_src_ptr src = (my_src_ptr) cinfo->src;

	src->pub.next_input_byte = src->data;
	src->pub.bytes_in_buffer = src->size;
}

 
METHODDEF(boolean)
fill_input_buffer (j_decompress_ptr cinfo)
{
	ERREXIT(cinfo, JERR_INPUT_EOF);
	return TRUE;
}


METHODDEF(void)
skip_input_data (j_decompress_ptr cinfo, long num_bytes)
{
	my_src_ptr src = (my_src_ptr) cinfo->src;

	if (num_bytes > 0) {
		if ((size_t)num_bytes > src->pub.bytes_in_buffer) {
			ERREXIT(cinfo, JERR_INPUT_EOF);
		}
		src->pub.next_input_byte += (size_t) num_bytes;
		src->pub.bytes_in_buffer -= (size_t) num_bytes;
	}
}


METHODDEF(void)
term_source (j_decompress_ptr cinfo)
{
	/* no work necessary here */
}

METHODDEF(void)
init_destination (j_compress_ptr cinfo)
{
	my_dest_ptr dest = (my_dest_ptr) cinfo->dest;

	dest->pub.next_output_byte = *dest->data;
	dest->pub.free_in_buffer = *dest->size;
}


METHODDEF(boolean)
empty_output_buffer (j_compress_ptr cinfo)
{
	ERREXIT(cinfo, JERR_BUFFER_SIZE);
	return TRUE;
}


METHODDEF(void)
term_destination (j_compress_ptr cinfo)
{
	my_dest_ptr dest = (my_dest_ptr) cinfo->dest;

	*dest->size -= dest->pub.free_in_buffer;
}

/*
 * Prepare for input from a memory buffer.
 * The caller must have already allocated the buffer, and is
 * responsible for freeing it after finishing decompression.
 */

GLOBAL(void)
jpeg_memory_src (j_decompress_ptr cinfo, const void *data, size_t size)
{
	my_src_ptr src = NULL;

	if (cinfo->src == NULL) {	/* first time for this JPEG object? */
		cinfo->src = (struct jpeg_source_mgr *)
			(*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_PERMANENT,
			sizeof(my_source_mgr));
	}

	src = (my_src_ptr) cinfo->src;
	src->pub.init_source = init_source;
	src->pub.fill_input_buffer = fill_input_buffer;
	src->pub.skip_input_data = skip_input_data;
	src->pub.resync_to_restart = jpeg_resync_to_restart; /* use default method */
	src->pub.term_source = term_source;
	src->pub.next_input_byte = NULL;
	src->pub.bytes_in_buffer = 0;
	src->data = (JOCTET *) data;
	src->size = (size_t) size;
}

/*
 * Prepare for output to a memory buffer.
 * The caller must have already allocated the buffer, and is
 * responsible for freeing it after finishing compression.
 */

GLOBAL(void)
jpeg_memory_dest (j_compress_ptr cinfo, void **data, size_t *size)
{
	my_dest_ptr dest = NULL;

	if (cinfo->dest == NULL) {	/* first time for this JPEG object? */
		cinfo->dest = (struct jpeg_destination_mgr *)
			(*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_PERMANENT,
			sizeof(my_destination_mgr));
	}

	dest = (my_dest_ptr) cinfo->dest;
	dest->pub.init_destination = init_destination;
	dest->pub.empty_output_buffer = empty_output_buffer;
	dest->pub.term_destination = term_destination;
	dest->pub.next_output_byte = NULL;
	dest->pub.free_in_buffer = 0;
	dest->data = (JOCTET **) data;
	dest->size = (size_t *) size;
}
