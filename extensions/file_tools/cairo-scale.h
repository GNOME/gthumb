/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2012 The Free Software Foundation, Inc.
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

#ifndef CAIRO_ROTATE_H
#define CAIRO_ROTATE_H

#include <glib.h>
#include <gdk/gdk.h>

G_BEGIN_DECLS

cairo_surface_t *  _cairo_image_surface_scale  (cairo_surface_t *image,
						int              width,
						int              height,
						gboolean         high_quality);

G_END_DECLS

#endif /* CAIRO_ROTATE_H */
