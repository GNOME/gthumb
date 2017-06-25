/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2017 The Free Software Foundation, Inc.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <config.h>
#include <glib.h>
#include <glib-object.h>
#ifdef HAVE_LCMS2
#include <lcms2.h>
#endif /* HAVE_LCMS2 */
#include "gth-icc-profile.h"


struct _GthICCDataPrivate {
	GthICCProfile icc_profile;
	char *filename;
};


G_DEFINE_TYPE (GthICCData, gth_icc_data, G_TYPE_OBJECT)


static void
gth_icc_data_finalize (GObject *object)
{
	GthICCData *icc_data;

	g_return_if_fail (object != NULL);
	g_return_if_fail (GTH_IS_ICC_DATA (object));

	icc_data = GTH_ICC_DATA (object);
	gth_icc_profile_free (icc_data->priv->icc_profile);
	g_free (icc_data->priv->filename);

	/* Chain up */
	G_OBJECT_CLASS (gth_icc_data_parent_class)->finalize (object);
}


static void
gth_icc_data_class_init (GthICCDataClass *klass)
{
	GObjectClass *gobject_class;

	g_type_class_add_private (klass, sizeof (GthICCDataPrivate));

	gobject_class = (GObjectClass*) klass;
	gobject_class->finalize = gth_icc_data_finalize;
}


static void
gth_icc_data_init (GthICCData *self)
{
	self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, GTH_TYPE_ICC_DATA, GthICCDataPrivate);
	self->priv->icc_profile = NULL;
	self->priv->filename = NULL;
}


void
gth_icc_profile_free (GthICCProfile icc_profile)
{
#ifdef HAVE_LCMS2
	if (icc_profile != NULL)
		cmsCloseProfile ((cmsHPROFILE) icc_profile);
#endif
}


static char *
gth_icc_profile_get_id (GthICCProfile icc_profile)
{
	GString *str;

	str = g_string_new ("");

#ifdef HAVE_LCMS2
	{
		/* taken from colord:
		 * Copyright (C) 2013 Richard Hughes <richard@hughsie.com>
		 * Licensed under the GNU Lesser General Public License Version 2.1
		 * */
		cmsUInt8Number  icc_id[16];
		gboolean        md5_precooked = FALSE;
		gchar          *md5 = NULL;
		guint           i;

		cmsGetHeaderProfileID ((cmsHPROFILE) icc_profile, icc_id);
		for (i = 0; i < 16; i++) {
			if (icc_id[i] != 0) {
				md5_precooked = TRUE;
				break;
			}
		}
		if (md5_precooked) {
			/* convert to a hex string */
			md5 = g_new0 (gchar, 32 + 1);
			for (i = 0; i < 16; i++)
				g_snprintf (md5 + i * 2, 3, "%02x", icc_id[i]);
		}

		if (md5 != NULL) {
			g_string_append (str, "md5://");
			g_string_append (str, md5);
		}
	}
#endif

	return g_string_free (str, FALSE);
}

void
gth_icc_transform_free (GthICCTransform transform)
{
#ifdef HAVE_LCMS2
	if (transform != NULL)
		cmsDeleteTransform ((cmsHTRANSFORM) transform);
#endif
}


GthICCData *
gth_icc_data_new (const char    *filename,
		  GthICCProfile  profile)
{
	GthICCData *icc_data;

	icc_data = g_object_new (GTH_TYPE_ICC_DATA, NULL);
	if (filename != NULL)
		icc_data->priv->filename = g_strdup (filename);
	else
		icc_data->priv->filename = gth_icc_profile_get_id (profile);
	icc_data->priv->icc_profile = profile;

	return icc_data;
}


const char *
gth_icc_data_get_filename (GthICCData *self)
{
	g_return_val_if_fail (self != NULL, NULL);
	return self->priv->filename;
}


GthICCProfile
gth_icc_data_get_profile (GthICCData *self)
{
	g_return_val_if_fail (self != NULL, NULL);
	return self->priv->icc_profile;
}


gboolean
gth_icc_data_equal (GthICCData *a,
		    GthICCData *b)
{
	g_return_val_if_fail ((a == NULL) || (b == NULL), NULL);
	return g_strcmp0 (a->priv->filename, b->priv->filename) == 0;
}
