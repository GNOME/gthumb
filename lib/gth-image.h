#ifndef GTH_IMAGE_H
#define GTH_IMAGE_H

#include <glib.h>
#include <glib-object.h>
#include <gio/gio.h>
#include "lib/gth-color-manager.h"
#include "lib/gth-icc-profile.h"
#include "lib/lib.h"

G_BEGIN_DECLS

#if G_BYTE_ORDER == G_LITTLE_ENDIAN
#define GTH_IMAGE_MEMORY_FORMAT GDK_MEMORY_B8G8R8A8_PREMULTIPLIED
#elif G_BYTE_ORDER == G_BIG_ENDIAN
#define GTH_IMAGE_MEMORY_FORMAT GDK_MEMORY_A8R8G8B8_PREMULTIPLIED
#endif

#define GTH_TYPE_IMAGE (gth_image_get_type ())
#define GTH_IMAGE(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTH_TYPE_IMAGE, GthImage))
#define GTH_IMAGE_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), GTH_TYPE_IMAGE, GthImageClass))
#define GTH_IS_IMAGE(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTH_TYPE_IMAGE))
#define GTH_IS_IMAGE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GTH_TYPE_IMAGE))
#define GTH_IMAGE_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS((obj), GTH_TYPE_IMAGE, GthImageClass))

typedef struct _GthImage GthImage;
typedef struct _GthImageClass GthImageClass;
typedef struct _GthImagePrivate GthImagePrivate;

struct _GthImage {
	GObject __parent;
	GthImagePrivate *priv;
};

struct _GthImageClass {
	GObjectClass __parent_class;
	gboolean (*get_is_scalable) (GthImage *self);
	cairo_surface_t * (*get_scaled_texture) (GthImage *self, double factor, guint x, guint y, guint width, guint height);
};

GType gth_image_get_type (void);

GthImage * gth_image_new (guint width, guint height);
GthImage * gth_image_new_from_texture (GdkTexture* texture);
GthImage * gth_image_new_from_cairo_surface (cairo_surface_t* surface);
GthImage * gth_image_dup (GthImage *self);
void gth_image_init_pixels (GthImage *self, guint width, guint height);
void gth_image_copy_pixels (GthImage *src, GthImage *dest);
void gth_image_copy_metadata (GthImage *src, GthImage *dest);
guchar * gth_image_get_pixels (GthImage *self, gsize *size);
guint gth_image_get_row_stride (GthImage *self);
GdkTexture * gth_image_get_texture (GthImage *self);
GdkTexture * gth_image_get_texture_for_rect (GthImage *self, guint x, guint y, guint width, guint height);
GthImage * gth_image_get_subimage (GthImage *source, guint x, guint y, guint width, guint height);
guchar * gth_image_prepare_edit (GthImage *self, int *row_stride, int *width, int *height);
void gth_image_copy_from_rgba_big_endian (GthImage *self, guchar *data, gboolean with_alpha, int row_stride);

// Properties
guint gth_image_get_width (GthImage *self);
guint gth_image_get_height (GthImage *self);
gsize gth_image_get_size (GthImage *self);
void gth_image_set_has_alpha (GthImage *self, gboolean has_alpha);
gboolean gth_image_get_has_alpha (GthImage *self, gboolean *has_alpha);
void gth_image_set_natural_size (GthImage *self, guint width, guint height);
gboolean gth_image_get_natural_size (GthImage *self, guint *width, guint *height);
gboolean gth_image_has_original_size (GthImage *self);
void gth_image_set_original_image_size (GthImage *self, guint width, guint height);
gboolean gth_image_get_original_image_size (GthImage *self, guint *width, guint *height);

// Attributes
void gth_image_set_attribute (GthImage *self, const char *key, const char *value);
gboolean gth_image_remove_attribute (GthImage *self, const char *key);
const char * gth_image_get_attribute (GthImage *self, const char *key);
GHashTable * gth_image_get_attributes (GthImage *self);
GFileInfo * gth_image_get_info (GthImage *self);
void gth_image_set_info (GthImage *self, GFileInfo *info);

// Scalable images
gboolean gth_image_get_is_scalable (GthImage *self);
cairo_surface_t * gth_image_get_scaled_texture (GthImage *self, double factor, guint x, guint y, guint width, guint height);

// Animated images
void gth_image_add_frame (GthImage *self, GthImage *frame, uint delay);
gboolean gth_image_get_is_animated (GthImage *self);
guint gth_image_get_frames (GthImage *self);
gboolean gth_image_change_time (GthImage *self, GthChangeTime op, gulong milliseconds);
void gth_image_next_frame (GthImage *self);

// ICC profile
void gth_image_set_icc_profile (GthImage *self, GthIccProfile *profile);
GthIccProfile * gth_image_get_icc_profile (GthImage *self);
bool gth_image_has_icc_profile (GthImage *self);
void gth_image_apply_icc_profile (GthImage *self,
	GthColorManager	*color_manager,
	GthIccProfile *out_profile,
	GCancellable *cancellable);
void gth_image_apply_icc_profile_async (GthImage *self,
	GthColorManager	*color_manager,
	GthIccProfile *out_profile,
	GCancellable *cancellable,
	GAsyncReadyCallback callback,
	gpointer user_data);
gboolean gth_image_apply_icc_profile_finish(GthImage *self,
	GAsyncResult *result,
	GError **error);

// Resize
GthImage * gth_image_resize (GthImage *self,
	guint size, GthResizeFlags flags, GthScaleFilter quality,
	GCancellable *cancellable);
void gth_image_resize_async (GthImage *self,
	guint size, GthResizeFlags flags, GthScaleFilter quality,
	GCancellable *cancellable,
	GAsyncReadyCallback callback, gpointer user_data);
GthImage * gth_image_resize_finish (GthImage *self, GAsyncResult *result,
	GError **error);

// Transform
GthImage * gth_image_apply_transform (GthImage *self, GthTransform transform, GCancellable *cancellable);

// Edit

void gth_image_fill_vertical (GthImage *self, GthImage *pattern, GthFill fill);
void gth_image_render_frame (GthImage *canvas, GthImage *background,
	guint32 background_color, GthImage *foreground, guint foreground_x,
	guint foreground_y, gboolean blend);

G_END_DECLS

#endif /* GTH_IMAGE_H */
