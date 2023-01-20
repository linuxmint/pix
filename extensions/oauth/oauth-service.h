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

#ifndef OAUTH_SERVICE_H
#define OAUTH_SERVICE_H

#include <glib-object.h>
#include "web-service.h"

#define OAUTH_TYPE_SERVICE         (oauth_service_get_type ())
#define OAUTH_SERVICE(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), OAUTH_TYPE_SERVICE, OAuthService))
#define OAUTH_SERVICE_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST ((k), OAUTH_TYPE_SERVICE, OAuthServiceClass))
#define OAUTH_IS_SERVICE(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), OAUTH_TYPE_SERVICE))
#define OAUTH_IS_SERVICE_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), OAUTH_TYPE_SERVICE))
#define OAUTH_SERVICE_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS((o), OAUTH_TYPE_SERVICE, OAuthServiceClass))

typedef struct _OAuthService         OAuthService;
typedef struct _OAuthServicePrivate  OAuthServicePrivate;
typedef struct _OAuthServiceClass    OAuthServiceClass;

struct _OAuthService {
	WebService __parent;
	OAuthServicePrivate *priv;
};

struct _OAuthServiceClass {
	WebServiceClass __parent_class;
};

GType		oauth_service_get_type		(void) G_GNUC_CONST;
void		oauth_service_add_signature	(OAuthService *self,
						 const char   *method,
						 const char   *url,
						 GHashTable   *parameters);
void            oauth_service_set_token         (OAuthService *self,
					         const char   *token);
const char *    oauth_service_get_token         (OAuthService *self);
void            oauth_service_set_token_secret  (OAuthService *self,
						 const char   *token_secret);
const char *    oauth_service_get_token_secret  (OAuthService *self);

#endif /* OAUTH_SERVICE_H */
