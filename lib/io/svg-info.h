#ifndef SVG_INFO_H
#define SVG_INFO_H

#include "lib/lib.h"
#include "image-info.h"

G_BEGIN_DECLS

gboolean load_svg_info (const char *buffer, int buffer_size, GthImageInfo *image_info, GCancellable *cancellable);

G_END_DECLS

#endif /* SVG_INFO_H */
