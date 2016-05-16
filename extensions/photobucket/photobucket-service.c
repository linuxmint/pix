/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2010-2012 Free Software Foundation, Inc.
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


G_DEFINE_TYPE (PhotobucketService, photobucket_service, OAUTH_TYPE_SERVICE)


typedef struct {
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
	goffset              wrote_body_data_size;
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
	g_free (post_photos);
}


struct _PhotobucketServicePrivate {
	PostPhotosData  *post_photos;
};


static void
photobucket_service_finalize (GObject *object)
{
	PhotobucketService *self;

	self = PHOTOBUCKET_SERVICE (object);

	post_photos_data_free (self->priv->post_photos);

	G_OBJECT_CLASS (photobucket_service_parent_class)->finalize (object);
}


/* -- flickr_service_get_user_info -- */


static void
get_user_info_ready_cb (SoupSession *session,
			SoupMessage *msg,
			gpointer     user_data)
{
	PhotobucketService *self = user_data;
	GSimpleAsyncResult *result;
	DomDocument        *doc = NULL;
	GError             *error = NULL;

	result = _web_service_get_result (WEB_SERVICE (self));

	if (msg->status_code != 200) {
		g_simple_async_result_set_error (result,
						 SOUP_HTTP_ERROR,
						 msg->status_code,
						 "%s",
						 soup_status_get_phrase (msg->status_code));
		g_simple_async_result_complete_in_idle (result);
		return;
	}

	if (photobucket_utils_parse_response (msg, &doc, &error)) {
		OAuthAccount *account;
		gboolean      success;
		DomElement   *node;

		account = web_service_get_current_account (WEB_SERVICE (self));
		success = FALSE;

		for (node = DOM_ELEMENT (doc)->first_child; node; node = node->next_sibling) {
			if (g_strcmp0 (node->tag_name, "response") == 0) {
				DomElement *child;

				for (child = DOM_ELEMENT (node)->first_child; child; child = child->next_sibling) {
					if (g_strcmp0 (child->tag_name, "content") == 0) {
						success = TRUE;
						dom_domizable_load_from_element (DOM_DOMIZABLE (account), child);
						g_simple_async_result_set_op_res_gpointer (result, account, (GDestroyNotify) g_object_unref);
						break;
					}
				}
				break;
			}
		}

		if (! success) {
			error = g_error_new_literal (WEB_SERVICE_ERROR, WEB_SERVICE_ERROR_GENERIC, _("Unknown error"));
			g_simple_async_result_set_from_error (result, error);
		}

		g_object_unref (doc);
	}
	else
		g_simple_async_result_set_from_error (result, error);

	g_simple_async_result_complete_in_idle (result);
}


static void
photobucket_service_get_user_info (WebService          *base,
				   GCancellable        *cancellable,
				   GAsyncReadyCallback  callback,
				   gpointer             user_data)
{
	PhotobucketService *self = PHOTOBUCKET_SERVICE (base);
	OAuthAccount       *account;
	char               *url;
	GHashTable         *data_set;
	SoupMessage        *msg;

	account = web_service_get_current_account (WEB_SERVICE (self));
	g_return_if_fail (account != NULL);

	url = g_strconcat ("http://api.photobucket.com/user/", account->username, NULL);

	data_set = g_hash_table_new (g_str_hash, g_str_equal);
	oauth_service_add_signature (OAUTH_SERVICE (self), "GET", url, data_set);
	msg = soup_form_request_new_from_hash ("GET", url, data_set);
	_web_service_send_message (WEB_SERVICE (self),
				   msg,
				   cancellable,
				   callback,
				   user_data,
				   photobucket_service_get_user_info,
				   get_user_info_ready_cb,
				   self);

	g_hash_table_destroy (data_set);
	g_free (url);
}


static void
photobucket_service_class_init (PhotobucketServiceClass *klass)
{
	GObjectClass    *object_class;
	WebServiceClass *service_class;

	g_type_class_add_private (klass, sizeof (PhotobucketServicePrivate));

	object_class = (GObjectClass*) klass;
	object_class->finalize = photobucket_service_finalize;

	service_class = (WebServiceClass*) klass;
	service_class->get_user_info = photobucket_service_get_user_info;
}


static void
photobucket_service_init (PhotobucketService *self)
{
	self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, PHOTOBUCKET_TYPE_SERVICE, PhotobucketServicePrivate);
	self->priv->post_photos = NULL;
}


PhotobucketService *
photobucket_service_new (GCancellable  *cancellable,
			 GtkWidget     *browser,
			 GtkWidget     *dialog)
{
	return g_object_new (PHOTOBUCKET_TYPE_SERVICE,
			     "service-name", "photobucket",
			     "service-address", "www.photobucket.com",
			     "service-protocol", "http",
			     "account-type", PHOTOBUCKET_TYPE_ACCOUNT,
			     "consumer", &photobucket_consumer,
			     "cancellable", cancellable,
			     "browser", browser,
			     "dialog", dialog,
			     NULL);
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

	result = _web_service_get_result (WEB_SERVICE (self));

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
			        GCancellable        *cancellable,
			        GAsyncReadyCallback  callback,
			        gpointer             user_data)
{
	OAuthAccount *account;
	GHashTable   *data_set;
	char         *url;
	SoupMessage  *msg;

	account = web_service_get_current_account (WEB_SERVICE (self));
	g_return_if_fail (account != NULL);
	g_return_if_fail (PHOTOBUCKET_ACCOUNT (account)->subdomain != NULL);

	gth_task_progress (GTH_TASK (self),
			   _("Getting the album list"),
			   NULL,
			   TRUE,
			   0.0);

	data_set = g_hash_table_new (g_str_hash, g_str_equal);
	g_hash_table_insert (data_set, "recurse", "true");
	g_hash_table_insert (data_set, "view", "nested");
	g_hash_table_insert (data_set, "media", "none");
	url = g_strconcat ("http://api.photobucket.com/album/", account->username, NULL);
	oauth_service_add_signature (OAUTH_SERVICE (self), "GET", url, data_set);
	g_free (url);

	url = g_strconcat ("http://", PHOTOBUCKET_ACCOUNT (account)->subdomain, "/album/", account->username, NULL);
	msg = soup_form_request_new_from_hash ("GET", url, data_set);
	_web_service_send_message (WEB_SERVICE (self),
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

	result = _web_service_get_result (WEB_SERVICE (self));

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
	OAuthAccount    *account;
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

	gth_task_progress (GTH_TASK (self),
			   _("Creating the new album"),
			   NULL,
			   TRUE,
			   0.0);

	data_set = g_hash_table_new (g_str_hash, g_str_equal);
	g_hash_table_insert (data_set, "name", album->name);
	identifier = soup_uri_encode (parent_album, NULL);
	url = g_strconcat ("http://api.photobucket.com/album/", identifier, NULL);
	oauth_service_add_signature (OAUTH_SERVICE (self), "POST", url, data_set);

	g_free (identifier);
	g_free (url);

	account = web_service_get_current_account (WEB_SERVICE (self));

	url = g_strconcat ("http://", PHOTOBUCKET_ACCOUNT (account)->subdomain, "/album/", parent_album, NULL);
	msg = soup_form_request_new_from_hash ("POST", url, data_set);
	_web_service_send_message (WEB_SERVICE (self),
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

	result = _web_service_get_result (WEB_SERVICE (self));
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
	DomDocument        *doc = NULL;
	GError             *error = NULL;
	GthFileData        *file_data;

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
upload_photo_wrote_body_data_cb (SoupMessage *msg,
                		 SoupBuffer  *chunk,
                		 gpointer     user_data)
{
	PhotobucketService *self = user_data;
	GthFileData        *file_data;
	char               *details;
	double              current_file_fraction;

	if (self->priv->post_photos->current == NULL)
		return;

	self->priv->post_photos->wrote_body_data_size += chunk->length;
	if (self->priv->post_photos->wrote_body_data_size > msg->request_body->length)
		return;

	file_data = self->priv->post_photos->current->data;
	/* Translators: %s is a filename */
	details = g_strdup_printf (_("Uploading '%s'"), g_file_info_get_display_name (file_data->info));
	current_file_fraction = (double) self->priv->post_photos->wrote_body_data_size / msg->request_body->length;
	gth_task_progress (GTH_TASK (self),
			   NULL,
			   details,
			   FALSE,
			   (double) (self->priv->post_photos->uploaded_size + (g_file_info_get_size (file_data->info) * current_file_fraction)) / self->priv->post_photos->total_size);

	g_free (details);
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
	OAuthAccount       *account;
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
		oauth_service_add_signature (OAUTH_SERVICE (self), "POST", url, data_set);

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

	account = web_service_get_current_account (WEB_SERVICE (self));

	self->priv->post_photos->wrote_body_data_size = 0;
	url = g_strconcat ("http://", PHOTOBUCKET_ACCOUNT (account)->subdomain, "/album/", identifier, "/upload", NULL);
	msg = soup_form_request_new_from_multipart (url, multipart);
	g_signal_connect (msg,
			  "wrote-body-data",
			  (GCallback) upload_photo_wrote_body_data_cb,
			  self);

	_web_service_send_message (WEB_SERVICE (self),
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
	_g_file_load_async (file_data->file,
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
				   PhotobucketAlbum    *album,
				   int                  size,
				   gboolean             scramble,
				   GList               *file_list, /* GFile list */
				   GCancellable        *cancellable,
				   GAsyncReadyCallback  callback,
				   gpointer             user_data)
{
	gth_task_progress (GTH_TASK (self),
			   _("Uploading the files to the server"),
			   "",
			   TRUE,
			   0.0);

	post_photos_data_free (self->priv->post_photos);
	self->priv->post_photos = g_new0 (PostPhotosData, 1);
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
