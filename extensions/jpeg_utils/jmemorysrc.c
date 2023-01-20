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
#include <stdio.h>
#include <jpeglib.h>
#include <glib.h>
#include <gio/gio.h>
#include "jmemorysrc.h"


#define JPEG_ERROR(cinfo,code)  \
  ((cinfo)->err->msg_code = (code), \
   (*(cinfo)->err->error_exit) ((j_common_ptr) (cinfo)))


static void
init_source (j_decompress_ptr cinfo)
{
	/* void */
}


static gboolean
fill_input_buffer (j_decompress_ptr cinfo)
{
	static JOCTET mybuffer[4];

	/* The whole JPEG data is expected to reside in the supplied memory
	 * buffer, so any request for more data beyond the given buffer size
	 * is treated as an error.
	 */

	JPEG_ERROR (cinfo, G_IO_ERROR_NOT_FOUND);

	/* Insert a fake EOI marker */
	mybuffer[0] = (JOCTET) 0xFF;
	mybuffer[1] = (JOCTET) JPEG_EOI;

	cinfo->src->next_input_byte = mybuffer;
	cinfo->src->bytes_in_buffer = 2;

	return TRUE;
}


static void
skip_input_data (j_decompress_ptr cinfo,
		 long             num_bytes)
{
	struct jpeg_source_mgr * src = cinfo->src;

	/* Just a dumb implementation for now.  Could use fseek() except
	 * it doesn't work on pipes.  Not clear that being smart is worth
	 * any trouble anyway --- large skips are infrequent.
	 */
	if (num_bytes > 0) {
		while (num_bytes > (long) src->bytes_in_buffer) {
			num_bytes -= (long) src->bytes_in_buffer;
			(void) (*src->fill_input_buffer) (cinfo);
			/* note we assume that fill_input_buffer will never return FALSE,
			 * so suspension need not be handled.
			 */
		}
		src->next_input_byte += (size_t) num_bytes;
		src->bytes_in_buffer -= (size_t) num_bytes;
	}
}


static void
term_source (j_decompress_ptr cinfo)
{
	/* void */
}


void
_jpeg_memory_src (j_decompress_ptr  cinfo,
		  void             *in_buffer,
		  gsize             in_buffer_size)
{
	struct jpeg_source_mgr *src;

	if (cinfo->src == NULL) {
		cinfo->src = (struct jpeg_source_mgr *)
			(*cinfo->mem->alloc_small) ((j_common_ptr) cinfo,
						    JPOOL_PERMANENT,
						    sizeof (struct jpeg_source_mgr));
	}

	src = cinfo->src;
	src->init_source = init_source;
	src->fill_input_buffer = fill_input_buffer;
	src->skip_input_data = skip_input_data;
	src->resync_to_restart = jpeg_resync_to_restart;
	src->term_source = term_source;
	src->bytes_in_buffer = (size_t) in_buffer_size;
	src->next_input_byte = (JOCTET *) in_buffer;
}
