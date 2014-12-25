/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2014 Free Software Foundation, Inc.
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
#include "gth-points.h"


void
gth_points_init (GthPoints *p,
		 int	    n)
{
	p->n = n;
	p->p = g_new (GthPoint, p->n);
}


void
gth_points_dispose (GthPoints *p)
{
	if (p->p != NULL)
		g_free (p->p);
	gth_points_init (p, 0);
}


void
gth_points_copy (GthPoints *source,
		 GthPoints *dest)
{
	int i;

	gth_points_init (dest, source->n);
	for (i = 0; i < source->n; i++) {
		dest->p[i].x = source->p[i].x;
		dest->p[i].y = source->p[i].y;
	}
}
