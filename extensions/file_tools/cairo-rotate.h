/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2011 The Free Software Foundation, Inc.
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

#ifndef CAIRO_ROTATE_H
#define CAIRO_ROTATE_H

#include <glib.h>
#include <gdk/gdk.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

G_BEGIN_DECLS

void               _cairo_image_surface_rotate_get_cropping_parameters (cairo_surface_t       *image,
					   				double                 angle,
					   				double                *p1_plus_p2,
					   				double                *p_min);
void               _cairo_image_surface_rotate_get_cropping_region     (cairo_surface_t       *image,
									double                 angle,
									double                 p1,
									double                 p2,
									cairo_rectangle_int_t *region);
double             _cairo_image_surface_rotate_get_align_angle         (gboolean               vertical,
					   	   	  	  	GdkPoint              *p1,
					   	   	  	  	GdkPoint              *p2);
cairo_surface_t *  _cairo_image_surface_rotate                         (cairo_surface_t       *image,
		    	     	     	     	     	     	        double                 angle,
		    	     	     	     	     	     	        gboolean               high_quality,
		    	     	     	     	     	     	        GdkRGBA               *background_color,
		    	     	     	     	     	     	        GthAsyncTask          *task);

G_END_DECLS

#endif /* CAIRO_ROTATE_H */
