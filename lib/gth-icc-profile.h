#ifndef GTH_ICC_PROFILE_H
#define GTH_ICC_PROFILE_H

#include <glib.h>

G_BEGIN_DECLS

typedef gpointer GthCmsProfile;
typedef gpointer GthCmsTransform;

#define GTH_ICC_PROFILE_ID_UNKNOWN "unknown://"
#define GTH_ICC_PROFILE_WITH_MD5 "md5://"
#define GTH_ICC_PROFILE_FROM_PROPERTY "property://"

#define GTH_TYPE_ICC_PROFILE            (gth_icc_profile_get_type ())
#define GTH_ICC_PROFILE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTH_TYPE_ICC_PROFILE, GthIccProfile))
#define GTH_ICC_PROFILE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GTH_TYPE_ICC_PROFILE, GthIccProfileClass))
#define GTH_IS_ICC_PROFILE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTH_TYPE_ICC_PROFILE))
#define GTH_IS_ICC_PROFILE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GTH_TYPE_ICC_PROFILE))
#define GTH_ICC_PROFILE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), GTH_TYPE_ICC_PROFILE, GthIccProfileClass))

#define GTH_TYPE_ICC_TRANSFORM            (gth_icc_transform_get_type ())
#define GTH_ICC_TRANSFORM(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTH_TYPE_ICC_TRANSFORM, GthIccTransform))
#define GTH_ICC_TRANSFORM_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GTH_TYPE_ICC_TRANSFORM, GthIccTransformClass))
#define GTH_IS_ICC_TRANSFORM(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTH_TYPE_ICC_TRANSFORM))
#define GTH_IS_ICC_TRANSFORM_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GTH_TYPE_ICC_TRANSFORM))
#define GTH_ICC_TRANSFORM_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), GTH_TYPE_ICC_TRANSFORM, GthIccTransformClass))

typedef struct _GthIccProfile         GthIccProfile;
typedef struct _GthIccProfilePrivate  GthIccProfilePrivate;
typedef struct _GthIccProfileClass    GthIccProfileClass;

typedef struct _GthIccTransform         GthIccTransform;
typedef struct _GthIccTransformPrivate  GthIccTransformPrivate;
typedef struct _GthIccTransformClass    GthIccTransformClass;

struct _GthIccProfile {
	GObject __parent;
	GthIccProfilePrivate *priv;
};

struct _GthIccProfileClass {
	GObjectClass __parent_class;
};

struct _GthIccTransform {
	GObject __parent;
	GthIccTransformPrivate *priv;
};

struct _GthIccTransformClass {
	GObjectClass __parent_class;
};

void			gth_cms_profile_free		(GthCmsProfile	  profile);
void			gth_cms_transform_free		(GthCmsTransform  transform);

GType			gth_icc_profile_get_type	(void);
GthIccProfile *		gth_icc_profile_new		(const char	 *id,
							 GthCmsProfile	  profile);
GthIccProfile *		gth_icc_profile_new_srgb	(void);
GthIccProfile *		gth_icc_profile_new_adobergb	(void);
GthIccProfile *		gth_icc_profile_new_srgb_with_gamma
							(double gamma);
const char *		gth_icc_profile_get_id		(GthIccProfile	 *icc_profile);
const char *            gth_icc_profile_get_description	(GthIccProfile	 *icc_profile);
gboolean                gth_icc_profile_id_is_unknown   (const char      *id);
GthCmsProfile		gth_icc_profile_get_profile	(GthIccProfile	 *icc_profile);
gboolean		gth_icc_profile_equal		(GthIccProfile	 *a,
							 GthIccProfile	 *b);

GType			gth_icc_transform_get_type	(void);
GthIccTransform * 	gth_icc_transform_new		(GthCmsTransform  transform);
GthIccTransform *	gth_icc_transform_new_from_profiles
							(GthIccProfile   *from_profile,
							 GthIccProfile   *to_profile);
GthCmsTransform         gth_icc_transform_get_transform (GthIccTransform *transform);

G_END_DECLS

#endif /* GTH_ICC_PROFILE_H */
