#ifndef ZLIB_UTILS_H
#define ZLIB_UTILS_H

#include <glib.h>

G_BEGIN_DECLS

gboolean zlib_decompress (void *zipped_buffer, gsize zipped_size, guint8 **buffer, gsize *size);

G_END_DECLS

#endif /* ZLIB_UTILS_H */
