/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2010 Free Software Foundation, Inc.
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
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <gthumb.h>
#include "enum-types.h"
#include "gth-contact-sheet-theme.h"


static void
_g_key_file_get_color (GKeyFile  *key_file,
		       char      *group_name,
		       char      *key,
		       GdkColor  *color,
		       GError   **error)
{
	char *spec;

	spec = g_key_file_get_string (key_file, group_name, key, error);
	if (spec != NULL)
		gdk_color_parse (spec, color);

	g_free (spec);
}


GthContactSheetTheme *
gth_contact_sheet_theme_new_from_key_file (GKeyFile *key_file)
{
	GthContactSheetTheme *theme;
	char                 *nick;

	theme = g_new0 (GthContactSheetTheme, 1);
	theme->ref = 1;

	theme->display_name = g_key_file_get_string (key_file, "Theme", "Name", NULL);
	theme->authors = g_key_file_get_string (key_file, "Theme", "Authors", NULL);
	theme->copyright = g_key_file_get_string (key_file, "Theme", "Copyright", NULL);
	theme->version = g_key_file_get_string (key_file, "Theme", "Version", NULL);

	nick = g_key_file_get_string (key_file, "Background", "Type", NULL);
	theme->background_type = _g_enum_type_get_value_by_nick (GTH_TYPE_CONTACT_SHEET_BACKGROUND_TYPE, nick)->value;
	g_free (nick);

	_g_key_file_get_color (key_file, "Background", "Color1", &theme->background_color1, NULL);
	_g_key_file_get_color (key_file, "Background", "Color2", &theme->background_color2, NULL);
	_g_key_file_get_color (key_file, "Background", "Color3", &theme->background_color3, NULL);
	_g_key_file_get_color (key_file, "Background", "Color4", &theme->background_color4, NULL);

	nick = g_key_file_get_string (key_file, "Frame", "Style", NULL);
	theme->frame_style = _g_enum_type_get_value_by_nick (GTH_TYPE_CONTACT_SHEET_FRAME_STYLE, nick)->value;
	g_free (nick);

	_g_key_file_get_color (key_file, "Frame", "Color", &theme->frame_color, NULL);
	theme->frame_hpadding = 8 /*g_key_file_get_integer (key_file, "Frame", "HPadding", NULL)*/;
	theme->frame_vpadding = 8 /*g_key_file_get_integer (key_file, "Frame", "VPadding", NULL)*/;

	theme->header_font_name = g_key_file_get_string (key_file, "Header", "Font", NULL);
	_g_key_file_get_color (key_file, "Header", "Color", &theme->header_color, NULL);

	theme->footer_font_name = g_key_file_get_string (key_file, "Footer", "Font", NULL);
	_g_key_file_get_color (key_file, "Footer", "Color", &theme->footer_color, NULL);

	theme->caption_font_name = g_key_file_get_string (key_file, "Caption", "Font", NULL);
	_g_key_file_get_color (key_file, "Caption", "Color", &theme->caption_color, NULL);
	theme->caption_spacing = 3; /* g_key_file_get_integer (key_file, "Caption", "Spacing", NULL); */

	theme->row_spacing = 15;
	theme->col_spacing = 15;
	theme->frame_border = 0;

	return theme;
}


GthContactSheetTheme *
gth_contact_sheet_theme_ref (GthContactSheetTheme *theme)
{
	theme->ref++;
	return theme;
}


void
gth_contact_sheet_theme_unref (GthContactSheetTheme *theme)
{
	theme->ref--;
	if (theme->ref > 0)
		return;

	g_free (theme->display_name);
	g_free (theme->authors);
	g_free (theme->copyright);
	g_free (theme->version);
	g_free (theme->header_font_name);
	g_free (theme->footer_font_name);
	g_free (theme->caption_font_name);
	g_free (theme);
}


void
gth_contact_sheet_theme_paint_background (GthContactSheetTheme *theme,
					  cairo_t              *cr)
{
	cairo_surface_t *surface;
	int              width;
	int              height;
	cairo_pattern_t *pattern;
	cairo_color_t    color;

	surface = cairo_get_target (cr);
	if (cairo_surface_status (surface) != CAIRO_STATUS_SUCCESS)
		return;

	width = cairo_image_surface_get_width (surface);
	height = cairo_image_surface_get_height (surface);

	switch (theme->background_type) {
	case GTH_CONTACT_SHEET_BACKGROUND_TYPE_SOLID:
		gdk_cairo_set_source_color (cr, &theme->background_color1);
		cairo_rectangle (cr, 0, 0, width, height);
		cairo_fill (cr);
		break;

	case GTH_CONTACT_SHEET_BACKGROUND_TYPE_VERTICAL:
		pattern = cairo_pattern_create_linear (0.0, 0.0, 0.0, height);
		_gdk_color_to_cairo_color (&theme->background_color1, &color);
		cairo_pattern_add_color_stop_rgba (pattern, 0.0, color.r, color.g, color.b, 1.0);
		_gdk_color_to_cairo_color (&theme->background_color2, &color);
		cairo_pattern_add_color_stop_rgba (pattern, height, color.r, color.g, color.b, 1.0);
		cairo_pattern_set_filter (pattern, CAIRO_FILTER_BEST);
		cairo_set_source (cr, pattern);
		cairo_rectangle (cr, 0, 0, width, height);
		cairo_fill (cr);
		cairo_pattern_destroy (pattern);
		break;

	case GTH_CONTACT_SHEET_BACKGROUND_TYPE_HORIZONTAL:
		pattern = cairo_pattern_create_linear (0.0, 0.0, width, 0.0);
		_gdk_color_to_cairo_color (&theme->background_color1, &color);
		cairo_pattern_add_color_stop_rgba (pattern, 0.0, color.r, color.g, color.b, 1.0);
		_gdk_color_to_cairo_color (&theme->background_color2, &color);
		cairo_pattern_add_color_stop_rgba (pattern, width, color.r, color.g, color.b, 1.0);
		cairo_pattern_set_filter (pattern, CAIRO_FILTER_BEST);
		cairo_set_source (cr, pattern);
		cairo_rectangle (cr, 0, 0, width, height);
		cairo_fill (cr);
		cairo_pattern_destroy (pattern);
		break;

	case GTH_CONTACT_SHEET_BACKGROUND_TYPE_FULL:
		surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, width, height);
		_cairo_paint_full_gradient (surface,
					    &theme->background_color1,
					    &theme->background_color2,
					    &theme->background_color3,
					    &theme->background_color4);
		cairo_set_source_surface (cr, surface, 0, 0);
		cairo_rectangle (cr, 0, 0, width, height);
		cairo_fill (cr);
		cairo_surface_destroy (surface);
		break;
	}
}


void
gth_contact_sheet_theme_paint_frame (GthContactSheetTheme *theme,
				     cairo_t              *cr,
				     GdkRectangle         *frame_rect,
				     GdkRectangle         *image_rect)
{
	switch (theme->frame_style) {
	case GTH_CONTACT_SHEET_FRAME_STYLE_NONE:
		break;

	case GTH_CONTACT_SHEET_FRAME_STYLE_SHADOW:
		_cairo_draw_drop_shadow (cr,
					 image_rect->x,
					 image_rect->y,
					 image_rect->width,
					 image_rect->height,
					 3);
		break;

	case GTH_CONTACT_SHEET_FRAME_STYLE_SIMPLE:
		gdk_cairo_set_source_color (cr, &theme->frame_color);
		_cairo_draw_frame (cr,
				   image_rect->x,
				   image_rect->y,
				   image_rect->width,
				   image_rect->height,
				   3);
		break;

	case GTH_CONTACT_SHEET_FRAME_STYLE_SIMPLE_WITH_SHADOW:
		_cairo_draw_drop_shadow (cr,
					 image_rect->x,
					 image_rect->y,
					 image_rect->width,
					 image_rect->height,
					 5);

		gdk_cairo_set_source_color (cr, &theme->frame_color);
		_cairo_draw_frame (cr,
				   image_rect->x,
				   image_rect->y,
				   image_rect->width,
				   image_rect->height,
				   3);
		break;

	case GTH_CONTACT_SHEET_FRAME_STYLE_SLIDE:
		_cairo_draw_slide (cr,
				   frame_rect->x,
				   frame_rect->y,
				   frame_rect->width,
				   frame_rect->height,
				   image_rect->width,
				   image_rect->height,
				   &theme->frame_color,
				   TRUE);
		break;

	default:
		break;
	}
}


GdkPixbuf *
gth_contact_sheet_theme_create_preview (GthContactSheetTheme *theme,
					int                   preview_size)
{
	const int        preview_frame_border = 5;
	cairo_surface_t *surface;
	cairo_t         *cr;
	GdkRectangle     frame_rect;
	GdkRectangle     image_rect;
	GdkRectangle     text_rect;
	/*
	PangoContext    *pango_context;
	PangoLayout     *pango_layout;
	*/
	GdkPixbuf       *pixbuf;

	surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, preview_size, preview_size);
	cr = cairo_create (surface);
	cairo_surface_destroy (surface);

	gth_contact_sheet_theme_paint_background (theme, cr);
	image_rect.width = preview_size / 2;
	image_rect.height = preview_size / 2;
	image_rect.x = (preview_size - image_rect.width) / 2;
	image_rect.y = (preview_size - image_rect.height) / 2;
	frame_rect.x = image_rect.x - preview_frame_border;
	frame_rect.y = image_rect.y - preview_frame_border;
	frame_rect.width = image_rect.width + (preview_frame_border * 2);
	frame_rect.height = image_rect.height + (preview_frame_border * 2);
	gth_contact_sheet_theme_paint_frame (theme, cr, &frame_rect, &image_rect);

	cairo_set_source_rgb (cr, 1.0, 1.0, 1.0);
	cairo_rectangle (cr, image_rect.x, image_rect.y, image_rect.width, image_rect.height);
	cairo_fill (cr);

	/* header */

	gdk_cairo_set_source_color (cr, &theme->header_color);
	text_rect.width = preview_size / 3;
	text_rect.height = 6;
	text_rect.x = (preview_size - text_rect.width) / 2;
	text_rect.y = 4;
	cairo_rectangle (cr, text_rect.x, text_rect.y, text_rect.width, text_rect.height);
	cairo_fill (cr);

	/* footer */

	gdk_cairo_set_source_color (cr, &theme->footer_color);
	text_rect.width = preview_size / 5;
	text_rect.height = 4;
	text_rect.x = (preview_size - text_rect.width) / 2;
	text_rect.y = preview_size - text_rect.height - 4;
	cairo_rectangle (cr, text_rect.x, text_rect.y, text_rect.width, text_rect.height);
	cairo_fill (cr);

	/*
	pango_context = gdk_pango_context_get ();
	pango_context_set_language (pango_context, gtk_get_default_language ());
	pango_layout = pango_layout_new (pango_context);
	pango_layout_set_alignment (pango_layout, PANGO_ALIGN_CENTER);

	if (font_name != NULL)
		font_desc = pango_font_description_from_string (font_name);
	else
		font_desc = pango_font_description_from_string (DEFAULT_FONT);
	pango_layout_set_font_description (pango_layout, font_desc);
	pango_layout_set_text (pango_layout, text, -1);
	pango_layout_set_width (pango_layout, width * PANGO_SCALE);
	pango_layout_get_pixel_extents (pango_layout, NULL, &bounds);
	*/

	pixbuf = _gdk_pixbuf_new_from_cairo_surface (cr);

	cairo_destroy (cr);

	return pixbuf;
}
