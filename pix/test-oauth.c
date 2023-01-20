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
#include <string.h>
#include "gsignature.h"


static void
oauth_connection_add_signature (const char *method,
				const char *url,
			        GHashTable *parameters,
			        const char *expected)
{
	char     *timestamp;
	char     *nonce;
	char     *consumer_key;
	char     *consumer_secret;
	GString  *param_string;
	GList    *keys;
	GList    *scan;
	GString  *base_string;
	GString  *signature_key;
	char     *signature;

	/* example values from photobucket */

	timestamp = "1208289833";
	nonce = "cc09413d20742699147d083d755023f7";
	consumer_key = "1020304";
	consumer_secret = "9eb84090956c484e32cb6c08455a667b";

	/* Add the OAuth specific parameters */

	g_hash_table_insert (parameters, "oauth_timestamp", timestamp);
	g_hash_table_insert (parameters, "oauth_nonce", nonce);
	g_hash_table_insert (parameters, "oauth_version", "1.0");
	g_hash_table_insert (parameters, "oauth_signature_method", "HMAC-SHA1");
	g_hash_table_insert (parameters, "oauth_consumer_key", consumer_key);

	/* Create the parameter string */

	param_string = g_string_new ("");
	keys = g_hash_table_get_keys (parameters);
	keys = g_list_sort (keys, (GCompareFunc) strcmp);
	for (scan = keys; scan; scan = scan->next) {
		char *key = scan->data;

		g_string_append (param_string, key);
		g_string_append (param_string, "=");
		g_string_append_uri_escaped (param_string,
					     g_hash_table_lookup (parameters, key),
					     NULL,
					     TRUE);
		if (scan->next != NULL)
			g_string_append (param_string, "&");
	}

	/* Create the Base String */

	base_string = g_string_new ("");
	g_string_append_uri_escaped (base_string, method, NULL, TRUE);
	g_string_append (base_string, "&");
	g_string_append_uri_escaped (base_string, url, NULL, TRUE);
	g_string_append (base_string, "&");
	g_string_append_uri_escaped (base_string, param_string->str, NULL, TRUE);

	/* Calculate the signature value */

	signature_key = g_string_new ("");
	g_string_append (signature_key, consumer_secret);
	g_string_append (signature_key, "&");
	signature = g_compute_signature_for_string (G_CHECKSUM_SHA1,
						    G_SIGNATURE_ENC_BASE64,
						    signature_key->str,
						    signature_key->len,
						    base_string->str,
						    base_string->len);
	g_hash_table_insert (parameters, "oauth_signature", signature);

	g_assert_cmpstr (signature, == , expected);

	g_free (signature);
	g_string_free (signature_key, TRUE);
	g_string_free (base_string, TRUE);
	g_list_free (keys);
	g_string_free (param_string, TRUE);
}


static void
test_oauth_signature (void)
{
	GHashTable  *data_set;

	data_set = g_hash_table_new (g_str_hash, g_str_equal);

	oauth_connection_add_signature ("GET",
					"http://api.photobucket.com/ping",
					data_set,
				        "2j4y6ocB4d4tTxDoHaNulKpA46c=");

	g_hash_table_destroy (data_set);
}


int
main (int   argc,
      char *argv[])
{
	g_test_init (&argc, &argv, NULL);
	g_test_add_func ("/oauth/signature", test_oauth_signature);
	return g_test_run ();
}
