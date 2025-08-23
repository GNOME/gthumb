#ifndef SAVE_TIFF_H
#define SAVE_TIFF_H

#include "lib/lib.h"
#include "lib/gth-image.h"
#include "lib/gth-option.h"

G_BEGIN_DECLS

typedef enum {
	GTH_TIFF_OPTION_DEFAULT_EXT,
	GTH_TIFF_OPTION_COMPRESSION,
	GTH_TIFF_OPTION_HRESOLUTION,
	GTH_TIFF_OPTION_VRESOLUTION,
} GthTiffOption;

typedef enum {
	GTH_TIFF_COMPRESSION_NONE,
	GTH_TIFF_COMPRESSION_DEFLATE,
	GTH_TIFF_COMPRESSION_JPEG,
} GthTiffCompression;

GBytes* save_tiff (GthImage *image, GthOption **options, GCancellable *cancellable, GError **error);

G_END_DECLS

#endif /* SAVE_TIFF_H */
