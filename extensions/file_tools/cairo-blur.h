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

#ifndef CAIRO_BLUR_H
#define CAIRO_BLUR_H

#include <glib.h>
#include <cairo.h>
#include <gthumb.h>

G_BEGIN_DECLS

gboolean _cairo_image_surface_blur     (cairo_surface_t *source,
	 	                        int              radius,
	 	                        GthAsyncTask    *task);
gboolean _cairo_image_surface_sharpen  (cairo_surface_t *source,
				        int              radius,
				        double           amount,
				        guchar           threshold,
				        GthAsyncTask    *task);

G_END_DECLS

#endif /* CAIRO_BLUR_H */
