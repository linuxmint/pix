/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2009 The Free Software Foundation, Inc.
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
#include <zlib.h> 
#include "zlib-utils.h"


#define BUFFER_SIZE (16 * 1024)


gboolean 
zlib_decompress_buffer (void   *zipped_buffer, 
			gsize   zipped_size, 
		        void  **buffer, 
		        gsize  *size)
{
	z_stream  strm;
	int       ret;
	guint     n;	
	gsize     count;
	guchar   *local_buffer = NULL;
	guchar    tmp_buffer[BUFFER_SIZE];
	
	strm.zalloc = Z_NULL;
	strm.zfree = Z_NULL;
	strm.opaque = Z_NULL;
	strm.avail_in = 0;
	strm.next_in = Z_NULL;
	ret = inflateInit2 (&strm, 16+15);
	if (ret != Z_OK)
		return FALSE;

	count = 0;
	strm.avail_in = zipped_size;
	strm.next_in = zipped_buffer;
	do {
		do {
			strm.avail_out = BUFFER_SIZE;
			strm.next_out = tmp_buffer;
			ret = inflate (&strm, Z_NO_FLUSH);
			
			switch (ret) {
			case Z_NEED_DICT:   
			case Z_DATA_ERROR:
			case Z_MEM_ERROR:
				g_free (local_buffer);
				inflateEnd (&strm);
				return FALSE;
			}
			
			n = BUFFER_SIZE - strm.avail_out;
			local_buffer = g_realloc (local_buffer, count + n + 1);
			memcpy (local_buffer + count, tmp_buffer, n);
			count += n;
		} 
		while (strm.avail_out == 0);
	} 
	while (ret != Z_STREAM_END);

	inflateEnd (&strm);
	
	*buffer = local_buffer;
	*size = count;
	
	return ret == Z_STREAM_END;
}
