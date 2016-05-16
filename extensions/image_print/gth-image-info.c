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
#include "gth-image-info.h"


#define THUMBNAIL_SIZE 256


static void
gth_rectangle_init (GthRectangle *rect)
{
	rect->x = 0.0;
	rect->y = 0.0;
	rect->width = 0.0;
	rect->height = 0.0;
}


GthImageInfo *
gth_image_info_new (GthFileData *file_data)
{
	GthImageInfo *image_info;

	image_info = g_new0 (GthImageInfo, 1);
	image_info->ref_count = 1;
	image_info->file_data = g_object_ref (file_data);
	image_info->image = NULL;
	image_info->thumbnail_original = NULL;
	image_info->thumbnail = NULL;
	image_info->thumbnail_active = NULL;
	image_info->rotation = GTH_TRANSFORM_NONE;
	image_info->zoom = 1.0;
	image_info->transformation.x = 0.0;
	image_info->transformation.y = 0.0;
	image_info->print_comment = FALSE;
	image_info->page = -1;
	image_info->active = FALSE;
	image_info->reset = TRUE;
	gth_rectangle_init (&image_info->boundary_box);
	gth_rectangle_init (&image_info->maximized_box);
	gth_rectangle_init (&image_info->image_box);
	gth_rectangle_init (&image_info->comment_box);

	return image_info;
}


GthImageInfo *
gth_image_info_ref (GthImageInfo *image_info)
{
	image_info->ref_count++;
	return image_info;
}


void
gth_image_info_unref (GthImageInfo *image_info)
{
	if (image_info == NULL)
		return;

	image_info->ref_count--;
	if (image_info->ref_count > 0)
		return;

	_g_object_unref (image_info->file_data);
	cairo_surface_destroy (image_info->image);
	cairo_surface_destroy (image_info->thumbnail_original);
	cairo_surface_destroy (image_info->thumbnail);
	cairo_surface_destroy (image_info->thumbnail_active);
	g_free (image_info->comment_text);
	g_free (image_info);
}


void
gth_image_info_set_image  (GthImageInfo    *image_info,
			   cairo_surface_t *image)
{
	int thumb_w;
	int thumb_h;

	g_return_if_fail (image != NULL);

	_cairo_clear_surface (&image_info->image);
	_cairo_clear_surface (&image_info->thumbnail_original);
	_cairo_clear_surface (&image_info->thumbnail);
	_cairo_clear_surface (&image_info->thumbnail_active);

	image_info->image = cairo_surface_reference (image);
	thumb_w = image_info->original_width = image_info->image_width = cairo_image_surface_get_width (image);
	thumb_h = image_info->original_height = image_info->image_height = cairo_image_surface_get_height (image);
	if (scale_keeping_ratio (&thumb_w, &thumb_h, THUMBNAIL_SIZE, THUMBNAIL_SIZE, FALSE))
		image_info->thumbnail_original = _cairo_image_surface_scale (image,
								 	     thumb_w,
								 	     thumb_h,
								 	     SCALE_FILTER_BEST,
								 	     NULL);
	else
		image_info->thumbnail_original = cairo_surface_reference (image_info->image);

	image_info->thumbnail = cairo_surface_reference (image_info->thumbnail_original);
	image_info->thumbnail_active = _cairo_image_surface_color_shift (image_info->thumbnail, 30);
}


void
gth_image_info_reset (GthImageInfo *image_info)
{
	image_info->reset = TRUE;
}


void
gth_image_info_rotate (GthImageInfo *image_info,
		       int           angle)
{
	angle = angle % 360;
	image_info->rotation = GTH_TRANSFORM_NONE;
	switch (angle) {
	case 90:
		image_info->rotation = GTH_TRANSFORM_ROTATE_90;
		break;
	case 180:
		image_info->rotation = GTH_TRANSFORM_ROTATE_180;
		break;
	case 270:
		image_info->rotation = GTH_TRANSFORM_ROTATE_270;
		break;
	default:
		break;
	}

	_cairo_clear_surface (&image_info->thumbnail);
	if (image_info->thumbnail_original != NULL)
		image_info->thumbnail = _cairo_image_surface_transform (image_info->thumbnail_original, image_info->rotation);

	_cairo_clear_surface (&image_info->thumbnail_active);
	if (image_info->thumbnail != NULL)
		image_info->thumbnail_active = _cairo_image_surface_color_shift (image_info->thumbnail, 30);

	if ((angle == 90) || (angle == 270)) {
		image_info->image_width = image_info->original_height;
		image_info->image_height = image_info->original_width;
	}
	else {
		image_info->image_width = image_info->original_width;
		image_info->image_height = image_info->original_height;
	}
}
