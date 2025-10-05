#include <config.h>
#include <math.h>
#include <gif_lib.h>
#include "load-gif.h"

// GthImageGif (private class)
#define GTH_IMAGE_GIF(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), gth_image_gif_get_type(), GthImageGif))

typedef struct {
	GthImage __parent;
	GPtrArray *frames;
	guint total_time; // Milliseconds
	guint current_time;
	int current_frame;
} GthImageGif;

typedef GthImageClass GthImageGifClass;

G_DEFINE_TYPE (GthImageGif, gth_image_gif, GTH_TYPE_IMAGE)

typedef struct {
	int ref;
	GthImage *image;
	guint start; // Milliseconds
	guint delay; // Milliseconds
} GthFrame;

static GthFrame * gth_frame_new (GthImage *image, guint delay) {
	GthFrame *frame = g_new0 (GthFrame, 1);
	frame->ref = 1;
	frame->image = g_object_ref (image);
	frame->delay = delay;
	return frame;
}

static GthFrame * gth_frame_ref (GthFrame *frame) {
	frame->ref++;
	return frame;
}

static void gth_frame_unref (GthFrame *frame) {
	frame->ref--;
	if (frame->ref == 0) {
		g_object_unref (frame->image);
		g_free (frame);
	}
}

static void gth_image_gif_finalize (GObject *object) {
	GthImageGif *self = GTH_IMAGE_GIF (object);
	g_ptr_array_unref (self->frames);
	G_OBJECT_CLASS (gth_image_gif_parent_class)->finalize (object);
}

static void gth_image_gif_init (GthImageGif *self) {
	self->frames = g_ptr_array_new_with_free_func ((GDestroyNotify) gth_frame_unref);
	self->total_time = 0;
	self->current_time = 0;
	self->current_frame = 0;
}

static gboolean gth_image_gif_get_is_animated (GthImage *base) {
	GthImageGif *self = GTH_IMAGE_GIF (base);
	return self->frames->len > 1;
}

static gboolean gth_image_gif_change_time (GthImage *base, GthChangeTime op, gulong milliseconds) {
	GthImageGif *self = GTH_IMAGE_GIF (base);
	if (op == GTH_CHANGE_TIME_ADD) {
		self->current_time += milliseconds;
	}
	else if (op == GTH_CHANGE_TIME_SET) {
		self->current_time = milliseconds;
	}
	GthFrame *frame = g_ptr_array_index (self->frames, self->current_frame);
	//g_print ("> %ul > %ul\n", self->current_time, frame->start + frame->delay);
	while (self->current_time > frame->start + frame->delay) {
		self->current_frame += 1;
		if (self->current_frame >= self->frames->len) {
			// TODO: always loop?
			self->current_frame = 0;
			self->current_time = self->current_time - self->total_time;
		}
		frame = g_ptr_array_index (self->frames, self->current_frame);
	}
	gth_image_set_pixels (GTH_IMAGE (self), frame->image);
	return TRUE;
}

static void gth_image_gif_class_init (GthImageGifClass *klass) {
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	object_class->finalize = gth_image_gif_finalize;

	GthImageClass *image_class = GTH_IMAGE_CLASS (klass);
	image_class->get_is_animated = gth_image_gif_get_is_animated;
	image_class->change_time = gth_image_gif_change_time;
}

static GthImage * gth_image_gif_new (guint width, guint height) {
	GthImageGif *image = (GthImageGif *) g_object_new (gth_image_gif_get_type (), NULL);
	gth_image_set_natural_size (GTH_IMAGE (image), width, height);
	gth_image_set_has_alpha (GTH_IMAGE (image), TRUE);
	return (GthImage *) image;
}

static void gth_image_gif_add_frame (GthImageGif *self, GthFrame *frame) {
	g_ptr_array_add (self->frames, gth_frame_ref (frame));
	frame->start = self->total_time;
	self->total_time += frame->delay;
	if (self->frames->len == 1) {
		gth_image_set_pixels (GTH_IMAGE (self), frame->image);
		self->current_frame = 0;
		self->current_time = 0;
	}
}

typedef struct {
	const void *buffer;
	gsize size;
	int offset;
} ReadData;

static int read_bytes_func (GifFileType *file, GifByteType *dest, int size) {
	ReadData *read_data = file->UserData;
	int next_offset = read_data->offset + size;
	if (next_offset > read_data->size) {
		size = (int) (read_data->size - read_data->offset);
	}
	//g_print (">> READ %d\n", size);
	if (size <= 0) {
		return 0;
	}
	memcpy (dest, read_data->buffer + read_data->offset, size);
	read_data->offset = next_offset;
	return size;
}

static const char *get_error_text (int code) {
	const char *str = GifErrorString (code);
	return (str != NULL) ? str : "Undefined error";
}

typedef struct {
	ColorMapObject *color_map;
	const uint8_t *screen_buffer;
	int screen_row_size;
	GraphicsControlBlock gcb;
	GthImage *prev_image;
	int last_disposal_mode;
} RenderContext;

static GthImage * render_frame (GifFileType *file, RenderContext *render_ctx) {
	GthImage *image = gth_image_new (file->SWidth, file->SHeight);
	if (image == NULL) {
		//g_print (">> [1]\n");
		return NULL;
	}
	int image_row_stride;
	guchar *image_row = gth_image_prepare_edit (image, &image_row_stride, NULL, NULL);
	guchar *pixel_p;

	int prev_image_row_stride;
	guchar *prev_image_row = NULL;
	guchar *prev_pixel_p = NULL;
	gboolean has_previous_image = render_ctx->prev_image != NULL;
	if (has_previous_image) {
		prev_image_row = gth_image_prepare_edit (render_ctx->prev_image, &prev_image_row_stride, NULL, NULL);
	}

	uint8_t * screen_row;
	GifColorType *entry;
	for (int i = 0; i < file->SHeight; i++) {
		screen_row = render_ctx->screen_buffer + (render_ctx->screen_row_size * i);
		pixel_p = image_row;
		prev_pixel_p = prev_image_row;
		for (int j = 0; j < file->SWidth; j++) {
			if ((screen_row[j] == render_ctx->gcb.TransparentColor)
				// Outside the current image
				|| (i < file->Image.Top)
				|| (i >= file->Image.Top + file->Image.Height)
				|| (j < file->Image.Left)
				|| (j >= file->Image.Left + file->Image.Width))
			{
				if (render_ctx->last_disposal_mode == DISPOSE_BACKGROUND) {
					*(guint32*) pixel_p = (guint32) file->SBackGroundColor;
				}
				else if (has_previous_image) {
					*(guint32*) pixel_p = *(guint32*) prev_pixel_p;
				}
				else {
					*(guint32*) pixel_p = (guint32) 0;
				}
			}
			else {
				// Inside the current image and not transparent.
				// Check if color is within color palette
				if (screen_row[j] >= render_ctx->color_map->ColorCount) {
					//g_print (">> [2] %d >= %d\n", screen_row[j], render_ctx->color_map->ColorCount);
					g_object_unref (image);
					return NULL;
				}
				else {
					entry = &render_ctx->color_map->Colors[screen_row[j]];
					*(guint32*) pixel_p = RGBA_TO_PIXEL (entry->Red, entry->Green, entry->Blue, 0xFF);
				}
			}
			pixel_p += 4;
			if (has_previous_image) {
				prev_pixel_p += 4;
			}
		}
		image_row += image_row_stride;
		if (has_previous_image) {
			prev_image_row += prev_image_row_stride;
		}
	}
	return image;
}

GthImage * load_gif (GBytes *bytes, guint requested_size, GCancellable *cancellable, GError **error) {
	gsize buffer_size;
	const void *buffer = g_bytes_get_data (bytes, &buffer_size);
	ReadData read_data = {
		.buffer = buffer,
		.size = buffer_size,
		.offset = 0,
	};

	int error_code;
	GifFileType *file = DGifOpen (&read_data, read_bytes_func, &error_code);
	if (file == NULL) {
		//g_print ("> ERROR %d\n", error_code);
		g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_FAILED, get_error_text (error_code));
		return NULL;
	}

	//g_print ("> ScreenSize: %dx%d\n", file->SWidth, file->SHeight);

	if ((file->SWidth == 0) || (file->SHeight == 0)) {
		g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_FAILED, _("Invalid file format"));
		DGifCloseFile (file, NULL);
		return NULL;
	}

	// Screen buffer
	int screen_row_size = file->SWidth * sizeof (GifPixelType);
	int screen_buffer_size = file->SHeight * screen_row_size;
	uint8_t *screen_buffer = (uint8_t *) g_malloc0 (screen_buffer_size);
	if (screen_buffer == NULL) {
		g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_FAILED, "Failed to allocate memory");
		DGifCloseFile (file, NULL);
		return NULL;
	}
	//memset (screen_buffer, (GifPixelType) file->SBackGroundColor, screen_buffer_size);

	// Scan the content of the GIF file and load the image(s):

	GthImage *image = NULL;
	static const int InterlacedOffset[] = { 0, 4, 2, 1}; // The way Interlaced image should.
	static const int InterlacedJumps[] = { 8, 8, 4, 2};  // be read - offsets and jumps...
	int image_idx = 0;
	GifRecordType record_type;
	RenderContext render_ctx = {
		.color_map = NULL,
		.screen_buffer = screen_buffer,
		.screen_row_size = screen_row_size,
		.prev_image = NULL,
		.last_disposal_mode = DISPOSAL_UNSPECIFIED
	};
	uint8_t *screen_row;

	file->ExtensionBlocks = NULL;
	file->ExtensionBlockCount = 0;

	do {
		int row, col, width, height;
		GifByteType *ext_data;
		int ext_code;

		if ((cancellable != NULL) && g_cancellable_is_cancelled (cancellable)) {
			g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_CANCELLED, "Cancelled");
			goto close;
		}

		if (DGifGetRecordType (file, &record_type) == GIF_ERROR) {
			g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_FAILED, get_error_text (file->Error));
			goto close;
		}

		switch (record_type) {
		case IMAGE_DESC_RECORD_TYPE:
			//g_print ("> IMAGE_DESC_RECORD_TYPE\n");
			if (DGifGetImageDesc (file) == GIF_ERROR) {
				g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_FAILED, get_error_text (file->Error));
				goto close;
			}

			render_ctx.gcb.DisposalMode = DISPOSAL_UNSPECIFIED;
			render_ctx.gcb.UserInputFlag = 0;
			render_ctx.gcb.DelayTime = 10;
			render_ctx.gcb.TransparentColor = NO_TRANSPARENT_COLOR;

			//g_print ("  ExtensionBlockCount %d\n", file->ExtensionBlockCount);
			for (int i = 0; i < file->ExtensionBlockCount; i++) {
				ExtensionBlock *block = file->ExtensionBlocks + i;
				if ((uint8_t) block->Function == 0xF9) {
					DGifExtensionToGCB (block->ByteCount, block->Bytes, &render_ctx.gcb);
					//g_print ("  > DisposalMode: %d\n", render_ctx.gcb.DisposalMode);
					//g_print ("    DelayTime: %d\n", render_ctx.gcb.DelayTime);
					//g_print ("    TransparentColor: %d\n", render_ctx.gcb.TransparentColor);
				}
			}

			row = file->Image.Top; // Image Position relative to Screen.
			col = file->Image.Left;
			width = file->Image.Width;
			height = file->Image.Height;
			if (image_idx == 0) {
				image = gth_image_gif_new (file->SWidth, file->SHeight);
			}
			image_idx++;
			//g_print ("  Image %d at (%d, %d) [%dx%d]:\n", image_idx, col, row, width, height);

			if ((file->Image.Left + file->Image.Width > file->SWidth)
				|| (file->Image.Top + file->Image.Height > file->SHeight))
			{
				g_set_error (error, G_IO_ERROR, G_IO_ERROR_FAILED, "Image %d is not inside the screen.", image_idx);
				goto close;
			}

			//g_print ("  Interlace: %d\n", file->Image.Interlace);
			if (file->Image.Interlace) {
				// Need to perform 4 passes on the images:
				for (int i = 0; i < 4; i++) {
					for (int j = row + InterlacedOffset[i]; j < row + height; j += InterlacedJumps[i]) {
						uint8_t *screen_row = screen_buffer + (j * screen_row_size) + col;
						if (DGifGetLine (file, screen_row, width) == GIF_ERROR) {
							g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_FAILED, get_error_text (file->Error));
							goto close;
						}
					}
				}
			}
			else {
				uint8_t *screen_row = screen_buffer + (row * screen_row_size) + col;
				for (int i = 0; i < height; i++) {
					if (DGifGetLine (file, screen_row, width) == GIF_ERROR) {
						g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_FAILED, get_error_text (file->Error));
						goto close;
					}
					screen_row += screen_row_size;
				}
			}

			render_ctx.color_map = (file->Image.ColorMap ? file->Image.ColorMap : file->SColorMap);
			if (render_ctx.color_map == NULL) {
				g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_FAILED, "Gif Image does not have a colormap.");
				goto close;
			}

			// Check that the background color is valid.
			if ((file->SBackGroundColor < 0)
				|| (file->SBackGroundColor >= render_ctx.color_map->ColorCount))
			{
				file->SBackGroundColor = 0;
				//g_set_error (error, G_IO_ERROR, G_IO_ERROR_FAILED, "Background color out of range for colormap: %d (colors: %d)", file->SBackGroundColor, render_ctx.color_map->ColorCount);
				//goto close;
			}

			GthImage *frame_image = render_frame (file, &render_ctx);
			if (frame_image == NULL) {
				g_set_error (error, G_IO_ERROR, G_IO_ERROR_FAILED, "Failed to create frame %d", image_idx);
				goto close;
			}

			guint milliseconds = (render_ctx.gcb.DelayTime > 0) ? (guint) render_ctx.gcb.DelayTime * 10 : 100;
			GthFrame *frame = gth_frame_new (frame_image, milliseconds);
			gth_image_gif_add_frame (GTH_IMAGE_GIF (image), frame);

			if (render_ctx.gcb.DisposalMode == DISPOSE_BACKGROUND) {
				// Reset prev_image to NULL.
				if (render_ctx.prev_image != NULL) {
					g_object_unref (render_ctx.prev_image);
					render_ctx.prev_image = NULL;
				}
				//memset (screen_buffer, (GifPixelType) file->SBackGroundColor, screen_buffer_size);
			}
			else if (render_ctx.gcb.DisposalMode != DISPOSE_PREVIOUS) {
				if (render_ctx.prev_image != NULL) {
					g_object_unref (render_ctx.prev_image);
				}
				render_ctx.prev_image = g_object_ref (frame_image);
			}
			render_ctx.last_disposal_mode = render_ctx.gcb.DisposalMode;

			g_object_unref (frame_image);
			gth_frame_unref (frame);

			file->ExtensionBlocks = NULL;
			file->ExtensionBlockCount = 0;
			break;

		case EXTENSION_RECORD_TYPE:
			if (DGifGetExtension (file, &ext_code, &ext_data) == GIF_ERROR) {
				g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_FAILED, get_error_text (file->Error));
				goto close;
			}
			// Create an extension block with our data
			if (ext_data != NULL) {
				if (GifAddExtensionBlock (&file->ExtensionBlockCount,
					&file->ExtensionBlocks,
					ext_code,
					ext_data[0],
					&ext_data[1]) == GIF_ERROR)
				{
					return (GIF_ERROR);
				}
			}
			while (ext_data != NULL) {
				if (DGifGetExtensionNext (file, &ext_data) == GIF_ERROR) {
					g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_FAILED, "Failed");
					goto close;
				}
				if (ext_data != NULL) {
					// Continue the extension block
					if (GifAddExtensionBlock (&file->ExtensionBlockCount,
						&file->ExtensionBlocks,
						CONTINUE_EXT_FUNC_CODE,
						ext_data[0],
						&ext_data[1]) == GIF_ERROR)
					{
						g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_FAILED, "Failed");
						goto close;
					}
				}
			}
			break;

		case TERMINATE_RECORD_TYPE:
			break;

		default: // Should be trapped by DGifGetRecordType.
			break;
		}
	}
	while (record_type != TERMINATE_RECORD_TYPE);

close:

	if (render_ctx.prev_image != NULL) {
		g_object_unref (render_ctx.prev_image);
		render_ctx.prev_image = NULL;
	}
	g_free (screen_buffer);
	DGifCloseFile (file, NULL);

	if ((error != NULL) && (*error != NULL)) {
		if (image != NULL) {
			g_object_unref (image);
			image = NULL;
		}
	}
	return image;
}

// GIF format described here: https://giflib.sourceforge.net/whatsinagif/bits_and_bytes.html
gboolean load_gif_info (const char *buffer, int buffer_size, GthImageInfo *image_info, GCancellable *cancellable) {
	if (buffer_size < 13) {
		//g_print ("  [0]\n");
		return FALSE;
	}
	const uint8_t *data = buffer;

	image_info->width = ((int) data[7] << 8) + (int) data[6];
	image_info->height = ((int) data[9] << 8) + (int) data[8];
	//g_print ("> screen size: %d %d\n", image_info->width, image_info->height);
	return TRUE;

	uint8_t flags = data[10];
	//g_print ("> buffer[%d]: %X\n", 10, (uint8_t) data[10]);
	gboolean has_color_table = (flags & 0b10000000) != 0;
	//g_print ("  has_color_table: %d\n", has_color_table);
	int offset = 13;
	if (has_color_table) {
		// Size of Global Color Table
		int size = (flags & 0b00000111);
		int entries = (int) pow (2, size + 1);
		//g_print ("  N: %d (entries: %d) (size: %d)\n", N, entries, size);
		// Skip the color table
		offset += entries * 3;
	}
	//g_print ("  [1.1] data[offset]: %X (offset: %d)\n", (uint8_t) data[offset], offset);
	if (offset > buffer_size) {
		//g_print ("  [1.2]\n");
		return FALSE;
	}
	// Skip extension blocks
	while (data[offset] == 0x21) {
		if (offset + 3 > buffer_size) {
			//g_print ("  [2]\n");
			return FALSE;
		}
		offset += 2;
		int block_size = data[offset];
		offset += 1;
		while (block_size > 0) {
			offset += block_size;
			if (offset + 1 > buffer_size) {
				//g_print ("  [3]\n");
				return FALSE;
			}
			block_size = data[offset];
			offset += 1;
		}
	}
	// Image descriptor
	if ((offset + 8 > buffer_size) || (data[offset] != 0x2C)) {
		//g_print ("  [4] data[offset]: %X (offset: %d) (buffer_size: %d)\n", (uint8_t) data[offset], offset, buffer_size);
		return FALSE;
	}
	// First image size
	image_info->width = ((int) data[offset + 6] << 8) + (int) data[offset + 5];
	image_info->height = ((int) data[offset + 8] << 8) + (int) data[offset + 7];
	return TRUE;
}
