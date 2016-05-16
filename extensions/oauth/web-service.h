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

#ifndef WEB_SERVICE_H
#define WEB_SERVICE_H

#include <gtk/gtk.h>
#include <gthumb.h>
#include "oauth-account.h"

#define WEB_SERVICE_ERROR web_service_error_quark()
GQuark web_service_error_quark (void);

typedef enum {
	WEB_SERVICE_ERROR_GENERIC,
	WEB_SERVICE_ERROR_TOKEN_EXPIRED
} WebSericeError;

typedef enum {
	WEB_AUTHORIZATION_READ,
	WEB_AUTHORIZATION_WRITE
} WebAuthorization;

#define WEB_TYPE_SERVICE         (web_service_get_type ())
#define WEB_SERVICE(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), WEB_TYPE_SERVICE, WebService))
#define WEB_SERVICE_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST ((k), WEB_TYPE_SERVICE, WebServiceClass))
#define WEB_IS_SERVICE(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), WEB_TYPE_SERVICE))
#define WEB_IS_SERVICE_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), WEB_TYPE_SERVICE))
#define WEB_SERVICE_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS((o), WEB_TYPE_SERVICE, WebServiceClass))

typedef struct _WebService         WebService;
typedef struct _WebServicePrivate  WebServicePrivate;
typedef struct _WebServiceClass    WebServiceClass;

struct _WebService {
	GthTask __parent;
	WebServicePrivate *priv;
};

struct _WebServiceClass {
	GthTaskClass __parent_class;

	/*< signals >*/

	void  (*account_ready)		(WebService		 *self);
	void  (*accounts_changed)	(WebService		 *self);

	/*< virtual functions >*/

	void  (*ask_authorization)	(WebService		 *self);
	void  (*get_user_info)		(WebService		 *self,
					 GCancellable		 *cancellable,
					 GAsyncReadyCallback	  callback,
					 gpointer		  user_data);
};

GType           web_service_get_type		(void) G_GNUC_CONST;
void            web_service_autoconnect		(WebService		 *self);
void            web_service_connect		(WebService		 *self,
						 OAuthAccount		 *account);
void		web_service_set_current_account	(WebService		 *self,
						 OAuthAccount            *account);
OAuthAccount *  web_service_get_current_account	(WebService		 *self);
GList *         web_service_get_accounts	(WebService		 *self);
void            web_service_edit_accounts	(WebService		 *self,
						 GtkWindow		 *parent);
void            web_service_account_ready	(WebService		 *self);
void            web_service_ask_authorization	(WebService		 *self);
void            web_service_get_user_info       (WebService		 *self,
						 GCancellable		 *cancellable,
						 GAsyncReadyCallback	  callback,
						 gpointer		  user_data);
OAuthAccount *  web_service_get_user_info_finish(WebService		 *self,
						 GAsyncResult		 *result,
						 GError			**error);

/* -- utilities -- */

void            _web_service_send_message	(WebService		 *self,
						 SoupMessage		 *msg,
						 GCancellable		 *cancellable,
						 GAsyncReadyCallback	  callback,
						 gpointer		  user_data,
						 gpointer		  source_tag,
						 SoupSessionCallback	  soup_session_cb,
						 gpointer		  soup_session_cb_data);
GSimpleAsyncResult *
		_web_service_get_result		(WebService		 *self);
void            _web_service_reset_result       (WebService		 *self);
SoupMessage *	_web_service_get_message	(WebService		 *self);
void            _web_service_set_auth_dialog	(WebService		 *self,
						 GtkDialog               *dialog);
GtkWidget *     _web_service_get_auth_dialog    (WebService		 *self);

#endif /* WEB_SERVICE_H */

