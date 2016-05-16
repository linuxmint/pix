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

#include <config.h>
#include <glib.h>
#include <glib/gi18n.h>
#include <gthumb.h>
#include "photobucket-account.h"
#include "photobucket-consumer.h"


gboolean
photobucket_utils_parse_response (SoupMessage         *msg,
				  DomDocument        **doc_p,
				  GError             **error)
{
	SoupBuffer  *body;
	DomDocument *doc;
	DomElement  *node;

	body = soup_message_body_flatten (msg->response_body);

	doc = dom_document_new ();
	if (! dom_document_load (doc, body->data, body->length, error)) {
		if (msg->status_code != 200) {
			g_clear_error (error);
			*error = g_error_new_literal (SOUP_HTTP_ERROR, msg->status_code, soup_status_get_phrase (msg->status_code));
		}
		g_object_unref (doc);
		soup_buffer_free (body);
		return FALSE;
	}

	soup_buffer_free (body);

	for (node = DOM_ELEMENT (doc)->first_child; node; node = node->next_sibling) {
		if (g_strcmp0 (node->tag_name, "response") == 0) {
			DomElement *child;
			const char *status = NULL;
			const char *message = NULL;
			const char *code = NULL;

			for (child = node->first_child; child; child = child->next_sibling) {
				if (g_strcmp0 (child->tag_name, "status") == 0) {
					status = dom_element_get_inner_text (child);
				}
				else if (g_strcmp0 (child->tag_name, "message") == 0) {
					message = dom_element_get_inner_text (child);
				}
				else if (g_strcmp0 (child->tag_name, "code") == 0) {
					code = dom_element_get_inner_text (child);
				}
			}

			if (status == NULL) {
				*error = g_error_new_literal (WEB_SERVICE_ERROR, 999, _("Unknown error"));
			}
			else if (strcmp (status, "Exception") == 0) {
				int error_code = (code != NULL) ? atoi (code) : 999;
				if (error_code == 7)
					error_code = WEB_SERVICE_ERROR_TOKEN_EXPIRED;
				*error = g_error_new_literal (WEB_SERVICE_ERROR,
							      error_code,
							      (message != NULL) ? message : _("Unknown error"));
			}

			if (*error != NULL) {
				g_object_unref (doc);
				return FALSE;
			}
		}
	}

	*doc_p = doc;

	return TRUE;
}


static char *
photobucket_get_authorization_url (OAuthService *self)
{
	char *escaped_token;
	char *uri;

	escaped_token = soup_uri_encode (oauth_service_get_token (self), NULL);
	uri = g_strconcat ("http://photobucket.com/apilogin/login?oauth_token=",
			   escaped_token,
			   NULL);

	g_free (escaped_token);

	return uri;
}


static void
photobucket_access_token_response (OAuthService       *self,
				   SoupMessage        *msg,
				   SoupBuffer         *body,
				   GSimpleAsyncResult *result)
{
	GHashTable *values;
	char       *token;
	char       *token_secret;

	values = soup_form_decode (body->data);

	token = g_hash_table_lookup (values, "oauth_token");
	token_secret = g_hash_table_lookup (values, "oauth_token_secret");
	if ((token != NULL) && (token_secret != NULL)) {
		OAuthAccount *account;

		oauth_service_set_token (self, token);
		oauth_service_set_token_secret (self, token_secret);

		account = g_object_new (PHOTOBUCKET_TYPE_ACCOUNT,
					"id", g_hash_table_lookup (values, "username"),
					"name", g_hash_table_lookup (values, "username"),
					"username", g_hash_table_lookup (values, "username"),
					"token", token,
					"token-secret", token_secret,
					"subdomain", g_hash_table_lookup (values, "subdomain"),
					"home-url", g_hash_table_lookup (values, "homeurl"),
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


OAuthConsumer photobucket_consumer = {
	"149829931",
	"b4e542229836cc59b66489c6d2d8ca04",
	"http://api.photobucket.com/login/request",
	photobucket_get_authorization_url,
	"http://api.photobucket.com/login/access",
	photobucket_access_token_response
};
