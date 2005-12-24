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

#ifdef HAVE_LIBEXIF
#include "gth-exif-utils.h"
#endif /* HAVE_LIBEXIF */

typedef struct _RotationData RotationData;
struct _RotationData {
    GthTransform  rot_type;
    GthTransform  tran_type;
};

RotationData*	        rotation_data_new ();

void			update_rotation_from_exif_data (const char      *filename,
							RotationData 	*rot_data);
void			apply_transformation_jpeg      (GtkWindow       *win,
							const char      *filename,
							RotationData 	*rot_data);
void			apply_transformation_generic   (GtkWindow       *win,
							const char      *filename,
							RotationData 	*rot_data);
#ifdef HAVE_LIBEXIF
ExifShort		get_next_value_rotation_90     (int value);
ExifShort		get_next_value_mirror          (int value);
ExifShort		get_next_value_flip            (int value);

void			update_orientation_field       (const char      *filename, 
							RotationData    *rot_data);
gboolean		swap_fields                    (ExifContent     *content,
							ExifTag          tag1,
							ExifTag          tag2);
void			swap_xy_exif_fields            (const char      *filename);
#endif /* HAVE_LIBEXIF */

#endif /* ROTATION_UTILS_H */
