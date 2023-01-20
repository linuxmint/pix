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

#ifndef GTH_BUFFER_DATA_H
#define GTH_BUFFER_DATA_H

#include <glib.h>

G_BEGIN_DECLS

typedef struct _GthBufferData GthBufferData;

GthBufferData * gth_buffer_data_new   (void);
void            gth_buffer_data_free  (GthBufferData  *buffer_data,
				       gboolean        free_segment);
gboolean        gth_buffer_data_write (GthBufferData  *buffer_data,
				       const void     *buffer,
				       gsize           len,
				       GError        **error);
gboolean        gth_buffer_data_putc  (GthBufferData  *buffer_data,
				       int             c,
				       GError        **error);
goffset         gth_buffer_data_seek  (GthBufferData  *buffer_data,
				       goffset         offset,
				       int             whence);
void            gth_buffer_data_get   (GthBufferData  *buffer_data,
				       char          **buffer,
				       gsize          *buffer_size);

G_END_DECLS

#endif /* GTH_BUFFER_DATA_H */
