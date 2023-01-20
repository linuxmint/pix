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
#include "jmemorydest.h"


#define TMP_BUF_SIZE  4096
#define JPEG_ERROR(cinfo,code)  \
  ((cinfo)->err->msg_code = (code), \
   (*(cinfo)->err->error_exit) ((j_common_ptr) (cinfo)))


typedef struct {
	struct jpeg_destination_mgr pub;

	guchar **out_buffer;
	gsize   *out_buffer_size;
	gsize    bytes_written;
	JOCTET  *tmp_buffer;
} mem_destination_mgr;

typedef mem_destination_mgr * mem_dest_ptr;


static void
init_destination (j_compress_ptr cinfo)
{
	mem_dest_ptr dest = (mem_dest_ptr) cinfo->dest;

	*dest->out_buffer = NULL;
	*dest->out_buffer_size = 0;
	dest->tmp_buffer = (JOCTET *)
		(*cinfo->mem->alloc_small) ((j_common_ptr) cinfo,
					    JPOOL_IMAGE,
				            TMP_BUF_SIZE * sizeof (JOCTET));
	dest->bytes_written = 0;
	dest->pub.next_output_byte = dest->tmp_buffer;
	dest->pub.free_in_buffer = TMP_BUF_SIZE;
}


static gboolean
empty_output_buffer (j_compress_ptr cinfo)
{
	mem_dest_ptr dest = (mem_dest_ptr) cinfo->dest;

	*dest->out_buffer = g_realloc (*dest->out_buffer, *dest->out_buffer_size + TMP_BUF_SIZE);
	if (*dest->out_buffer != NULL) {
		*dest->out_buffer_size = *dest->out_buffer_size + TMP_BUF_SIZE;
		memcpy (*dest->out_buffer + dest->bytes_written, dest->tmp_buffer, TMP_BUF_SIZE);
		dest->bytes_written += TMP_BUF_SIZE;
	}
	else
		JPEG_ERROR (cinfo, G_IO_ERROR_FAILED);

	dest->pub.next_output_byte = dest->tmp_buffer;
	dest->pub.free_in_buffer = TMP_BUF_SIZE;

	return TRUE;
}


static void
term_destination (j_compress_ptr cinfo)
{
	mem_dest_ptr dest = (mem_dest_ptr) cinfo->dest;
	size_t      datacount;

	datacount = TMP_BUF_SIZE - dest->pub.free_in_buffer;
	if (datacount > 0) {
		*dest->out_buffer = g_realloc (*dest->out_buffer, *dest->out_buffer_size + datacount);
		if (*dest->out_buffer != NULL) {
			*dest->out_buffer_size = *dest->out_buffer_size + datacount;
			memcpy (*dest->out_buffer + dest->bytes_written, dest->tmp_buffer, datacount);
			dest->bytes_written += datacount;
		}
		else
			JPEG_ERROR (cinfo, G_IO_ERROR_FAILED);
	}
}


void
_jpeg_memory_dest (j_compress_ptr   cinfo,
		   void           **out_buffer,
		   gsize           *out_buffer_size)
{
	mem_dest_ptr dest;

	if (cinfo->dest == NULL) {
		cinfo->dest = (struct jpeg_destination_mgr *)
			(*cinfo->mem->alloc_small) ((j_common_ptr) cinfo,
						    JPOOL_PERMANENT,
						    sizeof (mem_destination_mgr));
	}

	dest = (mem_dest_ptr) cinfo->dest;
	dest->pub.init_destination = init_destination;
	dest->pub.empty_output_buffer = empty_output_buffer;
	dest->pub.term_destination = term_destination;
	dest->out_buffer = (guchar **) out_buffer;
	dest->out_buffer_size = out_buffer_size;
	dest->bytes_written = 0;
}
