#ifndef LOAD_RAW_H
#define LOAD_RAW_H

#include "lib/lib.h"
#include "lib/gth-image.h"
#include "lib/io/image-info.h"

G_BEGIN_DECLS

GthImage* load_raw (GBytes *buffer, guint requested_size, GCancellable *cancellable, GError **error);
gboolean load_raw_info (GInputStream *stream, GthImageInfo *image_info, GCancellable *cancellable);

G_END_DECLS

#endif /* LOAD_RAW_H */
