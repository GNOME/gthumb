#ifndef LIB_UTIL_H
#define LIB_UTIL_H

#include <glib.h>

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

#endif /* LIB_UTIL_H */
