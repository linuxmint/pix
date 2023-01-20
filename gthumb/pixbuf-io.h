/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2001-2009 The Free Software Foundation, Inc.
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

#ifndef PIXBUF_IO_H
#define PIXBUF_IO_H

#include <glib.h>
#include <gdk/gdk.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include "gth-file-data.h"
#include "typedefs.h"

G_BEGIN_DECLS

GthImage  * gth_pixbuf_new_from_file           (GInputStream     *istream,
						GthFileData      *file,
						int               requested_size,
						int              *original_width,
						int              *original_height,
						gboolean         *loaded_original,
						gboolean          scale_to_original,
						GCancellable     *cancellable,
						GError          **error);
GthImage *  gth_pixbuf_animation_new_from_file (GInputStream     *istream,
						GthFileData      *file_data,
						int               requested_size,
						int              *original_width,
						int              *original_height,
						gboolean         *loaded_original,
						gpointer          user_data,
						GCancellable     *cancellable,
						GError          **error);

G_END_DECLS

#endif /* PIXBUF_IO_H */
