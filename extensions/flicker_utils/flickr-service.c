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
#include "flickr-account.h"
#include "flickr-consumer.h"
#include "flickr-photo.h"
#include "flickr-photoset.h"
#include "flickr-service.h"


#define IMAGES_PER_PAGE 500
#define RESPONSE_FORMAT "rest"


enum {
	PROP_0,
	PROP_SERVER
};


typedef struct {
	FlickrPrivacy        privacy_level;
	FlickrSafety         safety_level;
	gboolean             hidden;
	int                  max_width;
	int                  max_height;
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


typedef struct {
	FlickrPhotoset      *photoset;
	GList               *photo_ids;
	GCancellable        *cancellable;
	GAsyncReadyCallback  callback;
	gpointer             user_data;
	int                  n_files;
	GList               *current;
	int                  n_current;
} AddPhotosData;


static void
add_photos_data_free (AddPhotosData *add_photos)
{
	if (add_photos == NULL)
		return;
	_g_object_unref (add_photos->photoset);
	_g_string_list_free (add_photos->photo_ids);
	_g_object_unref (add_photos->cancellable);
	g_free (add_photos);
}


/* -- flickr_service -- */


struct _FlickrServicePrivate {
	PostPhotosData *post_photos;
	AddPhotosData  *add_photos;
	FlickrServer   *server;
	OAuthConsumer  *consumer;
	GChecksum      *checksum;
	char           *frob;
};


G_DEFINE_TYPE_WITH_CODE (FlickrService,
			 flickr_service,
			 OAUTH_TYPE_SERVICE,
			 G_ADD_PRIVATE (FlickrService))


static void
flickr_service_set_property (GObject      *object,
			     guint         property_id,
			     const GValue *value,
			     GParamSpec   *pspec)
{
	FlickrService *self;

        self = FLICKR_SERVICE (object);

	switch (property_id) {
	case PROP_SERVER:
		self->priv->server = g_value_get_pointer (value);
		self->priv->consumer = oauth_consumer_copy (&flickr_consumer);
		self->priv->consumer->request_token_url = self->priv->server->request_token_url;
		self->priv->consumer->access_token_url = self->priv->server->access_token_url;
		self->priv->consumer->consumer_key = self->priv->server->consumer_key;
		self->priv->consumer->consumer_secret = self->priv->server->consumer_secret;
		g_object_set (self, "consumer", self->priv->consumer, NULL);
		break;
	default:
		break;
	}
}


static void
flickr_service_get_property (GObject    *object,
			     guint       property_id,
			     GValue     *value,
			     GParamSpec *pspec)
{
	FlickrService *self;

        self = FLICKR_SERVICE (object);

	switch (property_id) {
	case PROP_SERVER:
		g_value_set_pointer (value, self->priv->server);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
		break;
	}
}


static void
flickr_service_finalize (GObject *object)
{
	FlickrService *self;

	self = FLICKR_SERVICE (object);

	post_photos_data_free (self->priv->post_photos);
	add_photos_data_free (self->priv->add_photos);
	oauth_consumer_free (self->priv->consumer);
	g_checksum_free (self->priv->checksum);
	g_free (self->priv->frob);

	G_OBJECT_CLASS (flickr_service_parent_class)->finalize (object);
}


/* -- flickr_service_old_auth_get_frob -- */


static void
flickr_service_old_auth_add_api_sig (FlickrService *self,
				     GHashTable    *data_set)
{
	GList *keys;
	GList *scan;

	g_hash_table_insert (data_set, "api_key", (gpointer) self->priv->server->consumer_key);
	if (oauth_service_get_token (OAUTH_SERVICE (self)) != NULL)
		g_hash_table_insert (data_set, "auth_token", (gpointer) oauth_service_get_token (OAUTH_SERVICE (self)));

	g_checksum_reset (self->priv->checksum);
	g_checksum_update (self->priv->checksum, (guchar *) self->priv->server->consumer_secret, -1);

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
flickr_service_old_auth_get_frob_ready_cb (SoupSession *session,
					   SoupMessage *msg,
					   gpointer     user_data)
{
	FlickrService      *self = user_data;
	GTask              *task;
	SoupBuffer         *body;
	DomDocument        *doc = NULL;
	GError             *error = NULL;

	g_free (self->priv->frob);
	self->priv->frob = NULL;

	task = _web_service_get_task (WEB_SERVICE (self));

	body = soup_message_body_flatten (msg->response_body);
	if (flickr_utils_parse_response (body, &doc, &error)) {
		DomElement *root;
		DomElement *child;

		root = DOM_ELEMENT (doc)->first_child;
		for (child = root->first_child; child; child = child->next_sibling)
			if (g_strcmp0 (child->tag_name, "frob") == 0)
				self->priv->frob = g_strdup (dom_element_get_inner_text (child));

		if (self->priv->frob == NULL)
			g_task_return_error (task, g_error_new_literal (WEB_SERVICE_ERROR, WEB_SERVICE_ERROR_GENERIC, _("Unknown error")));
		else
			g_task_return_boolean (task, TRUE);

		g_object_unref (doc);
	}
	else
		g_task_return_error (task, error);

	soup_buffer_free (body);
}


static void
flickr_service_old_auth_get_frob (FlickrService       *self,
				  GCancellable        *cancellable,
				  GAsyncReadyCallback  callback,
				  gpointer             user_data)
{
	GHashTable  *data_set;
	SoupMessage *msg;

	oauth_service_set_token (OAUTH_SERVICE (self), NULL);

	data_set = g_hash_table_new (g_str_hash, g_str_equal);
	g_hash_table_insert (data_set, "method", "flickr.auth.getFrob");
	flickr_service_old_auth_add_api_sig (self, data_set);
	msg = soup_form_request_new_from_hash ("GET", self->priv->server->rest_url, data_set);
	_web_service_send_message (WEB_SERVICE (self),
				   msg,
				   cancellable,
				   callback,
				   user_data,
				   flickr_service_old_auth_get_frob,
				   flickr_service_old_auth_get_frob_ready_cb,
				   self);

	g_hash_table_destroy (data_set);
}


static gboolean
flickr_service_old_auth_get_frob_finish (FlickrService  *self,
					 GAsyncResult   *result,
					 GError        **error)
{
	return g_task_propagate_boolean (G_TASK (result), error);
}


/* -- flickr_service_old_auth_get_token -- */


static void
flickr_service_old_auth_get_token_ready_cb (SoupSession *session,
					    SoupMessage *msg,
					    gpointer     user_data)
{
	FlickrService      *self = user_data;
	GTask              *task;
	SoupBuffer         *body;
	DomDocument        *doc = NULL;
	GError             *error = NULL;

	task = _web_service_get_task (WEB_SERVICE (self));

	body = soup_message_body_flatten (msg->response_body);
	if (flickr_utils_parse_response (body, &doc, &error)) {
		DomElement *response;
		DomElement *auth;
		const char *token;

		token = NULL;
		response = DOM_ELEMENT (doc)->first_child;
		for (auth = response->first_child; auth; auth = auth->next_sibling) {
			if (g_strcmp0 (auth->tag_name, "auth") == 0) {
				DomElement *node;

				for (node = auth->first_child; node; node = node->next_sibling) {
					if (g_strcmp0 (node->tag_name, "token") == 0) {
						token = dom_element_get_inner_text (node);
						oauth_service_set_token (OAUTH_SERVICE (self), token);
						break;
					}
				}

				for (node = auth->first_child; node; node = node->next_sibling) {
					if (g_strcmp0 (node->tag_name, "user") == 0) {
						FlickrAccount *account;

						account = g_object_new (FLICKR_TYPE_ACCOUNT,
									"id", dom_element_get_attribute (node, "nsid"),
									"username", dom_element_get_attribute (node, "username"),
									"name", dom_element_get_attribute (node, "fullname"),
									"token", token,
									NULL);
						g_task_return_pointer (task, account, g_object_unref);
						break;
					}
				}
			}
		}

		if (token == NULL)
			g_task_return_error (task, g_error_new_literal (WEB_SERVICE_ERROR, WEB_SERVICE_ERROR_GENERIC, _("Unknown error")));

		g_object_unref (doc);
	}
	else
		g_task_return_error (task, error);

	soup_buffer_free (body);
}


static void
flickr_service_old_auth_get_token (FlickrService       *self,
				   GCancellable        *cancellable,
				   GAsyncReadyCallback  callback,
				   gpointer             user_data)
{
	GHashTable  *data_set;
	SoupMessage *msg;

	oauth_service_set_token (OAUTH_SERVICE (self), NULL);

	data_set = g_hash_table_new (g_str_hash, g_str_equal);
	g_hash_table_insert (data_set, "method", "flickr.auth.getToken");
	g_hash_table_insert (data_set, "frob", self->priv->frob);
	flickr_service_old_auth_add_api_sig (self, data_set);
	msg = soup_form_request_new_from_hash ("GET", self->priv->server->rest_url, data_set);
	_web_service_send_message (WEB_SERVICE (self),
				   msg,
				   cancellable,
				   callback,
				   user_data,
				   flickr_service_old_auth_get_token,
				   flickr_service_old_auth_get_token_ready_cb,
				   self);

	g_hash_table_destroy (data_set);
}


static OAuthAccount *
flickr_service_old_auth_get_token_finish (FlickrService  *self,
					  GAsyncResult   *result,
					  GError        **error)
{
	return g_task_propagate_pointer (G_TASK (result), error);
}


static char *
old_auth_get_access_type_name (WebAuthorization access_type)
{
	char *name = NULL;

	switch (access_type) {
	case WEB_AUTHORIZATION_READ:
		name = "read";
		break;

	case WEB_AUTHORIZATION_WRITE:
		name = "write";
		break;
	}

	return name;
}


static char *
flickr_service_old_auth_get_login_link (FlickrService    *self,
					WebAuthorization  access_type)
{
	GHashTable *data_set;
	GString    *link;
	GList      *keys;
	GList      *scan;

	g_return_val_if_fail (self->priv->frob != NULL, NULL);

	data_set = g_hash_table_new (g_str_hash, g_str_equal);
	g_hash_table_insert (data_set, "frob", self->priv->frob);
	g_hash_table_insert (data_set, "perms", old_auth_get_access_type_name (access_type));
	flickr_service_old_auth_add_api_sig (self, data_set);

	link = g_string_new (self->priv->server->authorization_url);
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


/* -- flickr_service_ask_authorization -- */


#define _RESPONSE_CONTINUE 1
#define _RESPONSE_AUTHORIZE 2


static void
old_auth_token_ready_cb (GObject      *source_object,
			 GAsyncResult *res,
			 gpointer      user_data)
{
	FlickrService *self = user_data;
	GError        *error = NULL;
	OAuthAccount  *account;

	account = flickr_service_old_auth_get_token_finish (self, res, &error);

	if (account == NULL) {
		gtk_dialog_response (GTK_DIALOG (_web_service_get_auth_dialog (WEB_SERVICE (self))), GTK_RESPONSE_CANCEL);
		gth_task_completed (GTH_TASK (self), error);
		return;
	}

	web_service_set_current_account (WEB_SERVICE (self), account);
	gtk_dialog_response (GTK_DIALOG (_web_service_get_auth_dialog (WEB_SERVICE (self))), GTK_RESPONSE_OK);

	g_object_unref (account);
}


static void
old_authorization_complete (FlickrService *self)
{
	GtkWidget  *dialog;
	char       *text;
	char       *secondary_text;

	dialog = _web_service_get_auth_dialog (WEB_SERVICE (self));

	gtk_dialog_set_response_sensitive (GTK_DIALOG (dialog), _RESPONSE_AUTHORIZE, FALSE);
	gtk_dialog_set_response_sensitive (GTK_DIALOG (dialog), _RESPONSE_CONTINUE, TRUE);

	text = g_strdup_printf (_("Return to this window when you have finished the authorization process on %s"), self->priv->server->display_name);
	secondary_text = g_strdup (_("Once you’re done, click the “Continue” button below."));
	g_object_set (dialog, "text", text, "secondary-text", secondary_text, NULL);
	gtk_window_present (GTK_WINDOW (dialog));

	g_free (secondary_text);
	g_free (text);
}


static void
old_authorization_dialog_response_cb (GtkDialog *dialog,
				      int        response_id,
				      gpointer   user_data)
{
	FlickrService *self = user_data;

	switch (response_id) {
	case _RESPONSE_AUTHORIZE:
		{
			char   *url;
			GError *error = NULL;

			url = flickr_service_old_auth_get_login_link (self, WEB_AUTHORIZATION_WRITE);
			if (gtk_show_uri_on_window (GTK_WINDOW (dialog), url, GDK_CURRENT_TIME, &error))
				old_authorization_complete (self);
			else
				gth_task_completed (GTH_TASK (self), error);

			g_free (url);
		}
		break;

	case _RESPONSE_CONTINUE:
		gtk_widget_hide (GTK_WIDGET (dialog));
		gth_task_dialog (GTH_TASK (self), FALSE, NULL);
		flickr_service_old_auth_get_token (self,
						   gth_task_get_cancellable (GTH_TASK (self)),
						   old_auth_token_ready_cb,
						   self);
		break;

	default:
		break;
	}
}


static void
old_auth_frob_ready_cb (GObject      *source_object,
			GAsyncResult *result,
			gpointer      user_data)
{
	FlickrService *self = user_data;
	GError        *error = NULL;
	GtkWidget     *dialog;
	char          *text;
	char          *secondary_text;

	if (! flickr_service_old_auth_get_frob_finish (self, result, &error)) {
		gth_task_completed (GTH_TASK (self), error);
		return;
	}

	dialog = gtk_message_dialog_new (NULL,
				         GTK_DIALOG_MODAL,
					 GTK_MESSAGE_OTHER,
					 GTK_BUTTONS_NONE,
					 NULL);
	gtk_dialog_add_buttons (GTK_DIALOG (dialog),
			        _GTK_LABEL_CANCEL, GTK_RESPONSE_CANCEL,
				_("C_ontinue"), _RESPONSE_CONTINUE,
				_("_Authorize…"), _RESPONSE_AUTHORIZE,
				NULL);

	text = g_strdup_printf (_("gThumb requires your authorization to upload the photos to %s"), self->priv->server->display_name);
	secondary_text = g_strdup_printf (_("Click “Authorize” to open your web browser and authorize gthumb to upload photos to %s. When you’re finished, return to this window to complete the authorization."), self->priv->server->display_name);
	g_object_set (dialog, "text", text, "secondary-text", secondary_text, NULL);

	gtk_dialog_set_response_sensitive (GTK_DIALOG (dialog), _RESPONSE_AUTHORIZE, TRUE);
	gtk_dialog_set_response_sensitive (GTK_DIALOG (dialog), _RESPONSE_CONTINUE, FALSE);

	g_signal_connect (dialog,
			  "response",
			  G_CALLBACK (old_authorization_dialog_response_cb),
			  self);

	_web_service_set_auth_dialog (WEB_SERVICE (self), GTK_DIALOG (dialog));
	gtk_window_present (GTK_WINDOW (dialog));

	g_free (secondary_text);
	g_free (text);
}


static void
flickr_service_ask_authorization (WebService *base)
{
	FlickrService *self = FLICKR_SERVICE (base);

	if (self->priv->server->new_authentication) {
		WEB_SERVICE_CLASS (flickr_service_parent_class)->ask_authorization (base);
		return;
	}

	/* old authentication process, still used by 23hq.com */

	flickr_service_old_auth_get_frob (self,
					  gth_task_get_cancellable (GTH_TASK (self)),
					  old_auth_frob_ready_cb,
					  self);
}


static void
flickr_service_add_signature (FlickrService *self,
			      const char    *method,
			      const char    *url,
			      GHashTable    *parameters)
{
	if (self->priv->server->new_authentication)
		oauth_service_add_signature (OAUTH_SERVICE (self), method, url, parameters);
	else
		flickr_service_old_auth_add_api_sig (self, parameters);
}


/* -- flickr_service_get_user_info -- */


static void
get_user_info_ready_cb (SoupSession *session,
			SoupMessage *msg,
			gpointer     user_data)
{
	FlickrService      *self = user_data;
	GTask              *task;
	SoupBuffer         *body;
	DomDocument        *doc = NULL;
	GError             *error = NULL;

	task = _web_service_get_task (WEB_SERVICE (self));

	if (msg->status_code != 200) {
		g_task_return_new_error (task,
					 SOUP_HTTP_ERROR,
					 msg->status_code,
					 "%s",
					 soup_status_get_phrase (msg->status_code));
		return;
	}

	body = soup_message_body_flatten (msg->response_body);
	if (flickr_utils_parse_response (body, &doc, &error)) {
		OAuthAccount *account;
		DomElement   *response;
		DomElement   *node;
		gboolean      success = FALSE;

		account = _g_object_ref (web_service_get_current_account (WEB_SERVICE (self)));
		if (account == NULL)
			account = g_object_new (FLICKR_TYPE_ACCOUNT,
						"token", oauth_service_get_token (OAUTH_SERVICE (self)),
						"token-secret", oauth_service_get_token_secret (OAUTH_SERVICE (self)),
						NULL);

		response = DOM_ELEMENT (doc)->first_child;
		for (node = response->first_child; node; node = node->next_sibling) {
			if (g_strcmp0 (node->tag_name, "user") == 0) {
				success = TRUE;
				flickr_account_load_extra_data (FLICKR_ACCOUNT (account), node);
				g_task_return_pointer (task, g_object_ref (account), (GDestroyNotify) g_object_unref);
			}
		}

		if (! success)
			g_task_return_error (task, g_error_new_literal (WEB_SERVICE_ERROR, WEB_SERVICE_ERROR_GENERIC, _("Unknown error")));

		g_object_unref (account);
		g_object_unref (doc);
	}
	else
		g_task_return_error (task, error);

	soup_buffer_free (body);
}


static void
flickr_service_get_user_info (WebService          *base,
			      GCancellable        *cancellable,
			      GAsyncReadyCallback  callback,
			      gpointer             user_data)
{
	FlickrService *self = FLICKR_SERVICE (base);
	OAuthAccount  *account;
	GHashTable    *data_set;
	SoupMessage   *msg;

	account = web_service_get_current_account (WEB_SERVICE (self));
	if (account != NULL) {
		oauth_service_set_token (OAUTH_SERVICE (self), account->token);
		oauth_service_set_token_secret (OAUTH_SERVICE (self), account->token_secret);
	}

	data_set = g_hash_table_new (g_str_hash, g_str_equal);
	g_hash_table_insert (data_set, "format", RESPONSE_FORMAT);
	g_hash_table_insert (data_set, "method", "flickr.people.getUploadStatus");
	flickr_service_add_signature (self, "GET", self->priv->server->rest_url, data_set);
	msg = soup_form_request_new_from_hash ("GET", self->priv->server->rest_url, data_set);
	_web_service_send_message (WEB_SERVICE (self),
				   msg,
				   cancellable,
				   callback,
				   user_data,
				   flickr_service_get_user_info,
				   get_user_info_ready_cb,
				   self);

	g_hash_table_destroy (data_set);
}


static void
flickr_service_class_init (FlickrServiceClass *klass)
{
	GObjectClass    *object_class;
	WebServiceClass *service_class;

	object_class = (GObjectClass*) klass;
	object_class->set_property = flickr_service_set_property;
	object_class->get_property = flickr_service_get_property;
	object_class->finalize = flickr_service_finalize;

	service_class = (WebServiceClass*) klass;
	service_class->ask_authorization = flickr_service_ask_authorization;
	service_class->get_user_info = flickr_service_get_user_info;

	/* properties */

	g_object_class_install_property (object_class,
					 PROP_SERVER,
					 g_param_spec_pointer ("server",
                                                               "Server",
                                                               "",
                                                               G_PARAM_READWRITE));
}


static void
flickr_service_init (FlickrService *self)
{
	self->priv = flickr_service_get_instance_private (self);
	self->priv->post_photos = NULL;
	self->priv->add_photos = NULL;
	self->priv->server = NULL;
	self->priv->checksum = g_checksum_new (G_CHECKSUM_MD5);
	self->priv->frob = NULL;
}


FlickrService *
flickr_service_new (FlickrServer  *server,
		    GCancellable  *cancellable,
		    GtkWidget     *browser,
		    GtkWidget     *dialog)
{
	g_return_val_if_fail (server != NULL, NULL);

	return g_object_new (FLICKR_TYPE_SERVICE,
			     "service-name", server->name,
			     "service-address", server->url,
			     "service-protocol", server->protocol,
			     "account-type", FLICKR_TYPE_ACCOUNT,
			     "cancellable", cancellable,
			     "browser", browser,
			     "dialog", dialog,
			     "server", server,
			     NULL);
}


FlickrServer *
flickr_service_get_server (FlickrService *self)
{
	return self->priv->server;
}


/* -- flickr_service_list_photosets -- */


static void
list_photosets_ready_cb (SoupSession *session,
			 SoupMessage *msg,
			 gpointer     user_data)
{
	FlickrService      *self = user_data;
	GTask              *task;
	SoupBuffer         *body;
	DomDocument        *doc = NULL;
	GError             *error = NULL;

	task = _web_service_get_task (WEB_SERVICE (self));

	if (msg->status_code != 200) {
		g_task_return_new_error (task,
					 SOUP_HTTP_ERROR,
					 msg->status_code,
					 "%s",
					 soup_status_get_phrase (msg->status_code));
		return;
	}

	body = soup_message_body_flatten (msg->response_body);
	if (flickr_utils_parse_response (body, &doc, &error)) {
		DomElement *response;
		DomElement *node;
		GList      *photosets = NULL;

		response = DOM_ELEMENT (doc)->first_child;
		for (node = response->first_child; node; node = node->next_sibling) {
			if (g_strcmp0 (node->tag_name, "photosets") == 0) {
				DomElement *child;

				for (child = node->first_child; child; child = child->next_sibling) {
					if (g_strcmp0 (child->tag_name, "photoset") == 0) {
						FlickrPhotoset *photoset;

						photoset = flickr_photoset_new ();
						dom_domizable_load_from_element (DOM_DOMIZABLE (photoset), child);
						photosets = g_list_prepend (photosets, photoset);
					}
				}
			}
		}

		photosets = g_list_reverse (photosets);
		g_task_return_pointer (task, photosets, (GDestroyNotify) _g_object_list_unref);

		g_object_unref (doc);
	}
	else
		g_task_return_error (task, error);

	soup_buffer_free (body);
}


void
flickr_service_list_photosets (FlickrService       *self,
			       GCancellable        *cancellable,
			       GAsyncReadyCallback  callback,
			       gpointer             user_data)
{
	GHashTable  *data_set;
	SoupMessage *msg;

	gth_task_progress (GTH_TASK (self), _("Getting the album list"), NULL, TRUE, 0.0);

	data_set = g_hash_table_new (g_str_hash, g_str_equal);
	g_hash_table_insert (data_set, "format", RESPONSE_FORMAT);
	g_hash_table_insert (data_set, "method", "flickr.photosets.getList");
	flickr_service_add_signature (self, "GET", self->priv->server->rest_url, data_set);
	msg = soup_form_request_new_from_hash ("GET", self->priv->server->rest_url, data_set);
	_web_service_send_message (WEB_SERVICE (self),
				   msg,
				   cancellable,
				   callback,
				   user_data,
				   flickr_service_list_photosets,
				   list_photosets_ready_cb,
				   self);

	g_hash_table_destroy (data_set);
}


GList *
flickr_service_list_photosets_finish (FlickrService  *service,
				      GAsyncResult   *result,
				      GError        **error)
{
	return g_task_propagate_pointer (G_TASK (result), error);
}


/* -- flickr_service_create_photoset_finish -- */


static void
create_photoset_ready_cb (SoupSession *session,
			  SoupMessage *msg,
			  gpointer     user_data)
{
	FlickrService      *self = user_data;
	GTask              *task;
	SoupBuffer         *body;
	DomDocument        *doc = NULL;
	GError             *error = NULL;

	task = _web_service_get_task (WEB_SERVICE (self));

	if (msg->status_code != 200) {
		g_task_return_new_error (task,
					 SOUP_HTTP_ERROR,
					 msg->status_code,
					 "%s",
					 soup_status_get_phrase (msg->status_code));
		return;
	}

	body = soup_message_body_flatten (msg->response_body);
	if (flickr_utils_parse_response (body, &doc, &error)) {
		DomElement     *response;
		DomElement     *node;
		FlickrPhotoset *photoset = NULL;

		response = DOM_ELEMENT (doc)->first_child;
		for (node = response->first_child; node; node = node->next_sibling) {
			if (g_strcmp0 (node->tag_name, "photoset") == 0) {
				photoset = flickr_photoset_new ();
				dom_domizable_load_from_element (DOM_DOMIZABLE (photoset), node);
				g_task_return_pointer (task, photoset, (GDestroyNotify) g_object_unref);
			}
		}

		if (photoset == NULL)
			g_task_return_error (task, g_error_new_literal (WEB_SERVICE_ERROR, WEB_SERVICE_ERROR_GENERIC, _("Unknown error")));

		g_object_unref (doc);
	}
	else
		g_task_return_error (task, error);

	soup_buffer_free (body);
}


void
flickr_service_create_photoset (FlickrService       *self,
				FlickrPhotoset      *photoset,
				GCancellable        *cancellable,
				GAsyncReadyCallback  callback,
				gpointer             user_data)
{
	GHashTable  *data_set;
	SoupMessage *msg;

	g_return_if_fail (photoset != NULL);
	g_return_if_fail (photoset->primary != NULL);

	gth_task_progress (GTH_TASK (self), _("Creating the new album"), NULL, TRUE, 0.0);

	data_set = g_hash_table_new (g_str_hash, g_str_equal);
	g_hash_table_insert (data_set, "format", RESPONSE_FORMAT);
	g_hash_table_insert (data_set, "method", "flickr.photosets.create");
	g_hash_table_insert (data_set, "title", photoset->title);
	g_hash_table_insert (data_set, "primary_photo_id", photoset->primary);
	flickr_service_add_signature (self, "GET", self->priv->server->rest_url, data_set);
	msg = soup_form_request_new_from_hash ("GET", self->priv->server->rest_url, data_set);
	_web_service_send_message (WEB_SERVICE (self),
				   msg,
				   cancellable,
				   callback,
				   user_data,
				   flickr_service_create_photoset,
				   create_photoset_ready_cb,
				   self);

	g_hash_table_destroy (data_set);
}


FlickrPhotoset *
flickr_service_create_photoset_finish (FlickrService  *self,
				       GAsyncResult   *result,
				       GError        **error)
{
	return g_task_propagate_pointer (G_TASK (result), error);
}


/* -- flickr_service_add_photos_to_set -- */


static void
add_photos_to_set_done (FlickrService *self,
			GError        *error)
{
	GTask *task;

	task = _web_service_get_task (WEB_SERVICE (self));
	if (task == NULL)
		task = g_task_new (G_OBJECT (self),
				   NULL,
				   self->priv->add_photos->callback,
				   self->priv->add_photos->user_data);

	if (error == NULL)
		g_task_return_boolean (task, TRUE);
	else
		g_task_return_error (task, g_error_copy (error));
}


static void add_current_photo_to_set (FlickrService *self);


static void
add_next_photo_to_set (FlickrService *self)
{
	self->priv->add_photos->current = self->priv->add_photos->current->next;
	self->priv->add_photos->n_current += 1;
	add_current_photo_to_set (self);
}


static void
add_current_photo_to_set_ready_cb (SoupSession *session,
				   SoupMessage *msg,
				   gpointer     user_data)
{
	FlickrService      *self = user_data;
	GTask              *task;
	SoupBuffer         *body;
	DomDocument        *doc = NULL;
	GError             *error = NULL;

	task = _web_service_get_task (WEB_SERVICE (self));

	if (msg->status_code != 200) {
		g_task_return_new_error (task,
					 SOUP_HTTP_ERROR,
					 msg->status_code,
					 "%s",
					 soup_status_get_phrase (msg->status_code));
		return;
	}

	body = soup_message_body_flatten (msg->response_body);
	if (! flickr_utils_parse_response (body, &doc, &error)) {
		soup_buffer_free (body);
		add_photos_to_set_done (self, error);
		return;
	}

	g_object_unref (doc);
	soup_buffer_free (body);

	add_next_photo_to_set (self);
}


static void
add_current_photo_to_set (FlickrService *self)
{
	char        *photo_id;
	GHashTable  *data_set;
	SoupMessage *msg;

	if (self->priv->add_photos->current == NULL) {
		add_photos_to_set_done (self, NULL);
		return;
	}

	gth_task_progress (GTH_TASK (self),
			   _("Creating the new album"),
			   "",
			   FALSE,
			   (double) self->priv->add_photos->n_current / (self->priv->add_photos->n_files + 1));

	photo_id = self->priv->add_photos->current->data;
	if (g_strcmp0 (photo_id, self->priv->add_photos->photoset->primary) == 0) {
		add_next_photo_to_set (self);
		return;
	}

	data_set = g_hash_table_new (g_str_hash, g_str_equal);
	g_hash_table_insert (data_set, "format", RESPONSE_FORMAT);
	g_hash_table_insert (data_set, "method", "flickr.photosets.addPhoto");
	g_hash_table_insert (data_set, "photoset_id", self->priv->add_photos->photoset->id);
	g_hash_table_insert (data_set, "photo_id", photo_id);
	flickr_service_add_signature (self, "POST", self->priv->server->rest_url, data_set);
	msg = soup_form_request_new_from_hash ("POST", self->priv->server->rest_url, data_set);
	_web_service_send_message (WEB_SERVICE (self),
				   msg,
				   self->priv->add_photos->cancellable,
				   self->priv->add_photos->callback,
				   self->priv->add_photos->user_data,
				   flickr_service_add_photos_to_set,
				   add_current_photo_to_set_ready_cb,
				   self);

	g_hash_table_destroy (data_set);
}


void
flickr_service_add_photos_to_set (FlickrService        *self,
				  FlickrPhotoset       *photoset,
				  GList                *photo_ids,
				  GCancellable         *cancellable,
				  GAsyncReadyCallback   callback,
				  gpointer              user_data)
{
	gth_task_progress (GTH_TASK (self), _("Creating the new album"), NULL, TRUE, 0.0);

	add_photos_data_free (self->priv->add_photos);
	self->priv->add_photos = g_new0 (AddPhotosData, 1);
	self->priv->add_photos->photoset = _g_object_ref (photoset);
	self->priv->add_photos->photo_ids = _g_string_list_dup (photo_ids);
	self->priv->add_photos->cancellable = _g_object_ref (cancellable);
	self->priv->add_photos->callback = callback;
	self->priv->add_photos->user_data = user_data;
	self->priv->add_photos->n_files = g_list_length (self->priv->add_photos->photo_ids);
	self->priv->add_photos->current = self->priv->add_photos->photo_ids;
	self->priv->add_photos->n_current = 1;

	_web_service_reset_task (WEB_SERVICE (self));
	add_current_photo_to_set (self);
}


gboolean
flickr_service_add_photos_to_set_finish (FlickrService  *self,
					 GAsyncResult   *result,
					 GError        **error)
{
	return g_task_propagate_boolean (G_TASK (result), error);
}


/* -- flickr_service_post_photos -- */


static void
post_photos_done (FlickrService *self,
		  GError        *error)
{
	GTask *task;

	task = _web_service_get_task (WEB_SERVICE (self));
	if (error == NULL) {
		self->priv->post_photos->ids = g_list_reverse (self->priv->post_photos->ids);
		g_task_return_pointer (task, self->priv->post_photos->ids, (GDestroyNotify) _g_string_list_free);
		self->priv->post_photos->ids = NULL;
	}
	else {
		if (self->priv->post_photos->current != NULL) {
			GthFileData *file_data = self->priv->post_photos->current->data;
			char        *msg;

			msg = g_strdup_printf (_("Could not upload “%s”: %s"), g_file_info_get_display_name (file_data->info), error->message);
			g_free (error->message);
			error->message = msg;
		}
		g_task_return_error (task, error);
	}
}


static void flickr_service_post_current_file (FlickrService *self);


static void
post_photo_ready_cb (SoupSession *session,
		     SoupMessage *msg,
		     gpointer     user_data)
{
	FlickrService *self = user_data;
	SoupBuffer    *body;
	DomDocument   *doc = NULL;
	GError        *error = NULL;
	GthFileData   *file_data;

	if (msg->status_code != 200) {
		GError *error;

		error = g_error_new_literal (SOUP_HTTP_ERROR, msg->status_code, soup_status_get_phrase (msg->status_code));
		post_photos_done (self, error);
		g_error_free (error);

		return;
	}

	body = soup_message_body_flatten (msg->response_body);
	if (flickr_utils_parse_response (body, &doc, &error)) {
		DomElement *response;
		DomElement *node;

		/* save the file id */

		response = DOM_ELEMENT (doc)->first_child;
		for (node = response->first_child; node; node = node->next_sibling) {
			if (g_strcmp0 (node->tag_name, "photoid") == 0) {
				const char *id;

				id = dom_element_get_inner_text (node);
				self->priv->post_photos->ids = g_list_prepend (self->priv->post_photos->ids, g_strdup (id));
			}
		}

		g_object_unref (doc);
	}
	else {
		soup_buffer_free (body);
		post_photos_done (self, error);
		return;
	}

	soup_buffer_free (body);

	file_data = self->priv->post_photos->current->data;
	self->priv->post_photos->uploaded_size += g_file_info_get_size (file_data->info);
	self->priv->post_photos->current = self->priv->post_photos->current->next;
	flickr_service_post_current_file (self);
}


static char *
get_safety_value (FlickrSafety safety_level)
{
	char *value = NULL;

	switch (safety_level) {
	case FLICKR_SAFETY_SAFE:
		value = "1";
		break;

	case FLICKR_SAFETY_MODERATE:
		value = "2";
		break;

	case FLICKR_SAFETY_RESTRICTED:
		value = "3";
		break;
	}

	return value;
}


static void
upload_photo_wrote_body_data_cb (SoupMessage *msg,
                		 SoupBuffer  *chunk,
                		 gpointer     user_data)
{
	FlickrService *self = user_data;
	GthFileData   *file_data;
	char          *details;
	double         current_file_fraction;

	if (self->priv->post_photos->current == NULL)
		return;

	self->priv->post_photos->wrote_body_data_size += chunk->length;
	if (self->priv->post_photos->wrote_body_data_size > msg->request_body->length)
		return;

	file_data = self->priv->post_photos->current->data;
	/* Translators: %s is a filename */
	details = g_strdup_printf (_("Uploading “%s”"), g_file_info_get_display_name (file_data->info));
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
	FlickrService *self = user_data;
	GthFileData   *file_data;
	SoupMultipart *multipart;
	char          *uri;
	SoupBuffer    *body;
	void          *resized_buffer;
	gsize          resized_count;
	SoupMessage   *msg;

	if (error != NULL) {
		post_photos_done (self, error);
		return;
	}

	file_data = self->priv->post_photos->current->data;
	multipart = soup_multipart_new ("multipart/form-data");

	/* the metadata part */

	{
		GHashTable *data_set;
		char       *title;
		char       *description;
		char       *tags;
		GObject    *metadata;
		GList      *keys;
		GList      *scan;

		data_set = g_hash_table_new (g_str_hash, g_str_equal);
		g_hash_table_insert (data_set, "format", RESPONSE_FORMAT);

		title = gth_file_data_get_attribute_as_string (file_data, "general::title");
		if (title != NULL)
			g_hash_table_insert (data_set, "title", title);

		description = gth_file_data_get_attribute_as_string (file_data, "general::description");
		if (description != NULL)
			g_hash_table_insert (data_set, "description", description);

		tags = NULL;
		metadata = g_file_info_get_attribute_object (file_data->info, "general::tags");
		if (metadata != NULL)
			tags = gth_string_list_join (GTH_STRING_LIST (gth_metadata_get_string_list (GTH_METADATA (metadata))), " ");
		if (tags != NULL)
			g_hash_table_insert (data_set, "tags", tags);

		g_hash_table_insert (data_set, "is_public", (self->priv->post_photos->privacy_level == FLICKR_PRIVACY_PUBLIC) ? "1" : "0");
		g_hash_table_insert (data_set, "is_friend", ((self->priv->post_photos->privacy_level == FLICKR_PRIVACY_FRIENDS) || (self->priv->post_photos->privacy_level == FLICKR_PRIVACY_FRIENDS_FAMILY)) ? "1" : "0");
		g_hash_table_insert (data_set, "is_family", ((self->priv->post_photos->privacy_level == FLICKR_PRIVACY_FAMILY) || (self->priv->post_photos->privacy_level == FLICKR_PRIVACY_FRIENDS_FAMILY)) ? "1" : "0");
		g_hash_table_insert (data_set, "safety_level", get_safety_value (self->priv->post_photos->safety_level));
		g_hash_table_insert (data_set, "hidden", self->priv->post_photos->hidden ? "2" : "1");
		flickr_service_add_signature (self, "POST", self->priv->server->upload_url, data_set);

		keys = g_hash_table_get_keys (data_set);
		for (scan = keys; scan; scan = scan->next) {
			char *key = scan->data;
			soup_multipart_append_form_string (multipart, key, g_hash_table_lookup (data_set, key));
		}

		g_free (tags);
		g_list_free (keys);
		g_free (description);
		g_free (title);
		g_hash_table_unref (data_set);
	}

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

	uri = g_file_get_uri (file_data->file);
	soup_multipart_append_form_file (multipart,
					 "photo",
					 uri,
					 gth_file_data_get_mime_type (file_data),
					 body);

	soup_buffer_free (body);
	g_free (uri);

	/* send the file */

	self->priv->post_photos->wrote_body_data_size = 0;
	msg = soup_form_request_new_from_multipart (self->priv->server->upload_url, multipart);
	g_signal_connect (msg,
			  "wrote-body-data",
			  (GCallback) upload_photo_wrote_body_data_cb,
			  self);

	_web_service_send_message (WEB_SERVICE (self),
				   msg,
				   self->priv->post_photos->cancellable,
				   self->priv->post_photos->callback,
				   self->priv->post_photos->user_data,
				   flickr_service_post_photos,
				   post_photo_ready_cb,
				   self);

	soup_multipart_free (multipart);
}


static void
flickr_service_post_current_file (FlickrService *self)
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
	FlickrService *self = user_data;
	GList         *scan;

	if (error != NULL) {
		post_photos_done (self, error);
		return;
	}

	self->priv->post_photos->file_list = _g_object_list_ref (files);
	for (scan = self->priv->post_photos->file_list; scan; scan = scan->next) {
		GthFileData *file_data = scan->data;

		self->priv->post_photos->total_size += g_file_info_get_size (file_data->info);
		self->priv->post_photos->n_files += 1;
	}

	self->priv->post_photos->current = self->priv->post_photos->file_list;
	flickr_service_post_current_file (self);
}


void
flickr_service_post_photos (FlickrService       *self,
			    FlickrPrivacy	 privacy_level,
			    FlickrSafety	 safety_level,
			    gboolean             hidden,
			    int                  max_width,
			    int                  max_height,
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
	self->priv->post_photos->privacy_level = privacy_level;
	self->priv->post_photos->safety_level = safety_level;
	self->priv->post_photos->hidden = hidden;
	self->priv->post_photos->max_width = max_width;
	self->priv->post_photos->max_height = max_height;
	self->priv->post_photos->cancellable = _g_object_ref (cancellable);
	self->priv->post_photos->callback = callback;
	self->priv->post_photos->user_data = user_data;
	self->priv->post_photos->total_size = 0;
	self->priv->post_photos->n_files = 0;

	_g_query_all_metadata_async (file_list,
				     GTH_LIST_DEFAULT,
				     "*",
				     self->priv->post_photos->cancellable,
				     post_photos_info_ready_cb,
				     self);
}


GList *
flickr_service_post_photos_finish (FlickrService  *self,
				   GAsyncResult   *result,
				   GError        **error)
{
	return g_task_propagate_pointer (G_TASK (result), error);
}


/* -- flickr_service_list_photos -- */


typedef struct {
	FlickrService       *self;
	FlickrPhotoset      *photoset;
	char                *extras;
	GCancellable        *cancellable;
	GAsyncReadyCallback  callback;
	gpointer             user_data;
	GList               *photos;
	int                  position;
} FlickrListPhotosData;


static void
flickr_list_photos_data_free (FlickrListPhotosData *data)
{
	_g_object_unref (data->self);
	_g_object_unref (data->photoset);
	g_free (data->extras);
	_g_object_unref (data->cancellable);
	g_free (data);
}


static void
flickr_service_list_photoset_page (FlickrListPhotosData *data,
				   int                   page);


static void
flickr_service_list_photoset_paged_ready_cb (SoupSession *session,
					     SoupMessage *msg,
					     gpointer     user_data)
{
	FlickrListPhotosData *data = user_data;
	FlickrService        *self = data->self;
	GTask                *task;
	SoupBuffer           *body;
	DomDocument          *doc = NULL;
	GError               *error = NULL;

	task = _web_service_get_task (WEB_SERVICE (self));

	if (msg->status_code != 200) {
		g_task_return_new_error (task,
					 SOUP_HTTP_ERROR,
					 msg->status_code,
					 "%s",
					 soup_status_get_phrase (msg->status_code));
		flickr_list_photos_data_free (data);
		return;
	}

	body = soup_message_body_flatten (msg->response_body);
	if (flickr_utils_parse_response (body, &doc, &error)) {
		DomElement *response;
		DomElement *node;
		int         pages = 0;
		int         page = 0;

		response = DOM_ELEMENT (doc)->first_child;
		for (node = response->first_child; node; node = node->next_sibling) {
			if (g_strcmp0 (node->tag_name, "photoset") == 0) {
				DomElement *child;

				for (child = node->first_child; child; child = child->next_sibling) {
					if (g_strcmp0 (child->tag_name, "photo") == 0) {
						FlickrPhoto *photo;

						photo = flickr_photo_new (self->priv->server);
						dom_domizable_load_from_element (DOM_DOMIZABLE (photo), child);
						photo->position = data->position++;
						data->photos = g_list_prepend (data->photos, photo);
					}
				}

				pages = dom_element_get_attribute_as_int (node, "pages");
				page = dom_element_get_attribute_as_int (node, "page");
			}
		}

		if (page > pages) {
			g_task_return_new_error (task,
						 SOUP_HTTP_ERROR,
						 0,
						 "%s",
						 "Invalid data");
			flickr_list_photos_data_free (data);
		}
		else if (page < pages) {
			/* read the next page */
			flickr_service_list_photoset_page (data, page + 1);
		}
		else { /* page == pages */
			data->photos = g_list_reverse (data->photos);
			g_task_return_pointer (task,
					       _g_object_list_ref (data->photos),
					       (GDestroyNotify) _g_object_list_unref);
			flickr_list_photos_data_free (data);
		}

		g_object_unref (doc);
	}
	else
		g_task_return_error (task, error);

	soup_buffer_free (body);
}


static void
flickr_service_list_photoset_page (FlickrListPhotosData *data,
				   int                   n_page)
{
	FlickrService *self = data->self;
	GHashTable    *data_set;
	char          *page = NULL;
	char          *per_page = NULL;
	SoupMessage   *msg;

	g_return_if_fail (data->photoset != NULL);

	gth_task_progress (GTH_TASK (self),
			   _("Getting the photo list"),
			   NULL,
			   TRUE,
			   0.0);

	data_set = g_hash_table_new (g_str_hash, g_str_equal);
	g_hash_table_insert (data_set, "format", RESPONSE_FORMAT);
	g_hash_table_insert (data_set, "method", "flickr.photosets.getPhotos");
	g_hash_table_insert (data_set, "photoset_id", data->photoset->id);
	if (data->extras != NULL)
		g_hash_table_insert (data_set, "extras", (char *) data->extras);

	if (n_page > 0) {
		page = g_strdup_printf ("%d", IMAGES_PER_PAGE);
		g_hash_table_insert (data_set, "per_page", page);

		per_page = g_strdup_printf ("%d", n_page);
		g_hash_table_insert (data_set, "page", per_page);
	}

	flickr_service_add_signature (self, "GET", self->priv->server->rest_url, data_set);
	msg = soup_form_request_new_from_hash ("GET", self->priv->server->rest_url, data_set);
	_web_service_send_message (WEB_SERVICE (self),
				   msg,
				   data->cancellable,
				   data->callback,
				   data->user_data,
				   flickr_service_list_photos,
				   flickr_service_list_photoset_paged_ready_cb,
				   data);

	g_free (per_page);
	g_free (page);
	g_hash_table_destroy (data_set);
}


void
flickr_service_list_photos (FlickrService       *self,
			    FlickrPhotoset      *photoset,
			    const char          *extras,
			    GCancellable        *cancellable,
			    GAsyncReadyCallback  callback,
			    gpointer             user_data)
{
	FlickrListPhotosData *data;

	data = g_new0 (FlickrListPhotosData, 1);
	data->self = _g_object_ref (self);
	data->photoset = _g_object_ref (photoset);
	data->extras = g_strdup (extras);
	data->cancellable = _g_object_ref (cancellable);
	data->callback = callback;
	data->user_data = user_data;
	data->photos = NULL;
	data->position = 0;

	flickr_service_list_photoset_page (data, 1);
}


GList *
flickr_service_list_photos_finish (FlickrService  *self,
				   GAsyncResult   *result,
				   GError        **error)
{
	return g_task_propagate_pointer (G_TASK (result), error);
}
