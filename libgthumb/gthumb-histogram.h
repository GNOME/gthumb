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

#ifndef GTHUMB_HISTOGRAM_H
#define GTHUMB_HISTOGRAM_H


#include <gdk-pixbuf/gdk-pixbuf.h>


typedef struct {
	int   **values;
	int    *values_max;
	int     n_channels;
	int     cur_channel;
} GthumbHistogram;


GthumbHistogram * gthumb_histogram_new                 ();

void              gthumb_histogram_free                (GthumbHistogram *histogram);

void              gthumb_histogram_calculate           (GthumbHistogram *histogram,
							const GdkPixbuf *pixbuf);

double            gthumb_histogram_get_count           (GthumbHistogram *histogram,
							int              start,
							int              end);

double            gthumb_histogram_get_value           (GthumbHistogram *histogram,
							int              channel,
							int              bin);

double            gthumb_histogram_get_channel         (GthumbHistogram *histogram,
							int              channel,
							int              bin);

double            gthumb_histogram_get_max             (GthumbHistogram *histogram,
							int              channel);

int               gthumb_histogram_get_nchannels       (GthumbHistogram *histogram);

void              gthumb_histogram_set_current_channel (GthumbHistogram *histogram,
							int              channel);

int               gthumb_histogram_get_current_channel (GthumbHistogram *histogram);


#endif /* GTHUMB_HISTOGRAM_H */
