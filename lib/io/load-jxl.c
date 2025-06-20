#include <jxl/decode.h>
#include <jxl/thread_parallel_runner.h>
#include "image-info.h"
#include "load-jxl.h"

GthImage* load_jxl (GBytes *buffer, guint requested_size, GCancellable *cancellable, GError **error) {
	return NULL;
}

gboolean load_jxl_info (GInputStream *stream, GthImageInfo *image_info, guchar *buffer, gsize size, GCancellable *cancellable) {
	JxlSignature sig = JxlSignatureCheck (buffer, size);
	if (sig <= JXL_SIG_INVALID) {
		return FALSE;
	}

	JxlDecoder *dec = JxlDecoderCreate (NULL);
	if (dec == NULL) {
		return FALSE;
	}

	if (JxlDecoderSubscribeEvents (dec, JXL_DEC_BASIC_INFO) != JXL_DEC_SUCCESS) {
		JxlDecoderDestroy (dec);
		return FALSE;
	}

	if (JxlDecoderSetInput (dec, buffer, size) != JXL_DEC_SUCCESS) {
		JxlDecoderDestroy (dec);
		return FALSE;
	}

	gsize buffer_size = size;
	gboolean success = FALSE;
	JxlDecoderStatus status = JXL_DEC_NEED_MORE_INPUT;
	while (status != JXL_DEC_SUCCESS) {
		status = JxlDecoderProcessInput (dec);
		if (status == JXL_DEC_ERROR) {
			break;
		}
		else if (status == JXL_DEC_NEED_MORE_INPUT) {
			JxlDecoderReleaseInput (dec);
			if (!g_input_stream_read_all (G_INPUT_STREAM (stream), buffer, buffer_size, &size, cancellable,	NULL)) {
				break;
			}
			if (JxlDecoderSetInput (dec, buffer, size) != JXL_DEC_SUCCESS) {
				break;
			}
		}
		else if (status == JXL_DEC_BASIC_INFO) {
			JxlBasicInfo info;
			if (JxlDecoderGetBasicInfo (dec, &info) == JXL_DEC_SUCCESS) {
				image_info->width = info.xsize;
				image_info->height = info.ysize;
				success = TRUE;
			}
			break;
		}
	}
	JxlDecoderDestroy (dec);

	return success;
}
