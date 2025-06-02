#ifndef JMEMORYSRC_H
#define JMEMORYSRC_H

#include <jpeglib.h>
#include <glib.h>

void _jpeg_memory_src (j_decompress_ptr cinfo,
		       const void *in_buffer,
		       gsize in_buffer_size);

#endif /* JMEMORYSRC_H */
