#ifndef LOAD_JXL_H
#define LOAD_JXL_H

#include "lib/lib.h"
#include "lib/gth-image.h"

G_BEGIN_DECLS

GthImage* load_jxl (GBytes *buffer, guint requested_size, GCancellable *cancellable, GError **error);
gboolean load_jxl_info (GInputStream *stream, GthImageInfo *image_info, guchar *buffer, gsize size, GCancellable *cancellable);

G_END_DECLS

#endif /* LOAD_JXL_H */
