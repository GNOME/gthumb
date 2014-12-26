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

	if (source == NULL) {
		gth_points_init (dest, 0);
		return;
	}

	gth_points_init (dest, source->n);
	for (i = 0; i < source->n; i++) {
		dest->p[i].x = source->p[i].x;
		dest->p[i].y = source->p[i].y;
	}
}


int
gth_points_add_point (GthPoints	*points,
		      double	 x,
		      double	 y)
{
	GthPoint *old_p;
	int       old_n;
	gboolean  add_point;
	int       i, old_i;
	int       new_point_pos = -1;

	old_p = points->p;
	old_n = points->n;

	add_point = TRUE;
	for (old_i = 0; old_i < old_n; old_i++) {
		if (old_p[old_i].x == x) {
			old_p[old_i].y = y;
			add_point = FALSE;
			new_point_pos = old_i;
			break;
		}
	}

	if (add_point) {
		points->n = old_n + 1;
		points->p = g_new (GthPoint, points->n);

		for (i = 0, old_i = 0; (i < points->n) && (old_i < old_n); /* void */) {
			if (old_p[old_i].x < x) {
				points->p[i].x = old_p[old_i].x;
				points->p[i].y = old_p[old_i].y;
				i++;
				old_i++;
			}
			else
				break;
		}

		points->p[i].x = x;
		points->p[i].y = y;
		new_point_pos = i;
		i++;

		while (old_i < old_n) {
			points->p[i].x = old_p[old_i].x;
			points->p[i].y = old_p[old_i].y;
			i++;
			old_i++;
		}

		g_free (old_p);
	}

	return new_point_pos;
}


void
gth_points_delete_point	(GthPoints *points,
			 int        n_point)
{
	GthPoint *old_p;
	int       old_n, old_i;
	int       i;

	old_p = points->p;
	old_n = points->n;

	points->n = old_n - 1;
	points->p = g_new (GthPoint, points->n);
	for (old_i = 0, i = 0; old_i < old_n; old_i++) {
		if (old_i != n_point) {
			points->p[i].x = old_p[old_i].x;
			points->p[i].y = old_p[old_i].y;
			i++;
		}
	}

	g_free (old_p);
}
