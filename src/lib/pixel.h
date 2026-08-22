#ifndef LIB_PIXEL_H
#define LIB_PIXEL_H

#include <stdint.h>
#include <glib.h>

#define PIXEL_BYTES 4
#define CLAMP_TEMP(x, min, max) (temp = (x), (guchar) CLAMP (temp, min, max))
#define PIXEL_CLAMP(x) CLAMP_TEMP (x, 0, 255)

#if G_BYTE_ORDER == G_LITTLE_ENDIAN

// ARGB (little endian)
#define PIXEL_ALPHA 3
#define PIXEL_RED 2
#define PIXEL_GREEN 1
#define PIXEL_BLUE 0

// ABGR (little endian)
#define ABGR_ALPHA 3
#define ABGR_BLUE 2
#define ABGR_GREEN 1
#define ABGR_RED 0

#elif G_BYTE_ORDER == G_BIG_ENDIAN

// ARGB (big endian)
#define PIXEL_ALPHA 0
#define PIXEL_RED 1
#define PIXEL_GREEN 2
#define PIXEL_BLUE 3

// ABGR (big endian)
#define ABGR_ALPHA 0
#define ABGR_BLUE 1
#define ABGR_GREEN 2
#define ABGR_RED 3

#endif

#define PACK_RGBA(red, green, blue, alpha) \
	((guint32) (((alpha) << 24) | ((red) << 16) | ((green) << 8) | (blue)))

#define PIXEL_MULTIPLY_ALPHA(result, pixel, alpha) \
	temp = ((alpha) * (pixel)) + 0x80; \
	result = ((temp + (temp >> 8)) >> 8);

#define PIXEL_OVER(bg, fg, alpha) \
	PIXEL_CLAMP (bg * (1 - alpha) + fg * alpha);

// Pegtop's formula https://en.wikipedia.org/wiki/Blend_modes#Soft_Light
#define PIXEL_SOFT_LIGHT(a, b) \
	PIXEL_CLAMP ((((double) a * a) / 255) + (2 * (b * (((double) a * (255 - a)) / 255) / 255)))

extern guchar add_alpha_table[256][256];
extern guchar remove_alpha_table[256][256];

#define PIXEL_ADD_ALPHA(value, alpha) \
	add_alpha_table[value][alpha]

#define PIXEL_REMOVE_ALPHA(value, alpha) \
	remove_alpha_table[value][alpha]

#define RGBA_TO_PIXEL(pixel, red, green, blue, alpha) \
	G_STMT_START { \
		if (alpha == 0xFF) { \
			*(guint32*) pixel = PACK_RGBA (red, green, blue, 0xFF); \
		} \
		else if (alpha == 0) { \
			*(guint32*) pixel = 0; \
		} \
		else { \
			pixel[PIXEL_ALPHA] = (alpha); \
			pixel[PIXEL_RED] = PIXEL_ADD_ALPHA (red, alpha); \
			pixel[PIXEL_GREEN] = PIXEL_ADD_ALPHA (green, alpha); \
			pixel[PIXEL_BLUE] = PIXEL_ADD_ALPHA (blue, alpha); \
		} \
	} G_STMT_END

#define PIXEL_TO_RGBA(pixel, red, green, blue, alpha) \
	G_STMT_START { \
		alpha = pixel[PIXEL_ALPHA]; \
		red = PIXEL_REMOVE_ALPHA (pixel[PIXEL_RED], alpha); \
		green = PIXEL_REMOVE_ALPHA (pixel[PIXEL_GREEN], alpha);	\
		blue = PIXEL_REMOVE_ALPHA (pixel[PIXEL_BLUE], alpha); \
	} G_STMT_END

void pixel_init_tables ();
void pixel_line_to_rgb_big_endian (guchar *dest, const guchar *src, guint width);
void pixel_line_to_rgba_big_endian (guchar *dest, const guchar *src, guint width);
void rgba_big_endian_line_to_pixel (guchar *dest, const guchar *src, guint width);
void abgr_line_to_pixel (guchar *dest, const guchar *src, guint width);
void rgb_big_endian_line_to_pixel (guchar *dest, const guchar *src, guint width);
void pixel_over (uint8_t* background, uint8_t* foreground);

#endif /* LIB_PIXEL_H */
