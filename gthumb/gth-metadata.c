/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2009 Free Software Foundation, Inc.
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
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Street #330, Boston, MA 02111-1307, USA.
 */

#include <config.h>
#include "glib-utils.h"
#include "gth-metadata.h"


enum  {
	GTH_METADATA_DUMMY_PROPERTY,
	GTH_METADATA_ID,
	GTH_METADATA_DESCRIPTION,
	GTH_METADATA_RAW,
	GTH_METADATA_FORMATTED
};

struct _GthMetadataPrivate {
	char *id;
	char *description;
	char *raw;
	char *formatted;
};

static gpointer gth_metadata_parent_class = NULL;


static void
gth_metadata_get_property (GObject    *object,
			   guint       property_id,
			   GValue     *value,
			   GParamSpec *pspec)
{
	GthMetadata *self;

	self = GTH_METADATA (object);
	switch (property_id) {
	case GTH_METADATA_ID:
		g_value_set_string (value, self->priv->id);
		break;
	case GTH_METADATA_DESCRIPTION:
		g_value_set_string (value, self->priv->description);
		break;
	case GTH_METADATA_RAW:
		g_value_set_string (value, self->priv->raw);
		break;
	case GTH_METADATA_FORMATTED:
		g_value_set_string (value, self->priv->formatted);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
		break;
	}
}


static void
gth_metadata_set_property (GObject      *object,
			   guint         property_id,
			   const GValue *value,
			   GParamSpec   *pspec)
{
	GthMetadata *self;

	self = GTH_METADATA (object);
	switch (property_id) {
	case GTH_METADATA_ID:
		_g_strset (&self->priv->id, g_value_get_string (value));
		break;
	case GTH_METADATA_DESCRIPTION:
		_g_strset (&self->priv->description, g_value_get_string (value));
		break;
	case GTH_METADATA_RAW:
		_g_strset (&self->priv->raw, g_value_get_string (value));
		break;
	case GTH_METADATA_FORMATTED:
		_g_strset (&self->priv->formatted, g_value_get_string (value));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
		break;
	}
}


static void
gth_metadata_finalize (GObject *obj)
{
	GthMetadata *self;

	self = GTH_METADATA (obj);

	g_free (self->priv->id);
	g_free (self->priv->description);
	g_free (self->priv->raw);
	g_free (self->priv->formatted);

	G_OBJECT_CLASS (gth_metadata_parent_class)->finalize (obj);
}


static void
gth_metadata_class_init (GthMetadataClass *klass)
{
	gth_metadata_parent_class = g_type_class_peek_parent (klass);
	g_type_class_add_private (klass, sizeof (GthMetadataPrivate));

	G_OBJECT_CLASS (klass)->get_property = gth_metadata_get_property;
	G_OBJECT_CLASS (klass)->set_property = gth_metadata_set_property;
	G_OBJECT_CLASS (klass)->finalize = gth_metadata_finalize;

	g_object_class_install_property (G_OBJECT_CLASS (klass),
					 GTH_METADATA_ID,
					 g_param_spec_string ("id",
					 		      "ID",
					 		      "Metadata unique identifier",
					 		      NULL,
					 		      G_PARAM_STATIC_NAME | G_PARAM_STATIC_NICK | G_PARAM_STATIC_BLURB | G_PARAM_READABLE | G_PARAM_WRITABLE));
	g_object_class_install_property (G_OBJECT_CLASS (klass),
					 GTH_METADATA_DESCRIPTION,
					 g_param_spec_string ("description",
					 		      "Description",
					 		      "Metadata description",
					 		      NULL,
					 		      G_PARAM_STATIC_NAME | G_PARAM_STATIC_NICK | G_PARAM_STATIC_BLURB | G_PARAM_READABLE | G_PARAM_WRITABLE));
	g_object_class_install_property (G_OBJECT_CLASS (klass),
					 GTH_METADATA_RAW,
					 g_param_spec_string ("raw",
					 		      "Raw value",
					 		      "Metadata raw value",
					 		      NULL,
					 		      G_PARAM_STATIC_NAME | G_PARAM_STATIC_NICK | G_PARAM_STATIC_BLURB | G_PARAM_READABLE | G_PARAM_WRITABLE));
	g_object_class_install_property (G_OBJECT_CLASS (klass),
					 GTH_METADATA_FORMATTED,
					 g_param_spec_string ("formatted",
					 		      "Formatted value",
					 		      "Metadata formatted value",
					 		      NULL,
					 		      G_PARAM_STATIC_NAME | G_PARAM_STATIC_NICK | G_PARAM_STATIC_BLURB | G_PARAM_READABLE | G_PARAM_WRITABLE));
}


static void
gth_metadata_instance_init (GthMetadata *self)
{
	self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, GTH_TYPE_METADATA, GthMetadataPrivate);
	self->priv->id = NULL;
	self->priv->description = NULL;
	self->priv->raw = NULL;
	self->priv->formatted = NULL;
}


GType
gth_metadata_get_type (void)
{
	static GType gth_metadata_type_id = 0;

	if (gth_metadata_type_id == 0) {
		static const GTypeInfo g_define_type_info = {
			sizeof (GthMetadataClass),
			(GBaseInitFunc) NULL,
			(GBaseFinalizeFunc) NULL,
			(GClassInitFunc) gth_metadata_class_init,
			(GClassFinalizeFunc) NULL,
			NULL,
			sizeof (GthMetadata),
			0,
			(GInstanceInitFunc) gth_metadata_instance_init,
			NULL
		};
		gth_metadata_type_id = g_type_register_static (G_TYPE_OBJECT, "GthMetadata", &g_define_type_info, 0);
	}
	return gth_metadata_type_id;
}


GthMetadata *
gth_metadata_new (void)
{
	return g_object_new (GTH_TYPE_METADATA, NULL);
}


const char *
gth_metadata_get_raw (GthMetadata *metadata)
{
	return metadata->priv->raw;
}


const char *
gth_metadata_get_formatted (GthMetadata *metadata)
{
	return metadata->priv->formatted;
}


GthMetadataInfo *
gth_metadata_info_dup (GthMetadataInfo *info)
{
	GthMetadataInfo *new_info;

	new_info = g_new0 (GthMetadataInfo, 1);
	if (info->id != NULL)
		new_info->id = g_strdup (info->id);
	if (info->type != NULL)
		new_info->type = g_strdup (info->type);
	if (info->display_name != NULL)
		new_info->display_name = g_strdup (info->display_name);
	if (info->category != NULL)
		new_info->category = g_strdup (info->category);
	new_info->sort_order = info->sort_order;
	new_info->flags = info->flags;

	return new_info;
}


void
set_attribute_from_string (GFileInfo  *info,
			   const char *key,
			   const char *raw,
			   const char *formatted)
{
	GthMetadata *metadata;

	metadata = g_object_new (GTH_TYPE_METADATA,
				 "id", key,
				 "raw", raw,
				 "formatted", (formatted != NULL ? formatted : raw),
				 NULL);
	g_file_info_set_attribute_object (info, key, G_OBJECT (metadata));

	g_object_unref (metadata);
}
