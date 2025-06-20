#ifndef LOAD_HEIF_H
#define LOAD_HEIF_H

#include "lib/lib.h"
#include "lib/gth-image.h"

G_BEGIN_DECLS

GthImage* load_heif (GBytes *buffer, guint requested_size, GCancellable *cancellable, GError **error);
gboolean load_heif_info (GInputStream *stream, GthImageInfo *image_info, guchar *buffer, gsize size, GCancellable *cancellable);

G_END_DECLS

#endif /* LOAD_HEIF_H */
