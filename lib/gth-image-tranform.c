#include <config.h>
#include <math.h>
#include "lib/gth-image.h"
#include "lib/types.h"

GthImage * gth_image_tranform (GthImage *source, GthTransform transform, GCancellable *cancellable) {
	int source_stride, source_width, source_height;
	unsigned char *source_line = gth_image_prepare_edit (source, &source_stride, &source_width, &source_height);

	int destination_width;
	int destination_height;
	int line_start;
	int line_step;
	int pixel_step;
	get_transformation_steps (transform,
		source_width,
		source_height,
		&destination_width,
		&destination_height,
		&line_start,
		&line_step,
		&pixel_step);

	GthImage *destination = gth_image_new ((uint) destination_width, (uint) destination_height);
	unsigned char *destination_line = gth_image_get_pixels (destination, NULL) + line_start;
	while (source_height-- > 0) {
		unsigned char *source_pixel = source_line;
		unsigned char *destination_pixel = destination_line;
		for (uint col = 0; col < source_width; col++) {
			memcpy (destination_pixel, source_pixel, 4);
			source_pixel += 4;
			destination_pixel += pixel_step;
		}
		source_line += source_stride;
		destination_line += line_step;

		if ((cancellable != NULL) && g_cancellable_is_cancelled (cancellable)) {
			g_object_unref (destination);
			destination = NULL;
			break;
		}
	}

	return destination;
}
