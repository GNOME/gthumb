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
#include "picasa-web-photo.h"


static gpointer picasa_web_photo_parent_class = NULL;


static void
picasa_web_photo_finalize (GObject *obj)
{
	PicasaWebPhoto *self;

	self = PICASA_WEB_PHOTO (obj);

	g_free (self->etag);
	g_free (self->id);
	g_free (self->album_id);
	g_free (self->title);
	g_free (self->summary);
	g_free (self->uri);
	g_free (self->keywords);

	G_OBJECT_CLASS (picasa_web_photo_parent_class)->finalize (obj);
}


static void
picasa_web_photo_class_init (PicasaWebPhotoClass *klass)
{
	picasa_web_photo_parent_class = g_type_class_peek_parent (klass);
	G_OBJECT_CLASS (klass)->finalize = picasa_web_photo_finalize;
}


static DomElement*
picasa_web_photo_create_element (DomDomizable *base,
				DomDocument  *doc)
{
	PicasaWebPhoto *self;
	DomElement     *element;
	char           *value;

	self = PICASA_WEB_PHOTO (base);

	element = dom_document_create_element (doc, "entry",
					       "xmlns", "http://www.w3.org/2005/Atom",
					       "xmlns:media", "http://search.yahoo.com/mrss/",
					       "xmlns:gphoto", "http://schemas.google.com/photos/2007",
					       NULL);
	if (self->id != NULL)
		dom_element_append_child (element, dom_document_create_element_with_text (doc, self->id, "gphoto:id", NULL));
	if (self->album_id != NULL)
		dom_element_append_child (element, dom_document_create_element_with_text (doc, self->id, "gphoto:albumid", NULL));
	if (self->title != NULL)
		dom_element_append_child (element, dom_document_create_element_with_text (doc, self->id, "title", NULL));
	if (self->summary != NULL)
		dom_element_append_child (element, dom_document_create_element_with_text (doc, self->id, "summary", NULL));
	if (self->uri != NULL)
		dom_element_append_child (element, dom_document_create_element (doc, "content", "src", self->uri, NULL));

	switch (self->access) {
	case PICASA_WEB_ACCESS_ALL:
		value = "all";
		break;
	case PICASA_WEB_ACCESS_PRIVATE:
		value = "private";
		break;
	case PICASA_WEB_ACCESS_PUBLIC:
		value = "public";
		break;
	case PICASA_WEB_ACCESS_VISIBLE:
		value = "visible";
		break;
	}
	dom_element_append_child (element, dom_document_create_element_with_text (doc, value, "gphoto:access", NULL));

	if (self->keywords != NULL) {
		DomElement *group;

		group = dom_document_create_element (doc, "media:group", NULL);
		dom_element_append_child (group, dom_document_create_element_with_text (doc, self->keywords, "media:keywords", NULL));
		dom_element_append_child (element, group);
	}

	dom_element_append_child (element,
				  dom_document_create_element (doc, "category",
							       "scheme", "http://schemas.google.com/g/2005#kind",
							       "term", "http://schemas.google.com/photos/2007#photo",
							       NULL));

	return element;
}


static void
picasa_web_photo_load_from_element (DomDomizable *base,
				   DomElement   *element)
{
	PicasaWebPhoto *self;
	DomElement     *node;

	self = PICASA_WEB_PHOTO (base);

	picasa_web_photo_set_id (self, NULL);
	picasa_web_photo_set_album_id (self, NULL);
	picasa_web_photo_set_title (self, NULL);
	picasa_web_photo_set_summary (self, NULL);
	picasa_web_photo_set_uri (self, NULL);
	picasa_web_photo_set_access (self, NULL);
	picasa_web_photo_set_keywords (self, NULL);

	picasa_web_photo_set_etag (self, dom_element_get_attribute (element, "gd:etag"));
	for (node = element->first_child; node; node = node->next_sibling) {
		if (g_strcmp0 (node->tag_name, "gphoto:id") == 0) {
			picasa_web_photo_set_id (self, dom_element_get_inner_text (node));
		}
		else if (g_strcmp0 (node->tag_name, "gphoto:albumid") == 0) {
			picasa_web_photo_set_album_id (self, dom_element_get_inner_text (node));
		}
		else if (g_strcmp0 (node->tag_name, "title") == 0) {
			picasa_web_photo_set_title (self, dom_element_get_inner_text (node));
		}
		else if (g_strcmp0 (node->tag_name, "summary") == 0) {
			picasa_web_photo_set_summary (self, dom_element_get_inner_text (node));
		}
		else if (g_strcmp0 (node->tag_name, "content") == 0) {
			picasa_web_photo_set_uri (self, dom_element_get_attribute (node, "src"));
		}
		else if (g_strcmp0 (node->tag_name, "gphoto:access") == 0) {
			picasa_web_photo_set_access (self, dom_element_get_inner_text (node));
		}
		else if (g_strcmp0 (node->tag_name, "media:group") == 0) {
			DomElement *child;

			for (child = node->first_child; child; child = child->next_sibling) {
				if (g_strcmp0 (child->tag_name, "media:keywords") == 0) {
					picasa_web_photo_set_keywords (self, dom_element_get_inner_text (child));
					break;
				}
			}
		}
	}
}


static void
picasa_web_photo_dom_domizable_interface_init (DomDomizableIface *iface)
{
	iface->create_element = picasa_web_photo_create_element;
	iface->load_from_element = picasa_web_photo_load_from_element;
}


static void
picasa_web_photo_instance_init (PicasaWebPhoto *self)
{
}


GType
picasa_web_photo_get_type (void)
{
	static GType picasa_web_photo_type_id = 0;

	if (picasa_web_photo_type_id == 0) {
		static const GTypeInfo g_define_type_info = {
			sizeof (PicasaWebPhotoClass),
			(GBaseInitFunc) NULL,
			(GBaseFinalizeFunc) NULL,
			(GClassInitFunc) picasa_web_photo_class_init,
			(GClassFinalizeFunc) NULL,
			NULL,
			sizeof (PicasaWebPhoto),
			0,
			(GInstanceInitFunc) picasa_web_photo_instance_init,
			NULL
		};
		static const GInterfaceInfo dom_domizable_info = {
			(GInterfaceInitFunc) picasa_web_photo_dom_domizable_interface_init,
			(GInterfaceFinalizeFunc) NULL,
			NULL
		};

		picasa_web_photo_type_id = g_type_register_static (G_TYPE_OBJECT,
								  "PicasaWebPhoto",
								  &g_define_type_info,
								  0);
		g_type_add_interface_static (picasa_web_photo_type_id, DOM_TYPE_DOMIZABLE, &dom_domizable_info);
	}

	return picasa_web_photo_type_id;
}


PicasaWebPhoto *
picasa_web_photo_new (void)
{
	return g_object_new (PICASA_WEB_TYPE_PHOTO, NULL);
}


void
picasa_web_photo_set_etag (PicasaWebPhoto *self,
			   const char    *value)
{
	g_free (self->etag);
	self->etag = NULL;
	if (value != NULL)
		self->etag = g_strdup (value);
}


void
picasa_web_photo_set_id (PicasaWebPhoto *self,
			const char    *value)
{
	g_free (self->id);
	self->id = NULL;
	if (value != NULL)
		self->id = g_strdup (value);
}


void
picasa_web_photo_set_album_id (PicasaWebPhoto *self,
			       const char    *value)
{
	g_free (self->album_id);
	self->album_id = NULL;
	if (value != NULL)
		self->album_id = g_strdup (value);
}


void
picasa_web_photo_set_title (PicasaWebPhoto *self,
			    const char     *value)
{
	g_free (self->title);
	self->title = NULL;
	if (value != NULL)
		self->title = g_strdup (value);
}


void
picasa_web_photo_set_summary (PicasaWebPhoto *self,
			      const char     *value)
{
	g_free (self->summary);
	self->summary = NULL;
	if (value != NULL)
		self->summary = g_strdup (value);
}


void
picasa_web_photo_set_uri (PicasaWebPhoto *self,
			  const char     *value)
{
	g_free (self->uri);
	self->uri = NULL;
	if (value != NULL)
		self->uri = g_strdup (value);
}


void
picasa_web_photo_set_access (PicasaWebPhoto *self,
			     const char     *value)
{
	if (value == NULL)
		self->access = PICASA_WEB_ACCESS_PRIVATE;
	else if (strcmp (value, "all") == 0)
		self->access = PICASA_WEB_ACCESS_ALL;
	else if (strcmp (value, "private") == 0)
		self->access = PICASA_WEB_ACCESS_PRIVATE;
	else if (strcmp (value, "public") == 0)
		self->access = PICASA_WEB_ACCESS_PUBLIC;
	else if (strcmp (value, "visible") == 0)
		self->access = PICASA_WEB_ACCESS_VISIBLE;
	else
		self->access = PICASA_WEB_ACCESS_PRIVATE;
}


void
picasa_web_photo_set_keywords (PicasaWebPhoto *self,
			       const char     *value)
{
	g_free (self->keywords);
	self->keywords = NULL;
	if (value != NULL)
		self->keywords = g_strdup (value);
}
