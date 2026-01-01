#include <config.h>
#include <glib.h>
#ifdef HAVE_LCMS2
#include <lcms2.h>
#endif /* HAVE_LCMS2 */
#include "lib/gth-color-manager.h"
#include "lib/gth-image.h"
#include "lib/pixel.h"


typedef enum {
	METADATA_FLAG_NONE = 0,
	METADATA_FLAG_HAS_ALPHA = 1 << 1,
	METADATA_FLAG_ORIGINAL_SIZE = 1 << 2,
	METADATA_FLAG_ORIGINAL_IMAGE_SIZE = 1 << 3,
	METADATA_FLAG_ICC_PROFILE = 1 << 4,
} MetadataFlags;


struct _GthImagePrivate {
	GBytes *bytes;
	int row_stride;
	guint width;
	guint height;

	// Metatadata
	MetadataFlags metadata_flags;
	gboolean has_alpha;
	GthIccProfile *icc_profile;
	GHashTable *attributes;
	GFileInfo *info;

	// The image could be loaded at a smaller size (for example when
	// generating thumbnails), this is the original size:
	guint original_width;
	guint original_height;

	// Info about the original image if this image is a thumbnail.
	// TODO: move to attributes
	guint original_image_width;
	guint original_image_height;

	// Animation
	GPtrArray *frames;
	guint total_time; // Milliseconds
};

typedef struct {
	guint ref;
	GthImage *image;
	guint start; // Milliseconds
	guint delay; // Milliseconds
} GthFrame;

G_DEFINE_TYPE_WITH_CODE (GthImage,
			 gth_image,
			 G_TYPE_OBJECT,
			 G_ADD_PRIVATE (GthImage))

static GthFrame * gth_frame_new (GthImage *image, guint delay) {
	GthFrame *frame = g_new0 (GthFrame, 1);
	frame->ref = 1;
	frame->image = g_object_ref (image);
	frame->delay = delay;
	return frame;
}

static GthFrame * gth_frame_ref (GthFrame *frame) {
	g_return_val_if_fail (frame != NULL, NULL);
	frame->ref++;
	return frame;
}

static void gth_frame_unref (GthFrame *frame) {
	g_return_if_fail (frame != NULL);
	if (frame->ref > 0) {
		frame->ref--;
		if (frame->ref == 0) {
			g_object_unref (frame->image);
			g_free (frame);
		}
	}
}

static void _gth_image_free_data (GthImage *self) {
	if (self->priv->bytes != NULL) {
		g_bytes_unref (self->priv->bytes);
		self->priv->bytes = NULL;
	}
}

static void _gth_image_free_icc_profile (GthImage *self) {
	if (self->priv->icc_profile != NULL) {
		g_object_unref (self->priv->icc_profile);
		self->priv->icc_profile = NULL;
	}
}

static void gth_image_finalize (GObject *object) {
	g_return_if_fail (object != NULL);
	g_return_if_fail (GTH_IS_IMAGE (object));

	GthImage *self = GTH_IMAGE (object);
	_gth_image_free_data (self);
	_gth_image_free_icc_profile (self);
	if (self->priv->attributes != NULL) {
		g_hash_table_unref (self->priv->attributes);
	}
	_g_object_unref (self->priv->info);
	g_ptr_array_unref (self->priv->frames);

	/* Chain up */
	G_OBJECT_CLASS (gth_image_parent_class)->finalize (object);
}

static gboolean base_get_is_scalable (GthImage *self) {
	return FALSE;
}

static cairo_surface_t * base_get_scaled_texture (GthImage *self, double factor, guint x, guint y, guint width, guint height) {
	return NULL;
}

static void gth_image_class_init (GthImageClass *klass) {
	GObjectClass *gobject_class = (GObjectClass*) klass;
	gobject_class->finalize = gth_image_finalize;

	klass->get_is_scalable = base_get_is_scalable;
	klass->get_scaled_texture = base_get_scaled_texture;
}

static void gth_image_init (GthImage *self) {
	self->priv = gth_image_get_instance_private (self);
	self->priv->bytes = NULL;
	self->priv->row_stride = 0;
	self->priv->width = 0;
	self->priv->height = 0;
	self->priv->metadata_flags = METADATA_FLAG_NONE;
	self->priv->has_alpha = FALSE;
	self->priv->original_width = 0;
	self->priv->original_height = 0;
	self->priv->icc_profile = NULL;
	self->priv->attributes = NULL;
	self->priv->info = g_file_info_new ();
	self->priv->frames = g_ptr_array_new_with_free_func ((GDestroyNotify) gth_frame_unref);
	self->priv->total_time = 0;
}

void gth_image_init_pixels (GthImage *self, guint width, guint height) {
	g_return_if_fail (GTH_IS_IMAGE (self));
	g_return_if_fail (width > 0);
	g_return_if_fail (height > 0);
	g_return_if_fail (self->priv->bytes == NULL);

	self->priv->row_stride = (int) (width * PIXEL_BYTES);
	gsize size = (gsize) self->priv->row_stride * height;
	// TODO: check the size
	guchar *buffer = g_malloc (size);
	// TODO: check if buffer is NULL
	self->priv->bytes = g_bytes_new_take (buffer, size);
	self->priv->width = width;
	self->priv->height = height;
}

GthImage * gth_image_new (guint width, guint height) {
	g_return_val_if_fail (width > 0, NULL);
	g_return_val_if_fail (height > 0, NULL);
	GthImage *image = (GthImage *) g_object_new (GTH_TYPE_IMAGE, NULL);
	gth_image_init_pixels (image, width, height);
	return image;
}

GthImage * gth_image_new_from_texture (GdkTexture* texture) {
	g_return_val_if_fail (texture != NULL, NULL);
	GthImage *image = gth_image_new (
		(guint) gdk_texture_get_width (texture),
		(guint) gdk_texture_get_height (texture));
	const guchar *buffer = g_bytes_get_data (image->priv->bytes, NULL);
	gdk_texture_download (texture, buffer, image->priv->row_stride);
	return image;
}

GthImage * gth_image_new_from_cairo_surface (cairo_surface_t* surface) {
	g_return_val_if_fail (surface != NULL, NULL);

	GthImage *image = (GthImage *) g_object_new (GTH_TYPE_IMAGE, NULL);

	cairo_surface_flush (surface);
	guint width = (guint) cairo_image_surface_get_width (surface);
	guint height = (guint) cairo_image_surface_get_height (surface);
	int src_stride = cairo_image_surface_get_stride (surface);
	guchar *src_pixels = cairo_image_surface_get_data (surface);

	gth_image_init_pixels (image, width, height);

	int dest_stride;
	guchar *dest_pixels = gth_image_prepare_edit (image, &dest_stride, NULL, NULL);
	const guchar *src_row = src_pixels;
	guchar *dest_row = dest_pixels;
	size_t row_size = MIN (dest_stride, src_stride);
	for (int h = 0; h < height; h++) {
		memcpy (dest_row, src_row, row_size);
		src_row += src_stride;
		dest_row += dest_stride;
	}
	return image;
}

GthImage * gth_image_dup (GthImage *self) {
	g_return_val_if_fail (GTH_IS_IMAGE (self), NULL);
	GthImage *image = (GthImage *) g_object_new (GTH_TYPE_IMAGE, NULL);
	gth_image_copy_pixels (self, image);
	gth_image_copy_metadata (self, image);
	return image;
}

static GBytes * _g_bytes_new_slice (GBytes *bytes, gsize offset, gsize new_size) {
	gsize old_size;
	const guchar *old_buffer = g_bytes_get_data (bytes, &old_size);
	old_buffer += offset;
	guchar *new_buffer = g_memdup2 (old_buffer, old_size);
	return g_bytes_new_take (new_buffer, new_size);
}

void gth_image_copy_pixels (GthImage *src, GthImage *dest) {
	_gth_image_free_data (dest);
	dest->priv->bytes = _g_bytes_new_slice (src->priv->bytes, 0, g_bytes_get_size (src->priv->bytes));
	dest->priv->row_stride = src->priv->row_stride;
	dest->priv->width = src->priv->width;
	dest->priv->height = src->priv->height;
}

static void gth_image_set_frame (GthImage *self, GthImage *frame) {
	_gth_image_free_data (self);
	self->priv->bytes = g_bytes_ref (frame->priv->bytes);
	self->priv->row_stride = frame->priv->row_stride;
	self->priv->width = frame->priv->width;
	self->priv->height = frame->priv->height;
}

void gth_image_copy_metadata (GthImage *src, GthImage *dest) {
	g_return_if_fail (GTH_IS_IMAGE (src));
	g_return_if_fail (GTH_IS_IMAGE (dest));

	dest->priv->metadata_flags = src->priv->metadata_flags;
	if (src->priv->metadata_flags & METADATA_FLAG_HAS_ALPHA) {
		dest->priv->has_alpha = src->priv->has_alpha;
	}
	if (src->priv->metadata_flags & METADATA_FLAG_ORIGINAL_SIZE) {
		dest->priv->original_width = src->priv->original_width;
		dest->priv->original_height = src->priv->original_height;
	}
	if (src->priv->metadata_flags & METADATA_FLAG_ICC_PROFILE) {
		gth_image_set_icc_profile (dest, gth_image_get_icc_profile (src));
	}

	// Copy the attributes

	if (src->priv->attributes != NULL) {
		GHashTableIter iter;
		gpointer key, value;
		g_hash_table_iter_init (&iter, src->priv->attributes);
		while (g_hash_table_iter_next (&iter, &key, &value)) {
			gth_image_set_attribute (dest, (char*) key, (char*) value);
		}
	}

	if (src->priv->info != NULL) {
		g_file_info_copy_into (src->priv->info, dest->priv->info);
	}
}

guchar* gth_image_get_pixels (GthImage *self, gsize *size) {
	g_return_val_if_fail (GTH_IS_IMAGE (self), NULL);
	return (guchar*) g_bytes_get_data (self->priv->bytes, size);
}

guint gth_image_get_row_stride (GthImage *self) {
	g_return_val_if_fail (GTH_IS_IMAGE (self), 0);
	return (guint) self->priv->row_stride;
}

guchar * gth_image_prepare_edit (GthImage *self, int *row_stride, int *width, int *height) {
	g_return_val_if_fail (GTH_IS_IMAGE (self), NULL);
	if (row_stride != NULL)
		*row_stride = self->priv->row_stride;
	if (width != NULL)
		*width = (int) self->priv->width;
	if (height != NULL)
		*height = (int) self->priv->height;
	return gth_image_get_pixels (self, NULL);
}

void gth_image_copy_from_rgba_big_endian (GthImage *self, guchar *data, gboolean with_alpha, int data_stride) {
	unsigned char *surface_data = gth_image_get_pixels (self, NULL);
	int row_stride = self->priv->row_stride;
	if (with_alpha) {
		for (int row = 0; row < self->priv->height; row++) {
			rgba_big_endian_line_to_pixel (surface_data, data, self->priv->width);
			surface_data += row_stride;
			data += data_stride;
		}
	}
	else {
		for (int row = 0; row < self->priv->height; row++) {
			rgb_big_endian_line_to_pixel (surface_data, data, self->priv->width);
			surface_data += row_stride;
			data += data_stride;
		}
	}
}

GdkTexture * gth_image_get_texture (GthImage *self) {
	g_return_val_if_fail (GTH_IS_IMAGE (self), NULL);
	return gdk_memory_texture_new (
		self->priv->width,
		self->priv->height,
		GTH_IMAGE_MEMORY_FORMAT,
		self->priv->bytes,
		(gsize) self->priv->row_stride);
}

GdkTexture * gth_image_get_texture_for_rect (GthImage *self, guint x, guint y, guint width, guint height, guint frame_index) {
	g_return_val_if_fail (GTH_IS_IMAGE (self), NULL);
	GthImagePrivate *priv = self->priv;
	if ((priv->frames->len > 0) && (frame_index > 0) && (frame_index < priv->frames->len)) {
		GthFrame *frame = g_ptr_array_index (priv->frames, frame_index);
		priv = frame->image->priv;
	}

	// Check x and width
	if (width == 0) {
		return NULL;
	}
	if (x >= priv->width) {
		return NULL;
	}
	guint max_width = priv->width - x;
	width = CLAMP (width, 0, max_width);

	// Check y and height
	if (height == 0) {
		return NULL;
	}
	if (y >= priv->height) {
		return NULL;
	}
	guint max_height = priv->height - y;
	height = CLAMP (height, 0, max_height);

	gsize start_offset = (y * priv->row_stride) + (x * PIXEL_BYTES);
	gsize end_offset = ((y + height - 1) * priv->row_stride) + ((x + width) * PIXEL_BYTES);
	GBytes* bytes = g_bytes_new_from_bytes (priv->bytes, start_offset, end_offset - start_offset);
	//g_print ("> Image: [%u,%u] (stride: %d)\n", priv->width, priv->height, priv->row_stride);
	//g_print ("  Rect: (%u,%u)[%u,%u]\n", x, y, width, height);
	//g_print ("  Bytes: [%ld -> %ld] (size: %ld)\n", start_offset, end_offset, g_bytes_get_size (priv->bytes));
	GdkTexture *texture = gdk_memory_texture_new ((int) width, (int) height, GTH_IMAGE_MEMORY_FORMAT, bytes, (gsize) priv->row_stride);
	g_bytes_unref (bytes);
	return texture;
}

GthImage * gth_image_get_subimage (GthImage *source, guint x, guint y, guint width, guint height) {
	g_return_val_if_fail (GTH_IS_IMAGE (source), NULL);
	if (x + width > source->priv->width) {
		g_printf ("> subimage: x: %u, width: %u, max_width: %u\n", x, width, source->priv->width);
	}
	g_return_val_if_fail (x + width <= source->priv->width, NULL);
	if (y + height > source->priv->height) {
		g_printf ("> subimage: y: %u, height: %u, max_height: %u\n", y, height, source->priv->height);
	}
	g_return_val_if_fail (y + height <= source->priv->height, NULL);
	GthImage *image = (GthImage *) g_object_new (GTH_TYPE_IMAGE, NULL);
	image->priv->row_stride = source->priv->row_stride;
	image->priv->width = width;
	image->priv->height = height;
	gsize start_offset = (y * source->priv->row_stride) + (x * PIXEL_BYTES);
	gsize end_offset = ((y + height - 1) * source->priv->row_stride) + ((x + width) * PIXEL_BYTES);
	image->priv->bytes = g_bytes_new_from_bytes (source->priv->bytes, start_offset,	end_offset - start_offset);
	if (image->priv->bytes == NULL) {
		g_object_unref (image);
		return NULL;
	}
	gth_image_copy_metadata (source, image);
	return image;
}

gboolean gth_image_get_rgba (GthImage *self, guint x, guint y, guchar *red, guchar *green, guchar *blue, guchar *alpha) {
	g_return_if_fail (GTH_IS_IMAGE (self));
	if ((x >= self->priv->width) || (y >= self->priv->height)) {
		*red = 0;
		*green = 0;
		*blue = 0;
		*alpha = 0;
		return FALSE;
	}
	const guchar *buffer = g_bytes_get_data (self->priv->bytes, NULL);
	guchar *pixel_p = buffer + (y * self->priv->row_stride) + (x * PIXEL_BYTES);
	int temp;
	PIXEL_TO_RGBA (pixel_p, *red, *green, *blue, *alpha);
	return TRUE;
}

guint gth_image_get_width (GthImage *self) {
	g_return_val_if_fail (GTH_IS_IMAGE (self), 0);
	return self->priv->width;
}

guint gth_image_get_height (GthImage *self) {
	g_return_val_if_fail (GTH_IS_IMAGE (self), 0);
	return self->priv->height;
}

gsize gth_image_get_size (GthImage *self) {
	g_return_val_if_fail (GTH_IS_IMAGE (self), 0);
	return (self->priv->bytes != NULL) ? g_bytes_get_size (self->priv->bytes) : 0;
}

gboolean gth_image_get_is_empty (GthImage *self) {
	g_return_val_if_fail (GTH_IS_IMAGE (self), 0);
	return gth_image_get_size (self) == 0;
}

void gth_image_set_has_alpha (GthImage *self, gboolean has_alpha) {
	g_return_if_fail (GTH_IS_IMAGE (self));
	self->priv->metadata_flags |= METADATA_FLAG_HAS_ALPHA;
	self->priv->has_alpha = has_alpha;
}

gboolean gth_image_get_has_alpha (GthImage *self, gboolean *has_alpha) {
	g_return_val_if_fail (GTH_IS_IMAGE (self), FALSE);
	if ((self->priv->metadata_flags & METADATA_FLAG_HAS_ALPHA) == 0) {
		if (has_alpha != NULL) {
			*has_alpha = FALSE;
		}
		return FALSE;
	}
	if (has_alpha != NULL) {
		*has_alpha = self->priv->has_alpha;
	}
	return TRUE;
}

gboolean gth_image_get_has_alpha_if_valid (GthImage *self) {
	gboolean has_alpha = FALSE;
	gth_image_get_has_alpha (self, &has_alpha);
	return has_alpha;
}

void gth_image_get_natural_size (GthImage *self, guint *width, guint *height) {
	g_return_if_fail (GTH_IS_IMAGE (self));
	if (width != NULL) *width = self->priv->width;
	if (height != NULL) *height = self->priv->height;
}

void gth_image_set_original_size (GthImage *self, guint width, guint height) {
	g_return_if_fail (GTH_IS_IMAGE (self));
	self->priv->metadata_flags |= METADATA_FLAG_ORIGINAL_SIZE;
	self->priv->original_width = width;
	self->priv->original_height = height;
}

gboolean gth_image_get_original_size (GthImage *self, guint *width, guint *height) {
	g_return_val_if_fail (GTH_IS_IMAGE (self), FALSE);
	if ((self->priv->metadata_flags & METADATA_FLAG_ORIGINAL_SIZE) == 0)
		return FALSE;
	if (width != NULL) {
		*width = self->priv->original_width;
	}
	if (height != NULL) {
		*height = self->priv->original_height;
	}
	return TRUE;
}

gboolean gth_image_has_original_size (GthImage *self) {
	g_return_val_if_fail (GTH_IS_IMAGE (self), FALSE);
	return (self->priv->metadata_flags & METADATA_FLAG_ORIGINAL_SIZE) != 0;
}

// Size of the image the thumbnail refers to.
void gth_image_set_original_image_size (GthImage *self, guint width, guint height) {
	g_return_if_fail (GTH_IS_IMAGE (self));
	self->priv->metadata_flags |= METADATA_FLAG_ORIGINAL_IMAGE_SIZE;
	self->priv->original_image_width = width;
	self->priv->original_image_height = height;
}

gboolean gth_image_get_original_image_size (GthImage *self, guint *width, guint *height) {
	g_return_val_if_fail (GTH_IS_IMAGE (self), FALSE);
	if ((self->priv->metadata_flags & METADATA_FLAG_ORIGINAL_IMAGE_SIZE) == 0)
		return FALSE;
	if (width != NULL) {
		*width = self->priv->original_image_width;
	}
	if (height != NULL) {
		*height = self->priv->original_image_height;
	}
	return TRUE;
}

void gth_image_set_attribute (GthImage *self, const char *key, const char *value) {
	g_return_if_fail (GTH_IS_IMAGE (self));
	if (value == NULL) {
		gth_image_remove_attribute (self, key);
	}
	else {
		if (self->priv->attributes == NULL) {
			self->priv->attributes = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_free);
		}
		g_hash_table_insert (self->priv->attributes, g_strdup (key), g_strdup (value));
	}
}

gboolean gth_image_remove_attribute (GthImage *self, const char *key) {
	g_return_if_fail (GTH_IS_IMAGE (self));
	if (self->priv->attributes == NULL) {
		return false;
	}
	return g_hash_table_remove (self->priv->attributes, key);
}

const char * gth_image_get_attribute (GthImage *self, const char *key) {
	g_return_val_if_fail (GTH_IS_IMAGE (self), NULL);
	if (self->priv->attributes == NULL)
		return NULL;
	return g_hash_table_lookup (self->priv->attributes, key);
}

GHashTable * gth_image_get_attributes (GthImage *self) {
	g_return_val_if_fail (GTH_IS_IMAGE (self), NULL);
	return self->priv->attributes;
}

GFileInfo * gth_image_get_info (GthImage *self) {
	g_return_val_if_fail (GTH_IS_IMAGE (self), NULL);
	return self->priv->info;
}

void gth_image_set_info (GthImage *self, GFileInfo *info) {
	g_return_val_if_fail (GTH_IS_IMAGE (self), NULL);
	_g_object_ref (info);
	_g_object_unref (self->priv->info);
	self->priv->info = info;
}

gboolean gth_image_get_is_scalable (GthImage *self) {
	g_return_val_if_fail (GTH_IS_IMAGE (self), FALSE);
	return GTH_IMAGE_GET_CLASS (self)->get_is_scalable (self);
}

cairo_surface_t* gth_image_get_scaled_texture (GthImage *self, double factor, guint x, guint y, guint width, guint height) {
	g_return_val_if_fail (GTH_IS_IMAGE (self), NULL);
	return GTH_IMAGE_GET_CLASS (self)->get_scaled_texture (self, factor, x, y, width, height);
}

void gth_image_add_frame (GthImage *self, GthImage *frame_image, guint delay) {
	g_return_if_fail (GTH_IS_IMAGE (self));
	g_return_if_fail (GTH_IS_IMAGE (frame_image));
	GthImagePrivate *priv = self->priv;
	GthFrame *frame = gth_frame_new (frame_image, delay);
	g_ptr_array_add (priv->frames, frame);
	frame->start = priv->total_time;
	priv->total_time += frame->delay;
	if (priv->frames->len == 1) {
		gth_image_set_frame (self, frame->image);
	}
}

gboolean gth_image_get_is_animated (GthImage *self) {
	g_return_val_if_fail (GTH_IS_IMAGE (self), FALSE);
	return self->priv->frames->len > 1;
}

guint gth_image_get_frames (GthImage *self) {
	g_return_val_if_fail (GTH_IS_IMAGE (self), 0);
	return self->priv->frames->len;
}

gboolean gth_image_get_frame_at (GthImage *self, gulong *time, guint *frame_index) {
	g_return_val_if_fail (GTH_IS_IMAGE (self), FALSE);
	GthImagePrivate *priv = self->priv;
	if (priv->frames->len == 0) {
		return FALSE;
	}
	if (priv->frames->len == 1) {
		*frame_index = 0;
		return TRUE;
	}
	guint current_frame = 0;
	GthFrame *frame = g_ptr_array_index (priv->frames, current_frame);
	gulong current_time = *time;
	while (current_time > frame->start + frame->delay) {
		//g_print ("> %ul > %ul\n", current_time, frame->start + frame->delay);
		current_frame += 1;
		if (current_frame >= priv->frames->len) {
			// TODO: always loop?
			current_frame = 0;
			current_time = current_time - priv->total_time;
		}
		frame = g_ptr_array_index (priv->frames, current_frame);
	}
	*time = current_time;
	*frame_index = current_frame;
	return TRUE;
}

gboolean gth_image_next_frame (GthImage *self, guint *frame_index) {
	g_return_if_fail (GTH_IS_IMAGE (self));
	g_return_val_if_fail (GTH_IS_IMAGE (self), FALSE);
	GthImagePrivate *priv = self->priv;
	if (priv->frames->len <= 1) {
		return FALSE;
	}
	if (*frame_index >= priv->frames->len - 1) {
		*frame_index = 0;
	}
	else {
		*frame_index = *frame_index + 1;
	}
	return TRUE;
}

void gth_image_set_icc_profile (GthImage *self, GthIccProfile *profile) {
	g_return_if_fail (GTH_IS_IMAGE (self));

	if ((self->priv->icc_profile == NULL) && (profile != NULL)) {
		gth_image_set_attribute (self, "Private::ColorProfile", gth_icc_profile_get_description (profile));
	}

	_g_object_ref (profile);
	_gth_image_free_icc_profile (self);
	self->priv->icc_profile = profile;

	if (self->priv->icc_profile != NULL) {
		self->priv->metadata_flags |= METADATA_FLAG_ICC_PROFILE;
	}
	else {
		self->priv->metadata_flags &= ~METADATA_FLAG_ICC_PROFILE;
	}
}

GthIccProfile * gth_image_get_icc_profile (GthImage *self) {
	g_return_val_if_fail (GTH_IS_IMAGE (self), NULL);
	return self->priv->icc_profile;
}

bool gth_image_has_icc_profile (GthImage *self) {
	g_return_val_if_fail (GTH_IS_IMAGE (self), NULL);
	return self->priv->icc_profile != NULL;
}

void gth_image_apply_icc_profile (GthImage *self,
	GthColorManager *color_manager,
	GthIccProfile *out_profile,
	GCancellable *cancellable)
{
	g_return_if_fail (GTH_IS_IMAGE (self));

#if HAVE_LCMS2

	if (out_profile == NULL) {
		return;
	}

	if (self->priv->bytes == NULL) {
		return;
	}

	if (self->priv->icc_profile == NULL) {
		return;
	}

	GthIccTransform *transform = gth_color_manager_get_transform (
		color_manager,
		self->priv->icc_profile,
		out_profile
	);

	if (transform == NULL) {
		return;
	}

	cmsHTRANSFORM hTransform = (cmsHTRANSFORM) gth_icc_transform_get_transform (transform);
	const unsigned char *row_pointer = gth_image_get_pixels (self, NULL);
	for (guint row = 0; row < self->priv->height; row++) {
		if (g_cancellable_is_cancelled (cancellable)) {
			break;
		}
		cmsDoTransform (hTransform, row_pointer, row_pointer, self->priv->width);
		row_pointer += self->priv->row_stride;
	}

	g_object_unref (transform);

	gth_image_set_icc_profile (self, out_profile);

#endif
}

// -- gth_image_apply_icc_profile_async --

typedef struct {
	GthImage *image;
	GthColorManager *color_manager;
	GthIccProfile *out_profile;
} ApplyProfileData;

static void apply_profile_data_free (gpointer user_data) {
	ApplyProfileData *apd = user_data;
	g_object_unref (apd->image);
	g_object_unref (apd->color_manager);
	g_object_unref (apd->out_profile);
	g_free (apd);
}

static void _gth_image_apply_icc_profile_thread (GTask *task,
	gpointer source_object,
	gpointer task_data,
	GCancellable *cancellable)
{
	ApplyProfileData *apd = g_task_get_task_data (task);
	gth_image_apply_icc_profile (apd->image, apd->color_manager, apd->out_profile, cancellable);

	if ((cancellable != NULL) && g_cancellable_is_cancelled (cancellable)) {
		g_task_return_error (task, g_error_new_literal (G_IO_ERROR, G_IO_ERROR_CANCELLED, ""));
		return;
	}

	g_task_return_boolean (task, TRUE);
}

void gth_image_apply_icc_profile_async (GthImage *image,
	GthColorManager *color_manager,
	GthIccProfile *out_profile,
	GCancellable *cancellable,
	GAsyncReadyCallback callback,
	gpointer user_data)
{
	g_return_if_fail (image != NULL);
	g_return_if_fail (out_profile != NULL);

	ApplyProfileData *apd = g_new (ApplyProfileData, 1);
	apd->image = g_object_ref (image);
	apd->color_manager = g_object_ref (color_manager);
	apd->out_profile = g_object_ref (out_profile);

	GTask *task = g_task_new (NULL, cancellable, callback, user_data);
	g_task_set_task_data (task, apd, apply_profile_data_free);
	g_task_run_in_thread (task, _gth_image_apply_icc_profile_thread);

	g_object_unref (task);
}

gboolean gth_image_apply_icc_profile_finish (GthImage *self,
	GAsyncResult *result,
	GError **error)
{
	return g_task_propagate_boolean (G_TASK (result), error);
}
