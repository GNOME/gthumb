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
#include <glib.h>
#include <glib/gi18n.h>
#include <gthumb.h>
#include "photobucket-album.h"
#include "photobucket-consumer.h"
#include "photobucket-photo.h"
#include "photobucket-service.h"


typedef struct {
	PhotobucketAccount  *account;
	PhotobucketAlbum    *album;
	int                  size;
	gboolean             scramble;
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
	_g_object_unref (post_photos->album);
	g_object_unref (post_photos->account);
	g_free (post_photos);
}


struct _PhotobucketServicePrivate
{
	OAuthConnection *conn;
	PostPhotosData  *post_photos;
};


static gpointer parent_class = NULL;


static void
photobucket_service_finalize (GObject *object)
{
	PhotobucketService *self;

	self = PHOTOBUCKET_SERVICE (object);

	_g_object_unref (self->priv->conn);
	post_photos_data_free (self->priv->post_photos);

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
	self->priv->post_photos = NULL;
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
photobucket_service_new (OAuthConnection *conn)
{
	PhotobucketService *self;

	self = (PhotobucketService *) g_object_new (PHOTOBUCKET_TYPE_SERVICE, NULL);
	self->priv->conn = g_object_ref (conn);

	return self;
}


/* -- photobucket_service_get_albums -- */


static DomElement *
get_content_root (DomDocument *doc)
{
	DomElement *root;

	for (root = DOM_ELEMENT (doc)->first_child; root; root = root->next_sibling) {
		if (g_strcmp0 (root->tag_name, "response") == 0) {
			DomElement *child;

			for (child = root->first_child; child; child = child->next_sibling)
				if (g_strcmp0 (child->tag_name, "content") == 0)
					return child;
		}
	}

	g_assert_not_reached ();

	return NULL;
}


static void
read_albums_recursively (DomElement  *root,
			 GList      **albums)
{
	DomElement *node;

	for (node = root->first_child; node; node = node->next_sibling) {
		if (g_strcmp0 (node->tag_name, "album") == 0) {
			PhotobucketAlbum *album;

			album = photobucket_album_new ();
			dom_domizable_load_from_element (DOM_DOMIZABLE (album), node);
			*albums = g_list_prepend (*albums, album);

			if (atoi (dom_element_get_attribute (node, "subalbum_count")) > 0)
				read_albums_recursively (node, albums);
		}
	}
}


static void
get_albums_ready_cb (SoupSession *session,
		     SoupMessage *msg,
		     gpointer     user_data)
{
	PhotobucketService *self = user_data;
	GSimpleAsyncResult *result;
	DomDocument        *doc = NULL;
	GError             *error = NULL;

	result = oauth_connection_get_result (self->priv->conn);

	if (photobucket_utils_parse_response (msg, &doc, &error)) {
		GList *albums = NULL;

		read_albums_recursively (get_content_root (doc), &albums);
		albums = g_list_reverse (albums);
		g_simple_async_result_set_op_res_gpointer (result, albums, (GDestroyNotify) _g_object_list_unref);

		g_object_unref (doc);
	}
	else
		g_simple_async_result_set_from_error (result, error);

	g_simple_async_result_complete_in_idle (result);
}


void
photobucket_service_get_albums (PhotobucketService  *self,
			        PhotobucketAccount  *account,
			        GCancellable        *cancellable,
			        GAsyncReadyCallback  callback,
			        gpointer             user_data)
{
	GHashTable  *data_set;
	char        *url;
	SoupMessage *msg;

	g_return_if_fail (account != NULL);
	g_return_if_fail (account->subdomain != NULL);

	gth_task_progress (GTH_TASK (self->priv->conn), _("Getting the album list"), NULL, TRUE, 0.0);

	data_set = g_hash_table_new (g_str_hash, g_str_equal);
	g_hash_table_insert (data_set, "recurse", "true");
	g_hash_table_insert (data_set, "view", "nested");
	g_hash_table_insert (data_set, "media", "none");
	url = g_strconcat ("http://api.photobucket.com/album/", OAUTH_ACCOUNT (account)->username, NULL);
	oauth_connection_add_signature (self->priv->conn, "GET", url, data_set);
	g_free (url);

	url = g_strconcat ("http://", account->subdomain, "/album/", OAUTH_ACCOUNT (account)->username, NULL);
	msg = soup_form_request_new_from_hash ("GET", url, data_set);
	oauth_connection_send_message (self->priv->conn,
				       msg,
				       cancellable,
				       callback,
				       user_data,
				       photobucket_service_get_albums,
				       get_albums_ready_cb,
				       self);

	g_free (url);
	g_hash_table_destroy (data_set);
}


GList *
photobucket_service_get_albums_finish (PhotobucketService  *service,
				       GAsyncResult        *result,
				       GError             **error)
{
	if (g_simple_async_result_propagate_error (G_SIMPLE_ASYNC_RESULT (result), error))
		return NULL;
	else
		return _g_object_list_ref (g_simple_async_result_get_op_res_gpointer (G_SIMPLE_ASYNC_RESULT (result)));
}


/* -- photobucket_service_create_album -- */


typedef struct {
	PhotobucketService *service;
	PhotobucketAlbum   *album;
} CreateAlbumData;


static void
create_album_data_free (CreateAlbumData *create_album_data)
{
	g_object_unref (create_album_data->service);
	g_object_unref (create_album_data->album);
	g_free (create_album_data);
}


static void
create_album_ready_cb (SoupSession *session,
		       SoupMessage *msg,
		       gpointer     user_data)
{
	CreateAlbumData    *create_album_data = user_data;
	PhotobucketService *self = create_album_data->service;
	GSimpleAsyncResult *result;
	DomDocument        *doc = NULL;
	GError             *error = NULL;

	result = oauth_connection_get_result (self->priv->conn);

	if (photobucket_utils_parse_response (msg, &doc, &error)) {
		g_simple_async_result_set_op_res_gpointer (result, g_object_ref (create_album_data->album), g_object_unref);
		g_object_unref (doc);
	}
	else
		g_simple_async_result_set_from_error (result, error);

	g_simple_async_result_complete_in_idle (result);

	create_album_data_free (create_album_data);
}


void
photobucket_service_create_album (PhotobucketService  *self,
				  PhotobucketAccount  *account,
				  const char          *parent_album,
			          PhotobucketAlbum    *album,
			          GCancellable        *cancellable,
			          GAsyncReadyCallback  callback,
			          gpointer             user_data)
{
	CreateAlbumData *create_album_data;
	char            *path;
	GHashTable      *data_set;
	char            *identifier;
	char            *url;
	SoupMessage     *msg;

	g_return_if_fail (album != NULL);
	g_return_if_fail (album->name != NULL);

	create_album_data = g_new0 (CreateAlbumData, 1);
	create_album_data->service = g_object_ref (self);
	create_album_data->album = photobucket_album_new ();

	path = g_strconcat (parent_album, "/", album->name, NULL);
	photobucket_album_set_name (create_album_data->album, path);
	g_free (path);

	gth_task_progress (GTH_TASK (self->priv->conn), _("Creating the new album"), NULL, TRUE, 0.0);

	data_set = g_hash_table_new (g_str_hash, g_str_equal);
	g_hash_table_insert (data_set, "name", album->name);
	identifier = soup_uri_encode (parent_album, NULL);
	url = g_strconcat ("http://api.photobucket.com/album/", identifier, NULL);
	oauth_connection_add_signature (self->priv->conn, "POST", url, data_set);

	g_free (identifier);
	g_free (url);

	url = g_strconcat ("http://", account->subdomain, "/album/", parent_album, NULL);
	msg = soup_form_request_new_from_hash ("POST", url, data_set);
	oauth_connection_send_message (self->priv->conn,
				       msg,
				       cancellable,
				       callback,
				       user_data,
				       photobucket_service_create_album,
				       create_album_ready_cb,
				       create_album_data);

	g_free (url);
	g_hash_table_destroy (data_set);
}


PhotobucketAlbum *
photobucket_service_create_album_finish (PhotobucketService  *self,
				         GAsyncResult        *result,
				         GError             **error)
{
	if (g_simple_async_result_propagate_error (G_SIMPLE_ASYNC_RESULT (result), error))
		return NULL;
	else
		return g_object_ref (g_simple_async_result_get_op_res_gpointer (G_SIMPLE_ASYNC_RESULT (result)));
}


/* -- photobucket_service_upload_photos -- */


static void
upload_photos_done (PhotobucketService *self,
		    GError             *error)
{
	GSimpleAsyncResult *result;

	result = oauth_connection_get_result (self->priv->conn);
	if (error == NULL) {
		g_simple_async_result_set_op_res_gboolean (result, TRUE);
	}
	else {
		if (self->priv->post_photos->current != NULL) {
			GthFileData *file_data = self->priv->post_photos->current->data;
			char        *msg;

			msg = g_strdup_printf (_("Could not upload '%s': %s"), g_file_info_get_display_name (file_data->info), error->message);
			g_free (error->message);
			error->message = msg;
		}
		g_simple_async_result_set_from_error (result, error);
	}

	g_simple_async_result_complete_in_idle (result);
}


static void photobucket_service_upload_current_file (PhotobucketService *self);


static void
upload_photo_ready_cb (SoupSession *session,
		       SoupMessage *msg,
		       gpointer     user_data)
{
	PhotobucketService *self = user_data;
	GSimpleAsyncResult *result;
	DomDocument        *doc = NULL;
	GError             *error = NULL;
	GthFileData        *file_data;

	result = oauth_connection_get_result (self->priv->conn);
	if (! photobucket_utils_parse_response (msg, &doc, &error)) {
		upload_photos_done (self, error);
		return;
	}
	else
		g_object_unref (doc);

	file_data = self->priv->post_photos->current->data;
	self->priv->post_photos->uploaded_size += g_file_info_get_size (file_data->info);
	self->priv->post_photos->current = self->priv->post_photos->current->next;
	photobucket_service_upload_current_file (self);
}


static void
upload_photo_file_buffer_ready_cb (void     **buffer,
				   gsize      count,
				   GError    *error,
				   gpointer   user_data)
{
	PhotobucketService *self = user_data;
	GthFileData        *file_data;
	SoupMultipart      *multipart;
	char               *identifier;
	char               *uri;
	SoupBuffer         *body;
	char               *url;
	SoupMessage        *msg;

	if (error != NULL) {
		upload_photos_done (self, error);
		return;
	}

	file_data = self->priv->post_photos->current->data;
	multipart = soup_multipart_new ("multipart/form-data");
	identifier = soup_uri_encode (self->priv->post_photos->album->name, NULL);

	/* the metadata part */

	{
		GHashTable *data_set;
		char       *title;
		char       *description;
		char       *url;
		char       *s_size = NULL;
		GList      *keys;
		GList      *scan;

		data_set = g_hash_table_new (g_str_hash, g_str_equal);
		g_hash_table_insert (data_set, "type", "image");
		title = gth_file_data_get_attribute_as_string (file_data, "general::title");
		if (title != NULL)
			g_hash_table_insert (data_set, "title", title);
		description = gth_file_data_get_attribute_as_string (file_data, "general::description");
		if (description != NULL)
			g_hash_table_insert (data_set, "description", description);
		if (self->priv->post_photos->size != 0) {
			s_size = g_strdup_printf ("%d", self->priv->post_photos->size);
			g_hash_table_insert (data_set, "size", s_size);
		}
		if (self->priv->post_photos->scramble)
			g_hash_table_insert (data_set, "scramble", "true");
		url = g_strconcat ("http://api.photobucket.com", "/album/", identifier, "/upload", NULL);
		oauth_connection_add_signature (self->priv->conn, "POST", url, data_set);

		keys = g_hash_table_get_keys (data_set);
		for (scan = keys; scan; scan = scan->next) {
			char *key = scan->data;
			soup_multipart_append_form_string (multipart, key, g_hash_table_lookup (data_set, key));
		}

		g_list_free (keys);
		g_free (url);
		g_free (s_size);
		g_hash_table_unref (data_set);
	}

	/* the file part */

	uri = g_file_get_uri (file_data->file);
	body = soup_buffer_new (SOUP_MEMORY_TEMPORARY, *buffer, count);
	soup_multipart_append_form_file (multipart,
					 "uploadfile",
					 _g_uri_get_basename (uri),
					 gth_file_data_get_mime_type (file_data),
					 body);

	soup_buffer_free (body);
	g_free (uri);

	/* send the file */

	{
		char *details;

		/* Translators: %s is a filename */
		details = g_strdup_printf (_("Uploading '%s'"), g_file_info_get_display_name (file_data->info));
		gth_task_progress (GTH_TASK (self->priv->conn),
				   NULL, details,
				   FALSE,
				   (double) (self->priv->post_photos->uploaded_size + (g_file_info_get_size (file_data->info) / 2.0)) / self->priv->post_photos->total_size);

		g_free (details);
	}

	url = g_strconcat ("http://", self->priv->post_photos->account->subdomain, "/album/", identifier, "/upload", NULL);
	msg = soup_form_request_new_from_multipart (url, multipart);
	oauth_connection_send_message (self->priv->conn,
				       msg,
				       self->priv->post_photos->cancellable,
				       self->priv->post_photos->callback,
				       self->priv->post_photos->user_data,
				       photobucket_service_upload_photos,
				       upload_photo_ready_cb,
				       self);

	g_free (url);
	soup_multipart_free (multipart);
}


static void
photobucket_service_upload_current_file (PhotobucketService *self)
{
	GthFileData *file_data;

	if (self->priv->post_photos->current == NULL) {
		upload_photos_done (self, NULL);
		return;
	}

	file_data = self->priv->post_photos->current->data;
	g_load_file_async (file_data->file,
			   G_PRIORITY_DEFAULT,
			   self->priv->post_photos->cancellable,
			   upload_photo_file_buffer_ready_cb,
			   self);
}


static void
upload_photos_info_ready_cb (GList    *files,
		             GError   *error,
		             gpointer  user_data)
{
	PhotobucketService *self = user_data;
	GList              *scan;

	if (error != NULL) {
		upload_photos_done (self, error);
		return;
	}

	self->priv->post_photos->file_list = _g_object_list_ref (files);
	for (scan = self->priv->post_photos->file_list; scan; scan = scan->next) {
		GthFileData *file_data = scan->data;

		self->priv->post_photos->total_size += g_file_info_get_size (file_data->info);
		self->priv->post_photos->n_files += 1;
	}

	self->priv->post_photos->current = self->priv->post_photos->file_list;
	photobucket_service_upload_current_file (self);
}


void
photobucket_service_upload_photos (PhotobucketService  *self,
				   PhotobucketAccount  *account,
				   PhotobucketAlbum    *album,
				   int                  size,
				   gboolean             scramble,
				   GList               *file_list, /* GFile list */
				   GCancellable        *cancellable,
				   GAsyncReadyCallback  callback,
				   gpointer             user_data)
{
	gth_task_progress (GTH_TASK (self->priv->conn), _("Uploading the files to the server"), NULL, TRUE, 0.0);

	post_photos_data_free (self->priv->post_photos);
	self->priv->post_photos = g_new0 (PostPhotosData, 1);
	self->priv->post_photos->account = g_object_ref (account);
	self->priv->post_photos->album = _g_object_ref (album);
	self->priv->post_photos->size = size;
	self->priv->post_photos->scramble = scramble;
	self->priv->post_photos->cancellable = _g_object_ref (cancellable);
	self->priv->post_photos->callback = callback;
	self->priv->post_photos->user_data = user_data;
	self->priv->post_photos->total_size = 0;
	self->priv->post_photos->n_files = 0;

	_g_query_all_metadata_async (file_list,
				     GTH_LIST_DEFAULT,
				     "*",
				     self->priv->post_photos->cancellable,
				     upload_photos_info_ready_cb,
				     self);
}


gboolean
photobucket_service_upload_photos_finish (PhotobucketService  *self,
				          GAsyncResult        *result,
				          GError             **error)
{
	if (g_simple_async_result_propagate_error (G_SIMPLE_ASYNC_RESULT (result), error))
		return FALSE;
	else
		return TRUE;
}
