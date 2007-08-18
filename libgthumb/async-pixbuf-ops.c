/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2001, 2003 Free Software Foundation, Inc.
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

/* Some bits are based upon the gimp source code, the original copyright
 * note follows:
 *
 * The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include <config.h>
#include <math.h>
#include <string.h>
#include "pixbuf-utils.h"
#include "gth-pixbuf-op.h"
#include "gthumb-histogram.h"
#include "async-pixbuf-ops.h"

#ifdef HAVE_RINT
#define RINT(x) rint(x)
#else
#define RINT(x) floor ((x) + 0.5)
#endif /* HAVE_RINT */


enum {
	RED_PIX   = 0,
	GREEN_PIX = 1,
	BLUE_PIX  = 2,
	ALPHA_PIX = 3
};


typedef void (*IterateFunc) (guchar *src, gpointer extra_data);

static void
_gdk_pixbuf_iterate (GdkPixbuf   *src,
		     gpointer     extra_data,
		     IterateFunc  iterate_func)
{
        guint   h          = gdk_pixbuf_get_height (src);
	guchar *pixels     = gdk_pixbuf_get_pixels (src);
	int     n_channels = gdk_pixbuf_get_n_channels (src);
	int     rowstride  = gdk_pixbuf_get_rowstride (src);

	while (h--) {
		guint   w = gdk_pixbuf_get_width (src);
                guchar *p = pixels;
                 
		while (w--) {
			(*iterate_func) (p, extra_data); 
			p += n_channels;
		}
  
		pixels += rowstride;
	}
}


/* -- desaturate -- */


static void
desaturate_step (GthPixbufOp *pixop)
{
	guchar min, max, lightness;

	max = MAX (pixop->src_pixel[RED_PIX], pixop->src_pixel[GREEN_PIX]);
	max = MAX (max, pixop->src_pixel[BLUE_PIX]);

	min = MIN (pixop->src_pixel[RED_PIX], pixop->src_pixel[GREEN_PIX]);
	min = MIN (min, pixop->src_pixel[BLUE_PIX]);
	
	lightness = (max + min) / 2;
	
	pixop->dest_pixel[RED_PIX]   = lightness;
	pixop->dest_pixel[GREEN_PIX] = lightness;
	pixop->dest_pixel[BLUE_PIX]  = lightness;
	
	if (pixop->has_alpha)
		pixop->dest_pixel[ALPHA_PIX] = pixop->src_pixel[ALPHA_PIX];
}


GthPixbufOp*
_gdk_pixbuf_desaturate (GdkPixbuf *src,
			GdkPixbuf *dest)
{
	return gth_pixbuf_op_new (src, dest,
				  NULL,
				  desaturate_step,
				  NULL,
				  NULL);
}


/* -- invert -- */


static void
invert_step (GthPixbufOp *pixop)
{
	pixop->dest_pixel[RED_PIX]   = 255 - pixop->src_pixel[RED_PIX];
	pixop->dest_pixel[GREEN_PIX] = 255 - pixop->src_pixel[GREEN_PIX];
	pixop->dest_pixel[BLUE_PIX]  = 255 - pixop->src_pixel[BLUE_PIX];
}


GthPixbufOp* 
_gdk_pixbuf_invert (GdkPixbuf *src,
		    GdkPixbuf *dest)
{
	return gth_pixbuf_op_new (src, dest,
				  NULL,
				  invert_step,
				  NULL,
				  NULL);
}


/* -- brightness contrast -- */


typedef struct {
	double brightness;
	double contrast;
} BCData;


static guchar
bc_func (guchar u_value,
	 double brightness, 
	 double contrast)
{
	float  nvalue;
	double power;
	float  value;

	value = (float) u_value / 255.0;

	/* apply brightness */
	if (brightness < 0.0)
		value = value * (1.0 + brightness);
	else
		value = value + ((1.0 - value) * brightness);
	
	/* apply contrast */
	if (contrast < 0.0) {
		if (value > 0.5)
			nvalue = 1.0 - value;
		else
			nvalue = value;

		if (nvalue < 0.0)
			nvalue = 0.0;

		nvalue = 0.5 * pow (nvalue * 2.0 , (double) (1.0 + contrast));

		if (value > 0.5)
			value = 1.0 - nvalue;
		else
			value = nvalue;
	} else {
		if (value > 0.5)
			nvalue = 1.0 - value;
		else
			nvalue = value;
		
		if (nvalue < 0.0)
			nvalue = 0.0;
		
		power = (contrast == 1.0) ? 127 : 1.0 / (1.0 - contrast);
		nvalue = 0.5 * pow (2.0 * nvalue, power);
		
		if (value > 0.5)
			value = 1.0 - nvalue;
		else
			value = nvalue;
	}
	
	return (guchar) (value * 255);
}


static void
brightness_contrast_step (GthPixbufOp *pixop)
{
	BCData *data = pixop->data;

	pixop->dest_pixel[RED_PIX]   = bc_func (pixop->src_pixel[RED_PIX],
						data->brightness,
						data->contrast);
	pixop->dest_pixel[GREEN_PIX] = bc_func (pixop->src_pixel[GREEN_PIX],
						data->brightness,
						data->contrast);
	pixop->dest_pixel[BLUE_PIX]  = bc_func (pixop->src_pixel[BLUE_PIX],
						data->brightness,
						data->contrast);
}


static void
brightness_contrast_release (GthPixbufOp *pixop)
{
	g_free (pixop->data);
}


GthPixbufOp*
_gdk_pixbuf_brightness_contrast (GdkPixbuf *src,
				 GdkPixbuf *dest,
				 double     brightness,
				 double     contrast)
{
	BCData *data;

	data = g_new (BCData, 1);

	data->brightness = brightness;
	data->contrast = contrast;

	return gth_pixbuf_op_new (src, dest,
				  NULL,
				  brightness_contrast_step,
				  brightness_contrast_release,
				  data);	
}


/* -- posterize -- */


typedef struct {
	int levels;
} PosterizeData;


static guchar
posterize_func (guchar u_value,
		int    levels)
{
	double value;

	value = (float) u_value / 255.0;
	value = RINT (value * (levels - 1.0)) / (levels - 1.0);

	return (guchar) (value * 255.0);
}


static void
posterize_step (GthPixbufOp *pixop)
{
	PosterizeData *data = pixop->data;

	pixop->dest_pixel[RED_PIX]   = posterize_func (pixop->src_pixel[RED_PIX], data->levels);
	pixop->dest_pixel[GREEN_PIX] = posterize_func (pixop->src_pixel[GREEN_PIX], data->levels);
	pixop->dest_pixel[BLUE_PIX]  = posterize_func (pixop->src_pixel[BLUE_PIX], data->levels);
}


static void
posterize_release (GthPixbufOp *pixop)
{
	g_free (pixop->data);
}


GthPixbufOp*
_gdk_pixbuf_posterize (GdkPixbuf *src,
		       GdkPixbuf *dest, 
		       int        levels)
{
	PosterizeData *data;

	data = g_new (PosterizeData, 1);

	if (levels < 2)
		levels = 2;
	data->levels = levels;

	return gth_pixbuf_op_new (src, dest,
				  NULL,
				  posterize_step,
				  posterize_release,
				  data);	
}


/* -- gimpcolorspace -- */


static void
gimp_rgb_to_hls_int (gint *red,
		     gint *green,
		     gint *blue)
{
  gint    r, g, b;
  gdouble h, l, s;
  gint    min, max;
  gint    delta;

  r = *red;
  g = *green;
  b = *blue;

  if (r > g)
    {
      max = MAX (r, b);
      min = MIN (g, b);
    }
  else
    {
      max = MAX (g, b);
      min = MIN (r, b);
    }

  l = (max + min) / 2.0;

  if (max == min)
    {
      s = 0.0;
      h = 0.0;
    }
  else
    {
      delta = (max - min);

      if (l < 128)
	s = 255 * (gdouble) delta / (gdouble) (max + min);
      else
	s = 255 * (gdouble) delta / (gdouble) (511 - max - min);

      if (r == max)
	h = (g - b) / (gdouble) delta;
      else if (g == max)
	h = 2 + (b - r) / (gdouble) delta;
      else
	h = 4 + (r - g) / (gdouble) delta;

      h = h * 42.5;

      if (h < 0)
	h += 255;
      else if (h > 255)
	h -= 255;
    }

  *red   = h;
  *green = l;
  *blue  = s;
}


static gint
gimp_rgb_to_l_int (gint red,
		   gint green,
		   gint blue)
{
  gint min, max;

  if (red > green)
    {
      max = MAX (red,   blue);
      min = MIN (green, blue);
    }
  else
    {
      max = MAX (green, blue);
      min = MIN (red,   blue);
    }

  return (max + min) / 2.0;
}

static gint
gimp_hls_value (gdouble n1,
		gdouble n2,
		gdouble hue)
{
  gdouble value;

  if (hue > 255)
    hue -= 255;
  else if (hue < 0)
    hue += 255;
  if (hue < 42.5)
    value = n1 + (n2 - n1) * (hue / 42.5);
  else if (hue < 127.5)
    value = n2;
  else if (hue < 170)
    value = n1 + (n2 - n1) * ((170 - hue) / 42.5);
  else
    value = n1;

  return (gint) (value * 255);
}


static void
gimp_hls_to_rgb_int (gint *hue,
		     gint *lightness,
		     gint *saturation)
{
  gdouble h, l, s;
  gdouble m1, m2;

  h = *hue;
  l = *lightness;
  s = *saturation;

  if (s == 0)
    {
      /*  achromatic case  */
      *hue        = l;
      *lightness  = l;
      *saturation = l;
    }
  else
    {
      if (l < 128)
	m2 = (l * (255 + s)) / 65025.0;
      else
	m2 = (l + s - (l * s) / 255.0) / 255.0;

      m1 = (l / 127.5) - m2;

      /*  chromatic case  */
      *hue        = gimp_hls_value (m1, m2, h + 85);
      *lightness  = gimp_hls_value (m1, m2, h);
      *saturation = gimp_hls_value (m1, m2, h - 85);
    }
}


/* -- hue, lightness, saturation -- */


typedef enum
{
	GIMP_ALL_HUES,
	GIMP_RED_HUES,
	GIMP_YELLOW_HUES,
	GIMP_GREEN_HUES,
	GIMP_CYAN_HUES,
	GIMP_BLUE_HUES,
	GIMP_MAGENTA_HUES
} GimpHueRange;


typedef struct 
{
	double hue[7];
	double lightness[7];
	double saturation[7];

	int    hue_transfer[6][256];
	int    lightness_transfer[6][256];
	int    saturation_transfer[6][256];
} HueSaturationData;


static void
hue_saturation_data_init (HueSaturationData *hs)
{
	GimpHueRange partition;

	g_return_if_fail (hs != NULL);

	for (partition = GIMP_ALL_HUES; partition <= GIMP_MAGENTA_HUES; partition++) {
		hs->hue[partition]        = 0.0;
		hs->lightness[partition]  = 0.0;
		hs->saturation[partition] = 0.0;
	}
}


static void
hue_saturation_calculate_transfers (HueSaturationData *hs)
{
	int value;
	int hue;
	int i;

	g_return_if_fail (hs != NULL);

	/*  Calculate transfers  */
	for (hue = 0; hue < 6; hue++)
		for (i = 0; i < 256; i++) {
			/* Hue */
			value = (hs->hue[0] + hs->hue[hue + 1]) * 255.0 / 360.0;
			if ((i + value) < 0)
				hs->hue_transfer[hue][i] = 255 + (i + value);
			else if ((i + value) > 255)
				hs->hue_transfer[hue][i] = i + value - 255;
			else
				hs->hue_transfer[hue][i] = i + value;

			/*  Lightness  */
			value = (hs->lightness[0] + hs->lightness[hue + 1]) * 127.0 / 100.0;
			value = CLAMP (value, -255, 255);
			if (value < 0)
				hs->lightness_transfer[hue][i] = (unsigned char) ((i * (255 + value)) / 255);
			else
				hs->lightness_transfer[hue][i] = (unsigned char) (i + ((255 - i) * value) / 255);
			
			/*  Saturation  */
			value = (hs->saturation[0] + hs->saturation[hue + 1]) * 255.0 / 100.0;
			value = CLAMP (value, -255, 255);
			hs->saturation_transfer[hue][i] = CLAMP ((i * (255 + value)) / 255, 0, 255);
		}
}


static void
hue_saturation_init (GthPixbufOp *pixop)
{
	HueSaturationData *data = pixop->data;
	hue_saturation_calculate_transfers (data);
}


static void
hue_saturation_step (GthPixbufOp *pixop)
{
	HueSaturationData *data = pixop->data;
	int                r, g, b, hue_idx;

	r = pixop->src_pixel[RED_PIX];
	g = pixop->src_pixel[GREEN_PIX];
	b = pixop->src_pixel[BLUE_PIX];
	
	gimp_rgb_to_hls_int (&r, &g, &b);
	
	if (r < 43)
		hue_idx = 0;
	else if (r < 85)
		hue_idx = 1;
	else if (r < 128)
		hue_idx = 2;
	else if (r < 171)
		hue_idx = 3;
	else if (r < 213)
		hue_idx = 4;
	else
		hue_idx = 5;
	
	r = data->hue_transfer[hue_idx][r];
	g = data->lightness_transfer[hue_idx][g];
	b = data->saturation_transfer[hue_idx][b];
	
	gimp_hls_to_rgb_int (&r, &g, &b);
	
	pixop->dest_pixel[RED_PIX] = r;
	pixop->dest_pixel[GREEN_PIX] = g;
	pixop->dest_pixel[BLUE_PIX] = b;
	
	if (pixop->has_alpha)
		pixop->dest_pixel[ALPHA_PIX] = pixop->src_pixel[ALPHA_PIX];
}


static void
hue_saturation_release (GthPixbufOp *pixop)
{
	g_free (pixop->data);
}


GthPixbufOp*
_gdk_pixbuf_hue_lightness_saturation (GdkPixbuf *src,
				      GdkPixbuf *dest,
				      double     hue,
				      double     lightness,
				      double     saturation)
{
	HueSaturationData *data;

	data = g_new (HueSaturationData, 1);

	hue_saturation_data_init (data);
	data->hue[GIMP_ALL_HUES] = hue;
	data->lightness[GIMP_ALL_HUES] = lightness;
	data->saturation[GIMP_ALL_HUES] = saturation;

	return gth_pixbuf_op_new (src, dest,
				  hue_saturation_init,
				  hue_saturation_step,
				  hue_saturation_release,
				  data);
}


/*  -- color balance -- */


#define SQR(x) ((x) * (x))
#define CLAMP0255(a) CLAMP(a,0,255)


typedef enum {
	GIMP_SHADOWS,
	GIMP_MIDTONES,
	GIMP_HIGHLIGHTS
} GimpTransferMode;


typedef struct {
	double   cyan_red[3];
	double   magenta_green[3];
	double   yellow_blue[3];
	
	guchar   r_lookup[256];
	guchar   g_lookup[256];
	guchar   b_lookup[256];

	gboolean preserve_luminosity;

	/*  for lightening  */
	double   highlights_add[256];
	double   midtones_add[256];
	double   shadows_add[256];

	/*  for darkening  */
	double   highlights_sub[256];
	double   midtones_sub[256];
	double   shadows_sub[256];
} ColorBalanceData;


static void
color_balance_data_init (ColorBalanceData *cb)
{
	GimpTransferMode range;

	g_return_if_fail (cb != NULL);

	for (range = GIMP_SHADOWS; range <= GIMP_HIGHLIGHTS; range++) {
		cb->cyan_red[range]      = 0.0;
		cb->magenta_green[range] = 0.0;
		cb->yellow_blue[range]   = 0.0;
	}
}


static void
color_balance_transfer_init (ColorBalanceData *data)
{
	int i;

	for (i = 0; i < 256; i++) {
		data->highlights_add[i] = data->shadows_sub[255 - i] = (1.075 - 1 / ((double) i / 16.0 + 1));

		data->midtones_add[i] =data-> midtones_sub[i] = 0.667 * (1 - SQR (((double) i - 127.0) / 127.0));

		data->shadows_add[i] = data->highlights_sub[i] = 0.667 * (1 - SQR (((double) i - 127.0) / 127.0));
	}
}


static void
color_balance_create_lookup_tables (ColorBalanceData *cb)
{
	double  *cyan_red_transfer[3];
	double  *magenta_green_transfer[3];
	double  *yellow_blue_transfer[3];
	int      i;
	gint32   r_n, g_n, b_n;
	
	g_return_if_fail (cb != NULL);
	
	color_balance_transfer_init (cb);

	/*  Set the transfer arrays  (for speed)  */
	cyan_red_transfer[GIMP_SHADOWS] = (cb->cyan_red[GIMP_SHADOWS] > 0) ? cb->shadows_add : cb->shadows_sub;
	cyan_red_transfer[GIMP_MIDTONES] = (cb->cyan_red[GIMP_MIDTONES] > 0) ? cb->midtones_add : cb->midtones_sub;
	cyan_red_transfer[GIMP_HIGHLIGHTS] = (cb->cyan_red[GIMP_HIGHLIGHTS] > 0) ? cb->highlights_add : cb->highlights_sub;
	
	magenta_green_transfer[GIMP_SHADOWS] = (cb->magenta_green[GIMP_SHADOWS] > 0) ? cb->shadows_add : cb->shadows_sub;
	magenta_green_transfer[GIMP_MIDTONES] =	(cb->magenta_green[GIMP_MIDTONES] > 0) ? cb->midtones_add : cb->midtones_sub;
	magenta_green_transfer[GIMP_HIGHLIGHTS] = (cb->magenta_green[GIMP_HIGHLIGHTS] > 0) ? cb->highlights_add : cb->highlights_sub;

	yellow_blue_transfer[GIMP_SHADOWS] = (cb->yellow_blue[GIMP_SHADOWS] > 0) ? cb->shadows_add : cb->shadows_sub;
	yellow_blue_transfer[GIMP_MIDTONES] = (cb->yellow_blue[GIMP_MIDTONES] > 0) ? cb->midtones_add : cb->midtones_sub;
	yellow_blue_transfer[GIMP_HIGHLIGHTS] =	(cb->yellow_blue[GIMP_HIGHLIGHTS] > 0) ? cb->highlights_add : cb->highlights_sub;
	
	for (i = 0; i < 256; i++) {
		r_n = i;
		g_n = i;
		b_n = i;
		
		r_n += cb->cyan_red[GIMP_SHADOWS] * cyan_red_transfer[GIMP_SHADOWS][r_n];
		r_n = CLAMP0255 (r_n);
		r_n += cb->cyan_red[GIMP_MIDTONES] * cyan_red_transfer[GIMP_MIDTONES][r_n];
		r_n = CLAMP0255 (r_n);
		r_n += cb->cyan_red[GIMP_HIGHLIGHTS] * cyan_red_transfer[GIMP_HIGHLIGHTS][r_n];
		r_n = CLAMP0255 (r_n);
		
		g_n += cb->magenta_green[GIMP_SHADOWS] * magenta_green_transfer[GIMP_SHADOWS][g_n];
		g_n = CLAMP0255 (g_n);
		g_n += cb->magenta_green[GIMP_MIDTONES] * magenta_green_transfer[GIMP_MIDTONES][g_n];
		g_n = CLAMP0255 (g_n);
		g_n += cb->magenta_green[GIMP_HIGHLIGHTS] * magenta_green_transfer[GIMP_HIGHLIGHTS][g_n];
		g_n = CLAMP0255 (g_n);
		
		b_n += cb->yellow_blue[GIMP_SHADOWS] * yellow_blue_transfer[GIMP_SHADOWS][b_n];
		b_n = CLAMP0255 (b_n);
		b_n += cb->yellow_blue[GIMP_MIDTONES] * yellow_blue_transfer[GIMP_MIDTONES][b_n];
		b_n = CLAMP0255 (b_n);
		b_n += cb->yellow_blue[GIMP_HIGHLIGHTS] * yellow_blue_transfer[GIMP_HIGHLIGHTS][b_n];
		b_n = CLAMP0255 (b_n);
		
		cb->r_lookup[i] = r_n;
		cb->g_lookup[i] = g_n;
		cb->b_lookup[i] = b_n;
	}
}


static void
color_balance_init (GthPixbufOp *pixop)
{
	ColorBalanceData *data = pixop->data;
	int               i;

	for (i = 0; i < 256; i++) {
		data->highlights_add[i] = 0.0;
		data->midtones_add[i] = 0.0;
		data->shadows_add[i] = 0.0;

		data->highlights_sub[i] = 0.0;
		data->midtones_sub[i] = 0.0;
		data->shadows_sub[i] = 0.0;
	}

	color_balance_create_lookup_tables (data);
}


static void
color_balance_step (GthPixbufOp *pixop)
{
	ColorBalanceData *data = pixop->data;
	int               r, g, b;
	int               r_n, g_n, b_n;

	r = pixop->src_pixel[RED_PIX];
	g = pixop->src_pixel[GREEN_PIX];
	b = pixop->src_pixel[BLUE_PIX];
	
	r_n = data->r_lookup[r];
	g_n = data->g_lookup[g];
	b_n = data->b_lookup[b];
	
	if (data->preserve_luminosity) {
		gimp_rgb_to_hls_int (&r_n, &g_n, &b_n);
		g_n = gimp_rgb_to_l_int (r, g, b);
		gimp_hls_to_rgb_int (&r_n, &g_n, &b_n);
	}
	
	pixop->dest_pixel[RED_PIX]   = r_n;
	pixop->dest_pixel[GREEN_PIX] = g_n;
	pixop->dest_pixel[BLUE_PIX]  = b_n;
	
	if (pixop->has_alpha)
		pixop->dest_pixel[ALPHA_PIX] = pixop->src_pixel[ALPHA_PIX];
}


static void
color_balance_release (GthPixbufOp *pixop)
{
	g_free (pixop->data);
}


GthPixbufOp*
_gdk_pixbuf_color_balance (GdkPixbuf *src,
			   GdkPixbuf *dest,
			   double     cyan_red,
			   double     magenta_green,
			   double     yellow_blue,
			   gboolean   preserve_luminosity)
{
	ColorBalanceData *data;

	data = g_new (ColorBalanceData, 1);
	data->preserve_luminosity = preserve_luminosity;

	color_balance_data_init (data);
	data->cyan_red[GIMP_MIDTONES]      = cyan_red;
	data->magenta_green[GIMP_MIDTONES] = magenta_green;
	data->yellow_blue[GIMP_MIDTONES]   = yellow_blue;

	return gth_pixbuf_op_new (src, dest,
				  color_balance_init,
				  color_balance_step,
				  color_balance_release,
				  data);	
}


/* -- equalize histogram -- */

typedef struct {
	GthumbHistogram  *histogram;
	int             **part;
} EqHistogramData;


#define MAX_N_CHANNELS 4


static void
eq_histogram_setup (GthumbHistogram  *hist, 
		    int             **part)
{
	int  i, k, j;
	int  pixels_per_value;
	int  desired;
	int  sum, dif;

	pixels_per_value = gthumb_histogram_get_count (hist, 0, 255) / 256.0;

	for (k = 0; k < gthumb_histogram_get_nchannels (hist); k++) {
		/* First and last points in partition */
		part[k][0]   = 0;
		part[k][256] = 256;

		/* Find intermediate points */
		j   = 0;
		sum = (gthumb_histogram_get_value (hist, k + 1, 0) + 
		       gthumb_histogram_get_value (hist, k + 1, 1));
		
		for (i = 1; i < 256; i++) {
			desired = i * pixels_per_value;

			while (sum <= desired) {
				j++;
				sum += gthumb_histogram_get_value (hist, k + 1, j + 1);
			}

			/* Nearest sum */
			dif = sum - gthumb_histogram_get_value (hist, k + 1, j);

			if ((sum - desired) > (dif / 2.0))
				part[k][i] = j;
			else
				part[k][i] = j + 1;
		}
	}
}


static void
eq_histogram_init (GthPixbufOp *pixop)
{
	EqHistogramData *data = pixop->data;
	int              i;

	data->histogram = gthumb_histogram_new ();
	gthumb_histogram_calculate (data->histogram, pixop->src);

	data->part = g_new0 (int *, MAX_N_CHANNELS + 1);
	for (i = 0; i < MAX_N_CHANNELS + 1; i++)
		data->part[i] = g_new0 (int, 257);

	eq_histogram_setup (data->histogram, data->part);
}


static guchar
eq_func (guchar   u_value,
	 int    **part,
	 int      channel)
{
	guchar i = 0;
	while (part[channel][i + 1] <= u_value)
		i++;
	return i;
}


static void
eq_histogram_step (GthPixbufOp *pixop)
{
	EqHistogramData *data = pixop->data;

	pixop->dest_pixel[RED_PIX]   = eq_func (pixop->src_pixel[RED_PIX], data->part, 0);
	pixop->dest_pixel[GREEN_PIX] = eq_func (pixop->src_pixel[GREEN_PIX], data->part, 1);
	pixop->dest_pixel[BLUE_PIX]  = eq_func (pixop->src_pixel[BLUE_PIX], data->part, 2);
	
	if (pixop->has_alpha)
		pixop->dest_pixel[ALPHA_PIX] = eq_func (pixop->src_pixel[ALPHA_PIX], data->part, 3);
}


static void
eq_histogram_release (GthPixbufOp *pixop)
{
	EqHistogramData *data = pixop->data;
	int              i;

	for (i = 0; i < MAX_N_CHANNELS + 1; i++)
		g_free (data->part[i]);
	g_free (data->part);

	gthumb_histogram_free (data->histogram);

	g_free (data);
}


GthPixbufOp*
_gdk_pixbuf_eq_histogram (GdkPixbuf *src,
			  GdkPixbuf *dest)
{
	EqHistogramData *data;

	data = g_new (EqHistogramData, 1);

	return gth_pixbuf_op_new (src, dest,
				  eq_histogram_init,
				  eq_histogram_step,
				  eq_histogram_release,
				  data);	
}


/* -- adjust levels -- */


typedef struct {
	double gamma[5];
	
	double low_input[5];
	double high_input[5];
	
	double low_output[5];
	double high_output[5];
} Levels;


typedef struct {
	GthumbHistogram *hist;
	Levels          *levels;
} AdjustLevelsData;


static void
levels_channel_auto (Levels          *levels,
		     GthumbHistogram *hist,
		     int              channel)
{
	int    i;
	double count, new_count, percentage, next_percentage;

	g_return_if_fail (levels != NULL);
	g_return_if_fail (hist != NULL);

	levels->gamma[channel]       = 1.0;
	levels->low_output[channel]  = 0;
	levels->high_output[channel] = 255;
	
	count = gthumb_histogram_get_count (hist, 0, 255);
	
	if (count == 0.0) {
		levels->low_input[channel]  = 0;
		levels->high_input[channel] = 0;

	} else {
		/*  Set the low input  */

		new_count = 0.0;
		for (i = 0; i < 255; i++) {
			double value;
			double next_value;

			value = gthumb_histogram_get_value (hist, channel, i);
			next_value = gthumb_histogram_get_value (hist, channel, i + 1);

			new_count += value;
			percentage = new_count / count;
			next_percentage = (new_count + next_value) / count;

			if (fabs (percentage - 0.006) < fabs (next_percentage - 0.006)) {
				levels->low_input[channel] = i + 1;
				break;
			}
		}

		/*  Set the high input  */

		new_count = 0.0;
		for (i = 255; i > 0; i--) {
			double value;
			double next_value;

			value = gthumb_histogram_get_value (hist, channel, i);
			next_value = gthumb_histogram_get_value (hist, channel, i - 1);

			new_count += value;
			percentage = new_count / count;
			next_percentage = (new_count + next_value) / count;

			if (fabs (percentage - 0.006) < fabs (next_percentage - 0.006))	{
				levels->high_input[channel] = i - 1;
				break;
			}
		}
	}
}


static void
adjust_levels_init (GthPixbufOp *pixop)
{
	AdjustLevelsData *data = pixop->data;
	int               channel;

	data->hist = gthumb_histogram_new ();
	gthumb_histogram_calculate (data->hist, pixop->src);

	data->levels = g_new0 (Levels, 1);

	for (channel = 0; channel < MAX_N_CHANNELS + 1; channel++) {
		data->levels->gamma[channel]       = 1.0;
		data->levels->low_input[channel]   = 0;
		data->levels->high_input[channel]  = 255;
		data->levels->low_output[channel]  = 0;
		data->levels->high_output[channel] = 255;
	}

	for (channel = 1; channel < MAX_N_CHANNELS; channel++) 
		levels_channel_auto (data->levels, data->hist, channel);
}


static guchar
levels_func (guchar  value,
	     Levels *levels,
	     int     channel)
{
	double inten;
	int    j;
	
	inten = value;
	
	/* For color  images this runs through the loop with j = channel + 1
	 * the first time and j = 0 the second time
	 *
	 * For bw images this runs through the loop with j = 0 the first and
	 *  only time
	 */
	for (j = channel + 1; j >= 0; j -= (channel + 1)) {
		inten /= 255.0;

		/*  determine input intensity  */

		if (levels->high_input[j] != levels->low_input[j])
			inten = (255.0 * inten - levels->low_input[j]) /
				(levels->high_input[j] - levels->low_input[j]);
		else
			inten = 255.0 * inten - levels->low_input[j];
		
		if (levels->gamma[j] != 0.0) {
			if (inten >= 0.0)
				inten =  pow ( inten, (1.0 / levels->gamma[j]));
			else
				inten = -pow (-inten, (1.0 / levels->gamma[j]));
		}

		/*  determine the output intensity  */

		if (levels->high_output[j] >= levels->low_output[j])
			inten = inten * (levels->high_output[j] - levels->low_output[j]) +
				levels->low_output[j];
		else if (levels->high_output[j] < levels->low_output[j])
			inten = levels->low_output[j] - inten *
				(levels->low_output[j] - levels->high_output[j]);
	}

	if (inten < 0.0) 
		inten = 0.0;
	else if (inten > 255.0)	
		inten = 255.0;

	return (guchar) inten;
}


static void
adjust_levels_step (GthPixbufOp *pixop)
{
	AdjustLevelsData *data = pixop->data;

	pixop->dest_pixel[RED_PIX]   = levels_func (pixop->src_pixel[RED_PIX], data->levels, 0);
	pixop->dest_pixel[GREEN_PIX] = levels_func (pixop->src_pixel[GREEN_PIX], data->levels, 1);
	pixop->dest_pixel[BLUE_PIX]  = levels_func (pixop->src_pixel[BLUE_PIX], data->levels, 2);
	
	if (pixop->has_alpha)
		pixop->dest_pixel[ALPHA_PIX] = pixop->src_pixel[ALPHA_PIX];
}


static void
adjust_levels_release (GthPixbufOp *pixop)
{
	AdjustLevelsData *data = pixop->data;

	gthumb_histogram_free (data->hist);
	g_free (data->levels);

	g_free (data);
}


GthPixbufOp*
_gdk_pixbuf_adjust_levels (GdkPixbuf *src,
			   GdkPixbuf *dest)
{
	AdjustLevelsData *data;

	data = g_new (AdjustLevelsData, 1);

	return gth_pixbuf_op_new (src, dest,
				  adjust_levels_init,
				  adjust_levels_step,
				  adjust_levels_release,
				  data);	
}


typedef struct {
	int      alpha;
	guchar   lut[256][3];
	guchar   min[3];
	guchar   max[3];
	gboolean has_alpha;
} StretchContrastData;


static void
stretch__find_min_max (guchar   *src,
		       gpointer  extra_data)
{
	StretchContrastData *data = extra_data;
	int b;

	for (b = 0; b < data->alpha; b++) 
		if (! data->has_alpha || src[data->alpha]) {
			if (src[b] < data->min[b])
				data->min[b] = src[b];
			if (src[b] > data->max[b])
				data->max[b] = src[b];
		}
}


static void
stretch_contrast_init (GthPixbufOp *pixop)
{
	StretchContrastData *data = pixop->data;
	int b;

	data->has_alpha = gdk_pixbuf_get_has_alpha (pixop->src);
	data->alpha = gdk_pixbuf_get_n_channels (pixop->src);

	data->min[0] = data->min[1] = data->min[2] = 255;
	data->max[0] = data->max[1] = data->max[2] = 0;
	_gdk_pixbuf_iterate (pixop->src, data, stretch__find_min_max);

	/* Calculate LUTs with stretched contrast */
	for (b = 0; b < data->alpha; b++) {
		int range = data->max[b] - data->min[b];

		if (range != 0) {
			int x;
			for (x = data->min[b]; x <= data->max[b]; x++)
				data->lut[x][b] = 255 * (x - data->min[b]) / range;
		} else
			data->lut[data->min[b]][b] = data->min[b];
	}
}


static void
stretch_contrast_step (GthPixbufOp *pixop)
{
	StretchContrastData *data = pixop->data;
	int b;

	for (b = 0; b < data->alpha; b++)
		pixop->dest_pixel[b] = data->lut[pixop->src_pixel[b]][b];

	if (data->has_alpha)
		pixop->dest_pixel[data->alpha] = pixop->src_pixel[data->alpha];
}


static void
stretch_contrast_release (GthPixbufOp *pixop)
{
	g_free (pixop->data);
}


GthPixbufOp*
_gdk_pixbuf_stretch_contrast (GdkPixbuf *src,
			      GdkPixbuf *dest)
{
	StretchContrastData *data;

	data = g_new (StretchContrastData, 1);

	return gth_pixbuf_op_new (src, dest,
				  stretch_contrast_init,
				  stretch_contrast_step,
				  stretch_contrast_release,
				  data);	
}


typedef struct {
	int      alpha;
	guchar   lut[256];
	guchar   min;
	guchar   max;
	gboolean has_alpha;
} NormalizeContrastData;


static void
normalize__find_min_max (guchar   *src,
			 gpointer  extra_data)
{
	NormalizeContrastData *data = extra_data;
	int b;

	for (b = 0; b < data->alpha; b++) 
		if (! data->has_alpha || src[data->alpha]) {
			if (src[b] < data->min)
				data->min = src[b];
			if (src[b] > data->max)
				data->max = src[b];
		}
}


static void
normalize_contrast_init (GthPixbufOp *pixop)
{
	NormalizeContrastData *data = pixop->data;
	int range;

	data->has_alpha = gdk_pixbuf_get_has_alpha (pixop->src);
	data->alpha = gdk_pixbuf_get_n_channels (pixop->src);

	data->min = 255;
	data->max = 0;
	_gdk_pixbuf_iterate (pixop->src, data, normalize__find_min_max);

	/* Calculate LUT */

	range = data->max - data->min;

	if (range != 0) {
		int x;
		for (x = data->min; x <= data->max; x++)
			data->lut[x] = 255 * (x - data->min) / range;
	} else
		data->lut[data->min] = data->min;
}


static void
normalize_contrast_step (GthPixbufOp *pixop)
{
	NormalizeContrastData *data = pixop->data;
	int b;

	for (b = 0; b < data->alpha; b++)
		pixop->dest_pixel[b] = data->lut[pixop->src_pixel[b]];
                                                                               
	if (data->has_alpha)
		pixop->dest_pixel[data->alpha] = pixop->src_pixel[data->alpha];
}


static void
normalize_contrast_release (GthPixbufOp *pixop)
{
	g_free (pixop->data);
}


GthPixbufOp*
_gdk_pixbuf_normalize_contrast (GdkPixbuf *src,
				GdkPixbuf *dest)
{
	NormalizeContrastData *data;

	data = g_new (NormalizeContrastData, 1);

	return gth_pixbuf_op_new (src, dest,
				  normalize_contrast_init,
				  normalize_contrast_step,
				  normalize_contrast_release,
				  data);	
}


/* -- image dithering  -- */


typedef struct {
	GthDither dither_type;
	double *error_row_1;
	double *error_row_2;
} DitherData;


static void
dither_init (GthPixbufOp *pixop)
{
	DitherData *data = pixop->data;

	data->error_row_1 = g_new0 (double, pixop->width * pixop->bytes_per_pixel);
	data->error_row_2 = g_new0 (double, pixop->width * pixop->bytes_per_pixel);
}


static double*
get_error_pixel (GthPixbufOp *pixop,
		 gboolean     next_line,
		 int          column)
{
	DitherData *data = pixop->data;
	int         ofs;

	if ((column < 0) || (column >= pixop->width))
		return NULL;
	if (next_line && (pixop->line >= pixop->height - 1))
		return NULL;

	ofs = column * pixop->bytes_per_pixel;
	if (next_line)
		return data->error_row_2 + ofs;
	else 
		return data->error_row_1 + ofs;
}


static void
distribute_error (GthPixbufOp *pixop,
		  double      *dest,
		  double      *src,
		  double       weight)
{
	int i;

	if (dest == NULL)
		return;

	for (i = 0; i < pixop->bytes_per_pixel; i++)
		dest[i] += weight * src[i];
}


static guint
shade_value (guint val,
	     int   shades)
{
	double dval = val;
	double shade_size = 256.0 / (shades - 1);

	dval = dval / shade_size;
	dval = floor (dval + 0.5);
	dval = shade_size * dval;

	if (dval > 255.0)
		dval = 255.0;
	if (dval < 0.0)
		dval = 0.0;

	return (guint) dval;
}


static void
dither_step (GthPixbufOp *pixop)
{
	DitherData *data = pixop->data;
	guchar      min, max, lightness;
	int         red_pix, green_pix, blue_pix, alpha_pix = 0;
	double     *error_pixel, *ngh_error_pixel;
	int         dir = 1;

	if (pixop->line_step == 0) {
		double *tmp;
		int     row_size;

		tmp = data->error_row_1;
		data->error_row_1 = data->error_row_2;
		data->error_row_2 = tmp;

		row_size = pixop->width * pixop->bytes_per_pixel * sizeof (double);
		memset (data->error_row_2, 0, row_size);
	}

	/**/

	if (data->dither_type == GTH_DITHER_BLACK_WHITE) { /* Desaturate for better result. */
		red_pix   = pixop->src_pixel[RED_PIX];
		green_pix = pixop->src_pixel[GREEN_PIX];
		blue_pix  = pixop->src_pixel[BLUE_PIX];

		max = MAX (red_pix, green_pix);
		max = MAX (max, blue_pix);
		min = MIN (red_pix, green_pix);
		min = MIN (min, blue_pix);
		lightness = (max + min) / 2;
		
		pixop->src_pixel[RED_PIX]   = lightness;
		pixop->src_pixel[GREEN_PIX] = lightness;
		pixop->src_pixel[BLUE_PIX]  = lightness;
	}

	/**/

	error_pixel = get_error_pixel (pixop, 0, pixop->column);

	red_pix   = CLAMP (pixop->src_pixel[RED_PIX] + error_pixel[RED_PIX], 0, 255);
	green_pix = CLAMP (pixop->src_pixel[GREEN_PIX] + error_pixel[GREEN_PIX], 0, 255);
	blue_pix  = CLAMP (pixop->src_pixel[BLUE_PIX] + error_pixel[BLUE_PIX], 0, 255);
	if (pixop->has_alpha)
		alpha_pix = CLAMP (pixop->src_pixel[ALPHA_PIX] + error_pixel[ALPHA_PIX], 0, 255);

	/* Find the closest color available. */

	if (data->dither_type == GTH_DITHER_BLACK_WHITE) {
		max = MAX (red_pix, green_pix);
		max = MAX (max, blue_pix);
		min = MIN (red_pix, green_pix);
		min = MIN (min, blue_pix);
		lightness = (((max + min) / 2) > 125) ? 255 : 0;
		
		pixop->dest_pixel[RED_PIX]   = lightness;
		pixop->dest_pixel[GREEN_PIX] = lightness;
		pixop->dest_pixel[BLUE_PIX]  = lightness;

	} else if (data->dither_type == GTH_DITHER_WEB_PALETTE) {
		pixop->dest_pixel[RED_PIX]   = shade_value (red_pix, 6);
		pixop->dest_pixel[GREEN_PIX] = shade_value (green_pix, 6);
		pixop->dest_pixel[BLUE_PIX]  = shade_value (blue_pix, 6);
	}

	if (pixop->has_alpha)
		pixop->dest_pixel[ALPHA_PIX] = pixop->src_pixel[ALPHA_PIX];

	/* Calculate the error values. */

	error_pixel[RED_PIX]   = red_pix - pixop->dest_pixel[RED_PIX];
	error_pixel[GREEN_PIX] = green_pix - pixop->dest_pixel[GREEN_PIX];
	error_pixel[BLUE_PIX]  = blue_pix - pixop->dest_pixel[BLUE_PIX];
	if (pixop->has_alpha)
		error_pixel[ALPHA_PIX] = alpha_pix - pixop->dest_pixel[ALPHA_PIX];

	/* Distribute the error over the neighboring pixels. */

	if (! pixop->ltr)
		dir = -1;

	ngh_error_pixel = get_error_pixel (pixop, FALSE, pixop->column + dir);
	distribute_error (pixop, ngh_error_pixel, error_pixel, 7.0/16);

	ngh_error_pixel = get_error_pixel (pixop, TRUE, pixop->column - dir);
	distribute_error (pixop, ngh_error_pixel, error_pixel, 3.0/16);

	ngh_error_pixel = get_error_pixel (pixop, TRUE, pixop->column);
	distribute_error (pixop, ngh_error_pixel, error_pixel, 5.0/16);

	ngh_error_pixel = get_error_pixel (pixop, TRUE, pixop->column + dir);
	distribute_error (pixop, ngh_error_pixel, error_pixel, 1.0/16);

	if (pixop->line_step == pixop->width - 1)
		pixop->ltr = ! pixop->ltr;
}


static void
dither_release (GthPixbufOp *pixop)
{
	DitherData *data = pixop->data;

	g_free (data->error_row_1);
	g_free (data->error_row_2);
	g_free (pixop->data);
}


GthPixbufOp*
_gdk_pixbuf_dither (GdkPixbuf *src,
		    GdkPixbuf *dest,
		    GthDither  dither_type)
{
	DitherData *data;

	data = g_new0 (DitherData, 1);
	data->dither_type = dither_type;

	return gth_pixbuf_op_new (src, dest,
				  dither_init,
				  dither_step,
				  dither_release,
				  data);	
}


/* -- scale image -- */


typedef struct {
	gboolean percentage;
	gboolean keep_ratio;
	int      width;
	int      height;
} ScaleData;


static void
scale_step (GthPixbufOp *pixop)
{
	ScaleData *data = pixop->data;
	int        w, h;
	int        new_w, new_h;

	w = gdk_pixbuf_get_width (pixop->src);
	h = gdk_pixbuf_get_height (pixop->src);

	if (data->percentage) {
		new_w = w * ((double)data->width / 100.0);
		new_h = h * ((double)data->height / 100.0);
	} else if (data->keep_ratio) {
		new_w = w;
		new_h = h;
		scale_keepping_ratio (&new_w, &new_h,
				      data->width, data->height);
	} else {
		new_w = data->width;
		new_h = data->height;
	}

	if ((new_w > 1) && (new_h > 1))	{
		if(pixop->dest != NULL)
			g_object_unref(pixop->dest);
		pixop->dest = _gdk_pixbuf_scale_simple_safe (pixop->src, new_w, new_h, GDK_INTERP_BILINEAR);
	}
}


static void
scale_free_data (GthPixbufOp *pixop)
{
	ScaleData *data = pixop->data;
	g_free (data);
}


GthPixbufOp*
_gdk_pixbuf_scale (GdkPixbuf *src,
		   GdkPixbuf *dest,
		   gboolean   percentage,
		   gboolean   keep_ratio,
		   int        width,
		   int        height)
{
	ScaleData   *data;
	GthPixbufOp *pixop;

	data = g_new0 (ScaleData, 1);
	data->percentage = percentage;
	data->keep_ratio = keep_ratio;
	data->width = width;
	data->height = height;

	pixop = gth_pixbuf_op_new (src, dest,
				   NULL,
				   scale_step,
				   NULL,
				   data);	
	pixop->free_data_func = scale_free_data;
	gth_pixbuf_op_set_single_step (pixop, TRUE);

	return pixop;
}
