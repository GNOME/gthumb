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
#include "photobucket-service.h"


struct _PhotobucketServicePrivate
{
	PhotobucketConnection *conn;
	PhotobucketUser       *user;
};


static gpointer parent_class = NULL;


static void
photobucket_service_finalize (GObject *object)
{
	PhotobucketService *self;

	self = PHOTOBUCKET_SERVICE (object);

	_g_object_unref (self->priv->conn);
	_g_object_unref (self->priv->user);

	G_OBJECT_CLASS (parent_class)->finalize (object);
}


static void
photobucket_service_class_init (PhotobucketServiceClass *klass)
{
	GObjectClass *object_class;

	parent_class = g_type_class_peek_parent (klass);
	g_type_class_add_private (klass, sizeof (PhotobucketServicePrivate));

	object_class = (GObjectClass*) klass;
	object_class->finalize = photobucket_service_finalize;
}


static void
photobucket_service_init (PhotobucketService *self)
{
	self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, PHOTOBUCKET_TYPE_SERVICE, PhotobucketServicePrivate);
	self->priv->conn = NULL;
	self->priv->user = NULL;
}


GType
photobucket_service_get_type (void)
{
	static GType type = 0;

	if (! type) {
		GTypeInfo type_info = {
			sizeof (PhotobucketServiceClass),
			NULL,
			NULL,
			(GClassInitFunc) photobucket_service_class_init,
			NULL,
			NULL,
			sizeof (PhotobucketService),
			0,
			(GInstanceInitFunc) photobucket_service_init
		};

		type = g_type_register_static (G_TYPE_OBJECT,
					       "PhotobucketService",
					       &type_info,
					       0);
	}

	return type;
}


PhotobucketService *
photobucket_service_new (PhotobucketConnection *conn)
{
	PhotobucketService *self;

	self = (PhotobucketService *) g_object_new (PHOTOBUCKET_TYPE_SERVICE, NULL);
	self->priv->conn = g_object_ref (conn);

	return self;
}


/* -- photobucket_service_login_request -- */


static void
login_request_ready_cb (SoupSession *session,
			SoupMessage *msg,
			gpointer     user_data)
{
	PhotobucketService *self = user_data;
	GSimpleAsyncResult *result;
	SoupBuffer         *body;
	DomDocument        *doc = NULL;
	GError             *error = NULL;

	result = oauth_connection_get_result (self->priv->conn);

	if (msg->status_code != 200) {
		g_simple_async_result_set_error (result,
						 SOUP_HTTP_ERROR,
						 msg->status_code,
						 "%s",
						 soup_status_get_phrase (msg->status_code));
		g_simple_async_result_complete_in_idle (result);
		return;
	}

	body = soup_message_body_flatten (msg->response_body);
	if (photobucket_utils_parse_response (body, &doc, &error)) {
		DomElement *root;
		char       *uid = NULL;

		root = DOM_ELEMENT (doc)->first_child;
		if (g_strcmp0 (root->tag_name, "users_getLoggedInUser_response") == 0)
			uid = g_strdup (dom_element_get_inner_text (root));

		if (uid == NULL) {
			error = g_error_new_literal (PHOTOBUCKET_CONNECTION_ERROR, 0, _("Unknown error"));
			g_simple_async_result_set_from_error (result, error);
		}
		else
			g_simple_async_result_set_op_res_gboolean (result, TRUE);

		g_object_unref (doc);
	}
	else
		g_simple_async_result_set_from_error (result, error);

	g_simple_async_result_complete_in_idle (result);

	soup_buffer_free (body);
}


void
photobucket_service_login_request (PhotobucketService  *self,
				   GCancellable        *cancellable,
				   GAsyncReadyCallback  callback,
				   gpointer             user_data)
{
	GHashTable  *data_set;
	SoupMessage *msg;

	gth_task_progress (GTH_TASK (self->priv->conn), _("Connecting to the server"), _("Getting account information"), TRUE, 0.0);

	data_set = g_hash_table_new (g_str_hash, g_str_equal);
	uri = "http://api.photobucket.com/login/request";
	oauth_connection_add_signature (self->priv->conn, "POST", uri, data_set);
	msg = soup_form_request_new_from_hash ("POST", uri, data_set);
	oauth_connection_send_message (self->priv->conn,
				       msg,
				       cancellable,
				       callback,
				       user_data,
				       photobucket_service_login_request,
				       login_request_ready_cb,
				       self);

	g_hash_table_destroy (data_set);
}


gboolean
photobucket_service_login_request_finish (PhotobucketService  *self,
					    GAsyncResult     *result,
					    GError          **error)
{
	if (g_simple_async_result_propagate_error (G_SIMPLE_ASYNC_RESULT (result), error))
		return FALSE;
	else
		return TRUE;
}


char *
photobucket_service_get_login_link (PhotobucketService *self)
{

}


/* utilities */


gboolean
photobucket_utils_parse_response (SoupBuffer   *body,
				  DomDocument **doc_p,
				  GError      **error)
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
				error = g_error_new_literal (OAUTH_CONNECTION_ERROR, 999, _("Unknown error"));
				g_simple_async_result_set_from_error (self->priv->result, error);
			}
			else if (strcmp (status, "Exception") == 0) {
				*error = g_error_new_literal (OAUTH_CONNECTION_ERROR,
							      (code != NULL) ? atoi (code) : 999,
							      (message != NULL) ? message : _("Unknown error"));
			}

			g_object_unref (doc);
			return FALSE;
		}
	}

	*doc_p = doc;

	return TRUE;
}


GList *
photobucket_accounts_load_from_file (void)
{
	GList       *accounts = NULL;
	char        *filename;
	char        *buffer;
	gsize        len;
	DomDocument *doc;

	filename = gth_user_dir_get_file (GTH_DIR_CONFIG, GTHUMB_DIR, "accounts", "photobucket.xml", NULL);
	if (! g_file_get_contents (filename, &buffer, &len, NULL)) {
		g_free (filename);
		return NULL;
	}

	doc = dom_document_new ();
	if (dom_document_load (doc, buffer, len, NULL)) {
		DomElement *node;

		node = DOM_ELEMENT (doc)->first_child;
		if ((node != NULL) && (g_strcmp0 (node->tag_name, "accounts") == 0)) {
			DomElement *child;

			for (child = node->first_child;
			     child != NULL;
			     child = child->next_sibling)
			{
				if (strcmp (child->tag_name, "account") == 0) {
					PhotobucketAccount *account;

					account = photobucket_account_new ();
					dom_domizable_load_from_element (DOM_DOMIZABLE (account), child);

					accounts = g_list_prepend (accounts, account);
				}
			}

			accounts = g_list_reverse (accounts);
		}
	}

	g_object_unref (doc);
	g_free (buffer);
	g_free (filename);

	return accounts;
}


PhotobucketAccount *
photobucket_accounts_find_default (GList *accounts)
{
	GList *scan;

	for (scan = accounts; scan; scan = scan->next) {
		PhotobucketAccount *account = scan->data;

		if (account->is_default)
			return g_object_ref (account);
	}

	return NULL;
}


void
photobucket_accounts_save_to_file (GList         *accounts,
			      PhotobucketAccount *default_account)
{
	DomDocument *doc;
	DomElement  *root;
	GList       *scan;
	char        *buffer;
	gsize        len;
	char        *filename;
	GFile       *file;

	doc = dom_document_new ();
	root = dom_document_create_element (doc, "accounts", NULL);
	dom_element_append_child (DOM_ELEMENT (doc), root);
	for (scan = accounts; scan; scan = scan->next) {
		PhotobucketAccount *account = scan->data;
		DomElement    *node;

		if ((default_account != NULL) && g_strcmp0 (account->username, default_account->username) == 0)
			account->is_default = TRUE;
		else
			account->is_default = FALSE;
		node = dom_domizable_create_element (DOM_DOMIZABLE (account), doc);
		dom_element_append_child (root, node);
	}

	gth_user_dir_make_dir_for_file (GTH_DIR_CONFIG, GTHUMB_DIR, "accounts", "photobucket.xml", NULL);
	filename = gth_user_dir_get_file (GTH_DIR_CONFIG, GTHUMB_DIR, "accounts", "photobucket.xml", NULL);
	file = g_file_new_for_path (filename);
	buffer = dom_document_dump (doc, &len);
	g_write_file (file, FALSE, G_FILE_CREATE_PRIVATE | G_FILE_CREATE_REPLACE_DESTINATION, buffer, len, NULL, NULL);

	g_free (buffer);
	g_object_unref (file);
	g_free (filename);
	g_object_unref (doc);
}
