#ifndef ZLIB_UTILS_H
#define ZLIB_UTILS_H

#include <glib.h>

G_BEGIN_DECLS

gboolean zlib_decompress_buffer (void *zipped_buffer, gsize zipped_size, void **buffer, gsize *size);

G_END_DECLS

#endif /* ZLIB_UTILS_H */
