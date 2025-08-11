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
	guchar *buffer;
	gsize size;
	int row_stride;
	guint width;
	guint height;
	MetadataFlags metadata_flags;
	gboolean has_alpha;
	guint original_width;
	guint original_height;
	guint original_image_width;
	guint original_image_height;
	GthIccProfile *icc_profile;
	GBytes *bytes;
	GHashTable *attributes;
};


G_DEFINE_TYPE_WITH_CODE (GthImage,
			 gth_image,
			 G_TYPE_OBJECT,
			 G_ADD_PRIVATE (GthImage))


static void _gth_image_free_data (GthImage *self) {
	if (self->priv->buffer != NULL) {
		g_free (self->priv->buffer);
		self->priv->buffer = NULL;
	}
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

	/* Chain up */
	G_OBJECT_CLASS (gth_image_parent_class)->finalize (object);
}


static gboolean base_get_can_scale (GthImage *self) {
	return FALSE;
}


static GthImage * base_scale (GthImage *self, double factor) {
	return NULL;
}


static void gth_image_class_init (GthImageClass *klass) {
	GObjectClass *gobject_class = (GObjectClass*) klass;
	gobject_class->finalize = gth_image_finalize;

	klass->get_can_scale = base_get_can_scale;
	klass->scale = base_scale;
}


static void gth_image_init (GthImage *self) {
	self->priv = gth_image_get_instance_private (self);
	self->priv->buffer = NULL;
	self->priv->size = 0;
	self->priv->row_stride = 0;
	self->priv->width = 0;
	self->priv->height = 0;
	self->priv->metadata_flags = METADATA_FLAG_NONE;
	self->priv->has_alpha = FALSE;
	self->priv->original_width = 0;
	self->priv->original_height = 0;
	self->priv->icc_profile = NULL;
	self->priv->bytes = NULL;
	self->priv->attributes = NULL;
}


GthImage * gth_image_new (guint width, guint height) {
	g_return_val_if_fail (width > 0, NULL);
	g_return_val_if_fail (height > 0, NULL);
	GthImage *image = (GthImage *) g_object_new (GTH_TYPE_IMAGE, NULL);
	gth_image_init_pixels (image, width, height);
	gth_image_set_natural_size (image, width, height);
	return image;
}


GthImage * gth_image_new_from_texture (GdkTexture* texture) {
	g_return_val_if_fail (texture != NULL, NULL);
	GthImage *image = gth_image_new (
		(guint) gdk_texture_get_width (texture),
		(guint) gdk_texture_get_height (texture));
	gdk_texture_download (texture, image->priv->buffer, image->priv->row_stride);
	return image;
}


void gth_image_init_pixels (GthImage *self, guint width, guint height) {
	g_return_if_fail (GTH_IS_IMAGE (self));
	g_return_if_fail (width > 0);
	g_return_if_fail (height > 0);
	g_return_if_fail (self->priv->buffer == NULL);

	self->priv->row_stride = (int) (width * PIXEL_BYTES);
	self->priv->size = (gsize) self->priv->row_stride * height;
	// TODO: check the size
	self->priv->buffer = g_malloc (self->priv->size);
	// TODO: check if buffer is NULL
	self->priv->width = width;
	self->priv->height = height;
	self->priv->bytes = g_bytes_new_static (self->priv->buffer, self->priv->size);
}


GthImage * gth_image_dup (GthImage *self) {
	g_return_val_if_fail (GTH_IS_IMAGE (self), NULL);

	GthImage *image = (GthImage *) g_object_new (GTH_TYPE_IMAGE, NULL);
	gth_image_copy_pixels (self, image);
	gth_image_copy_metadata (self, image);

	return image;
}


void gth_image_copy_pixels (GthImage *src, GthImage *dest) {
	dest->priv->buffer = g_memdup2 (src->priv->buffer, src->priv->size);
	dest->priv->size = src->priv->size;
	dest->priv->row_stride = src->priv->row_stride;
	dest->priv->width = src->priv->width;
	dest->priv->height = src->priv->height;
	dest->priv->bytes = g_bytes_new_static (dest->priv->buffer, dest->priv->size);
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
}


guchar* gth_image_get_pixels (GthImage *self, gsize *size, int *row_stride) {
	g_return_val_if_fail (GTH_IS_IMAGE (self), NULL);
	if (size != NULL)
		*size = self->priv->size;
	if (row_stride != NULL)
		*row_stride = self->priv->row_stride;
	return self->priv->buffer;
}


guint gth_image_get_row_stride (GthImage *self) {
	g_return_val_if_fail (GTH_IS_IMAGE (self), 0);
	return (guint) self->priv->row_stride;
}


guchar * gth_image_prepare_edit (GthImage *self, int *row_stride, guint *width, guint *height) {
	g_return_val_if_fail (GTH_IS_IMAGE (self), NULL);
	if (row_stride != NULL)
		*row_stride = self->priv->row_stride;
	if (width != NULL)
		*width = self->priv->width;
	if (height != NULL)
		*height = self->priv->height;
	return self->priv->buffer;
}


void gth_image_copy_from_rgba_big_endian (GthImage *self, guchar *data, gboolean with_alpha, int data_stride) {
	int surface_stride;
	unsigned char *surface_data = gth_image_get_pixels (self, NULL, &surface_stride);
	if (with_alpha) {
		for (int row = 0; row < self->priv->height; row++) {
			rgba_big_endian_line_to_pixel (surface_data, data, self->priv->width);
			surface_data += surface_stride;
			data += data_stride;
		}
	}
	else {
		for (int row = 0; row < self->priv->height; row++) {
			rgb_big_endian_line_to_pixel (surface_data, data, self->priv->width);
			surface_data += surface_stride;
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


GdkTexture * gth_image_get_texture_for_rect (GthImage *self, guint x, guint y, guint width, guint height) {
	g_return_val_if_fail (GTH_IS_IMAGE (self), NULL);

	// Check x and width
	if (width == 0) {
		return NULL;
	}
	if (x >= self->priv->width) {
		return NULL;
	}
	guint max_width = self->priv->width - x;
	width = CLAMP (width, 0, max_width);

	// Check y and height
	if (height == 0) {
		return NULL;
	}
	if (y >= self->priv->height) {
		return NULL;
	}
	guint max_height = self->priv->height - y;
	height = CLAMP (height, 0, max_height);

	gsize start_offset = (y * self->priv->row_stride) + (x * PIXEL_BYTES);
	gsize end_offset = ((y + height - 1) * self->priv->row_stride) + ((x + width) * PIXEL_BYTES);
	GBytes* bytes = g_bytes_new_from_bytes (self->priv->bytes,
		start_offset,
		end_offset - start_offset);
	//g_print ("> Image: [%u,%u] (stride: %d)\n", self->priv->width, self->priv->height, self->priv->row_stride);
	//g_print ("> Rect: (%u,%u)[%u,%u]\n", x, y, width, height);
	//g_print ("  Bytes: [%ld -> %ld] (size: %ld)\n", start_offset, end_offset, g_bytes_get_size (self->priv->bytes));
	return gdk_memory_texture_new (
		(int) width,
		(int) height,
		GTH_IMAGE_MEMORY_FORMAT,
		bytes,
		(gsize) self->priv->row_stride);
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
	return self->priv->size;
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


void gth_image_set_natural_size (GthImage *self, guint width, guint height) {
	g_return_if_fail (GTH_IS_IMAGE (self));
	self->priv->metadata_flags |= METADATA_FLAG_ORIGINAL_SIZE;
	self->priv->original_width = width;
	self->priv->original_height = height;
}


gboolean gth_image_get_natural_size (GthImage *self, guint *width, guint *height) {
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


gboolean gth_image_get_can_scale (GthImage *self) {
	g_return_val_if_fail (GTH_IS_IMAGE (self), FALSE);
	return GTH_IMAGE_GET_CLASS (self)->get_can_scale (self);
}


GthImage* gth_image_scale (GthImage *self, double zoom) {
	g_return_val_if_fail (GTH_IS_IMAGE (self), FALSE);
	return GTH_IMAGE_GET_CLASS (self)->scale (self, zoom);
}

void gth_image_set_icc_profile (GthImage *self, GthIccProfile *profile) {
	g_return_if_fail (GTH_IS_IMAGE (self));
	if (profile != NULL) {
		g_object_ref (profile);
	}
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

	if (self->priv->buffer == NULL) {
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
	const unsigned char *row_pointer = self->priv->buffer;
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
