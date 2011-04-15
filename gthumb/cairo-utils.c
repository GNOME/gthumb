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
#include <math.h>
#include "cairo-utils.h"


typedef struct {
	gboolean has_alpha;
} cairo_surface_metadata_t;


static cairo_user_data_key_t surface_metadata_key;


static void
surface_metadata_free (void *data)
{
	cairo_surface_metadata_t *metadata = data;
	g_free (metadata);
}


void
_gdk_color_to_cairo_color (GdkColor      *g_color,
			   cairo_color_t *c_color)
{
	c_color->r = (double) g_color->red / 65535;
	c_color->g = (double) g_color->green / 65535;
	c_color->b = (double) g_color->blue / 65535;
	c_color->a = 1.0;
}


void
_gdk_color_to_cairo_color_255 (GdkColor          *g_color,
			       cairo_color_255_t *c_color)
{
	c_color->r = (guchar) 255.0 * g_color->red / 65535.0;
	c_color->g = (guchar) 255.0 * g_color->green / 65535.0;
	c_color->b = (guchar) 255.0 * g_color->blue / 65535.0;
	c_color->a = 0xff;
}


void
_cairo_paint_full_gradient (cairo_surface_t *surface,
			    GdkColor        *h_color1,
			    GdkColor        *h_color2,
			    GdkColor        *v_color1,
			    GdkColor        *v_color2)
{
	cairo_color_255_t  hcolor1;
	cairo_color_255_t  hcolor2;
	cairo_color_255_t  vcolor1;
	cairo_color_255_t  vcolor2;
	int                width;
	int                height;
	int                s_stride;
	unsigned char     *s_pixels;
	int                h, w;
	double             x, y;
	double             x_y, x_1_y, y_1_x, _1_x_1_y;
	guchar             red, green, blue;

	if (cairo_surface_status (surface) != CAIRO_STATUS_SUCCESS)
		return;

	_gdk_color_to_cairo_color_255 (h_color1, &hcolor1);
	_gdk_color_to_cairo_color_255 (h_color2, &hcolor2);
	_gdk_color_to_cairo_color_255 (v_color1, &vcolor1);
	_gdk_color_to_cairo_color_255 (v_color2, &vcolor2);

	width = cairo_image_surface_get_width (surface);
	height = cairo_image_surface_get_height (surface);
	s_stride = cairo_image_surface_get_stride (surface);
	s_pixels = cairo_image_surface_get_data (surface);

	for (h = 0; h < height; h++) {
		guchar *s_iter = s_pixels;

	        x = (double) (height - h) / height;

	        for (w = 0; w < width; w++) {
	        	y        = (double) (width - w) / width;
			x_y      = x * y;
			x_1_y    = x * (1.0 - y);
			y_1_x    = y * (1.0 - x);
			_1_x_1_y = (1.0 - x) * (1.0 - y);

			red   = hcolor1.r * x_y + hcolor2.r * x_1_y + vcolor1.r * y_1_x + vcolor2.r * _1_x_1_y;
			green = hcolor1.g * x_y + hcolor2.g * x_1_y + vcolor1.g * y_1_x + vcolor2.g * _1_x_1_y;
			blue  = hcolor1.b * x_y + hcolor2.b * x_1_y + vcolor1.b * y_1_x + vcolor2.b * _1_x_1_y;

			CAIRO_SET_RGBA (s_iter, red, green, blue, 0xff);

			s_iter += 4;
		}

		s_pixels += s_stride;
	}
}


void
_cairo_clear_surface (cairo_surface_t  **surface)
{
	cairo_surface_destroy (*surface);
	*surface = NULL;
}


gboolean
_cairo_image_surface_has_alpha (cairo_surface_t *surface)
{
	cairo_surface_metadata_t *metadata;

	if (surface == NULL)
		return FALSE;

	metadata = cairo_surface_get_user_data (surface, &surface_metadata_key);
	if (metadata != NULL)
		return metadata->has_alpha;

	return cairo_image_surface_get_format (surface) == CAIRO_FORMAT_ARGB32;
}


cairo_surface_t *
_cairo_image_surface_create_from_pixbuf (GdkPixbuf *pixbuf)
{
	cairo_surface_t          *surface;
	cairo_surface_metadata_t *metadata;
	int                      width;
	int                      height;
	int                      p_stride;
	int                      p_n_channels;
	guchar                  *p_pixels;
	int                      s_stride;
	unsigned char           *s_pixels;
	int                      h, w;

	if (pixbuf == NULL)
		return NULL;

	g_object_get (G_OBJECT (pixbuf),
		      "width", &width,
		      "height", &height,
		      "rowstride", &p_stride,
		      "n-channels", &p_n_channels,
		      "pixels", &p_pixels,
		      NULL );
	surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, width, height);
	s_stride = cairo_image_surface_get_stride (surface);
	s_pixels = cairo_image_surface_get_data (surface);

	metadata = g_new0 (cairo_surface_metadata_t, 1);
	metadata->has_alpha = (p_n_channels == 4);
	cairo_surface_set_user_data (surface, &surface_metadata_key, metadata, surface_metadata_free);

	if (p_n_channels == 4) {
		guchar *s_iter;
		guchar *p_iter;
		double  alpha_factor;
		guchar  red, green, blue, alpha;

		for (h = 0; h < height; h++) {
			s_iter = s_pixels;
			p_iter = p_pixels;

			for (w = 0; w < width; w++) {
				alpha = p_iter[3];
				alpha_factor = (double) alpha / 255.0;
				red   = (guchar) (alpha_factor * p_iter[0]) ;
				green = (guchar) (alpha_factor * p_iter[1]);
				blue  = (guchar) (alpha_factor * p_iter[2]);

				CAIRO_SET_RGBA (s_iter, red, green, blue, alpha);

				s_iter += 4;
				p_iter += p_n_channels;
			}

			s_pixels += s_stride;
			p_pixels += p_stride;
		}
	}
	else {
		guchar *s_iter;
		guchar *p_iter;

		for (h = 0; h < height; h++) {
			s_iter = s_pixels;
			p_iter = p_pixels;

			for (w = 0; w < width; w++) {
				CAIRO_SET_RGBA (s_iter, p_iter[0], p_iter[1], p_iter[2], 0xff);
				s_iter += 4;
				p_iter += p_n_channels;
			}

			s_pixels += s_stride;
			p_pixels += p_stride;
		}
	}

	return surface;
}


void
_cairo_draw_rounded_box (cairo_t *cr,
			 double   x,
			 double   y,
			 double   w,
			 double   h,
			 double   r)
{
	cairo_move_to (cr, x, y + r);
	if (r > 0)
		cairo_arc (cr, x + r, y + r, r, 1.0 * M_PI, 1.5 * M_PI);
	cairo_rel_line_to (cr, w - (r * 2), 0);
	if (r > 0)
		cairo_arc (cr, x + w - r, y + r, r, 1.5 * M_PI, 2.0 * M_PI);
	cairo_rel_line_to (cr, 0, h - (r * 2));
	if (r > 0)
		cairo_arc (cr, x + w - r, y + h - r, r, 0.0 * M_PI, 0.5 * M_PI);
	cairo_rel_line_to (cr, - (w - (r * 2)), 0);
	if (r > 0)
		cairo_arc (cr, x + r, y + h - r, r, 0.5 * M_PI, 1.0 * M_PI);
	cairo_rel_line_to (cr, 0, - (h - (r * 2)));
}


void
_cairo_draw_drop_shadow (cairo_t *cr,
			 double   x,
			 double   y,
			 double   w,
			 double   h,
			 double   r)
{
	int i;

	cairo_save (cr);
	cairo_set_line_width (cr, 1);
	for (i = r; i >= 0; i--) {
		cairo_set_source_rgba (cr, 0.0, 0.0, 0.0, (0.1 / r)* (r - i + 1));
		_cairo_draw_rounded_box (cr,
					 x + r - i,
					 y + r - i,
					 w + (i * 2),
					 h + (i * 2),
					 r);
		cairo_fill (cr);
	}
	cairo_restore (cr);
}


void
_cairo_draw_frame (cairo_t *cr,
		   double   x,
		   double   y,
		   double   w,
		   double   h,
		   double   r)
{
	cairo_save (cr);
	cairo_set_line_width (cr, r);
	cairo_rectangle (cr, x - (r / 2), y - (r / 2), w + r, h + r);
	cairo_stroke (cr);
	cairo_restore (cr);
}


void
_cairo_draw_slide (cairo_t  *cr,
		   double    frame_x,
		   double    frame_y,
		   double    frame_width,
		   double    frame_height,
		   double    image_width,
		   double    image_height,
		   GdkColor *frame_color,
		   gboolean  draw_inner_border)
{
	const double dark_gray = 0.60;
	const double mid_gray = 0.80;
	const double darker_gray = 0.45;
	const double light_gray = 0.99;
	double       frame_x2;
	double       frame_y2;

	frame_x += 0.5;
	frame_y += 0.5;

	frame_x2 = frame_x + frame_width;
	frame_y2 = frame_y + frame_height;

	cairo_save (cr);
	cairo_set_antialias (cr, CAIRO_ANTIALIAS_NONE);
	cairo_set_line_width (cr, 1.0);

	/* background. */

	gdk_cairo_set_source_color (cr, frame_color);
	cairo_rectangle (cr,
			 frame_x,
			 frame_y,
			 frame_width,
			 frame_height);
	cairo_fill (cr);

	if ((image_width > 0) && (image_height > 0)) {
		double image_x, image_y;

		image_x = frame_x + (frame_width - image_width) / 2 - 0.5;
		image_y = frame_y + (frame_height - image_height) / 2 - 0.5;

		/* inner border. */

		if (draw_inner_border) {
			double image_x2, image_y2;

			cairo_set_source_rgb (cr, 1.0, 1.0, 1.0);
			cairo_rectangle (cr,
					 image_x,
					 image_y,
					 image_width,
					 image_height);
			cairo_fill (cr);

			image_x -= 1;
			image_y -= 1;
			image_x2 = image_x + image_width + 2;
			image_y2 = image_y + image_height + 2;

			cairo_set_source_rgb (cr, dark_gray, dark_gray, dark_gray);
			cairo_move_to (cr, image_x, image_y);
			cairo_line_to (cr, image_x2 - 1, image_y);
			cairo_move_to (cr, image_x, image_y);
			cairo_line_to (cr, image_x, image_y2 - 1);
			cairo_stroke (cr);

			cairo_set_source_rgb (cr, mid_gray, mid_gray, mid_gray);
			cairo_move_to (cr, image_x2 - 1, image_y - 1);
			cairo_line_to (cr, image_x2 - 1, image_y2 - 1);
			cairo_move_to (cr, image_x, image_y2 - 1);
			cairo_line_to (cr, image_x2, image_y2 - 1);
			cairo_stroke (cr);
		}
	}

	/* outer border. */

	cairo_set_source_rgb (cr, mid_gray, mid_gray, mid_gray);
	cairo_move_to (cr, frame_x - 1, frame_y);
	cairo_line_to (cr, frame_x2, frame_y);
	cairo_move_to (cr, frame_x, frame_y - 1);
	cairo_line_to (cr, frame_x, frame_y2);
	cairo_stroke (cr);

	cairo_set_source_rgb (cr, darker_gray, darker_gray, darker_gray);
	cairo_move_to (cr, frame_x2, frame_y - 1);
	cairo_line_to (cr, frame_x2, frame_y2);
	cairo_move_to (cr, frame_x - 1, frame_y2);
	cairo_line_to (cr, frame_x2, frame_y2);
	cairo_stroke (cr);

	cairo_set_source_rgb (cr, light_gray, light_gray, light_gray);
	cairo_move_to (cr, frame_x, frame_y + 1);
	cairo_line_to (cr, frame_x2 - 1, frame_y + 1);
	cairo_move_to (cr, frame_x + 1, frame_y);
	cairo_line_to (cr, frame_x + 1, frame_y2 - 1);
	cairo_stroke (cr);

	cairo_set_source_rgb (cr, dark_gray, dark_gray, dark_gray);
	cairo_move_to (cr, frame_x2 - 1, frame_y);
	cairo_line_to (cr, frame_x2 - 1, frame_y2 - 1);
	cairo_move_to (cr, frame_x, frame_y2 - 1);
	cairo_line_to (cr, frame_x2 - 1, frame_y2 - 1);
	cairo_stroke (cr);

	cairo_restore (cr);
}
