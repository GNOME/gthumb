/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2001 The Free Software Foundation, Inc.
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


#include <string.h>
#include "gthumb-histogram.h"


#define MAX_N_CHANNELS 4


GthumbHistogram *
gthumb_histogram_new ()
{
	GthumbHistogram *histogram;
	int              i;

	histogram = g_new0 (GthumbHistogram, 1);

	histogram->values = g_new0 (int *, MAX_N_CHANNELS + 1);
	for (i = 0; i < MAX_N_CHANNELS + 1; i++)
		histogram->values[i] = g_new0 (int, 256);

	histogram->values_max = g_new0 (int, MAX_N_CHANNELS + 1);

	return histogram;
}


void
gthumb_histogram_free (GthumbHistogram *histogram)
{
	int i;

	if (histogram == NULL)
		return;

	for (i = 0; i < MAX_N_CHANNELS + 1; i++)
		g_free (histogram->values[i]);
	g_free (histogram->values);
	g_free (histogram->values_max);

	g_free (histogram);
}


static void
histogram_reset_values (GthumbHistogram *histogram)
{
	int i;

	for (i = 0; i < MAX_N_CHANNELS + 1; i++) {
		memset (histogram->values[i], 0, sizeof (int) * 256);
		histogram->values_max[i] = 0;
	}
}


void
gthumb_histogram_calculate (GthumbHistogram *histogram,
			    const GdkPixbuf *pixbuf)
{
	int    **values = histogram->values;
	int     *values_max = histogram->values_max;  
	int      width, height, n_channels;
	int      rowstride;
	guchar  *line, *pixel;
	int      i, j, max;

	g_return_if_fail (histogram != NULL);

	if (pixbuf == NULL) {
		histogram->n_channels = 0;
		histogram_reset_values (histogram);
		return;
	}

	n_channels = gdk_pixbuf_get_n_channels (pixbuf);
	rowstride  = gdk_pixbuf_get_rowstride (pixbuf);
	line       = gdk_pixbuf_get_pixels (pixbuf);
	width      = gdk_pixbuf_get_width (pixbuf);
	height     = gdk_pixbuf_get_height (pixbuf);

	histogram->n_channels = n_channels + 1;
	histogram_reset_values (histogram);

	for (i = 0; i < height; i++) {
		pixel = line;
		line += rowstride;

		for (j = 0; j < width; j++) {
			/* count values for each RGB channel */		
			values[1][pixel[0]] += 1;
			values[2][pixel[1]] += 1;
			values[3][pixel[2]] += 1;
			if (n_channels > 3)
				values[4][ pixel[3] ] += 1;
				
			/* count value for Value channel */
			max = MAX (pixel[0], pixel[1]);
			max = MAX (pixel[2], max);
			values[0][max] += 1;

			/* track max value for each channel */
			values_max[0] = MAX (values_max[0], values[0][max]);
			values_max[1] = MAX (values_max[1], values[1][pixel[0]]);
			values_max[2] = MAX (values_max[2], values[2][pixel[1]]);
			values_max[3] = MAX (values_max[3], values[3][pixel[2]]);
			if (n_channels > 3)
				values_max[4] = MAX (values_max[4], values[4][pixel[3]]);

			pixel += n_channels;
		}
	}
}


double
gthumb_histogram_get_count (GthumbHistogram *histogram,
			    int              start,
			    int              end)
{
	int    i;
	double count = 0;

	g_return_val_if_fail (histogram != NULL, 0.0);

	for (i = start; i <= end; i++)
		count += histogram->values[0][i];
	
	return count;
}


double
gthumb_histogram_get_value   (GthumbHistogram *histogram,
			      int              channel,
			      int              bin)
{
	g_return_val_if_fail (histogram != NULL, 0.0);

	if ((channel < histogram->n_channels) 
	    && (bin >= 0) && (bin < 256))
		return (double) histogram->values[channel][bin];

	return 0.0;
}


double
gthumb_histogram_get_max     (GthumbHistogram *histogram,
			      int              channel)
{
	g_return_val_if_fail (histogram != NULL, 0.0);

	if (channel < histogram->n_channels)
		return (double) histogram->values_max[channel];

	return 0.0;
}


int
gthumb_histogram_get_nchannels (GthumbHistogram *histogram)
{
	g_return_val_if_fail (histogram != NULL, 0.0);
	return histogram->n_channels - 1;
}


void
gthumb_histogram_set_current_channel (GthumbHistogram *histogram,
				      int              channel)
{
	g_return_if_fail (histogram != NULL);
	histogram->cur_channel = channel;
}


int
gthumb_histogram_get_current_channel (GthumbHistogram *histogram)
{
	g_return_val_if_fail (histogram != NULL, 0.0);
	return histogram->cur_channel;
}
