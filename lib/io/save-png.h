#ifndef SAVE_PNG_H
#define SAVE_PNG_H

#include "lib/lib.h"
#include "lib/gth-image.h"

G_BEGIN_DECLS

GBytes* save_png (GthImage *image, GCancellable *cancellable, GError **error);

G_END_DECLS

#endif /* SAVE_PNG_H */
