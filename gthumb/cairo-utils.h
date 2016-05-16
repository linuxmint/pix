/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2010 Free Software Foundation, Inc.
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

#ifndef CAIRO_UTILS_H
#define CAIRO_UTILS_H

#include <glib.h>
#include <gdk/gdk.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <cairo.h>
#include "typedefs.h"


#define CAIRO_MAX_IMAGE_SIZE 32767

#if G_BYTE_ORDER == G_LITTLE_ENDIAN /* BGRA */

#define CAIRO_RED   2
#define CAIRO_GREEN 1
#define CAIRO_BLUE  0
#define CAIRO_ALPHA 3

#elif G_BYTE_ORDER == G_BIG_ENDIAN /* ARGB */

#define CAIRO_RED   1
#define CAIRO_GREEN 2
#define CAIRO_BLUE  3
#define CAIRO_ALPHA 0

#else /* PDP endianness: RABG */

#define CAIRO_RED   0
#define CAIRO_GREEN 3
#define CAIRO_BLUE  2
#define CAIRO_ALPHA 1

#endif

#define CAIRO_SET_RGB(pixel, red, green, blue)					\
	G_STMT_START {								\
		pixel[CAIRO_RED] = (red);					\
		pixel[CAIRO_GREEN] = (green);					\
		pixel[CAIRO_BLUE] = (blue);					\
		pixel[CAIRO_ALPHA] = 0xff;					\
	} G_STMT_END

#define CAIRO_SET_RGBA(pixel, red, green, blue, alpha)				\
	G_STMT_START {								\
		pixel[CAIRO_ALPHA] = (alpha);					\
		if (pixel[CAIRO_ALPHA] == 0xff) {				\
			pixel[CAIRO_RED] = (red);				\
			pixel[CAIRO_GREEN] = (green);				\
			pixel[CAIRO_BLUE] = (blue);				\
		}								\
		else {								\
			double factor = (double) pixel[CAIRO_ALPHA] / 0xff;	\
			pixel[CAIRO_RED] = factor * (red);			\
			pixel[CAIRO_GREEN] = factor * (green);			\
			pixel[CAIRO_BLUE] = factor * (blue);			\
		}								\
	} G_STMT_END

#define CAIRO_GET_RGB(pixel, red, green, blue)					\
	G_STMT_START {								\
		red = pixel[CAIRO_RED];						\
		green = pixel[CAIRO_GREEN];					\
		blue = pixel[CAIRO_BLUE];					\
	} G_STMT_END

#define CAIRO_GET_RGBA(pixel, red, green, blue, alpha)				\
	G_STMT_START {								\
		alpha = pixel[CAIRO_ALPHA];					\
		if (alpha == 0xff) {						\
			red = pixel[CAIRO_RED];					\
			green = pixel[CAIRO_GREEN];				\
			blue = pixel[CAIRO_BLUE];				\
		}								\
		else {								\
			double factor = (double) 0xff / alpha;			\
			red = factor * pixel[CAIRO_RED];			\
			green = factor * pixel[CAIRO_GREEN];			\
			blue = factor * pixel[CAIRO_BLUE];			\
		}								\
	} G_STMT_END

#define CAIRO_COPY_RGBA(pixel, red, green, blue, alpha)	\
	G_STMT_START {					\
		alpha = pixel[CAIRO_ALPHA];		\
		red = pixel[CAIRO_RED];			\
		green = pixel[CAIRO_GREEN];		\
		blue = pixel[CAIRO_BLUE];		\
	} G_STMT_END

#define CAIRO_RGBA_TO_UINT32(red, green, blue, alpha) 				\
	(((alpha) << 24) | ((red) << 16) | ((green) << 8) | (blue))

#define interpolate_value(original, reference, distance) 			\
	(((distance) * (reference)) + ((1.0 - (distance)) * (original)))


/* types */

typedef struct {
	guchar r;
	guchar g;
	guchar b;
	guchar a;
} cairo_color_255_t;

typedef struct {
	gboolean has_alpha;
} cairo_surface_metadata_t;

extern const unsigned char cairo_channel[4];

/* math */

int                _cairo_multiply_alpha                    (int                    color,
							     int                    alpha);
gboolean           _cairo_rectangle_contains_point          (cairo_rectangle_int_t *rect,
			 	 	 	 	     int                    x,
			 	 	 	 	     int                    y);

/* colors */

void               _gdk_color_to_cairo_color                (GdkColor              *g_color,
							     GdkRGBA               *c_color);
void               _gdk_color_to_cairo_color_255            (GdkColor              *g_color,
			         	 	 	     cairo_color_255_t     *c_color);
void               _gdk_rgba_to_cairo_color_255             (GdkRGBA               *g_color,
			         	 	 	     cairo_color_255_t     *c_color);

/* surface */

void               _cairo_clear_surface                     (cairo_surface_t      **surface);
cairo_surface_metadata_t *
		   _cairo_image_surface_get_metadata        (cairo_surface_t       *surface);
gboolean           _cairo_image_surface_get_has_alpha       (cairo_surface_t       *surface);
cairo_surface_t *  _cairo_image_surface_copy                (cairo_surface_t       *surface);
cairo_surface_t *  _cairo_image_surface_copy_subsurface     (cairo_surface_t       *surface,
				      	      	      	     int                    src_x,
				      	      	      	     int                    src_y,
				      	      	      	     int                    width,
				      	      	      	     int                    height);
cairo_surface_t *  _cairo_image_surface_create_from_pixbuf  (GdkPixbuf             *pixbuf);
cairo_surface_t *  _cairo_image_surface_create_compatible   (cairo_surface_t       *surface);
void               _cairo_image_surface_transform_get_steps (cairo_format_t         format,
							     int                    width,
							     int                    height,
							     GthTransform           transform,
							     int                   *destination_width_p,
							     int                   *destination_height_p,
							     int                   *line_start_p,
							     int                   *line_step_p,
							     int                   *pixel_step_p);
cairo_surface_t *  _cairo_image_surface_transform           (cairo_surface_t       *image,
							     GthTransform           transform);
cairo_surface_t *  _cairo_image_surface_color_shift         (cairo_surface_t       *image,
							     int                    shift);
void               _cairo_copy_line_as_rgba_big_endian      (guchar                *dest,
							     guchar                *src,
							     guint                  width,
							     guint                  alpha);
void               _cairo_copy_line_as_rgba_little_endian   (guchar                *dest,
							     guchar                *src,
							     guint                  width,
							     guint                  alpha);

/* paint / draw */

void              _cairo_paint_full_gradient                (cairo_surface_t       *surface,
							     GdkRGBA               *h_color1,
							     GdkRGBA               *h_color2,
							     GdkRGBA               *v_color1,
							     GdkRGBA               *v_color2);
void              _cairo_draw_rounded_box                   (cairo_t               *cr,
			 	 	 	 	     double                 x,
			 	 	 	 	     double                 y,
			 	 	 	 	     double                 w,
			 	 	 	 	     double                 h,
			 	 	 	 	     double                 r);
void              _cairo_draw_drop_shadow                   (cairo_t               *cr,
 	 	 	 	 	 	 	     double                 x,
 	 	 	 	 	 	 	     double                 y,
 	 	 	 	 	 	 	     double                 w,
 	 	 	 	 	 	 	     double                 h,
 	 	 	 	 	 	 	     double                 r);
void              _cairo_draw_frame                         (cairo_t               *cr,
 	 	 	 	 	 	 	     double                 x,
 	 	 	 	 	 	 	     double                 y,
 	 	 	 	 	 	 	     double                 w,
 	 	 	 	 	 	 	     double                 h,
 	 	 	 	 	 	 	     double                 r);
void              _cairo_draw_slide                         (cairo_t               *cr,
		   	   	   	   	 	     double                 frame_x,
		   	   	   	   	 	     double                 frame_y,
		   	   	   	   	 	     double                 frame_width,
		   	   	   	   	 	     double                 frame_height,
		   	   	   	   	 	     double                 image_width,
		   	   	   	   	 	     double                 image_height,
		   	   	   	   	 	     GdkRGBA               *frame_color,
		   	   	   	   	 	     gboolean               draw_inner_border);
void              _cairo_paint_grid                         (cairo_t               *cr,
							     cairo_rectangle_int_t *rectangle,
							     GthGridType            grid_type);

#endif /* CAIRO_UTILS_H */
