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
#include "picasa-web-album.h"


static gpointer picasa_web_album_parent_class = NULL;


static void
picasa_web_album_finalize (GObject *obj)
{
	PicasaWebAlbum *self;

	self = PICASA_WEB_ALBUM (obj);

	g_free (self->id);
	g_free (self->title);
	g_free (self->summary);
	g_free (self->edit_url);

	G_OBJECT_CLASS (picasa_web_album_parent_class)->finalize (obj);
}


static void
picasa_web_album_class_init (PicasaWebAlbumClass *klass)
{
	picasa_web_album_parent_class = g_type_class_peek_parent (klass);
	G_OBJECT_CLASS (klass)->finalize = picasa_web_album_finalize;
}


static DomElement*
picasa_web_album_create_element (DomDomizable *base,
				 DomDocument  *doc)
{
	PicasaWebAlbum *self;
	DomElement     *element;
	char           *value;

	self = PICASA_WEB_ALBUM (base);

	element = dom_document_create_element (doc, "entry",
					       "xmlns", "http://www.w3.org/2005/Atom",
					       "xmlns:media", "http://search.yahoo.com/mrss/",
					       "xmlns:gphoto", "http://schemas.google.com/photos/2007",
					       NULL);
	if (self->id != NULL)
		dom_element_append_child (element, dom_document_create_element_with_text (doc, self->id, "id", NULL));
	if (self->title != NULL)
		dom_element_append_child (element, dom_document_create_element_with_text (doc, self->title, "title", "type", "text", NULL));
	if (self->summary != NULL)
		dom_element_append_child (element, dom_document_create_element_with_text (doc, self->summary, "summary", "type", "text", NULL));

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

	dom_element_append_child (element,
				  dom_document_create_element (doc, "category",
							       "scheme", "http://schemas.google.com/g/2005#kind",
							       "term", "http://schemas.google.com/photos/2007#album",
							       NULL));

	return element;
}


static void
picasa_web_album_load_from_element (DomDomizable *base,
				    DomElement   *element)
{
	PicasaWebAlbum *self;
	DomElement     *node;

	self = PICASA_WEB_ALBUM (base);

	picasa_web_album_set_id (self, NULL);
	picasa_web_album_set_title (self, NULL);
	picasa_web_album_set_summary (self, NULL);
	picasa_web_album_set_edit_url (self, NULL);
	picasa_web_album_set_access (self, NULL);
	self->n_photos = 0;

	picasa_web_album_set_etag (self, dom_element_get_attribute (element, "gd:etag"));
	for (node = element->first_child; node; node = node->next_sibling) {
		if (g_strcmp0 (node->tag_name, "gphoto:id") == 0) {
			picasa_web_album_set_id (self, dom_element_get_inner_text (node));
		}
		else if (g_strcmp0 (node->tag_name, "title") == 0) {
			picasa_web_album_set_title (self, dom_element_get_inner_text (node));
		}
		else if (g_strcmp0 (node->tag_name, "summary") == 0) {
			picasa_web_album_set_summary (self, dom_element_get_inner_text (node));
		}
		else if (g_strcmp0 (node->tag_name, "link") == 0) {
			if (g_strcmp0 (dom_element_get_attribute (node, "rel"), "edit") == 0)
				picasa_web_album_set_edit_url (self, dom_element_get_attribute (node, "href"));
		}
		else if (g_strcmp0 (node->tag_name, "gphoto:access") == 0) {
			picasa_web_album_set_access (self, dom_element_get_inner_text (node));
		}
		else if (g_strcmp0 (node->tag_name, "gphoto:numphotos") == 0) {
			picasa_web_album_set_n_photos (self, dom_element_get_inner_text (node));
		}
	}
}


static void
picasa_web_album_dom_domizable_interface_init (DomDomizableIface *iface)
{
	iface->create_element = picasa_web_album_create_element;
	iface->load_from_element = picasa_web_album_load_from_element;
}


static void
picasa_web_album_instance_init (PicasaWebAlbum *self)
{
}


GType
picasa_web_album_get_type (void)
{
	static GType picasa_web_album_type_id = 0;

	if (picasa_web_album_type_id == 0) {
		static const GTypeInfo g_define_type_info = {
			sizeof (PicasaWebAlbumClass),
			(GBaseInitFunc) NULL,
			(GBaseFinalizeFunc) NULL,
			(GClassInitFunc) picasa_web_album_class_init,
			(GClassFinalizeFunc) NULL,
			NULL,
			sizeof (PicasaWebAlbum),
			0,
			(GInstanceInitFunc) picasa_web_album_instance_init,
			NULL
		};
		static const GInterfaceInfo dom_domizable_info = {
			(GInterfaceInitFunc) picasa_web_album_dom_domizable_interface_init,
			(GInterfaceFinalizeFunc) NULL,
			NULL
		};

		picasa_web_album_type_id = g_type_register_static (G_TYPE_OBJECT,
								   "PicasaWebAlbum",
								   &g_define_type_info,
								   0);
		g_type_add_interface_static (picasa_web_album_type_id, DOM_TYPE_DOMIZABLE, &dom_domizable_info);
	}

	return picasa_web_album_type_id;
}


PicasaWebAlbum *
picasa_web_album_new (void)
{
	return g_object_new (PICASA_WEB_TYPE_ALBUM, NULL);
}


void
picasa_web_album_set_etag (PicasaWebAlbum *self,
			   const char     *value)
{
	g_free (self->etag);
	self->etag = NULL;
	if (value != NULL)
		self->etag = g_strdup (value);
}


void
picasa_web_album_set_id (PicasaWebAlbum *self,
			 const char     *value)
{
	g_free (self->id);
	self->id = NULL;
	if (value != NULL)
		self->id = g_strdup (value);
}


void
picasa_web_album_set_title (PicasaWebAlbum *self,
			    const char     *value)
{
	g_free (self->title);
	self->title = NULL;
	if (value != NULL)
		self->title = g_strdup (value);
}


void
picasa_web_album_set_summary (PicasaWebAlbum *self,
			      const char     *value)
{
	g_free (self->summary);
	self->summary = NULL;
	if (value != NULL)
		self->summary = g_strdup (value);
}


void
picasa_web_album_set_edit_url (PicasaWebAlbum *self,
			       const char     *value)
{
	g_free (self->edit_url);
	self->edit_url = NULL;
	if (value != NULL)
		self->edit_url = g_strdup (value);
}


void
picasa_web_album_set_access (PicasaWebAlbum *self,
			     const char     *value)
{
	if (value == NULL)
		self->access = PICASA_WEB_ACCESS_PRIVATE;
	else if (strcmp (value, "all"))
		self->access = PICASA_WEB_ACCESS_ALL;
	else if (strcmp (value, "private"))
		self->access = PICASA_WEB_ACCESS_PRIVATE;
	else if (strcmp (value, "public"))
		self->access = PICASA_WEB_ACCESS_PUBLIC;
	else if (strcmp (value, "visible"))
		self->access = PICASA_WEB_ACCESS_VISIBLE;
	else
		self->access = PICASA_WEB_ACCESS_PRIVATE;
}


void
picasa_web_album_set_n_photos (PicasaWebAlbum *self,
			       const char     *value)
{
	if (value != NULL)
		self->n_photos = atoi (value);
	else
		self->n_photos = 0;
}
