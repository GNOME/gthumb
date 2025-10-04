#ifndef LOAD_GIF_H
#define LOAD_GIF_H

#include "lib/lib.h"
#include "lib/gth-image.h"
#include "lib/io/image-info.h"

G_BEGIN_DECLS

GthImage * load_gif (GBytes *bytes, guint requested_size, GCancellable *cancellable, GError **error);
gboolean load_gif_info (const char *buffer, int buffer_size, GthImageInfo *image_info, GCancellable *cancellable);

G_END_DECLS

#endif /* LOAD_GIF_H */
