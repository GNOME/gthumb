#include "lib/pixel.h"

guint32
pixel_from_rgba_multiply_alpha (guchar r, guchar g, guchar b, guchar a)
{
	int temp;

	temp = (a * r) + 0x80;
	r = ((temp + (temp >> 8)) >> 8);

	temp = (a * g) + 0x80;
	g = ((temp + (temp >> 8)) >> 8);

	temp = (a * b) + 0x80;
	b = ((temp + (temp >> 8)) >> 8);

	return RGBA_TO_PIXEL (r, g, b, a);
}
