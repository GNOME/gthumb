#ifndef SAVE_AVIF_H
#define SAVE_AVIF_H

#include "lib/lib.h"
#include "lib/gth-image.h"
#include "lib/gth-option.h"

G_BEGIN_DECLS

typedef enum {
	GTH_AVIF_OPTION_QUALITY,
	GTH_AVIF_OPTION_LOSSLESS,
} GthAvifOption;

GBytes* save_avif (GthImage *image, GthOption **options, GCancellable *cancellable, GError **error);

G_END_DECLS

#endif /* SAVE_AVIF_H */
