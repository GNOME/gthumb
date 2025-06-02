#ifndef LOAD_JPEG_H
#define LOAD_JPEG_H

#include "lib/lib.h"
#include "lib/gth-image.h"

G_BEGIN_DECLS

GthImage * load_jpeg (GBytes *bytes, guint requested_size, GCancellable *cancellable, GError **error);

G_END_DECLS

#endif /* LOAD_JPEG_H */
