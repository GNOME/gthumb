#ifndef JPEG_INFO_H
#define JPEG_INFO_H

#include "lib/lib.h"

#define JPEG_SEGMENT_MAX_SIZE (64 * 1024)

typedef enum /*< skip >*/ {
	_JPEG_INFO_NONE = 0,
	_JPEG_INFO_IMAGE_SIZE = 1 << 0,
	_JPEG_INFO_ICC_PROFILE = 1 << 1,
	_JPEG_INFO_EXIF_ORIENTATION = 1 << 2,
	_JPEG_INFO_EXIF_COLORIMETRY = 1 << 3,
	_JPEG_INFO_EXIF_COLOR_SPACE = 1 << 4,
} JpegInfoFlags;

typedef struct {
	JpegInfoFlags	valid;
	int		width;
	int		height;
	GthTransform	orientation;
	gpointer	icc_data;
	gsize		icc_data_size;
	GthColorSpace	color_space;
} JpegInfoData;

void		_jpeg_info_data_init		(JpegInfoData	 *data);
void		_jpeg_info_data_dispose		(JpegInfoData	 *data);
gboolean	_jpeg_info_get_from_stream	(GInputStream	 *stream,
						 JpegInfoFlags	  flags,
						 JpegInfoData	 *data,
						 GCancellable	 *cancellable,
						 GError		**error);
gboolean	_jpeg_info_get_from_buffer	(guchar		 *in_buffer,
						 gsize		  in_buffer_size,
						 JpegInfoFlags	  flags,
						 JpegInfoData	 *data);
gboolean	_read_exif_data_tags		(guchar		 *exif_data,
						 gsize		  exif_data_size,
						 JpegInfoFlags	  flags,
						 JpegInfoData	 *data);

#endif /* JPEG_INFO_H */
