#ifndef LOAD_SVG_H
#define LOAD_SVG_H

#include "lib/lib.h"
#include "lib/gth-image.h"

G_BEGIN_DECLS

GthImage * load_svg (GBytes *bytes, guint requested_size, GCancellable *cancellable, GError **error);

G_END_DECLS

#endif /* LOAD_SVG_H */
