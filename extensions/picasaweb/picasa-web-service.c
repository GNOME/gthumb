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
#include "picasa-web-photo.h"
#include "picasa-web-service.h"


typedef struct {
	PicasaWebAlbum      *album;
	GList               *file_list;
	GCancellable        *cancellable;
        GAsyncReadyCallback  callback;
        gpointer             user_data;
	GList               *current;
	goffset              total_size;
	goffset              uploaded_size;
	int                  n_files;
	int                  uploaded_files;
} PostPhotosData;


static void
post_photos_data_free (PostPhotosData *post_photos)
{
	if (post_photos == NULL)
		return;
	_g_object_unref (post_photos->cancellable);
	_g_object_list_unref (post_photos->file_list);
	g_object_unref (post_photos->album);
	g_free (post_photos);
}


struct _PicasaWebServicePrivate
{
	GoogleConnection *conn;
	PicasaWebUser    *user;
	PostPhotosData   *post_photos;
};


static gpointer parent_class = NULL;


static void
picasa_web_service_finalize (GObject *object)
{
	PicasaWebService *self;

	self = PICASA_WEB_SERVICE (object);

	_g_object_unref (self->priv->conn);
	_g_object_unref (self->priv->user);
	post_photos_data_free (self->priv->post_photos);

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
	self->priv->user = NULL;
	self->priv->post_photos = NULL;
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


PicasaWebUser *
picasa_web_service_get_user (PicasaWebService *self)
{
	return self->priv->user;
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
			PicasaWebAlbum *album;

			self->priv->user = picasa_web_user_new ();
			dom_domizable_load_from_element (DOM_DOMIZABLE (self->priv->user), feed_node);

			album = NULL;
			for (node = feed_node->first_child;
			     node != NULL;
			     node = node->next_sibling)
			{
				if (g_strcmp0 (node->tag_name, "entry") == 0) { /* read the album data */
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

	gth_task_progress (GTH_TASK (self->priv->conn), _("Getting the album list"), NULL, TRUE, 0.0);

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
		return _g_object_list_ref (g_simple_async_result_get_op_res_gpointer (G_SIMPLE_ASYNC_RESULT (result)));
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
		g_simple_async_result_set_op_res_gpointer (result, album, (GDestroyNotify) g_object_unref);
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

	g_return_if_fail (self->priv->user != NULL);

	gth_task_progress (GTH_TASK (self->priv->conn), _("Creating the new album"), NULL, TRUE, 0.0);

	doc = dom_document_new ();
	entry = dom_domizable_create_element (DOM_DOMIZABLE (album), doc);
	dom_element_set_attribute (entry, "xmlns", "http://www.w3.org/2005/Atom");
	dom_element_set_attribute (entry, "xmlns:media", "http://search.yahoo.com/mrss/");
	dom_element_set_attribute (entry, "xmlns:gphoto", "http://schemas.google.com/photos/2007");
	dom_element_append_child (DOM_ELEMENT (doc), entry);
	buffer = dom_document_dump (doc, &len);

	url = g_strconcat ("http://picasaweb.google.com/data/feed/api/user/", self->priv->user->id, NULL);
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
		return g_object_ref (g_simple_async_result_get_op_res_gpointer (G_SIMPLE_ASYNC_RESULT (result)));
}


/* -- picasa_web_service_post_photos -- */


static void
post_photos_done (PicasaWebService *self,
		  GError           *error)
{
	GSimpleAsyncResult *result;

	result = google_connection_get_result (self->priv->conn);
	if (error == NULL)
		g_simple_async_result_set_op_res_gboolean (result, TRUE);
	else
		g_simple_async_result_set_from_error (result, error);
	g_simple_async_result_complete_in_idle (result);
}


static void picasa_wev_service_post_current_file (PicasaWebService *self);


static void
post_photo_ready_cb (SoupSession *session,
		     SoupMessage *msg,
		     gpointer     user_data)
{
	PicasaWebService *self = user_data;

	if (msg->status_code != 201) {
		GError *error;

		error = g_error_new (SOUP_HTTP_ERROR, msg->status_code, "%s", soup_status_get_phrase (msg->status_code));
		post_photos_done (self, error);
		g_error_free (error);

		return;
	}

	self->priv->post_photos->current = self->priv->post_photos->current->next;
	picasa_wev_service_post_current_file (self);
}


static void
post_photo_file_buffer_ready_cb (void     *buffer,
				 gsize     count,
				 GError   *error,
				 gpointer  user_data)
{
	PicasaWebService   *self = user_data;
	GthFileData        *file_data;
	SoupMultipart      *multipart;
	const char         *filename;
	char               *value;
	GObject            *metadata;
	DomDocument        *doc;
	DomElement         *entry;
	char               *entry_buffer;
	gsize               entry_len;
	SoupMessageHeaders *headers;
	SoupBuffer         *body;
	char               *details;
	char               *url;
	SoupMessage        *msg;

	if (error != NULL) {
		post_photos_done (self, error);
		return;
	}

	file_data = self->priv->post_photos->current->data;
	multipart = soup_multipart_new ("multipart/related");

	/* the metadata part */

	doc = dom_document_new ();
	entry = dom_document_create_element (doc, "entry",
					     "xmlns", "http://www.w3.org/2005/Atom",
					     "xmlns:gphoto", "http://schemas.google.com/photos/2007",
					     "xmlns:media", "http://search.yahoo.com/mrss/",
					     NULL);

	filename = g_file_info_get_display_name (file_data->info);
	dom_element_append_child (entry,
				  dom_document_create_element_with_text (doc, filename, "title", NULL));

	value = gth_file_data_get_attribute_as_string (file_data, "general::description");
	if (value == NULL)
		value = gth_file_data_get_attribute_as_string (file_data, "general::title");
	dom_element_append_child (entry,
				  dom_document_create_element_with_text (doc, value, "summary", NULL));

	value = gth_file_data_get_attribute_as_string (file_data, "general::location");
	if (value != NULL)
		dom_element_append_child (entry,
					  dom_document_create_element_with_text (doc, value, "gphoto:location", NULL));

	metadata = g_file_info_get_attribute_object (file_data->info, "general::tags");
	if ((metadata != NULL) && GTH_IS_STRING_LIST (metadata))
		value = gth_string_list_join (GTH_STRING_LIST (metadata), ", ");
	if (value != NULL) {
		DomElement *group;

		group = dom_document_create_element (doc, "media:group", NULL);
		dom_element_append_child (group,
					  dom_document_create_element_with_text (doc, value, "media:keywords", NULL));
		dom_element_append_child (entry, group);

		g_free (value);
	}

	dom_element_append_child (entry,
				  dom_document_create_element (doc, "category",
							       "scheme", "http://schemas.google.com/g/2005#kind",
							       "term", "http://schemas.google.com/photos/2007#photo",
							       NULL));
	dom_element_append_child (DOM_ELEMENT (doc), entry);
	entry_buffer = dom_document_dump (doc, &entry_len);

	headers = soup_message_headers_new (SOUP_MESSAGE_HEADERS_REQUEST);
	soup_message_headers_append (headers, "Content-Type", "application/atom+xml");
	body = soup_buffer_new (SOUP_MEMORY_TAKE, entry_buffer, entry_len);
	soup_multipart_append_part (multipart, headers, body);

	soup_buffer_free (body);
	soup_message_headers_free (headers);
	g_object_unref (doc);

	/* the file part */

	body = soup_buffer_new (SOUP_MEMORY_TEMPORARY, buffer, count);
	soup_multipart_append_form_file (multipart,
					 "file",
					 NULL,
					 gth_file_data_get_mime_type (file_data),
					 body);

	soup_buffer_free (body);

	/* send the file */

	/* Translators: %s is a filename */
	details = g_strdup_printf (_("Uploading '%s'"), filename);
	gth_task_progress (GTH_TASK (self->priv->conn), NULL, details, TRUE, 0.0);
	g_free (details);

	url = g_strconcat ("http://picasaweb.google.com/data/feed/api/user/",
			   self->priv->user->id,
			   "/albumid/",
			   self->priv->post_photos->album->id,
			   NULL);
	msg = soup_form_request_new_from_multipart (url, multipart);
	google_connection_send_message (self->priv->conn,
					msg,
					self->priv->post_photos->cancellable,
					self->priv->post_photos->callback,
					self->priv->post_photos->user_data,
					picasa_web_service_post_photos,
					post_photo_ready_cb,
					self);

	g_free (url);
	soup_multipart_free (multipart);
}


static void
picasa_wev_service_post_current_file (PicasaWebService *self)
{
	GthFileData *file_data;

	if (self->priv->post_photos->current == NULL) {
		post_photos_done (self, NULL);
		return;
	}

	file_data = self->priv->post_photos->current->data;
	g_load_file_async (file_data->file,
			   G_PRIORITY_DEFAULT,
			   self->priv->post_photos->cancellable,
			   post_photo_file_buffer_ready_cb,
			   self);
}


static void
post_photos_info_ready_cb (GList    *files,
		           GError   *error,
		           gpointer  user_data)
{
	PicasaWebService *self = user_data;

	if (error != NULL) {
		post_photos_done (self, error);
		return;
	}

	self->priv->post_photos->file_list = _g_object_list_ref (files);
	self->priv->post_photos->current = self->priv->post_photos->file_list;
	picasa_wev_service_post_current_file (self);
}


void
picasa_web_service_post_photos (PicasaWebService    *self,
			        PicasaWebAlbum      *album,
			        GList               *file_list, /* GFile list */
			        GCancellable        *cancellable,
			        GAsyncReadyCallback  callback,
			        gpointer             user_data)
{
	GList *scan;

	g_return_if_fail (album != NULL);
	g_return_if_fail (self->priv->post_photos == NULL);

	gth_task_progress (GTH_TASK (self->priv->conn), _("Uploading the files to the server"), NULL, TRUE, 0.0);

	self->priv->post_photos = g_new0 (PostPhotosData, 1);
	self->priv->post_photos->album = g_object_ref (album);
	self->priv->post_photos->cancellable = _g_object_ref (cancellable);
	self->priv->post_photos->callback = callback;
	self->priv->post_photos->user_data = user_data;
	self->priv->post_photos->total_size = 0;
	self->priv->post_photos->n_files = 0;
	for (scan = self->priv->post_photos->file_list; scan; scan = scan->next) {
		GthFileData *file_data = scan->data;

		self->priv->post_photos->total_size += g_file_info_get_size (file_data->info);
		self->priv->post_photos->n_files += 1;
	}

	_g_query_all_metadata_async (file_list,
				     FALSE,
				     TRUE,
				     "*",
				     self->priv->post_photos->cancellable,
				     post_photos_info_ready_cb,
				     self);
}


gboolean
picasa_web_service_post_photos_finish (PicasaWebService  *self,
				       GAsyncResult      *result,
				       GError           **error)
{
	if (g_simple_async_result_propagate_error (G_SIMPLE_ASYNC_RESULT (result), error))
		return FALSE;
	else
		return TRUE;
}


/* -- picasa_web_service_list_photos -- */


static void
list_photos_ready_cb (SoupSession *session,
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
		GList      *photos = NULL;

		feed_node = DOM_ELEMENT (doc)->first_child;
		while ((feed_node != NULL) && g_strcmp0 (feed_node->tag_name, "feed") != 0)
			feed_node = feed_node->next_sibling;

		if (feed_node != NULL) {
			DomElement     *node;
			PicasaWebPhoto *photo;

			self->priv->user = picasa_web_user_new ();
			dom_domizable_load_from_element (DOM_DOMIZABLE (self->priv->user), feed_node);

			photo = NULL;
			for (node = feed_node->first_child;
			     node != NULL;
			     node = node->next_sibling)
			{
				if (g_strcmp0 (node->tag_name, "entry") == 0) { /* read the photo data */
					if (photo != NULL)
						photos = g_list_prepend (photos, photo);
					photo = picasa_web_photo_new ();
					dom_domizable_load_from_element (DOM_DOMIZABLE (photo), node);
				}
			}
			if (photo != NULL)
				photos = g_list_prepend (photos, photo);
		}
		photos = g_list_reverse (photos);
		g_simple_async_result_set_op_res_gpointer (result, photos, (GDestroyNotify) _g_object_list_unref);
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
picasa_web_service_list_photos (PicasaWebService    *self,
				PicasaWebAlbum      *album,
				GCancellable        *cancellable,
				GAsyncReadyCallback  callback,
				gpointer             user_data)
{
	char        *url;
	SoupMessage *msg;

	g_return_if_fail (album != NULL);

	gth_task_progress (GTH_TASK (self->priv->conn), _("Getting the photo list"), NULL, TRUE, 0.0);

	url = g_strconcat ("http://picasaweb.google.com/data/feed/api/user/",
			   self->priv->user->id,
			   "/albumid/",
			   album->id,
			   NULL);
	msg = soup_message_new ("GET", url);
	google_connection_send_message (self->priv->conn,
					msg,
					cancellable,
					callback,
					user_data,
					picasa_web_service_list_photos,
					list_photos_ready_cb,
					self);

	g_free (url);
}


GList *
picasa_web_service_list_photos_finish (PicasaWebService  *self,
				       GAsyncResult      *result,
				       GError           **error)
{
	if (g_simple_async_result_propagate_error (G_SIMPLE_ASYNC_RESULT (result), error))
		return NULL;
	else
		return _g_object_list_ref (g_simple_async_result_get_op_res_gpointer (G_SIMPLE_ASYNC_RESULT (result)));
}


/* utilities */


GList *
picasa_web_accounts_load_from_file (char **_default)
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
					if ((_default != NULL)  && (g_strcmp0 (dom_element_get_attribute (child, "default"), "1") == 0))
						*_default = g_strdup (value);
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
picasa_web_accounts_save_to_file (GList      *accounts,
				  const char *_default)
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
		const char *email = scan->data;
		DomElement *node;

		node = dom_document_create_element (doc, "account",
						    "email", email,
						    NULL);
		if (g_strcmp0 (email, _default) == 0)
			dom_element_set_attribute (node, "default", "1");
		dom_element_append_child (root, node);
	}

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
