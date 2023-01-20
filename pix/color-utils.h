/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2013 Free Software Foundation, Inc.
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

#ifndef COLOR_UTILS_H
#define COLOR_UTILS_H

#include <glib.h>

void gimp_rgb_to_hsv	(guchar  red,
			 guchar  green,
			 guchar  blue,
			 guchar *hue,
			 guchar *sat,
			 guchar *val);
void gimp_hsv_to_rgb	(guchar  hue,
			 guchar  sat,
			 guchar  val,
			 guchar *red,
			 guchar *green,
			 guchar *blue);
void gimp_rgb_to_hsl	(guchar  red,
			 guchar  green,
			 guchar  blue,
			 guchar *hue,
			 guchar *sat,
			 guchar *lum);
int  gimp_hsl_value	(gdouble n1,
			 gdouble n2,
			 gdouble hue);
void gimp_hsl_to_rgb	(guchar  hue,
			 guchar  sat,
			 guchar  lum,
			 guchar *red,
			 guchar *green,
			 guchar *blue);

void rgb_to_hsl	(guchar  red,
			 guchar  green,
			 guchar  blue,
			 double *hue,
			 double *sat,
			 double *lum);

#endif /* COLOR_UTILS_H */
