/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2014 The Free Software Foundation, Inc.
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

#ifndef CAIRO_EFFECTS_H
#define CAIRO_EFFECTS_H

#include <glib.h>
#include <cairo.h>
#include <gthumb.h>
#include "gth-curve.h"

G_BEGIN_DECLS

gboolean cairo_image_surface_apply_curves	(cairo_surface_t  *source,
						 GthCurve        **curve,
						 GthAsyncTask     *task);
gboolean cairo_image_surface_apply_vignette	(cairo_surface_t  *source,
						 GthCurve        **curve,
						 guchar	           vignette_alpha,
						 GthAsyncTask     *task);
gboolean cairo_image_surface_apply_bcs		(cairo_surface_t  *source,
						 double            brightness,
						 double            contrast,
						 double            saturation,
						 GthAsyncTask     *task);
gboolean cairo_image_surface_colorize		(cairo_surface_t  *source,
						 guchar            color_red,
						 guchar            color_green,
						 guchar            color_blue,
						 guchar            color_alpha,
						 GthAsyncTask     *task);
gboolean cairo_image_surface_add_color		(cairo_surface_t  *source,
						 guchar            color_red,
						 guchar            color_green,
						 guchar            color_blue,
						 guchar            color_alpha,
						 GthAsyncTask     *task);

G_END_DECLS

#endif /* CAIRO_EFFECTS_H */
