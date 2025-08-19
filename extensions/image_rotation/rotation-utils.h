/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2005, 2009 Free Software Foundation, Inc.
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

#ifndef ROTATION_UTILS_H
#define ROTATION_UTILS_H

#include <config.h>
#include <gtk/gtk.h>
#include <gthumb.h>

typedef enum {
	GTH_TRANSFORM_FLAG_DEFAULT = 0,
	GTH_TRANSFORM_FLAG_CHANGE_IMAGE = 1 << 0,
	GTH_TRANSFORM_FLAG_SKIP_METADATA = 1 << 1,
	GTH_TRANSFORM_FLAG_RESET = 1 << 2,
	GTH_TRANSFORM_FLAG_ALWAYS_SAVE = 1 << 3,
} GthTransformFlags;

GthTransform	get_next_transformation		(GthTransform  original,
						 GthTransform transform);
void		apply_transformation_async	(GthFileData *file_data,
						 GthTransform transform,
						 GthTransformFlags flags,
						 GCancellable *cancellable,
						 GAsyncReadyCallback ready_func,
						 gpointer data);
gboolean	apply_transformation_finish	(GAsyncResult *result,
						 GError **error);

#endif /* ROTATION_UTILS_H */
