/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2009 The Free Software Foundation, Inc.
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


#include <config.h>
#include <string.h>
#include <stdio.h>
#include <jpeglib.h>
#include <glib.h>
#include <gio/gio.h>


#define TMP_BUF_SIZE  4096
#define JPEG_ERROR(cinfo,code)  \
  ((cinfo)->err->msg_code = (code), \
   (*(cinfo)->err->error_exit) ((j_common_ptr) (cinfo)))


typedef struct {
	struct jpeg_source_mgr pub;

	JOCTET  *in_buffer;
	gsize    in_buffer_size;
	goffset  bytes_read;
	JOCTET  *tmp_buffer;
} mem_source_mgr;

typedef mem_source_mgr * mem_src_ptr;


static void
init_source (j_decompress_ptr cinfo)
{
	mem_src_ptr src = (mem_src_ptr) cinfo->src;
	src->bytes_read = 0;
}


static gboolean
fill_input_buffer (j_decompress_ptr cinfo)
{
	mem_src_ptr src = (mem_src_ptr) cinfo->src;
	size_t      nbytes;

	if (src->bytes_read + TMP_BUF_SIZE > src->in_buffer_size)
		nbytes = src->in_buffer_size - src->bytes_read;
	else
		nbytes = TMP_BUF_SIZE;

	if (nbytes <= 0) {
		if (src->bytes_read == 0)
			JPEG_ERROR (cinfo, G_IO_ERROR_NOT_FOUND);

		/* Insert a fake EOI marker */
		src->tmp_buffer[0] = (JOCTET) 0xFF;
		src->tmp_buffer[1] = (JOCTET) JPEG_EOI;
		nbytes = 2;
	}
	else
		memcpy (src->tmp_buffer, src->in_buffer + src->bytes_read, nbytes);

	src->pub.next_input_byte = src->tmp_buffer;
	src->pub.bytes_in_buffer = nbytes;
	src->bytes_read += nbytes;

	return TRUE;
}


static void
skip_input_data (j_decompress_ptr cinfo,
		 long             num_bytes)
{
	mem_src_ptr src = (mem_src_ptr) cinfo->src;

	src->bytes_read += num_bytes;
	if (src->bytes_read < 0)
		src->bytes_read = 0;
	fill_input_buffer (cinfo);
}


static void
term_source (j_decompress_ptr cinfo)
{
	/* void */
}


void
_jpeg_memory_src (j_decompress_ptr  cinfo,
		  void             *in_buffer,
		  gsize             in_buffer_size)
{
	mem_src_ptr src;

	if (cinfo->src == NULL) {
		cinfo->src = (struct jpeg_source_mgr *)
			(*cinfo->mem->alloc_small) ((j_common_ptr) cinfo,
						    JPOOL_PERMANENT,
						    sizeof (mem_source_mgr));
		src = (mem_src_ptr) cinfo->src;
		src->tmp_buffer = (JOCTET *)
			(*cinfo->mem->alloc_small) ((j_common_ptr) cinfo,
						    JPOOL_PERMANENT,
						    TMP_BUF_SIZE * sizeof(JOCTET));
	}

	src = (mem_src_ptr) cinfo->src;
	src->pub.init_source = init_source;
	src->pub.fill_input_buffer = fill_input_buffer;
	src->pub.skip_input_data = skip_input_data;
	src->pub.resync_to_restart = jpeg_resync_to_restart;
	src->pub.term_source = term_source;
	src->in_buffer = (JOCTET *) in_buffer;
	src->in_buffer_size = in_buffer_size;
	src->pub.bytes_in_buffer = 0;
	src->pub.next_input_byte = NULL;
}
