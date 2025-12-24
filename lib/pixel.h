#ifndef LIB_PIXEL_H
#define LIB_PIXEL_H

#include <stdint.h>
#include <glib.h>

#define PIXEL_BYTES 4
#define CLAMP_TEMP(x, min, max) (temp = (x), CLAMP (temp, min, max))
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


#define RGBA_TO_PIXEL(pixel, red, green, blue, alpha) \
	G_STMT_START { \
		if (alpha == 0xFF) { \
			*(guint32*) pixel = PACK_RGBA (red, green, blue, 0xFF); \
		} \
		else if (alpha == 0) { \
			*(guint32*) pixel = 0; \
		} \
		else { \
			PIXEL_MULTIPLY_ALPHA (r, red, alpha); \
			PIXEL_MULTIPLY_ALPHA (g, green, alpha); \
			PIXEL_MULTIPLY_ALPHA (b, blue, alpha); \
			*(guint32*) pixel = PACK_RGBA (r, g, b, alpha); \
		} \
	} G_STMT_END

#define PIXEL_TO_RGBA(pixel, red, green, blue, alpha) \
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

guint32 pixel_from_rgba_multiply_alpha (guchar r, guchar g, guchar b, guchar a);
void pixel_line_to_rgb_big_endian (guchar *dest, guchar *src, guint width);
void pixel_line_to_rgba_big_endian (guchar *dest, guchar *src, guint width);
void rgba_big_endian_line_to_pixel (guchar *dest, guchar *src, guint width);
void abgr_line_to_pixel (guchar *dest, guchar *src, guint width);
void rgb_big_endian_line_to_pixel (guchar *dest, guchar *src, guint width);
void pixel_over (uint8_t* background, uint8_t* foreground);

#endif /* LIB_PIXEL_H */
