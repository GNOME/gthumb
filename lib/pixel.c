#include "lib/pixel.h"


guint32 pixel_from_rgba_multiply_alpha (guchar r, guchar g, guchar b, guchar a) {
	int temp;

	temp = (a * r) + 0x80;
	r = ((temp + (temp >> 8)) >> 8);

	temp = (a * g) + 0x80;
	g = ((temp + (temp >> 8)) >> 8);

	temp = (a * b) + 0x80;
	b = ((temp + (temp >> 8)) >> 8);

	return RGBA_TO_PIXEL (r, g, b, a);
}


#define GET_PIXEL_RGBA(pixel, red, green, blue, alpha) \
	G_STMT_START { \
		alpha = pixel[PIXEL_ALPHA]; \
		if (alpha == 0xff) { \
			red = pixel[PIXEL_RED]; \
			green = pixel[PIXEL_GREEN]; \
			blue = pixel[PIXEL_BLUE]; \
		} \
		else { \
			double factor = (double) 0xff / alpha; \
			red = PIXEL_CLAMP (factor * pixel[PIXEL_RED]); \
			green = PIXEL_CLAMP (factor * pixel[PIXEL_GREEN]); \
			blue = PIXEL_CLAMP (factor * pixel[PIXEL_BLUE]); \
		} \
	} G_STMT_END


void pixel_line_to_rgba_big_endian (guchar *dest, guchar *src, guint width) {
	int temp;
	for (guint x = 0; x < width; x++) {
		GET_PIXEL_RGBA (src, dest[0], dest[1], dest[2], dest[3]);
		src += 4;
		dest += 4;
	}
}

#define GET_PIXEL_RGB(pixel, red, green, blue) \
	G_STMT_START { \
		red = pixel[PIXEL_RED]; \
		green = pixel[PIXEL_GREEN]; \
		blue = pixel[PIXEL_BLUE]; \
	} G_STMT_END


void pixel_line_to_rgb_big_endian (guchar *dest, guchar *src, guint width) {
	for (guint x = 0; x < width; x++) {
		GET_PIXEL_RGB (src, dest[0], dest[1], dest[2]);
		src += 4;
		dest += 3;
	}
}

static void non_premultiplied_line_to_pixel_line (int r_idx, int g_idx, int b_idx, int a_idx, guchar *dest, guchar *src, guint width) {
	guchar r, g, b, a;
	guint temp; // used in PIXEL_MULTIPLY_ALPHA
	for (guint x = 0; x < width; x++) {
		a = src[a_idx];
		if (a == 0xFF) {
			*(guint32*) dest = RGBA_TO_PIXEL (src[r_idx], src[g_idx], src[b_idx], 0xFF);
		}
		else if (a == 0) {
			*(guint32*) dest = 0;
		}
		else {
			PIXEL_MULTIPLY_ALPHA (r, src[r_idx], a);
			PIXEL_MULTIPLY_ALPHA (g, src[g_idx], a);
			PIXEL_MULTIPLY_ALPHA (b, src[b_idx], a);
			*(guint32*) dest = RGBA_TO_PIXEL (r, g, b, a);
		}
		src += 4;
		dest += 4;
	}
}

void rgba_big_endian_line_to_pixel (guchar *dest, guchar *src, guint width) {
	non_premultiplied_line_to_pixel_line (
		0, 1, 2, 3,
		dest, src, width);
}

void abgr_line_to_pixel (guchar *dest, guchar *src, guint width) {
	non_premultiplied_line_to_pixel_line (
		ABGR_RED, ABGR_GREEN, ABGR_BLUE, ABGR_ALPHA,
		dest, src, width);
}

void rgb_big_endian_line_to_pixel (guchar *dest, guchar *src, guint width) {
	for (guint x = 0; x < width; x++) {
		*(guint32*) dest = RGBA_TO_PIXEL (src[0], src[1], src[2], 0xFF);
		src += 3;
		dest += 4;
	}
}

#undef GET_PIXEL_RGBA
#undef GET_PIXEL_RGB
