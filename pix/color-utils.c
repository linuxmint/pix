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

#include <config.h>
#include "color-utils.h"
#include "glib-utils.h"


/* RGB <-> HSV */


void
gimp_rgb_to_hsv (guchar  red,
		 guchar  green,
		 guchar  blue,
		 guchar *hue,
		 guchar *sat,
		 guchar *val)
{
	guchar min, max;

	min = MIN3 (red, green, blue);
	max = MAX3 (red, green, blue);

	*val = max;
	if (*val == 0) {
		*hue = *sat = 0;
		return;
	}

	*sat = 255 * (long)(max - min) / *val;
	if (*sat == 0) {
		*hue = 0;
		return;
	}

	if (max == min)
		*hue = 0;
	else if (max == red)
		*hue = 0 + 43 * (green - blue) / (max - min);
	else if (max == green)
		*hue = 85 + 43 * (blue - red) / (max - min);
	else if (max == blue)
		*hue = 171 + 43 * (red - green) / (max - min);
}


void
gimp_hsv_to_rgb (guchar  hue,
		 guchar  sat,
		 guchar  val,
		 guchar *red,
		 guchar *green,
		 guchar *blue)
{
	guchar region, remainder, p, q, t;

	if (sat == 0) {
		*red = *green = *blue = val;
		return;
	}

	region = hue / 43;
	remainder = (hue - (region * 43)) * 6;

	p = (val * (255 - sat)) >> 8;
	q = (val * (255 - ((sat * remainder) >> 8))) >> 8;
	t = (val * (255 - ((sat * (255 - remainder)) >> 8))) >> 8;

	switch (region) {
	case 0:
		*red = val;
		*green = t;
		*blue = p;
		break;
	case 1:
		*red = q;
		*green = val;
		*blue = p;
		break;
	case 2:
		*red = p;
		*green = val;
		*blue = t;
		break;
	case 3:
		*red = p;
		*green = q;
		*blue = val;
		break;
	case 4:
		*red = t;
		*green = p;
		*blue = val;
		break;
	default:
		*red = val;
		*green = p;
		*blue = q;
		break;
	}
}


/* RGB <-> HSL */


void
rgb_to_hsl (guchar  red,
	    guchar  green,
	    guchar  blue,
	    double *hue,
	    double *sat,
	    double *lum)
{
	double min, max;

	min = MIN3 (red, green, blue);
	max = MAX3 (red, green, blue);

	*lum = (max + min) / 2.0;

	if (max == min) {
		*hue = *sat = 0;
		return;
	}

	if (*lum < 128)
		*sat = 255.0 * (max - min) / (max + min);
	else
		*sat = 255.0 * (max - min) / (512.0 - max - min);

	if (max == min)
		*hue = 0;
	else if (max == red)
		*hue = 0.0 + 43.0 * (double) (green - blue) / (max - min);
	else if (max == green)
		*hue = 85.0 + 43.0 * (double) (blue - red) / (max - min);
	else if (max == blue)
		*hue = 171.0 + 43.0 * (double) (red - green) / (max - min);
}


void
gimp_rgb_to_hsl (guchar  red,
		 guchar  green,
		 guchar  blue,
		 guchar *hue,
		 guchar *sat,
		 guchar *lum)
{
	guchar min, max;

	min = MIN3 (red, green, blue);
	max = MAX3 (red, green, blue);

	*lum = (max + min) / 2;

	if (max == min) {
		*hue = *sat = 0;
		return;
	}

	if (*lum < 128)
		*sat = 255 * (long) (max - min) / (max + min);
	else
		*sat = 255 * (long) (max - min) / (512 - max - min);

	if (max == min)
		*hue = 0;
	else if (max == red)
		*hue = 0 + 43 * (green - blue) / (max - min);
	else if (max == green)
		*hue = 85 + 43 * (blue - red) / (max - min);
	else if (max == blue)
		*hue = 171 + 43 * (red - green) / (max - min);
}


inline gint
gimp_hsl_value (gdouble n1,
                gdouble n2,
                gdouble hue)
{
	gdouble value;

	if (hue > 255)
		hue -= 255;
	else if (hue < 0)
		hue += 255;

	if (hue < 42.5)
		value = n1 + (n2 - n1) * (hue / 42.5);
	else if (hue < 127.5)
		value = n2;
	else if (hue < 170)
		value = n1 + (n2 - n1) * ((170 - hue) / 42.5);
	else
		value = n1;

	return value * 255.0;
}


void
gimp_hsl_to_rgb (guchar  hue,
		 guchar  sat,
		 guchar  lum,
		 guchar *red,
		 guchar *green,
		 guchar *blue)
{
	if (sat == 0) {
		*red = lum;
		*green = lum;
		*blue = lum;
	}
	else {
		gdouble h, s, l, m1, m2;

		h = hue;
		s = sat;
		l = lum;

		if (l < 128)
		        m2 = (l * (255 + s)) / 65025.0;
		else
			m2 = (l + s - (l * s) / 255.0) / 255.0;

		m1 = (l / 127.5) - m2;

		*red = gimp_hsl_value (m1, m2, h + 85);
		*green = gimp_hsl_value (m1, m2, h);
		*blue  = gimp_hsl_value (m1, m2, h - 85);
	}
}
