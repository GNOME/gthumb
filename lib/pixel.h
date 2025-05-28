#ifndef LIB_PIXEL_H
#define LIB_PIXEL_H

#include <glib.h>

#define RGBA_TO_PIXEL(red, green, blue, alpha) \
	(((alpha) << 24) | ((red) << 16) | ((green) << 8) | (blue))

guint32 pixel_from_rgba_multiply_alpha (guchar r, guchar g, guchar b, guchar a);

#endif /* LIB_PIXEL_H */
