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

#ifndef PHOTOBUCKET_ACCOUNT_H
#define PHOTOBUCKET_ACCOUNT_H

#include <glib.h>
#include <glib-object.h>
#include <extensions/oauth/oauth-account.h>

G_BEGIN_DECLS

#define PHOTOBUCKET_TYPE_ACCOUNT            (photobucket_account_get_type ())
#define PHOTOBUCKET_ACCOUNT(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), PHOTOBUCKET_TYPE_ACCOUNT, PhotobucketAccount))
#define PHOTOBUCKET_ACCOUNT_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), PHOTOBUCKET_TYPE_ACCOUNT, PhotobucketAccountClass))
#define PHOTOBUCKET_IS_ACCOUNT(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PHOTOBUCKET_TYPE_ACCOUNT))
#define PHOTOBUCKET_IS_ACCOUNT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), PHOTOBUCKET_TYPE_ACCOUNT))
#define PHOTOBUCKET_ACCOUNT_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), PHOTOBUCKET_TYPE_ACCOUNT, PhotobucketAccountClass))

typedef struct _PhotobucketAccount PhotobucketAccount;
typedef struct _PhotobucketAccountClass PhotobucketAccountClass;

struct _PhotobucketAccount {
	OAuthAccount parent_instance;

	char     *subdomain;
	char     *home_url;
	char     *album_url;
	goffset   megabytes_used;
	goffset   megabytes_allowed;
	gboolean  is_premium;
	gboolean  is_public;
};

struct _PhotobucketAccountClass {
	OAuthAccountClass parent_class;
};

GType             photobucket_account_get_type               (void);
OAuthAccount *    photobucket_account_new                    (void);
void              photobucket_account_set_subdomain          (PhotobucketAccount *self,
							      const char         *value);
void              photobucket_account_set_home_url           (PhotobucketAccount *self,
							      const char         *value);
void              photobucket_account_set_album_url          (PhotobucketAccount *self,
							      const char         *value);
void              photobucket_account_set_megabytes_used     (PhotobucketAccount *self,
							      const char         *value);
void              photobucket_account_set_megabytes_allowed  (PhotobucketAccount *self,
						              const char         *value);
void              photobucket_account_set_is_premium         (PhotobucketAccount *self,
							      const char         *value);
void              photobucket_account_set_is_public          (PhotobucketAccount *self,
							      const char         *value);

G_END_DECLS

#endif /* PHOTOBUCKET_ACCOUNT_H */
