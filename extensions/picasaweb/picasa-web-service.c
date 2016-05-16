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
#include <json-glib/json-glib.h>
#include <gthumb.h>
#include <extensions/oauth/oauth.h>
#include "picasa-web-album.h"
#include "picasa-web-photo.h"
#include "picasa-web-service.h"


#define ATOM_ENTRY_MIME_TYPE "application/atom+xml; charset=UTF-8; type=entry"
/*#define PICASA_WEB_API_VERSION "1.0"
#define PICASA_WEB_AUTHENTICATION_RESPONSE_CHOOSE_ACCOUNT 2
#define PICASA_WEB_HTTP_SERVER "https://www.picasa_web.com"
#define PICASA_WEB_MAX_IMAGE_SIZE 2048
#define PICASA_WEB_MIN_IMAGE_SIZE 720 FIXME */
#define PICASA_WEB_REDIRECT_URI "urn:ietf:wg:oauth:2.0:oob"
#define PICASA_WEB_REDIRECT_TITLE "Success code="
#define PICASA_WEB_SERVICE_ERROR_TOKEN_EXPIRED 190
#define GTHUMB_PICASA_WEB_CLIENT_ID "499958842898.apps.googleusercontent.com"
#define GTHUMB_PICASA_WEB_CLIENT_SECRET "-DdIqzDxVRc_Wkobuf-2g-of"


/* -- post_photos_data -- */


typedef struct {
	PicasaWebAlbum      *album;
	GList               *file_list;
	int                  max_width;
	int                  max_height;
	GCancellable        *cancellable;
        GAsyncReadyCallback  callback;
        gpointer             user_data;
	GList               *current;
	guint64              total_size;
	guint64              uploaded_size;
	guint64              wrote_body_data_size;
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


/* -- picasa_web_service -- */


G_DEFINE_TYPE (PicasaWebService, picasa_web_service, WEB_TYPE_SERVICE)


struct _PicasaWebServicePrivate {
	char                    *access_token;
	char                    *refresh_token;
	guint64         	 quota_limit;
	guint64       		 quota_used;
	PostPhotosData		*post_photos;
};


static void
picasa_web_service_finalize (GObject *object)
{
	PicasaWebService *self = PICASA_WEB_SERVICE (object);

	post_photos_data_free (self->priv->post_photos);
	g_free (self->priv->access_token);
	g_free (self->priv->refresh_token);

	G_OBJECT_CLASS (picasa_web_service_parent_class)->finalize (object);
}


/* -- connection utilities -- */


static void
_picasa_web_service_add_headers (PicasaWebService *self,
				 SoupMessage      *msg)
{
	if (self->priv->access_token != NULL) {
		char *value;

		value = g_strconcat ("Bearer ", self->priv->access_token, NULL);
		soup_message_headers_replace (msg->request_headers, "Authorization", value);
		g_free (value);
	}

	soup_message_headers_replace (msg->request_headers, "GData-Version", "2");
}


static gboolean
picasa_web_utils_parse_json_response (SoupMessage  *msg,
				      JsonNode    **node,
				      GError      **error)
{
	JsonParser *parser;
	SoupBuffer *body;

	g_return_val_if_fail (msg != NULL, FALSE);
	g_return_val_if_fail (node != NULL, FALSE);

	*node = NULL;

	if ((msg->status_code != 200) && (msg->status_code != 400)) {
		*error = g_error_new (SOUP_HTTP_ERROR,
				      msg->status_code,
				      "%s",
				      soup_status_get_phrase (msg->status_code));
		return FALSE;
	}

	body = soup_message_body_flatten (msg->response_body);
	parser = json_parser_new ();
	if (json_parser_load_from_data (parser, body->data, body->length, error)) {
		JsonObject *obj;

		*node = json_node_copy (json_parser_get_root (parser));

		obj = json_node_get_object (*node);
		if (json_object_has_member (obj, "error")) {
			JsonObject *error_obj;

			error_obj = json_object_get_object_member (obj, "error");
			*error = g_error_new (WEB_SERVICE_ERROR,
					      json_object_get_int_member (error_obj, "code"),
					      "%s",
					      json_object_get_string_member (error_obj, "message"));

			json_node_free (*node);
			*node = NULL;
		}
	}

	g_object_unref (parser);
	soup_buffer_free (body);

	return *node != NULL;
}


/* -- _picasa_web_service_get_refresh_token -- */

static void
_picasa_web_service_get_refresh_token_ready_cb (SoupSession *session,
						SoupMessage *msg,
						gpointer     user_data)
{
	PicasaWebService   *self = user_data;
	GSimpleAsyncResult *result;
	GError             *error = NULL;
	JsonNode           *node;

	result = _web_service_get_result (WEB_SERVICE (self));

	if (picasa_web_utils_parse_json_response (msg, &node, &error)) {
		JsonObject *obj;

		obj = json_node_get_object (node);
		_g_strset (&self->priv->access_token, json_object_get_string_member (obj, "access_token"));
		_g_strset (&self->priv->refresh_token, json_object_get_string_member (obj, "refresh_token"));
	}
	else
		g_simple_async_result_set_from_error (result, error);

	g_simple_async_result_complete_in_idle (result);
}


static void
_picasa_web_service_get_refresh_token (PicasaWebService    *self,
				       const char          *authorization_code,
				       GCancellable        *cancellable,
				       GAsyncReadyCallback  callback,
				       gpointer             user_data)
{
	GHashTable  *data_set;
	SoupMessage *msg;

	data_set = g_hash_table_new (g_str_hash, g_str_equal);
	g_hash_table_insert (data_set, "code", (gpointer) authorization_code);
	g_hash_table_insert (data_set, "client_id", GTHUMB_PICASA_WEB_CLIENT_ID);
	g_hash_table_insert (data_set, "client_secret", GTHUMB_PICASA_WEB_CLIENT_SECRET);
	g_hash_table_insert (data_set, "redirect_uri", PICASA_WEB_REDIRECT_URI);
	g_hash_table_insert (data_set, "grant_type", "authorization_code");
	msg = soup_form_request_new_from_hash ("POST", "https://accounts.google.com/o/oauth2/token", data_set);
	_picasa_web_service_add_headers (self, msg);
	_web_service_send_message (WEB_SERVICE (self),
				   msg,
				   cancellable,
				   callback,
				   user_data,
				   _picasa_web_service_get_refresh_token,
				   _picasa_web_service_get_refresh_token_ready_cb,
				   self);

	g_hash_table_destroy (data_set);
}


static gboolean
_picasa_web_service_get_refresh_token_finish (PicasaWebService  *service,
					      GAsyncResult      *result,
					      GError           **error)
{
	return ! g_simple_async_result_propagate_error (G_SIMPLE_ASYNC_RESULT (result), error);
}


/* -- picasa_web_service_ask_authorization -- */


static void
refresh_token_ready_cb (GObject      *source_object,
		        GAsyncResult *result,
		        gpointer      user_data)
{
	PicasaWebService *self = user_data;
	GError           *error = NULL;
	GtkWidget        *dialog;
	gboolean          success;

	dialog = _web_service_get_auth_dialog (WEB_SERVICE (self));
	success = _picasa_web_service_get_refresh_token_finish (self, result, &error);
	gtk_dialog_response (GTK_DIALOG (dialog), success ? GTK_RESPONSE_OK : GTK_RESPONSE_CANCEL);
}


static void
ask_authorization_dialog_loaded_cb (OAuthAskAuthorizationDialog *dialog,
				    gpointer                     user_data)
{
	PicasaWebService *self = user_data;
	const char       *title;

	title = oauth_ask_authorization_dialog_get_title (dialog);
	if (title == NULL)
		return;

	if (g_str_has_prefix (title, PICASA_WEB_REDIRECT_TITLE)) {
		const char *authorization_code;

		gtk_widget_hide (GTK_WIDGET (dialog));
		gth_task_dialog (GTH_TASK (self), FALSE, NULL);

		authorization_code = title + strlen (PICASA_WEB_REDIRECT_TITLE);
		_picasa_web_service_get_refresh_token (self,
						       authorization_code,
						       gth_task_get_cancellable (GTH_TASK (self)),
						       refresh_token_ready_cb,
						       self);
	}
}


static char *
picasa_web_service_get_authorization_url (PicasaWebService *self)
{
	GHashTable *data_set;
	GString    *link;
	GList      *keys;
	GList      *scan;

	data_set = g_hash_table_new (g_str_hash, g_str_equal);
	g_hash_table_insert (data_set, "response_type", "code");
	g_hash_table_insert (data_set, "client_id", GTHUMB_PICASA_WEB_CLIENT_ID);
	g_hash_table_insert (data_set, "redirect_uri", PICASA_WEB_REDIRECT_URI);
	g_hash_table_insert (data_set, "scope", "https://picasaweb.google.com/data/ https://www.googleapis.com/auth/userinfo.profile");

	link = g_string_new ("https://accounts.google.com/o/oauth2/auth?");
	keys = g_hash_table_get_keys (data_set);
	for (scan = keys; scan; scan = scan->next) {
		char *key = scan->data;
		char *encoded;

		if (scan != keys)
			g_string_append (link, "&");
		g_string_append (link, key);
		g_string_append (link, "=");
		encoded = soup_uri_encode (g_hash_table_lookup (data_set, key), NULL);
		g_string_append (link, encoded);

		g_free (encoded);
	}

	g_list_free (keys);
	g_hash_table_destroy (data_set);

	return g_string_free (link, FALSE);
}


static void
picasa_web_service_ask_authorization (WebService *base)
{
	PicasaWebService *self = PICASA_WEB_SERVICE (base);
	GtkWidget        *dialog;

	_g_strset (&self->priv->refresh_token, NULL);
	_g_strset (&self->priv->access_token, NULL);

	dialog = oauth_ask_authorization_dialog_new (picasa_web_service_get_authorization_url (self));
	gtk_window_set_default_size (GTK_WINDOW (dialog), 680, 580);
	_web_service_set_auth_dialog (WEB_SERVICE (self), GTK_DIALOG (dialog));

	g_signal_connect (OAUTH_ASK_AUTHORIZATION_DIALOG (dialog),
			  "loaded",
			  G_CALLBACK (ask_authorization_dialog_loaded_cb),
			  self);

	gtk_widget_show (dialog);
}


/* -- _picasa_web_service_get_access_token -- */


static void
_picasa_web_service_get_access_token_ready_cb (SoupSession *session,
					       SoupMessage *msg,
					       gpointer     user_data)
{
	PicasaWebService   *self = user_data;
	GSimpleAsyncResult *result;
	GError             *error = NULL;
	JsonNode           *node;

	result = _web_service_get_result (WEB_SERVICE (self));

	if (picasa_web_utils_parse_json_response (msg, &node, &error)) {
		JsonObject   *obj;
		OAuthAccount *account;

		obj = json_node_get_object (node);
		account = web_service_get_current_account (WEB_SERVICE (self));
		if (account != NULL)
			g_object_set (account,
				      "token", json_object_get_string_member (obj, "access_token"),
				      NULL);
		else
			_g_strset (&self->priv->access_token, json_object_get_string_member (obj, "access_token"));
	}
	else
		g_simple_async_result_set_from_error (result, error);

	g_simple_async_result_complete_in_idle (result);
}


static void
_picasa_web_service_get_access_token (PicasaWebService    *self,
				      const char          *refresh_token,
				      GCancellable        *cancellable,
				      GAsyncReadyCallback  callback,
				      gpointer             user_data)
{
	GHashTable  *data_set;
	SoupMessage *msg;

	_g_strset (&self->priv->access_token, NULL);

	data_set = g_hash_table_new (g_str_hash, g_str_equal);
	g_hash_table_insert (data_set, "refresh_token", (gpointer) refresh_token);
	g_hash_table_insert (data_set, "client_id", GTHUMB_PICASA_WEB_CLIENT_ID);
	g_hash_table_insert (data_set, "client_secret", GTHUMB_PICASA_WEB_CLIENT_SECRET);
	g_hash_table_insert (data_set, "grant_type", "refresh_token");
	msg = soup_form_request_new_from_hash ("POST", "https://accounts.google.com/o/oauth2/token", data_set);
	_picasa_web_service_add_headers (self, msg);
	_web_service_send_message (WEB_SERVICE (self),
				   msg,
				   cancellable,
				   callback,
				   user_data,
				   _picasa_web_service_get_access_token,
				   _picasa_web_service_get_access_token_ready_cb,
				   self);

	g_hash_table_destroy (data_set);
}


static gboolean
_picasa_web_service_get_access_token_finish (PicasaWebService  *service,
					     GAsyncResult      *result,
					     GError           **error)
{
	return ! g_simple_async_result_propagate_error (G_SIMPLE_ASYNC_RESULT (result), error);
}


/* -- picasa_web_service_get_user_info -- */


static void
picasa_web_service_get_user_info_ready_cb (SoupSession *session,
				           SoupMessage *msg,
				           gpointer     user_data)
{
	PicasaWebService   *self = user_data;
	GSimpleAsyncResult *result;
	GError             *error = NULL;
	JsonNode           *node;

	result = _web_service_get_result (WEB_SERVICE (self));

	if (picasa_web_utils_parse_json_response (msg, &node, &error)) {
		OAuthAccount *account;

		account = (OAuthAccount *) json_gobject_deserialize (OAUTH_TYPE_ACCOUNT, node);
		g_object_set (account,
			      "token", self->priv->access_token,
			      "token-secret", self->priv->refresh_token,
			      NULL);
		g_simple_async_result_set_op_res_gpointer (result,
							   g_object_ref (account),
							   (GDestroyNotify) g_object_unref);

		_g_object_unref (account);
		json_node_free (node);
	}
	else
		g_simple_async_result_set_from_error (result, error);

	g_simple_async_result_complete_in_idle (result);
}


typedef struct {
	PicasaWebService    *service;
	GCancellable        *cancellable;
	GAsyncReadyCallback  callback;
	gpointer	     user_data;
} AccessTokenData;


static void
access_token_data_free (AccessTokenData *data)
{
	_g_object_unref (data->cancellable);
	g_free (data);
}


static void
picasa_web_service_get_user_info (WebService          *base,
				  GCancellable        *cancellable,
				  GAsyncReadyCallback  callback,
				  gpointer	       user_data);


static void
access_token_ready_cb (GObject      *source_object,
		       GAsyncResult *result,
		       gpointer      user_data)
{
	AccessTokenData  *data = user_data;
	PicasaWebService *self = data->service;
	GError           *error = NULL;

	if (! _picasa_web_service_get_access_token_finish (self, result, &error)) {
		GSimpleAsyncResult *result;

		result = g_simple_async_result_new (G_OBJECT (self),
						    data->callback,
						    data->user_data,
						    picasa_web_service_get_user_info);
		g_simple_async_result_take_error (result, error);
		g_simple_async_result_complete_in_idle (result);

		access_token_data_free (data);
		return;
	}

	/* call get_user_info again, now that we have the access token */
	picasa_web_service_get_user_info (WEB_SERVICE (self),
					  data->cancellable,
					  data->callback,
					  data->user_data);

	access_token_data_free (data);
}


static void
picasa_web_service_get_user_info (WebService          *base,
				  GCancellable        *cancellable,
				  GAsyncReadyCallback  callback,
				  gpointer	       user_data)
{
	PicasaWebService *self = PICASA_WEB_SERVICE (base);
	OAuthAccount     *account;
	GHashTable       *data_set;
	SoupMessage      *msg;

	account = web_service_get_current_account (WEB_SERVICE (self));
	if (account != NULL) {
		_g_strset (&self->priv->refresh_token, account->token_secret);
		_g_strset (&self->priv->access_token, account->token);
	}

	data_set = g_hash_table_new (g_str_hash, g_str_equal);
	if (self->priv->access_token != NULL) {
		msg = soup_form_request_new_from_hash ("GET", "https://www.googleapis.com/oauth2/v2/userinfo", data_set);
		_picasa_web_service_add_headers (self, msg);
		_web_service_send_message (WEB_SERVICE (self),
					   msg,
					   cancellable,
					   callback,
					   user_data,
					   picasa_web_service_get_user_info,
					   picasa_web_service_get_user_info_ready_cb,
					   self);
	}
	else {
		/* Get the access token from the refresh token */

		AccessTokenData *data;

		data = g_new0 (AccessTokenData, 1);
		data->service = self;
		data->cancellable = _g_object_ref (cancellable);
		data->callback = callback;
		data->user_data = user_data;
		_picasa_web_service_get_access_token (self,
						      self->priv->refresh_token,
						      cancellable,
						      access_token_ready_cb,
						      data);
	}

	g_hash_table_destroy (data_set);
}


static void
picasa_web_service_class_init (PicasaWebServiceClass *klass)
{
	GObjectClass    *object_class;
	WebServiceClass *service_class;

	g_type_class_add_private (klass, sizeof (PicasaWebServicePrivate));

	object_class = (GObjectClass*) klass;
	object_class->finalize = picasa_web_service_finalize;

	service_class = (WebServiceClass*) klass;
	service_class->ask_authorization = picasa_web_service_ask_authorization;
	service_class->get_user_info = picasa_web_service_get_user_info;
}


static void
picasa_web_service_init (PicasaWebService *self)
{
	self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, PICASA_TYPE_WEB_SERVICE, PicasaWebServicePrivate);
	self->priv->refresh_token = NULL;
	self->priv->access_token = NULL;
	self->priv->quota_limit = 0;
	self->priv->quota_used = 0;
	self->priv->post_photos = NULL;
}


PicasaWebService *
picasa_web_service_new (GCancellable	*cancellable,
		        GtkWidget	*browser,
		        GtkWidget	*dialog)
{
	return g_object_new (PICASA_TYPE_WEB_SERVICE,
			     "service-name", "picasaweb",
			     "service-address", "picasaweb.google.com",
			     "service-protocol", "https",
			     "cancellable", cancellable,
			     "browser", browser,
			     "dialog", dialog,
			     NULL);
}


guint64
picasa_web_service_get_free_space (PicasaWebService *self)
{
	if (self->priv->quota_limit >= self->priv->quota_used)
		return (guint64) (self->priv->quota_limit - self->priv->quota_used);
	else
		return 0;
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
				else if (g_strcmp0 (node->tag_name, "gphoto:quotalimit") == 0) {
					self->priv->quota_limit = g_ascii_strtoull (dom_element_get_inner_text (node), NULL, 10);
				}
				else if (g_strcmp0 (node->tag_name, "gphoto:quotacurrent") == 0) {
					self->priv->quota_used = g_ascii_strtoull (dom_element_get_inner_text (node), NULL, 10);
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
			        GCancellable        *cancellable,
			        GAsyncReadyCallback  callback,
			        gpointer             user_data)
{
	OAuthAccount *account;
	SoupMessage  *msg;
	char         *url;

	account = web_service_get_current_account (WEB_SERVICE (self));
	g_return_if_fail (account != NULL);

	gth_task_progress (GTH_TASK (self), _("Getting the album list"), NULL, TRUE, 0.0);

	url = g_strconcat ("https://picasaweb.google.com/data/feed/api/user/", account->id, NULL);
	msg = soup_message_new ("GET", url);
	_picasa_web_service_add_headers (self, msg);
	_web_service_send_message (WEB_SERVICE (self),
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

	result = _web_service_get_result (WEB_SERVICE (self));

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
	OAuthAccount *account;
	DomDocument  *doc;
	DomElement   *entry;
	char         *buffer;
	gsize         len;
	char         *url;
	SoupMessage  *msg;

	account = web_service_get_current_account (WEB_SERVICE (self));
	g_return_if_fail (account != NULL);

	gth_task_progress (GTH_TASK (self), _("Creating the new album"), NULL, TRUE, 0.0);

	doc = dom_document_new ();
	entry = dom_domizable_create_element (DOM_DOMIZABLE (album), doc);
	dom_element_append_child (DOM_ELEMENT (doc), entry);
	buffer = dom_document_dump (doc, &len);

	url = g_strconcat ("https://picasaweb.google.com/data/feed/api/user/", account->id, NULL);
	msg = soup_message_new ("POST", url);
	soup_message_set_request (msg, ATOM_ENTRY_MIME_TYPE, SOUP_MEMORY_TAKE, buffer, len);
	_picasa_web_service_add_headers (self, msg);
	_web_service_send_message (WEB_SERVICE (self),
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

	result = _web_service_get_result (WEB_SERVICE (self));
	if (error == NULL) {
		g_simple_async_result_set_op_res_gboolean (result, TRUE);
	}
	else  {
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


static void picasa_wev_service_post_current_file (PicasaWebService *self);


static void
post_photo_ready_cb (SoupSession *session,
		     SoupMessage *msg,
		     gpointer     user_data)
{
	PicasaWebService *self = user_data;

	if (msg->status_code != 201) {
		GError *error;

		error = g_error_new_literal (SOUP_HTTP_ERROR,
					     msg->status_code,
					     soup_status_get_phrase (msg->status_code));
		post_photos_done (self, error);
		g_error_free (error);

		return;
	}

	self->priv->post_photos->current = self->priv->post_photos->current->next;
	picasa_wev_service_post_current_file (self);
}


static void
upload_photo_wrote_body_data_cb (SoupMessage *msg,
                		 SoupBuffer  *chunk,
                		 gpointer     user_data)
{
	PicasaWebService *self = user_data;
	GthFileData      *file_data;
	char             *details;
	double            current_file_fraction;

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
post_photo_file_buffer_ready_cb (void     **buffer,
				 gsize      count,
				 GError    *error,
				 gpointer   user_data)
{
	PicasaWebService   *self = user_data;
	OAuthAccount       *account;
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
	void               *resized_buffer;
	gsize               resized_count;
	char               *url;
	SoupMessage        *msg;

	if (error != NULL) {
		post_photos_done (self, error);
		return;
	}

	account = web_service_get_current_account (WEB_SERVICE (self));
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
	if (metadata != NULL)
		value = gth_string_list_join (GTH_STRING_LIST (gth_metadata_get_string_list (GTH_METADATA (metadata))), ", ");
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

	if (_g_buffer_resize_image (*buffer,
				    count,
				    file_data,
				    self->priv->post_photos->max_width,
				    self->priv->post_photos->max_height,
				    &resized_buffer,
				    &resized_count,
				    self->priv->post_photos->cancellable,
				    &error))
	{
		body = soup_buffer_new (SOUP_MEMORY_TAKE, resized_buffer, resized_count);
	}
	else if (error == NULL) {
		body = soup_buffer_new (SOUP_MEMORY_TEMPORARY, *buffer, count);
	}
	else {
		soup_multipart_free (multipart);
		post_photos_done (self, error);
		return;
	}

	soup_multipart_append_form_file (multipart,
					 "file",
					 NULL,
					 gth_file_data_get_mime_type (file_data),
					 body);

	soup_buffer_free (body);

	/* send the file */

	self->priv->post_photos->wrote_body_data_size = 0;
	url = g_strconcat ("https://picasaweb.google.com/data/feed/api/user/",
			   account->id,
			   "/albumid/",
			   self->priv->post_photos->album->id,
			   NULL);
	msg = soup_form_request_new_from_multipart (url, multipart);
	g_signal_connect (msg,
			  "wrote-body-data",
			  (GCallback) upload_photo_wrote_body_data_cb,
			  self);

	_picasa_web_service_add_headers (self, msg);
	_web_service_send_message (WEB_SERVICE (self),
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
	_g_file_load_async (file_data->file,
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
	GList            *scan;

	if (error != NULL) {
		post_photos_done (self, error);
		return;
	}

	self->priv->post_photos->file_list = _g_object_list_ref (files);
	self->priv->post_photos->total_size = 0;
	self->priv->post_photos->n_files = 0;
	for (scan = self->priv->post_photos->file_list; scan; scan = scan->next) {
		GthFileData *file_data = scan->data;

		self->priv->post_photos->total_size += g_file_info_get_size (file_data->info);
		self->priv->post_photos->n_files += 1;
	}

	self->priv->post_photos->current = self->priv->post_photos->file_list;
	picasa_wev_service_post_current_file (self);
}


void
picasa_web_service_post_photos (PicasaWebService    *self,
			        PicasaWebAlbum      *album,
			        GList               *file_list, /* GFile list */
			        int                  max_width,
			        int                  max_height,
			        GCancellable        *cancellable,
			        GAsyncReadyCallback  callback,
			        gpointer             user_data)
{
	g_return_if_fail (album != NULL);
	g_return_if_fail (self->priv->post_photos == NULL);

	gth_task_progress (GTH_TASK (self),
			   _("Uploading the files to the server"),
			   "",
			   TRUE,
			   0.0);

	self->priv->post_photos = g_new0 (PostPhotosData, 1);
	self->priv->post_photos->album = g_object_ref (album);
	self->priv->post_photos->max_width = max_width;
	self->priv->post_photos->max_height = max_height;
	self->priv->post_photos->cancellable = _g_object_ref (cancellable);
	self->priv->post_photos->callback = callback;
	self->priv->post_photos->user_data = user_data;

	_g_query_all_metadata_async (file_list,
				     GTH_LIST_DEFAULT,
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
	OAuthAccount *account;
	char         *url;
	SoupMessage  *msg;

	account = web_service_get_current_account (WEB_SERVICE (self));
	g_return_if_fail (account != NULL);
	g_return_if_fail (album != NULL);

	gth_task_progress (GTH_TASK (self),
			   _("Getting the photo list"),
			   NULL,
			   TRUE,
			   0.0);

	url = g_strconcat ("https://picasaweb.google.com/data/feed/api/user/",
			   account->id,
			   "/albumid/",
			   album->id,
			   NULL);
	msg = soup_message_new ("GET", url);
	_picasa_web_service_add_headers (self, msg);
	_web_service_send_message (WEB_SERVICE (self),
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
