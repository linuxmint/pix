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

#ifndef OAUTH_CONSUMER_H
#define OAUTH_CONSUMER_H


#include <glib.h>
#ifdef HAVE_LIBSOUP_GNOME
#include <libsoup/soup-gnome.h>
#else
#include <libsoup/soup.h>
#endif /* HAVE_LIBSOUP_GNOME */
#include "oauth-account.h"
#include "oauth-service.h"


typedef void   (*OAuthResponseFunc)	(OAuthService       *self,
				 	 SoupMessage        *msg,
				 	 SoupBuffer         *body,
				 	 GSimpleAsyncResult *result);
typedef char * (*OAuthStringFunc)      (OAuthService        *self);


typedef struct {
	const char		*consumer_key;
	const char		*consumer_secret;
	const char		*request_token_url;
	OAuthStringFunc		 get_authorization_url;
	const char		*access_token_url;
	OAuthResponseFunc	 access_token_response;
} OAuthConsumer;


OAuthConsumer *   oauth_consumer_copy  (OAuthConsumer *consumer);
void              oauth_consumer_free  (OAuthConsumer *consumer);


#endif /* OAUTH_CONSUMER_H */
