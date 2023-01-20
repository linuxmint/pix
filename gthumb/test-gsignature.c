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
test_g_signature_md5 (void)
{
	char    *sig;
	GString *key;
	GString *data;
	int      i;

	/* -- test cases taken from http://www.faqs.org/rfcs/rfc2202.html -- */

	/* test case 1 */

	key = g_string_new ("");
	for (i = 0; i < 16; i++)
		g_string_append (key, "\x0b");
	sig = g_compute_signature_for_string (G_CHECKSUM_MD5, G_SIGNATURE_ENC_HEX, key->str, -1, "Hi There", -1);
	g_assert_cmpstr (sig, == , "9294727a3638bb1c13f48ef8158bfc9d");
	g_free (sig);
	g_string_free (key, TRUE);

	/* test case 2 */

	sig = g_compute_signature_for_string (G_CHECKSUM_MD5, G_SIGNATURE_ENC_HEX, "Jefe", -1, "what do ya want for nothing?", -1);
	g_assert_cmpstr (sig, == , "750c783e6ab0b503eaa86e310a5db738");
	g_free (sig);

	/* test case 3 */

	key = g_string_new ("");
	for (i = 0; i < 16; i++)
		g_string_append (key, "\xaa");
	data = g_string_new ("");
	for (i = 0; i < 50; i++)
		g_string_append (data, "\xdd");
	sig = g_compute_signature_for_string (G_CHECKSUM_MD5, G_SIGNATURE_ENC_HEX, key->str, -1, data->str, -1);
	g_assert_cmpstr (sig, == , "56be34521d144c88dbb8c733f0e8b3f6");
	g_free (sig);
	g_string_free (data, TRUE);
	g_string_free (key, TRUE);

	/* test case 4 */

	data = g_string_new ("");
	for (i = 0; i < 50; i++)
		g_string_append (data, "\xcd");
	sig = g_compute_signature_for_string (G_CHECKSUM_MD5, G_SIGNATURE_ENC_HEX, "\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0a\x0b\x0c\x0d\x0e\x0f\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19", -1, data->str, -1);
	g_assert_cmpstr (sig, == , "697eaf0aca3a3aea3a75164746ffaa79");
	g_free (sig);
	g_string_free (data, TRUE);

	/* test case 5 */

	key = g_string_new ("");
	for (i = 0; i < 16; i++)
		g_string_append (key, "\x0c");
	sig = g_compute_signature_for_string (G_CHECKSUM_MD5, G_SIGNATURE_ENC_HEX, key->str, -1, "Test With Truncation", -1);
	g_assert_cmpstr (sig, == , "56461ef2342edc00f9bab995690efd4c");
	g_free (sig);
	g_string_free (key, TRUE);

	/* test case 6 */

	key = g_string_new ("");
	for (i = 0; i < 80; i++)
		g_string_append (key, "\xaa");
	sig = g_compute_signature_for_string (G_CHECKSUM_MD5, G_SIGNATURE_ENC_HEX, key->str, -1, "Test Using Larger Than Block-Size Key - Hash Key First", -1);
	g_assert_cmpstr (sig, == , "6b1ab7fe4bd7bf8f0b62e6ce61b9d0cd");
	g_free (sig);
	g_string_free (key, TRUE);

	/* test case 7 */

	key = g_string_new ("");
	for (i = 0; i < 80; i++)
		g_string_append (key, "\xaa");
	sig = g_compute_signature_for_string (G_CHECKSUM_MD5, G_SIGNATURE_ENC_HEX, key->str, -1, "Test Using Larger Than Block-Size Key and Larger Than One Block-Size Data", -1);
	g_assert_cmpstr (sig, == , "6f630fad67cda0ee1fb1f562db3aa53e");
	g_free (sig);
	g_string_free (key, TRUE);
}


static void
test_g_signature_sha1 (void)
{
	char    *sig;
	GString *key;
	GString *data;
	int      i;

	/* -- test cases taken from http://www.faqs.org/rfcs/rfc2202.html -- */

	/* test case 1 */

	key = g_string_new ("");
	for (i = 0; i < 20; i++)
		g_string_append (key, "\x0b");
	sig = g_compute_signature_for_string (G_CHECKSUM_SHA1, G_SIGNATURE_ENC_HEX, key->str, -1, "Hi There", -1);
	g_assert_cmpstr (sig, == , "b617318655057264e28bc0b6fb378c8ef146be00");
	g_free (sig);
	g_string_free (key, TRUE);

	/* test case 2 */

	sig = g_compute_signature_for_string (G_CHECKSUM_SHA1, G_SIGNATURE_ENC_HEX, "Jefe", -1, "what do ya want for nothing?", -1);
	g_assert_cmpstr (sig, == , "effcdf6ae5eb2fa2d27416d5f184df9c259a7c79");
	g_free (sig);

	/* test case 3 */

	key = g_string_new ("");
	for (i = 0; i < 20; i++)
		g_string_append (key, "\xaa");
	data = g_string_new ("");
	for (i = 0; i < 50; i++)
		g_string_append (data, "\xdd");
	sig = g_compute_signature_for_string (G_CHECKSUM_SHA1, G_SIGNATURE_ENC_HEX, key->str, -1, data->str, -1);
	g_assert_cmpstr (sig, == , "125d7342b9ac11cd91a39af48aa17b4f63f175d3");
	g_free (sig);
	g_string_free (data, TRUE);
	g_string_free (key, TRUE);

	/* test case 4 */

	data = g_string_new ("");
	for (i = 0; i < 50; i++)
		g_string_append (data, "\xcd");
	sig = g_compute_signature_for_string (G_CHECKSUM_SHA1, G_SIGNATURE_ENC_HEX, "\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0a\x0b\x0c\x0d\x0e\x0f\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19", -1, data->str, -1);
	g_assert_cmpstr (sig, == , "4c9007f4026250c6bc8414f9bf50c86c2d7235da");
	g_free (sig);
	g_string_free (data, TRUE);

	/* test case 5 */

	key = g_string_new ("");
	for (i = 0; i < 20; i++)
		g_string_append (key, "\x0c");
	sig = g_compute_signature_for_string (G_CHECKSUM_SHA1, G_SIGNATURE_ENC_HEX, key->str, -1, "Test With Truncation", -1);
	g_assert_cmpstr (sig, == , "4c1a03424b55e07fe7f27be1d58bb9324a9a5a04");
	g_free (sig);
	g_string_free (key, TRUE);

	/* test case 6 */

	key = g_string_new ("");
	for (i = 0; i < 80; i++)
		g_string_append (key, "\xaa");
	sig = g_compute_signature_for_string (G_CHECKSUM_SHA1, G_SIGNATURE_ENC_HEX, key->str, -1, "Test Using Larger Than Block-Size Key - Hash Key First", -1);
	g_assert_cmpstr (sig, == , "aa4ae5e15272d00e95705637ce8a3b55ed402112");
	g_free (sig);
	g_string_free (key, TRUE);

	/* test case 7 */

	key = g_string_new ("");
	for (i = 0; i < 80; i++)
		g_string_append (key, "\xaa");
	sig = g_compute_signature_for_string (G_CHECKSUM_SHA1, G_SIGNATURE_ENC_HEX, key->str, -1, "Test Using Larger Than Block-Size Key and Larger Than One Block-Size Data", -1);
	g_assert_cmpstr (sig, == , "e8e99d0f45237d786d6bbaa7965c7808bbff1a91");
	g_free (sig);
	g_string_free (key, TRUE);

	/* -- test case taken from php-5.3.2/ext/hash/tests/hash_hmac_bacis.phpt -- */

	sig = g_compute_signature_for_string (G_CHECKSUM_SHA1, G_SIGNATURE_ENC_HEX, "secret", -1, "This is a sample string used to test the hash_hmac function with various hashing algorithms", -1);
	g_assert_cmpstr (sig, == , "5bfdb62b97e2c987405463e9f7c193139c0e1fd0");
	g_free (sig);

	/* -- test case created using the crypto-js library (http://code.google.com/p/crypto-js/) -- */

	sig = g_compute_signature_for_string (G_CHECKSUM_SHA1, G_SIGNATURE_ENC_HEX, "Secret Passphrase", -1, "Message", -1);
	g_assert_cmpstr (sig, == , "e90f713295ea4cc06c92c9248696ffafc5d01faf");
	g_free (sig);

	key = g_string_new ("");
	for (i = 0; i < 1; i++)
		g_string_append (key, "aa");
	sig = g_compute_signature_for_string (G_CHECKSUM_SHA1, G_SIGNATURE_ENC_HEX, key->str, -1, "Test Using Larger Than Block-Size Key - Hash Key First", -1);
	g_assert_cmpstr (sig, == , "4f8e4da5b85182a352041a22b4d566b4b53abf9e");
	g_free (sig);
	g_string_free (key, TRUE);

	key = g_string_new ("");
	for (i = 0; i < 20; i++)
		g_string_append (key, "aa");
	sig = g_compute_signature_for_string (G_CHECKSUM_SHA1, G_SIGNATURE_ENC_HEX, key->str, -1, "Test Using Larger Than Block-Size Key - Hash Key First", -1);
	g_assert_cmpstr (sig, == , "cb8da9d75b1cd177c00e8c46bd9c17fa313b9f6c");
	g_free (sig);
	g_string_free (key, TRUE);

	key = g_string_new ("");
	for (i = 0; i < 80; i++)
		g_string_append (key, "aa");
	sig = g_compute_signature_for_string (G_CHECKSUM_SHA1, G_SIGNATURE_ENC_HEX, key->str, -1, "Test Using Larger Than Block-Size Key - Hash Key First", -1);
	g_assert_cmpstr (sig, == , "f196c43f06a566cb096a72227a3196d97236898b");
	g_free (sig);
	g_string_free (key, TRUE);
}


int
main (int   argc,
      char *argv[])
{
	g_test_init (&argc, &argv, NULL);

	g_test_add_func ("/gsignature/g_compute_signature_for_string/md5", test_g_signature_md5);
	g_test_add_func ("/gsignature/g_compute_signature_for_string/sha1", test_g_signature_sha1);

	return g_test_run ();
}
