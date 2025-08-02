#include <config.h>
#include <glib.h>
#include <glib-object.h>
#ifdef HAVE_LCMS2
#include <lcms2.h>
#endif /* HAVE_LCMS2 */
#include "lib/gth-icc-profile.h"


struct _GthIccProfilePrivate {
	GthIccType type;
	GBytes *bytes;
	double gamma;
	char *id;
	char *description;
	GthCmsProfile cms_profile;
};


struct _GthIccTransformPrivate {
	GthCmsTransform cms_transform;
};


G_DEFINE_TYPE_WITH_CODE (GthIccProfile,
			 gth_icc_profile,
			 G_TYPE_OBJECT,
			 G_ADD_PRIVATE (GthIccProfile))

G_DEFINE_TYPE_WITH_CODE (GthIccTransform,
			 gth_icc_transform,
			 G_TYPE_OBJECT,
			 G_ADD_PRIVATE (GthIccTransform))


void
gth_cms_profile_free (GthCmsProfile profile)
{
#ifdef HAVE_LCMS2
	if (profile != NULL) {
		cmsCloseProfile ((cmsHPROFILE) profile);
	}
#endif
}


void
gth_cms_transform_free (GthCmsTransform transform)
{
#ifdef HAVE_LCMS2
	if (transform != NULL) {
		cmsDeleteTransform ((cmsHTRANSFORM) transform);
	}
#endif
}


/* -- GthIccProfile -- */


static void
gth_icc_profile_finalize (GObject *object)
{
	GthIccProfile *icc_profile;

	g_return_if_fail (object != NULL);
	g_return_if_fail (GTH_IS_ICC_PROFILE (object));

	icc_profile = GTH_ICC_PROFILE (object);
	gth_cms_profile_free (icc_profile->priv->cms_profile);
	g_free (icc_profile->priv->id);
	g_free (icc_profile->priv->description);
	g_bytes_unref (icc_profile->priv->bytes);

	/* Chain up */
	G_OBJECT_CLASS (gth_icc_profile_parent_class)->finalize (object);
}


static void
gth_icc_profile_class_init (GthIccProfileClass *klass)
{
	GObjectClass *gobject_class;

	gobject_class = (GObjectClass*) klass;
	gobject_class->finalize = gth_icc_profile_finalize;
}


static void
gth_icc_profile_init (GthIccProfile *self)
{
	self->priv = gth_icc_profile_get_instance_private (self);
	self->priv->type = GTH_ICC_TYPE_UNKNOWN;
	self->priv->cms_profile = NULL;
	self->priv->id = NULL;
	self->priv->description = NULL;
	self->priv->bytes = NULL;
	self->priv->gamma = 0.0;
}


static char *
_gth_cms_profile_create_id (GthCmsProfile cms_profile)
{
	GString  *str;
	gboolean  id_set = FALSE;

	str = g_string_new ("");

#ifdef HAVE_LCMS2
	{
		// Taken from colord:
		// Copyright (C) 2013 Richard Hughes <richard@hughsie.com>
		// Licensed under the GNU Lesser General Public License Version 2.1
		//
		cmsUInt8Number  icc_id[16];
		gboolean        md5_precooked = FALSE;
		gchar          *md5 = NULL;
		guint           i;

		cmsGetHeaderProfileID ((cmsHPROFILE) cms_profile, icc_id);
		for (i = 0; i < 16; i++) {
			if (icc_id[i] != 0) {
				md5_precooked = TRUE;
				break;
			}
		}
		if (md5_precooked) {
			// Convert to a hex string.
			md5 = g_new0 (gchar, 32 + 1);
			for (i = 0; i < 16; i++)
				g_snprintf (md5 + i * 2, 3, "%02x", icc_id[i]);
		}

		if (md5 != NULL) {
			g_string_append (str, GTH_ICC_PROFILE_WITH_MD5);
			g_string_append (str, md5);
			id_set = TRUE;
		}
	}
#endif

	if (!id_set) {
		g_string_append (str, GTH_ICC_PROFILE_ID_UNKNOWN);
	}

	return g_string_free (str, FALSE);
}


GthIccProfile *
gth_icc_profile_new_srgb (void)
{
#ifdef HAVE_LCMS2

	static GthIccProfile *icc_profile = NULL;

	if (icc_profile == NULL) {
		icc_profile = g_object_new (GTH_TYPE_ICC_PROFILE, NULL);
		icc_profile->priv->type = GTH_ICC_TYPE_SRGB;
		icc_profile->priv->id = g_strdup ("standard://srgb");
		icc_profile->priv->description = g_strdup ("sRGB");
		icc_profile->priv->cms_profile = (GthCmsProfile) cmsCreate_sRGBProfile ();
	}
	return g_object_ref (icc_profile);

#else

	return NULL;

#endif
}


GthIccProfile *
gth_icc_profile_new_adobergb (void)
{
#ifdef HAVE_LCMS2

	static GthIccProfile *icc_profile = NULL;

	if (icc_profile == NULL) {
		// Voodoo numbers taken from:
		// https://github.com/ellelstone/elles_icc_profiles/blob/master/code/make-elles-profiles.c
		// Released under the GNU GPL v2 or later.
		cmsCIExyY d65_srgb_adobe_specs = { 0.3127, 0.3290, 1.0 };
		cmsCIExyYTRIPLE adobe_primaries_prequantized = {
			{ 0.639996511, 0.329996864, 1.0 },
			{ 0.210005295, 0.710004866, 1.0 },
			{ 0.149997606, 0.060003644, 1.0 }
		};
		cmsToneCurve *tonecurve = cmsBuildGamma (NULL, 2.19921875);
		cmsToneCurve *curve[3];
		curve[0] = curve[1] = curve[2] = tonecurve;

		GthCmsProfile cms_profile = (GthCmsProfile) cmsCreateRGBProfile (
			&d65_srgb_adobe_specs,
			&adobe_primaries_prequantized,
			curve);

		icc_profile = g_object_new (GTH_TYPE_ICC_PROFILE, NULL);
		icc_profile->priv->type = GTH_ICC_TYPE_ADOBERGB;
		icc_profile->priv->id = g_strdup ("standard://adobergb-comp");
		icc_profile->priv->description = g_strdup ("Adobe RGB compatible");
		icc_profile->priv->cms_profile = cms_profile;
	}

	return g_object_ref (icc_profile);

#else

	return NULL;

#endif
}


GthIccProfile *
gth_icc_profile_new_srgb_with_gamma (double gamma)
{
#ifdef HAVE_LCMS2

	cmsCIExyY WhitePoint;
	if (!cmsWhitePointFromTemp (&WhitePoint, 6500))
		return NULL;

	cmsCIExyYTRIPLE Rec709Primaries = {
		{0.6400, 0.3300, 1.0},
		{0.3000, 0.6000, 1.0},
		{0.1500, 0.0600, 1.0}
	};

	cmsToneCurve* Gamma[3];
	Gamma[0] = Gamma[1] = Gamma[2] = cmsBuildGamma (NULL, gamma);
	if (Gamma[0] == NULL) {
		return NULL;
	}

	cmsHPROFILE cms_profile = cmsCreateRGBProfile (&WhitePoint, &Rec709Primaries, Gamma);
	cmsFreeToneCurve (Gamma[0]);
	if (cms_profile == NULL) {
		return NULL;
	}

	GthIccProfile *icc_profile = g_object_new (GTH_TYPE_ICC_PROFILE, NULL);
	icc_profile->priv->type = GTH_ICC_TYPE_SRGB_GAMMA;
	icc_profile->priv->gamma = gamma;
	icc_profile->priv->id = _gth_cms_profile_create_id (cms_profile);
	icc_profile->priv->description = g_strdup_printf ("sRGB Gamma=1/%.1f", gamma);
	icc_profile->priv->cms_profile = cms_profile;
	return icc_profile;

#else

	return NULL;

#endif
}


GthIccProfile *
gth_icc_profile_new_from_bytes (GBytes *bytes, const char *id)
{
	gsize size;
	gconstpointer data = g_bytes_get_data (bytes, &size);
	g_return_val_if_fail (data != NULL, NULL);
	g_return_val_if_fail (size > 0, NULL);

	GthIccProfile *icc_profile = g_object_new (GTH_TYPE_ICC_PROFILE, NULL);
	icc_profile->priv->type = GTH_ICC_TYPE_BYTES;
	icc_profile->priv->bytes = g_bytes_ref (bytes);
	icc_profile->priv->cms_profile = cmsOpenProfileFromMem (data, size);
	icc_profile->priv->id = (id != NULL) ? g_strdup (id) : _gth_cms_profile_create_id (icc_profile->priv->cms_profile);
	return icc_profile;
}


const char *
gth_icc_profile_get_id (GthIccProfile *self)
{
	g_return_val_if_fail (GTH_IS_ICC_PROFILE (self), NULL);
	return self->priv->id;
}


static char *
_g_utf8_try_from_any (const char *str) {
	if (str == NULL)
		return NULL;
	char *utf8_str;
	if (! g_utf8_validate (str, -1, NULL))
		utf8_str = g_locale_to_utf8 (str, -1, NULL, NULL, NULL);
	else
		utf8_str = g_strdup (str);
	return utf8_str;
}


const char *
gth_icc_profile_get_description	(GthIccProfile *self)
{
	if (self->priv->description != NULL) {
		return self->priv->description;
	}

#ifdef HAVE_LCMS2

	GString *color_profile = g_string_new ("");
	cmsHPROFILE hProfile = (cmsHPROFILE) gth_icc_profile_get_profile (self);
	const int buffer_size = 128;
	wchar_t buffer[buffer_size];
	cmsUInt32Number size = cmsGetProfileInfo (hProfile, cmsInfoDescription, "en", "US", (wchar_t *) buffer, buffer_size);
	if (size > 0) {
		for (int i = 0; (i < size) && (buffer[i] != 0); i++) {
			g_string_append_c (color_profile, buffer[i]);
		}
	}

	if (color_profile->len > 0) {
		self->priv->description = _g_utf8_try_from_any (color_profile->str);
	}

	g_string_free (color_profile, TRUE);

	return self->priv->description;

#else

	return NULL;

#endif
}


gboolean
gth_icc_profile_id_is_unknown (const char *id)
{
	return (id == NULL) || (g_strcmp0 (id, GTH_ICC_PROFILE_ID_UNKNOWN) == 0);
}


GthCmsProfile
gth_icc_profile_get_profile (GthIccProfile *self)
{
	g_return_val_if_fail (self != NULL, NULL);
	return self->priv->cms_profile;
}


gboolean
gth_icc_profile_equal (GthIccProfile *a,
		       GthIccProfile *b)
{
	g_return_val_if_fail ((a == NULL) || (b == NULL), FALSE);

	if (gth_icc_profile_id_is_unknown (a->priv->id)
		|| gth_icc_profile_id_is_unknown (b->priv->id))
	{
		return FALSE;
	}
	else {
		return g_strcmp0 (a->priv->id, b->priv->id) == 0;
	}
}


GthIccType
gth_icc_profile_get_known_type (GthIccProfile *self)
{
	g_return_val_if_fail (self != NULL, GTH_ICC_TYPE_UNKNOWN);
	return self->priv->type;
}


double
gth_icc_profile_get_gamma (GthIccProfile *self)
{
	g_return_val_if_fail (self != NULL, 0.0);
	return self->priv->gamma;
}


GBytes *
gth_icc_profile_get_bytes (GthIccProfile *self)
{
	g_return_val_if_fail (self != NULL, NULL);
	switch (self->priv->type) {
	case GTH_ICC_TYPE_BYTES:
		g_return_val_if_fail (self->priv->bytes != NULL, NULL);
		return g_bytes_ref (self->priv->bytes);

	case GTH_ICC_TYPE_SRGB:
	case GTH_ICC_TYPE_ADOBERGB:
	case GTH_ICC_TYPE_SRGB_GAMMA:
		guint32 size;
		if (cmsSaveProfileToMem (self->priv->cms_profile, NULL, &size)) {
			gpointer data = g_malloc (size);
			if (cmsSaveProfileToMem (self->priv->cms_profile, data, &size)) {
				GBytes *bytes = g_bytes_new_take (data, size);
				data = NULL;
				return bytes;
			}
			g_free (data);
		}
		break;

	default:
		break;
	}
	return NULL;
}


/* -- GthIccTransform -- */


static void
gth_icc_transform_finalize (GObject *object)
{
	GthIccTransform *icc_transform;

	g_return_if_fail (object != NULL);
	g_return_if_fail (GTH_IS_ICC_TRANSFORM (object));

	icc_transform = GTH_ICC_TRANSFORM (object);
	gth_cms_transform_free (icc_transform->priv->cms_transform);

	/* Chain up */
	G_OBJECT_CLASS (gth_icc_transform_parent_class)->finalize (object);
}


static void
gth_icc_transform_class_init (GthIccTransformClass *klass)
{
	GObjectClass *gobject_class;

	gobject_class = (GObjectClass*) klass;
	gobject_class->finalize = gth_icc_transform_finalize;
}


static void
gth_icc_transform_init (GthIccTransform *self)
{
	self->priv = gth_icc_transform_get_instance_private (self);
	self->priv->cms_transform = NULL;
}


GthIccTransform *
gth_icc_transform_new (GthCmsTransform transform)
{
	GthIccTransform *icc_transform;

	icc_transform = g_object_new (GTH_TYPE_ICC_TRANSFORM, NULL);
	icc_transform->priv->cms_transform = transform;

	return icc_transform;
}


#if HAVE_LCMS2
#if G_BYTE_ORDER == G_LITTLE_ENDIAN /* BGRA */
#define _LCMS2_PIXEL_FORMAT TYPE_BGRA_8
#elif G_BYTE_ORDER == G_BIG_ENDIAN /* ARGB */
#define _LCMS2_PIXEL_FORMAT TYPE_ARGB_8
#endif
#endif


GthIccTransform *
gth_icc_transform_new_from_profiles (GthIccProfile *from_profile,
				     GthIccProfile *to_profile)
{
#if HAVE_LCMS2

	GthCmsTransform cms_transform = (GthCmsTransform) cmsCreateTransform (
		(cmsHPROFILE) gth_icc_profile_get_profile (from_profile),
		_LCMS2_PIXEL_FORMAT,
		(cmsHPROFILE) gth_icc_profile_get_profile (to_profile),
		_LCMS2_PIXEL_FORMAT,
		INTENT_PERCEPTUAL,
		0
	);
	if (cms_transform == NULL)
		return NULL;
	return gth_icc_transform_new (cms_transform);

#else

	return NULL;

#endif
}


GthCmsTransform
gth_icc_transform_get_transform (GthIccTransform *transform)
{
	g_return_val_if_fail (transform != NULL, NULL);
	return transform->priv->cms_transform;
}
