#ifndef SAVE_WEBP_H
#define SAVE_WEBP_H

#include "lib/lib.h"
#include "lib/gth-image.h"
#include "lib/gth-option.h"

G_BEGIN_DECLS

typedef enum {
	GTH_WEBP_OPTION_QUALITY,
	GTH_WEBP_OPTION_METHOD,
	GTH_WEBP_OPTION_LOSSLESS,
} GthWebpOption;

GBytes* save_webp (GthImage *image, GthOption **options, GCancellable *cancellable, GError **error);

G_END_DECLS

#endif /* SAVE_WEBP_H */
