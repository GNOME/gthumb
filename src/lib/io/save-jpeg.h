#ifndef SAVE_JPEG_H
#define SAVE_JPEG_H

#include "lib/lib.h"
#include "lib/gth-image.h"
#include "lib/gth-option.h"

G_BEGIN_DECLS

typedef enum {
	GTH_JPEG_OPTION_QUALITY,
	GTH_JPEG_OPTION_SMOOTHING,
	GTH_JPEG_OPTION_OPTIMIZE,
	GTH_JPEG_OPTION_PROGRESSIVE,
} GthJpegOption;

GBytes* save_jpeg (GthImage *image, GthOption **options, GCancellable *cancellable, GError **error);

G_END_DECLS

#endif /* SAVE_JPEG_H */
