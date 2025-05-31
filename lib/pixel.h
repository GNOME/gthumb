#ifndef LIB_PIXEL_H
#define LIB_PIXEL_H

#include <glib.h>

#define CLAMP_TEMP(x, min, max) (temp = (x), CLAMP (temp, min, max))
#define PIXEL_CLAMP(x) CLAMP_TEMP (x, 0, 255)

#if G_BYTE_ORDER == G_LITTLE_ENDIAN /* BGRA */

#define PIXEL_RED   2
#define PIXEL_GREEN 1
#define PIXEL_BLUE  0
#define PIXEL_ALPHA 3

#elif G_BYTE_ORDER == G_BIG_ENDIAN /* ARGB */

#define PIXEL_RED   1
#define PIXEL_GREEN 2
#define PIXEL_BLUE  3
#define PIXEL_ALPHA 0

#else /* PDP endianness: RABG */

#define PIXEL_RED   0
#define PIXEL_GREEN 3
#define PIXEL_BLUE  2
#define PIXEL_ALPHA 1

#endif

#define RGBA_TO_PIXEL(red, green, blue, alpha) \
	(((alpha) << 24) | ((red) << 16) | ((green) << 8) | (blue))

guint32 pixel_from_rgba_multiply_alpha (guchar r, guchar g, guchar b, guchar a);

#endif /* LIB_PIXEL_H */
