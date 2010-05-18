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
#include <glib.h>
#include <glib/gi18n.h>
#include <gthumb.h>
#include "photobucket-account.h"
#include "photobucket-consumer.h"


gboolean
photobucket_utils_parse_response (SoupBuffer          *body,
				  DomDocument        **doc_p,
				  GSimpleAsyncResult  *result,
				  GError             **error)
{
	DomDocument *doc;
	DomElement  *node;

	doc = dom_document_new ();
	if (! dom_document_load (doc, body->data, body->length, error)) {
		g_object_unref (doc);
		return FALSE;
	}

	for (node = DOM_ELEMENT (doc)->first_child; node; node = node->next_sibling) {
		if (g_strcmp0 (node->tag_name, "response") == 0) {
			DomElement *child;
			const char *status = NULL;
			const char *message;
			const char *code;

			for (child = node->first_child; child; child = child->next_sibling) {
				if (g_strcmp0 (child->tag_name, "status") == 0) {
					status = dom_element_get_inner_text (child);
				}
				else if (g_strcmp0 (child->tag_name, "message") == 0) {
					message = dom_element_get_inner_text (child);
				}
				else if (g_strcmp0 (child->tag_name, "code") == 0) {
					code = dom_element_get_inner_text (child);
				}
			}

			if (status == NULL) {
				*error = g_error_new_literal (OAUTH_CONNECTION_ERROR, 999, _("Unknown error"));
				g_simple_async_result_set_from_error (result, *error);
			}
			else if (strcmp (status, "Exception") == 0) {
				*error = g_error_new_literal (OAUTH_CONNECTION_ERROR,
							      (code != NULL) ? atoi (code) : 999,
							      (message != NULL) ? message : _("Unknown error"));
			}

			if (*error != NULL) {
				g_object_unref (doc);
				return FALSE;
			}
		}
	}

	*doc_p = doc;

	return TRUE;
}


static void
photobucket_login_request_response (OAuthConnection    *self,
				    SoupMessage        *msg,
				    SoupBuffer         *body,
				    GSimpleAsyncResult *result)
{
	GHashTable *values;
	char       *token;
	char       *token_secret;

	values = soup_form_decode (body->data);
	token = g_hash_table_lookup (values, "oauth_token");
	token_secret = g_hash_table_lookup (values, "oauth_token_secret");
	if ((token != NULL) && (token_secret != NULL)) {
		oauth_connection_set_token (self, token, token_secret);
		g_simple_async_result_set_op_res_gboolean (result, TRUE);
	}
	else {
		GError *error;

		error = g_error_new_literal (OAUTH_CONNECTION_ERROR, 0, _("Unknown error"));
		g_simple_async_result_set_from_error (result, error);
	}

	g_hash_table_destroy (values);
}


static char *
phoyobucket_get_login_link (OAuthConnection *self)
{
	char *token;
	char *uri;

	token = soup_uri_encode (oauth_connection_get_token (self), NULL);
	uri = g_strconcat ("http://photobucket.com/apilogin/login?oauth_token=", token, NULL);

	g_free (token);

	return uri;
}


static void
photobucket_get_access_token_response (OAuthConnection    *self,
				       SoupMessage        *msg,
				       SoupBuffer         *body,
				       GSimpleAsyncResult *result)
{
	GHashTable *values;
	char       *username;
	char       *token;
	char       *token_secret;

	values = soup_form_decode (body->data);

	username = g_hash_table_lookup (values, "username");
	token = g_hash_table_lookup (values, "oauth_token");
	token_secret = g_hash_table_lookup (values, "oauth_token_secret");
	if ((username != NULL) && (token != NULL) && (token_secret != NULL)) {
		OAuthAccount *account;

		oauth_connection_set_token (self, token, token_secret);

		account = photobucket_account_new ();
		oauth_account_set_username (account, username);
		oauth_account_set_token (account, token);
		oauth_account_set_token_secret (account, token_secret);
		photobucket_account_set_subdomain (PHOTOBUCKET_ACCOUNT (account), g_hash_table_lookup (values, "subdomain"));
		photobucket_account_set_home_url (PHOTOBUCKET_ACCOUNT (account), g_hash_table_lookup (values, "homeurl"));
		g_simple_async_result_set_op_res_gpointer (result, account, g_object_unref);
	}
	else {
		GError *error;

		error = g_error_new_literal (OAUTH_CONNECTION_ERROR, 0, _("Unknown error"));
		g_simple_async_result_set_from_error (result, error);
	}

	g_hash_table_destroy (values);
}


static char *
photobucket_get_check_token_url (OAuthConnection *self,
				 OAuthAccount    *account,
			         gboolean         for_signature)
{
	if (for_signature)
		return g_strconcat ("http://api.photobucket.com/user/", account->username, NULL);
	else
		return g_strconcat ("http://", PHOTOBUCKET_ACCOUNT (account)->subdomain, "/user/", account->username, NULL);
}


static void
photobucket_check_token_response (OAuthConnection    *self,
				  SoupMessage        *msg,
				  SoupBuffer         *body,
				  GSimpleAsyncResult *result,
				  OAuthAccount       *account)
{
	DomDocument *doc = NULL;
	GError      *error = NULL;

	if (photobucket_utils_parse_response (body, &doc, result, &error)) {
		DomElement *node;

		for (node = DOM_ELEMENT (doc)->first_child; node; node = node->next_sibling) {
			if (g_strcmp0 (node->tag_name, "response") == 0) {
				DomElement *child;

				for (child = DOM_ELEMENT (node)->first_child; child; child = child->next_sibling) {
					if (g_strcmp0 (child->tag_name, "content") == 0) {
						dom_domizable_load_from_element (DOM_DOMIZABLE (account), child);
						break;
					}
				}
				break;
			}
		}

		g_simple_async_result_set_op_res_gboolean (result, TRUE);
		g_object_unref (doc);
	}
	else
		g_simple_async_result_set_from_error (result, error);
}


OAuthConsumer photobucket_consumer = {
	"Photobucket",
	"photobucket",
	"http://www.photobucket.com",
	"http",
	"149829931",
	"b4e542229836cc59b66489c6d2d8ca04",
	"http://api.photobucket.com/login/request",
	photobucket_login_request_response,
	phoyobucket_get_login_link,
	"http://api.photobucket.com/login/access",
	photobucket_get_access_token_response,
	photobucket_get_check_token_url,
	photobucket_check_token_response
};
