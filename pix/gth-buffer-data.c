/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2009 Free Software Foundation, Inc.
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
#include <glib/gi18n.h>
#include "gth-buffer-data.h"
#include "gth-error.h"


#define INITIAL_ALLOCATED_SIZE 1024


struct _GthBufferData {
	char    *buffer;
	gsize    buffer_size;
	gsize    allocated_size;
	goffset  current_pos;
};


GthBufferData *
gth_buffer_data_new (void)
{
	GthBufferData *buffer_data;

	buffer_data = g_new (GthBufferData, 1);
	buffer_data->buffer = g_try_malloc (INITIAL_ALLOCATED_SIZE);
	buffer_data->buffer_size = 0;
	buffer_data->allocated_size = INITIAL_ALLOCATED_SIZE;
	buffer_data->current_pos = 0;

	return buffer_data;
}


void
gth_buffer_data_free (GthBufferData *buffer_data,
		      gboolean       free_segment)
{
	if (free_segment)
		g_free (buffer_data->buffer);
	g_free (buffer_data);
}


static gboolean
gth_buffer_data_alloc_new_space (GthBufferData  *buffer_data,
				 gsize           len,
				 GError        **error)
{
	if (buffer_data->current_pos + len > buffer_data->buffer_size)
		buffer_data->buffer_size = buffer_data->current_pos + len;

	if (buffer_data->buffer_size > buffer_data->allocated_size) {
		gsize  new_allocated_size;
		char  *new_buffer;

		new_allocated_size = MAX (buffer_data->allocated_size * 2, buffer_data->buffer_size + len);
		new_buffer = g_try_realloc (buffer_data->buffer, new_allocated_size);
		if (new_buffer == NULL) {
			g_set_error_literal (error,
					     GTH_ERROR,
					     GTH_ERROR_GENERIC,
                                             _("Insufficient memory"));
			return FALSE;
		}

		buffer_data->buffer = new_buffer;
		buffer_data->allocated_size = new_allocated_size;
	}

	return TRUE;
}


gboolean
gth_buffer_data_write (GthBufferData  *buffer_data,
		       const void     *buffer,
		       gsize           len,
		       GError        **error)
{
	if (len <= 0)
		return TRUE;

	if (! gth_buffer_data_alloc_new_space (buffer_data, len, error))
		return FALSE;

	memcpy (buffer_data->buffer + buffer_data->current_pos, buffer, len);
	buffer_data->current_pos += len;

	return TRUE;
}


gboolean
gth_buffer_data_putc (GthBufferData  *buffer_data,
		      int             c,
		      GError        **error)
{
	if (! gth_buffer_data_alloc_new_space (buffer_data, 1, error))
		return FALSE;

	buffer_data->buffer[buffer_data->current_pos] = (char) c;
	buffer_data->current_pos += 1;

	return TRUE;
}


goffset
gth_buffer_data_seek (GthBufferData *buffer_data,
		      goffset        offset,
		      int            whence)
{
        switch (whence) {
        case SEEK_SET:
        	buffer_data->current_pos = offset;
                break;
        case SEEK_CUR:
        	buffer_data->current_pos += offset;
                break;
        case SEEK_END:
        	buffer_data->current_pos = buffer_data->buffer_size + offset;
                break;
        default:
                return -1;
        }

        return buffer_data->current_pos;
}


void
gth_buffer_data_get (GthBufferData  *buffer_data,
		     char          **buffer,
		     gsize          *buffer_size)
{
	*buffer = buffer_data->buffer;
	*buffer_size = buffer_data->buffer_size;
}
