#ifndef SAVE_PNG_H
#define SAVE_PNG_H

#include "lib/lib.h"
#include "lib/gth-image.h"
#include "lib/gth-option.h"

G_BEGIN_DECLS

typedef enum {
	GTH_PNG_OPTION_COMPRESSION_LEVEL
} GthPngOption;

GBytes* save_png (GthImage *image, GthOption **options, GCancellable *cancellable, GError **error);

G_END_DECLS

#endif /* SAVE_PNG_H */
