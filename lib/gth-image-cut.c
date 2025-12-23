#include <config.h>
#include <math.h>
#include "lib/gth-image.h"
#include "lib/types.h"

GthImage * gth_image_cut (GthImage *source, guint x, guint y, guint width, guint height, GCancellable *cancellable) {
	int source_stride, source_width, source_height;
	unsigned char *source_line = gth_image_prepare_edit (source, &source_stride, &source_width, &source_height);
	g_return_val_if_fail (x + width <= source_width, NULL);
	g_return_val_if_fail (y + height <= source_height, NULL);

	GthImage *destination = gth_image_new (width, height);
	unsigned char *destination_line = gth_image_get_pixels (destination, NULL);
	int destination_stride = gth_image_get_row_stride (destination);
	source_line += y * source_stride;
	guint x_offset = x * PIXEL_BYTES;
	while (height > 0) {
		unsigned char *source_column = source_line + x_offset;
		memcpy (destination_line, source_column, destination_stride);
		source_line += source_stride;
		destination_line += destination_stride;
		if ((cancellable != NULL) && g_cancellable_is_cancelled (cancellable)) {
			g_object_unref (destination);
			destination = NULL;
			break;
		}
		height--;
	}
	return destination;
}
