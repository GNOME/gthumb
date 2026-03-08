#ifndef GTH_IMAGE_H
#define GTH_IMAGE_H

#include <glib.h>
#include <glib-object.h>
#include <gio/gio.h>
#include "lib/gth-color-manager.h"
#include "lib/gth-icc-profile.h"
#include "lib/gth-point.h"
#include "lib/gth-points.h"
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
GthImage * gth_image_new_as_frame (GthImage *self);
void gth_image_init_pixels (GthImage *self, guint width, guint height);
void gth_image_copy_pixels (GthImage *src, GthImage *dest);
void gth_image_copy_pixels_with_mask (GthImage *src, GthImage *dest, guint x, guint y, guint width, guint height);
void gth_image_copy_metadata (GthImage *src, GthImage *dest);
guchar * gth_image_get_pixels (GthImage *self, gsize *size);
guint gth_image_get_row_stride (GthImage *self);
GdkTexture * gth_image_get_texture (GthImage *self);
GdkTexture * gth_image_get_texture_for_rect (GthImage *self, guint x, guint y, guint width, guint height, guint frame_index);
GthImage * gth_image_get_subimage (GthImage *source, guint x, guint y, guint width, guint height);
gboolean gth_image_get_rgba (GthImage *self, guint x, guint y, guchar *red, guchar *green, guchar *blue, guchar *alpha);
guchar * gth_image_prepare_edit (GthImage *self, int *row_stride, int *width, int *height);
void gth_image_copy_from_rgba_big_endian (GthImage *self, const guchar *data, gboolean with_alpha, int row_stride);
void gth_image_copy_to_rgba_big_endian (GthImage *source, GthImage *dest);
cairo_surface_t * gth_image_to_surface (GthImage *source);

// Properties
guint gth_image_get_width (GthImage *self);
guint gth_image_get_height (GthImage *self);
gsize gth_image_get_size (GthImage *self);
gboolean gth_image_get_is_empty (GthImage *self);
void gth_image_set_has_alpha (GthImage *self, gboolean has_alpha);
gboolean gth_image_get_has_alpha (GthImage *self, gboolean *has_alpha);
gboolean gth_image_get_has_alpha_if_valid (GthImage *self);
void gth_image_get_natural_size (GthImage *self, guint *width, guint *height);
void gth_image_set_original_size (GthImage *self, guint width, guint height);
gboolean gth_image_get_original_size (GthImage *self, guint *width, guint *height);
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
gboolean gth_image_get_frame_at (GthImage *self, gulong *time, guint *frame_index);
gboolean gth_image_next_frame (GthImage *self, guint *frame_index);

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
GthImage * gth_image_resize_to (GthImage *self,
	guint scaled_width, guint scaled_height,
	GthScaleFilter quality,	GCancellable *cancellable);
GthImage * gth_image_resize (GthImage *self,
	guint size, GthResizeFlags flags, GthScaleFilter quality,
	GCancellable *cancellable);
void gth_image_resize_async (GthImage *self,
	guint size, GthResizeFlags flags, GthScaleFilter quality,
	GCancellable *cancellable,
	GAsyncReadyCallback callback, gpointer user_data);
GthImage * gth_image_resize_finish (GthImage *self, GAsyncResult *result,
	GError **error);

// Rotate
GthImage * gth_image_rotate (GthImage *self, float degrees,
	GdkRGBA *background_color, GthRotateFilter filter,
	GCancellable *cancellable);
void gth_image_rotate_async (GthImage *self, float degrees,
	GdkRGBA *background_color, GthRotateFilter filter,
	GCancellable *cancellable, GAsyncReadyCallback callback,
	gpointer user_data);
GthImage * gth_image_rotate_finish (GthImage *self, GAsyncResult *result,
	GError **error);

// Transform
GthImage * gth_image_apply_transform (GthImage *self, GthTransform transform, GCancellable *cancellable);
GthImage * gth_image_cut (GthImage *self, guint x, guint y, guint width, guint height, GCancellable *cancellable);

// Edit

void gth_image_fill_vertical (GthImage *self, GthImage *pattern, GthFill fill);
void gth_image_render_frame (GthImage *canvas, GthImage *background,
	guint32 background_color, GthImage *foreground, guint foreground_x,
	guint foreground_y, gboolean blend);
gboolean gth_image_apply_value_map (GthImage *self, guchar *value_map, GCancellable *cancellable);
gboolean gth_image_stretch_histogram (GthImage *self, double crop_size, GCancellable *cancellable);
gboolean gth_image_equalize_histogram (GthImage *self, gboolean linear, GCancellable *cancellable);
gboolean gth_image_gamma_correction (GthImage *self, double gamma, GCancellable *cancellable);
gboolean gth_image_adjust_brightness (GthImage *self, double amount, GCancellable *cancellable);
gboolean gth_image_adjust_contrast (GthImage *self, double amount, GCancellable *cancellable);
gboolean gth_image_grayscale (GthImage *self, double red_weight, double green_weight, double blue_weight, double amount, GCancellable *cancellable);
gboolean gth_image_grayscale_saturation (GthImage *self, double amount, GCancellable *cancellable);
GthImage * gth_image_blur (GthImage *self, int radius, GCancellable *cancellable);
GthImage * gth_image_gaussian_blur (GthImage *self, int radius, GCancellable *cancellable);
gboolean gth_image_progressive_blur (GthImage *source, int max_radius, GCancellable *cancellable);
gboolean gth_image_sharpen (GthImage *self, double amount, int radius, double threshold, GCancellable *cancellable);
gboolean gth_image_apply_vignette (GthImage *self, double amount, GCancellable *cancellable);
gboolean gth_image_apply_radial_mask (GthImage *background, GthImage *foreground, double amount, GCancellable *cancellable);
gboolean gth_image_apply_curve (GthImage *self, GthPoints *points, GCancellable *cancellable);
gboolean gth_image_colorize (GthImage *self, double red_amount, double green_amount, double blue_amount, GCancellable *cancellable);
gboolean gth_image_soft_light_with_radial_gradient (GthImage *self, GCancellable *cancellable);
gboolean gth_image_dither_ordered (GthImage *self, GCancellable *cancellable);
gboolean gth_image_dither_error_diffusion (GthImage *self, GCancellable *cancellable);
gboolean gth_image_pixelize (GthImage *self, guint size, GCancellable *cancellable);

G_END_DECLS

#endif /* GTH_IMAGE_H */
