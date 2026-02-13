#include <config.h>
#include <png.h>
#include <lcms2.h>
#include "lib/jpeg/jpeg-info.h" // For reading the color profile in EXIF data
#include "lib/gth-icc-profile.h"
#include "load-png.h"

#define PNG_SETJMP(ptr) setjmp(png_jmpbuf(ptr))
#define PNG_IDAT "IDAT"
#define PNG_IEND "IEND"
#define APNG_ACTL "acTL"
#define APNG_FCTL "fcTL"
#define APNG_FDAT "fdAT"
#define APNG_CHUNKS APNG_ACTL "\0" APNG_FCTL "\0" APNG_FDAT "\0"
#define APNG_DISPOSE_OP_NONE 0
#define APNG_DISPOSE_OP_BACKGROUND 1
#define APNG_DISPOSE_OP_PREVIOUS 2
#define APNG_BLEND_OP_SOURCE 0
#define APNG_BLEND_OP_OVER 1

typedef struct {
	guint sequence_number;
	GBytes *bytes;
} DataChunk;

typedef struct {
	guint sequence_number;
	guint width;
	guint height;
	guint x_offset;
	guint y_offset;
	guint delay; // milliseconds
	uint8_t dispose_op;
	uint8_t blend_op;
	GPtrArray *data_chunks;
} Frame;

typedef struct {
	guint n_frames;
	guint n_loops;
	GPtrArray *frames;
	guint next_idx;
	Frame *last_frame;
} Animation;

typedef struct {
	GBytes *bytes;
	GCancellable *cancellable;
	size_t bytes_offset;
	GError **error;
	png_struct *png_ptr;
	png_info *png_info_ptr;
	GthImage *image;
	Animation animation;
} LoaderData;

static void init_animation (Animation *animation) {
	animation->n_frames = 0;
	animation->n_loops = 0;
	animation->frames = NULL;
	animation->last_frame = NULL;
}

static DataChunk * data_chunk_new (guint n, GBytes *bytes) {
	DataChunk *chunk = g_new0 (DataChunk, 1);
	chunk->sequence_number = n;
	chunk->bytes = bytes;
	return chunk;
}

static void data_chunk_free (DataChunk *chunk) {
	g_bytes_unref (chunk->bytes);
	g_free (chunk);
}

static Frame* frame_new () {
	Frame *frame = g_new0 (Frame, 1);
	frame->sequence_number = 0;
	frame->width = 0;
	frame->height = 0;
	frame->x_offset = 0;
	frame->y_offset = 0;
	frame->delay = 0;
	frame->dispose_op = APNG_DISPOSE_OP_NONE;
	frame->blend_op = APNG_BLEND_OP_SOURCE;
	frame->data_chunks = g_ptr_array_new_with_free_func ((GDestroyNotify) data_chunk_free);
	return frame;
}

static void frame_free (Frame *frame) {
	g_ptr_array_unref (frame->data_chunks);
	g_free (frame);
}

static void loader_data_destroy (LoaderData *loader_data) {
	png_destroy_read_struct (&loader_data->png_ptr, &loader_data->png_info_ptr, NULL);
	g_bytes_unref (loader_data->bytes);
	if (loader_data->image != NULL) {
		g_object_unref (loader_data->image);
	}
	if (loader_data->animation.frames != NULL) {
		g_ptr_array_unref (loader_data->animation.frames);
	}
}

static void error_func (png_structp png_ptr, png_const_charp message) {
	GError ***error_p = png_get_error_ptr (png_ptr);
	GError **error = *error_p;

	if ((error != NULL) && (*error == NULL)) {
		g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_INVALID_DATA, message);
	}
	longjmp (png_jmpbuf(png_ptr), 1);
}

static void warning_func (png_structp png_ptr, png_const_charp message) {
	// Ignored
}

static void read_buffer_func (png_structp png_ptr, png_bytep buffer, png_size_t size) {
	LoaderData *loader_data = png_get_io_ptr (png_ptr);

	if ((loader_data->cancellable != NULL)
		&& g_cancellable_is_cancelled (loader_data->cancellable))
	{
		g_set_error_literal (loader_data->error, G_IO_ERROR, G_IO_ERROR_CANCELLED, "Cancelled");
		png_error (png_ptr, "Cancelled");
		return;
	}

	gsize max_size;
	gconstpointer bytes = g_bytes_get_data (loader_data->bytes, &max_size);
	if (loader_data->bytes_offset + size > max_size) {
		png_error (png_ptr, "Wrong size");
		return;
	}
	memcpy (buffer, bytes + loader_data->bytes_offset, size);
	loader_data->bytes_offset += size;
}

static int read_unknown_chunk (png_structp png_ptr, png_unknown_chunkp chunk) {
	//g_print ("> UNKNOWN CHUNK: '%s'\n", chunk->name);
	LoaderData *loader_data = png_get_user_chunk_ptr (png_ptr);
	if (g_strcmp0 ((char*) chunk->name, APNG_ACTL) == 0) {
		// Animation Control
		loader_data->animation.n_frames = png_get_uint_31 (png_ptr, chunk->data);
		loader_data->animation.n_loops = png_get_uint_31 (png_ptr, chunk->data + 4);
		//g_print ("> FRAMES: %u\n", loader_data->animation.n_frames);
		//g_print ("  LOOPS: %u\n", loader_data->animation.n_loops);
	}
	else if (g_strcmp0 ((char*) chunk->name, APNG_FCTL) == 0) {
		// Frame Control
		Frame *frame = frame_new ();
		frame->sequence_number = png_get_uint_31 (png_ptr, chunk->data);
		frame->width = png_get_uint_31 (png_ptr, chunk->data + 4);
		frame->height = png_get_uint_31 (png_ptr, chunk->data + 8);
		frame->x_offset = png_get_uint_31 (png_ptr, chunk->data + 12);
		frame->y_offset = png_get_uint_31 (png_ptr, chunk->data + 16);
		guint delay_num = png_get_uint_16 (chunk->data + 20);
		guint delay_den = png_get_uint_16 (chunk->data + 22);
		if (delay_den == 0) {
			delay_den = 100;
		}
		frame->delay = (guint) (((double) delay_num / delay_den) * 1000);
		frame->dispose_op = chunk->data[24];
		frame->blend_op = chunk->data[25];

		if (loader_data->animation.frames == NULL) {
			loader_data->animation.frames = g_ptr_array_new_with_free_func ((GDestroyNotify) frame_free);
		}
		g_ptr_array_add (loader_data->animation.frames, frame);
		loader_data->animation.last_frame = frame;

		//g_print ("> SEQ NUM: %u\n", frame->sequence_number);
		//g_print ("  WIDTH: %u\n", frame->width);
		//g_print ("  HEIGHT: %u\n", frame->height);
		//g_print ("  OFS LEFT: %u\n", frame->x_offset);
		//g_print ("  OFS TOP: %u\n", frame->y_offset);
		//g_print ("  DELAY MSEC: %u\n", frame->delay);
		//g_print ("  DISPOSE OP: %u\n", frame->dispose_op);
		//g_print ("  BLEND OP: %u\n", frame->blend_op);
	}
	else if (g_strcmp0 ((char*) chunk->name, APNG_FDAT) == 0) {
		// Frame Image Data
		guint sequence_number = png_get_uint_31 (png_ptr, chunk->data);
		Frame *frame = loader_data->animation.last_frame;
		//g_print ("  SEQ NUM: %u (frame: %p) (size: %d)\n", sequence_number, frame, chunk->size - 4);
		if ((frame != NULL) && (chunk->size > 4)) {
			GBytes *bytes = g_bytes_new (chunk->data + 4, chunk->size - 4);
			g_ptr_array_add (frame->data_chunks, data_chunk_new (sequence_number, bytes));
		}
	}
	return 1;
}

static void transform_to_argb32_format_func (png_structp png, png_row_infop row_info, png_bytep data) {
	rgba_big_endian_line_to_pixel (data, data, row_info->rowbytes / 4);
}

static GthImage* _load_png (GBytes *bytes, gboolean animation_frame, GCancellable *cancellable, GError **error);

// CRC implementation: https://www.w3.org/TR/png-3/#D-CRCAppendix

// Table of CRCs of all 8-bit messages.
unsigned long crc_table[256];

// Flag: has the table been computed? Initially false.
int crc_table_computed = 0;

// Make the table for a fast CRC
void make_crc_table (void) {
	unsigned long c;
	int n, k;
	for (n = 0; n < 256; n++) {
		c = (unsigned long) n;
		for (k = 0; k < 8; k++) {
			if (c & 1)
				c = 0xedb88320L ^ (c >> 1);
			else
				c = c >> 1;
		}
		crc_table[n] = c;
	}
	crc_table_computed = 1;
}

// Update a running CRC with the bytes buf[0..len-1]--the CRC
// should be initialized to all 1's, and the transmitted value
// is the 1's complement of the final running CRC (see the
// crc() routine below).
unsigned long update_crc(unsigned long crc, unsigned char *buf, int len) {
	if (!crc_table_computed) {
		make_crc_table ();
	}
	unsigned long c = crc;
	for (int n = 0; n < len; n++) {
		c = crc_table[(c ^ buf[n]) & 0xff] ^ (c >> 8);
	}
	return c;
}

// Return the CRC of the bytes buf[0..len-1].
unsigned long crc(unsigned char *buf, int len) {
	return update_crc (0xffffffffL, buf, len) ^ 0xffffffffL;
}

/*static void inspect_content (GBytes *bytes) {
	gsize size;
	const uint8_t *data = g_bytes_get_data (bytes, &size);
	gsize offset = 8;
	while (offset < size) {
		const uint8_t *chunk_data = data + offset;

		// Chunk data length
		uint32_t length = png_get_uint_32 (chunk_data);

		// Chunk type
		uint8_t chunk_type[5] = { 0, };
		memcpy (chunk_type, chunk_data + 4, 4);

		g_print ("  CHUNK: '%s' (DATA SIZE: %u)\n", chunk_type, length);

		// Next chunk
		offset += 12 + length;
	}
}*/

static bool _compose_animation (LoaderData *loader_data, GCancellable *cancellable, GError **error) {
	Animation *animation = &loader_data->animation;

	//g_print (">> ANI FRAMES: %u <=> %d\n",
	//	animation->n_frames,
	//	animation->frames->len);

	if (animation->frames->len != animation->n_frames) {
		g_set_error (error, G_IO_ERROR,	G_IO_ERROR_FAILED,
			"Wrong number of frames, %d, it should be %u.",
			animation->frames->len, animation->n_frames);
		return false;
	}

	// Build the frame metadata with all the animation chunks
	// before the first IDAT
	gsize size;
	const uint8_t *animation_data = g_bytes_get_data (loader_data->bytes, &size);

	// Find the first IDAT block
	gsize offset = 8;
	while (offset < size) {
		const uint8_t *chunk_data = animation_data + offset;

		// Chunk data length
		uint32_t length = png_get_uint_32 (chunk_data);

		// Chunk type
		uint8_t chunk_type[5] = { 0, };
		memcpy (chunk_type, chunk_data + 4, 4);

		//g_print ("> NEXT CHUNK: '%s' (offset: %ld)\n", chunk_type, offset);
		if (g_strcmp0 ((char*) chunk_type, PNG_IDAT) == 0) {
			break;
		}

		// Next chunk
		offset += 12 + length;
	}

	guint metadata_len = offset;
	//g_print ("> METADATA SIZE: %u\n", metadata_len);

	guint canvas_width = gth_image_get_width (loader_data->image);
	guint canvas_height = gth_image_get_height (loader_data->image);
	GthImage *background = NULL;

	// For each frame build the png image appending to the
	// animation metadata the frame data chunks and the end chunk.
	// Load the frame png with _load_png
	// TODO: sort frames if required.
	for (int i = 0; i < animation->frames->len; i++) {
		Frame *frame = g_ptr_array_index (animation->frames, i);

		// Check frame size and position
		if ((frame->x_offset + frame->width > canvas_width)
			|| (frame->y_offset + frame->height > canvas_height))
		{
			g_set_error (error, G_IO_ERROR,	G_IO_ERROR_FAILED,
				"Frame %d does not fit the canvas size.", i);
			return false;
		}

		// Check first frame size and position.
		if (i == 0) {
			if ((frame->x_offset != 0)
				|| (frame->y_offset != 0))
			{
				g_set_error (error, G_IO_ERROR,	G_IO_ERROR_FAILED,
					"Frame %d has wrong origin (should be 0).", i);
				return false;
			}

			if ((frame->width != canvas_width)
				|| (frame->height != canvas_height))
			{
				g_set_error (error, G_IO_ERROR,	G_IO_ERROR_FAILED,
					"Frame %d has wrong size.", i);
				return false;
			}
		}

		if (frame->data_chunks->len == 0) {
			if (i == 0) {
				// Use the static image as first frame.
				GthImage *first_frame = gth_image_new_as_frame (loader_data->image);
				gth_image_add_frame (loader_data->image, first_frame, frame->delay);
				if (frame->dispose_op == APNG_DISPOSE_OP_NONE) {
					background = g_object_ref (first_frame);
				}
				g_object_unref (first_frame);
				continue;
			}

			// Error: frame without data
			g_set_error (error, G_IO_ERROR,	G_IO_ERROR_FAILED,
				"Frame %d has no data.", i);
			return false;
		}

		//g_print ("> BUILD FRAME %d (DATA CHUNKS: %d)\n", i, frame->data_chunks->len);

		GByteArray* frame_data = g_byte_array_new ();

		// Copy the main animation metadata
		g_byte_array_append (frame_data, animation_data, metadata_len);

		// Update width and height
		uint8_t *IHDR_chunk = frame_data->data + 8;
		uint8_t *IHDR_data = IHDR_chunk + 4 + 4;
		png_save_uint_32 (IHDR_data, frame->width);
		png_save_uint_32 (IHDR_data + 4, frame->height);
		// Update the IHDR chunk CRC
		uint8_t chunk_field[4];
		long chunk_crc = crc (IHDR_chunk + 4, 4 + 13);
		png_save_uint_32 (IHDR_chunk + 21, chunk_crc);

		// Add the IDAT chunks
		// TODO: sort the data chunks if required
		guint chunk_offset;
		for (int j = 0; j < frame->data_chunks->len; j++) {
			chunk_offset = frame_data->len;
			DataChunk *chunk = g_ptr_array_index (frame->data_chunks, j);
			// fdAT -> IDAT
			gsize chunk_size;
			const uint8_t *chunk_data = g_bytes_get_data (chunk->bytes, &chunk_size);
			png_save_uint_32 (chunk_field, chunk_size);
			g_byte_array_append (frame_data, chunk_field, 4);
			g_byte_array_append (frame_data, (const uint8_t *) PNG_IDAT, 4);
			g_byte_array_append (frame_data, chunk_data, chunk_size);
			// CRC of type and data (not length)
			chunk_crc = crc (frame_data->data + chunk_offset + 4, 4 + chunk_size);
			png_save_uint_32 (chunk_field, chunk_crc);
			g_byte_array_append (frame_data, chunk_field, 4);
		}

		// Add IEND
		chunk_offset = frame_data->len;
		png_save_uint_32 (chunk_field, 0); // length
		g_byte_array_append (frame_data, chunk_field, 4);
		g_byte_array_append (frame_data, (const uint8_t *) PNG_IEND, 4);
		chunk_crc = crc (frame_data->data + chunk_offset + 4, 4);
		png_save_uint_32 (chunk_field, chunk_crc);
		g_byte_array_append (frame_data, chunk_field, 4);

		GBytes *frame_bytes = g_byte_array_free_to_bytes (frame_data);
		//inspect_content (frame_bytes);

		GthImage *foreground = _load_png (frame_bytes, true, cancellable, error);
		g_bytes_unref (frame_bytes);

		if (foreground == NULL) {
			g_set_error (error, G_IO_ERROR,	G_IO_ERROR_FAILED,
				"Cannot allocate memory for frame %d.", i);
			return false;
		}

		// Render the new frame and add to loader_data.image

		GthImage *canvas = gth_image_new (canvas_width, canvas_height);
		if (canvas == NULL) {
			g_set_error (error, G_IO_ERROR,	G_IO_ERROR_FAILED,
				"Cannot allocate memory for frame %d.", i);
			return false;
		}
		gth_image_render_frame (canvas, background, 0,
			foreground, frame->x_offset, frame->y_offset,
			(frame->blend_op == APNG_BLEND_OP_OVER));
		gth_image_add_frame (loader_data->image, canvas, frame->delay);

		if (frame->dispose_op != APNG_DISPOSE_OP_PREVIOUS) {
			if (background != NULL) {
				g_object_unref (background);
				background = NULL;
			}
			if (frame->dispose_op == APNG_DISPOSE_OP_NONE) {
				background = g_object_ref (canvas);
			}
		}

		g_object_unref (canvas);
		g_object_unref (foreground);
	}
	if (background != NULL) {
		g_object_unref (background);
	}
	return true;
}

static GthImage* _load_png (GBytes *bytes, gboolean animation_frame, GCancellable *cancellable, GError **error) {
	LoaderData loader_data;
	loader_data.bytes = g_bytes_ref (bytes);
	loader_data.cancellable = cancellable;
	loader_data.bytes_offset = 0;
	loader_data.image = NULL;
	loader_data.error = error;
	loader_data.png_ptr = png_create_read_struct (PNG_LIBPNG_VER_STRING,
		&loader_data.error, error_func,	warning_func);
	init_animation (&loader_data.animation);

	if (loader_data.png_ptr == NULL) {
		loader_data_destroy (&loader_data);
		g_set_error_literal (error, G_IO_ERROR,	G_IO_ERROR_FAILED,
			"Could not create the png struct");
		return NULL;
	}

	loader_data.png_info_ptr = png_create_info_struct (loader_data.png_ptr);
	if (loader_data.png_info_ptr == NULL) {
		loader_data_destroy (&loader_data);
		g_set_error_literal (error, G_IO_ERROR,	G_IO_ERROR_FAILED,
			"Could not create the png info struct");
		return NULL;
	}

	if (PNG_SETJMP (loader_data.png_ptr)) {
		loader_data_destroy (&loader_data);
		return NULL;
	}

	png_set_read_fn (loader_data.png_ptr, &loader_data, read_buffer_func);
	if (!animation_frame) {
		png_set_read_user_chunk_fn (loader_data.png_ptr, &loader_data, read_unknown_chunk);
		// Ignore all unknown chunks
		png_set_keep_unknown_chunks (loader_data.png_ptr, PNG_HANDLE_CHUNK_NEVER, NULL, 0);
		// Save animation related chunks
		png_set_keep_unknown_chunks (loader_data.png_ptr, PNG_HANDLE_CHUNK_IF_SAFE, (png_const_bytep) APNG_CHUNKS, 3);
	}
	png_set_gamma (loader_data.png_ptr, PNG_DEFAULT_sRGB, PNG_DEFAULT_sRGB);
	png_read_info (loader_data.png_ptr, loader_data.png_info_ptr);

	png_uint_32 width, height;
	int bit_depth, color_type, interlace_type;

	png_get_IHDR (loader_data.png_ptr, loader_data.png_info_ptr,
		&width,	&height, &bit_depth, &color_type, &interlace_type,
		NULL, NULL);

	loader_data.image = gth_image_new (width, height);
	if (loader_data.image == NULL) {
		g_set_error_literal (error, G_IO_ERROR,	G_IO_ERROR_FAILED,
			"Failed to allocate memory");
		loader_data_destroy (&loader_data);
		return NULL;
	}
	gth_image_set_has_alpha (loader_data.image, (color_type & PNG_COLOR_MASK_ALPHA) || (color_type & PNG_COLOR_MASK_PALETTE));

	// Set the data transformations.

	png_set_strip_16 (loader_data.png_ptr);
	png_set_packing (loader_data.png_ptr);
	if (color_type == PNG_COLOR_TYPE_PALETTE) {
		png_set_palette_to_rgb (loader_data.png_ptr);
	}
	if ((color_type == PNG_COLOR_TYPE_GRAY) && (bit_depth < 8)) {
		png_set_expand_gray_1_2_4_to_8 (loader_data.png_ptr);
	}
	if (png_get_valid (loader_data.png_ptr, loader_data.png_info_ptr, PNG_INFO_tRNS)) {
	      png_set_tRNS_to_alpha (loader_data.png_ptr);
	}
	png_set_filler (loader_data.png_ptr, 0xff, PNG_FILLER_AFTER);
	if ((color_type == PNG_COLOR_TYPE_GRAY) || (color_type == PNG_COLOR_TYPE_GRAY_ALPHA)) {
		png_set_gray_to_rgb (loader_data.png_ptr);
	}
	if (interlace_type != PNG_INTERLACE_NONE) {
		png_set_interlace_handling (loader_data.png_ptr);
	}
	png_set_read_user_transform_fn (loader_data.png_ptr, transform_to_argb32_format_func);
	png_read_update_info (loader_data.png_ptr, loader_data.png_info_ptr);

	// Read the image.

	int row_stride;
	guchar *surface_row = gth_image_prepare_edit (loader_data.image, &row_stride, NULL, NULL);
	png_bytep *row_pointers = g_new (png_bytep, height);
	for (int row = 0; row < height; row++) {
		row_pointers[row] = surface_row;
		surface_row += row_stride;
	}
	png_read_image (loader_data.png_ptr, row_pointers);
	png_read_end (loader_data.png_ptr, loader_data.png_info_ptr);

	// Read some metadata.

	if (!animation_frame) {
		png_textp text_ptr;
		int num_texts;
		if (png_get_text (loader_data.png_ptr,
			loader_data.png_info_ptr,
			&text_ptr,
			&num_texts))
		{
			int original_image_width = -1;
			int original_image_height = -1;
			for (int i = 0; i < num_texts; i++) {
				if (strcmp (text_ptr[i].key, "Thumb::Image::Width") == 0) {
					original_image_width = atoi (text_ptr[i].text);
				}
				else if (strcmp (text_ptr[i].key, "Thumb::Image::Height") == 0) {
					original_image_height = atoi (text_ptr[i].text);
				}
			}
			if ((original_image_width > 0) && (original_image_height > 0)) {
				gth_image_set_original_image_size (loader_data.image,
					original_image_width,
					original_image_height);
			}
		}
	}

	g_free (row_pointers);

	// Read the color profile.

	{
		GthIccProfile *profile = NULL;
		int            intent;
		png_charp      name;
		int            compression_type	;
		png_bytep      icc_data;
		png_uint_32    icc_data_size;
		double         gamma;
		png_bytep      exif_data;
		png_uint_32    exif_data_size;

		if (png_get_sRGB (loader_data.png_ptr,
			loader_data.png_info_ptr,
			&intent) == PNG_INFO_sRGB)
		{
			profile = gth_icc_profile_new_srgb ();
		}
		else if (png_get_iCCP (loader_data.png_ptr,
				loader_data.png_info_ptr,
				&name,
				&compression_type,
				&icc_data,
				&icc_data_size) == PNG_INFO_iCCP)
		{
			if ((icc_data_size > 0) && (icc_data != NULL)) {
				GBytes *bytes = g_bytes_new (icc_data, icc_data_size);
				profile = gth_icc_profile_new_from_bytes (bytes, NULL);
				g_bytes_unref (bytes);
			}
		}
		else if (png_get_gAMA (loader_data.png_ptr,
				loader_data.png_info_ptr,
				&gamma))
		{
			profile = gth_icc_profile_new_srgb_with_gamma (1.0 / gamma);
		}
#ifdef PNG_eXIf_SUPPORTED
		else if (png_get_eXIf_1 (loader_data.png_ptr,
				loader_data.png_info_ptr,
				&exif_data_size,
				&exif_data))
		{
			JpegInfoData jpeg_info;
			_jpeg_info_data_init (&jpeg_info);
			if (_read_exif_data_tags (exif_data, exif_data_size, _JPEG_INFO_EXIF_COLOR_SPACE, &jpeg_info)) {
				if (jpeg_info.valid & _JPEG_INFO_EXIF_COLOR_SPACE) {
					if (jpeg_info.color_space == GTH_COLOR_SPACE_SRGB) {
						profile = gth_icc_profile_new_srgb ();
					}
					else if (jpeg_info.color_space == GTH_COLOR_SPACE_ADOBERGB) {
						profile = gth_icc_profile_new_adobergb ();
					}
				}
			}
		}
#endif
		if (profile != NULL) {
			gth_image_set_icc_profile (loader_data.image, profile);
			g_object_unref (profile);
		}
	}

	// Compose the animation if any.
	if (!animation_frame && (loader_data.animation.frames != NULL)) {
		if (!_compose_animation (&loader_data, cancellable, error)) {
			loader_data_destroy (&loader_data);
			return NULL;
		}
	}

	GthImage *result = g_object_ref (loader_data.image);
	loader_data_destroy (&loader_data);
	return result;
}

GthImage* load_png (GBytes *bytes, guint requested_size, GCancellable *cancellable, GError **error) {
	//inspect_content (bytes);
	return _load_png (bytes, FALSE, cancellable, error);
}

GHashTable* load_png_attributes (GBytes *bytes, GError **error) {
	LoaderData loader_data;
	loader_data.bytes = g_bytes_ref (bytes);
	loader_data.cancellable = NULL;
	loader_data.bytes_offset = 0;
	loader_data.image = NULL;
	loader_data.error = error;
	loader_data.png_ptr = png_create_read_struct (
		PNG_LIBPNG_VER_STRING,
		&loader_data.error,
		error_func,
		warning_func);
	init_animation (&loader_data.animation);

	if (loader_data.png_ptr == NULL) {
		loader_data_destroy (&loader_data);
		g_set_error_literal (error,
			G_IO_ERROR,
			G_IO_ERROR_FAILED,
			"Could not create the png struct");
		return NULL;
	}

	loader_data.png_info_ptr = png_create_info_struct (loader_data.png_ptr);
	if (loader_data.png_info_ptr == NULL) {
		loader_data_destroy (&loader_data);
		g_set_error_literal (error,
			G_IO_ERROR,
			G_IO_ERROR_FAILED,
			"Could not create the png info struct");
		return NULL;
	}

	if (PNG_SETJMP (loader_data.png_ptr)) {
		loader_data_destroy (&loader_data);
		return NULL;
	}

	png_set_read_fn (loader_data.png_ptr, &loader_data, read_buffer_func);
	png_read_info (loader_data.png_ptr, loader_data.png_info_ptr);

	GHashTable *attributes = g_hash_table_new_full (
		g_str_hash,
		g_str_equal,
		g_free,
		g_free);

	png_textp text_ptr;
	int num_texts;
	if (png_get_text (loader_data.png_ptr,
		loader_data.png_info_ptr,
		&text_ptr,
		&num_texts))
	{
		for (int i = 0; i < num_texts; i++) {
			g_hash_table_insert (
				attributes,
				g_strdup (text_ptr[i].key),
				g_strdup (text_ptr[i].text));
		}
	}

	loader_data_destroy (&loader_data);
	return attributes;
}
