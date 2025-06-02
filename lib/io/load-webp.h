#ifndef LOAD_WEBP_H
#define LOAD_WEBP_H

#include "lib/lib.h"
#include "lib/gth-image.h"

G_BEGIN_DECLS

GthImage* load_webp (GBytes *buffer, guint requested_size, GCancellable *cancellable, GError **error);

G_END_DECLS

#endif /* LOAD_WEBP_H */
