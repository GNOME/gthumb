#include <math.h>
#include "lib/gth-image.h"
#include "lib/types.h"


void
gth_image_fill_vertical (GthImage *self, GthImage *pattern, GthFill fill) {
	int row_stride;
	guint width;
	guint height;
	guchar *pixels = gth_image_prepare_edit (self, &row_stride, &width, &height);

	int pattern_stride;
	guint pattern_width;
	guint pattern_height;
	guchar *pattern_pixels = gth_image_prepare_edit (pattern, &pattern_stride, &pattern_width, &pattern_height);

	if (width <= pattern_width * 2)
		return;

	int first_column = (fill == GTH_FILL_END) ? (width - pattern_width) : 0;
	guchar *pattern_row = pattern_pixels;
	guchar *dest_row = pixels;
	int pattern_row_idx = 0;
	int temp;
	int alpha;
	float factor;
	for (int row_idx = 0; row_idx < height; row_idx++) {
		guchar *dest_pixel = dest_row + (first_column * 4);
		guchar *pattern_pixel = pattern_row;
		for (int col = 0; col < pattern_width; col++) {
			alpha = pattern_pixel[PIXEL_ALPHA];
			if (alpha == 0xFF) {
				memcpy (dest_pixel, pattern_pixel, 4);
			}
			else {
				factor = (float) (0xFF - alpha) / 0xFF;
				dest_pixel[PIXEL_RED] = PIXEL_CLAMP (factor * dest_pixel[PIXEL_RED] + pattern_pixel[PIXEL_RED]);
				dest_pixel[PIXEL_GREEN] = PIXEL_CLAMP (factor * dest_pixel[PIXEL_GREEN] + pattern_pixel[PIXEL_GREEN]);
				dest_pixel[PIXEL_BLUE] = PIXEL_CLAMP (factor * dest_pixel[PIXEL_BLUE] + pattern_pixel[PIXEL_BLUE]);
			}
			dest_pixel += 4;
			pattern_pixel += 4;
		}
		dest_row += row_stride;

		// Restart from row 0 if the pattern ends.
		pattern_row_idx += 1;
		if (pattern_row_idx >= pattern_height) {
			pattern_row_idx = 0;
			pattern_row = pattern_pixels;
		}
		else {
			pattern_row += pattern_stride;
		}
	}
}
