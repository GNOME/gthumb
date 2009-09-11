/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2001-2009 The Free Software Foundation, Inc.
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

#ifndef GTH_HISTOGRAM_H
#define GTH_HISTOGRAM_H

#include <gdk-pixbuf/gdk-pixbuf.h>

#define MAX_N_CHANNELS 4

typedef struct {
	int   **values;
	int    *values_max;
	int     n_channels;
	int     cur_channel;
} GthHistogram;

GthHistogram * gth_histogram_new                 (void);
void           gth_histogram_free                (GthHistogram    *histogram);
void           gth_histogram_calculate           (GthHistogram    *histogram,
						  const GdkPixbuf *pixbuf);
double         gth_histogram_get_count           (GthHistogram    *histogram,
						  int              start,
						  int              end);
double         gth_histogram_get_value           (GthHistogram    *histogram,
						  int              channel,
						  int              bin);
double         gth_histogram_get_channel         (GthHistogram    *histogram,
						  int              channel,
						  int              bin);
double         gth_histogram_get_max             (GthHistogram    *histogram,
						  int              channel);
int            gth_histogram_get_nchannels       (GthHistogram    *histogram);
void           gth_histogram_set_current_channel (GthHistogram    *histogram,
						  int              channel);
int            gth_histogram_get_current_channel (GthHistogram    *histogram);

#endif /* GTH_HISTOGRAM_H */
