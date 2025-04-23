/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2011-2015 Free Software Foundation, Inc.
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
#include "jpeg-info.h"


void
_jpeg_info_data_init (JpegInfoData *data)
{
	data->valid = _JPEG_INFO_NONE;
	data->width = 0;
	data->height = 0;
	data->orientation = GTH_TRANSFORM_NONE;
	data->icc_data = NULL;
	data->icc_data_size = 0;
}


void
_jpeg_info_data_dispose (JpegInfoData *data)
{
	if (data->valid & _JPEG_INFO_ICC_PROFILE)
		g_free (data->icc_data);
}


static guchar
_g_input_stream_read_byte (GInputStream  *stream,
			   GCancellable  *cancellable,
			   GError       **error)
{
	guchar v;
	return (g_input_stream_read (stream, &v, 1, cancellable, error) > 0) ? v : 0;
}


static guchar
_jpeg_read_segment_marker (GInputStream  *stream,
			   GCancellable  *cancellable,
			   GError       **error)
{
	guchar marker_id;

	if (_g_input_stream_read_byte (stream, cancellable, error) != 0xff)
		return 0x00;

	while ((marker_id = _g_input_stream_read_byte (stream, cancellable, error)) == 0xff)
		/* skip padding */;

	return marker_id;
}


static gboolean
_jpeg_skip_segment_data (GInputStream  *stream,
			 guchar         marker_id,
			 GCancellable  *cancellable,
			 GError       **error)
{
	if (marker_id == 0xd9)  /* EOI => end of image */
		return FALSE;
	if (marker_id == 0xda)  /* SOS => end of header */
		return FALSE;

	if ((marker_id != 0xd0)
	    && (marker_id != 0xd1)
	    && (marker_id != 0xd2)
	    && (marker_id != 0xd3)
	    && (marker_id != 0xd4)
	    && (marker_id != 0xd5)
	    && (marker_id != 0xd6)
	    && (marker_id != 0xd7)
	    && (marker_id != 0xd8)
	    && (marker_id != 0x01))
	{
		guint h, l;
		guint segment_size;

		/* skip to the next segment */

		h = _g_input_stream_read_byte (stream, cancellable, error);
		l = _g_input_stream_read_byte (stream, cancellable, error);
		segment_size = (h << 8) + l;

		if (g_input_stream_skip (stream, segment_size - 2, cancellable, error) < 0)
			return FALSE;
	}

	return TRUE;
}


static gboolean
_jpeg_exif_tags_from_app1_segment (guchar	 *in_buffer,
				   gsize	  app1_segment_size,
				   JpegInfoFlags  flags,
				   JpegInfoData	 *data)
{
	//g_print ("> _jpeg_exif_tags_from_app1_segment\n");

	/* Length includes itself, so must be at least 2 */
	/* Following Exif data length must be at least 6 */

	if (app1_segment_size < 6)
		return FALSE;

	/* Read Exif head, check for "Exif" */

	int pos = 0;
	if ((in_buffer[pos++] != 'E')
	    || (in_buffer[pos++] != 'x')
	    || (in_buffer[pos++] != 'i')
	    || (in_buffer[pos++] != 'f')
	    || (in_buffer[pos++] != 0)
	    || (in_buffer[pos++] != 0))
	{
		return FALSE;
	}

	guchar *exif_data = in_buffer + pos;
	gsize exif_data_size = app1_segment_size - pos;
	return _read_exif_data_tags (exif_data, exif_data_size, flags, data);
}

gboolean
_read_exif_data_tags (guchar		 *exif_data,
		      gsize		  exif_data_size,
		      JpegInfoFlags	  flags,
		      JpegInfoData	 *data)
{
	gboolean big_endian;
	guint    offset, number_of_tags, tagnum;
	int      remaining_tags;

	// Length of an IFD entry
	if (exif_data_size < 12)
		return FALSE;

	remaining_tags = 0;
	if (flags & _JPEG_INFO_EXIF_ORIENTATION) {
		remaining_tags += 1;
		//g_print ("> CHECK EXIF ORIENTATION\n");
	}
	if (flags & _JPEG_INFO_EXIF_COLORIMETRY) {
		remaining_tags += 3;
		//g_print ("> CHECK EXIF COLORIMETRY\n");
	}
	if (flags & _JPEG_INFO_EXIF_COLOR_SPACE) {
		remaining_tags += 1;
		//g_print ("> CHECK EXIF COLOR_SPACE\n");
	}
	if (remaining_tags == 0)
		return TRUE;

	/* Discover byte order */

	if ((exif_data[0] == 0x49) && (exif_data[1] == 0x49))
		big_endian = FALSE;
	else if ((exif_data[0] == 0x4D) && (exif_data[1] == 0x4D))
		big_endian = TRUE;
	else
		return FALSE;

	/* Check Tag Mark */

	if (big_endian) {
		if (exif_data[2] != 0)
			return FALSE;
		if (exif_data[3] != 0x2A)
			return FALSE;
	}
	else {
		if (exif_data[3] != 0)
			return FALSE;
		if (exif_data[2] != 0x2A)
			return FALSE;
	}

	/* Get first IFD offset (offset to IFD0) */

	if (big_endian) {
		if (exif_data[4] != 0)
			return FALSE;
		if (exif_data[5] != 0)
			return FALSE;
		offset = (exif_data[6] << 8) + exif_data[7];
	}
	else {
		if (exif_data[7] != 0)
			return FALSE;
		if (exif_data[6] != 0)
			return FALSE;
		offset = (exif_data[5] << 8) + exif_data[4];
	}

read_ifd:

	//g_print ("\n");
	//g_print ("  OFFSET: %d, LENGTH: %d\n", offset, length);
	if (offset > (gssize) exif_data_size - 2) /* check end of data segment */
		return FALSE;

	/* Get the number of directory entries contained in this IFD */

	if (big_endian)
		number_of_tags = (exif_data[offset] << 8) + exif_data[offset+1];
	else
		number_of_tags = (exif_data[offset+1] << 8) + exif_data[offset];
	if (number_of_tags == 0)
		return FALSE;

	offset += 2;

	/* Search the tags in IFD0 */

	for (;;) {
		//g_print ("> OFFSET: %d\n", offset);

		if (offset > (gssize) exif_data_size - 12) /* check end of data segment (entry size is 12) */
			return FALSE;

		//g_print ("  DATA: x%02x x%02x x%02x x%02x x%02x x%02x x%02x x%02x x%02x x%02x x%02x x%02x\n",
		//	exif_data[offset+0],
		//	exif_data[offset+1],
		//	exif_data[offset+2],
		//	exif_data[offset+3],
		//	exif_data[offset+4],
		//	exif_data[offset+5],
		//	exif_data[offset+6],
		//	exif_data[offset+7],
		//	exif_data[offset+8],
		//	exif_data[offset+9],
		//	exif_data[offset+10],
		//	exif_data[offset+11]
		//);

		/* Get Tag number */

		if (big_endian)
			tagnum = (exif_data[offset] << 8) + exif_data[offset+1];
		else
			tagnum = (exif_data[offset+1] << 8) + exif_data[offset];

		//g_print ("  TAGNUM: 0x%04x\n", tagnum);

		if ((flags & _JPEG_INFO_EXIF_ORIENTATION) && (tagnum == 0x0112)) { /* Orientation */
			int orientation;

			if (big_endian) {
				// Number of components: 1
				if (exif_data[offset + 4] != 0)
					return FALSE;
				if (exif_data[offset + 5] != 0)
					return FALSE;
				if (exif_data[offset + 6] != 0)
					return FALSE;
				if (exif_data[offset + 7] != 1)
					return FALSE;
				// Value: 2 bytes
				if (exif_data[offset + 8] != 0)
					return FALSE;
				orientation = exif_data[offset + 9];
			}
			else {
				// Number of components: 1
				if (exif_data[offset + 4] != 1)
					return FALSE;
				if (exif_data[offset + 5] != 0)
					return FALSE;
				if (exif_data[offset + 6] != 0)
					return FALSE;
				if (exif_data[offset + 7] != 0)
					return FALSE;
				// Value: 2 bytes
				if (exif_data[offset + 9] != 0)
					return FALSE;
				orientation = exif_data[offset + 8];
			}
			if (orientation > 8)
				orientation = 0;
			data->orientation = orientation;
			data->valid |= _JPEG_INFO_EXIF_ORIENTATION;

			remaining_tags--;
		}

		if ((flags & _JPEG_INFO_EXIF_COLORIMETRY) && (tagnum == 0x012D)) { /* TransferFunction */
			remaining_tags--;
		}

		if ((flags & _JPEG_INFO_EXIF_COLORIMETRY) && (tagnum == 0x013E)) { /* WhitePoint */
			remaining_tags--;
		}

		if ((flags & _JPEG_INFO_EXIF_COLORIMETRY) && (tagnum == 0x013F)) { /* PrimaryChromaticities */
			remaining_tags--;
		}

		if ((flags & _JPEG_INFO_EXIF_COLOR_SPACE) && (tagnum == 0x0001)) { /* InteropIndex */
			int string_length;
			int string_offset;
			if (big_endian) {
				// Format: 2 (string)
				if (exif_data[offset + 2] != 0)
					return FALSE;
				if (exif_data[offset + 3] != 2)
					return FALSE;
				// String length: 4 bytes
				string_length = (exif_data[offset + 4] << 24)
					+ (exif_data[offset + 5] << 16)
					+ (exif_data[offset + 6] << 8)
					+ exif_data[offset + 7];
				if (string_length <= 4) {
					string_offset = offset + 8;
				}
				else {
					// String offset: 4 bytes
					string_offset = (exif_data[offset + 8] << 24)
						+ (exif_data[offset + 9] << 16)
						+ (exif_data[offset + 10] << 8)
						+ exif_data[offset + 11];
				}
			}
			else {
				// Format: 2 (string)
				if (exif_data[offset + 2] != 2)
					return FALSE;
				if (exif_data[offset + 3] != 0)
					return FALSE;
				// String length: 4 bytes
				string_length = (exif_data[offset + 7] << 24)
					+ (exif_data[offset + 6] << 16)
					+ (exif_data[offset + 5] << 8)
					+ exif_data[offset + 4];
				if (string_length <= 4) {
					string_offset = offset + 8;
				}
				else {
					// String offset: 4 bytes
					string_offset = (exif_data[offset + 11] << 24)
						+ (exif_data[offset + 10] << 16)
						+ (exif_data[offset + 9] << 8)
						+ exif_data[offset + 8];
				}
			}
			//g_print ("  string_offset: %d\n", string_offset);
			//g_print ("  string_length: %d\n", string_length);
			if ((string_offset >= 0)
				&& (string_length >= 4)
				&& (string_offset + string_length < exif_data_size)
				&& (exif_data[string_offset + string_length] == 0))
			{
				const char *string_data = (char *) exif_data + string_offset;
				//g_print ("  InteropIndex: '%s'\n", string_data);
				if (strncmp (string_data, "R03", 3) == 0) {
					data->color_space = GTH_COLOR_SPACE_ADOBERGB;
					//g_print ("  ADOBE RGB\n");
					remaining_tags--;
				}
				else if (strncmp (string_data, "R98", 3) == 0) {
					data->color_space = GTH_COLOR_SPACE_SRGB;
					//g_print ("  SRGB\n");
					remaining_tags--;
				}
				else {
					data->color_space = GTH_COLOR_SPACE_UNKNOWN;
					//g_print ("  UNKNOWN\n");
				}
				data->valid |= _JPEG_INFO_EXIF_COLOR_SPACE;
			}
		}

		if ((flags & _JPEG_INFO_EXIF_COLOR_SPACE) && (tagnum == 0xA001)) { /* ColorSpace */
			int value;

			if (big_endian) {
				// Number of components: 1
				if (exif_data[offset + 4] != 0)
					return FALSE;
				if (exif_data[offset + 5] != 0)
					return FALSE;
				if (exif_data[offset + 6] != 0)
					return FALSE;
				if (exif_data[offset + 7] != 1)
					return FALSE;
				// Value: 2 bytes
				value = (exif_data[offset + 8] << 8) + exif_data[offset + 9];
			}
			else {
				// Number of components: 1
				if (exif_data[offset + 4] != 1)
					return FALSE;
				if (exif_data[offset + 5] != 0)
					return FALSE;
				if (exif_data[offset + 6] != 0)
					return FALSE;
				if (exif_data[offset + 7] != 0)
					return FALSE;
				// Value: 2 bytes
				value = exif_data[offset + 8] + (exif_data[offset + 9] << 8);
			}

			//g_print ("  COLORSPACE: %04x\n", value);

			if (value == 1) {
				data->color_space = GTH_COLOR_SPACE_SRGB;
				remaining_tags--;
			}
			else if (value == 2) { // Not standard.
				data->color_space = GTH_COLOR_SPACE_ADOBERGB;
				remaining_tags--;
			}
			else if (value == 0xFFFF) {
				data->color_space = GTH_COLOR_SPACE_UNCALIBRATED;
			}
			else {
				data->color_space = GTH_COLOR_SPACE_UNKNOWN;
			}
			data->valid |= _JPEG_INFO_EXIF_COLOR_SPACE;
		}

		if (remaining_tags == 0)
			break;

		//g_print ("number_of_tags: %d\n", number_of_tags);

		// ExifOffset or InteropOffset
		if ((tagnum == 0x8769) || (tagnum == 0xA005)) {
			if (big_endian) {
				// Data format: 4 (unsigned long)
				if (exif_data[offset + 2] != 0)
					return FALSE;
				if (exif_data[offset + 3] != 4)
					return FALSE;
				// Number of components: 1
				if (exif_data[offset + 4] != 0)
					return FALSE;
				if (exif_data[offset + 5] != 0)
					return FALSE;
				if (exif_data[offset + 6] != 0)
					return FALSE;
				if (exif_data[offset + 7] != 1)
					return FALSE;
				// Offset: 4 bytes
				offset = (exif_data[offset + 8] << 24)
					+ (exif_data[offset + 9] << 16)
					+ (exif_data[offset + 10] << 8)
					+ exif_data[offset + 11];
			}
			else {
				// Data format: 4 (unsigned long)
				if (exif_data[offset + 2] != 4)
					return FALSE;
				if (exif_data[offset + 3] != 0)
					return FALSE;
				// Number of components: 1
				if (exif_data[offset + 4] != 1)
					return FALSE;
				if (exif_data[offset + 5] != 0)
					return FALSE;
				if (exif_data[offset + 6] != 0)
					return FALSE;
				if (exif_data[offset + 7] != 0)
					return FALSE;
				// Offset: 4 bytes
				offset = exif_data[offset + 8]
					+ (exif_data[offset + 9] << 8)
					+ (exif_data[offset + 10] << 16)
					+ (exif_data[offset + 11] << 24);
			}
			//g_print ("  EXIFOFFSET: %u\n", offset);
			if (offset > 0) {
				goto read_ifd;
			}
			else {
				return FALSE;
			}
		}

		offset += 12;
		if (--number_of_tags == 0) {
			// Offset to IFD1 (4 bytes) (ignored)
			return FALSE;
		}
	}

	return TRUE;
}


/* -- _jpeg_get_icc_profile_chunk_from_app2_segment -- */


typedef struct {
	int	 seq_n;
	int	 tot;
	guchar  *in_buffer;
	guchar	*data;
	gsize    size;
} ICCProfileChunk;


static void
icc_profile_chunk_free (ICCProfileChunk *chunk)
{
	g_free (chunk->in_buffer);
	g_free (chunk);
}


static int
icc_chunk_compare (gconstpointer a,
		   gconstpointer b)
{
	const ICCProfileChunk *chunk_a = a;
	const ICCProfileChunk *chunk_b = b;

	if (chunk_a->seq_n < chunk_b->seq_n)
		return -1;
	if (chunk_a->seq_n > chunk_b->seq_n)
		return 1;
	return 0;
}


static ICCProfileChunk *
_jpeg_get_icc_profile_chunk_from_app2_segment (guchar *in_buffer,
					       gsize   app2_segment_size)
{
	int		 pos;
	guint		 length;
	ICCProfileChunk *chunk;

	length = app2_segment_size;
	if (length <= 14)
		return NULL;

	pos = 0;

	/* check for "ICC_PROFILE" */

	if ((in_buffer[pos++] != 'I')
	    || (in_buffer[pos++] != 'C')
	    || (in_buffer[pos++] != 'C')
	    || (in_buffer[pos++] != '_')
	    || (in_buffer[pos++] != 'P')
	    || (in_buffer[pos++] != 'R')
	    || (in_buffer[pos++] != 'O')
	    || (in_buffer[pos++] != 'F')
	    || (in_buffer[pos++] != 'I')
	    || (in_buffer[pos++] != 'L')
	    || (in_buffer[pos++] != 'E')
	    || (in_buffer[pos++] != 0))
	{
		return NULL;
	}

	chunk = g_new (ICCProfileChunk, 1);
	chunk->in_buffer = in_buffer;
	chunk->seq_n = in_buffer[pos++];
	chunk->tot = in_buffer[pos++];
	chunk->data = in_buffer + 14;
	chunk->size = app2_segment_size - 14;

	return chunk;
}


#define _JPEG_MARKER_SOF0 0xc0
#define _JPEG_MARKER_SOF1 0xc2
#define _JPEG_MARKER_APP1 0xe1
#define _JPEG_MARKER_APP2 0xe2


gboolean
_jpeg_info_get_from_stream (GInputStream	 *stream,
			    JpegInfoFlags	  flags,
			    JpegInfoData	 *data,
			    GCancellable	 *cancellable,
			    GError		**error)
{
	GList    *icc_chunks;
	guchar    marker_id;

	g_return_val_if_fail (data->valid == _JPEG_INFO_NONE, FALSE);

	icc_chunks = NULL;
	while ((marker_id = _jpeg_read_segment_marker (stream, cancellable, error)) != 0x00) {
		gboolean segment_data_consumed = FALSE;

		if (((flags & _JPEG_INFO_IMAGE_SIZE) && ! (data->valid & _JPEG_INFO_IMAGE_SIZE))
		    && ((marker_id == _JPEG_MARKER_SOF0) || (marker_id == _JPEG_MARKER_SOF1)))
		{
			guint h, l;
			guint size;

			/* size */

			h = _g_input_stream_read_byte (stream, cancellable, error);
			l = _g_input_stream_read_byte (stream, cancellable, error);
			size = (h << 8) + l;

			/* data precision */

			(void) _g_input_stream_read_byte (stream, cancellable, error);

			/* height */

			h = _g_input_stream_read_byte (stream, cancellable, error);
			l = _g_input_stream_read_byte (stream, cancellable, error);
			data->height = (h << 8) + l;

			/* width */

			h = _g_input_stream_read_byte (stream, cancellable, error);
			l = _g_input_stream_read_byte (stream, cancellable, error);
			data->width = (h << 8) + l;

			g_input_stream_skip (stream, size - 7, cancellable, error);

			segment_data_consumed = TRUE;
			data->valid |= _JPEG_INFO_IMAGE_SIZE;
		}

		if (((flags & _JPEG_INFO_EXIF_ORIENTATION)
		     || (flags & _JPEG_INFO_EXIF_COLORIMETRY)
		     || (flags & _JPEG_INFO_EXIF_COLOR_SPACE))
		    && (marker_id == _JPEG_MARKER_APP1))
		{
			guint   h, l;
			guint   app1_segment_size;
			guchar *app1_segment;

			h = _g_input_stream_read_byte (stream, cancellable, error);
			l = _g_input_stream_read_byte (stream, cancellable, error);
			app1_segment_size = (h << 8) + l - 2;

			app1_segment = g_new (guchar, app1_segment_size);
			if (g_input_stream_read_all (stream,
						     app1_segment,
						     app1_segment_size,
						     NULL,
						     cancellable,
						     error))
			{
				_jpeg_exif_tags_from_app1_segment (app1_segment, app1_segment_size, flags, data);
			}

			segment_data_consumed = TRUE;

			g_free (app1_segment);
		}

		if ((flags & _JPEG_INFO_ICC_PROFILE) && (marker_id == _JPEG_MARKER_APP2)) {
			guint   h, l;
			gsize   app2_segment_size;
			guchar *app2_segment;

			/* size */

			h = _g_input_stream_read_byte (stream, cancellable, error);
			l = _g_input_stream_read_byte (stream, cancellable, error);
			app2_segment_size = (h << 8) + l - 2;

			app2_segment = g_new (guchar, app2_segment_size);
			if (g_input_stream_read_all (stream,
						     app2_segment,
						     app2_segment_size,
						     NULL,
						     cancellable,
						     error))
			{
				ICCProfileChunk *chunk;

				chunk = _jpeg_get_icc_profile_chunk_from_app2_segment (app2_segment, app2_segment_size);
				if (chunk != NULL)
					icc_chunks = g_list_prepend (icc_chunks, chunk);
			}

			segment_data_consumed = TRUE;
		}

		if (! segment_data_consumed && ! _jpeg_skip_segment_data (stream, marker_id, cancellable, error))
			break;
	}

	if (flags & _JPEG_INFO_ICC_PROFILE) {
		gboolean	 valid_icc = (icc_chunks != NULL);
		GOutputStream	*ostream;
		GList		*scan;
		int		 seq_n;

		ostream = g_memory_output_stream_new (NULL, 0, g_realloc, g_free);
		icc_chunks = g_list_sort (icc_chunks, icc_chunk_compare);
		seq_n = 1;
		for (scan = icc_chunks; scan; scan = scan->next) {
			ICCProfileChunk *chunk = scan->data;

			if (chunk->seq_n != seq_n) {
				valid_icc = FALSE;
				g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_INVALID_DATA, "Invalid ICC data");
				break;
			}

			g_output_stream_write_all (ostream, chunk->data, chunk->size, NULL, cancellable, error);
			seq_n++;
		}

		if (valid_icc && g_output_stream_close (ostream, NULL, NULL)) {
			data->valid |= _JPEG_INFO_ICC_PROFILE;
			data->icc_data = g_memory_output_stream_steal_data (G_MEMORY_OUTPUT_STREAM (ostream));
			data->icc_data_size = g_memory_output_stream_get_data_size (G_MEMORY_OUTPUT_STREAM (ostream));
		}

		g_object_unref (ostream);
	}

	g_list_free_full (icc_chunks, (GDestroyNotify) icc_profile_chunk_free);

	return (flags == data->valid);
}


gboolean
_jpeg_info_get_from_buffer (guchar		 *in_buffer,
			    gsize		  in_buffer_size,
			    JpegInfoFlags	  flags,
			    JpegInfoData	 *data)
{
	GInputStream *stream;
	gboolean      result;

	stream = g_memory_input_stream_new_from_data (in_buffer, in_buffer_size, NULL);
	result = _jpeg_info_get_from_stream (stream, flags, data, NULL, NULL);

	g_object_unref (stream);

	return result;
}
