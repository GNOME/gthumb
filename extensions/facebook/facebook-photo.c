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
#include "facebook-photo.h"


static gpointer facebook_photo_parent_class = NULL;


static void
facebook_photo_finalize (GObject *obj)
{
	FacebookPhoto *self;

	self = FACEBOOK_PHOTO (obj);

	g_free (self->id);
	g_free (self->secret);
	g_free (self->server);
	g_free (self->title);

	G_OBJECT_CLASS (facebook_photo_parent_class)->finalize (obj);
}


static void
facebook_photo_class_init (FacebookPhotoClass *klass)
{
	facebook_photo_parent_class = g_type_class_peek_parent (klass);
	G_OBJECT_CLASS (klass)->finalize = facebook_photo_finalize;
}


static DomElement*
facebook_photo_create_element (DomDomizable *base,
				DomDocument  *doc)
{
	FacebookPhoto *self;
	DomElement  *element;

	self = FACEBOOK_PHOTO (base);

	element = dom_document_create_element (doc, "photo", NULL);
	if (self->id != NULL)
		dom_element_set_attribute (element, "id", self->id);
	if (self->secret != NULL)
		dom_element_set_attribute (element, "secret", self->secret);
	if (self->server != NULL)
		dom_element_set_attribute (element, "server", self->server);
	if (self->title != NULL)
		dom_element_set_attribute (element, "title", self->title);
	if (self->is_primary)
		dom_element_set_attribute (element, "isprimary", "1");

	return element;
}


static void
facebook_photo_load_from_element (DomDomizable *base,
				DomElement   *element)
{
	FacebookPhoto *self;

	if ((element == NULL) || (g_strcmp0 (element->tag_name, "photo") != 0))
		return;

	self = FACEBOOK_PHOTO (base);

	facebook_photo_set_id (self, dom_element_get_attribute (element, "id"));
	facebook_photo_set_secret (self, dom_element_get_attribute (element, "secret"));
	facebook_photo_set_server (self, dom_element_get_attribute (element, "server"));
	facebook_photo_set_title (self, dom_element_get_attribute (element, "title"));
	facebook_photo_set_is_primary (self, dom_element_get_attribute (element, "isprimary"));
	facebook_photo_set_url_sq (self, dom_element_get_attribute (element, "url_sq"));
	facebook_photo_set_url_t (self, dom_element_get_attribute (element, "url_t"));
	facebook_photo_set_url_s (self, dom_element_get_attribute (element, "url_s"));
	facebook_photo_set_url_m (self, dom_element_get_attribute (element, "url_m"));
	facebook_photo_set_url_o (self, dom_element_get_attribute (element, "url_o"));
}


static void
facebook_photo_dom_domizable_interface_init (DomDomizableIface *iface)
{
	iface->create_element = facebook_photo_create_element;
	iface->load_from_element = facebook_photo_load_from_element;
}


static void
facebook_photo_instance_init (FacebookPhoto *self)
{
}


GType
facebook_photo_get_type (void)
{
	static GType facebook_photo_type_id = 0;

	if (facebook_photo_type_id == 0) {
		static const GTypeInfo g_define_type_info = {
			sizeof (FacebookPhotoClass),
			(GBaseInitFunc) NULL,
			(GBaseFinalizeFunc) NULL,
			(GClassInitFunc) facebook_photo_class_init,
			(GClassFinalizeFunc) NULL,
			NULL,
			sizeof (FacebookPhoto),
			0,
			(GInstanceInitFunc) facebook_photo_instance_init,
			NULL
		};
		static const GInterfaceInfo dom_domizable_info = {
			(GInterfaceInitFunc) facebook_photo_dom_domizable_interface_init,
			(GInterfaceFinalizeFunc) NULL,
			NULL
		};

		facebook_photo_type_id = g_type_register_static (G_TYPE_OBJECT,
								  "FacebookPhoto",
								  &g_define_type_info,
								  0);
		g_type_add_interface_static (facebook_photo_type_id, DOM_TYPE_DOMIZABLE, &dom_domizable_info);
	}

	return facebook_photo_type_id;
}


FacebookPhoto *
facebook_photo_new (void)
{
	return g_object_new (FACEBOOK_TYPE_PHOTO, NULL);
}


void
facebook_photo_set_id (FacebookPhoto *self,
		     const char  *value)
{
	_g_strset (&self->id, value);
}


void
facebook_photo_set_secret (FacebookPhoto *self,
			 const char  *value)
{
	_g_strset (&self->secret, value);
}


void
facebook_photo_set_server (FacebookPhoto *self,
			 const char  *value)
{
	_g_strset (&self->server, value);
}


void
facebook_photo_set_title (FacebookPhoto *self,
			const char  *value)
{
	_g_strset (&self->title, value);
}


void
facebook_photo_set_is_primary (FacebookPhoto *self,
			     const char  *value)
{
	self->is_primary = (g_strcmp0 (value, "1") == 0);
}


void
facebook_photo_set_url_sq (FacebookPhoto *self,
			 const char  *value)
{
	_g_strset (&self->url_sq, value);
}


void
facebook_photo_set_url_t (FacebookPhoto *self,
			const char  *value)
{
	_g_strset (&self->url_t, value);
}


void
facebook_photo_set_url_s (FacebookPhoto *self,
			const char  *value)
{
	_g_strset (&self->url_s, value);
}


void
facebook_photo_set_url_m (FacebookPhoto *self,
			const char  *value)
{
	_g_strset (&self->url_m, value);
}


void
facebook_photo_set_url_o (FacebookPhoto *self,
			const char  *value)
{
	_g_strset (&self->url_o, value);
}


void
facebook_photo_set_original_format (FacebookPhoto *self,
				  const char  *value)
{
	_g_strset (&self->original_format, value);

	g_free (self->mime_type);
	self->mime_type = NULL;
	if (self->original_format != NULL)
		self->mime_type = g_strconcat ("image/", self->original_format, NULL);
}
