/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2010 Free Software Foundation, Inc.
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
#include <stdlib.h>
#include <string.h>
#include <gthumb.h>
#include "photobucket-album.h"


static gpointer photobucket_album_parent_class = NULL;


static void
photobucket_album_finalize (GObject *obj)
{
	PhotobucketAlbum *self;

	self = PHOTOBUCKET_ALBUM (obj);

	g_free (self->name);

	G_OBJECT_CLASS (photobucket_album_parent_class)->finalize (obj);
}


static void
photobucket_album_class_init (PhotobucketAlbumClass *klass)
{
	photobucket_album_parent_class = g_type_class_peek_parent (klass);
	G_OBJECT_CLASS (klass)->finalize = photobucket_album_finalize;
}


static DomElement*
photobucket_album_create_element (DomDomizable *base,
			          DomDocument  *doc)
{
	PhotobucketAlbum *self;
	DomElement    *element;

	self = PHOTOBUCKET_ALBUM (base);

	element = dom_document_create_element (doc, "photoset", NULL);
	if (self->name != NULL)
		dom_element_append_child (element, dom_document_create_element_with_text (doc, self->name, "name", NULL));

	return element;
}


static void
photobucket_album_load_from_element (DomDomizable *base,
				     DomElement   *element)
{
	PhotobucketAlbum *self;

	self = PHOTOBUCKET_ALBUM (base);

	photobucket_album_set_name (self, dom_element_get_attribute (element, "name"));
	self->photo_count = atoi (dom_element_get_attribute (element, "photo_count"));
	self->video_count = atoi (dom_element_get_attribute (element, "video_count"));
}


static void
photobucket_album_dom_domizable_interface_init (DomDomizableIface *iface)
{
	iface->create_element = photobucket_album_create_element;
	iface->load_from_element = photobucket_album_load_from_element;
}


static void
photobucket_album_instance_init (PhotobucketAlbum *self)
{
	self->name = NULL;
	self->photo_count = 0;
	self->video_count = 0;
}


GType
photobucket_album_get_type (void)
{
	static GType photobucket_album_type_id = 0;

	if (photobucket_album_type_id == 0) {
		static const GTypeInfo g_define_type_info = {
			sizeof (PhotobucketAlbumClass),
			(GBaseInitFunc) NULL,
			(GBaseFinalizeFunc) NULL,
			(GClassInitFunc) photobucket_album_class_init,
			(GClassFinalizeFunc) NULL,
			NULL,
			sizeof (PhotobucketAlbum),
			0,
			(GInstanceInitFunc) photobucket_album_instance_init,
			NULL
		};
		static const GInterfaceInfo dom_domizable_info = {
			(GInterfaceInitFunc) photobucket_album_dom_domizable_interface_init,
			(GInterfaceFinalizeFunc) NULL,
			NULL
		};

		photobucket_album_type_id = g_type_register_static (G_TYPE_OBJECT,
								 "PhotobucketAlbum",
								 &g_define_type_info,
								 0);
		g_type_add_interface_static (photobucket_album_type_id, DOM_TYPE_DOMIZABLE, &dom_domizable_info);
	}

	return photobucket_album_type_id;
}


PhotobucketAlbum *
photobucket_album_new (void)
{
	return g_object_new (PHOTOBUCKET_TYPE_ALBUM, NULL);
}


void
photobucket_album_set_name (PhotobucketAlbum *self,
			    const char       *value)
{
	_g_strset (&self->name, value);
}
