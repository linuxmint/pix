/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2012 Free Software Foundation, Inc.
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
#include "oauth-ask-authorization-dialog.h"
#include "oauth-consumer.h"
#include "oauth-service.h"


#define OAUTH_VERSION "1.0"
#define OAUTH_SIGNATURE_METHOD "HMAC-SHA1"
#define OAUTH_CALLBACK "http://localhost/"


enum {
	PROP_0,
	PROP_CONSUMER
};


struct _OAuthServicePrivate {
	OAuthConsumer *consumer;
	char          *timestamp;
	char          *nonce;
	char          *signature;
	char          *token;
	char          *token_secret;
	char          *verifier;
};


G_DEFINE_TYPE_WITH_CODE (OAuthService,
			 oauth_service,
			 WEB_TYPE_SERVICE,
			 G_ADD_PRIVATE (OAuthService))


static void
oauth_service_set_property (GObject      *object,
			    guint         property_id,
			    const GValue *value,
			    GParamSpec   *pspec)
{
	OAuthService *self;

        self = OAUTH_SERVICE (object);

	switch (property_id) {
	case PROP_CONSUMER:
		self->priv->consumer = g_value_get_pointer (value);
		break;
	default:
		break;
	}
}


static void
oauth_service_get_property (GObject    *object,
			    guint       property_id,
			    GValue     *value,
			    GParamSpec *pspec)
{
	OAuthService *self;

        self = OAUTH_SERVICE (object);

	switch (property_id) {
	case PROP_CONSUMER:
		g_value_set_pointer (value, self->priv->consumer);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
		break;
	}
}


static void
oauth_service_finalize (GObject *object)
{
	OAuthService *self;

	self = OAUTH_SERVICE (object);

	g_free (self->priv->verifier);
	g_free (self->priv->token);
	g_free (self->priv->token_secret);
	g_free (self->priv->signature);
	g_free (self->priv->nonce);
	g_free (self->priv->timestamp);

	G_OBJECT_CLASS (oauth_service_parent_class)->finalize (object);
}


/* -- oauth_service_get_request_token -- */


static void
_oauth_service_get_request_token_ready_cb (SoupSession *session,
					   SoupMessage *msg,
					   gpointer     user_data)
{
	OAuthService       *self = user_data;
	GTask              *task;
	SoupBuffer         *body;
	GHashTable         *values;
	char               *token;
	char               *token_secret;

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
	values = soup_form_decode (body->data);
	token = g_hash_table_lookup (values, "oauth_token");
	token_secret = g_hash_table_lookup (values, "oauth_token_secret");
	if ((token != NULL) && (token_secret != NULL)) {
		oauth_service_set_token (self, token);
		oauth_service_set_token_secret (self, token_secret);
		g_task_return_boolean (task, TRUE);
	}
	else
		g_task_return_error (task, g_error_new_literal (WEB_SERVICE_ERROR, WEB_SERVICE_ERROR_GENERIC, _("Unknown error")));

	g_hash_table_destroy (values);
	soup_buffer_free (body);
}


static void
_oauth_service_get_request_token (OAuthService        *self,
				  GCancellable        *cancellable,
				  GAsyncReadyCallback  callback,
				  gpointer             user_data)
{
	GHashTable  *data_set;
	SoupMessage *msg;

	data_set = g_hash_table_new (g_str_hash, g_str_equal);
	g_hash_table_insert (data_set, "oauth_callback", OAUTH_CALLBACK);
	oauth_service_add_signature (self, "POST", self->priv->consumer->request_token_url, data_set);
	msg = soup_form_request_new_from_hash ("POST", self->priv->consumer->request_token_url, data_set);
	_web_service_send_message (WEB_SERVICE (self),
				   msg,
				   cancellable,
				   callback,
				   user_data,
				   _oauth_service_get_request_token,
				   _oauth_service_get_request_token_ready_cb,
				   self);

	g_hash_table_destroy (data_set);
}


static gboolean
oauth_service_get_request_token_finish (OAuthService  *self,
					GAsyncResult  *result,
					GError       **error)
{
	return g_task_propagate_boolean (G_TASK (result), error);
}


/* -- _oauth_service_get_access_token -- */


static void
_oauth_service_get_access_token_ready_cb (SoupSession *session,
					  SoupMessage *msg,
					  gpointer     user_data)
{
	OAuthService       *self = user_data;
	GTask              *task;
	SoupBuffer         *body;

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
	self->priv->consumer->access_token_response (self, msg, body, task);

	soup_buffer_free (body);
}


static void
_oauth_service_get_access_token (OAuthService        *self,
				 const char          *verifier,
				 GCancellable        *cancellable,
				 GAsyncReadyCallback  callback,
				 gpointer             user_data)
{
	GHashTable  *data_set;
	SoupMessage *msg;

	data_set = g_hash_table_new (g_str_hash, g_str_equal);
	if (verifier != NULL)
		g_hash_table_insert (data_set, "oauth_verifier", (gpointer) verifier);
	oauth_service_add_signature (self, "POST", self->priv->consumer->access_token_url, data_set);
	msg = soup_form_request_new_from_hash ("POST", self->priv->consumer->access_token_url, data_set);
	_web_service_send_message (WEB_SERVICE (self),
				   msg,
				   cancellable,
				   callback,
				   user_data,
				   _oauth_service_get_access_token,
				   _oauth_service_get_access_token_ready_cb,
				   self);

	g_hash_table_destroy (data_set);
}


static OAuthAccount *
_oauth_service_get_access_token_finish (OAuthService  *self,
					GAsyncResult  *result,
					GError       **error)
{
	return g_task_propagate_pointer (G_TASK (result), error);
}


/* -- oauth_service_ask_authorization  -- */


static void
get_access_token_ready_cb (GObject      *source_object,
			   GAsyncResult *result,
			   gpointer      user_data)
{
	OAuthService *self = user_data;
	GError       *error = NULL;
	GtkWidget    *dialog;
	OAuthAccount *account;

	dialog = _web_service_get_auth_dialog (WEB_SERVICE (self));
	account = _oauth_service_get_access_token_finish (self, result, &error);
	if (account == NULL) {
		gtk_dialog_response (GTK_DIALOG (dialog), GTK_RESPONSE_CANCEL);
		gth_task_completed (GTH_TASK (self), error);
		return;
	}

	web_service_set_current_account (WEB_SERVICE (self), account);
	gtk_dialog_response (GTK_DIALOG (dialog), GTK_RESPONSE_OK);

	g_object_unref (account);
}


static void
ask_authorization_dialog_load_request_cb (OAuthAskAuthorizationDialog *dialog,
					  gpointer                     user_data)
{
	OAuthService *self = user_data;
	const char   *uri;

	uri = oauth_ask_authorization_dialog_get_uri (dialog);
	if (uri == NULL)
		return;

	if (g_str_has_prefix (uri, OAUTH_CALLBACK)) {
		const char *uri_data;
		GHashTable *data;
		gboolean    success = FALSE;

		uri_data = uri + strlen (OAUTH_CALLBACK "?");

		data = soup_form_decode (uri_data);
		_g_str_set (&self->priv->token, g_hash_table_lookup (data, "oauth_token"));

		if (self->priv->token != NULL) {
			gtk_widget_hide (GTK_WIDGET (dialog));
			gth_task_dialog (GTH_TASK (self), FALSE, NULL);

			success = TRUE;
			_oauth_service_get_access_token (self,
							 g_hash_table_lookup (data, "oauth_verifier"),
							 gth_task_get_cancellable (GTH_TASK (self)),
							 get_access_token_ready_cb,
							 self);
		}

		if (! success)
			gtk_dialog_response (GTK_DIALOG (dialog), GTK_RESPONSE_CANCEL);

		g_hash_table_destroy (data);
	}
}


static void
get_request_token_ready_cb (GObject      *source_object,
	        	    GAsyncResult *result,
	        	    gpointer      user_data)
{
	OAuthService *self = user_data;
	GError       *error = NULL;
	char         *url;
	GtkWidget    *dialog;

	if (! oauth_service_get_request_token_finish (self, result, &error)) {
		gth_task_completed (GTH_TASK (self), error);
		return;
	}

	url = self->priv->consumer->get_authorization_url (self);
	dialog = oauth_ask_authorization_dialog_new (url);
	_gtk_window_resize_to_fit_screen_height (dialog, 1024);
	_web_service_set_auth_dialog (WEB_SERVICE (self), GTK_DIALOG (dialog));
	g_signal_connect (OAUTH_ASK_AUTHORIZATION_DIALOG (dialog),
			  "load-request",
			  G_CALLBACK (ask_authorization_dialog_load_request_cb),
			  self);
	gtk_widget_show (dialog);

	g_free (url);
}


static void
oauth_service_ask_authorization (WebService *base)
{
	OAuthService *self = OAUTH_SERVICE (base);

	oauth_service_set_token (self, NULL);
	oauth_service_set_token_secret (self, NULL);
	_oauth_service_get_request_token (self,
					  gth_task_get_cancellable (GTH_TASK (self)),
					  get_request_token_ready_cb,
					  self);
}


static void
oauth_service_class_init (OAuthServiceClass *klass)
{
	GObjectClass    *object_class;
	WebServiceClass *service_class;

	object_class = (GObjectClass*) klass;
	object_class->set_property = oauth_service_set_property;
	object_class->get_property = oauth_service_get_property;
	object_class->finalize = oauth_service_finalize;

	service_class = (WebServiceClass*) klass;
	service_class->ask_authorization = oauth_service_ask_authorization;

	/* properties */

	g_object_class_install_property (object_class,
					 PROP_CONSUMER,
					 g_param_spec_pointer ("consumer",
                                                               "Consumer",
                                                               "",
                                                               G_PARAM_READWRITE));
}


static void
oauth_service_init (OAuthService *self)
{
	self->priv = oauth_service_get_instance_private (self);
	self->priv->consumer = NULL;
	self->priv->timestamp = NULL;
	self->priv->nonce = NULL;
	self->priv->signature = NULL;
	self->priv->token = NULL;
	self->priv->token_secret = NULL;
	self->priv->verifier = NULL;
}


/* -- oauth_service_add_signature -- */


static char *
oauth_create_timestamp (GTimeVal *t)
{
	return g_strdup_printf ("%ld", t->tv_sec);
}


static char *
oauth_create_nonce (GTimeVal *t)
{
	char *s;
	char *v;

	s = g_strdup_printf ("%ld%u", t->tv_usec, g_random_int ());
	v = g_compute_checksum_for_string (G_CHECKSUM_MD5, s, -1);

	g_free (s);

	return v;
}


void
oauth_service_add_signature (OAuthService *self,
			     const char   *method,
			     const char   *url,
			     GHashTable   *parameters)
{
	GTimeVal  t;
	GString  *param_string;
	GList    *keys;
	GList    *scan;
	GString  *base_string;
	GString  *signature_key;

	/* Add the OAuth specific parameters */

	g_get_current_time (&t);

	g_free (self->priv->timestamp);
	self->priv->timestamp = oauth_create_timestamp (&t);
	g_hash_table_insert (parameters, "oauth_timestamp", self->priv->timestamp);

	g_free (self->priv->nonce);
	self->priv->nonce = oauth_create_nonce (&t);
	g_hash_table_insert (parameters, "oauth_nonce", self->priv->nonce);
	g_hash_table_insert (parameters, "oauth_version", OAUTH_VERSION);
	g_hash_table_insert (parameters, "oauth_signature_method", OAUTH_SIGNATURE_METHOD);
	g_hash_table_insert (parameters, "oauth_consumer_key", (gpointer) self->priv->consumer->consumer_key);
	if (self->priv->token != NULL)
		g_hash_table_insert (parameters, "oauth_token", self->priv->token);

	/* Create the parameter string */

	param_string = g_string_new ("");
	keys = g_hash_table_get_keys (parameters);
	keys = g_list_sort (keys, (GCompareFunc) strcmp);
	for (scan = keys; scan; scan = scan->next) {
		char *key = scan->data;
		char *value = g_hash_table_lookup (parameters, key);

		g_string_append_uri_escaped (param_string, key, NULL, FALSE);
		g_string_append (param_string, "=");
		g_string_append_uri_escaped (param_string, value, NULL, FALSE);
		if (scan->next != NULL)
			g_string_append (param_string, "&");
	}

	/* Create the Base String */

	base_string = g_string_new ("");
	g_string_append_uri_escaped (base_string, method, NULL, FALSE);
	g_string_append (base_string, "&");
	g_string_append_uri_escaped (base_string, url, NULL, FALSE);
	g_string_append (base_string, "&");
	g_string_append_uri_escaped (base_string, param_string->str, NULL, FALSE);

	/* Calculate the signature value */

	signature_key = g_string_new ("");
	g_string_append_uri_escaped (signature_key, self->priv->consumer->consumer_secret, NULL, FALSE);
	g_string_append (signature_key, "&");
	if (self->priv->token_secret != NULL)
		g_string_append_uri_escaped (signature_key, self->priv->token_secret, NULL, FALSE);
	g_free (self->priv->signature);
	self->priv->signature = g_compute_signature_for_string (G_CHECKSUM_SHA1,
								G_SIGNATURE_ENC_BASE64,
							        signature_key->str,
							        signature_key->len,
							        base_string->str,
							        base_string->len);
	g_hash_table_insert (parameters, "oauth_signature", self->priv->signature);

	g_string_free (signature_key, TRUE);
	g_string_free (base_string, TRUE);
	g_list_free (keys);
	g_string_free (param_string, TRUE);
}


void
oauth_service_set_token (OAuthService *self,
			 const char   *token)
{
	_g_str_set (&self->priv->token, token);
}


const char *
oauth_service_get_token (OAuthService *self)
{
	return self->priv->token;
}


void
oauth_service_set_token_secret (OAuthService *self,
				const char   *token_secret)
{
	_g_str_set (&self->priv->token_secret, token_secret);
}


const char *
oauth_service_get_token_secret (OAuthService *self)
{
	return self->priv->token_secret;
}
