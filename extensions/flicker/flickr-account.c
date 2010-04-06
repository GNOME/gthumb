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
#ifdef HAVE_GNOME_KEYRING
#include <gnome-keyring.h>
#endif /* HAVE_GNOME_KEYRING */
#include <gthumb.h>
#include "flickr-account.h"


static gpointer flickr_account_parent_class = NULL;


static void
flickr_account_finalize (GObject *obj)
{
	FlickrAccount *self;

	self = FLICKR_ACCOUNT (obj);

	g_free (self->username);
	g_free (self->token);

	G_OBJECT_CLASS (flickr_account_parent_class)->finalize (obj);
}


static void
flickr_account_class_init (FlickrAccountClass *klass)
{
	flickr_account_parent_class = g_type_class_peek_parent (klass);
	G_OBJECT_CLASS (klass)->finalize = flickr_account_finalize;
}


static DomElement*
flickr_account_create_element (DomDomizable *base,
			       DomDocument  *doc)
{
	FlickrAccount *self;
	DomElement    *element;
	gboolean       set_token;

	self = FLICKR_ACCOUNT (base);

	element = dom_document_create_element (doc, "account", NULL);
	if (self->username != NULL)
		dom_element_set_attribute (element, "username", self->username);

	/* Don't save the token in the configuration file if gnome-keyring is
	 * available. */

	set_token = TRUE;
#ifdef HAVE_GNOME_KEYRING
	if (gnome_keyring_is_available ())
		set_token = FALSE;
#endif
	if (set_token && (self->token != NULL))
		dom_element_set_attribute (element, "token", self->token);

	if (self->is_default)
		dom_element_set_attribute (element, "default", "1");

	return element;
}


static void
flickr_account_load_from_element (DomDomizable *base,
			          DomElement   *element)
{
	FlickrAccount *self;

	self = FLICKR_ACCOUNT (base);

	flickr_account_set_username (self, dom_element_get_attribute (element, "username"));
	flickr_account_set_token (self, dom_element_get_attribute (element, "token"));
	self->is_default = (g_strcmp0 (dom_element_get_attribute (element, "default"), "1") == 0);
}


static void
flickr_account_dom_domizable_interface_init (DomDomizableIface *iface)
{
	iface->create_element = flickr_account_create_element;
	iface->load_from_element = flickr_account_load_from_element;
}


static void
flickr_account_instance_init (FlickrAccount *self)
{
}


GType
flickr_account_get_type (void)
{
	static GType flickr_account_type_id = 0;

	if (flickr_account_type_id == 0) {
		static const GTypeInfo g_define_type_info = {
			sizeof (FlickrAccountClass),
			(GBaseInitFunc) NULL,
			(GBaseFinalizeFunc) NULL,
			(GClassInitFunc) flickr_account_class_init,
			(GClassFinalizeFunc) NULL,
			NULL,
			sizeof (FlickrAccount),
			0,
			(GInstanceInitFunc) flickr_account_instance_init,
			NULL
		};
		static const GInterfaceInfo dom_domizable_info = {
			(GInterfaceInitFunc) flickr_account_dom_domizable_interface_init,
			(GInterfaceFinalizeFunc) NULL,
			NULL
		};

		flickr_account_type_id = g_type_register_static (G_TYPE_OBJECT,
								   "FlickrAccount",
								   &g_define_type_info,
								   0);
		g_type_add_interface_static (flickr_account_type_id, DOM_TYPE_DOMIZABLE, &dom_domizable_info);
	}

	return flickr_account_type_id;
}


FlickrAccount *
flickr_account_new (void)
{
	return g_object_new (FLICKR_TYPE_ACCOUNT, NULL);
}


void
flickr_account_set_username (FlickrAccount *self,
			     const char    *value)
{
	g_free (self->username);
	self->username = NULL;
	if (value != NULL)
		self->username = g_strdup (value);
}


void
flickr_account_set_token (FlickrAccount *self,
			  const char    *value)
{
	g_free (self->token);
	self->token = NULL;
	if (value != NULL)
		self->token = g_strdup (value);
}


int
flickr_account_cmp (FlickrAccount *a,
		    FlickrAccount *b)
{
	if ((a == NULL) && (b == NULL))
		return 0;
	else if (a == NULL)
		return 1;
	else if (b == NULL)
		return -1;
	else
		return g_strcmp0 (a->username, b->username);
}
