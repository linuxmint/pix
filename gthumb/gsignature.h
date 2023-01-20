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

#ifndef G_SIGNATURE_H
#define G_SIGNATURE_H

#include <glib.h>

G_BEGIN_DECLS

typedef struct _GSignature GSignature;
typedef enum {
	G_SIGNATURE_ENC_HEX,
	G_SIGNATURE_ENC_BASE64
} GSignatureEnc;

GSignature *           g_signature_new         (GChecksumType     checksum_type,
					        const gchar      *key,
					        gssize            key_length);
GSignature *           g_signature_copy        (const GSignature *signature);
void                   g_signature_free        (GSignature       *signature);
void                   g_signature_reset       (GSignature       *signature);
void                   g_signature_update      (GSignature       *signature,
                                                const guchar     *data,
                                                gssize            length);
const gchar *          g_signature_get_string  (GSignature       *signature);
void                   g_signature_get_value   (GSignature       *signature,
						guint8           *buffer,
						gsize            *buffer_len);

gchar *g_compute_signature_for_data   (GChecksumType  checksum_type,
				       GSignatureEnc  encoding,
				       const gchar   *key,
				       gssize         key_length,
                                       const guchar  *data,
                                       gsize          data_length);
gchar *g_compute_signature_for_string (GChecksumType  checksum_type,
				       GSignatureEnc  encoding,
				       const gchar   *key,
				       gssize         key_length,
                                       const gchar   *str,
                                       gssize         str_length);

G_END_DECLS

#endif /* G_SIGNATURE_H */
