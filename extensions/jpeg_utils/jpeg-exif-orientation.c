/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2011 Free Software Foundation, Inc.
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
#include "jpeg-exif-orientation.h"


GthTransform
_jpeg_exif_orientation (guchar *in_buffer,
			gsize   in_buffer_size)
{
	int       pos = 0;
	guint     length;
	gboolean  is_motorola;
	guchar   *exif_data;
	guint     offset, number_of_tags, tagnum;
	int       orientation;

	/* Read File head, check for JPEG SOI + Exif APP1 */

	if ((in_buffer[pos++] != 0xFF)
	    || (in_buffer[pos++] != 0xD8)
	    || (in_buffer[pos++] != 0xFF)
	    || (in_buffer[pos++] != 0xE1))
	{
	    return 0;
	}

	if (in_buffer_size < 2)
		return 0;

	/* Length includes itself, so must be at least 2 */
	/* Following Exif data length must be at least 6 */

	length = ((guint) in_buffer[pos] << 8) + ((guint) in_buffer[pos + 1]);
	if (length < 8)
		return 0;

	pos += 2;

	/* Read Exif head, check for "Exif" */

	if ((in_buffer[pos++] != 0x45)
	    || (in_buffer[pos++] != 0x78)
	    || (in_buffer[pos++] != 0x69)
	    || (in_buffer[pos++] != 0x66)
	    || (in_buffer[pos++] != 0)
	    || (in_buffer[pos++] != 0))
	{
		return 0;
	}

	/* Length of an IFD entry */

	if (length < 12)
		return 0;

	exif_data = in_buffer + pos;

	/* Discover byte order */

	if ((exif_data[0] == 0x49) && (exif_data[1] == 0x49))
		is_motorola = FALSE;
	else if ((exif_data[0] == 0x4D) && (exif_data[1] == 0x4D))
		is_motorola = TRUE;
	else
		return 0;

	/* Check Tag Mark */

	if (is_motorola) {
		if (exif_data[2] != 0)
			return 0;
		if (exif_data[3] != 0x2A)
			return 0;
	}
	else {
		if (exif_data[3] != 0)
			return 0;
		if (exif_data[2] != 0x2A)
			return 0;
	}

	/* Get first IFD offset (offset to IFD0) */

	if (is_motorola) {
		if (exif_data[4] != 0)
			return 0;
		if (exif_data[5] != 0)
			return 0;
		offset = exif_data[6];
		offset <<= 8;
		offset += exif_data[7];
	}
	else {
		if (exif_data[7] != 0)
			return 0;
		if (exif_data[6] != 0)
			return 0;
		offset = exif_data[5];
		offset <<= 8;
		offset += exif_data[4];
	}

	if (offset > length - 2) /* check end of data segment */
		return 0;

	/* Get the number of directory entries contained in this IFD */

	if (is_motorola) {
		number_of_tags = exif_data[offset];
		number_of_tags <<= 8;
		number_of_tags += exif_data[offset+1];
	}
	else {
		number_of_tags = exif_data[offset+1];
		number_of_tags <<= 8;
		number_of_tags += exif_data[offset];
	}
	if (number_of_tags == 0)
		return 0;

	offset += 2;

	/* Search for Orientation Tag in IFD0 */

	for (;;) {
		if (offset > length - 12) /* check end of data segment */
			return 0;

		/* Get Tag number */

		if (is_motorola) {
			tagnum = exif_data[offset];
			tagnum <<= 8;
			tagnum += exif_data[offset+1];
		}
		else {
			tagnum = exif_data[offset+1];
			tagnum <<= 8;
			tagnum += exif_data[offset];
		}

		if (tagnum == 0x0112) /* found Orientation Tag */
			break;

		if (--number_of_tags == 0)
			return 0;

		offset += 12;
	}

	/* Get the Orientation value */

	if (is_motorola) {
		if (exif_data[offset + 8] != 0)
			return 0;
		orientation = exif_data[offset + 9];
	}
	else {
		if (exif_data[offset + 9] != 0)
			return 0;
		orientation = exif_data[offset + 8];
	}

	if (orientation > 8)
		orientation = 0;

	return (GthTransform) orientation;
}
