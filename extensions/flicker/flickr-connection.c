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
#include "flickr-connection.h"
#include "flickr-user.h"


#undef DEBUG_FLICKR_CONNECTION


GQuark
flickr_connection_error_quark (void)
{
	static GQuark quark;

        if (!quark)
                quark = g_quark_from_static_string ("flickr_connection");

        return quark;
}


/* -- FlickrConnection -- */


struct _FlickrConnectionPrivate
{
	SoupSession        *session;
	char               *frob;
	char               *token;
	char               *username;
	char               *user_id;
	GCancellable       *cancellable;
	GSimpleAsyncResult *result;
	GChecksum          *checksum;
};


static gpointer parent_class = NULL;


static void
flickr_connection_finalize (GObject *object)
{
	FlickrConnection *self;

	self = FLICKR_CONNECTION (object);

	g_checksum_free (self->priv->checksum);
	_g_object_unref (self->priv->result);
	_g_object_unref (self->priv->cancellable);
	g_free (self->priv->user_id);
	g_free (self->priv->username);
	g_free (self->priv->token);
	g_free (self->priv->frob);
	_g_object_unref (self->priv->session);

	G_OBJECT_CLASS (parent_class)->finalize (object);
}


static void
flickr_connection_exec (GthTask *base)
{
	/* void */
}


static void
flickr_connection_cancelled (GthTask *base)
{
	/* void */
}


static void
flickr_connection_class_init (FlickrConnectionClass *klass)
{
	GObjectClass *object_class;
	GthTaskClass *task_class;

	parent_class = g_type_class_peek_parent (klass);
	g_type_class_add_private (klass, sizeof (FlickrConnectionPrivate));

	object_class = (GObjectClass*) klass;
	object_class->finalize = flickr_connection_finalize;

	task_class = (GthTaskClass*) klass;
	task_class->exec = flickr_connection_exec;
	task_class->cancelled = flickr_connection_cancelled;
}


static void
flickr_connection_init (FlickrConnection *self)
{
	self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, FLICKR_TYPE_CONNECTION, FlickrConnectionPrivate);
	self->priv->session = NULL;
	self->priv->username = NULL;
	self->priv->user_id = NULL;
	self->priv->token = NULL;
	self->priv->frob = NULL;
	self->priv->cancellable = NULL;
	self->priv->result = NULL;
	self->priv->checksum = g_checksum_new (G_CHECKSUM_MD5);
}


GType
flickr_connection_get_type (void)
{
	static GType type = 0;

	if (! type) {
		GTypeInfo type_info = {
			sizeof (FlickrConnectionClass),
			NULL,
			NULL,
			(GClassInitFunc) flickr_connection_class_init,
			NULL,
			NULL,
			sizeof (FlickrConnection),
			0,
			(GInstanceInitFunc) flickr_connection_init
		};

		type = g_type_register_static (GTH_TYPE_TASK,
					       "FlickrConnection",
					       &type_info,
					       0);
	}

	return type;
}


FlickrConnection *
flickr_connection_new (FlickrServer *server)
{
	FlickrConnection *self;

	self = (FlickrConnection *) g_object_new (FLICKR_TYPE_CONNECTION, NULL);
	self->server = server;

	return self;
}


FlickrServer *
flickr_connection_get_server (FlickrConnection *self)
{
	return self->server;
}


void
flickr_connection_send_message (FlickrConnection    *self,
				SoupMessage         *msg,
				GCancellable        *cancellable,
				GAsyncReadyCallback  callback,
				gpointer             user_data,
				gpointer             source_tag,
				SoupSessionCallback  soup_session_cb,
				gpointer             soup_session_cb_data)
{
	if (self->priv->session == NULL) {
		self->priv->session = soup_session_async_new_with_options (
#ifdef HAVE_LIBSOUP_GNOME
			SOUP_SESSION_ADD_FEATURE_BY_TYPE, SOUP_TYPE_PROXY_RESOLVER_GNOME,
#endif
			NULL);

#ifdef DEBUG_FLICKR_CONNECTION
		{
			SoupLogger *logger;

			logger = soup_logger_new (SOUP_LOGGER_LOG_BODY, -1);
			soup_session_add_feature (self->priv->session, SOUP_SESSION_FEATURE (logger));

			g_object_unref (logger);
		}
#endif
	}

	_g_object_unref (self->priv->cancellable);
	self->priv->cancellable = _g_object_ref (cancellable);

	_g_object_unref (self->priv->result);
	self->priv->result = g_simple_async_result_new (G_OBJECT (soup_session_cb_data),
							callback,
							user_data,
							source_tag);

	soup_session_queue_message (self->priv->session,
				    msg,
				    soup_session_cb,
				    soup_session_cb_data);
}


GSimpleAsyncResult *
flickr_connection_get_result (FlickrConnection *self)
{
	return self->priv->result;
}


void
flickr_connection_reset_result (FlickrConnection *self)
{
	_g_object_unref (self->priv->result);
	self->priv->result = NULL;
}


void
flickr_connection_add_api_sig (FlickrConnection *self,
			       GHashTable       *data_set)
{
	GList *keys;
	GList *scan;

	g_hash_table_insert (data_set, "api_key", (gpointer) self->server->api_key);
	if (self->priv->token != NULL)
		g_hash_table_insert (data_set, "auth_token", self->priv->token);

	g_checksum_reset (self->priv->checksum);
	g_checksum_update (self->priv->checksum, (guchar *) self->server->shared_secret, -1);

	keys = g_hash_table_get_keys (data_set);
	keys = g_list_sort (keys, (GCompareFunc) strcmp);
	for (scan = keys; scan; scan = scan->next) {
		char *key = scan->data;

		g_checksum_update (self->priv->checksum, (guchar *) key, -1);
		g_checksum_update (self->priv->checksum, g_hash_table_lookup (data_set, key), -1);
	}
	g_hash_table_insert (data_set, "api_sig", (gpointer) g_checksum_get_string (self->priv->checksum));

	g_list_free (keys);
}


static void
connection_frob_ready_cb (SoupSession *session,
			  SoupMessage *msg,
			  gpointer     user_data)
{
	FlickrConnection *self = user_data;
	SoupBuffer       *body;
	DomDocument      *doc = NULL;
	GError           *error = NULL;

	g_free (self->priv->frob);
	self->priv->frob = NULL;

	body = soup_message_body_flatten (msg->response_body);
	if (flickr_utils_parse_response (body, &doc, &error)) {
		DomElement *root;
		DomElement *child;

		root = DOM_ELEMENT (doc)->first_child;
		for (child = root->first_child; child; child = child->next_sibling)
			if (g_strcmp0 (child->tag_name, "frob") == 0)
				self->priv->frob = g_strdup (dom_element_get_inner_text (child));

		if (self->priv->frob == NULL) {
			error = g_error_new_literal (FLICKR_CONNECTION_ERROR, 0, _("Unknown error"));
			g_simple_async_result_set_from_error (self->priv->result, error);
		}
		else
			g_simple_async_result_set_op_res_gboolean (self->priv->result, TRUE);

		g_object_unref (doc);
	}
	else
		g_simple_async_result_set_from_error (self->priv->result, error);

	g_simple_async_result_complete_in_idle (self->priv->result);

	soup_buffer_free (body);
}


void
flickr_connection_get_frob (FlickrConnection    *self,
			    GCancellable        *cancellable,
			    GAsyncReadyCallback  callback,
			    gpointer             user_data)
{
	GHashTable  *data_set;
	SoupMessage *msg;

	gth_task_progress (GTH_TASK (self), _("Connecting to the server"), NULL, TRUE, 0.0);

	g_free (self->priv->token);
	self->priv->token = NULL;

	data_set = g_hash_table_new (g_str_hash, g_str_equal);
	g_hash_table_insert (data_set, "method", "flickr.auth.getFrob");
	flickr_connection_add_api_sig (self, data_set);
	msg = soup_form_request_new_from_hash ("GET", self->server->rest_url, data_set);
	flickr_connection_send_message (self,
					msg,
					cancellable,
					callback,
					user_data,
					flickr_connection_get_frob,
					connection_frob_ready_cb,
					self);

	g_hash_table_destroy (data_set);
}


gboolean
flickr_connection_get_frob_finish (FlickrConnection  *self,
				   GAsyncResult      *result,
				   GError           **error)
{
	if (g_simple_async_result_propagate_error (G_SIMPLE_ASYNC_RESULT (result), error))
		return FALSE;
	else
		return TRUE;
}


static char *
get_access_type_name (FlickrAccessType access_type)
{
	char *name = NULL;

	switch (access_type) {
	case FLICKR_ACCESS_READ:
		name = "read";
		break;

	case FLICKR_ACCESS_WRITE:
		name = "write";
		break;

	case FLICKR_ACCESS_DELETE:
		name = "delete";
		break;
	}

	return name;
}

char *
flickr_connection_get_login_link (FlickrConnection *self,
				  FlickrAccessType  access_type)
{
	GHashTable *data_set;
	GString    *link;
	GList      *keys;
	GList      *scan;

	g_return_val_if_fail (self->priv->frob != NULL, NULL);

	data_set = g_hash_table_new (g_str_hash, g_str_equal);
	g_hash_table_insert (data_set, "frob", self->priv->frob);
	g_hash_table_insert (data_set, "perms", get_access_type_name (access_type));
	flickr_connection_add_api_sig (self, data_set);

	link = g_string_new (self->server->authentication_url);
	g_string_append (link, "?");
	keys = g_hash_table_get_keys (data_set);
	for (scan = keys; scan; scan = scan->next) {
		char *key = scan->data;

		if (scan != keys)
			g_string_append (link, "&");
		g_string_append (link, key);
		g_string_append (link, "=");
		g_string_append (link, g_hash_table_lookup (data_set, key));
	}

	g_list_free (keys);
	g_hash_table_destroy (data_set);

	return g_string_free (link, FALSE);
}


static void
connection_token_ready_cb (SoupSession *session,
			   SoupMessage *msg,
			   gpointer     user_data)
{
	FlickrConnection *self = user_data;
	SoupBuffer       *body;
	DomDocument      *doc = NULL;
	GError           *error = NULL;

	body = soup_message_body_flatten (msg->response_body);
	if (flickr_utils_parse_response (body, &doc, &error)) {
		DomElement *response;
		DomElement *auth;

		response = DOM_ELEMENT (doc)->first_child;
		for (auth = response->first_child; auth; auth = auth->next_sibling) {
			if (g_strcmp0 (auth->tag_name, "auth") == 0) {
				DomElement *node;

				for (node = auth->first_child; node; node = node->next_sibling) {
					if (g_strcmp0 (node->tag_name, "token") == 0) {
						self->priv->token = g_strdup (dom_element_get_inner_text (node));
					}
					else if (g_strcmp0 (node->tag_name, "user") == 0) {
						self->priv->username = g_strdup (dom_element_get_attribute (node, "username"));
						self->priv->user_id = g_strdup (dom_element_get_attribute (node, "nsid"));
					}
				}
			}
		}

		if (self->priv->token == NULL) {
			error = g_error_new_literal (FLICKR_CONNECTION_ERROR, 0, _("Unknown error"));
			g_simple_async_result_set_from_error (self->priv->result, error);
		}
		else
			g_simple_async_result_set_op_res_gboolean (self->priv->result, TRUE);

		g_object_unref (doc);
	}
	else
		g_simple_async_result_set_from_error (self->priv->result, error);

	g_simple_async_result_complete_in_idle (self->priv->result);

	soup_buffer_free (body);
}


void
flickr_connection_get_token (FlickrConnection    *self,
			     GCancellable        *cancellable,
			     GAsyncReadyCallback  callback,
			     gpointer             user_data)
{
	GHashTable  *data_set;
	SoupMessage *msg;

	gth_task_progress (GTH_TASK (self), _("Connecting to the server"), NULL, TRUE, 0.0);

	g_free (self->priv->token);
	self->priv->token = NULL;

	data_set = g_hash_table_new (g_str_hash, g_str_equal);
	g_hash_table_insert (data_set, "method", "flickr.auth.getToken");
	g_hash_table_insert (data_set, "frob", self->priv->frob);
	flickr_connection_add_api_sig (self, data_set);
	msg = soup_form_request_new_from_hash ("GET", self->server->rest_url, data_set);
	flickr_connection_send_message (self,
					msg,
					cancellable,
					callback,
					user_data,
					flickr_connection_get_token,
					connection_token_ready_cb,
					self);

	g_hash_table_destroy (data_set);
}


gboolean
flickr_connection_get_token_finish (FlickrConnection  *self,
				    GAsyncResult      *result,
				    GError           **error)
{
	if (g_simple_async_result_propagate_error (G_SIMPLE_ASYNC_RESULT (result), error))
		return FALSE;
	else
		return TRUE;
}


void
flickr_connection_set_auth_token (FlickrConnection *self,
				  const char       *value)
{
	g_free (self->priv->token);
	self->priv->token = NULL;
	if (value != NULL)
		self->priv->token = g_strdup (value);
}


const char *
flickr_connection_get_auth_token (FlickrConnection *self)
{
	return self->priv->token;
}


const char *
flickr_connection_get_username (FlickrConnection *self)
{
	return self->priv->username;
}


const char *
flickr_connection_get_user_id (FlickrConnection *self)
{
	return self->priv->user_id;
}


/* utilities */


gboolean
flickr_utils_parse_response (SoupBuffer   *body,
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
		if (g_strcmp0 (node->tag_name, "rsp") == 0) {
			if (g_strcmp0 (dom_element_get_attribute (node, "stat"), "ok") != 0) {
				DomElement *child;

				for (child = node->first_child; child; child = child->next_sibling) {
					if (g_strcmp0 (child->tag_name, "err") == 0) {
						*error = g_error_new_literal (FLICKR_CONNECTION_ERROR,
									      atoi (dom_element_get_attribute (child, "code")),
									      dom_element_get_attribute (child, "msg"));
					}
				}

				g_object_unref (doc);
				return FALSE;
			}
		}
	}

	*doc_p = doc;

	return TRUE;
}
