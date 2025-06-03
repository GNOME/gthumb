#include <config.h>
#include <math.h>
#include "lib/gth-image.h"
#include "lib/types.h"

#ifdef HAVE_VECTOR_OPERATIONS
typedef double v4r __attribute__ ((vector_size(sizeof(double) * 4)));
#endif /* HAVE_VECTOR_OPERATIONS */


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
#ifdef HAVE_VECTOR_OPERATIONS
	v4r v_dest, v_src;
#endif

	for (int row_idx = 0; row_idx < height; row_idx++) {
		guchar *dest_pixel = dest_row + (first_column * PIXEL_BYTES);
		guchar *pattern_pixel = pattern_row;
		for (int col = 0; col < pattern_width; col++) {
			alpha = pattern_pixel[PIXEL_ALPHA];
			if (alpha == 0xFF) {
				memcpy (dest_pixel, pattern_pixel, PIXEL_BYTES);
			}
			else {
				factor = (float) (0xFF - alpha) / 0xFF;

#ifdef HAVE_VECTOR_OPERATIONS

				v_dest = (v4r) { dest_pixel[0], dest_pixel[1], dest_pixel[2], 0.0 };
				v_src = (v4r) { pattern_pixel[0], pattern_pixel[1], pattern_pixel[2], 0.0 };
				v_dest = v_dest * factor + v_src;
				dest_pixel[PIXEL_RED] = CLAMP (v_dest[PIXEL_RED], 0, 255);
				dest_pixel[PIXEL_GREEN] = CLAMP (v_dest[PIXEL_GREEN], 0, 255);
				dest_pixel[PIXEL_BLUE] = CLAMP (v_dest[PIXEL_BLUE], 0, 255);

#else /* !HAVE_VECTOR_OPERATIONS */

				dest_pixel[PIXEL_RED] = PIXEL_CLAMP (factor * dest_pixel[PIXEL_RED] + pattern_pixel[PIXEL_RED]);
				dest_pixel[PIXEL_GREEN] = PIXEL_CLAMP (factor * dest_pixel[PIXEL_GREEN] + pattern_pixel[PIXEL_GREEN]);
				dest_pixel[PIXEL_BLUE] = PIXEL_CLAMP (factor * dest_pixel[PIXEL_BLUE] + pattern_pixel[PIXEL_BLUE]);

#endif
			}
			dest_pixel += PIXEL_BYTES;
			pattern_pixel += PIXEL_BYTES;
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
