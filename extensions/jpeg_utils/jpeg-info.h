/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2001-2015 The Free Software Foundation, Inc.
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

typedef enum /*< skip >*/ {
	_JPEG_INFO_NONE = 0,
	_JPEG_INFO_IMAGE_SIZE = 1 << 0,
	_JPEG_INFO_ICC_PROFILE = 1 << 1,
	_JPEG_INFO_EXIF_ORIENTATION = 1 << 2,
	_JPEG_INFO_EXIF_COLORIMETRY = 1 << 3,
	_JPEG_INFO_EXIF_COLOR_SPACE = 1 << 4,
} JpegInfoFlags;

typedef struct {
	JpegInfoFlags	valid;
	int		width;
	int		height;
	GthTransform	orientation;
	gpointer	icc_data;
	gsize		icc_data_size;
	GthColorSpace   color_space;
} JpegInfoData;

void		_jpeg_info_data_init			(JpegInfoData		 *data);
void		_jpeg_info_data_dispose			(JpegInfoData		 *data);
gboolean	_jpeg_info_get_from_stream		(GInputStream		 *stream,
							 JpegInfoFlags		  flags,
							 JpegInfoData		 *data,
							 GCancellable		 *cancellable,
							 GError			**error);
gboolean	_jpeg_info_get_from_buffer		(guchar			 *in_buffer,
							 gsize			  in_buffer_size,
							 JpegInfoFlags		  flags,
							 JpegInfoData		 *data);

#endif /* JPEG_INFO_H */
