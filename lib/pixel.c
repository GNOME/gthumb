#include "lib/pixel.h"


guint32 pixel_from_rgba_multiply_alpha (guchar r, guchar g, guchar b, guchar a) {
	guint temp;
	PIXEL_MULTIPLY_ALPHA(r, r, a);
	PIXEL_MULTIPLY_ALPHA(g, g, a);
	PIXEL_MULTIPLY_ALPHA(b, b, a);
	return RGBA_TO_PIXEL (r, g, b, a);
}


void pixel_line_to_rgba_big_endian (guchar *dest, guchar *src, guint width) {
	int temp;
	for (guint x = 0; x < width; x++) {
		PIXEL_TO_RGBA (src, dest[0], dest[1], dest[2], dest[3]);
		src += 4;
		dest += 4;
	}
}


#define PIXEL_TO_RGB(pixel, red, green, blue) \
	G_STMT_START { \
		red = pixel[PIXEL_RED]; \
		green = pixel[PIXEL_GREEN]; \
		blue = pixel[PIXEL_BLUE]; \
	} G_STMT_END


void pixel_line_to_rgb_big_endian (guchar *dest, guchar *src, guint width) {
	for (guint x = 0; x < width; x++) {
		PIXEL_TO_RGB (src, dest[0], dest[1], dest[2]);
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

void pixel_over (uint8_t* background, uint8_t* foreground) {
	guint temp;
	uint8_t Falpha_comp = 0xff - foreground[PIXEL_ALPHA];
	PIXEL_OVER(background[PIXEL_RED], foreground[PIXEL_RED], Falpha_comp);
	PIXEL_OVER(background[PIXEL_GREEN], foreground[PIXEL_GREEN], Falpha_comp);
	PIXEL_OVER(background[PIXEL_BLUE], foreground[PIXEL_BLUE], Falpha_comp);
	PIXEL_OVER(background[PIXEL_ALPHA], foreground[PIXEL_ALPHA], Falpha_comp);
}

#undef PIXEL_TO_RGB
