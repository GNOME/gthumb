/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2001-2014 The Free Software Foundation, Inc.
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

#ifndef GTH_POINT_H
#define GTH_POINT_H

#include <glib.h>
#include <gtk/gtk.h>
#include <gthumb.h>

G_BEGIN_DECLS

typedef struct {
	double x;
	double y;
} GthPoint;


typedef struct {
	GthPoint *p;
	int       n;
} GthPoints;


void	gth_points_init		(GthPoints *p,
				 int	    n);
void	gth_points_dispose	(GthPoints *p);
void	gth_points_copy		(GthPoints *source,
				 GthPoints *dest);

G_END_DECLS

#endif /* GTH_POINT_H */
