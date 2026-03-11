#include <config.h>
#include <string.h>
#include <zlib.h>
#include "zlib-utils.h"

#define BUFFER_SIZE (16 * 1024)

gboolean zlib_decompress (void *zipped_buffer, gsize zipped_size, guint8 **buffer, gsize *size) {
	z_stream strm;
	strm.zalloc = Z_NULL;
	strm.zfree = Z_NULL;
	strm.opaque = Z_NULL;
	strm.avail_in = 0;
	strm.next_in = Z_NULL;
	int ret = inflateInit2 (&strm, 16+15);
	if (ret != Z_OK)
		return FALSE;

	strm.avail_in = zipped_size;
	strm.next_in = zipped_buffer;

	guchar *local_buffer = NULL;
	guchar tmp_buffer[BUFFER_SIZE];
	gsize count = 0;
	do {
		do {
			strm.avail_out = BUFFER_SIZE;
			strm.next_out = tmp_buffer;
			ret = inflate (&strm, Z_NO_FLUSH);

			switch (ret) {
			case Z_NEED_DICT:
			case Z_DATA_ERROR:
			case Z_MEM_ERROR:
				g_free (local_buffer);
				inflateEnd (&strm);
				return FALSE;
			}

			guint n = BUFFER_SIZE - strm.avail_out;
			local_buffer = g_realloc (local_buffer, count + n + 1);
			memcpy (local_buffer + count, tmp_buffer, n);
			count += n;
		}
		while (strm.avail_out == 0);
	}
	while (ret != Z_STREAM_END);

	inflateEnd (&strm);

	*buffer = (guint8*) local_buffer;
	*size = count;

	return ret == Z_STREAM_END;
}
