#include <math.h>
#include "lib/util.h"
#include "lib/pixel.h"


gboolean scale_keeping_ratio_min (
	guint *width,
	guint *height,
	guint min_width,
	guint min_height,
	guint max_width,
	guint max_height,
	gboolean allow_upscaling)
{
	if ((*width < max_width) && (*height < max_height) && !allow_upscaling)
		return FALSE;

	if (((*width < min_width) || (*height < min_height)) && !allow_upscaling)
		return FALSE;

	double w = *width;
	double h = *height;
	double min_w = min_width;
	double min_h = min_height;
	double max_w = max_width;
	double max_h = max_height;
	double factor = MAX (MIN (max_w / w, max_h / h), MAX (min_w / w, min_h / h));
	guint new_width  = (guint) MAX (floor (w * factor + 0.50), 1);
	guint new_height = (guint) MAX (floor (h * factor + 0.50), 1);
	gboolean modified = (new_width != *width) || (new_height != *height);

	*width = new_width;
	*height = new_height;

	return modified;
}


gboolean scale_keeping_ratio (
	guint *width,
	guint *height,
	guint max_width,
	guint max_height,
	gboolean allow_upscaling)
{
	return scale_keeping_ratio_min (
		width,
		height,
		0,
		0,
		max_width,
		max_height,
		allow_upscaling);
}


gboolean scale_if_larger (
	guint *width,
	guint *height,
	guint size)
{
	return scale_keeping_ratio (width, height, size, size, FALSE);
}


void get_transformation_steps (
	GthTransform transform,
	int width,
	int height,
	int *destination_width_p,
	int *destination_height_p,
	int *line_start_p,
	int *line_step_p,
	int *pixel_step_p)
{
	int destination_stride;
	int destination_width = 0;
	int destination_height = 0;
	int line_start = 0;
	int line_step = 0;
	int pixel_step = 0;

	switch (transform) {
	case GTH_TRANSFORM_NONE:
	default:
		destination_width = width;
		destination_height = height;
		destination_stride = destination_width * PIXEL_BYTES;
		line_start = 0;
		line_step = destination_stride;
		pixel_step = PIXEL_BYTES;
		break;

	case GTH_TRANSFORM_FLIP_H:
		destination_width = width;
		destination_height = height;
		destination_stride = destination_width * PIXEL_BYTES;
		line_start = (destination_width - 1) * PIXEL_BYTES;
		line_step = destination_stride;
		pixel_step = - PIXEL_BYTES;
		break;

	case GTH_TRANSFORM_ROTATE_180:
		destination_width = width;
		destination_height = height;
		destination_stride = destination_width * PIXEL_BYTES;
		line_start = ((destination_height - 1) * destination_stride) + ((destination_width - 1) * PIXEL_BYTES);
		line_step = - destination_stride;
		pixel_step = - PIXEL_BYTES;
		break;

	case GTH_TRANSFORM_FLIP_V:
		destination_width = width;
		destination_height = height;
		destination_stride = destination_width * PIXEL_BYTES;
		line_start = (destination_height - 1) * destination_stride;
		line_step = -destination_stride;
		pixel_step = PIXEL_BYTES;
		break;

	case GTH_TRANSFORM_TRANSPOSE:
		destination_width = height;
		destination_height = width;
		destination_stride = destination_width * PIXEL_BYTES;
		line_start = 0;
		line_step = PIXEL_BYTES;
		pixel_step = destination_stride;
		break;

	case GTH_TRANSFORM_ROTATE_90:
		destination_width = height;
		destination_height = width;
		destination_stride = destination_width * PIXEL_BYTES;
		line_start = (destination_width - 1) * PIXEL_BYTES;
		line_step = - PIXEL_BYTES;
		pixel_step = destination_stride;
		break;

	case GTH_TRANSFORM_TRANSVERSE:
		destination_width = height;
		destination_height = width;
		destination_stride = destination_width * PIXEL_BYTES;
		line_start = ((destination_height - 1) * destination_stride) + ((destination_width - 1) * PIXEL_BYTES);
		line_step = - PIXEL_BYTES;
		pixel_step = - destination_stride;
		break;

	case GTH_TRANSFORM_ROTATE_270:
		destination_width = height;
		destination_height = width;
		destination_stride = destination_width * PIXEL_BYTES;
		line_start = (destination_height - 1) * destination_stride;
		line_step = PIXEL_BYTES;
		pixel_step = - destination_stride;
		break;
	}

	if (destination_width_p != NULL)
		*destination_width_p = destination_width;
	if (destination_height_p != NULL)
		*destination_height_p = destination_height;
	if (line_start_p != NULL)
		*line_start_p = line_start;
	if (line_step_p != NULL)
		*line_step_p = line_step;
	if (pixel_step_p != NULL)
		*pixel_step_p = pixel_step;
}
