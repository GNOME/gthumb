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
#include "facebook-photoset.h"


static gpointer facebook_photoset_parent_class = NULL;


static void
facebook_photoset_finalize (GObject *obj)
{
	FacebookPhotoset *self;

	self = FACEBOOK_PHOTOSET (obj);

	g_free (self->id);
	g_free (self->title);
	g_free (self->description);
	g_free (self->primary);
	g_free (self->secret);
	g_free (self->server);
	g_free (self->farm);

	G_OBJECT_CLASS (facebook_photoset_parent_class)->finalize (obj);
}


static void
facebook_photoset_class_init (FacebookPhotosetClass *klass)
{
	facebook_photoset_parent_class = g_type_class_peek_parent (klass);
	G_OBJECT_CLASS (klass)->finalize = facebook_photoset_finalize;
}


static DomElement*
facebook_photoset_create_element (DomDomizable *base,
				DomDocument  *doc)
{
	FacebookPhotoset *self;
	DomElement     *element;
	char           *value;

	self = FACEBOOK_PHOTOSET (base);

	element = dom_document_create_element (doc, "photoset", NULL);
	if (self->id != NULL)
		dom_element_set_attribute (element, "id", self->id);
	if (self->primary != NULL)
		dom_element_set_attribute (element, "primary", self->primary);
	if (self->secret != NULL)
		dom_element_set_attribute (element, "secret", self->secret);
	if (self->server != NULL)
		dom_element_set_attribute (element, "server", self->server);
	if (self->n_photos >= 0) {
		value = g_strdup_printf ("%d", self->n_photos);
		dom_element_set_attribute (element, "photos", value);
		g_free (value);
	}
	if (self->farm != NULL)
		dom_element_set_attribute (element, "farm", self->farm);

	if (self->title != NULL)
		dom_element_append_child (element, dom_document_create_element_with_text (doc, self->title, "title", NULL));
	if (self->description != NULL)
		dom_element_append_child (element, dom_document_create_element_with_text (doc, self->description, "description", NULL));

	return element;
}


static void
facebook_photoset_load_from_element (DomDomizable *base,
				   DomElement   *element)
{
	FacebookPhotoset *self;
	DomElement     *node;

	self = FACEBOOK_PHOTOSET (base);

	facebook_photoset_set_id (self, dom_element_get_attribute (element, "id"));
	facebook_photoset_set_title (self, NULL);
	facebook_photoset_set_description (self, NULL);
	facebook_photoset_set_n_photos (self, dom_element_get_attribute (element, "photos"));
	facebook_photoset_set_primary (self, dom_element_get_attribute (element, "primary"));
	facebook_photoset_set_secret (self, dom_element_get_attribute (element, "secret"));
	facebook_photoset_set_server (self, dom_element_get_attribute (element, "server"));
	facebook_photoset_set_farm (self, dom_element_get_attribute (element, "farm"));
	facebook_photoset_set_url (self, dom_element_get_attribute (element, "url"));

	for (node = element->first_child; node; node = node->next_sibling) {
		if (g_strcmp0 (node->tag_name, "title") == 0) {
			facebook_photoset_set_title (self, dom_element_get_inner_text (node));
		}
		else if (g_strcmp0 (node->tag_name, "description") == 0) {
			facebook_photoset_set_description (self, dom_element_get_inner_text (node));
		}
	}
}


static void
facebook_photoset_dom_domizable_interface_init (DomDomizableIface *iface)
{
	iface->create_element = facebook_photoset_create_element;
	iface->load_from_element = facebook_photoset_load_from_element;
}


static void
facebook_photoset_instance_init (FacebookPhotoset *self)
{
	self->id = NULL;
	self->title = NULL;
	self->description = NULL;
	self->primary = NULL;
	self->secret = NULL;
	self->server = NULL;
	self->farm = NULL;
	self->url = NULL;
}


GType
facebook_photoset_get_type (void)
{
	static GType facebook_photoset_type_id = 0;

	if (facebook_photoset_type_id == 0) {
		static const GTypeInfo g_define_type_info = {
			sizeof (FacebookPhotosetClass),
			(GBaseInitFunc) NULL,
			(GBaseFinalizeFunc) NULL,
			(GClassInitFunc) facebook_photoset_class_init,
			(GClassFinalizeFunc) NULL,
			NULL,
			sizeof (FacebookPhotoset),
			0,
			(GInstanceInitFunc) facebook_photoset_instance_init,
			NULL
		};
		static const GInterfaceInfo dom_domizable_info = {
			(GInterfaceInitFunc) facebook_photoset_dom_domizable_interface_init,
			(GInterfaceFinalizeFunc) NULL,
			NULL
		};

		facebook_photoset_type_id = g_type_register_static (G_TYPE_OBJECT,
								   "FacebookPhotoset",
								   &g_define_type_info,
								   0);
		g_type_add_interface_static (facebook_photoset_type_id, DOM_TYPE_DOMIZABLE, &dom_domizable_info);
	}

	return facebook_photoset_type_id;
}


FacebookPhotoset *
facebook_photoset_new (void)
{
	return g_object_new (FACEBOOK_TYPE_PHOTOSET, NULL);
}


void
facebook_photoset_set_id (FacebookPhotoset *self,
			const char     *value)
{
	g_free (self->id);
	self->id = NULL;
	if (value != NULL)
		self->id = g_strdup (value);
}


void
facebook_photoset_set_title (FacebookPhotoset *self,
			   const char     *value)
{
	g_free (self->title);
	self->title = NULL;
	if (value != NULL)
		self->title = g_strdup (value);
}


void
facebook_photoset_set_description (FacebookPhotoset *self,
			         const char     *value)
{
	g_free (self->description);
	self->description = NULL;
	if (value != NULL)
		self->description = g_strdup (value);
}


void
facebook_photoset_set_n_photos (FacebookPhotoset *self,
			      const char     *value)
{
	if (value != NULL)
		self->n_photos = atoi (value);
	else
		self->n_photos = 0;
}


void
facebook_photoset_set_primary (FacebookPhotoset *self,
			     const char     *value)
{
	g_free (self->primary);
	self->primary = NULL;
	if (value != NULL)
		self->primary = g_strdup (value);
}


void
facebook_photoset_set_secret (FacebookPhotoset *self,
			    const char     *value)
{
	g_free (self->secret);
	self->secret = NULL;
	if (value != NULL)
		self->secret = g_strdup (value);
}


void
facebook_photoset_set_server (FacebookPhotoset *self,
			    const char     *value)
{
	g_free (self->server);
	self->server = NULL;
	if (value != NULL)
		self->server = g_strdup (value);
}


void
facebook_photoset_set_farm (FacebookPhotoset *self,
			  const char     *value)
{
	g_free (self->farm);
	self->farm = NULL;
	if (value != NULL)
		self->farm = g_strdup (value);
}


void
facebook_photoset_set_url (FacebookPhotoset *self,
			 const char     *value)
{
	g_free (self->url);
	self->url = NULL;
	if (value != NULL)
		self->url = g_strdup (value);
}
