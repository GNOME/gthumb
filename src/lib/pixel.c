#include <math.h>
#include "lib/pixel.h"

static GOnce pixel_init_table = G_ONCE_INIT;
guchar add_alpha_table[256][256];
guchar remove_alpha_table[256][256];


static gpointer _init_tables (gpointer data) {
	// add_alpha_table[v][a] = v * a / 255
	// remove_alpha_table[v][a] = v * 255 / a
	int temp;
	for (int v = 0; v <= 255; v++) {
		add_alpha_table[v][0] = 0;
		add_alpha_table[v][255] = v;
		remove_alpha_table[v][0] = 0;
		remove_alpha_table[v][255] = v;
		for (int a = 1; a < 255; a++) {
			temp = (int) round ((double) (v) * 255.0 / a);
			remove_alpha_table[v][a] = (guchar) CLAMP (temp, 0, 255);

			temp = (int) round ((double) (v) * a / 255.0);
			add_alpha_table[v][a] = (guchar) CLAMP (temp, 0, 255);
		}
	}
	return NULL;
}


void pixel_init_tables () {
	g_once (&pixel_init_table, _init_tables, NULL);
}


void pixel_line_to_rgba_big_endian (guchar *dest, const guchar *src, guint width) {
	guchar r, g, b, a;
	for (guint x = 0; x < width; x++) {
		PIXEL_TO_RGBA (src, r, g, b, a);
		dest[0] = r;
		dest[1] = g;
		dest[2] = b;
		dest[3] = a;
		src += 4;
		dest += 4;
	}
}


void pixel_line_to_rgb_big_endian (guchar *dest, const guchar *src, guint width) {
	guchar r, g, b;
	for (guint x = 0; x < width; x++) {
		r = src[PIXEL_RED];
		g = src[PIXEL_GREEN];
		b = src[PIXEL_BLUE];
		dest[0] = r;
		dest[1] = g;
		dest[2] = b;
		src += 4;
		dest += 3;
	}
}

static void non_premultiplied_line_to_pixel_line (int r_idx, int g_idx, int b_idx, int a_idx, guchar *dest, const guchar *src, guint width) {
	guchar r, g, b, a;
	for (guint x = 0; x < width; x++) {
		r = src[r_idx];
		g = src[g_idx];
		b = src[b_idx];
		a = src[a_idx];
		RGBA_TO_PIXEL (dest, r, g, b, a);
		src += 4;
		dest += 4;
	}
}

void rgba_big_endian_line_to_pixel (guchar *dest, const guchar *src, guint width) {
	non_premultiplied_line_to_pixel_line (
		0, 1, 2, 3,
		dest, src, width);
}

void abgr_line_to_pixel (guchar *dest, const guchar *src, guint width) {
	non_premultiplied_line_to_pixel_line (
		ABGR_RED, ABGR_GREEN, ABGR_BLUE, ABGR_ALPHA,
		dest, src, width);
}

void rgb_big_endian_line_to_pixel (guchar *dest, const guchar *src, guint width) {
	for (guint x = 0; x < width; x++) {
		*(guint32*) dest = PACK_RGBA (src[0], src[1], src[2], 0xFF);
		src += 3;
		dest += 4;
	}
}
