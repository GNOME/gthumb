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
#include "oauth-account.h"


static gpointer oauth_account_parent_class = NULL;


static void
oauth_account_finalize (GObject *obj)
{
	OAuthAccount *self;

	self = OAUTH_ACCOUNT (obj);

	g_free (self->username);
	g_free (self->token);
	g_free (self->token_secret);

	G_OBJECT_CLASS (oauth_account_parent_class)->finalize (obj);
}


static void
oauth_account_class_init (OAuthAccountClass *klass)
{
	oauth_account_parent_class = g_type_class_peek_parent (klass);
	G_OBJECT_CLASS (klass)->finalize = oauth_account_finalize;
}


static DomElement*
oauth_account_create_element (DomDomizable *base,
			      DomDocument  *doc)
{
	OAuthAccount *self;
	DomElement    *element;
	gboolean       set_token;

	self = OAUTH_ACCOUNT (base);

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
oauth_account_load_from_element (DomDomizable *base,
			          DomElement   *element)
{
	OAuthAccount *self;

	self = OAUTH_ACCOUNT (base);

	oauth_account_set_username (self, dom_element_get_attribute (element, "username"));
	oauth_account_set_token (self, dom_element_get_attribute (element, "token"));
	self->is_default = (g_strcmp0 (dom_element_get_attribute (element, "default"), "1") == 0);
}


static void
oauth_account_dom_domizable_interface_init (DomDomizableIface *iface)
{
	iface->create_element = oauth_account_create_element;
	iface->load_from_element = oauth_account_load_from_element;
}


static void
oauth_account_instance_init (OAuthAccount *self)
{
}


GType
oauth_account_get_type (void)
{
	static GType oauth_account_type_id = 0;

	if (oauth_account_type_id == 0) {
		static const GTypeInfo g_define_type_info = {
			sizeof (OAuthAccountClass),
			(GBaseInitFunc) NULL,
			(GBaseFinalizeFunc) NULL,
			(GClassInitFunc) oauth_account_class_init,
			(GClassFinalizeFunc) NULL,
			NULL,
			sizeof (OAuthAccount),
			0,
			(GInstanceInitFunc) oauth_account_instance_init,
			NULL
		};
		static const GInterfaceInfo dom_domizable_info = {
			(GInterfaceInitFunc) oauth_account_dom_domizable_interface_init,
			(GInterfaceFinalizeFunc) NULL,
			NULL
		};

		oauth_account_type_id = g_type_register_static (G_TYPE_OBJECT,
								   "OAuthAccount",
								   &g_define_type_info,
								   0);
		g_type_add_interface_static (oauth_account_type_id, DOM_TYPE_DOMIZABLE, &dom_domizable_info);
	}

	return oauth_account_type_id;
}


OAuthAccount *
oauth_account_new (void)
{
	return g_object_new (OAUTH_TYPE_ACCOUNT, NULL);
}


void
oauth_account_set_username (OAuthAccount *self,
			    const char    *value)
{
	_g_strset (&self->username, value);
}


void
oauth_account_set_token (OAuthAccount *self,
			 const char    *value)
{
	_g_strset (&self->token, value);
}


void
oauth_account_set_token_secret (OAuthAccount *self,
				const char   *value)
{
	_g_strset (&self->token_secret, value);
}


int
oauth_account_cmp (OAuthAccount *a,
		   OAuthAccount *b)
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
