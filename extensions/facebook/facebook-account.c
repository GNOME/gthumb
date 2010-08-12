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
#ifdef HAVE_GNOME_KEYRING
#include <gnome-keyring.h>
#endif /* HAVE_GNOME_KEYRING */
#include <gthumb.h>
#include "facebook-account.h"


static gpointer facebook_account_parent_class = NULL;


static void
facebook_account_finalize (GObject *obj)
{
	FacebookAccount *self;

	self = FACEBOOK_ACCOUNT (obj);

	g_free (self->user_id);
	g_free (self->username);
	g_free (self->session_key);
	g_free (self->secret);

	G_OBJECT_CLASS (facebook_account_parent_class)->finalize (obj);
}


static void
facebook_account_class_init (FacebookAccountClass *klass)
{
	facebook_account_parent_class = g_type_class_peek_parent (klass);
	G_OBJECT_CLASS (klass)->finalize = facebook_account_finalize;
}


static DomElement*
facebook_account_create_element (DomDomizable *base,
			       DomDocument  *doc)
{
	FacebookAccount *self;
	DomElement      *element;
	gboolean         set_secret;

	self = FACEBOOK_ACCOUNT (base);

	element = dom_document_create_element (doc, "account", NULL);
	if (self->user_id != NULL)
		dom_element_set_attribute (element, "uid", self->user_id);
	if (self->username != NULL)
		dom_element_set_attribute (element, "username", self->username);

	/* Don't save the secret in the configuration file if gnome-keyring is
	 * available. */

	set_secret = TRUE;
#ifdef HAVE_GNOME_KEYRING
	if (gnome_keyring_is_available ())
		set_secret = FALSE;
#endif
	if (set_secret) {
		if (self->session_key != NULL)
			dom_element_set_attribute (element, "session_key", self->session_key);
		if (self->secret != NULL)
			dom_element_set_attribute (element, "secret", self->secret);
	}
	if (self->is_default)
		dom_element_set_attribute (element, "default", "1");

	return element;
}


static void
facebook_account_load_from_element (DomDomizable *base,
			          DomElement   *element)
{
	FacebookAccount *self;

	self = FACEBOOK_ACCOUNT (base);

	_g_strset (&self->user_id, dom_element_get_attribute (element, "uid"));
	_g_strset (&self->username, dom_element_get_attribute (element, "username"));
	_g_strset (&self->session_key, dom_element_get_attribute (element, "session_key"));
	_g_strset (&self->secret, dom_element_get_attribute (element, "secret"));
	self->is_default = (g_strcmp0 (dom_element_get_attribute (element, "default"), "1") == 0);
}


static void
facebook_account_dom_domizable_interface_init (DomDomizableIface *iface)
{
	iface->create_element = facebook_account_create_element;
	iface->load_from_element = facebook_account_load_from_element;
}


static void
facebook_account_instance_init (FacebookAccount *self)
{
	self->user_id = NULL;
	self->username = NULL;
	self->session_key = NULL;
	self->secret = NULL;
	self->is_default = FALSE;
}


GType
facebook_account_get_type (void)
{
	static GType facebook_account_type_id = 0;

	if (facebook_account_type_id == 0) {
		static const GTypeInfo g_define_type_info = {
			sizeof (FacebookAccountClass),
			(GBaseInitFunc) NULL,
			(GBaseFinalizeFunc) NULL,
			(GClassInitFunc) facebook_account_class_init,
			(GClassFinalizeFunc) NULL,
			NULL,
			sizeof (FacebookAccount),
			0,
			(GInstanceInitFunc) facebook_account_instance_init,
			NULL
		};
		static const GInterfaceInfo dom_domizable_info = {
			(GInterfaceInitFunc) facebook_account_dom_domizable_interface_init,
			(GInterfaceFinalizeFunc) NULL,
			NULL
		};

		facebook_account_type_id = g_type_register_static (G_TYPE_OBJECT,
								   "FacebookAccount",
								   &g_define_type_info,
								   0);
		g_type_add_interface_static (facebook_account_type_id, DOM_TYPE_DOMIZABLE, &dom_domizable_info);
	}

	return facebook_account_type_id;
}


FacebookAccount *
facebook_account_new (void)
{
	return g_object_new (FACEBOOK_TYPE_ACCOUNT, NULL);
}


void
facebook_account_set_session_key (FacebookAccount *self,
				  const char      *value)
{
	_g_strset (&self->session_key, value);
}


void
facebook_account_set_secret (FacebookAccount *self,
			     const char      *value)
{
	_g_strset (&self->secret, value);
}


void
facebook_account_set_user_id (FacebookAccount *self,
			      const char      *value)
{
	_g_strset (&self->user_id, value);
}


void
facebook_account_set_username (FacebookAccount *self,
			       const char      *value)
{
	_g_strset (&self->username, value);
}


int
facebook_account_cmp (FacebookAccount *a,
		      FacebookAccount *b)
{
	if ((a == NULL) && (b == NULL))
		return 0;
	else if (a == NULL)
		return 1;
	else if (b == NULL)
		return -1;
	else
		return g_strcmp0 (a->user_id, b->user_id);
}
