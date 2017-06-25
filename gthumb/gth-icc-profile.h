/* -*- Mode: C; tab-width: 8;	 indent-tabs-mode: t; c-basic-offset: 8 -*- */

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

#ifndef GTH_ICC_PROFILE_H
#define GTH_ICC_PROFILE_H

#include <glib.h>

G_BEGIN_DECLS

typedef gpointer GthICCProfile;
typedef gpointer GthICCTransform;

#define GTH_TYPE_ICC_DATA            (gth_icc_data_get_type ())
#define GTH_ICC_DATA(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTH_TYPE_ICC_DATA, GthICCData))
#define GTH_ICC_DATA_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GTH_TYPE_ICC_DATA, GthICCDataClass))
#define GTH_IS_ICC_DATA(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTH_TYPE_ICC_DATA))
#define GTH_IS_ICC_DATA_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GTH_TYPE_ICC_DATA))
#define GTH_ICC_DATA_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), GTH_TYPE_ICC_DATA, GthICCDataClass))

typedef struct _GthICCData         GthICCData;
typedef struct _GthICCDataPrivate  GthICCDataPrivate;
typedef struct _GthICCDataClass    GthICCDataClass;

struct _GthICCData {
	GObject __parent;
	GthICCDataPrivate *priv;
};

struct _GthICCDataClass {
	GObjectClass __parent_class;
};

void		gth_icc_profile_free		(GthICCProfile	 icc_profile);
void		gth_icc_transform_free		(GthICCTransform transform);

GType		gth_icc_data_get_type		(void);
GthICCData * 	gth_icc_data_new		(const char	*filename,
						 GthICCProfile	 profile);
const char *	gth_icc_data_get_filename	(GthICCData	*icc_data);
GthICCProfile	gth_icc_data_get_profile	(GthICCData	*icc_data);
gboolean	gth_icc_data_equal		(GthICCData	*a,
		    	    	    	    	 GthICCData	*b);

G_END_DECLS

#endif /* GTH_ICC_PROFILE_H */
