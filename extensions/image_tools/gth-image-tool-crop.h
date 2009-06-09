/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2009 Free Software Foundation, Inc.
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

#ifndef GTH_IMAGE_TOOL_CROP_H
#define GTH_IMAGE_TOOL_CROP_H

#include <gthumb.h>

G_BEGIN_DECLS

#define GTH_TYPE_IMAGE_TOOL_CROP (gth_image_tool_crop_get_type ())
#define GTH_IMAGE_TOOL_CROP(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTH_TYPE_IMAGE_TOOL_CROP, GthImageToolCrop))
#define GTH_IMAGE_TOOL_CROP_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), GTH_TYPE_IMAGE_TOOL_CROP, GthImageToolCropClass))
#define GTH_IS_IMAGE_TOOL_CROP(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTH_TYPE_IMAGE_TOOL_CROP))
#define GTH_IS_IMAGE_TOOL_CROP_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GTH_TYPE_IMAGE_TOOL_CROP))
#define GTH_IMAGE_TOOL_CROP_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), GTH_TYPE_IMAGE_TOOL_CROP, GthImageToolCropClass))

typedef struct _GthImageToolCrop GthImageToolCrop;
typedef struct _GthImageToolCropClass GthImageToolCropClass;
typedef struct _GthImageToolCropPrivate GthImageToolCropPrivate;

struct _GthImageToolCrop {
	GtkButton parent_instance;
	GthImageToolCropPrivate *priv;
};

struct _GthImageToolCropClass {
	GtkButtonClass parent_class;
};

GType  gth_image_tool_crop_get_type  (void);

G_END_DECLS

#endif /* GTH_IMAGE_TOOL_CROP_H */
