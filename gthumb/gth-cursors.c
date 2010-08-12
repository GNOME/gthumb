/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2001-2008 Free Software Foundation, Inc.
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
 * 
 *  taken from : 
 *
 *  Eye of Gnome image viewer - mouse cursors
 *  Copyright (C) 2000 The Free Software Foundation
 */

#include <config.h>
#include <gdk/gdk.h>
#include "gth-cursors.h"


/* Cursor definitions.  Keep in sync with the CursorType enumeration in
 * cursors.h.
 */

#include "cursors/void-data.xbm"
#include "cursors/void-mask.xbm"
#include "cursors/hand-open-data.xbm"
#include "cursors/hand-open-mask.xbm"
#include "cursors/hand-closed-data.xbm"
#include "cursors/hand-closed-mask.xbm"

static struct {
	char *data;
	char *mask;
	int   data_width;
	int   data_height;
	int   mask_width;
	int   mask_height;
	int   hot_x, hot_y;
} cursors[] = {
	{ void_data_bits, void_mask_bits,
	  void_data_width, void_data_height,
	  void_mask_width, void_mask_height,
	  void_data_width / 2, void_data_height / 2 },
	{ hand_open_data_bits, hand_open_mask_bits,
	  hand_open_data_width, hand_open_data_height,
	  hand_open_mask_width, hand_open_mask_height,
	  hand_open_data_width / 2, hand_open_data_height / 2 },
	{ hand_closed_data_bits, hand_closed_mask_bits,
	  hand_closed_data_width, hand_closed_data_height,
	  hand_closed_mask_width, hand_closed_mask_height,
	  hand_closed_data_width / 2, hand_closed_data_height / 2 },
	{ NULL, NULL, 0, 0, 0, 0 }
};


/**
 * gth_cursor_get:
 * @window: Window whose screen and colormap determine the cursor's.
 * @type: A cursor type.
 * 
 * Creates a cursor.
 * 
 * Return value: The newly-created cursor.
 **/
GdkCursor *
gth_cursor_get (GdkWindow     *window, 
		GthCursorType  type)
{
	GdkBitmap *data;
	GdkBitmap *mask;
	GdkColor   black, white;
	GdkCursor *cursor;

	g_return_val_if_fail (window != NULL, NULL);
	g_return_val_if_fail (type < GTH_CURSOR_NUM_CURSORS, NULL);

	g_assert (cursors[type].data_width == cursors[type].mask_width);
	g_assert (cursors[type].data_height == cursors[type].mask_height);

	data = gdk_bitmap_create_from_data (window,
					    cursors[type].data,
					    cursors[type].data_width,
					    cursors[type].data_height);
	mask = gdk_bitmap_create_from_data (window,
					    cursors[type].mask,
					    cursors[type].mask_width,
					    cursors[type].mask_height);

	g_assert (data != NULL && mask != NULL);

	gdk_color_parse ("#000000", &black);
	gdk_color_parse ("#FFFFFF", &white);

	cursor = gdk_cursor_new_from_pixmap (data, mask, 
					     &white, &black,
					     cursors[type].hot_x, 
					     cursors[type].hot_y);
	g_assert (cursor != NULL);

	g_object_unref (data);
	g_object_unref (mask);

	return cursor;
}
