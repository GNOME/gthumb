#include <math.h>
#include "lib/util.h"

gboolean
scale_keeping_ratio_min (guint *width,
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


gboolean
scale_keeping_ratio (guint *width,
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


gboolean
scale_if_larger (guint *width,
		 guint *height,
		 guint size)
{
	return scale_keeping_ratio (width, height, size, size, FALSE);
}
