/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*-
 *
 * f-jpeg-utils.c: Utility functions for JPEG files.
 * 
 * Copyright (C) 2001 Red Hat Inc.
 * Copyright (C) 2001 The Free Software Foundation, Inc.
 * Copyright (C) 2003 Ettore Perazzoli
 *  
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *  
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *  
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *  
 * Authors: Alexander Larsson <alexl@redhat.com>
 *          Ettore Perazzoli <ettore@perazzoli.org>
 *          Paolo Bacchilega <paolo.bacch@tin.it>
 */

#include <config.h>

#ifdef HAVE_LIBJPEG

#include <setjmp.h>
#include <stdio.h>
#include <jpeglib.h>

#include <glib.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <libgnome/gnome-i18n.h>
#include <libgnomevfs/gnome-vfs-types.h>
#include <libgnomevfs/gnome-vfs-ops.h>
#include <libgnomevfs/gnome-vfs-utils.h>

#define BUFFER_SIZE 8192

typedef struct {
	struct jpeg_source_mgr pub;	/* public fields */
	GnomeVFSHandle *handle;
	JOCTET buffer[BUFFER_SIZE];
} Source;

typedef struct {
	struct jpeg_error_mgr pub;
	jmp_buf setjmp_buffer;
} ErrorHandlerData;

static void
fatal_error_handler (j_common_ptr cinfo)
{
	ErrorHandlerData *data;

	data = (ErrorHandlerData *) cinfo->err;
	longjmp (data->setjmp_buffer, 1);
}

static void
output_message_handler (j_common_ptr cinfo)
{
	/* If we don't supply this handler, libjpeg reports errors
	 * directly to stderr.
	 */
}

static void
init_source (j_decompress_ptr cinfo)
{
}

static gboolean
fill_input_buffer (j_decompress_ptr cinfo)
{
	Source *src;
	GnomeVFSFileSize nbytes;
	GnomeVFSResult result;
	
	src = (Source *) cinfo->src;
	result = gnome_vfs_read (src->handle,
				 src->buffer,
				 G_N_ELEMENTS (src->buffer),
				 &nbytes);
	
	if (result != GNOME_VFS_OK || nbytes == 0) {
		/* return a fake EOI marker so we will eventually terminate */
		src->buffer[0] = (JOCTET) 0xFF;
		src->buffer[1] = (JOCTET) JPEG_EOI;
		nbytes = 2;
	}
	
	src->pub.next_input_byte = src->buffer;
	src->pub.bytes_in_buffer = nbytes;
	
	return TRUE;
}

static void
skip_input_data (j_decompress_ptr cinfo, long num_bytes)
{
	Source *src;

	src = (Source *) cinfo->src;
	if (num_bytes > 0) {
		while (num_bytes > (long) src->pub.bytes_in_buffer) {
			num_bytes -= (long) src->pub.bytes_in_buffer;
			fill_input_buffer (cinfo);
		}
		src->pub.next_input_byte += (size_t) num_bytes;
		src->pub.bytes_in_buffer -= (size_t) num_bytes;
	}
}

static void
term_source (j_decompress_ptr cinfo)
{
}

static void
vfs_src (j_decompress_ptr cinfo, GnomeVFSHandle *handle)
{
	Source *src;
	
	if (cinfo->src == NULL) {	/* first time for this JPEG object? */
		cinfo->src = &(g_new (Source, 1))->pub;
	}

	src = (Source *) cinfo->src;
	src->pub.init_source = init_source;
	src->pub.fill_input_buffer = fill_input_buffer;
	src->pub.skip_input_data = skip_input_data;
	src->pub.resync_to_restart = jpeg_resync_to_restart; /* use default method */
	src->pub.term_source = term_source;
	src->handle = handle;
	src->pub.bytes_in_buffer = 0; /* forces fill_input_buffer on first read */
	src->pub.next_input_byte = NULL; /* until buffer loaded */
}

static void
vfs_src_free (j_decompress_ptr cinfo)
{
	g_free (cinfo->src);
}

static int
calculate_divisor (int width,
		   int height,
		   int target_width,
		   int target_height)
{
	if (width/8 > target_width && height/8 > target_height) {
		return 8;
	}
	if (width/4 > target_width && height/4 > target_height) {
		return 4;
	}
	if (width/2 > target_width && height/2 > target_height) {
		return 2;
	}
	return 1;
}

static void
free_buffer (guchar *pixels, gpointer data)
{
	g_free (pixels);
}


static GdkPixbuf *
do_load_internal (const char *path,
		  int target_width, int target_height,
		  int *original_width_return, int *original_height_return)
{
	struct jpeg_decompress_struct cinfo;
	ErrorHandlerData jerr;
	GnomeVFSHandle *handle;
	unsigned char *lines[1];
	guchar * volatile buffer;
	guchar * volatile pixels;
	guchar *ptr;
	guchar *uri;
	GnomeVFSResult result;
	unsigned int i;

	g_return_val_if_fail (g_path_is_absolute (path), NULL);

	if (original_width_return != NULL)
		*original_width_return = 0;
	if (original_height_return != NULL)
		*original_height_return = 0;

	uri = g_strconcat ("file://", path, NULL);
	result = gnome_vfs_open (&handle, uri, GNOME_VFS_OPEN_READ);
	g_free (uri);

	if (result != GNOME_VFS_OK)
		return NULL;
	
	cinfo.err = jpeg_std_error (&jerr.pub);
	jerr.pub.error_exit = fatal_error_handler;
	jerr.pub.output_message = output_message_handler;

	buffer = NULL;
	pixels = NULL;
	if (setjmp (jerr.setjmp_buffer)) {
		/* Handle a JPEG error. */
		jpeg_destroy_decompress (&cinfo);
		gnome_vfs_close (handle);
		g_free (buffer);
		g_free (pixels);
		return NULL;
	}

	jpeg_create_decompress (&cinfo);

	vfs_src (&cinfo, handle);
	jpeg_read_header (&cinfo, TRUE);

	if (target_width != 0 && target_height != 0) {
		cinfo.scale_num = 1;
		cinfo.scale_denom = calculate_divisor (cinfo.image_width,
						       cinfo.image_height,
						       target_width,
						       target_height);
		cinfo.dct_method = JDCT_FASTEST;
		cinfo.do_fancy_upsampling = FALSE;
	
		jpeg_start_decompress (&cinfo);

		pixels = g_malloc (cinfo.output_width *	cinfo.output_height * 3);

		ptr = pixels;
		if (cinfo.num_components == 1) {
			/* Allocate extra buffer for grayscale data */
			buffer = g_malloc (cinfo.output_width);
			lines[0] = buffer;
		} else {
			lines[0] = pixels;
		}
	
		while (cinfo.output_scanline < cinfo.output_height) {
			jpeg_read_scanlines (&cinfo, lines, 1);
		
			if (cinfo.num_components == 1) {
				/* Convert grayscale to rgb */
				for (i = 0; i < cinfo.output_width; i++) {
					ptr[i*3] = buffer[i];
					ptr[i*3+1] = buffer[i];
					ptr[i*3+2] = buffer[i];
				}
				ptr += cinfo.output_width * 3;
			} else {
				lines[0] += cinfo.output_width * 3;
			}
		}

		g_free (buffer);
		buffer = NULL;

		jpeg_finish_decompress (&cinfo);
	}

	jpeg_destroy_decompress (&cinfo);

	vfs_src_free (&cinfo);
	
	gnome_vfs_close (handle);

	if (original_width_return != NULL)
		*original_width_return = cinfo.image_width;
	if (original_height_return != NULL)
		*original_height_return = cinfo.image_height;

	if (target_width == 0 || target_height == 0)
		return NULL;

	return gdk_pixbuf_new_from_data (pixels, GDK_COLORSPACE_RGB, FALSE, 8,
					 cinfo.output_width,
					 cinfo.output_height,
					 cinfo.output_width * 3,
					 free_buffer, NULL);
}


/* Public API.  */

GdkPixbuf *
f_load_scaled_jpeg  (const char *path,
		     int target_width,
		     int target_height,
		     int *original_width_return,
		     int *original_height_return)
{
	return do_load_internal (path, target_width, target_height, original_width_return, original_height_return);
}


/* FIXME: Error reporting in this function sucks...  */
void
f_get_jpeg_size  (const char *path,
		  int *width_return,
		  int *height_return)
{
	do_load_internal (path, 0, 0, width_return, height_return);
}


#endif /* HAVE_LIBJPEG */
