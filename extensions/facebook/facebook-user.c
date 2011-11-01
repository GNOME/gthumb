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
#include "facebook-user.h"


static void facebook_user_dom_domizable_interface_init (DomDomizableInterface *iface);


G_DEFINE_TYPE_WITH_CODE (FacebookUser,
			 facebook_user,
			 G_TYPE_OBJECT,
			 G_IMPLEMENT_INTERFACE (DOM_TYPE_DOMIZABLE,
					        facebook_user_dom_domizable_interface_init))


static void
facebook_user_finalize (GObject *obj)
{
	FacebookUser *self;

	self = FACEBOOK_USER (obj);

	g_free (self->id);
	g_free (self->username);

	G_OBJECT_CLASS (facebook_user_parent_class)->finalize (obj);
}


static void
facebook_user_class_init (FacebookUserClass *klass)
{
	facebook_user_parent_class = g_type_class_peek_parent (klass);
	G_OBJECT_CLASS (klass)->finalize = facebook_user_finalize;
}


static DomElement*
facebook_user_create_element (DomDomizable *base,
			      DomDocument  *doc)
{
	FacebookUser *self;
	DomElement *element;

	self = FACEBOOK_USER (base);

	element = dom_document_create_element (doc, "user", NULL);
	if (self->id != NULL)
		dom_element_set_attribute (element, "id", self->id);
	if (self->username != NULL)
		dom_element_set_attribute (element, "name", self->username);

	return element;
}


static void
facebook_user_load_from_element (DomDomizable *base,
			         DomElement   *element)
{
	FacebookUser *self;
	DomElement *node;

	self = FACEBOOK_USER (base);

	for (node = element->first_child; node; node = node->next_sibling) {
		if (g_strcmp0 (node->tag_name, "uid") == 0) {
			_g_strset (&self->id, dom_element_get_inner_text (node));
		}
		else if (g_strcmp0 (node->tag_name, "name") == 0) {
			_g_strset (&self->username, dom_element_get_inner_text (node));
		}
	}
}


static void
facebook_user_dom_domizable_interface_init (DomDomizableInterface *iface)
{
	iface->create_element = facebook_user_create_element;
	iface->load_from_element = facebook_user_load_from_element;
}


static void
facebook_user_init (FacebookUser *self)
{
	self->id = NULL;
	self->username = NULL;
}


FacebookUser *
facebook_user_new (void)
{
	return g_object_new (FACEBOOK_TYPE_USER, NULL);
}


void
facebook_user_set_id (FacebookUser *self,
		      const char   *value)
{
	_g_strset (&self->id, value);
}


void
facebook_user_set_username (FacebookUser *self,
			    const char   *value)
{
	_g_strset (&self->username, value);
}
