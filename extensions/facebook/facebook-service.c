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
#include <json-glib/json-glib.h>
#include <gthumb.h>
#include <extensions/oauth/oauth.h>
#include "facebook-album.h"
#include "facebook-photo.h"
#include "facebook-service.h"


#define GTHUMB_FACEBOOK_NAMESPACE "gthumbviewer"
#define GTHUMB_FACEBOOK_APP_ID "110609985627460"
#define GTHUMB_FACEBOOK_APP_SECRET "8c0b99672a9bbc159ebec3c9a8240679"
#define FACEBOOK_API_VERSION "1.0"
#define FACEBOOK_AUTHENTICATION_RESPONSE_CHOOSE_ACCOUNT 2
#define FACEBOOK_HTTP_SERVER "https://www.facebook.com"
#define FACEBOOK_MAX_IMAGE_SIZE 2048
#define FACEBOOK_MIN_IMAGE_SIZE 720
#define FACEBOOK_REDIRECT_URI "https://apps.facebook.com/" GTHUMB_FACEBOOK_NAMESPACE
#define FACEBOOK_SERVICE_ERROR_TOKEN_EXPIRED 190


/* -- post_photos_data -- */


typedef struct {
	FacebookAlbum       *album;
	GList               *file_list;
	int                  max_resolution;
	FacebookVisibility   visibility_level;
	GCancellable        *cancellable;
        GAsyncReadyCallback  callback;
        gpointer             user_data;
	GList               *current;
	goffset              total_size;
	goffset              uploaded_size;
	goffset              wrote_body_data_size;
	int                  n_files;
	int                  uploaded_files;
	GList               *ids;
} PostPhotosData;


static void
post_photos_data_free (PostPhotosData *post_photos)
{
	if (post_photos == NULL)
		return;
	_g_string_list_free (post_photos->ids);
	_g_object_unref (post_photos->cancellable);
	_g_object_list_unref (post_photos->file_list);
	g_free (post_photos);
}


/* -- facebook_service -- */


G_DEFINE_TYPE (FacebookService, facebook_service, WEB_TYPE_SERVICE)


struct _FacebookServicePrivate {
	char           *state;
	char           *token;
	PostPhotosData *post_photos;
};


static void
facebook_service_finalize (GObject *object)
{
	FacebookService *self = FACEBOOK_SERVICE (object);

	post_photos_data_free (self->priv->post_photos);
	g_free (self->priv->token);
	g_free (self->priv->state);

	G_OBJECT_CLASS (facebook_service_parent_class)->finalize (object);
}


/* -- connection utilities -- */


static void
_facebook_service_set_access_token (FacebookService *self,
				    const char      *token)
{
	_g_strset (&self->priv->token, token);
}


static const char *
_facebook_service_get_access_token (FacebookService *self)
{
	return self->priv->token;
}


static void
_facebook_service_add_access_token (FacebookService *self,
				    GHashTable      *data_set)
{
	g_return_if_fail (self->priv->token != NULL);

	g_hash_table_insert (data_set, "access_token", self->priv->token);
}


static char *
get_access_type_name (WebAuthorization access_type)
{
	char *name = NULL;

	switch (access_type) {
	case WEB_AUTHORIZATION_READ:
		name = "";
		break;

	case WEB_AUTHORIZATION_WRITE:
		name = "publish_actions";
		break;
	}

	return name;
}


static char *
facebook_utils_get_authorization_url (WebAuthorization  access_type,
				      const char       *state)
{
	GHashTable *data_set;
	GString    *link;
	GList      *keys;
	GList      *scan;

	data_set = g_hash_table_new (g_str_hash, g_str_equal);
	g_hash_table_insert (data_set, "client_id", GTHUMB_FACEBOOK_APP_ID);
	g_hash_table_insert (data_set, "redirect_uri", FACEBOOK_REDIRECT_URI);
	g_hash_table_insert (data_set, "scope", get_access_type_name (access_type));
	g_hash_table_insert (data_set, "response_type", "token");
	g_hash_table_insert (data_set, "state", (char *) state);

	link = g_string_new ("https://www.facebook.com/dialog/oauth?");
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


static gboolean
facebook_utils_parse_response (SoupMessage  *msg,
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
			int         error_code;

			error_obj = json_object_get_object_member (obj, "error");
			switch (json_object_get_int_member (error_obj, "code")) {
			case FACEBOOK_SERVICE_ERROR_TOKEN_EXPIRED:
				error_code = WEB_SERVICE_ERROR_TOKEN_EXPIRED;
				break;
			default:
				error_code = WEB_SERVICE_ERROR_GENERIC;
				break;
			}

			*error = g_error_new (WEB_SERVICE_ERROR,
					      error_code,
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


/* -- facebook_service_ask_authorization -- */


static void
ask_authorization_dialog_redirected_cb (OAuthAskAuthorizationDialog *dialog,
					gpointer                     user_data)
{
	FacebookService *self = user_data;
	const char      *uri;

	uri = oauth_ask_authorization_dialog_get_uri (dialog);
	if (g_str_has_prefix (uri, FACEBOOK_REDIRECT_URI)) {
		const char *uri_data;
		GHashTable *data;
		const char *access_token;
		const char *state;

		uri_data = uri + strlen (FACEBOOK_REDIRECT_URI "#");

		data = soup_form_decode (uri_data);
		access_token = NULL;
		state = g_hash_table_lookup (data, "state");
		if (g_strcmp0 (state, self->priv->state) == 0) {
			access_token = g_hash_table_lookup (data, "access_token");
			_facebook_service_set_access_token (self, access_token);
		}

		gtk_dialog_response (GTK_DIALOG (dialog),
				     (access_token != NULL) ? GTK_RESPONSE_OK : GTK_RESPONSE_CANCEL);

		g_hash_table_destroy (data);
	}
}


static char *
facebook_create_state_for_authorization (void)
{
	GTimeVal  t;
	char     *s;
	char     *v;

	g_get_current_time (&t);
	s = g_strdup_printf ("%ld%u", t.tv_usec, g_random_int ());
	v = g_compute_checksum_for_string (G_CHECKSUM_MD5, s, -1);

	g_free (s);

	return v;
}


static void
facebook_service_ask_authorization (WebService *base)
{
	FacebookService *self = FACEBOOK_SERVICE (base);
	GtkWidget       *dialog;

	g_free (self->priv->state);
	self->priv->state = facebook_create_state_for_authorization ();

	dialog = oauth_ask_authorization_dialog_new (facebook_utils_get_authorization_url (WEB_AUTHORIZATION_WRITE, self->priv->state));
	_gtk_window_resize_to_fit_screen_height (dialog, 1024);
	_web_service_set_auth_dialog (WEB_SERVICE (self), GTK_DIALOG (dialog));

	g_signal_connect (OAUTH_ASK_AUTHORIZATION_DIALOG (dialog),
			  "redirected",
			  G_CALLBACK (ask_authorization_dialog_redirected_cb),
			  self);

	gtk_widget_show (dialog);
}


/* -- facebook_service_get_user_info -- */


static void
facebook_service_get_user_info_ready_cb (SoupSession *session,
				         SoupMessage *msg,
				         gpointer     user_data)
{
	FacebookService    *self = user_data;
	GSimpleAsyncResult *result;
	GError             *error = NULL;
	JsonNode           *node;

	result = _web_service_get_result (WEB_SERVICE (self));

	if (facebook_utils_parse_response (msg, &node, &error)) {
		OAuthAccount *account;

		account = (OAuthAccount *) json_gobject_deserialize (OAUTH_TYPE_ACCOUNT, node);
		g_object_set (account,
			      "token", _facebook_service_get_access_token (self),
			      "token-secret", _facebook_service_get_access_token (self),
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


static void
facebook_service_get_user_info (WebService          *base,
				GCancellable        *cancellable,
				GAsyncReadyCallback  callback,
				gpointer	     user_data)
{
	FacebookService *self = FACEBOOK_SERVICE (base);
	OAuthAccount    *account;
	GHashTable      *data_set;
	SoupMessage     *msg;

	account = web_service_get_current_account (WEB_SERVICE (self));
	if (account != NULL)
		_facebook_service_set_access_token (self, account->token_secret);

	data_set = g_hash_table_new (g_str_hash, g_str_equal);
	_facebook_service_add_access_token (self, data_set);
	msg = soup_form_request_new_from_hash ("GET", "https://graph.facebook.com/me", data_set);
	_web_service_send_message (WEB_SERVICE (self),
				   msg,
				   cancellable,
				   callback,
				   user_data,
				   facebook_service_get_user_info,
				   facebook_service_get_user_info_ready_cb,
				   self);

	g_hash_table_destroy (data_set);
}


static void
facebook_service_class_init (FacebookServiceClass *klass)
{
	GObjectClass    *object_class;
	WebServiceClass *service_class;

	g_type_class_add_private (klass, sizeof (FacebookServicePrivate));

	object_class = (GObjectClass*) klass;
	object_class->finalize = facebook_service_finalize;

	service_class = (WebServiceClass*) klass;
	service_class->ask_authorization = facebook_service_ask_authorization;
	service_class->get_user_info = facebook_service_get_user_info;
}


static void
facebook_service_init (FacebookService *self)
{
	self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, FACEBOOK_TYPE_SERVICE, FacebookServicePrivate);
	self->priv->state = NULL;
	self->priv->token = NULL;
	self->priv->post_photos = NULL;
}


FacebookService *
facebook_service_new (GCancellable *cancellable,
		      GtkWidget    *browser,
		      GtkWidget    *dialog)
{
	return g_object_new (FACEBOOK_TYPE_SERVICE,
			     "service-name", "facebook",
			     "service-address", "https://www.facebook.com",
			     "service-protocol", "https",
			     "cancellable", cancellable,
			     "browser", browser,
			     "dialog", dialog,
			     NULL);
}


/* -- facebook_service_get_albums -- */


static void
facebook_service_get_albums_ready_cb (SoupSession *session,
				      SoupMessage *msg,
				      gpointer     user_data)
{
	FacebookService    *self = user_data;
	GSimpleAsyncResult *result;
	JsonNode           *node;
	GError             *error = NULL;

	result = _web_service_get_result (WEB_SERVICE (self));

	if (facebook_utils_parse_response (msg, &node, &error)) {
		GList      *albums = NULL;
		JsonObject *obj;
		JsonArray  *data;
		int         i;

		obj = json_node_get_object (node);
		data = json_object_get_array_member (obj, "data");
		for (i = 0; i < json_array_get_length (data); i++) {
			JsonNode      *album_node;
			FacebookAlbum *album;

			album_node = json_array_get_element (data, i);
			album = (FacebookAlbum *) json_gobject_deserialize (FACEBOOK_TYPE_ALBUM, album_node);
			albums = g_list_prepend (albums, album);
		}

		albums = g_list_reverse (albums);
		g_simple_async_result_set_op_res_gpointer (result, albums, (GDestroyNotify) _g_object_list_unref);

		json_node_free (node);
	}
	else
		g_simple_async_result_set_from_error (result, error);

	g_simple_async_result_complete_in_idle (result);
}


void
facebook_service_get_albums (FacebookService     *self,
			     GCancellable        *cancellable,
			     GAsyncReadyCallback  callback,
			     gpointer             user_data)
{
	OAuthAccount *account;
	GHashTable   *data_set;
	char         *uri;
	SoupMessage  *msg;

	account = web_service_get_current_account (WEB_SERVICE (self));
	g_return_if_fail (account != NULL);

	gth_task_progress (GTH_TASK (self),
			   _("Getting the album list"),
			   NULL,
			   TRUE,
			   0.0);

	uri = g_strdup_printf ("https://graph.facebook.com/%s/albums", account->id);
	data_set = g_hash_table_new (g_str_hash, g_str_equal);
	_facebook_service_add_access_token (self, data_set);
	msg = soup_form_request_new_from_hash ("GET", uri, data_set);
	_web_service_send_message (WEB_SERVICE (self),
				   msg,
				   cancellable,
				   callback,
				   user_data,
				   facebook_service_get_albums,
				   facebook_service_get_albums_ready_cb,
				   self);

	g_free (uri);
	g_hash_table_destroy (data_set);
}


GList *
facebook_service_get_albums_finish (FacebookService  *service,
				    GAsyncResult     *result,
				    GError          **error)
{
	if (g_simple_async_result_propagate_error (G_SIMPLE_ASYNC_RESULT (result), error))
		return NULL;
	else
		return _g_object_list_ref (g_simple_async_result_get_op_res_gpointer (G_SIMPLE_ASYNC_RESULT (result)));
}


/* -- facebook_service_create_album -- */


typedef struct {
	FacebookService *service;
	FacebookAlbum   *album;
} CreateAlbumData;


static CreateAlbumData *
create_album_data_new (FacebookService *service,
		       FacebookAlbum   *album)
{
	CreateAlbumData *ca_data;

	ca_data = g_new0 (CreateAlbumData, 1);
	ca_data->service = g_object_ref (service);
	ca_data->album = g_object_ref (album);

	return ca_data;
}


static void
create_album_data_free (CreateAlbumData *ca_data)
{
	_g_object_unref (ca_data->service);
	_g_object_unref (ca_data->album);
	g_free (ca_data);
}


static void
facebook_service_create_album_ready_cb (SoupSession *session,
					SoupMessage *msg,
					gpointer     user_data)
{
	CreateAlbumData    *ca_data = user_data;
	FacebookService    *self = ca_data->service;
	GSimpleAsyncResult *result;
	JsonNode           *node;
	GError             *error = NULL;

	result = _web_service_get_result (WEB_SERVICE (self));

	if (facebook_utils_parse_response (msg, &node, &error)) {
		FacebookAlbum *album;
		JsonObject    *obj;
		const char    *id;

		album = g_object_ref (ca_data->album);
		obj = json_node_get_object (node);
		id = json_object_get_string_member (obj, "id");
		g_object_set (album, "id", id, NULL);
		g_simple_async_result_set_op_res_gpointer (result, album, (GDestroyNotify) _g_object_unref);

		json_node_free (node);
	}
	else
		g_simple_async_result_set_from_error (result, error);

	g_simple_async_result_complete_in_idle (result);

	create_album_data_free (ca_data);
}


void
facebook_service_create_album (FacebookService     *self,
			       FacebookAlbum       *album,
			       GCancellable        *cancellable,
			       GAsyncReadyCallback  callback,
			       gpointer             user_data)
{
	OAuthAccount    *account;
	CreateAlbumData *ca_data;
	char            *uri;
	GHashTable      *data_set;
	SoupMessage     *msg;

	account = web_service_get_current_account (WEB_SERVICE (self));
	g_return_if_fail (account != NULL);

	g_return_if_fail (album != NULL);
	g_return_if_fail (album->name != NULL);

	gth_task_progress (GTH_TASK (self),
			   _("Creating the new album"),
			   NULL,
			   TRUE,
			   0.0);

	ca_data = create_album_data_new (self, album);

	uri = g_strdup_printf ("https://graph.facebook.com/%s/albums", account->id);
	data_set = g_hash_table_new (g_str_hash, g_str_equal);
	g_hash_table_insert (data_set, "name", album->name);
	if (album->description != NULL)
		g_hash_table_insert (data_set, "message", album->description);
	if (album->privacy != NULL)
		g_hash_table_insert (data_set, "privacy", album->privacy);
	_facebook_service_add_access_token (self, data_set);
	msg = soup_form_request_new_from_hash ("POST", uri, data_set);
	_web_service_send_message (WEB_SERVICE (self),
				   msg,
				   cancellable,
				   callback,
				   user_data,
				   facebook_service_create_album,
				   facebook_service_create_album_ready_cb,
				   ca_data);

	g_hash_table_destroy (data_set);
}


FacebookAlbum *
facebook_service_create_album_finish (FacebookService  *self,
				      GAsyncResult     *result,
				      GError          **error)
{
	if (g_simple_async_result_propagate_error (G_SIMPLE_ASYNC_RESULT (result), error))
		return NULL;
	else
		return g_object_ref (g_simple_async_result_get_op_res_gpointer (G_SIMPLE_ASYNC_RESULT (result)));
}


/* -- facebook_service_upload_photos -- */


static void
upload_photos_done (FacebookService *self,
		    GError          *error)
{
	GSimpleAsyncResult *result;

	result = _web_service_get_result (WEB_SERVICE (self));
	if (error == NULL) {
		self->priv->post_photos->ids = g_list_reverse (self->priv->post_photos->ids);
		g_simple_async_result_set_op_res_gpointer (result, self->priv->post_photos->ids, (GDestroyNotify) _g_string_list_free);
		self->priv->post_photos->ids = NULL;
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


static void facebook_service_upload_current_file (FacebookService *self);


static void
upload_photo_ready_cb (SoupSession *session,
		       SoupMessage *msg,
		       gpointer     user_data)
{
	FacebookService *self = user_data;
	JsonNode        *node;
	GError          *error = NULL;
	GthFileData     *file_data;

	if (facebook_utils_parse_response (msg, &node, &error)) {
		JsonObject *obj;
		const char *id;

		obj = json_node_get_object (node);
		id = json_object_get_string_member (obj, "id");
		self->priv->post_photos->ids = g_list_prepend (self->priv->post_photos->ids, g_strdup (id));

		json_node_free (node);
	}
	else {
		upload_photos_done (self, error);
		return;
	}

	file_data = self->priv->post_photos->current->data;
	self->priv->post_photos->uploaded_size += g_file_info_get_size (file_data->info);
	self->priv->post_photos->current = self->priv->post_photos->current->next;
	facebook_service_upload_current_file (self);
}


static void
upload_photo_wrote_body_data_cb (SoupMessage *msg,
                		 SoupBuffer  *chunk,
                		 gpointer     user_data)
{
	FacebookService *self = user_data;
	GthFileData     *file_data;
	char            *details;
	double           current_file_fraction;

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
	FacebookService *self = user_data;
	GthFileData     *file_data;
	SoupMultipart   *multipart;
	char            *uri;
	SoupBuffer      *body;
	SoupMessage     *msg;

	if (error != NULL) {
		upload_photos_done (self, error);
		return;
	}

	file_data = self->priv->post_photos->current->data;
	multipart = soup_multipart_new ("multipart/form-data");

	/* the metadata part */

	{
		GHashTable *data_set;
		char       *title;
		char       *description;
		GList      *keys;
		GList      *scan;

		data_set = g_hash_table_new (g_str_hash, g_str_equal);

		title = gth_file_data_get_attribute_as_string (file_data, "general::title");
		description = gth_file_data_get_attribute_as_string (file_data, "general::description");
		if (description != NULL)
			g_hash_table_insert (data_set, "message", description);
		else if (title != NULL)
			g_hash_table_insert (data_set, "message", title);

		_facebook_service_add_access_token (self, data_set);

		keys = g_hash_table_get_keys (data_set);
		for (scan = keys; scan; scan = scan->next) {
			char *key = scan->data;
			char *value = g_hash_table_lookup (data_set, key);
			if (value != NULL)
				soup_multipart_append_form_string (multipart, key, value);
		}

		g_list_free (keys);
		g_hash_table_unref (data_set);
	}

	/* the file part: rotate and scale the image if required */

	{
		GInputStream    *stream;
		GthImage        *image;
		cairo_surface_t *surface;
		int              width;
		int              height;

		stream = g_memory_input_stream_new_from_data (*buffer, count, NULL);
		image = gth_image_new_from_stream (stream, -1, NULL, NULL, NULL, &error);
		if (image == NULL) {
			g_object_unref (stream);
			soup_multipart_free (multipart);
			upload_photos_done (self, error);
			return;
		}

		g_object_unref (stream);

		surface = gth_image_get_cairo_surface (image);
		width = cairo_image_surface_get_width (surface);
		height = cairo_image_surface_get_height (surface);
		if (scale_keeping_ratio (&width,
					 &height,
					 self->priv->post_photos->max_resolution,
					 self->priv->post_photos->max_resolution,
					 FALSE))
		{
			cairo_surface_t *scaled;

			scaled = _cairo_image_surface_scale (surface, width, height, SCALE_FILTER_BEST, NULL);
			cairo_surface_destroy (surface);
			surface = scaled;
		}

		g_free (*buffer);
		*buffer = NULL;

		gth_image_set_cairo_surface (image, surface);
		if (! gth_image_save_to_buffer (image,
						gth_file_data_get_mime_type (file_data),
						file_data,
					        (char **) buffer,
					        &count,
					        self->priv->post_photos->cancellable,
					        &error))
		{
			cairo_surface_destroy (surface);
			g_object_unref (image);
			soup_multipart_free (multipart);
			upload_photos_done (self, error);
			return;
		}

		cairo_surface_destroy (surface);
		g_object_unref (image);
	}

	uri = g_file_get_uri (file_data->file);
	body = soup_buffer_new (SOUP_MEMORY_TEMPORARY, *buffer, count);
	soup_multipart_append_form_file (multipart,
					 "source",
					 _g_uri_get_basename (uri),
					 gth_file_data_get_mime_type (file_data),
					 body);

	soup_buffer_free (body);
	g_free (uri);

	/* send the file */

	self->priv->post_photos->wrote_body_data_size = 0;
	uri = g_strdup_printf ("https://graph.facebook.com/%s/photos", self->priv->post_photos->album->id);
	msg = soup_form_request_new_from_multipart (uri, multipart);
	g_signal_connect (msg,
			  "wrote-body-data",
			  (GCallback) upload_photo_wrote_body_data_cb,
			  self);
	_web_service_send_message (WEB_SERVICE (self),
				   msg,
				   self->priv->post_photos->cancellable,
				   self->priv->post_photos->callback,
				   self->priv->post_photos->user_data,
				   facebook_service_upload_photos,
				   upload_photo_ready_cb,
				   self);

	g_free (uri);
	soup_multipart_free (multipart);
}


static void
facebook_service_upload_current_file (FacebookService *self)
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
	FacebookService *self = user_data;
	GList           *scan;

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
	facebook_service_upload_current_file (self);
}


void
facebook_service_upload_photos (FacebookService     *self,
				FacebookAlbum       *album,
				GList               *file_list, /* GFile list */
				int                  max_resolution,
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
	self->priv->post_photos->max_resolution = CLAMP (max_resolution, FACEBOOK_MIN_IMAGE_SIZE, FACEBOOK_MAX_IMAGE_SIZE);
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


GList *
facebook_service_upload_photos_finish (FacebookService  *self,
				       GAsyncResult     *result,
				       GError          **error)
{
	if (g_simple_async_result_propagate_error (G_SIMPLE_ASYNC_RESULT (result), error))
		return NULL;
	else
		return _g_string_list_dup (g_simple_async_result_get_op_res_gpointer (G_SIMPLE_ASYNC_RESULT (result)));
}


/* -- facebook_service_list_photos -- */


static void
facebook_service_list_photos_ready_cb (SoupSession *session,
				       SoupMessage *msg,
				       gpointer     user_data)
{
	FacebookService    *self = user_data;
	GSimpleAsyncResult *result;
	JsonNode           *node;
	GError             *error = NULL;

	result = _web_service_get_result (WEB_SERVICE (self));

	if (facebook_utils_parse_response (msg, &node, &error)) {
		GList      *photos = NULL;
		JsonObject *obj;
		JsonObject *obj_photos;
		JsonArray  *data;
		int         i;

		obj = json_node_get_object (node);
		obj_photos = json_object_get_object_member (obj, "photos");
		data = json_object_get_array_member (obj_photos, "data");
		for (i = 0; i < json_array_get_length (data); i++) {
			JsonNode      *photo_node;
			FacebookPhoto *photo;

			photo_node = json_array_get_element (data, i);
			photo = (FacebookPhoto *) json_gobject_deserialize (FACEBOOK_TYPE_PHOTO, photo_node);
			photo->position = i;
			photos = g_list_prepend (photos, photo);
		}

		photos = g_list_reverse (photos);
		g_simple_async_result_set_op_res_gpointer (result, photos, (GDestroyNotify) _g_object_list_unref);

		json_node_free (node);
	}
	else
		g_simple_async_result_set_from_error (result, error);

	g_simple_async_result_complete_in_idle (result);
}


void
facebook_service_list_photos (FacebookService     *self,
			      FacebookAlbum       *album,
			      int                  limit,
			      const char          *after,
			      GCancellable        *cancellable,
			      GAsyncReadyCallback  callback,
			      gpointer             user_data)
{
	char        *uri;
	GHashTable  *data_set;
	SoupMessage *msg;

	g_return_if_fail (album != NULL);

	gth_task_progress (GTH_TASK (self),
			   _("Getting the photo list"),
			   NULL,
			   TRUE,
			   0.0);

	uri = g_strdup_printf ("https://graph.facebook.com/%s", album->id);
	data_set = g_hash_table_new (g_str_hash, g_str_equal);
	g_hash_table_insert (data_set, "fields", "photos");
	if (limit > 0) {
		char *s_limit = g_strdup_printf ("%d", limit);
		g_hash_table_insert (data_set, "limit", s_limit);
		g_free (s_limit);
	}
	if (after != NULL)
		g_hash_table_insert (data_set, "after", (gpointer) after);
	_facebook_service_add_access_token (self, data_set);
	msg = soup_form_request_new_from_hash ("GET", uri, data_set);
	_web_service_send_message (WEB_SERVICE (self),
				   msg,
				   cancellable,
				   callback,
				   user_data,
				   facebook_service_list_photos,
				   facebook_service_list_photos_ready_cb,
				   self);

	g_hash_table_destroy (data_set);
	g_free (uri);
}


GList *
facebook_service_list_photos_finish (FacebookService  *self,
				   GAsyncResult   *result,
				   GError        **error)
{
	if (g_simple_async_result_propagate_error (G_SIMPLE_ASYNC_RESULT (result), error))
		return NULL;
	else
		return _g_object_list_ref (g_simple_async_result_get_op_res_gpointer (G_SIMPLE_ASYNC_RESULT (result)));
}
