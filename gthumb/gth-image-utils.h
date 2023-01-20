/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2001-2012 The Free Software Foundation, Inc.
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

#ifndef GTH_IMAGE_UTILS_H
#define GTH_IMAGE_UTILS_H

#include <glib.h>
#include "gth-file-data.h"

G_BEGIN_DECLS

gboolean    scale_keeping_ratio_min              (int             *width,
					          int             *height,
					          int              min_width,
					          int              min_height,
					          int              max_width,
					          int              max_height,
					          gboolean         allow_upscaling);
gboolean    scale_keeping_ratio                  (int             *width,
					          int             *height,
					          int              max_width,
					          int              max_height,
					          gboolean         allow_upscaling);
gboolean    _g_buffer_resize_image               (void            *buffer,
						  gsize            count,
						  GthFileData     *file_data,
						  int              max_width,
						  int              max_height,
						  void           **resized_buffer,
						  gsize           *resized_count,
						  GCancellable    *cancellable,
						  GError         **error);

G_END_DECLS

#endif /* GTH_IMAGE_UTILS_H */
