#ifndef LOAD_TIFF_H
#define LOAD_TIFF_H

#include "lib/lib.h"
#include "lib/gth-image.h"
#include "lib/io/image-info.h"

G_BEGIN_DECLS

GthImage * load_tiff (GBytes *bytes, guint requested_size, GCancellable *cancellable, GError **error);
gboolean load_tiff_info (GInputStream *stream, GthImageInfo *image_info, GCancellable *cancellable);

G_END_DECLS

#endif /* LOAD_TIFF_H */
