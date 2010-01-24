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
#include "picasa-web-album.h"
#include "picasa-web-service.h"


struct _PicasaWebServicePrivate
{
	GoogleConnection *conn;
	char             *user_id;
};


static gpointer parent_class = NULL;


static void
picasa_web_service_finalize (GObject *object)
{
	PicasaWebService *self;

	self = PICASA_WEB_SERVICE (object);
	_g_object_unref (self->priv->conn);
	g_free (self->priv->user_id);

	G_OBJECT_CLASS (parent_class)->finalize (object);
}


static void
picasa_web_service_class_init (PicasaWebServiceClass *klass)
{
	GObjectClass *object_class;

	parent_class = g_type_class_peek_parent (klass);
	g_type_class_add_private (klass, sizeof (PicasaWebServicePrivate));

	object_class = (GObjectClass*) klass;
	object_class->finalize = picasa_web_service_finalize;
}


static void
picasa_web_service_init (PicasaWebService *self)
{
	self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, PICASA_TYPE_WEB_SERVICE, PicasaWebServicePrivate);
	self->priv->conn = NULL;
	self->priv->user_id = g_strdup ("default");
}


GType
picasa_web_service_get_type (void)
{
	static GType type = 0;

	if (! type) {
		GTypeInfo type_info = {
			sizeof (PicasaWebServiceClass),
			NULL,
			NULL,
			(GClassInitFunc) picasa_web_service_class_init,
			NULL,
			NULL,
			sizeof (PicasaWebService),
			0,
			(GInstanceInitFunc) picasa_web_service_init
		};

		type = g_type_register_static (G_TYPE_OBJECT,
					       "PicasaWebService",
					       &type_info,
					       0);
	}

	return type;
}


PicasaWebService *
picasa_web_service_new (GoogleConnection *conn)
{
	PicasaWebService *self;

	self = (PicasaWebService *) g_object_new (PICASA_TYPE_WEB_SERVICE, NULL);
	self->priv->conn = g_object_ref (conn);

	return self;
}


/* -- picasa_web_service_list_albums -- */


static void
list_albums_ready_cb (SoupSession *session,
		      SoupMessage *msg,
		      gpointer     user_data)
{
	PicasaWebService   *self = user_data;
	GSimpleAsyncResult *result;
	SoupBuffer         *body;
	DomDocument        *doc;
	GError             *error = NULL;

	result = google_connection_get_result (self->priv->conn);

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
	doc = dom_document_new ();
	if (dom_document_load (doc, body->data, body->length, &error)) {
		DomElement *feed_node;
		GList      *albums = NULL;

		feed_node = DOM_ELEMENT (doc)->first_child;
		while ((feed_node != NULL) && g_strcmp0 (feed_node->tag_name, "feed") != 0)
			feed_node = feed_node->next_sibling;

		if (feed_node != NULL) {
			DomElement     *node;
			PicasaWebAlbum *album = NULL;

			for (node = feed_node->first_child;
			     node != NULL;
			     node = node->next_sibling)
			{
				if (g_strcmp0 (node->tag_name, "id") == 0) { /* get the user id */
					char *user_id;

					user_id = strrchr (dom_element_get_inner_text (node), '/');
					if (user_id != NULL) {
						g_free (self->priv->user_id);
						self->priv->user_id = g_strdup (user_id + 1);
					}
				}
				else if (g_strcmp0 (node->tag_name, "entry") == 0) { /* read the album data */
					if (album != NULL)
						albums = g_list_prepend (albums, album);
					album = picasa_web_album_new ();
					dom_domizable_load_from_element (DOM_DOMIZABLE (album), node);
				}
			}
			if (album != NULL)
				albums = g_list_prepend (albums, album);
		}
		albums = g_list_reverse (albums);
		g_simple_async_result_set_op_res_gpointer (result, albums, (GDestroyNotify) _g_object_list_unref);
	}
	else {
		g_simple_async_result_set_from_error (result, error);
		g_error_free (error);
	}
	g_simple_async_result_complete_in_idle (result);

	g_object_unref (doc);
	soup_buffer_free (body);
}


void
picasa_web_service_list_albums (PicasaWebService    *self,
			        const char          *user_id,
			        GCancellable        *cancellable,
			        GAsyncReadyCallback  callback,
			        gpointer             user_data)
{
	char        *url;
	SoupMessage *msg;

	g_return_if_fail (user_id != NULL);

	url = g_strconcat ("http://picasaweb.google.com/data/feed/api/user/", user_id, NULL);
	msg = soup_message_new ("GET", url);
	google_connection_send_message (self->priv->conn,
					msg,
					cancellable,
					callback,
					user_data,
					picasa_web_service_list_albums,
					list_albums_ready_cb,
					self);

	g_free (url);
}


GList *
picasa_web_service_list_albums_finish (PicasaWebService  *service,
				       GAsyncResult      *result,
				       GError           **error)
{
	if (g_simple_async_result_propagate_error (G_SIMPLE_ASYNC_RESULT (result), error))
		return NULL;
	else
		return g_simple_async_result_get_op_res_gpointer (G_SIMPLE_ASYNC_RESULT (result));
}


/* -- picasa_web_service_create_album -- */


static void
create_album_ready_cb (SoupSession *session,
		       SoupMessage *msg,
		       gpointer     user_data)
{
	PicasaWebService   *self = user_data;
	GSimpleAsyncResult *result;
	SoupBuffer         *body;
	DomDocument        *doc;
	GError             *error = NULL;

	result = google_connection_get_result (self->priv->conn);

	if (msg->status_code != 201) {
		g_simple_async_result_set_error (result,
						 SOUP_HTTP_ERROR,
						 msg->status_code,
						 "%s",
						 soup_status_get_phrase (msg->status_code));
		g_simple_async_result_complete_in_idle (result);
		return;
	}

	body = soup_message_body_flatten (msg->response_body);
	doc = dom_document_new ();
	if (dom_document_load (doc, body->data, body->length, &error)) {
		PicasaWebAlbum *album;

		album = picasa_web_album_new ();
		dom_domizable_load_from_element (DOM_DOMIZABLE (album), DOM_ELEMENT (doc)->first_child);
		g_simple_async_result_set_op_res_gpointer (result, album, (GDestroyNotify) _g_object_list_unref);
	}
	else {
		g_simple_async_result_set_from_error (result, error);
		g_error_free (error);
	}
	g_simple_async_result_complete_in_idle (result);

	g_object_unref (doc);
	soup_buffer_free (body);
}


void
picasa_web_service_create_album (PicasaWebService     *self,
				 PicasaWebAlbum       *album,
				 GCancellable         *cancellable,
				 GAsyncReadyCallback   callback,
				 gpointer              user_data)
{
	DomDocument *doc;
	DomElement  *entry;
	char        *buffer;
	gsize        len;
	char        *url;
	SoupMessage *msg;

	g_return_if_fail (self->priv->user_id != NULL);

	doc = dom_document_new ();
	entry = dom_domizable_create_element (DOM_DOMIZABLE (album), doc);
	dom_element_set_attribute (entry, "xmlns", "http://www.w3.org/2005/Atom");
	dom_element_set_attribute (entry, "xmlns:media", "http://search.yahoo.com/mrss/");
	dom_element_set_attribute (entry, "xmlns:gphoto", "http://schemas.google.com/photos/2007");
	dom_element_append_child (DOM_ELEMENT (doc), entry);
	buffer = dom_document_dump (doc, &len);

	url = g_strconcat ("http://picasaweb.google.com/data/feed/api/user/", self->priv->user_id, NULL);
	msg = soup_message_new ("POST", url);
	soup_message_set_request (msg, ATOM_ENTRY_MIME_TYPE, SOUP_MEMORY_TAKE, buffer, len);
	google_connection_send_message (self->priv->conn,
					msg,
					cancellable,
					callback,
					user_data,
					picasa_web_service_create_album,
					create_album_ready_cb,
					self);

	g_free (url);
	g_object_unref (doc);
}


PicasaWebAlbum *
picasa_web_service_create_album_finish (PicasaWebService  *service,
					GAsyncResult      *result,
					GError           **error)
{
	if (g_simple_async_result_propagate_error (G_SIMPLE_ASYNC_RESULT (result), error))
		return NULL;
	else
		return g_simple_async_result_get_op_res_gpointer (G_SIMPLE_ASYNC_RESULT (result));
}


/* utilities */


GList *
picasa_web_accounts_load_from_file (void)
{
	GList       *accounts = NULL;
	char        *filename;
	char        *buffer;
	gsize        len;
	DomDocument *doc;

	filename = gth_user_dir_get_file (GTH_DIR_CONFIG, GTHUMB_DIR, "accounts", "picasaweb.xml", NULL);
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
					const char *value;

					value = dom_element_get_attribute (child, "email");
					if (value != NULL)
						accounts = g_list_prepend (accounts, g_strdup (value));
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


void
picasa_web_accounts_save_to_file (GList *accounts)
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
	for (scan = accounts; scan; scan = scan->next)
		dom_element_append_child (root,
					  dom_document_create_element (doc, "account",
								       "email", (char *) scan->data,
								       NULL));

	gth_user_dir_make_dir_for_file (GTH_DIR_CONFIG, GTHUMB_DIR, "accounts", "picasaweb.xml", NULL);
	filename = gth_user_dir_get_file (GTH_DIR_CONFIG, GTHUMB_DIR, "accounts", "picasaweb.xml", NULL);
	file = g_file_new_for_path (filename);
	buffer = dom_document_dump (doc, &len);
	g_write_file (file, FALSE, G_FILE_CREATE_PRIVATE|G_FILE_CREATE_REPLACE_DESTINATION, buffer, len, NULL, NULL);

	g_free (buffer);
	g_object_unref (file);
	g_free (filename);
	g_object_unref (doc);
}
