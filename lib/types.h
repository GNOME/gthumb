#ifndef LIB_TYPES_H
#define LIB_TYPES_H

G_BEGIN_DECLS

// The GthTransform numeric values range from 1 to 8, corresponding to
// the valid range of Exif orientation tags.  The name associated with each
// numeric valid describes the data transformation required that will allow
// the orientation value to be reset to "1" without changing the displayed
// image.
// GthTransform and ExifShort values are interchangeably in a number of
// places.  The main difference is that ExifShort can have a value of zero,
// corresponding to an error or an absence of an Exif orientation tag.
// See bug 361913 for additional details.
typedef enum {
	GTH_TRANSFORM_NONE = 1,         // no transformation
	GTH_TRANSFORM_FLIP_H,           // horizontal flip
	GTH_TRANSFORM_ROTATE_180,       // 180-degree rotation
	GTH_TRANSFORM_FLIP_V,           // vertical flip
	GTH_TRANSFORM_TRANSPOSE,        // transpose across UL-to-LR axis (= rotate_90 + flip_h)
	GTH_TRANSFORM_ROTATE_90,        // 90-degree clockwise rotation
	GTH_TRANSFORM_TRANSVERSE,       // transpose across UR-to-LL axis (= rotate_90 + flip_v)
	GTH_TRANSFORM_ROTATE_270        // 270-degree clockwise
} GthTransform;

typedef enum {
	GTH_COLOR_SPACE_UNKNOWN,
	GTH_COLOR_SPACE_SRGB,
	GTH_COLOR_SPACE_ADOBERGB,
	GTH_COLOR_SPACE_UNCALIBRATED,
} GthColorSpace;

typedef enum {
	GTH_FORMAT_BGRA,
	GTH_FORMAT_ARGB,
} GthFormat;

#if G_BYTE_ORDER == G_LITTLE_ENDIAN
#define GTH_PIXEL_FORMAT GTH_FORMAT_BGRA
#elif G_BYTE_ORDER == G_BIG_ENDIAN
#define GTH_PIXEL_FORMAT GTH_FORMAT_ARGB
#endif

typedef enum {
	GTH_SCALE_FILTER_POINT,
	GTH_SCALE_FILTER_BOX,
	GTH_SCALE_FILTER_TRIANGLE,
	GTH_SCALE_FILTER_CUBIC,
	GTH_SCALE_FILTER_LANCZOS2,
	GTH_SCALE_FILTER_LANCZOS3,
	GTH_SCALE_FILTER_MITCHELL_NETRAVALI,
	N_SCALE_FILTERS,
} GthScaleFilter;

#define GTH_SCALE_FILTER_FAST GTH_SCALE_FILTER_POINT
#define GTH_SCALE_FILTER_BEST GTH_SCALE_FILTER_LANCZOS3
#define GTH_SCALE_FILTER_GOOD GTH_SCALE_FILTER_TRIANGLE

#define GTH_MAX_IMAGE_SIZE 32767

typedef enum {
	GTH_RESIZE_DEFAULT = 0, // KEEP_RATIO + NO UPSCALE
	GTH_RESIZE_IGNORE_RATIO = 1 << 1,
	GTH_RESIZE_UPSCALE = 1 << 2,
} GthResizeFlags;

typedef enum {
	GTH_FILL_START,
	GTH_FILL_END,
} GthFill;

typedef enum {
	GTH_TIME_OP_ADD,
	GTH_TIME_OP_SET,
} GthTimeOp;

G_END_DECLS

#endif /* LIB_TYPES_H */
