/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2005 Free Software Foundation, Inc.
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

#ifndef ROTATION_UTILS_H
#define ROTATION_UTILS_H

#include <config.h>
#include <gtk/gtk.h>
#include "typedefs.h"

#include "gth-exif-utils.h"

gboolean	jtransform_perfect_transform   (gint image_width, gint image_height,
						gint MCU_width, gint MCU_height,
						GthTransform transform);

void		apply_transformation_jpeg      (GtkWindow    *win,
						const char   *filename,
						GthTransform  transform,
						gboolean	     trim);
void		apply_transformation_generic   (GtkWindow    *win,
						const char   *filename,
						GthTransform  transform);

GthTransform	get_next_transformation	       (GthTransform  original, 
						GthTransform  transform);
GthTransform	get_rotation_part	       (GthTransform  transform);
GthTransform	get_mirror_part	               (GthTransform  transform);

GthTransform	read_orientation_field	       (const char   *path);
void		write_orientation_field        (const char   *filename, 
						GthTransform  transform);
							
gboolean	swap_fields                    (ExifContent  *content,
						ExifTag       tag1,
						ExifTag       tag2);
void	        swap_xy_exif_fields            (const char   *filename);

#endif /* ROTATION_UTILS_H */
