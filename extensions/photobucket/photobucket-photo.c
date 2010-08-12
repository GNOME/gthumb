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
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <config.h>
#include <stdlib.h>
#include <string.h>
#include <gthumb.h>
#include "photobucket-photo.h"


static gpointer photobucket_photo_parent_class = NULL;


static void
photobucket_photo_finalize (GObject *obj)
{
	PhotobucketPhoto *self;

	self = PHOTOBUCKET_PHOTO (obj);

	g_free (self->name);
	g_free (self->browse_url);
	g_free (self->url);
	g_free (self->thumb_url);
	g_free (self->description);
	g_free (self->title);

	G_OBJECT_CLASS (photobucket_photo_parent_class)->finalize (obj);
}


static void
photobucket_photo_class_init (PhotobucketPhotoClass *klass)
{
	photobucket_photo_parent_class = g_type_class_peek_parent (klass);
	G_OBJECT_CLASS (klass)->finalize = photobucket_photo_finalize;
}


static DomElement*
photobucket_photo_create_element (DomDomizable *base,
				DomDocument  *doc)
{
	PhotobucketPhoto *self;
	DomElement  *element;

	self = PHOTOBUCKET_PHOTO (base);

	element = dom_document_create_element (doc, "photo", NULL);
	if (self->name != NULL)
		dom_element_set_attribute (element, "name", self->name);

	return element;
}


static void
photobucket_photo_load_from_element (DomDomizable *base,
				     DomElement   *element)
{
	PhotobucketPhoto *self;
	DomElement       *node;

	if ((element == NULL) || (g_strcmp0 (element->tag_name, "photo") != 0))
		return;

	self = PHOTOBUCKET_PHOTO (base);

	photobucket_photo_set_name (self, dom_element_get_attribute (element, "name"));
	photobucket_photo_set_is_public (self, dom_element_get_attribute (element, "public"));

	for (node = element->first_child; node; node = node->next_sibling) {
		if (g_strcmp0 (node->tag_name, "browse_url") == 0) {
			photobucket_photo_set_browse_url (self, dom_element_get_inner_text (node));
		}
		else if (g_strcmp0 (node->tag_name, "url") == 0) {
			photobucket_photo_set_url (self, dom_element_get_inner_text (node));
		}
		else if (g_strcmp0 (node->tag_name, "thumb_url") == 0) {
			photobucket_photo_set_thumb_url (self, dom_element_get_inner_text (node));
		}
		else if (g_strcmp0 (node->tag_name, "description") == 0) {
			photobucket_photo_set_description (self, dom_element_get_inner_text (node));
		}
		else if (g_strcmp0 (node->tag_name, "title") == 0) {
			photobucket_photo_set_title (self, dom_element_get_inner_text (node));
		}
		else if (g_strcmp0 (node->tag_name, "is_sponsored") == 0) {
			photobucket_photo_set_is_sponsored (self, dom_element_get_inner_text (node));
		}
	}
}


static void
photobucket_photo_dom_domizable_interface_init (DomDomizableIface *iface)
{
	iface->create_element = photobucket_photo_create_element;
	iface->load_from_element = photobucket_photo_load_from_element;
}


static void
photobucket_photo_instance_init (PhotobucketPhoto *self)
{
}


GType
photobucket_photo_get_type (void)
{
	static GType photobucket_photo_type_id = 0;

	if (photobucket_photo_type_id == 0) {
		static const GTypeInfo g_define_type_info = {
			sizeof (PhotobucketPhotoClass),
			(GBaseInitFunc) NULL,
			(GBaseFinalizeFunc) NULL,
			(GClassInitFunc) photobucket_photo_class_init,
			(GClassFinalizeFunc) NULL,
			NULL,
			sizeof (PhotobucketPhoto),
			0,
			(GInstanceInitFunc) photobucket_photo_instance_init,
			NULL
		};
		static const GInterfaceInfo dom_domizable_info = {
			(GInterfaceInitFunc) photobucket_photo_dom_domizable_interface_init,
			(GInterfaceFinalizeFunc) NULL,
			NULL
		};

		photobucket_photo_type_id = g_type_register_static (G_TYPE_OBJECT,
								    "PhotobucketPhoto",
								    &g_define_type_info,
								    0);
		g_type_add_interface_static (photobucket_photo_type_id, DOM_TYPE_DOMIZABLE, &dom_domizable_info);
	}

	return photobucket_photo_type_id;
}


PhotobucketPhoto *
photobucket_photo_new (void)
{
	return g_object_new (PHOTOBUCKET_TYPE_PHOTO, NULL);
}


void
photobucket_photo_set_name (PhotobucketPhoto *self,
			    const char       *value)
{
	_g_strset (&self->name, value);
}


void
photobucket_photo_set_is_public (PhotobucketPhoto *self,
			         const char       *value)
{
	self->is_public = (g_strcmp0 (value, "1") == 0);
}


void
photobucket_photo_set_browse_url (PhotobucketPhoto *self,
			          const char       *value)
{
	_g_strset (&self->name, value);
}


void
photobucket_photo_set_url (PhotobucketPhoto *self,
			   const char       *value)
{
	_g_strset (&self->name, value);
}


void
photobucket_photo_set_thumb_url (PhotobucketPhoto *self,
			         const char       *value)
{
	_g_strset (&self->name, value);
}


void
photobucket_photo_set_description (PhotobucketPhoto *self,
			           const char       *value)
{
	_g_strset (&self->name, value);
}


void
photobucket_photo_set_title (PhotobucketPhoto *self,
			     const char       *value)
{
	_g_strset (&self->name, value);
}


void
photobucket_photo_set_is_sponsored (PhotobucketPhoto *self,
			            const char       *value)
{
	self->is_sponsored = (g_strcmp0 (value, "1") == 0);
}
