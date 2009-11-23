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

#ifndef GTH_IMAGE_INFO_H
#define GTH_IMAGE_INFO_H

#include <gtk/gtk.h>
#include <gthumb.h>

G_BEGIN_DECLS

typedef struct {
	double x;
	double y;
	double width;
	double height;
} GthRectangle;

typedef struct {
	int           ref_count;
	GthFileData  *file_data;
	int           original_width;
	int           original_height;
	int           pixbuf_width;
	int           pixbuf_height;
	GdkPixbuf    *pixbuf;
	GdkPixbuf    *thumbnail_original;
	GdkPixbuf    *thumbnail;
	GdkPixbuf    *thumbnail_active;
	int           page;
	int           row;
	int           col;
	GthTransform  rotation;
	double        zoom;
	GthRectangle  transformation;
	gboolean      reset;
	gboolean      print_comment;
	char         *comment_text;
	GthRectangle  boundary;
	GthRectangle  maximized;
	GthRectangle  image;
	GthRectangle  comment;
} GthImageInfo;

GthImageInfo *  gth_image_info_new        (GthFileData  *file_data);
GthImageInfo *  gth_image_info_ref        (GthImageInfo *image_info);
void            gth_image_info_unref      (GthImageInfo *image_info);
void            gth_image_info_set_pixbuf (GthImageInfo *image_info,
					   GdkPixbuf    *pixbuf);
void            gth_image_info_reset      (GthImageInfo *image_info);
void            gth_image_info_rotate     (GthImageInfo *image_info,
				           int           angle);

G_END_DECLS

#endif /* GTH_IMAGE_INFO_H */
