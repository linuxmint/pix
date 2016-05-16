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

#ifndef OAUTH_ASK_AUTHORIZATION_DIALOG_H
#define OAUTH_ASK_AUTHORIZATION_DIALOG_H

#include <gtk/gtk.h>
#include <gthumb.h>

G_BEGIN_DECLS

#define OAUTH_TYPE_ASK_AUTHORIZATION_DIALOG            (oauth_ask_authorization_dialog_get_type ())
#define OAUTH_ASK_AUTHORIZATION_DIALOG(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), OAUTH_TYPE_ASK_AUTHORIZATION_DIALOG, OAuthAskAuthorizationDialog))
#define OAUTH_ASK_AUTHORIZATION_DIALOG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), OAUTH_TYPE_ASK_AUTHORIZATION_DIALOG, OAuthAskAuthorizationDialogClass))
#define OAUTH_IS_ASK_AUTHORIZATION_DIALOG(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), OAUTH_TYPE_ASK_AUTHORIZATION_DIALOG))
#define OAUTH_IS_ASK_AUTHORIZATION_DIALOG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), OAUTH_TYPE_ASK_AUTHORIZATION_DIALOG))
#define OAUTH_ASK_AUTHORIZATION_DIALOG_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), OAUTH_TYPE_ASK_AUTHORIZATION_DIALOG, OAuthAskAuthorizationDialogClass))

typedef struct _OAuthAskAuthorizationDialog OAuthAskAuthorizationDialog;
typedef struct _OAuthAskAuthorizationDialogClass OAuthAskAuthorizationDialogClass;
typedef struct _OAuthAskAuthorizationDialogPrivate OAuthAskAuthorizationDialogPrivate;

struct _OAuthAskAuthorizationDialog {
	GtkDialog parent_instance;
	OAuthAskAuthorizationDialogPrivate *priv;
};

struct _OAuthAskAuthorizationDialogClass {
	GtkDialogClass parent_class;

	/*< signals >*/

	void  (*load_request)	(OAuthAskAuthorizationDialog *self);
	void  (*loaded)		(OAuthAskAuthorizationDialog *self);
	void  (*redirected)	(OAuthAskAuthorizationDialog *self);
};

GType          oauth_ask_authorization_dialog_get_type     (void);
GtkWidget *    oauth_ask_authorization_dialog_new          (const char                  *url);
GtkWidget *    oauth_ask_authorization_dialog_get_view     (OAuthAskAuthorizationDialog *self);
const char *   oauth_ask_authorization_dialog_get_uri      (OAuthAskAuthorizationDialog *self);
const char *   oauth_ask_authorization_dialog_get_title    (OAuthAskAuthorizationDialog *self);

G_END_DECLS

#endif /* OAUTH_ASK_AUTHORIZATION_DIALOG_H */
