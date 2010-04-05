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
#include "facebook-user.h"


static gpointer facebook_user_parent_class = NULL;


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

	return element;
}


static void
facebook_user_load_from_element (DomDomizable *base,
			       DomElement   *element)
{
	FacebookUser *self;
	DomElement *node;

	self = FACEBOOK_USER (base);

	facebook_user_set_id (self, dom_element_get_attribute (element, "id"));
	facebook_user_set_is_pro (self, dom_element_get_attribute (element, "ispro"));

	for (node = element->first_child; node; node = node->next_sibling) {
		if (g_strcmp0 (node->tag_name, "username") == 0) {
			facebook_user_set_username (self, dom_element_get_inner_text (node));
		}
		else if (g_strcmp0 (node->tag_name, "bandwidth") == 0) {
			facebook_user_set_max_bandwidth (self, dom_element_get_attribute (node, "maxbytes"));
			facebook_user_set_used_bandwidth (self, dom_element_get_attribute (node, "usedbytes"));
		}
		else if (g_strcmp0 (node->tag_name, "filesize") == 0) {
			facebook_user_set_max_filesize (self, dom_element_get_attribute (node, "maxbytes"));
		}
		else if (g_strcmp0 (node->tag_name, "videosize") == 0) {
			facebook_user_set_max_videosize (self, dom_element_get_attribute (node, "maxbytes"));
		}
		else if (g_strcmp0 (node->tag_name, "sets") == 0) {
			facebook_user_set_n_sets (self, dom_element_get_attribute (node, "created"));
		}
		else if (g_strcmp0 (node->tag_name, "videos") == 0) {
			facebook_user_set_n_videos (self, dom_element_get_attribute (node, "uploaded"));
		}
	}
}


static void
facebook_user_dom_domizable_interface_init (DomDomizableIface *iface)
{
	iface->create_element = facebook_user_create_element;
	iface->load_from_element = facebook_user_load_from_element;
}


static void
facebook_user_instance_init (FacebookUser *self)
{
	self->id = NULL;
	self->username = NULL;
}


GType
facebook_user_get_type (void)
{
	static GType facebook_user_type_id = 0;

	if (facebook_user_type_id == 0) {
		static const GTypeInfo g_define_type_info = {
			sizeof (FacebookUserClass),
			(GBaseInitFunc) NULL,
			(GBaseFinalizeFunc) NULL,
			(GClassInitFunc) facebook_user_class_init,
			(GClassFinalizeFunc) NULL,
			NULL,
			sizeof (FacebookUser),
			0,
			(GInstanceInitFunc) facebook_user_instance_init,
			NULL
		};
		static const GInterfaceInfo dom_domizable_info = {
			(GInterfaceInitFunc) facebook_user_dom_domizable_interface_init,
			(GInterfaceFinalizeFunc) NULL,
			NULL
		};

		facebook_user_type_id = g_type_register_static (G_TYPE_OBJECT,
								   "FacebookUser",
								   &g_define_type_info,
								   0);
		g_type_add_interface_static (facebook_user_type_id, DOM_TYPE_DOMIZABLE, &dom_domizable_info);
	}

	return facebook_user_type_id;
}


FacebookUser *
facebook_user_new (void)
{
	return g_object_new (FACEBOOK_TYPE_USER, NULL);
}


void
facebook_user_set_id (FacebookUser *self,
		    const char *value)
{
	g_free (self->id);
	self->id = NULL;
	if (value != NULL)
		self->id = g_strdup (value);
}


void
facebook_user_set_is_pro (FacebookUser *self,
			const char *value)
{
	self->is_pro = (g_strcmp0 (value, "1") == 0);
}


void
facebook_user_set_username(FacebookUser *self,
			 const char *value)
{
	g_free (self->username);
	self->username = NULL;
	if (value != NULL)
		self->username = g_strdup (value);
}


void
facebook_user_set_max_bandwidth (FacebookUser *self,
			       const char *value)
{
	self->max_bandwidth = g_ascii_strtoull (value, NULL, 10);
}


void
facebook_user_set_used_bandwidth (FacebookUser *self,
				const char *value)
{
	self->used_bandwidth = g_ascii_strtoull (value, NULL, 10);
}


void
facebook_user_set_max_filesize (FacebookUser *self,
			      const char *value)
{
	self->max_filesize = g_ascii_strtoull (value, NULL, 10);
}


void
facebook_user_set_max_videosize (FacebookUser *self,
			       const char *value)
{
	self->max_videosize = g_ascii_strtoull (value, NULL, 10);
}


void
facebook_user_set_n_sets (FacebookUser *self,
			const char *value)
{
	if (value != NULL)
		self->n_sets = atoi (value);
	else
		self->n_sets = 0;
}


void
facebook_user_set_n_videos (FacebookUser *self,
			  const char *value)
{
	if (value != NULL)
		self->n_videos = atoi (value);
	else
		self->n_videos = 0;
}
