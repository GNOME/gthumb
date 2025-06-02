#ifndef LIB_UTIL_H
#define LIB_UTIL_H

#include <glib.h>
#include "lib/types.h"

gboolean scale_keeping_ratio_min (
	guint *width,
	guint *height,
	guint min_width,
	guint min_height,
	guint max_width,
	guint max_height,
	gboolean allow_upscaling);

gboolean scale_keeping_ratio (
	guint *width,
	guint *height,
	guint max_width,
	guint max_height,
	gboolean allow_upscaling);

gboolean scale_if_larger (
	guint *width,
	guint *height,
	guint size);

void get_transformation_steps (
	GthTransform transform,
	int width,
	int height,
	int *destination_width_p,
	int *destination_height_p,
	int *line_start_p,
	int *line_step_p,
	int *pixel_step_p);

#endif /* LIB_UTIL_H */
