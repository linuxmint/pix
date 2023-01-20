/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2012 The Free Software Foundation, Inc.
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

#ifndef CAIRO_SCALE_H
#define CAIRO_SCALE_H

#include <glib.h>
#include "gth-async-task.h"

G_BEGIN_DECLS


#define _CAIRO_MAX_SCALE_FACTOR 1.33


typedef enum /*< skip >*/ {
	SCALE_FILTER_POINT,
	SCALE_FILTER_BOX,
	SCALE_FILTER_TRIANGLE,
	SCALE_FILTER_CUBIC,
	SCALE_FILTER_LANCZOS2,
	SCALE_FILTER_LANCZOS3,
	SCALE_FILTER_MITCHELL_NETRAVALI,
	N_SCALE_FILTERS,

	SCALE_FILTER_FAST = SCALE_FILTER_POINT,
	SCALE_FILTER_BEST = SCALE_FILTER_LANCZOS3,
	SCALE_FILTER_GOOD = SCALE_FILTER_TRIANGLE
} scale_filter_t;


cairo_surface_t *  _cairo_image_surface_scale_nearest   (cairo_surface_t 	 *image,
							 int              	  new_width,
							 int              	  new_height);
cairo_surface_t *  _cairo_image_surface_scale_fast      (cairo_surface_t 	 *image,
							 int              	  new_width,
							 int              	  new_height);
cairo_surface_t *  _cairo_image_surface_scale		(cairo_surface_t 	 *image,
							 int              	  width,
							 int              	  height,
							 scale_filter_t   	  quality,
							 GthAsyncTask    	 *task);
cairo_surface_t *  _cairo_image_surface_scale_squared   (cairo_surface_t 	 *image,
							 int              	  size,
							 scale_filter_t   	  quality,
							 GthAsyncTask    	 *task);
void               _cairo_image_surface_scale_async     (cairo_surface_t 	 *image,
							 int		 	  new_width,
							 int		  	  new_height,
							 scale_filter_t   	  quality,
							 GCancellable    	 *cancellable,
							 GAsyncReadyCallback	  ready_callback,
							 gpointer		  user_data);
cairo_surface_t *  _cairo_image_surface_scale_finish	(GAsyncResult		 *result,
							 GError			**error);

G_END_DECLS

#endif /* CAIRO_SCALE_H */
