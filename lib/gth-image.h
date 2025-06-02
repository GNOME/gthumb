#ifndef GTH_IMAGE_H
#define GTH_IMAGE_H

#include <glib.h>
#include <glib-object.h>
#include <gio/gio.h>
#include "lib/gth-color-manager.h"
#include "lib/gth-icc-profile.h"
#include "lib/lib.h"

G_BEGIN_DECLS

#define GTH_TYPE_IMAGE            (gth_image_get_type ())
#define GTH_IMAGE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTH_TYPE_IMAGE, GthImage))
#define GTH_IMAGE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GTH_TYPE_IMAGE, GthImageClass))
#define GTH_IS_IMAGE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTH_TYPE_IMAGE))
#define GTH_IS_IMAGE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GTH_TYPE_IMAGE))
#define GTH_IMAGE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), GTH_TYPE_IMAGE, GthImageClass))

typedef struct _GthImage         GthImage;
typedef struct _GthImageClass    GthImageClass;
typedef struct _GthImagePrivate  GthImagePrivate;

struct _GthImage {
	GObject __parent;
	GthImagePrivate *priv;
};

struct _GthImageClass {
	GObjectClass __parent_class;

	gboolean	(*get_is_zoomable)	(GthImage	*image);
	gboolean	(*set_zoom)		(GthImage	*image,
						 double		 zoom,
						 guint		*original_width,
						 guint		*original_height);
};

GType		gth_image_get_type			(void);
GthImage *	gth_image_new				(guint			 width,
							 guint			 height);
GthImage *	gth_image_dup				(GthImage		*image);
void		gth_image_copy_pixels			(GthImage		*src,
							 GthImage		*dest);
void		gth_image_copy_metadata			(GthImage		*src,
							 GthImage		*dest);
guchar*		gth_image_get_pixels			(GthImage		*image,
							 gsize			*size,
							 int			*row_stride);
guint		gth_image_get_width			(GthImage		*self);
guint		gth_image_get_height			(GthImage		*self);
gsize		gth_image_get_size			(GthImage		*image);
void		gth_image_set_has_alpha			(GthImage		*image,
							 gboolean		 has_alpha);
gboolean	gth_image_get_has_alpha			(GthImage		*image,
							 gboolean		*has_alpha);
void		gth_image_set_original_size		(GthImage		*image,
							 guint			 width,
							 guint			 height);
gboolean	gth_image_get_original_size		(GthImage		*image,
							 guint			*width,
							 guint			*height);
gboolean	gth_image_has_original_size		(GthImage		*image);
void		gth_image_set_original_image_size	(GthImage		*image,
							 guint			 width,
							 guint			 height);
gboolean	gth_image_get_original_image_size	(GthImage		*image,
							 guint			*width,
							 guint			*height);
void		gth_image_set_attribute			(GthImage		*image,
							 const char 		*key,
							 const char 		*value);
const char *	gth_image_get_attribute			(GthImage		*image,
							 const char 		*key);
GHashTable *    gth_image_get_attributes		(GthImage		*image);
gboolean	gth_image_get_is_zoomable		(GthImage		*image);
gboolean	gth_image_set_zoom			(GthImage		*image,
							 double			 zoom,
							 guint			*original_width,
							 guint			*original_height);
GdkTexture *	gth_image_get_gdk_texture		(GthImage		*image);
void		gth_image_set_icc_profile		(GthImage		*image,
							 GthIccProfile		*profile);
GthIccProfile *	gth_image_get_icc_profile		(GthImage		*image);
void		gth_image_apply_icc_profile		(GthImage		*image,
							 GthColorManager	*color_manager,
							 GthIccProfile		*out_profile,
							 GCancellable		*cancellable);
void		gth_image_apply_icc_profile_async	(GthImage		*image,
							 GthColorManager	*color_manager,
							 GthIccProfile		*out_profile,
							 GCancellable		*cancellable,
							 GAsyncReadyCallback	 callback,
							 gpointer		 user_data);
gboolean	gth_image_apply_icc_profile_finish	(GthImage		*image,
							 GAsyncResult		*result,
							 GError			**error);
GthImage *	gth_image_resize			(GthImage		*image,
							 guint			 size,
							 GthResizeFlags		 flags,
							 GthScaleFilter		 quality,
							 GCancellable		*cancellable);
void		gth_image_resize_async			(GthImage		*image,
							 guint			 size,
							 GthResizeFlags		 flags,
							 GthScaleFilter		 quality,
							 GCancellable		*cancellable,
							 GAsyncReadyCallback	 callback,
							 gpointer		 user_data);
GthImage *	gth_image_resize_finish			(GthImage		*image,
							 GAsyncResult		*result,
							 GError			**error);

G_END_DECLS

#endif /* GTH_IMAGE_H */
