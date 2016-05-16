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
#include "oauth-consumer.h"


OAuthConsumer *
oauth_consumer_copy (OAuthConsumer *consumer)
{
	OAuthConsumer *consumer_2;

	consumer_2 = g_new0 (OAuthConsumer, 1);
	consumer_2->consumer_key = consumer->consumer_key;
	consumer_2->consumer_secret = consumer->consumer_secret;
	consumer_2->request_token_url = consumer->request_token_url;
	consumer_2->get_authorization_url = consumer->get_authorization_url;
	consumer_2->access_token_url = consumer->access_token_url;
	consumer_2->access_token_response = consumer->access_token_response;

	return consumer_2;
}


void
oauth_consumer_free (OAuthConsumer *consumer)
{
	if (consumer == NULL)
		return;
	g_free (consumer);
}
