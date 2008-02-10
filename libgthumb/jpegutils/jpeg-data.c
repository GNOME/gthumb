/* jpeg-data.c
 *
 * Copyright � 2001 Lutz M�ller <lutz@users.sourceforge.net>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */


#include "config.h"

#include "jpeg-data.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/* #define DEBUG */


struct _JPEGDataPrivate
{
	unsigned int ref_count;
};

JPEGData *
jpeg_data_new (void)
{
	JPEGData *data;

	data = malloc (sizeof (JPEGData));
	if (!data)
		return (NULL);
	memset (data, 0, sizeof (JPEGData));
	data->priv = malloc (sizeof (JPEGDataPrivate));
	if (!data->priv) {
		free (data);
		return (NULL);
	}
	memset (data->priv, 0, sizeof (JPEGDataPrivate));
	data->priv->ref_count = 1;

	return (data);
}

void
jpeg_data_append_section (JPEGData *data)
{
	JPEGSection *s;

	if (!data->count)
		s = malloc (sizeof (JPEGSection));
	else
		s = realloc (data->sections,
			     sizeof (JPEGSection) * (data->count + 1));
	if (!s)
		return;

	data->sections = s;
	data->count++;
}

/* jpeg_data_save_file returns 1 on succes, 0 on failure */
int
jpeg_data_save_file (JPEGData *data, const char *path)
{
	FILE *f;
	unsigned char *d = NULL;
	unsigned int size = 0, written;

	jpeg_data_save_data (data, &d, &size);
	if (!d)
		return 0;

	remove (path);
	f = fopen (path, "wb");
	if (!f) {
		free (d);
		return 0;
	}
	written = fwrite (d, 1, size, f);
	fclose (f);
	free (d);
	if (written == size)  {
		return 1;
	}
	remove(path);
	return 0;
}

void
jpeg_data_save_data (JPEGData *data, unsigned char **d, unsigned int *ds)
{
	unsigned int i;
	JPEGSection s;

	if (!data)
		return;
	if (!d)
		return;
	if (!ds)
		return;

	for (*ds = i = 0; i < data->count; i++) {
		s = data->sections[i];
#ifdef DEBUG
		printf ("Writing marker 0x%x at position %i...\n",
			s.marker, *ds);
#endif

		/* Write the marker */
		*d = realloc (*d, sizeof (char) * (*ds + 2));
		(*d)[*ds + 0] = 0xff;
		(*d)[*ds + 1] = s.marker;
		*ds += 2;

		switch (s.marker) {
		case JPEG_MARKER_SOI:
		case JPEG_MARKER_EOI:
		case JPEG_MARKER_APP1:
			break;
		default:
			*d = realloc (*d, sizeof (char) *
					(*ds + s.content.generic.size + 2));
			(*d)[*ds + 0] = (s.content.generic.size + 2) >> 8;
			(*d)[*ds + 1] = (s.content.generic.size + 2) >> 0;
			*ds += 2;
			memcpy (*d + *ds, s.content.generic.data,
				s.content.generic.size);
			*ds += s.content.generic.size;

			/* In case of SOS, we need to write the data. */
			if (s.marker == JPEG_MARKER_SOS) {
				*d = realloc (*d, *ds + data->size);
				memcpy (*d + *ds, data->data, data->size);
				*ds += data->size;
			}
			break;
		}
	}
}

JPEGData *
jpeg_data_new_from_data (const unsigned char *d,
			 unsigned int size)
{
	JPEGData *data;

	data = jpeg_data_new ();
	jpeg_data_load_data (data, d, size);
	return (data);
}

void
jpeg_data_load_data (JPEGData *data, const unsigned char *d,
		     unsigned int size)
{
	unsigned int i, o, len;
	JPEGSection *s;
	JPEGMarker marker;

	if (!data)
		return;
	if (!d)
		return;

#ifdef DEBUG
	printf ("Parsing %i bytes...\n", size);
#endif

	for (o = 0; o < size;) {

		/*
		 * JPEG sections start with 0xff. The first byte that is
		 * not 0xff is a marker (hopefully).
		 */
		for (i = 0; i < 7; i++)
			if (d[o + i] != 0xff)
				break;
		if (!JPEG_IS_MARKER (d[o + i]))
			return;
		marker = d[o + i];

#ifdef DEBUG
		printf ("Found marker 0x%x ('%s') at %i.\n", marker,
			jpeg_marker_get_name (marker), o + i);
#endif

		/* Append this section */
		jpeg_data_append_section (data);
		s = &data->sections[data->count - 1];
		s->marker = marker;
		s->content.generic.data = NULL;
		o += i + 1;

		switch (s->marker) {
		case JPEG_MARKER_SOI:
		case JPEG_MARKER_EOI:
		case JPEG_MARKER_APP1:
			break;
		default:

			/* Read the length of the section */
			len = ((d[o] << 8) | d[o + 1]) - 2;
			if (len > size) { o = size; break; }
			o += 2;
			if (o + len > size) { o = size; break; }

			s->content.generic.size = len;
			s->content.generic.data =
					malloc (sizeof (char) * len);
			memcpy (s->content.generic.data, &d[o], len);

			/* In case of SOS, image data will follow. */
			if (s->marker == JPEG_MARKER_SOS) {
				data->size = size - 2 - o - len;
				data->data = malloc (
					sizeof (char) * data->size);
				memcpy (data->data, d + o + len,
					data->size);
				o += data->size;
			}
			o += len;
			break;
		}
	}
}

JPEGData *
jpeg_data_new_from_file (const char *path)
{
	JPEGData *data;

	data = jpeg_data_new ();
	jpeg_data_load_file (data, path);
	return (data);
}

void
jpeg_data_load_file (JPEGData   *data, 
		     const char *path)
{
	FILE          *f;
	unsigned char *d;
	unsigned int   size;

	if ((data == NULL) || (path == NULL))
		return;

	f = fopen (path, "rb");
	if (f == NULL)
		return;

	/* For now, we read the data into memory. Patches welcome... */
	fseek (f, 0, SEEK_END);
	size = ftell (f);
	fseek (f, 0, SEEK_SET);
	d = malloc (sizeof (char) * size);
	if (!d) {
		fclose (f);
		return;
	}
	if (fread (d, 1, size, f) != size) {
		free (d);
		fclose (f);
		return;
	}
	fclose (f);

	jpeg_data_load_data (data, d, size);
	free (d);
}

void
jpeg_data_ref (JPEGData *data)
{
	if (!data)
		return;

	data->priv->ref_count++;
}

void
jpeg_data_unref (JPEGData *data)
{
	if (!data)
		return;

	data->priv->ref_count--;
	if (!data->priv->ref_count)
		jpeg_data_free (data);
}

void
jpeg_data_free (JPEGData *data)
{
	unsigned int i;
	JPEGSection s;

	if (!data)
		return;

	if (data->count) {
		for (i = 0; i < data->count; i++) {
			s = data->sections[i];
			switch (s.marker) {
			case JPEG_MARKER_SOI:
			case JPEG_MARKER_EOI:
			case JPEG_MARKER_APP1:
				break;
			default:
				free (s.content.generic.data);
				break;
			}
		}
		free (data->sections);
	}

	if (data->data)
		free (data->data);
	free (data->priv);
	free (data);
}


static JPEGSection *
jpeg_data_get_section (JPEGData *data, JPEGMarker marker)
{
	unsigned int i;

	if (!data)
		return (NULL);

	for (i = 0; i < data->count; i++)
		if (data->sections[i].marker == marker)
			return (&data->sections[i]);
	return (NULL);
}


void
jpeg_data_dump (JPEGData *data)
{
	unsigned int i;
	JPEGContent content;
	JPEGMarker marker;

	if (!data)
		return;

	printf ("Dumping JPEG data (%i bytes of data)...\n", data->size);
	for (i = 0; i < data->count; i++) {
		marker = data->sections[i].marker;
		content = data->sections[i].content;
		printf ("Section %i (marker 0x%x - %s):\n", i, marker,
			jpeg_marker_get_name (marker));
		printf ("  Description: %s\n",
			jpeg_marker_get_description (marker));
		switch (marker) {
                case JPEG_MARKER_SOI:
                case JPEG_MARKER_EOI:
                case JPEG_MARKER_APP1:
			break;
                default:
			printf ("  Size: %i\n", content.generic.size);
                        printf ("  Unknown content.\n");
                        break;
                }
        }
}


void
jpeg_data_set_header_data (JPEGData *data, JPEGMarker marker,
			   unsigned char *buf, unsigned int size)
{
	JPEGSection *section;
	int i;
	
	section = jpeg_data_get_section (data, marker);
	if (!section) {
		jpeg_data_append_section (data);
		for (i = 0; i < data->count - 1; i++) {
			JPEGMarker m = data->sections[i].marker;
			if (m != JPEG_MARKER_SOI &&
			    (m < JPEG_MARKER_APP0 ||
			     m > JPEG_MARKER_APP15)) {
				memmove (&data->sections[i+1],
					 &data->sections[i],
					 sizeof (JPEGSection) *
					 (data->count - i - 1));
				break;
			}
		}
		section = &data->sections[i];
	} else {
		free (section->content.generic.data);
	}

	section->marker = marker;
	section->content.generic.data = malloc (size);
	memcpy (section->content.generic.data, buf, size);
	section->content.generic.size = size;
}

