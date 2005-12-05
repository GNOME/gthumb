/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2001, 2003, 2004 Free Software Foundation, Inc.
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
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include "gthumb-stock.h"
#include "icons/pixbufs.h"


static struct {
	const char    *stock_id;
	gconstpointer  default_pixbuf;
	gconstpointer  menu_pixbuf;
} items[] = {
	{ GTHUMB_STOCK_ADD_COMMENT,         add_comment_24_rgba,         add_comment_16_rgba },
	{ GTHUMB_STOCK_ADD_TO_CATALOG,      add_to_catalog_16_rgba,      NULL },
	{ GTHUMB_STOCK_BOOKMARK,            bookmark_16_rgba,            NULL },
	{ GTHUMB_STOCK_BRIGHTNESS_CONTRAST, brightness_contrast_22_rgba, brightness_contrast_16_rgba },
	{ GTHUMB_STOCK_CAMERA,              camera_24_rgba,              NULL },
	{ GTHUMB_STOCK_CATALOG,             catalog_24_rgba,             catalog_16_rgba },
	{ GTHUMB_STOCK_CHANGE_DATE,         change_date_16_rgba,         NULL },
	{ GTHUMB_STOCK_COLOR_BALANCE,       color_balance_22_rgba,       color_balance_16_rgba },
	{ GTHUMB_STOCK_CROP,                crop_16_rgba,                NULL },
	{ GTHUMB_STOCK_DESATURATE,          desaturate_16_rgba,          NULL },
	{ GTHUMB_STOCK_ENHANCE,             enhance_22_rgba,             NULL },
	{ GTHUMB_STOCK_EDIT_IMAGE,          edit_image_24_rgba,          NULL },
	{ GTHUMB_STOCK_FILM,                film_16_rgba,                NULL },
	{ GTHUMB_STOCK_FLIP,                flip_24_rgba,                flip_16_rgba },
	{ GTHUMB_STOCK_FULLSCREEN,          fullscreen_24_rgba,          fullscreen_16_rgba },
	{ GTHUMB_STOCK_HISTOGRAM,           histogram_22_rgba,           histogram_16_rgba },
	{ GTHUMB_STOCK_HUE_SATURATION,      hue_saturation_22_rgba,      hue_saturation_16_rgba },
	{ GTHUMB_STOCK_IMAGE,               image_24_rgba,               NULL },
	{ GTHUMB_STOCK_INDEX_IMAGE,         index_image_16_rgba,               NULL },
	{ GTHUMB_STOCK_INVERT,              invert_16_rgba,              NULL },
	{ GTHUMB_STOCK_LEVELS,              levels_22_rgba,              levels_16_rgba },
	{ GTHUMB_STOCK_MIRROR,              mirror_24_rgba,              mirror_16_rgba },
	{ GTHUMB_STOCK_NEXT_IMAGE,          next_image_24_rgba,          NULL },
	{ GTHUMB_STOCK_NORMAL_VIEW,         exit_fullscreen_24_rgba,     NULL },
	{ GTHUMB_STOCK_POSTERIZE,           posterize_22_rgba,           posterize_16_rgba },
	{ GTHUMB_STOCK_PREVIOUS_IMAGE,      prev_image_24_rgba,          NULL },
	{ GTHUMB_STOCK_REDUCE_COLORS,       reduce_colors_16_rgba,       NULL },
	{ GTHUMB_STOCK_RESET,               reset_16_rgba,               NULL },
	{ GTHUMB_STOCK_RESIZE,              resize_22_rgba,              resize_16_rgba },
	{ GTHUMB_STOCK_ROTATE,              rotate_16_rgba,              NULL },
	{ GTHUMB_STOCK_ROTATE_90,           rotate_90_24_rgba,           rotate_90_16_rgba },
	{ GTHUMB_STOCK_ROTATE_90_CC,        rotate_270_24_rgba,          rotate_270_16_rgba },
	{ GTHUMB_STOCK_SEARCH,              catalog_search_16_rgba,      NULL },
	{ GTHUMB_STOCK_SHOW_CATALOGS,       catalog_24_rgba,             catalog_16_rgba },
	{ GTHUMB_STOCK_SHOW_IMAGE,          image_24_rgba,               NULL },
	{ GTHUMB_STOCK_SLIDESHOW,           slideshow_24_rgba,           slideshow_16_rgba },
	{ GTHUMB_STOCK_SWAP,                swap_24_rgba,                swap_16_rgba },
	{ GTHUMB_STOCK_THRESHOLD,           threshold_16_rgba,           NULL },
	{ GTHUMB_STOCK_TRANSFORM,           transform_24_rgba,           transform_16_rgba }
};


static const GtkStockItem stock_items [] = {
	{ GTHUMB_STOCK_TRANSFORM, N_("Ro_tate Images"), 0, 0, GETTEXT_PACKAGE }
};


static gboolean stock_initialized = FALSE;


void
gthumb_stock_init (void)
{
	GtkIconFactory *factory;
	int             i;

	if (stock_initialized)
		return;
	stock_initialized = TRUE;

	gtk_stock_add_static (stock_items, G_N_ELEMENTS (stock_items));

	factory = gtk_icon_factory_new ();

	for (i = 0; i < G_N_ELEMENTS (items); i++) {
		GtkIconSet    *set;
		GtkIconSource *source;
		GdkPixbuf     *pixbuf;

		set = gtk_icon_set_new ();

		source = gtk_icon_source_new ();

		if (items[i].menu_pixbuf != NULL) {
			pixbuf = gdk_pixbuf_new_from_inline (-1, 
							     items[i].menu_pixbuf, 
							     FALSE, 
							     NULL);
			gtk_icon_source_set_pixbuf (source, pixbuf);

			gtk_icon_source_set_size_wildcarded (source, FALSE);
			gtk_icon_source_set_size (source, GTK_ICON_SIZE_MENU);
			gtk_icon_set_add_source (set, source);

			g_object_unref (pixbuf);
		}

		pixbuf = gdk_pixbuf_new_from_inline (-1, 
						     items[i].default_pixbuf, 
						     FALSE, 
						     NULL);
		
		gtk_icon_source_set_pixbuf (source, pixbuf);

		gtk_icon_source_set_size_wildcarded (source, FALSE);
		gtk_icon_source_set_size (source, GTK_ICON_SIZE_LARGE_TOOLBAR);
		gtk_icon_set_add_source (set, source);

		gtk_icon_source_set_size_wildcarded (source, TRUE);
		gtk_icon_source_set_state_wildcarded (source, TRUE);
		gtk_icon_source_set_direction_wildcarded (source, TRUE);
		gtk_icon_set_add_source (set, source);

		gtk_icon_factory_add (factory, items[i].stock_id, set);

		gtk_icon_set_unref (set);
		gtk_icon_source_free (source);
		g_object_unref (pixbuf);
	}

	gtk_icon_factory_add_default (factory);

	g_object_unref (factory);
}
