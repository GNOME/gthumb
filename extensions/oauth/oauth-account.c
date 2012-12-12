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
#ifdef HAVE_LIBSECRET
#include <libsecret/secret.h>
#endif /* HAVE_LIBSECRET */
#include <gthumb.h>
#include "oauth-account.h"


enum {
        PROP_0,
        PROP_ID,
        PROP_USERNAME,
        PROP_NAME,
        PROP_TOKEN,
        PROP_TOKEN_SECRET,
        PROP_IS_DEFAULT
};


static void oauth_account_dom_domizable_interface_init (DomDomizableInterface *iface);


G_DEFINE_TYPE_WITH_CODE (OAuthAccount,
			 oauth_account,
			 G_TYPE_OBJECT,
			 G_IMPLEMENT_INTERFACE (DOM_TYPE_DOMIZABLE,
					        oauth_account_dom_domizable_interface_init))


static void
oauth_account_finalize (GObject *obj)
{
	OAuthAccount *self;

	self = OAUTH_ACCOUNT (obj);

	g_free (self->id);
	g_free (self->username);
	g_free (self->name);
	g_free (self->token);
	g_free (self->token_secret);

	G_OBJECT_CLASS (oauth_account_parent_class)->finalize (obj);
}


static void
oauth_account_set_property (GObject      *object,
			    guint         property_id,
			    const GValue *value,
			    GParamSpec   *pspec)
{
	OAuthAccount *self;

        self = OAUTH_ACCOUNT (object);

	switch (property_id) {
	case PROP_ID:
		_g_strset (&self->id, g_value_get_string (value));
		break;
	case PROP_USERNAME:
		_g_strset (&self->username, g_value_get_string (value));
		if (self->name == NULL)
			_g_strset (&self->name, g_value_get_string (value));
		break;
	case PROP_NAME:
		_g_strset (&self->name, g_value_get_string (value));
		break;
	case PROP_TOKEN:
		_g_strset (&self->token, g_value_get_string (value));
		break;
	case PROP_TOKEN_SECRET:
		_g_strset (&self->token_secret, g_value_get_string (value));
		break;
	case PROP_IS_DEFAULT:
		self->is_default = g_value_get_boolean (value);
		break;
	default:
		break;
	}
}


static void
oauth_account_get_property (GObject    *object,
			    guint       property_id,
			    GValue     *value,
			    GParamSpec *pspec)
{
	OAuthAccount *self;

        self = OAUTH_ACCOUNT (object);

	switch (property_id) {
	case PROP_ID:
		g_value_set_string (value, self->id);
		break;
	case PROP_USERNAME:
		g_value_set_string (value, self->username);
		break;
	case PROP_NAME:
		g_value_set_string (value, self->name);
		break;
	case PROP_TOKEN:
		g_value_set_string (value, self->token);
		break;
	case PROP_TOKEN_SECRET:
		g_value_set_string (value, self->token_secret);
		break;
	case PROP_IS_DEFAULT:
		g_value_set_boolean (value, self->is_default);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
		break;
	}
}


static void
oauth_account_class_init (OAuthAccountClass *klass)
{
	GObjectClass *object_class;

	object_class = G_OBJECT_CLASS (klass);
	object_class->finalize = oauth_account_finalize;
	object_class->set_property = oauth_account_set_property;
	object_class->get_property = oauth_account_get_property;

	/* properties */

	g_object_class_install_property (object_class,
					 PROP_ID,
					 g_param_spec_string ("id",
                                                              "ID",
                                                              "",
                                                              NULL,
                                                              G_PARAM_READWRITE));
	g_object_class_install_property (object_class,
					 PROP_USERNAME,
					 g_param_spec_string ("username",
                                                              "Username",
                                                              "",
                                                              NULL,
                                                              G_PARAM_READWRITE));
	g_object_class_install_property (object_class,
					 PROP_NAME,
					 g_param_spec_string ("name",
                                                              "Name",
                                                              "",
                                                              NULL,
                                                              G_PARAM_READWRITE));
	g_object_class_install_property (object_class,
					 PROP_TOKEN,
					 g_param_spec_string ("token",
                                                              "Token",
                                                              "",
                                                              NULL,
                                                              G_PARAM_READWRITE));
	g_object_class_install_property (object_class,
					 PROP_TOKEN_SECRET,
					 g_param_spec_string ("token-secret",
                                                              "Token secret",
                                                              "",
                                                              NULL,
                                                              G_PARAM_READWRITE));
	g_object_class_install_property (object_class,
					 PROP_IS_DEFAULT,
					 g_param_spec_boolean ("is-default",
                                                               "Is default",
                                                               "",
                                                               FALSE,
                                                               G_PARAM_READWRITE));
}


static DomElement *
oauth_account_create_element (DomDomizable *base,
			      DomDocument  *doc)
{
	OAuthAccount *self;
	DomElement    *element;
	gboolean       set_token;

	self = OAUTH_ACCOUNT (base);

	element = dom_document_create_element (doc, "account", NULL);
	if (self->id != NULL)
		dom_element_set_attribute (element, "id", self->id);
	if (self->username != NULL)
		dom_element_set_attribute (element, "username", self->username);
	if (self->name != NULL)
		dom_element_set_attribute (element, "name", self->name);

	/* Don't save the token in the configuration file if the keyring is
	 * available. */

#ifdef HAVE_LIBSECRET
	set_token = FALSE;
#else
	set_token = TRUE;
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

	g_object_set (self,
		      "id", dom_element_get_attribute (element, "id"),
		      "username", dom_element_get_attribute (element, "username"),
		      "name", dom_element_get_attribute (element, "name"),
		      "token", dom_element_get_attribute (element, "token"),
		      "is-default", (g_strcmp0 (dom_element_get_attribute (element, "default"), "1") == 0),
		      NULL);
}


static void
oauth_account_dom_domizable_interface_init (DomDomizableInterface *iface)
{
	iface->create_element = oauth_account_create_element;
	iface->load_from_element = oauth_account_load_from_element;
}


static void
oauth_account_init (OAuthAccount *self)
{
	self->id = NULL;
	self->username = NULL;
	self->name = NULL;
	self->token = NULL;
	self->token_secret = NULL;
	self->is_default = FALSE;
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
	else if ((a->id != NULL) || (b->id != NULL))
		return g_strcmp0 (a->id, b->id);
	else if ((a->username != NULL) || (b->username != NULL))
		return g_strcmp0 (a->username, b->username);
	else
		return g_strcmp0 (a->name, b->name);
}
