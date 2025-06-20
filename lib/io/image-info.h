#ifndef IMAGE_INFO_H
#define IMAGE_INFO_H

#include "lib/lib.h"

G_BEGIN_DECLS

typedef struct {
	int width;
	int height;
	// TODO: color profile?
} GthImageInfo;

gboolean load_image_info (GFile *file, int *width, int *height, GCancellable *cancellable);

G_END_DECLS

#endif /* IMAGE_INFO_H */
