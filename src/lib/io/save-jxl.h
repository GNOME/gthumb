#ifndef SAVE_JXL_H
#define SAVE_JXL_H

#include "lib/lib.h"
#include "lib/gth-image.h"
#include "lib/gth-option.h"

G_BEGIN_DECLS

typedef enum {
	GTH_JXL_OPTION_EFFORT, // 3 -> 9 (default: 7)
	GTH_JXL_OPTION_DISTANCE, // 0.5 -> 3.0 (default: 1.0)
	GTH_JXL_OPTION_LOSSLESS,
} GthJxlOption;

GBytes* save_jxl (GthImage *image, GthOption **options, GCancellable *cancellable, GError **error);

G_END_DECLS

#endif /* SAVE_JXL_H */
