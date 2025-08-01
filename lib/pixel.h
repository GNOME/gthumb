#ifndef LIB_PIXEL_H
#define LIB_PIXEL_H

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

#define RGBA_TO_PIXEL(red, green, blue, alpha) \
	((guint32) (((alpha) << 24) | ((red) << 16) | ((green) << 8) | (blue)))

#define PIXEL_MULTIPLY_ALPHA(r, p, a) \
	temp = ((a) * (p)) + 0x80; \
	r = ((temp + (temp >> 8)) >> 8);

guint32 pixel_from_rgba_multiply_alpha (guchar r, guchar g, guchar b, guchar a);
void pixel_line_to_rgb_big_endian (guchar *dest, guchar *src, guint width);
void pixel_line_to_rgba_big_endian (guchar *dest, guchar *src, guint width);
void rgba_big_endian_line_to_pixel (guchar *dest, guchar *src, guint width);
void abgr_line_to_pixel (guchar *dest, guchar *src, guint width);
void rgb_big_endian_line_to_pixel (guchar *dest, guchar *src, guint width);

#endif /* LIB_PIXEL_H */
