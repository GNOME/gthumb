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
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */


#include <config.h>
#include "gth-image-info.h"


#define THUMBNAIL_SIZE 256


static void
gth_rectangle_init (GthRectangle *rect)
{
	rect->x = 0.0;
	rect->y = 0.0;
	rect->width = 0.0;
	rect->height = 0.0;
}


GthImageInfo *
gth_image_info_new (GthFileData *file_data)
{
	GthImageInfo *image_info;

	image_info = g_new0 (GthImageInfo, 1);
	image_info->ref_count = 1;
	image_info->file_data = g_object_ref (file_data);
	image_info->pixbuf = NULL;
	image_info->thumbnail_original = NULL;
	image_info->thumbnail = NULL;
	image_info->thumbnail_active = NULL;
	image_info->rotation = GTH_TRANSFORM_NONE;
	image_info->zoom = 1.0;
	image_info->transformation.x = 0.0;
	image_info->transformation.y = 0.0;
	image_info->print_comment = FALSE;
	image_info->page = -1;
	image_info->active = FALSE;
	image_info->reset = TRUE;
	gth_rectangle_init (&image_info->boundary);
	gth_rectangle_init (&image_info->maximized);
	gth_rectangle_init (&image_info->image);
	gth_rectangle_init (&image_info->comment);

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
	if (image_info == NULL)
		return;

	image_info->ref_count--;
	if (image_info->ref_count > 0)
		return;

	_g_object_unref (image_info->file_data);
	_g_object_unref (image_info->pixbuf);
	_g_object_unref (image_info->thumbnail_original);
	_g_object_unref (image_info->thumbnail);
	_g_object_unref (image_info->thumbnail_active);
	g_free (image_info->comment_text);
	g_free (image_info);
}


void
gth_image_info_set_pixbuf (GthImageInfo *image_info,
			   GdkPixbuf    *pixbuf)
{
	int thumb_w;
	int thumb_h;

	g_return_if_fail (pixbuf != NULL);

	_g_clear_object (&image_info->pixbuf);
	_g_clear_object (&image_info->thumbnail_original);
	_g_clear_object (&image_info->thumbnail);
	_g_clear_object (&image_info->thumbnail_active);

	image_info->pixbuf = g_object_ref (pixbuf);
	thumb_w = image_info->original_width = image_info->pixbuf_width = gdk_pixbuf_get_width (pixbuf);
	thumb_h = image_info->original_height = image_info->pixbuf_height = gdk_pixbuf_get_height (pixbuf);
	if (scale_keeping_ratio (&thumb_w, &thumb_h, THUMBNAIL_SIZE, THUMBNAIL_SIZE, FALSE))
		image_info->thumbnail_original = gdk_pixbuf_scale_simple (pixbuf,
									  thumb_w,
									  thumb_h,
									  GDK_INTERP_BILINEAR);
	else
		image_info->thumbnail_original = g_object_ref (image_info->pixbuf);

	image_info->thumbnail = g_object_ref (image_info->thumbnail_original);
	image_info->thumbnail_active = gdk_pixbuf_copy (image_info->thumbnail);
	_gdk_pixbuf_colorshift (image_info->thumbnail_active, image_info->thumbnail_active, 30);
}


void
gth_image_info_reset (GthImageInfo *image_info)
{
	image_info->reset = TRUE;
}


void
gth_image_info_rotate (GthImageInfo *image_info,
		       int           angle)
{
	angle = angle % 360;
	image_info->rotation = GTH_TRANSFORM_NONE;
	switch (angle) {
	case 90:
		image_info->rotation = GTH_TRANSFORM_ROTATE_90;
		break;
	case 180:
		image_info->rotation = GTH_TRANSFORM_ROTATE_180;
		break;
	case 270:
		image_info->rotation = GTH_TRANSFORM_ROTATE_270;
		break;
	default:
		break;
	}

	_g_clear_object (&image_info->thumbnail);
	if (image_info->thumbnail_original != NULL)
		image_info->thumbnail = _gdk_pixbuf_transform (image_info->thumbnail_original, image_info->rotation);

	_g_clear_object (&image_info->thumbnail_active);
	if (image_info->thumbnail != NULL) {
		image_info->thumbnail_active = gdk_pixbuf_copy (image_info->thumbnail);
		_gdk_pixbuf_colorshift (image_info->thumbnail_active, image_info->thumbnail_active, 30);
	}

	if ((angle == 90) || (angle == 270)) {
		image_info->pixbuf_width = image_info->original_height;
		image_info->pixbuf_height = image_info->original_width;
	}
	else {
		image_info->pixbuf_width = image_info->original_width;
		image_info->pixbuf_height = image_info->original_height;
	}
}
