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
#include "photobucket-account.h"


static gpointer photobucket_account_parent_class = NULL;


static void
photobucket_account_finalize (GObject *obj)
{
	PhotobucketAccount *self;

	self = PHOTOBUCKET_ACCOUNT (obj);

	g_free (self->subdomain);
	g_free (self->home_url);
	g_free (self->album_url);

	G_OBJECT_CLASS (photobucket_account_parent_class)->finalize (obj);
}


static void
photobucket_account_class_init (PhotobucketAccountClass *klass)
{
	photobucket_account_parent_class = g_type_class_peek_parent (klass);
	G_OBJECT_CLASS (klass)->finalize = photobucket_account_finalize;
}


static DomElement*
photobucket_account_create_element (DomDomizable *base,
				    DomDocument  *doc)
{
	PhotobucketAccount *self;
	DomElement         *element;

	self = PHOTOBUCKET_ACCOUNT (base);
	element = oauth_account_create_element (DOM_DOMIZABLE (self), doc);
	if (self->subdomain != NULL)
		dom_element_set_attribute (element, "subdomain", self->subdomain);

	return element;
}


static void
photobucket_account_load_from_element (DomDomizable *base,
				       DomElement   *element)
{
	PhotobucketAccount *self;

	self = PHOTOBUCKET_ACCOUNT (base);

	if (g_str_equal (element->tag_name, "content")) {
		DomElement *node;

		for (node = element->first_child; node; node = node->next_sibling) {
			if (g_str_equal (node->tag_name, "album_url")) {
				photobucket_account_set_album_url (self, dom_element_get_inner_text (node));
			}
			else if (g_str_equal (node->tag_name, "megabytes_used")) {
				photobucket_account_set_megabytes_used (self, dom_element_get_inner_text (node));
			}
			else if (g_str_equal (node->tag_name, "megabytes_allowed")) {
				photobucket_account_set_megabytes_allowed (self, dom_element_get_inner_text (node));
			}
			else if (g_str_equal (node->tag_name, "premium")) {
				photobucket_account_set_is_premium (self, dom_element_get_inner_text (node));
			}
			else if (g_str_equal (node->tag_name, "public")) {
				photobucket_account_set_is_public (self, dom_element_get_inner_text (node));
			}
		}
	}
	else if (g_str_equal (element->tag_name, "account")) {
		oauth_account_load_from_element (DOM_DOMIZABLE (self), element);
		photobucket_account_set_subdomain (self, dom_element_get_attribute (element, "subdomain"));
	}
}


static void
photobucket_account_dom_domizable_interface_init (DomDomizableIface *iface)
{
	iface->create_element = photobucket_account_create_element;
	iface->load_from_element = photobucket_account_load_from_element;
}


static void
photobucket_account_instance_init (PhotobucketAccount *self)
{
	self->subdomain = NULL;
	self->home_url = NULL;
	self->album_url = NULL;
	self->is_premium = FALSE;
	self->is_public = TRUE;
}


GType
photobucket_account_get_type (void)
{
	static GType photobucket_account_type_id = 0;

	if (photobucket_account_type_id == 0) {
		static const GTypeInfo g_define_type_info = {
			sizeof (PhotobucketAccountClass),
			(GBaseInitFunc) NULL,
			(GBaseFinalizeFunc) NULL,
			(GClassInitFunc) photobucket_account_class_init,
			(GClassFinalizeFunc) NULL,
			NULL,
			sizeof (PhotobucketAccount),
			0,
			(GInstanceInitFunc) photobucket_account_instance_init,
			NULL
		};
		static const GInterfaceInfo dom_domizable_info = {
			(GInterfaceInitFunc) photobucket_account_dom_domizable_interface_init,
			(GInterfaceFinalizeFunc) NULL,
			NULL
		};

		photobucket_account_type_id = g_type_register_static (OAUTH_TYPE_ACCOUNT,
								      "PhotobucketAccount",
								      &g_define_type_info,
								      0);
		g_type_add_interface_static (photobucket_account_type_id, DOM_TYPE_DOMIZABLE, &dom_domizable_info);
	}

	return photobucket_account_type_id;
}


OAuthAccount *
photobucket_account_new (void)
{
	return g_object_new (PHOTOBUCKET_TYPE_ACCOUNT, NULL);
}


void
photobucket_account_set_subdomain (PhotobucketAccount *self,
				   const char         *value)
{
	_g_strset (&self->subdomain, value);
}


void
photobucket_account_set_home_url (PhotobucketAccount *self,
				  const char         *value)
{
	_g_strset (&self->home_url, value);
}


void
photobucket_account_set_album_url (PhotobucketAccount *self,
				   const char         *value)
{
	_g_strset (&self->album_url, value);
}


void
photobucket_account_set_megabytes_used (PhotobucketAccount *self,
				        const char         *value)
{
	self->megabytes_used = g_ascii_strtoull (value, NULL, 10);
}


void
photobucket_account_set_megabytes_allowed (PhotobucketAccount *self,
					   const char         *value)
{
	self->megabytes_allowed = g_ascii_strtoull (value, NULL, 10);
}


void
photobucket_account_set_is_premium (PhotobucketAccount *self,
				    const char         *value)
{
	self->is_premium =  g_str_equal (value, "1");
}


void
photobucket_account_set_is_public (PhotobucketAccount *self,
				   const char         *value)
{
	self->is_public =  g_str_equal (value, "1");
}
