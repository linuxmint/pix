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

#ifndef JPEG_INFO_H
#define JPEG_INFO_H

#include <gthumb.h>

#define JPEG_SEGMENT_MAX_SIZE (64 * 1024)

gboolean      _jpeg_get_image_info                     (GInputStream      *stream,
		      	      	      	       	        int               *width,
		      	      	      	       	        int               *height,
		      	      	      	       	        GthTransform      *orientation,
		      	      	      	       	        GCancellable      *cancellable,
		      	      	      	       	        GError           **error);
GthTransform  _jpeg_exif_orientation                   (guchar            *in_buffer,
				      	      	        gsize              in_buffer_size);
GthTransform  _jpeg_exif_orientation_from_stream       (GInputStream      *stream,
				    	    	        GCancellable      *cancellable,
				    	    	        GError           **error);

#endif /* JPEG_INFO_H */
