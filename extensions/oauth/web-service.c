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
#ifdef HAVE_LIBSOUP_GNOME
#include <libsoup/soup-gnome.h>
#else
#include <libsoup/soup.h>
#endif /* HAVE_LIBSOUP_GNOME */
#ifdef HAVE_LIBSECRET
#include <libsecret/secret.h>
#endif /* HAVE_LIBSECRET */
#include "oauth-account-manager-dialog.h"
#include "oauth-account-chooser-dialog.h"
#include "web-service.h"


#undef  DEBUG_WEB_CONNECTION
#define WEB_AUTHENTICATION_RESPONSE_CHOOSE_ACCOUNT 2


GQuark
web_service_error_quark (void)
{
	static GQuark quark;

        if (! quark)
                quark = g_quark_from_static_string ("web-service-error");

        return quark;
}


G_DEFINE_TYPE (WebService, web_service, GTH_TYPE_TASK)


enum {
        PROP_0,
        PROP_SERVICE_NAME,
        PROP_SERVICE_ADDRESS,
        PROP_SERVICE_PROTOCOL,
        PROP_ACCOUNT_TYPE,
	PROP_CANCELLABLE,
	PROP_BROWSER,
	PROP_DIALOG
};


/* Signals */
enum {
	ACCOUNT_READY,
	ACCOUNTS_CHANGED,
	LAST_SIGNAL
};


static guint web_service_signals[LAST_SIGNAL] = { 0 };


struct _WebServicePrivate
{
	char               *service_name;
	char               *service_address;
	char               *service_protocol;
	GType               account_type;
	SoupSession        *session;
	SoupMessage        *msg;
	GCancellable       *cancellable;
	GSimpleAsyncResult *result;
	GList              *accounts;
	OAuthAccount       *account;
	GtkWidget          *browser;
	GtkWidget          *dialog;
	GtkWidget          *auth_dialog;
};


static void
web_service_finalize (GObject *object)
{
	WebService *self = WEB_SERVICE (object);

	_g_object_unref (self->priv->account);
	_g_object_list_unref (self->priv->accounts);
	_g_object_unref (self->priv->result);
	_g_object_unref (self->priv->cancellable);
	_g_object_unref (self->priv->session);
	g_free (self->priv->service_protocol);
	g_free (self->priv->service_address);
	g_free (self->priv->service_name);

	G_OBJECT_CLASS (web_service_parent_class)->finalize (object);
}


static void
web_service_set_property (GObject      *object,
			  guint         property_id,
			  const GValue *value,
			  GParamSpec   *pspec)
{
	WebService *self = WEB_SERVICE (object);

	switch (property_id) {
	case PROP_SERVICE_NAME:
		_g_strset (&self->priv->service_name, g_value_get_string (value));
		break;
	case PROP_SERVICE_ADDRESS:
		_g_strset (&self->priv->service_address, g_value_get_string (value));
		break;
	case PROP_SERVICE_PROTOCOL:
		_g_strset (&self->priv->service_protocol, g_value_get_string (value));
		break;
	case PROP_ACCOUNT_TYPE:
		self->priv->account_type = g_value_get_gtype (value);
		break;
	case PROP_CANCELLABLE:
		_g_object_unref (self->priv->cancellable);
		self->priv->cancellable = g_value_dup_object (value);
		break;
	case PROP_BROWSER:
		self->priv->browser = g_value_get_pointer (value);
		break;
	case PROP_DIALOG:
		self->priv->dialog = g_value_get_pointer (value);
		break;
	default:
		break;
	}
}


static void
web_service_get_property (GObject    *object,
			  guint       property_id,
			  GValue     *value,
			  GParamSpec *pspec)
{
	WebService *self = WEB_SERVICE (object);

	switch (property_id) {
	case PROP_SERVICE_NAME:
		g_value_set_string (value, self->priv->service_name);
		break;
	case PROP_SERVICE_ADDRESS:
		g_value_set_string (value, self->priv->service_address);
		break;
	case PROP_SERVICE_PROTOCOL:
		g_value_set_string (value, self->priv->service_protocol);
		break;
	case PROP_ACCOUNT_TYPE:
		g_value_set_gtype (value, self->priv->account_type);
		break;
	case PROP_CANCELLABLE:
		g_value_set_object (value, self->priv->cancellable);
		break;
	case PROP_BROWSER:
		g_value_set_pointer (value, self->priv->browser);
		break;
	case PROP_DIALOG:
		g_value_set_pointer (value, self->priv->dialog);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
		break;
	}
}


static void
web_service_constructed (GObject *object)
{
	WebService *self = WEB_SERVICE (object);

	self->priv->accounts = oauth_accounts_load_from_file (self->priv->service_name, self->priv->account_type);
	self->priv->account = oauth_accounts_find_default (self->priv->accounts);

	if (G_OBJECT_CLASS (web_service_parent_class)->constructed != NULL)
		G_OBJECT_CLASS (web_service_parent_class)->constructed (object);
}


static void
web_service_exec (GthTask *base)
{
	/* void */
}


static void
web_service_cancelled (GthTask *base)
{
	WebService *self = WEB_SERVICE (base);

	if ((self->priv->session == NULL) || (self->priv->msg == NULL))
		return;

	soup_session_cancel_message (self->priv->session,
				     self->priv->msg,
				     SOUP_STATUS_CANCELLED);
}


static void
web_service_class_init (WebServiceClass *klass)
{
	GObjectClass *object_class;
	GthTaskClass *task_class;

	g_type_class_add_private (klass, sizeof (WebServicePrivate));

	object_class = (GObjectClass*) klass;
	object_class->finalize = web_service_finalize;
	object_class->set_property = web_service_set_property;
	object_class->get_property = web_service_get_property;
	object_class->constructed = web_service_constructed;

	task_class = (GthTaskClass*) klass;
	task_class->exec = web_service_exec;
	task_class->cancelled = web_service_cancelled;

	/* properties */

	g_object_class_install_property (object_class,
					 PROP_SERVICE_NAME,
					 g_param_spec_string ("service-name",
                                                              "Service Name",
                                                              "",
                                                              NULL,
                                                              G_PARAM_READWRITE | G_PARAM_CONSTRUCT));
	g_object_class_install_property (object_class,
					 PROP_SERVICE_ADDRESS,
					 g_param_spec_string ("service-address",
                                                              "Service Address",
                                                              "",
                                                              NULL,
                                                              G_PARAM_READWRITE | G_PARAM_CONSTRUCT));
	g_object_class_install_property (object_class,
					 PROP_SERVICE_PROTOCOL,
					 g_param_spec_string ("service-protocol",
                                                              "Service Protocol",
                                                              "",
                                                              NULL,
                                                              G_PARAM_READWRITE | G_PARAM_CONSTRUCT));
	g_object_class_install_property (object_class,
					 PROP_ACCOUNT_TYPE,
					 g_param_spec_gtype ("account-type",
                                                             "Account type",
                                                             "",
                                                             OAUTH_TYPE_ACCOUNT,
                                                             G_PARAM_READWRITE | G_PARAM_CONSTRUCT));
	g_object_class_install_property (object_class,
					 PROP_CANCELLABLE,
					 g_param_spec_object ("cancellable",
                                                              "Cancellable",
                                                              "",
                                                              G_TYPE_CANCELLABLE,
                                                              G_PARAM_READWRITE));
	g_object_class_install_property (object_class,
					 PROP_BROWSER,
					 g_param_spec_pointer ("browser",
                                                               "Browser",
                                                               "",
                                                               G_PARAM_READWRITE));
	g_object_class_install_property (object_class,
					 PROP_DIALOG,
					 g_param_spec_pointer ("dialog",
                                                               "Dialog",
                                                               "",
                                                               G_PARAM_READWRITE));

	/* signals */

	web_service_signals[ACCOUNT_READY] =
		g_signal_new ("account-ready",
			      G_TYPE_FROM_CLASS (klass),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (WebServiceClass, account_ready),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__VOID,
			      G_TYPE_NONE,
			      0);
	web_service_signals[ACCOUNTS_CHANGED] =
		g_signal_new ("accounts-changed",
			      G_TYPE_FROM_CLASS (klass),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (WebServiceClass, accounts_changed),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__VOID,
			      G_TYPE_NONE,
			      0);
}


static void
web_service_init (WebService *self)
{
	self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, WEB_TYPE_SERVICE, WebServicePrivate);
	self->priv->service_name = NULL;
	self->priv->service_address = NULL;
	self->priv->service_protocol = NULL;
	self->priv->account_type = OAUTH_TYPE_ACCOUNT;
	self->priv->session = NULL;
	self->priv->msg = NULL;
	self->priv->cancellable = NULL;
	self->priv->result = NULL;
	self->priv->accounts = NULL;
	self->priv->account = NULL;
	self->priv->browser = NULL;
	self->priv->dialog = NULL;
	self->priv->auth_dialog = NULL;
}


/* -- authentication error dialog -- */


static void show_choose_account_dialog (WebService *self);


static void
authentication_error_dialog_response_cb (GtkDialog *dialog,
					 int        response_id,
					 gpointer   user_data)
{
	WebService *self = user_data;

	switch (response_id) {
	case GTK_RESPONSE_DELETE_EVENT:
	case GTK_RESPONSE_CANCEL:
		gtk_widget_destroy (GTK_WIDGET (dialog));
		gtk_dialog_response (GTK_DIALOG (self->priv->dialog), GTK_RESPONSE_DELETE_EVENT);
		break;

	case WEB_AUTHENTICATION_RESPONSE_CHOOSE_ACCOUNT:
		gtk_widget_destroy (GTK_WIDGET (dialog));
		show_choose_account_dialog (self);
		break;

	default:
		break;
	}
}


static void
show_authentication_error_dialog (WebService  *self,
				  GError     **error)
{
	GtkWidget *dialog;

	if (g_error_matches (*error, WEB_SERVICE_ERROR, WEB_SERVICE_ERROR_TOKEN_EXPIRED)) {
		web_service_ask_authorization (self);
		return;
	}

	dialog = _gtk_message_dialog_new (GTK_WINDOW (self->priv->browser),
					  GTK_DIALOG_MODAL,
					  GTK_STOCK_DIALOG_ERROR,
					  _("Could not connect to the server"),
					  (*error)->message,
					  _("Choose _Account..."), WEB_AUTHENTICATION_RESPONSE_CHOOSE_ACCOUNT,
					  GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
					  NULL);
	gth_task_dialog (GTH_TASK (self), TRUE, dialog);

	g_signal_connect (dialog,
			  "delete-event",
			  G_CALLBACK (gtk_true),
			  NULL);
	g_signal_connect (dialog,
			  "response",
			  G_CALLBACK (authentication_error_dialog_response_cb),
			  self);
	gtk_widget_show (dialog);

	g_clear_error (error);
}


/* -- web_service_autoconnect -- */


static void
set_current_account (WebService   *self,
		     OAuthAccount *account)
{
	GList *link;

	if (self->priv->account == account)
		return;

	link = g_list_find_custom (self->priv->accounts, account, (GCompareFunc) oauth_account_cmp);
	if (link != NULL) {
		self->priv->accounts = g_list_remove_link (self->priv->accounts, link);
		_g_object_list_unref (link);
	}

	_g_object_unref (self->priv->account);
	self->priv->account = NULL;

	if (account != NULL) {
		self->priv->account = g_object_ref (account);
		self->priv->accounts = g_list_prepend (self->priv->accounts, g_object_ref (self->priv->account));
	}
}


#ifdef HAVE_LIBSECRET


static char *
serialize_secret (const char *token,
		  const char *token_secret)
{
	GVariantBuilder *builder;
	GVariant        *variant;
	char            *secret;

	builder = g_variant_builder_new (G_VARIANT_TYPE_ARRAY);
	g_variant_builder_add (builder, "ms", token);
	g_variant_builder_add (builder, "ms", token_secret);
	variant = g_variant_builder_end (builder);
	secret = g_variant_print (variant, TRUE);

	g_variant_unref (variant);

	return secret;
}


static gboolean
deserialize_secret (const char  *secret,
		    char       **token,
		    char       **token_secret)
{
	GVariant *variant;

	variant = g_variant_parse (NULL, secret, NULL, NULL, NULL);
	if (variant == NULL)
		return FALSE;

	if (token != NULL)
		g_variant_get_child (variant, 0, "ms", token, NULL);
	if (token_secret != NULL)
		g_variant_get_child (variant, 1, "ms", token_secret, NULL);

	g_variant_unref (variant);

	return TRUE;
}


static void
password_store_ready_cb (GObject      *source_object,
			 GAsyncResult *result,
			 gpointer      user_data)
{
	WebService *self = user_data;

	secret_password_store_finish (result, NULL);
	web_service_account_ready (self);
}


#endif


static void
get_user_info_ready_cb (GObject      *source_object,
			GAsyncResult *res,
			gpointer      user_data)
{
	WebService   *self = user_data;
	GError       *error = NULL;
	OAuthAccount *account;

	account = web_service_get_user_info_finish (self, res, &error);
	if (account == NULL) {
		show_authentication_error_dialog (self, &error);
		return;
	}

	set_current_account (self, account);
	oauth_accounts_save_to_file (self->priv->service_name,
				     self->priv->accounts,
				     self->priv->account);

#ifdef HAVE_LIBSECRET
	{
		char *secret;

		secret = serialize_secret (account->token, account->token_secret);
		secret_password_store (SECRET_SCHEMA_COMPAT_NETWORK,
				       NULL,
				       self->priv->service_name,
				       secret,
				       self->priv->cancellable,
				       password_store_ready_cb,
				       self,
				       "user", account->id,
				       "server", self->priv->service_address,
				       "protocol", self->priv->service_protocol,
				       NULL);

		g_free (secret);
	}
#else
	web_service_account_ready (self);
#endif

	g_object_unref (account);
}


static void
connect_to_server_step2 (WebService *self)
{
	if ((self->priv->account->token == NULL) && (self->priv->account->token_secret == NULL)) {
		web_service_ask_authorization (self);
		return;
	}
	web_service_get_user_info (self,
				   self->priv->cancellable,
				   get_user_info_ready_cb,
				   self);
}


#ifdef HAVE_LIBSECRET


static void
password_lookup_ready_cb (GObject      *source_object,
			  GAsyncResult *result,
			  gpointer      user_data)
{
	WebService *self = user_data;
	char       *secret;

	secret = secret_password_lookup_finish (result, NULL);
	if (secret != NULL) {
		char *token;
		char *token_secret;

		if (deserialize_secret (secret, &token, &token_secret)) {
			g_object_set (G_OBJECT (self->priv->account),
				      "token", token,
				      "token-secret", token_secret,
				      NULL);

			g_free (token);
			g_free (token_secret);
		}

		g_free (secret);
	}

	connect_to_server_step2 (self);
}


#endif


static void
connect_to_server (WebService *self)
{
	g_return_if_fail (self->priv->account != NULL);
	g_return_if_fail (self->priv->account->id != NULL);

#ifdef HAVE_LIBSECRET
	if (self->priv->account->token_secret == NULL) {
		secret_password_lookup (SECRET_SCHEMA_COMPAT_NETWORK,
					self->priv->cancellable,
					password_lookup_ready_cb,
					self,
					"user", self->priv->account->id,
					"server", self->priv->service_address,
					"protocol", self->priv->service_protocol,
					NULL);
		return;
	}
#endif

	connect_to_server_step2 (self);
}


static void
account_chooser_dialog_response_cb (GtkDialog *dialog,
				    int        response_id,
				    gpointer   user_data)
{
	WebService *self = user_data;

	switch (response_id) {
	case GTK_RESPONSE_DELETE_EVENT:
	case GTK_RESPONSE_CANCEL:
		gtk_widget_destroy (GTK_WIDGET (dialog));
		gtk_dialog_response (GTK_DIALOG (self->priv->dialog), GTK_RESPONSE_DELETE_EVENT);
		break;

	case GTK_RESPONSE_OK:
		_g_object_unref (self->priv->account);
		self->priv->account = oauth_account_chooser_dialog_get_active (OAUTH_ACCOUNT_CHOOSER_DIALOG (dialog));
		if (self->priv->account != NULL) {
			gtk_widget_destroy (GTK_WIDGET (dialog));
			connect_to_server (self);
		}
		break;

	case OAUTH_ACCOUNT_CHOOSER_RESPONSE_NEW:
		gtk_widget_destroy (GTK_WIDGET (dialog));
		web_service_ask_authorization (self);
		break;

	default:
		break;
	}
}


static void
show_choose_account_dialog (WebService *self)
{
	GtkWidget *dialog;

	gth_task_dialog (GTH_TASK (self), TRUE, NULL);
	dialog = oauth_account_chooser_dialog_new (self->priv->accounts, self->priv->account);
	g_signal_connect (dialog,
			  "delete-event",
			  G_CALLBACK (gtk_true),
			  NULL);
	g_signal_connect (dialog,
			  "response",
			  G_CALLBACK (account_chooser_dialog_response_cb),
			  self);

	gtk_window_set_title (GTK_WINDOW (dialog), _("Choose Account"));
	gtk_window_set_transient_for (GTK_WINDOW (dialog), GTK_WINDOW (self->priv->browser));
	gtk_window_set_modal (GTK_WINDOW (dialog), TRUE);
	gtk_window_present (GTK_WINDOW (dialog));
}


void
web_service_autoconnect (WebService *self)
{
	gtk_widget_hide (self->priv->dialog);
	gth_task_dialog (GTH_TASK (self), FALSE, NULL);

	if (self->priv->accounts != NULL) {
		if (self->priv->account != NULL) {
			connect_to_server (self);
		}
		else if (self->priv->accounts->next == NULL) {
			self->priv->account = g_object_ref (self->priv->accounts->data);
			connect_to_server (self);
		}
		else
			show_choose_account_dialog (self);
	}
	else
		web_service_ask_authorization (self);
}


void
web_service_connect (WebService   *self,
		     OAuthAccount *account)
{
	set_current_account (self, account);
	web_service_autoconnect (self);
}


void
web_service_set_current_account (WebService   *self,
				 OAuthAccount *account)
{
	set_current_account (self, account);
}


OAuthAccount *
web_service_get_current_account (WebService *self)
{
	return self->priv->account;
}


GList *
web_service_get_accounts (WebService *self)
{
	return self->priv->accounts;
}


/* -- web_service_edit_accounts -- */


static void
account_manager_dialog_response_cb (GtkDialog *dialog,
			            int        response_id,
			            gpointer   user_data)
{
	WebService *self = user_data;

	switch (response_id) {
	case GTK_RESPONSE_DELETE_EVENT:
	case GTK_RESPONSE_CANCEL:
		gtk_widget_destroy (GTK_WIDGET (dialog));
		break;

	case GTK_RESPONSE_OK:
		_g_object_list_unref (self->priv->accounts);
		self->priv->accounts = oauth_account_manager_dialog_get_accounts (OAUTH_ACCOUNT_MANAGER_DIALOG (dialog));
		if (! g_list_find_custom (self->priv->accounts, self->priv->account, (GCompareFunc) oauth_account_cmp)) {
			_g_object_unref (self->priv->account);
			self->priv->account = NULL;
			web_service_autoconnect (self);
		}
		else
			g_signal_emit (self, web_service_signals[ACCOUNTS_CHANGED], 0);
		oauth_accounts_save_to_file (self->priv->service_name, self->priv->accounts, self->priv->account);
		gtk_widget_destroy (GTK_WIDGET (dialog));
		break;

	case OAUTH_ACCOUNT_CHOOSER_RESPONSE_NEW:
		gtk_widget_destroy (GTK_WIDGET (dialog));
		web_service_ask_authorization (self);
		break;

	default:
		break;
	}
}


void
web_service_edit_accounts (WebService *self,
			   GtkWindow  *parent)
{
	GtkWidget *dialog;

	dialog = oauth_account_manager_dialog_new (self->priv->accounts);
	g_signal_connect (dialog,
			  "delete-event",
			  G_CALLBACK (gtk_true),
			  NULL);
	g_signal_connect (dialog,
			  "response",
			  G_CALLBACK (account_manager_dialog_response_cb),
			  self);

	gtk_window_set_title (GTK_WINDOW (dialog), _("Edit Accounts"));
	gtk_window_set_transient_for (GTK_WINDOW (dialog), GTK_WINDOW (self->priv->dialog));
	gtk_window_set_modal (GTK_WINDOW (dialog), TRUE);
	gtk_window_present (GTK_WINDOW (dialog));
}


void
web_service_account_ready (WebService *self)
{
	g_signal_emit (self, web_service_signals[ACCOUNT_READY], 0);
}


void
web_service_ask_authorization (WebService *self)
{
	gth_task_progress (GTH_TASK (self),
			   _("Connecting to the server"),
			   _("Asking authorization"),
			   TRUE,
			   0.0);

	web_service_set_current_account (self, NULL);
	WEB_SERVICE_GET_CLASS (self)->ask_authorization (self);
}


void
web_service_get_user_info (WebService		 *self,
			   GCancellable		 *cancellable,
			   GAsyncReadyCallback	  callback,
			   gpointer		  user_data)
{
	gth_task_progress (GTH_TASK (self),
			   _("Connecting to the server"),
			   _("Getting account information"),
			   TRUE,
			   0.0);

	WEB_SERVICE_GET_CLASS (self)->get_user_info (self, cancellable, callback, user_data);
}


OAuthAccount *
web_service_get_user_info_finish (WebService	   *self,
				  GAsyncResult	   *result,
				  GError	  **error)
{
	if (g_simple_async_result_propagate_error (G_SIMPLE_ASYNC_RESULT (result), error))
		return NULL;
	else
		return g_object_ref (g_simple_async_result_get_op_res_gpointer (G_SIMPLE_ASYNC_RESULT (result)));
}


/* -- connection utilities -- */


void
_web_service_send_message (WebService          *self,
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

#ifdef DEBUG_WEB_CONNECTION
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
	self->priv->result = g_simple_async_result_new (G_OBJECT (self),
							callback,
							user_data,
							source_tag);

	self->priv->msg = msg;
	g_object_add_weak_pointer (G_OBJECT (msg), (gpointer *) &self->priv->msg);

	soup_session_queue_message (self->priv->session,
				    msg,
				    soup_session_cb,
				    soup_session_cb_data);
}


GSimpleAsyncResult *
_web_service_get_result (WebService *self)
{
	return self->priv->result;
}


void
_web_service_reset_result (WebService *self)
{
	self->priv->result = NULL;
}


SoupMessage *
_web_service_get_message (WebService *self)
{
	return self->priv->msg;
}


/* -- _web_service_set_auth_dialog -- */


static void
ask_authorization_dialog_response_cb (GtkDialog *dialog,
				      int        response_id,
				      gpointer   user_data)
{
	WebService *self = user_data;

	switch (response_id) {
	case GTK_RESPONSE_DELETE_EVENT:
	case GTK_RESPONSE_CANCEL:
		gtk_widget_destroy (GTK_WIDGET (dialog));
		gtk_dialog_response (GTK_DIALOG (self->priv->dialog), GTK_RESPONSE_DELETE_EVENT);
		break;

	case GTK_RESPONSE_OK:
		gtk_widget_destroy (GTK_WIDGET (dialog));
		gth_task_dialog (GTH_TASK (self), FALSE, NULL);
		web_service_get_user_info (self,
					   self->priv->cancellable,
					   get_user_info_ready_cb,
					   self);
		break;

	default:
		break;
	}
}


void
_web_service_set_auth_dialog (WebService *self,
			      GtkDialog  *dialog)
{
	self->priv->auth_dialog = GTK_WIDGET (dialog);
	g_object_add_weak_pointer (G_OBJECT (dialog), (gpointer *) &self->priv->auth_dialog);
	gth_task_dialog (GTH_TASK (self), TRUE, self->priv->auth_dialog);

	gtk_window_set_modal (GTK_WINDOW (dialog), TRUE);
	if (gtk_widget_get_visible (self->priv->dialog))
		gtk_window_set_transient_for (GTK_WINDOW (dialog), GTK_WINDOW (self->priv->dialog));
	else
		gtk_window_set_transient_for (GTK_WINDOW (dialog), GTK_WINDOW (self->priv->browser));

	g_signal_connect (dialog,
			  "delete-event",
			  G_CALLBACK (gtk_true),
			  NULL);
	g_signal_connect (dialog,
			  "response",
			  G_CALLBACK (ask_authorization_dialog_response_cb),
			  self);
}


GtkWidget *
_web_service_get_auth_dialog (WebService *self)
{
	return self->priv->auth_dialog;
}
