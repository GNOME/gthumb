/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2008 The Free Software Foundation, Inc.
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

#ifndef IMAGE_VIEWER_ENUMS_H
#define IMAGE_VIEWER_ENUMS_H

#define FRAME_BORDER    1
#define FRAME_BORDER2   ((FRAME_BORDER) * 2)

typedef enum { /*< skip >*/
	GTH_CHECK_TYPE_LIGHT,
	GTH_CHECK_TYPE_MIDTONE,
	GTH_CHECK_TYPE_DARK
} GthCheckType;


typedef enum { /*< skip >*/
	GTH_CHECK_SIZE_SMALL  = 4,
        GTH_CHECK_SIZE_MEDIUM = 8,
	GTH_CHECK_SIZE_LARGE  = 16
} GthCheckSize;


typedef enum { 
	GTH_FIT_NONE = 0,
	GTH_FIT_SIZE,
	GTH_FIT_SIZE_IF_LARGER,
	GTH_FIT_WIDTH,
	GTH_FIT_WIDTH_IF_LARGER
} GthFit;


/* transparenty type. */
typedef enum { /*< skip >*/
	GTH_TRANSP_TYPE_WHITE,
	GTH_TRANSP_TYPE_NONE,
	GTH_TRANSP_TYPE_BLACK,
	GTH_TRANSP_TYPE_CHECKED
} GthTranspType;


typedef enum { /*< skip >*/
	GTH_ZOOM_CHANGE_ACTUAL_SIZE = 0,
	GTH_ZOOM_CHANGE_KEEP_PREV,
	GTH_ZOOM_CHANGE_FIT_SIZE,
	GTH_ZOOM_CHANGE_FIT_SIZE_IF_LARGER,
	GTH_ZOOM_CHANGE_FIT_WIDTH,
	GTH_ZOOM_CHANGE_FIT_WIDTH_IF_LARGER
} GthZoomChange;


typedef enum { /*< skip >*/
	GTH_ZOOM_QUALITY_HIGH = 0,
	GTH_ZOOM_QUALITY_LOW
} GthZoomQuality;

#endif /* IMAGE_VIEWER_ENUMS_H */
