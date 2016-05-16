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
#include "flickr-account.h"
#include "flickr-consumer.h"
#include "flickr-service.h"


gboolean
flickr_utils_parse_response (SoupBuffer   *body,
			     DomDocument **doc_p,
			     GError      **error)
{
	DomDocument *doc;
	DomElement  *node;

	doc = dom_document_new ();
	if (! dom_document_load (doc, body->data, body->length, error)) {
		g_object_unref (doc);
		return FALSE;
	}

	for (node = DOM_ELEMENT (doc)->first_child; node; node = node->next_sibling) {
		if (g_strcmp0 (node->tag_name, "rsp") == 0) {
			if (g_strcmp0 (dom_element_get_attribute (node, "stat"), "ok") != 0) {
				DomElement *child;

				for (child = node->first_child; child; child = child->next_sibling) {
					if (g_strcmp0 (child->tag_name, "err") == 0) {
						*error = g_error_new_literal (WEB_SERVICE_ERROR,
									      atoi (dom_element_get_attribute (child, "code")),
									      dom_element_get_attribute (child, "msg"));
					}
				}

				g_object_unref (doc);
				return FALSE;
			}
		}
	}

	*doc_p = doc;

	return TRUE;
}


static char *
flickr_get_authorization_url (OAuthService *self)
{
	FlickrServer *server;
	char         *escaped_token;
	char         *uri;

	server = flickr_service_get_server (FLICKR_SERVICE (self));
	escaped_token = soup_uri_encode (oauth_service_get_token (self), NULL);
	uri = g_strconcat (server->authorization_url,
			   "?oauth_token=", escaped_token,
			   "&perms=write",
			   NULL);

	g_free (escaped_token);

	return uri;
}


static void
flickr_access_token_response (OAuthService       *self,
			      SoupMessage        *msg,
			      SoupBuffer         *body,
			      GSimpleAsyncResult *result)
{
	GHashTable *values;
	char       *username;
	char       *token;
	char       *token_secret;

	values = soup_form_decode (body->data);

	username = g_hash_table_lookup (values, "username");
	token = g_hash_table_lookup (values, "oauth_token");
	token_secret = g_hash_table_lookup (values, "oauth_token_secret");
	if ((username != NULL) && (token != NULL) && (token_secret != NULL)) {
		FlickrAccount *account;

		oauth_service_set_token (OAUTH_SERVICE (self), token);
		oauth_service_set_token_secret (OAUTH_SERVICE (self), token_secret);

		account = g_object_new (FLICKR_TYPE_ACCOUNT,
					"id", g_hash_table_lookup (values, "user_nsid"),
					"name", username,
					"token", token,
					"token-secret", token_secret,
					NULL);
		g_simple_async_result_set_op_res_gpointer (result, account, g_object_unref);
	}
	else {
		GError *error;

		error = g_error_new_literal (WEB_SERVICE_ERROR, WEB_SERVICE_ERROR_GENERIC, _("Unknown error"));
		g_simple_async_result_set_from_error (result, error);
	}

	g_hash_table_destroy (values);
}


OAuthConsumer flickr_consumer = {
	NULL,
	NULL,
	NULL,
	flickr_get_authorization_url,
	NULL,
	flickr_access_token_response
};
