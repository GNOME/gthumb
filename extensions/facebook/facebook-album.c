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
#include "facebook-album.h"


static gpointer facebook_album_parent_class = NULL;


static void
facebook_album_finalize (GObject *obj)
{
	FacebookAlbum *self;

	self = FACEBOOK_ALBUM (obj);

	g_free (self->id);
	g_free (self->name);
	g_free (self->description);
	g_free (self->location);
	g_free (self->link);

	G_OBJECT_CLASS (facebook_album_parent_class)->finalize (obj);
}


static void
facebook_album_class_init (FacebookAlbumClass *klass)
{
	facebook_album_parent_class = g_type_class_peek_parent (klass);
	G_OBJECT_CLASS (klass)->finalize = facebook_album_finalize;
}


static DomElement*
facebook_album_create_element (DomDomizable *base,
			       DomDocument  *doc)
{
	FacebookAlbum *self;
	DomElement    *element;

	self = FACEBOOK_ALBUM (base);

	element = dom_document_create_element (doc, "photoset", NULL);
	if (self->id != NULL)
		dom_element_set_attribute (element, "aid", self->id);
	if (self->name != NULL)
		dom_element_append_child (element, dom_document_create_element_with_text (doc, self->name, "name", NULL));
	if (self->description != NULL)
		dom_element_append_child (element, dom_document_create_element_with_text (doc, self->description, "description", NULL));

	return element;
}


static FacebookVisibility
get_visibility_by_name (const char *name)
{
	if (name == NULL)
		return FACEBOOK_VISIBILITY_EVERYONE;
	if (g_strcmp0 (name, "everyone") == 0)
		return FACEBOOK_VISIBILITY_EVERYONE;
	if (g_strcmp0 (name, "networks_friends") == 0)
		return FACEBOOK_VISIBILITY_NETWORKS_FRIENDS;
	if (g_strcmp0 (name, "friends_of_friends") == 0)
		return FACEBOOK_VISIBILITY_FRIENDS_OF_FRIENDS;
	if (g_strcmp0 (name, "all_friends") == 0)
		return FACEBOOK_VISIBILITY_ALL_FRIENDS;
	if (g_strcmp0 (name, "self") == 0)
		return FACEBOOK_VISIBILITY_SELF;
	if (g_strcmp0 (name, "custom") == 0)
		return FACEBOOK_VISIBILITY_CUSTOM;

	return FACEBOOK_VISIBILITY_EVERYONE;
}


static void
facebook_album_load_from_element (DomDomizable *base,
				   DomElement   *element)
{
	FacebookAlbum *self;
	DomElement     *node;

	self = FACEBOOK_ALBUM (base);

	_g_strset (&self->id, NULL);
	_g_strset (&self->name, NULL);
	_g_strset (&self->description, NULL);
	_g_strset (&self->location, NULL);
	_g_strset (&self->link, NULL);
	self->size = 0;
	self->visibility = FACEBOOK_VISIBILITY_SELF;

	for (node = element->first_child; node; node = node->next_sibling) {
		if (g_strcmp0 (node->tag_name, "aid") == 0) {
			_g_strset (&self->id, dom_element_get_inner_text (node));
		}
		else if (g_strcmp0 (node->tag_name, "name") == 0) {
			_g_strset (&self->name, dom_element_get_inner_text (node));
		}
		else if (g_strcmp0 (node->tag_name, "description") == 0) {
			_g_strset (&self->description, dom_element_get_inner_text (node));
		}
		else if (g_strcmp0 (node->tag_name, "location") == 0) {
			_g_strset (&self->location, dom_element_get_inner_text (node));
		}
		else if (g_strcmp0 (node->tag_name, "link") == 0) {
			_g_strset (&self->link, dom_element_get_inner_text (node));
		}
		else if (g_strcmp0 (node->tag_name, "size") == 0) {
			self->size = atoi (dom_element_get_inner_text (node));
		}
		else if (g_strcmp0 (node->tag_name, "visible") == 0) {
			self->visibility = get_visibility_by_name (dom_element_get_inner_text (node));
		}
	}
}


static void
facebook_album_dom_domizable_interface_init (DomDomizableIface *iface)
{
	iface->create_element = facebook_album_create_element;
	iface->load_from_element = facebook_album_load_from_element;
}


static void
facebook_album_instance_init (FacebookAlbum *self)
{
	self->id = NULL;
	self->name = NULL;
	self->description = NULL;
	self->location = NULL;
	self->link = NULL;
	self->size = 0;
	self->visibility = FACEBOOK_VISIBILITY_SELF;
}


GType
facebook_album_get_type (void)
{
	static GType facebook_album_type_id = 0;

	if (facebook_album_type_id == 0) {
		static const GTypeInfo g_define_type_info = {
			sizeof (FacebookAlbumClass),
			(GBaseInitFunc) NULL,
			(GBaseFinalizeFunc) NULL,
			(GClassInitFunc) facebook_album_class_init,
			(GClassFinalizeFunc) NULL,
			NULL,
			sizeof (FacebookAlbum),
			0,
			(GInstanceInitFunc) facebook_album_instance_init,
			NULL
		};
		static const GInterfaceInfo dom_domizable_info = {
			(GInterfaceInitFunc) facebook_album_dom_domizable_interface_init,
			(GInterfaceFinalizeFunc) NULL,
			NULL
		};

		facebook_album_type_id = g_type_register_static (G_TYPE_OBJECT,
								 "FacebookAlbum",
								 &g_define_type_info,
								 0);
		g_type_add_interface_static (facebook_album_type_id, DOM_TYPE_DOMIZABLE, &dom_domizable_info);
	}

	return facebook_album_type_id;
}


FacebookAlbum *
facebook_album_new (void)
{
	return g_object_new (FACEBOOK_TYPE_ALBUM, NULL);
}


void
facebook_album_set_name (FacebookAlbum *self,
			 const char    *value)
{
	_g_strset (&self->name, value);
}


void
facebook_album_set_location (FacebookAlbum *self,
			     const char    *value)
{
	_g_strset (&self->location, value);
}


void
facebook_album_set_description (FacebookAlbum *self,
				const char    *value)
{
	_g_strset (&self->description, value);
}
