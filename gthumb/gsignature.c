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


/* Algorithm description: http://en.wikipedia.org/wiki/HMAC
 * Reference implementation: http://www.faqs.org/rfcs/rfc2202.html  */


#define BLOCK_SIZE 64 /* in gchecksum.c DATASIZE is always 64 */


struct _GSignature
{
	GChecksumType  checksum_type;
	GChecksum     *primary;
	GChecksum     *internal;
	guchar         opad[BLOCK_SIZE];
	guchar         ipad[BLOCK_SIZE];
};


GSignature *
g_signature_new (GChecksumType  checksum_type,
		 const gchar   *key,
		 gssize         key_length)
{
	GSignature *signature;
	gchar      *local_key;
	int         i;

	if (key == NULL)
		return NULL;

	signature = g_slice_new0 (GSignature);
	signature->checksum_type = checksum_type;
	signature->primary = g_checksum_new (checksum_type);
	signature->internal = g_checksum_new (checksum_type);

	if (key_length == -1)
		key_length = strlen (key);

	if (key_length > BLOCK_SIZE) {
		GChecksum *key_checksum;
		gsize      digest_len;

		key_checksum = g_checksum_new (checksum_type);
		g_checksum_update (key_checksum, (guchar *) key, key_length);
		key_length = digest_len = g_checksum_type_get_length (checksum_type);
		local_key = g_new0 (char, key_length);
		g_checksum_get_digest (key_checksum, (guint8 *) local_key, &digest_len);

		g_checksum_free (key_checksum);
	}
	else
		local_key = (gchar *) key;

	for (i = 0; i < key_length; i++) {
		signature->opad[i] = 0x5c ^ local_key[i];
		signature->ipad[i] = 0x36 ^ local_key[i];
	}

	for (i = key_length; i < BLOCK_SIZE; i++) {
		signature->opad[i] = 0x5c;
		signature->ipad[i] = 0x36;
	}

	g_checksum_update (signature->primary, signature->opad, BLOCK_SIZE);
	g_checksum_update (signature->internal, signature->ipad, BLOCK_SIZE);

	if (local_key != key)
		g_free (local_key);

	return signature;
}


GSignature *
g_signature_copy (const GSignature *signature)
{
	GSignature *copy;

	g_return_val_if_fail (signature != NULL, NULL);

	copy = g_slice_new (GSignature);
	*copy = *signature;
	copy->primary = g_checksum_copy (signature->primary);
	copy->internal = g_checksum_copy (signature->internal);

	return copy;
}


void
g_signature_free (GSignature *signature)
{
	if (signature == NULL)
		return;

	g_checksum_free (signature->primary);
	g_checksum_free (signature->internal);
	g_slice_free (GSignature, signature);
}


void
g_signature_reset (GSignature *signature)
{
	g_checksum_reset (signature->internal);
	g_checksum_update (signature->internal, signature->ipad, BLOCK_SIZE);
}


void
g_signature_update (GSignature   *signature,
		    const guchar *data,
		    gssize        length)
{
	g_checksum_update (signature->internal, data, length);
}


const gchar *
g_signature_get_string (GSignature *signature)
{
	guint8 *internal_digest;
	gsize   digest_len;

	digest_len = g_checksum_type_get_length (signature->checksum_type);
	internal_digest = g_new (guint8, digest_len);
	g_checksum_get_digest (signature->internal, internal_digest, &digest_len);
	g_checksum_update (signature->primary, internal_digest, digest_len);

	g_free (internal_digest);

	return g_checksum_get_string (signature->primary);
}


void
g_signature_get_value (GSignature *signature,
		       guint8     *buffer,
		       gsize      *buffer_len)
{
	guint8 *internal_digest;
	gsize   digest_len;

	digest_len = g_checksum_type_get_length (signature->checksum_type);
	internal_digest = g_new (guint8, digest_len);
	g_checksum_get_digest (signature->internal, internal_digest, &digest_len);
	g_checksum_update (signature->primary, internal_digest, digest_len);
	g_checksum_get_digest (signature->primary, buffer, buffer_len);

	g_free (internal_digest);
}


gchar *
g_compute_signature_for_data (GChecksumType  checksum_type,
			      GSignatureEnc  encoding,
			      const gchar   *key,
			      gssize         key_length,
			      const guchar  *data,
			      gsize          data_length)
{
	GSignature *signature;
	gchar      *retval = NULL;

	g_return_val_if_fail (data != NULL, NULL);

	signature = g_signature_new (checksum_type, key, key_length);
	if (signature == NULL)
		return NULL;

	g_signature_update (signature, data, data_length);
	switch (encoding) {
	case G_SIGNATURE_ENC_BASE64:
		{
			gsize   buffer_len;
			guint8 *buffer;

			buffer_len = g_checksum_type_get_length (signature->checksum_type);
			buffer = g_new (guint8, buffer_len);
			g_signature_get_value (signature, buffer, &buffer_len);
			retval = g_base64_encode (buffer, buffer_len);

			g_free (buffer);
		}
		break;

	case G_SIGNATURE_ENC_HEX:
		retval = g_strdup (g_signature_get_string (signature));
		break;
	}

	g_signature_free (signature);

	return retval;
}


gchar *
g_compute_signature_for_string (GChecksumType  checksum_type,
				GSignatureEnc  encoding,
				const gchar   *key,
				gssize         key_length,
				const gchar   *str,
				gssize         str_length)
{
	g_return_val_if_fail (str != NULL, NULL);

	if (str_length < 0)
		str_length = strlen (str);

	return g_compute_signature_for_data (checksum_type, encoding, key, key_length, (const guchar *) str, str_length);
}
