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

#ifndef OAUTH_ACCOUNT_CHOOSER_DIALOG_H
#define OAUTH_ACCOUNT_CHOOSER_DIALOG_H

#include <gtk/gtk.h>
#include <gthumb.h>
#include "oauth-account.h"

G_BEGIN_DECLS

#define OAUTH_ACCOUNT_CHOOSER_RESPONSE_NEW 1

#define OAUTH_TYPE_ACCOUNT_CHOOSER_DIALOG            (oauth_account_chooser_dialog_get_type ())
#define OAUTH_ACCOUNT_CHOOSER_DIALOG(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), OAUTH_TYPE_ACCOUNT_CHOOSER_DIALOG, OAuthAccountChooserDialog))
#define OAUTH_ACCOUNT_CHOOSER_DIALOG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), OAUTH_TYPE_ACCOUNT_CHOOSER_DIALOG, OAuthAccountChooserDialogClass))
#define OAUTH_IS_ACCOUNT_CHOOSER_DIALOG(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), OAUTH_TYPE_ACCOUNT_CHOOSER_DIALOG))
#define OAUTH_IS_ACCOUNT_CHOOSER_DIALOG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), OAUTH_TYPE_ACCOUNT_CHOOSER_DIALOG))
#define OAUTH_ACCOUNT_CHOOSER_DIALOG_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), OAUTH_TYPE_ACCOUNT_CHOOSER_DIALOG, OAuthAccountChooserDialogClass))

typedef struct _OAuthAccountChooserDialog OAuthAccountChooserDialog;
typedef struct _OAuthAccountChooserDialogClass OAuthAccountChooserDialogClass;
typedef struct _OAuthAccountChooserDialogPrivate OAuthAccountChooserDialogPrivate;

struct _OAuthAccountChooserDialog {
	GtkDialog parent_instance;
	OAuthAccountChooserDialogPrivate *priv;
};

struct _OAuthAccountChooserDialogClass {
	GtkDialogClass parent_class;
};

GType           oauth_account_chooser_dialog_get_type    (void);
GtkWidget *     oauth_account_chooser_dialog_new         (GList                     *accounts,
							  OAuthAccount              *default_account);
OAuthAccount *  oauth_account_chooser_dialog_get_active  (OAuthAccountChooserDialog *self);

G_END_DECLS

#endif /* OAUTH_ACCOUNT_CHOOSER_DIALOG_H */
