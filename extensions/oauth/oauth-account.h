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

#ifndef OAUTH_ACCOUNT_H
#define OAUTH_ACCOUNT_H

#include <glib.h>
#include <glib-object.h>
#include <gthumb.h>

G_BEGIN_DECLS

#define OAUTH_TYPE_ACCOUNT            (oauth_account_get_type ())
#define OAUTH_ACCOUNT(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), OAUTH_TYPE_ACCOUNT, OAuthAccount))
#define OAUTH_ACCOUNT_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), OAUTH_TYPE_ACCOUNT, OAuthAccountClass))
#define OAUTH_IS_ACCOUNT(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), OAUTH_TYPE_ACCOUNT))
#define OAUTH_IS_ACCOUNT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), OAUTH_TYPE_ACCOUNT))
#define OAUTH_ACCOUNT_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), OAUTH_TYPE_ACCOUNT, OAuthAccountClass))

typedef struct _OAuthAccount OAuthAccount;
typedef struct _OAuthAccountClass OAuthAccountClass;
typedef struct _OAuthAccountPrivate OAuthAccountPrivate;

struct _OAuthAccount {
	GObject parent_instance;
	OAuthAccountPrivate *priv;

	char     *id;
	char     *username;
	char     *name;
	char     *token;
	char     *token_secret;
	gboolean  is_default;
};

struct _OAuthAccountClass {
	GObjectClass parent_class;
};

GType             oauth_account_get_type           (void);
OAuthAccount *    oauth_account_new                (void);
void              oauth_account_set_username       (OAuthAccount *self,
						    const char   *value);
void              oauth_account_set_token          (OAuthAccount *self,
						    const char   *value);
void              oauth_account_set_token_secret   (OAuthAccount *self,
						    const char   *value);
int               oauth_account_cmp                (OAuthAccount *a,
						    OAuthAccount *b);
DomElement *      oauth_account_create_element     (DomDomizable *base,
						    DomDocument  *doc);
void              oauth_account_load_from_element  (DomDomizable *base,
						    DomElement   *element);

/* -- utilities -- */

GList *           oauth_accounts_load_from_file    (const char   *service_name,
						    GType         account_type);
OAuthAccount *    oauth_accounts_find_default      (GList        *accounts);
void              oauth_accounts_save_to_file      (const char   *service_name,
						    GList        *accounts,
						    OAuthAccount *default_account);

G_END_DECLS

#endif /* OAUTH_ACCOUNT_H */
