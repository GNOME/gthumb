/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2009 The Free Software Foundation, Inc.
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


#include <config.h>
#include "gth-image-info.h"


GthImageInfo *
gth_image_info_new (GthFileData *file_data)
{
	GthImageInfo *image_info;

	image_info = g_new0 (GthImageInfo, 1);
	image_info->ref_count = 1;
	image_info->file_data = g_object_ref (file_data);
	image_info->pixbuf = NULL;
	image_info->thumbnail = NULL;
	image_info->thumbnail_active = NULL;
	image_info->width = 0.0;
	image_info->height = 0.0;
	image_info->scale_x = 0.0;
	image_info->scale_y = 0.0;
	image_info->trans_x = 0.0;
	image_info->trans_y = 0.0;
	image_info->rotate = 0;
	image_info->zoom = 0.0;
	image_info->min_x = 0.0;
	image_info->min_y = 0.0;
	image_info->max_x = 0.0;
	image_info->max_y = 0.0;
	image_info->comment_height = 0.0;
	image_info->print_comment = FALSE;

	return image_info;
}


GthImageInfo *
gth_image_info_ref (GthImageInfo *image_info)
{
	image_info->ref_count++;
	return image_info;
}


void
gth_image_info_unref (GthImageInfo *image_info)
{
	image_info->ref_count--;
	if (image_info->ref_count > 0)
		return;

	_g_object_unref (image_info->file_data);
	_g_object_unref (image_info->pixbuf);
	_g_object_unref (image_info->thumbnail);
	_g_object_unref (image_info->thumbnail_active);
	g_free (image_info);
}


void
gth_image_info_rotate (GthImageInfo *image_info,
		       int           angle)
{
	GdkPixbuf    *tmp_pixbuf;
	GthTransform  transform;

	transform = GTH_TRANSFORM_NONE;
	switch (angle) {
	case 90:
		transform = GTH_TRANSFORM_ROTATE_90;
		break;
	case 180:
		transform = GTH_TRANSFORM_ROTATE_180;
		break;
	case 270:
		transform = GTH_TRANSFORM_ROTATE_270;
		break;
	default:
		break;
	}

	if (transform == GTH_TRANSFORM_NONE)
		return;

	tmp_pixbuf = image_info->pixbuf;
	if (tmp_pixbuf != NULL) {
		image_info->pixbuf = _gdk_pixbuf_transform (tmp_pixbuf, transform);
		g_object_unref (tmp_pixbuf);
	}

	tmp_pixbuf = image_info->thumbnail;
	if (tmp_pixbuf != NULL) {
		image_info->thumbnail = _gdk_pixbuf_transform (tmp_pixbuf, transform);
		g_object_unref (tmp_pixbuf);
	}

	tmp_pixbuf = image_info->thumbnail_active;
	if (tmp_pixbuf != NULL) {
		image_info->thumbnail_active = _gdk_pixbuf_transform (tmp_pixbuf, transform);
		g_object_unref (tmp_pixbuf);
	}

	image_info->rotate = (image_info->rotate + angle) % 360;
	if ((angle == 90) || (angle == 270)) {
		int tmp = image_info->pixbuf_width;
		image_info->pixbuf_width = image_info->pixbuf_height;
		image_info->pixbuf_height = tmp;
	}
}
