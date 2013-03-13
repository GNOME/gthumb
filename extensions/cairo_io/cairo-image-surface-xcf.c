/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2013 Free Software Foundation, Inc.
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
#include <glib.h>
#include <gio/gio.h>
#include <gthumb.h>
#include "cairo-image-surface-xcf.h"


#define TILE_WIDTH				64
#define MAX_TILE_SIZE				(TILE_WIDTH * TILE_WIDTH * 4 * 1.5)
#define MIN3(x,y,z)				((y) <= (z) ? MIN ((x), (y)) : MIN ((x), (z)))
#define MAX3(x,y,z)				((y) >= (z) ? MAX ((x), (y)) : MAX ((x), (z)))
#define DISSOLVE_SEED				737893334
#define CLAMP_TEMP(x, min, max)                 (temp = (x), CLAMP (temp, min, max))
#define CLAMP_PIXEL(x)				CLAMP_TEMP(x, 0, 255)
#define GIMP_OP_NORMAL(xaA, xaB, aA)		(CLAMP_PIXEL (xaA + (double) xaB * (1.0 - aA)))
#define GIMP_OP_SUBTRACT(xaA, xaB)		(CLAMP_PIXEL (xaB - xaA))
#define GIMP_OP_DIVIDE(xaA, xaB, aA)		(CLAMP_PIXEL ((xaA > 0) ? (double) (xaB / xaA) * aA * 255.0 : xaB))
#define GIMP_OP_GRAIN_EXTRACT(xaA, xaB, aA)	(CLAMP_PIXEL (xaB - xaA + (127.0 * aA)))
#define GIMP_OP_GRAIN_MERGE(xaA, xaB, aA)	(CLAMP_PIXEL (xaB + xaA - (127.0 * aA)))


typedef enum {
	GIMP_RGB,
	GIMP_GRAY,
	GIMP_INDEXED
} GimpImageBaseType;


typedef enum {
	GIMP_COMPRESSION_NONE,
	GIMP_COMPRESSION_RLE,
	GIMP_COMPRESSION_ZLIB,
	GIMP_COMPRESSION_FRACTAL
} GimpCompression;


typedef enum {
	GIMP_RGB_IMAGE,
	GIMP_RGBA_IMAGE,
	GIMP_GRAY_IMAGE,
	GIMP_GRAYA_IMAGE,
	GIMP_INDEXED_IMAGE,
	GIMP_INDEXEDA_IMAGE
} GimpImageType;


typedef enum {
	GIMP_LAYER_MODE_NORMAL,
	GIMP_LAYER_MODE_DISSOLVE, /* (random dithering to discrete alpha) */
	GIMP_LAYER_MODE_BEHIND, /* (not selectable in the GIMP UI) */
	GIMP_LAYER_MODE_MULTIPLY,
	GIMP_LAYER_MODE_SCREEN,
	GIMP_LAYER_MODE_OVERLAY,
	GIMP_LAYER_MODE_DIFFERENCE,
	GIMP_LAYER_MODE_ADDITION,
	GIMP_LAYER_MODE_SUBTRACT,
	GIMP_LAYER_MODE_DARKEN_ONLY,
	GIMP_LAYER_MODE_LIGHTEN_ONLY,
	GIMP_LAYER_MODE_HUE, /* (H of HSV) */
	GIMP_LAYER_MODE_SATURATION, /* (S of HSV) */
	GIMP_LAYER_MODE_COLOR, /* (H and S of HSL) */
	GIMP_LAYER_MODE_VALUE, /* (V of HSV) */
	GIMP_LAYER_MODE_DIVIDE,
	GIMP_LAYER_MODE_DODGE,
	GIMP_LAYER_MODE_BURN,
	GIMP_LAYER_MODE_HARD_LIGHT,
	GIMP_LAYER_MODE_SOFT_LIGHT, /* (XCF version >= 2 only) */
	GIMP_LAYER_MODE_GRAIN_EXTRACT, /* (XCF version >= 2 only) */
	GIMP_LAYER_MODE_GRAIN_MERGE /* (XCF version >= 2 only) */
} GimpLayerMode;


typedef struct {
	guint            width;
	guint            height;
	GimpImageType    type;
	char            *name;
	guint32          opacity;
	gboolean         visible;
	gboolean         floating_selection;
	GimpLayerMode    mode;
	gboolean         apply_mask;
	gint32           h_offset;
	gint32           v_offset;
	cairo_surface_t *image;
	cairo_surface_t *mask;
	struct {
		gboolean dirty;
		int      rows;
		int      columns;
		int      n_tiles;
		int      last_row_height;
		int      last_col_width;
	} tiles;
	int              stride;
} GimpLayer;


typedef struct {
	guchar color1;
	guchar color2;
	guchar color3;
} GimpColormap;


static int cairo_rgba[4]  =   { CAIRO_RED, CAIRO_GREEN, CAIRO_BLUE, CAIRO_ALPHA };
static int cairo_graya[2] =   { CAIRO_RED, CAIRO_ALPHA };
static int cairo_indexed[2] = { 0, CAIRO_ALPHA };


/* -- GDataInputStream functions -- */


static gsize
_g_data_input_stream_read_byte_array (GDataInputStream  *stream,
				      guchar            *byte_array,
				      gsize              size,
				      GCancellable      *cancellable,
				      GError           **error)
{
	GError *local_error = NULL;
	gsize   i;

	for (i = 0; i < size; i++) {
		guchar byte;

		byte = g_data_input_stream_read_byte (stream, cancellable, &local_error);
		if (local_error != NULL) {
			/* ignore end-of-stream errors */
			if (local_error->code != G_IO_ERROR_FAILED)
				g_propagate_error (error, local_error);
			else
				g_error_free (local_error);
			break;
		}

		byte_array[i] = byte;
	}

	return i;
}


static char *
_g_data_input_stream_read_c_string (GDataInputStream  *stream,
				    gsize              size,
				    GCancellable      *cancellable,
				    GError           **error)
{
	char *string;
	int   i;

	g_return_val_if_fail (size > 0, NULL);

	string = g_new0 (char, size + 1);
	for (i = 0; i < size; i++)
		string[i] = (char) g_data_input_stream_read_byte (stream, cancellable, error);

	return string;
}


static char *
_g_data_input_stream_read_xcf_string (GDataInputStream  *stream,
				      GCancellable      *cancellable,
				      GError           **error)
{
	guint32 n_bytes;

	n_bytes = g_data_input_stream_read_uint32 (stream, cancellable, error);
	if (n_bytes == 0)
		return NULL;

	return _g_data_input_stream_read_c_string (stream, n_bytes, cancellable, error);
}


/* -- GimpLayer -- */


static GimpLayer *
gimp_layer_new (void)
{
	GimpLayer *layer;

	layer = g_new0 (GimpLayer, 1);
	layer->width = 0;
	layer->height = 0;
	layer->type = GIMP_RGBA_IMAGE;
	layer->name = NULL;
	layer->opacity = 255;
	layer->visible = TRUE;
	layer->floating_selection = FALSE;
	layer->mode = GIMP_LAYER_MODE_NORMAL;
	layer->apply_mask = FALSE;
	layer->h_offset = 0;
	layer->v_offset = 0;
	layer->image = NULL;
	layer->mask = NULL;
	layer->tiles.dirty = TRUE;

	return layer;
}


static gboolean
gimp_layer_get_tile_size (GimpLayer *layer,
			  int        n_tile,
			  goffset   *offset,
			  int       *width,
			  int       *height)
{
	int   tile_row, tile_column;
	gsize tile_width;
	gsize tile_height;

	if (layer->tiles.dirty) {
		layer->tiles.last_col_width = layer->width % TILE_WIDTH;
		layer->tiles.last_row_height = layer->height % TILE_WIDTH;

		layer->tiles.columns = layer->width / TILE_WIDTH;
		if (layer->tiles.last_col_width > 0)
			layer->tiles.columns++;
		else
			layer->tiles.last_col_width = TILE_WIDTH;

		layer->tiles.rows = layer->height / TILE_WIDTH;
		if (layer->tiles.last_row_height > 0)
			layer->tiles.rows++;
		else
			layer->tiles.last_row_height = TILE_WIDTH;

		layer->tiles.n_tiles = layer->tiles.columns * layer->tiles.rows;
		layer->tiles.dirty = FALSE;
		layer->stride = cairo_format_stride_for_width (CAIRO_FORMAT_ARGB32, layer->width);
	}

	if ((n_tile < 0) || (n_tile >= layer->tiles.n_tiles))
		return FALSE;

	tile_column = (n_tile % layer->tiles.columns);
	if (tile_column == layer->tiles.columns - 1)
		tile_width = layer->tiles.last_col_width;
	else
		tile_width = TILE_WIDTH;

	tile_row = (n_tile / layer->tiles.columns);
	if (tile_row == layer->tiles.rows - 1)
		tile_height = layer->tiles.last_row_height;
	else
		tile_height = TILE_WIDTH;

	*offset = ((tile_row * TILE_WIDTH) * layer->stride) + (tile_column * TILE_WIDTH * 4);
	*width = tile_width;
	*height = tile_height;

	return TRUE;
}


static void
gimp_layer_apply_mask (GimpLayer *layer)
{
	int     width;
	int     height;
	int     row_stride;
	guchar *image_row;
	guchar *mask_row;
	guchar *image_pixel;
	guchar *mask_pixel;
	int     i, j;

	if ((layer->image == NULL) || (layer->mask == NULL))
		return;

	cairo_surface_flush (layer->image);

	width = cairo_image_surface_get_width (layer->image);
	height = cairo_image_surface_get_height (layer->image);
	row_stride = cairo_image_surface_get_stride (layer->image);
	image_row = cairo_image_surface_get_data (layer->image);
	mask_row = cairo_image_surface_get_data (layer->mask);

	for (i = 0; i < height; i++) {
		image_pixel = image_row;
		mask_pixel = mask_row;

		for (j = 0; j < width; j++) {

			if (mask_pixel[0] != 0xff) {
				double factor = (double) mask_pixel[0] / 0xff;
				image_pixel[CAIRO_RED] *= factor;
				image_pixel[CAIRO_GREEN] *= factor;
				image_pixel[CAIRO_BLUE] *= factor;
				image_pixel[CAIRO_ALPHA] *= factor;
			}

			image_pixel += 4;
			mask_pixel += 4;
		}
		image_row += row_stride;
		mask_row += row_stride;
	}

	cairo_surface_mark_dirty (layer->image);
}


static void
gimp_layer_free (GimpLayer *layer)
{
	if (layer == NULL)
		return;
	if (layer->image != NULL)
		cairo_surface_destroy (layer->image);
	if (layer->mask != NULL)
		cairo_surface_destroy (layer->mask);
	g_free (layer->name);
	g_free (layer);
}


/* -- _cairo_image_surface_create_from_xcf -- */


static void
gimp_rgb_to_hsv (guchar  red,
		 guchar  green,
		 guchar  blue,
		 guchar *hue,
		 guchar *sat,
		 guchar *val)
{
	guchar min, max;

	min = MIN3 (red, green, blue);
	max = MAX3 (red, green, blue);

	*val = max;
	if (*val == 0) {
		*hue = *sat = 0;
		return;
	}

	*sat = 255 * (long)(max - min) / *val;
	if (*sat == 0) {
		*hue = 0;
		return;
	}

	if (max == red)
		*hue = 0 + 43 * (green - blue) / (max - min);
	else if (max == green)
		*hue = 85 + 43 * (blue - red) / (max - min);
	else if (max == blue)
		*hue = 171 + 43 * (red - green) / (max - min);
}


static void
gimp_hsv_to_rgb (guchar  hue,
		 guchar  sat,
		 guchar  val,
		 double *red,
		 double *green,
		 double *blue)
{
	guchar region, remainder, p, q, t;

	if (sat == 0) {
		*red = *green = *blue = val;
		return;
	}

	region = hue / 43;
	remainder = (hue - (region * 43)) * 6;

	p = (val * (255 - sat)) >> 8;
	q = (val * (255 - ((sat * remainder) >> 8))) >> 8;
	t = (val * (255 - ((sat * (255 - remainder)) >> 8))) >> 8;

	switch (region) {
	case 0:
		*red = val;
		*green = t;
		*blue = p;
		break;
	case 1:
		*red = q;
		*green = val;
		*blue = p;
		break;
	case 2:
		*red = p;
		*green = val;
		*blue = t;
		break;
	case 3:
		*red = p;
		*green = q;
		*blue = val;
		break;
	case 4:
		*red = t;
		*green = p;
		*blue = val;
		break;
	default:
		*red = val;
		*green = p;
		*blue = q;
		break;
	}
}


static void
_cairo_image_surface_paint_layer (cairo_surface_t *image,
				  GimpLayer       *layer)
{
	int     image_width;
	int     image_height;
	int     image_row_stride;
	guchar *image_row;
	int     layer_width;
	int     layer_height;
	int     layer_row_stride;
	guchar *layer_row;
	int     x, y, width, height;
	guchar *image_pixel;
	guchar *layer_pixel;
	GRand  *rand_gen;
	int     i, j;
	double  layer_opacity, layer_a, r, g, b, a, temp;
	guchar  image_hue, image_sat, image_val;
	guchar  layer_hue, layer_sat, layer_val;

	if ((image == NULL) || (layer->image == NULL))
		return;

	cairo_surface_flush (image);

	image_width = cairo_image_surface_get_width (image);
	image_height = cairo_image_surface_get_height (image);
	image_row_stride = cairo_image_surface_get_stride (image);

	layer_width = cairo_image_surface_get_width (layer->image);
	layer_height = cairo_image_surface_get_height (layer->image);
	layer_row_stride = cairo_image_surface_get_stride (layer->image);

	/* compute the layer <-> image intersection */

	{
		cairo_region_t        *region;
		cairo_rectangle_int_t  rect;

		rect.x = 0;
		rect.y = 0;
		rect.width = image_width;
		rect.height = image_height;
		region = cairo_region_create_rectangle (&rect);

		rect.x = layer->h_offset;
		rect.y = layer->v_offset;
		rect.width = layer_width;
		rect.height = layer_height;
		cairo_region_intersect_rectangle (region, &rect);
		cairo_region_get_extents (region, &rect);
		cairo_region_destroy (region);

		if ((rect.width == 0) || (rect.height == 0))
			return;

		x = rect.x;
		y = rect.y;
		width = rect.width;
		height = rect.height;
	}

	image_row = cairo_image_surface_get_data (image) + (y * image_row_stride) + (x * 4);

	/*g_print ("image: (%d, %d) [%d, %d]\n", x, y, width, height);*/

	x = (layer->h_offset < 0) ? -layer->h_offset : 0;
	y = (layer->v_offset < 0) ? -layer->v_offset : 0;
	layer_row = cairo_image_surface_get_data (layer->image) + (y * layer_row_stride) + (x * 4);

	/*g_print ("layer: (%d, %d) [%d, %d]\n", x, y, width, height);*/

	if (layer->mode == GIMP_LAYER_MODE_DISSOLVE)
		rand_gen = g_rand_new_with_seed (DISSOLVE_SEED);

	for (i = 0; i < height; i++) {
		image_pixel = image_row;
		layer_pixel = layer_row;

		for (j = 0; j < width; j++) {
			layer_opacity = (double) layer->opacity / 255.0;

			r = layer_opacity * layer_pixel[CAIRO_RED];
			g = layer_opacity * layer_pixel[CAIRO_GREEN];
			b = layer_opacity * layer_pixel[CAIRO_BLUE];
			a = layer_opacity * layer_pixel[CAIRO_ALPHA];

			layer_a = a / 255.0;

			switch (layer->mode) {
			case GIMP_LAYER_MODE_DISSOLVE:
				if ((g_rand_double (rand_gen) <= layer_a) && (layer_a > 0)) {
					image_pixel[CAIRO_RED] = CLAMP_PIXEL (r / layer_a);
					image_pixel[CAIRO_GREEN] = CLAMP_PIXEL (g / layer_a);
					image_pixel[CAIRO_BLUE] = CLAMP_PIXEL (b / layer_a);
					image_pixel[CAIRO_ALPHA] = 0xff;
				}
				break;

			case GIMP_LAYER_MODE_SUBTRACT:
				if (layer_a > 0.0) {
					r = layer_a * GIMP_OP_SUBTRACT (r / layer_a, image_pixel[CAIRO_RED]);
					g = layer_a * GIMP_OP_SUBTRACT (g / layer_a, image_pixel[CAIRO_GREEN]);
					b = layer_a * GIMP_OP_SUBTRACT (b / layer_a, image_pixel[CAIRO_BLUE]);

					image_pixel[CAIRO_RED] = GIMP_OP_NORMAL (r, image_pixel[CAIRO_RED], layer_a);
					image_pixel[CAIRO_GREEN] = GIMP_OP_NORMAL (g, image_pixel[CAIRO_GREEN], layer_a);
					image_pixel[CAIRO_BLUE] = GIMP_OP_NORMAL (b, image_pixel[CAIRO_BLUE], layer_a);
					image_pixel[CAIRO_ALPHA] = GIMP_OP_NORMAL (a, image_pixel[CAIRO_ALPHA], layer_a);
				}
				break;

			case GIMP_LAYER_MODE_DIVIDE:
				if (layer_a > 0.0) {
					r = layer_a * GIMP_OP_DIVIDE (r, image_pixel[CAIRO_RED], layer_a);
					g = layer_a * GIMP_OP_DIVIDE (g, image_pixel[CAIRO_GREEN], layer_a);
					b = layer_a * GIMP_OP_DIVIDE (b, image_pixel[CAIRO_BLUE], layer_a);

					image_pixel[CAIRO_RED] = GIMP_OP_NORMAL (r, image_pixel[CAIRO_RED], layer_a);
					image_pixel[CAIRO_GREEN] = GIMP_OP_NORMAL (g, image_pixel[CAIRO_GREEN], layer_a);
					image_pixel[CAIRO_BLUE] = GIMP_OP_NORMAL (b, image_pixel[CAIRO_BLUE], layer_a);
					image_pixel[CAIRO_ALPHA] = GIMP_OP_NORMAL (a, image_pixel[CAIRO_ALPHA], layer_a);
				}
				break;

			case GIMP_LAYER_MODE_HUE:
			case GIMP_LAYER_MODE_SATURATION:
			case GIMP_LAYER_MODE_VALUE:
				if (layer_a > 0.0) {
					gimp_rgb_to_hsv (image_pixel[CAIRO_RED],
							 image_pixel[CAIRO_GREEN],
							 image_pixel[CAIRO_BLUE],
							 &image_hue,
							 &image_sat,
							 &image_val);
					gimp_rgb_to_hsv (r / layer_a,
							 g / layer_a,
							 b / layer_a,
							 &layer_hue,
							 &layer_sat,
							 &layer_val);

					switch (layer->mode) {
					case GIMP_LAYER_MODE_HUE:
						gimp_hsv_to_rgb (layer_hue,
								 image_sat,
								 image_val,
								 &r,
								 &g,
								 &b);
						break;
					case GIMP_LAYER_MODE_SATURATION:
						gimp_hsv_to_rgb (image_hue,
								 layer_sat,
								 image_val,
								 &r,
								 &g,
								 &b);
						break;
					case GIMP_LAYER_MODE_VALUE:
						gimp_hsv_to_rgb (image_hue,
								 image_sat,
								 layer_val,
								 &r,
								 &g,
								 &b);
						break;
					default:
						break;
					}

					r *= layer_a;
					g *= layer_a;
					b *= layer_a;

					image_pixel[CAIRO_RED] = GIMP_OP_NORMAL (r, image_pixel[CAIRO_RED], layer_a);
					image_pixel[CAIRO_GREEN] = GIMP_OP_NORMAL (g, image_pixel[CAIRO_GREEN], layer_a);
					image_pixel[CAIRO_BLUE] = GIMP_OP_NORMAL (b, image_pixel[CAIRO_BLUE], layer_a);
					image_pixel[CAIRO_ALPHA] = GIMP_OP_NORMAL (a, image_pixel[CAIRO_ALPHA], layer_a);
				}
				break;

			case GIMP_LAYER_MODE_GRAIN_EXTRACT:
				image_pixel[CAIRO_RED] = GIMP_OP_GRAIN_EXTRACT (r, image_pixel[CAIRO_RED], layer_a);
				image_pixel[CAIRO_GREEN] = GIMP_OP_GRAIN_EXTRACT (g, image_pixel[CAIRO_GREEN], layer_a);
				image_pixel[CAIRO_BLUE] = GIMP_OP_GRAIN_EXTRACT (b, image_pixel[CAIRO_BLUE], layer_a);
				image_pixel[CAIRO_ALPHA] = GIMP_OP_NORMAL (a, image_pixel[CAIRO_ALPHA], layer_a);
				break;

			case GIMP_LAYER_MODE_GRAIN_MERGE:
				image_pixel[CAIRO_RED] = GIMP_OP_GRAIN_MERGE (r, image_pixel[CAIRO_RED], layer_a);
				image_pixel[CAIRO_GREEN] = GIMP_OP_GRAIN_MERGE (g, image_pixel[CAIRO_GREEN], layer_a);
				image_pixel[CAIRO_BLUE] = GIMP_OP_GRAIN_MERGE (b, image_pixel[CAIRO_BLUE], layer_a);
				image_pixel[CAIRO_ALPHA] = GIMP_OP_NORMAL (a, image_pixel[CAIRO_ALPHA], layer_a);
				break;

			case GIMP_LAYER_MODE_NORMAL:
			default:
				image_pixel[CAIRO_RED] = GIMP_OP_NORMAL (r, image_pixel[CAIRO_RED], layer_a);
				image_pixel[CAIRO_GREEN] = GIMP_OP_NORMAL (g, image_pixel[CAIRO_GREEN], layer_a);
				image_pixel[CAIRO_BLUE] = GIMP_OP_NORMAL (b, image_pixel[CAIRO_BLUE], layer_a);
				image_pixel[CAIRO_ALPHA] = GIMP_OP_NORMAL (a, image_pixel[CAIRO_ALPHA], layer_a);
				break;
			}

			image_pixel += 4;
			layer_pixel += 4;
		}

		image_row += image_row_stride;
		layer_row += layer_row_stride;
	}

	if (layer->mode == GIMP_LAYER_MODE_DISSOLVE)
		g_rand_free (rand_gen);

	cairo_surface_mark_dirty (image);
}


static cairo_surface_t *
_cairo_image_surface_create_from_layers (int                canvas_width,
					 int                canvas_height,
					 GimpImageBaseType  base_type,
					 GList             *layers)
{
	cairo_surface_t *image;
	GList           *scan;

	image = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, canvas_width, canvas_height);

	for (scan = layers; scan; scan = scan->next) {
		GimpLayer        *layer = scan->data;
		cairo_operator_t  op = -1;

		if (layer->image == NULL)
			continue;

		if ((layer->mask != NULL) && layer->apply_mask)
			gimp_layer_apply_mask (layer);

		/* the bottommost layer only supports NORMAL and DISSOLVE */

		if ((scan == layers) && (layer->mode != GIMP_LAYER_MODE_DISSOLVE))
			layer->mode = GIMP_LAYER_MODE_NORMAL;

		/* indexed images only support NORMAL and DISSOLVE */

		if ((base_type == GIMP_INDEXED) && (layer->mode != GIMP_LAYER_MODE_DISSOLVE))
			layer->mode = GIMP_LAYER_MODE_NORMAL;

		/* modes supported only by RGB images */

		if (base_type != GIMP_RGB) {
			if ((layer->mode == GIMP_LAYER_MODE_HUE)
			    || (layer->mode == GIMP_LAYER_MODE_SATURATION)
			    || (layer->mode == GIMP_LAYER_MODE_COLOR)
			    || (layer->mode == GIMP_LAYER_MODE_VALUE))
			{
				layer->mode = GIMP_LAYER_MODE_NORMAL;
			}
		}

		/* find the cairo operation equivalent to the layer mode */

		switch (layer->mode) {
		case GIMP_LAYER_MODE_NORMAL:
			op = CAIRO_OPERATOR_OVER;
			break;
		case GIMP_LAYER_MODE_DISSOLVE:
			break;
		case GIMP_LAYER_MODE_BEHIND:
			break;
		case GIMP_LAYER_MODE_MULTIPLY:
			op = CAIRO_OPERATOR_MULTIPLY;
			break;
		case GIMP_LAYER_MODE_SCREEN:
			op = CAIRO_OPERATOR_SCREEN;
			break;
		case GIMP_LAYER_MODE_OVERLAY:
			/* In Gimp OVERLAY and SOFT_LIGHT are identical */
			op = CAIRO_OPERATOR_SOFT_LIGHT;
			break;
		case GIMP_LAYER_MODE_DIFFERENCE:
			op = CAIRO_OPERATOR_DIFFERENCE;
			break;
		case GIMP_LAYER_MODE_ADDITION:
			op = CAIRO_OPERATOR_ADD;
			break;
		case GIMP_LAYER_MODE_SUBTRACT:
			break;
		case GIMP_LAYER_MODE_DARKEN_ONLY:
			op = CAIRO_OPERATOR_DARKEN;
			break;
		case GIMP_LAYER_MODE_LIGHTEN_ONLY:
			op = CAIRO_OPERATOR_LIGHTEN;
			break;
		case GIMP_LAYER_MODE_HUE:
			break;
		case GIMP_LAYER_MODE_SATURATION:
			break;
		case GIMP_LAYER_MODE_COLOR:
			op = CAIRO_OPERATOR_HSL_COLOR;
			break;
		case GIMP_LAYER_MODE_VALUE:
			break;
		case GIMP_LAYER_MODE_DIVIDE:
			break;
		case GIMP_LAYER_MODE_DODGE:
			op = CAIRO_OPERATOR_COLOR_DODGE;
			break;
		case GIMP_LAYER_MODE_BURN:
			op = CAIRO_OPERATOR_COLOR_BURN;
			break;
		case GIMP_LAYER_MODE_HARD_LIGHT:
			op = CAIRO_OPERATOR_HARD_LIGHT;
			break;
		case GIMP_LAYER_MODE_SOFT_LIGHT:
			op = CAIRO_OPERATOR_SOFT_LIGHT;
			break;
		case GIMP_LAYER_MODE_GRAIN_EXTRACT:
			break;
		case GIMP_LAYER_MODE_GRAIN_MERGE:
			break;
		}

		if (op != -1) {

			/* layer mode supported directly by cairo */

			cairo_t *cr;

			cr = cairo_create (image);
			cairo_set_source_surface (cr, layer->image, layer->h_offset, layer->v_offset);
			cairo_set_operator (cr, op);
			cairo_rectangle (cr, 0, 0, canvas_width, canvas_height);
			cairo_paint_with_alpha (cr, (double) layer->opacity / 255.0);

			cairo_destroy (cr);
		}
		else
			_cairo_image_surface_paint_layer (image, layer);
	}

	return image;
}


static void
_cairo_surface_data_post_production (guchar		*image_pixels,
				     int   		 row_stride,
				     int		 width,
				     int		 height,
				     int		 bpp,
				     GimpImageBaseType	 base_type)
{
	int     i, j;
	guchar *row, *pixel;

	row = image_pixels;
	for (i = 0; i < height; i++) {
		pixel = row;
		for (j = 0; j < width; j++) {

			if ((bpp <= 2) && (base_type != GIMP_INDEXED)) {

				/* copy the red channel value to the green and blue channels */

				pixel[CAIRO_GREEN] = pixel[CAIRO_RED];
				pixel[CAIRO_BLUE] = pixel[CAIRO_RED];
			}

			if ((bpp == 4) || (bpp == 2)) {

				/* multiply for the alpha channel */

				if (pixel[CAIRO_ALPHA] < 0xff) {
					double factor = (double) pixel[CAIRO_ALPHA] / 0xff;
					pixel[CAIRO_RED] *= factor;
					pixel[CAIRO_GREEN] *= factor;
					pixel[CAIRO_BLUE] *= factor;
				}
			}
			else if ((bpp == 3) || (bpp == 1)) {

				/* set the alpha channel to opaque */

				pixel[CAIRO_ALPHA] = 0xff;
			}

			pixel += 4;
		}
		row += row_stride;
	}
}


static cairo_surface_t *
_cairo_image_surface_create_from_hierarchy (GDataInputStream  *data_stream,
					    guint32            hierarchy_offset,
					    GimpLayer         *layer,
					    GimpColormap      *colormap,
					    GimpImageBaseType  base_type,
					    GimpCompression    compression,
					    GCancellable      *cancellable,
					    GError           **error)
{
	cairo_surface_t *image = NULL;
	guint32          width;
	guint32          height;
	guint32          bpp;
	guint32          level_offset;
	GArray          *tile_offsets = NULL;
	guint32          tile_offset;
	guint32          last_tile_offset;
	int              n_tiles;
	int              t;
	guchar          *image_pixels;
	int              row_stride;

	/* read the hierarchy structure */

	if (! g_seekable_seek (G_SEEKABLE (data_stream),
			       hierarchy_offset,
			       G_SEEK_SET,
			       cancellable,
			       error))
	{
		return NULL;
	}

	width = g_data_input_stream_read_uint32 (data_stream, cancellable, error);
	if (*error != NULL)
		goto read_error;

	height = g_data_input_stream_read_uint32 (data_stream, cancellable, error);
	if (*error != NULL)
		goto read_error;

	bpp = g_data_input_stream_read_uint32 (data_stream, cancellable, error);
	if (*error != NULL)
		goto read_error;

	level_offset = g_data_input_stream_read_uint32 (data_stream, cancellable, error);
	if (*error != NULL)
		goto read_error;

	/*
	g_print ("  hierarchy width: %" G_GUINT32_FORMAT "\n", width);
	g_print ("  hierarchy height: %" G_GUINT32_FORMAT "\n", height);
	g_print ("  hierarchy bpp: %" G_GUINT32_FORMAT "\n", bpp);
	g_print ("  level offset: %" G_GUINT32_FORMAT "\n", level_offset);
	*/

	/* read the level structure */

	if (! g_seekable_seek (G_SEEKABLE (data_stream),
			       level_offset,
			       G_SEEK_SET,
			       cancellable,
			       error))
	{
		goto read_error;
	}

	width = g_data_input_stream_read_uint32 (data_stream, cancellable, error);
	if (*error != NULL)
		goto read_error;

	height = g_data_input_stream_read_uint32 (data_stream, cancellable, error);
	if (*error != NULL)
		goto read_error;

	/*
	g_print ("  level width: %" G_GUINT32_FORMAT "\n", width);
	g_print ("  level height: %" G_GUINT32_FORMAT "\n", height);
	*/

	/* tiles */

	image = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, width, height);
	cairo_surface_flush (image);
	image_pixels = cairo_image_surface_get_data (image);
	row_stride = cairo_image_surface_get_stride (image);

	tile_offsets = g_array_new (FALSE, FALSE, sizeof (guint32));;
	n_tiles = 0;
	last_tile_offset = 0;
	while ((tile_offset = g_data_input_stream_read_uint32 (data_stream, cancellable, error)) != 0) {
		n_tiles += 1;
		last_tile_offset = tile_offset;
		g_array_append_val (tile_offsets, tile_offset);
		/*g_print ("    tile %d: %" G_GUINT32_FORMAT "\n", n_tiles, tile_offset);*/
	}
	tile_offset = last_tile_offset + MAX_TILE_SIZE;
	g_array_append_val (tile_offsets, tile_offset);

	if (*error != NULL)
		goto read_error;

	if (compression == GIMP_COMPRESSION_RLE) {

		/* go to the first tile */

		tile_offset = g_array_index (tile_offsets, guint32, 0);
		/*g_print ("    <seek: %" G_GUINT32_FORMAT ">\n", tile_offset);*/
		if (! g_seekable_seek (G_SEEKABLE (data_stream),
				       tile_offset,
				       G_SEEK_SET,
				       cancellable,
				       error))
		{
			goto read_error;
		}

		/* decompress the tile data */

		for (t = 0; t < n_tiles; t++) {
			goffset  tile_data_size;
			guchar  *tile_data;
			guchar  *tile_data_p;
			guchar  *tile_data_limit;
			gsize    data_read;
			goffset  tile_pixels_offset;
			gsize    tile_pixels_size;
			int      tile_width;
			int      tile_height;
			int      c;

			/* read the tile data */

			tile_data_size = g_array_index (tile_offsets, guint32, t + 1) - g_array_index (tile_offsets, guint32, t);
			if (tile_data_size <= 0) {
				/*g_print ("<tile %d: size error %" G_GUINT32_FORMAT " = %" G_GUINT32_FORMAT " - %" G_GUINT32_FORMAT ">", t, tile_data_size, g_array_index (tile_offsets, guint32, t + 1), g_array_index (tile_offsets, guint32, t));*/
				continue;
			}

			tile_data = g_malloc (tile_data_size);
			data_read = _g_data_input_stream_read_byte_array (data_stream, tile_data, tile_data_size, cancellable, error);
			if (*error != NULL)
				goto rle_error;

			/*g_print ("    tile size %d: %" G_GSIZE_FORMAT " (%" G_GSIZE_FORMAT ")\n", t, data_read, tile_data_size);*/

			/* decompress the channel streams */

			if (! gimp_layer_get_tile_size (layer,
							t,
							&tile_pixels_offset,
							&tile_width,
							&tile_height))
			{
				/*g_print ("<tile %d: error>", t);*/
				goto rle_error;
			}

			tile_pixels_size = tile_width * tile_height;
			tile_data_p = tile_data;
			tile_data_limit = tile_data + data_read - 1;

			for (c = 0; c < bpp; c++) {
				int     channel_offset;
				guchar *pixels_row;
				guchar *pixels;
				int     size;
				int     n, p, q, v;
				int     tile_column;

				if (base_type == GIMP_INDEXED)
					channel_offset = cairo_indexed[c];
				else if (bpp >= 3)
					channel_offset = cairo_rgba[c];
				else if (bpp <= 2)
					/* save the value in the red channel,
					 * it will be copied to the other channels in
					 * _cairo_surface_data_post_production */
					channel_offset = cairo_graya[c];
				else
					channel_offset = 0;
				pixels_row = image_pixels + tile_pixels_offset + channel_offset;
				pixels = pixels_row;

				size = tile_pixels_size;
				tile_column = 0;

#define SET_PIXEL(v) {						\
	tile_column++;						\
	if (tile_column > tile_width) {				\
		pixels_row += row_stride;			\
		pixels = pixels_row;				\
		tile_column = 1;				\
	}							\
	if ((base_type == GIMP_INDEXED) && (c == 0)) {		\
		guchar *color = (guchar *) (colormap + (v));	\
		pixels[CAIRO_RED] = color[0];			\
		pixels[CAIRO_GREEN] = color[1];			\
		pixels[CAIRO_BLUE] = color[2];			\
	}							\
	else							\
		*pixels = (v);					\
	pixels += 4;						\
}

				while (size > 0) {
					if (tile_data_p > tile_data_limit)
						goto rle_error;

					n = *tile_data_p++;

					if ((n >= 0) && (n <= 127)) {
						/* byte          n     For 0 <= n <= 126: a short run of identical bytes
  	  	  	  	  	  	 * byte          v     Repeat this value n+1 times
						 */

						/* byte          127   A long run of identical bytes
						 * byte          p
						 * byte          q
						 * byte          v     Repeat this value p*256 + q times
						 */

						if (n == 127) {
							if (tile_data_p + 2 > tile_data_limit)
								goto rle_error;
							p = *tile_data_p++;
							q = *tile_data_p++;
							v = *tile_data_p++;
							n = (p * 256) + q;
						}
						else {
							if (tile_data_p > tile_data_limit)
								goto rle_error;
							v = *tile_data_p++;
							n++;
						}

						size -= n;
						if (size < 0)
							goto rle_error;

						while (n-- > 0)
							SET_PIXEL (v);
					}
					else if ((n >= 128) && (n <= 255)) {
						/* byte          128   A long run of different bytes
						 * byte          p
						 * byte          q
						 * byte[p*256+q] data  Copy these verbatim to the output stream */

						/* byte          n     For 129 <= n <= 255: a short run of different bytes
						 * byte[256-n]   data  Copy these verbatim to the output stream */

						if (n == 128) {
							if (tile_data_p + 1 > tile_data_limit)
								goto rle_error;
							p = *tile_data_p++;
							q = *tile_data_p++;
							n = (p * 256) + q;
						}
						else
							n = 256 - n;

						if (tile_data_p + n - 1 > tile_data_limit)
							goto rle_error;

						size -= n;
						if (size < 0)
							goto rle_error;

						while (n-- > 0)
							SET_PIXEL (*tile_data_p++);
					}
				}
			}

#undef SET_PIXEL

rle_error:

			g_free (tile_data);
			if (*error != NULL)
				goto read_error;
		}
	}
	else if (compression == GIMP_COMPRESSION_NONE) {

		/* This is untested because Gimp doesn't save in uncompressed
		 * mode any more. */

		/* go to the first tile */

		tile_offset = g_array_index (tile_offsets, guint32, 0);
		/*g_print ("    <seek: %" G_GUINT32_FORMAT ">\n", tile_offset);*/
		if (! g_seekable_seek (G_SEEKABLE (data_stream),
				       tile_offset,
				       G_SEEK_SET,
				       cancellable,
				       error))
		{
			goto read_error;
		}

		/* decompress the tile data */

		for (t = 0; t < n_tiles; t++) {
			goffset  tile_data_size;
			guchar  *tile_data;
			guchar  *tile_data_p;
			gsize    data_read;
			goffset  tile_pixels_offset;
			int      tile_width;
			int      tile_height;
			guchar  *pixels_row;
			guchar  *pixels;
			int      i, j;

			/* get the tile size and position */

			if (! gimp_layer_get_tile_size (layer,
							t,
							&tile_pixels_offset,
							&tile_width,
							&tile_height))
			{
				/*g_print ("<tile %d: error>", t);*/
				continue;
			}

			/* read the tile data */

			tile_data_size = tile_width * tile_height * bpp;
			if (tile_data_size <= 0) {
				/*g_print ("<tile %d: size error %" G_GUINT32_FORMAT " = %" G_GUINT32_FORMAT " - %" G_GUINT32_FORMAT ">", t, tile_data_size, g_array_index (tile_offsets, guint32, t + 1), g_array_index (tile_offsets, guint32, t));*/
				continue;
			}

			tile_data = g_malloc (tile_data_size);
			data_read = _g_data_input_stream_read_byte_array (data_stream, tile_data, tile_data_size, cancellable, error);
			if (*error != NULL) {
				g_free (tile_data);
				goto read_error;
			}

			if (data_read != tile_data_size) {
				/*g_print ("    tile size %d: %" G_GSIZE_FORMAT " (%" G_GSIZE_FORMAT ")\n", t, data_read, tile_data_size);*/
				g_free (tile_data);
				continue;
			}

			tile_data_p = tile_data;
			pixels_row = image_pixels + tile_pixels_offset;

			for (i = 0; i < tile_height; i++) {
				pixels = pixels_row;

				for (j = 0; j < tile_width; j++) {
					if (base_type == GIMP_INDEXED) {
						guchar *color = (guchar *) (colormap + (*tile_data_p++));
						pixels[CAIRO_RED] = color[0];
						pixels[CAIRO_GREEN] = color[1];
						pixels[CAIRO_BLUE] = color[2];
					}
					else if (bpp >= 3) {
						pixels[CAIRO_RED] = *tile_data_p++;
						pixels[CAIRO_GREEN] = *tile_data_p++;
						pixels[CAIRO_BLUE] = *tile_data_p++;

					}
					else if (bpp <= 2)
						pixels[CAIRO_RED] = *tile_data_p++;

					if ((bpp == 2) || (bpp == 4))
						pixels[CAIRO_ALPHA] = *tile_data_p++;

					pixels += 4;
				}

				pixels_row += row_stride;
			}

			g_free (tile_data);
		}
	}

	_cairo_surface_data_post_production (image_pixels, row_stride, width, height, bpp, base_type);
	cairo_surface_mark_dirty (image);

	g_array_free (tile_offsets, TRUE);

	return image;

read_error:

	if (image != NULL)
		cairo_surface_destroy (image);
	if (tile_offsets != NULL)
		g_array_free (tile_offsets, TRUE);

	return NULL;
}


GthImage *
_cairo_image_surface_create_from_xcf (GInputStream  *istream,
		  	  	      GthFileData   *file_data,
		  	  	      int            requested_size,
		  	  	      int           *original_width,
		  	  	      int           *original_height,
		  	  	      gpointer       user_data,
		  	  	      GCancellable  *cancellable,
		  	  	      GError       **error)
{
	GthImage          *image = NULL;
	cairo_surface_t   *surface = NULL;
	GDataInputStream  *data_stream;
	char              *file_type;
	char              *version;
	guint32            canvas_width;
	guint32            canvas_height;
	GimpImageBaseType  base_type;
	guint32            property_type;
	guint32            payload_length;
	GimpCompression    compression;
	GList             *layers = NULL;
	guint32            n_colors;
	GimpColormap      *colormap = NULL;
	guint              n_properties;
	gboolean           read_properties;
	GArray            *layer_offsets = NULL;
	guint32            layer_offset;
	guint              n_layers;
	guint32            channel_offset;
	guint              n_channels;
	int                i;

	data_stream = g_data_input_stream_new (istream);
	g_data_input_stream_set_byte_order (data_stream, G_DATA_STREAM_BYTE_ORDER_BIG_ENDIAN);

	/* file type magic */

	file_type = _g_data_input_stream_read_c_string (data_stream, 9, cancellable, error);
	if (*error != NULL)
		goto out;

	if (g_strcmp0 (file_type, "gimp xcf ") != 0) {
		g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_INVALID_DATA, "Invalid format");
		g_free (file_type);
		goto out;
	}
	g_free (file_type);

	/* version */

	version = _g_data_input_stream_read_c_string (data_stream, 5, cancellable, error);
	if (*error != NULL)
		goto out;

	/*g_print ("version: %s\n", version);*/
	g_free (version);

	/* canvas size */

	canvas_width = g_data_input_stream_read_uint32 (data_stream, cancellable, error);
	if (*error != NULL)
		goto out;

	canvas_height = g_data_input_stream_read_uint32 (data_stream, cancellable, error);
	if (*error != NULL)
		goto out;

	/*g_print ("canvas size: %" G_GUINT32_FORMAT " x %" G_GUINT32_FORMAT "\n", canvas_width, canvas_height);*/

	/* base type */

	base_type = g_data_input_stream_read_uint32 (data_stream, cancellable, error);
	if (*error != NULL)
		goto out;

	/*g_print ("base type: %d\n", base_type);*/

	/* properties */

	compression = GIMP_COMPRESSION_RLE;
	layers = NULL;
	colormap = NULL;

	read_properties = TRUE;
	n_properties = 0;
	while (read_properties) {
		property_type = g_data_input_stream_read_uint32 (data_stream, cancellable, error);
		if (*error != NULL)
			goto out;

		payload_length = g_data_input_stream_read_uint32 (data_stream, cancellable, error);
		if (*error != NULL)
			goto out;

		n_properties += 1;

		switch (property_type) {
		case 0: /* PROP_END */
			read_properties = FALSE;
			break;

		case 1: /* PROP_COLORMAP */
			n_colors = g_data_input_stream_read_uint32 (data_stream, cancellable, error);
			if (*error != NULL)
				goto out;

			if (base_type == GIMP_INDEXED) {
				int     i;
				guchar *c;

				colormap = g_new (GimpColormap, n_colors);
				c = (guchar *) colormap;
				for (i = 0; i < n_colors; i++) {
					c[0] = g_data_input_stream_read_byte (data_stream, cancellable, error);
					if (*error != NULL)
						goto out;

					c[1] = g_data_input_stream_read_byte (data_stream, cancellable, error);
					if (*error != NULL)
						goto out;

					c[2] = g_data_input_stream_read_byte (data_stream, cancellable, error);
					if (*error != NULL)
						goto out;

					c += 3;
				}
			}
			else { /* when skipping the colormap do not trust the payload_length value. */
				g_input_stream_skip (G_INPUT_STREAM (data_stream), (n_colors * 3), cancellable, error);
				if (*error != NULL)
					goto out;
			}
			break;

		case 17: /* PROP_COMPRESSION */
			compression = g_data_input_stream_read_byte (data_stream, cancellable, error);
			if (*error != NULL)
				goto out;

			/*g_print ("compression: %d\n", compression);*/
			break;

		default:
			g_input_stream_skip (G_INPUT_STREAM (data_stream), payload_length, cancellable, error);
			if (*error != NULL)
				goto out;

			break;
		}
	}
	/*g_print ("properties: %d\n", n_properties);*/

	/* layers */

	n_layers = 0;
	layer_offsets = g_array_new (FALSE, FALSE, sizeof (guint32));
	while ((layer_offset = g_data_input_stream_read_uint32 (data_stream, cancellable, error)) != 0) {
		n_layers += 1;
		g_array_append_val (layer_offsets, layer_offset);
		/*g_print ("layer %d: %" G_GUINT32_FORMAT "\n", n_layers, layer_offset);*/
	}

	if (*error != NULL)
		goto out;

	/* channels */

	n_channels = 0;
	while ((channel_offset = g_data_input_stream_read_uint32 (data_stream, cancellable, error)) != 0) {
		n_channels += 1;
		/*g_print ("channel %d: %" G_GUINT32_FORMAT "\n", n_channels, channel_offset);*/
	}

	if (*error != NULL)
		goto out;

	/* read the layers */

	for (i = 0; i < n_layers; i++) {
		GimpLayer *layer;
		guint32    hierarchy_offset;
		guint32    mask_offset;
		char      *mask_name;

		layer_offset = g_array_index (layer_offsets, guint32, i);
		/*g_print ("layer %d: %" G_GUINT32_FORMAT "\n", i, layer_offset);*/

		if (! g_seekable_seek (G_SEEKABLE (data_stream),
				       layer_offset,
				       G_SEEK_SET,
				       cancellable,
				       error))
		{
			goto out;
		}

		layer = gimp_layer_new ();
		layers = g_list_prepend (layers, layer);

		/* size */

		layer->width = g_data_input_stream_read_uint32 (data_stream, cancellable, error);
		if (*error != NULL)
			goto out;

		layer->height = g_data_input_stream_read_uint32 (data_stream, cancellable, error);
		if (*error != NULL)
			goto out;

		/*g_print ("  size: %" G_GUINT32_FORMAT " x %" G_GUINT32_FORMAT "\n", layer->width, layer->height);*/

		/* type  */

		layer->type = g_data_input_stream_read_uint32 (data_stream, cancellable, error);
		if (*error != NULL)
			goto out;

		/*g_print ("  type: %d\n", layer->type);*/

		/* name */

		layer->name = _g_data_input_stream_read_xcf_string (data_stream, cancellable, error);
		if (*error != NULL)
			goto out;

		/*g_print ("  name: %s\n", layer->name);*/

		/* properties */

		read_properties = TRUE;
		n_properties = 0;
		while (read_properties) {
			property_type = g_data_input_stream_read_uint32 (data_stream, cancellable, error);
			if (*error != NULL)
				goto out;

			payload_length = g_data_input_stream_read_uint32 (data_stream, cancellable, error);
			if (*error != NULL)
				goto out;

			n_properties += 1;

			switch (property_type) {
			case 0: /* PROP_END */
				read_properties = FALSE;
				break;

			case 5: /* PROP_FLOATING_SELECTION */
				layer->floating_selection = TRUE;
				g_input_stream_skip (G_INPUT_STREAM (data_stream), payload_length, cancellable, error);
				break;

			case 6: /* PROP_OPACITY */
				layer->opacity = g_data_input_stream_read_uint32 (data_stream, cancellable, error);
				/*g_print ("  opacity: %d\n", layer->opacity);*/
				break;

			case 7: /* PROP_MODE */
				layer->mode = g_data_input_stream_read_uint32 (data_stream, cancellable, error);
				/*g_print ("  mode: %d\n", layer->mode);*/
				break;

			case 8: /* PROP_VISIBLE */
				layer->visible = g_data_input_stream_read_uint32 (data_stream, cancellable, error);
				/*g_print ("  visible: %d\n", layer->visible);*/
				break;

			case 11: /* PROP_APPLY_MASK */
				layer->apply_mask = (g_data_input_stream_read_uint32 (data_stream, cancellable, error) == 1);
				/*g_print ("  apply mask: %d\n", layer->apply_mask);*/
				break;

			case 15: /* PROP_OFFSETS */
				layer->h_offset = g_data_input_stream_read_uint32 (data_stream, cancellable, error);
				if (*error != NULL)
					goto out;

				layer->v_offset = g_data_input_stream_read_uint32 (data_stream, cancellable, error);
				/*g_print ("  coordinates: %d, %d\n", layer->h_offset, layer->v_offset);*/
				break;

			default:
				g_input_stream_skip (G_INPUT_STREAM (data_stream), payload_length, cancellable, error);
				break;
			}

			if (*error != NULL)
				goto out;
		}
		/*g_print ("  properties: %d\n", n_properties);*/

		/* hierarchy structure offset */

		hierarchy_offset = g_data_input_stream_read_uint32 (data_stream, cancellable, error);
		if (*error != NULL)
			goto out;

		/*g_print ("  hierarchy offset: %" G_GUINT32_FORMAT "\n", hierarchy_offset);*/

		/* layer mask offset */

		mask_offset = g_data_input_stream_read_uint32 (data_stream, cancellable, error);
		if (*error != NULL)
			goto out;

		/*g_print ("  mask offset: %" G_GUINT32_FORMAT "\n", mask_offset);*/

		/* the layer image */

		if (layer->floating_selection || ! layer->visible || (layer->opacity == 0))
			continue;

		layer->image = _cairo_image_surface_create_from_hierarchy (data_stream, hierarchy_offset, layer, colormap, base_type, compression, cancellable, error);
		if (*error != NULL)
			goto out;

		/* read the mask  */

		if (! layer->apply_mask || (mask_offset == 0))
			continue;

		if (! g_seekable_seek (G_SEEKABLE (data_stream),
				       mask_offset,
				       G_SEEK_SET,
				       cancellable,
				       error))
		{
			goto out;
		}

		/* mask width, height and name */

		/* mask_width = */ g_data_input_stream_read_uint32 (data_stream, cancellable, error);
		if (*error != NULL)
			goto out;

		/* mask_height = */ g_data_input_stream_read_uint32 (data_stream, cancellable, error);
		if (*error != NULL)
			goto out;

		mask_name = _g_data_input_stream_read_xcf_string (data_stream, cancellable, error);
		if (*error != NULL)
			goto out;

		g_free (mask_name);

		/* mask properties */

		read_properties = TRUE;
		n_properties = 0;
		while (read_properties) {
			property_type = g_data_input_stream_read_uint32 (data_stream, cancellable, error);
			if (*error != NULL)
				goto out;

			payload_length = g_data_input_stream_read_uint32 (data_stream, cancellable, error);
			if (*error != NULL)
				goto out;

			n_properties += 1;

			switch (property_type) {
			case 0: /* PROP_END */
				read_properties = FALSE;
				break;

			default:
				g_input_stream_skip (G_INPUT_STREAM (data_stream), payload_length, cancellable, error);
				if (*error != NULL)
					goto out;
				break;
			}
		}
		/*g_print ("  mask properties: %d\n", n_properties);*/

		/* the mask image */

		hierarchy_offset = g_data_input_stream_read_uint32 (data_stream, cancellable, error);
		if (*error != NULL)
			goto out;

		/*g_print ("  mask hierarchy offset: %" G_GUINT32_FORMAT "\n", hierarchy_offset);*/

		layer->mask = _cairo_image_surface_create_from_hierarchy (data_stream, hierarchy_offset, layer, colormap, base_type, compression, cancellable, error);
		if (*error != NULL)
			goto out;
	}

	surface = _cairo_image_surface_create_from_layers (canvas_width, canvas_height, base_type, layers);
	image = gth_image_new_for_surface (surface);
	cairo_surface_destroy (surface);

out:

	if (layers != NULL)
		g_list_free_full (layers, (GDestroyNotify) gimp_layer_free);
	if (layer_offsets != NULL)
		g_array_free (layer_offsets, TRUE);
	if (colormap != NULL)
		g_free (colormap);
	if (data_stream != NULL)
		g_object_unref (data_stream);

	return image;
}
