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

#ifndef GIMP_OP_H
#define GIMP_OP_H

#include <config.h>
#include <glib.h>
#include "cairo-utils.h"

/* Optimizations taken from xcftools 1.0.7 written by Henning Makholm
 *
 * xL : Layer color
 * xI : Image color
 * aL : Layer alpha
 * */

#define ADD_ALPHA(v, a)			(add_alpha_table[v][a])
#define ABS_TEMP2(x)			(temp2 = (x), (temp2 < 0) ? -temp2: temp2)
#define GIMP_OP_NORMAL(xL, xI, aL)	CLAMP_PIXEL (ADD_ALPHA (xL, aL) + ADD_ALPHA (xI, 255 - aL))
#define GIMP_OP_LIGHTEN_ONLY(xL, xI)	MAX (xI, xL)
#define GIMP_OP_SCREEN(xL, xI)		CLAMP_PIXEL (255 ^ ADD_ALPHA (255 - xI, 255 - xL))
#define GIMP_OP_DODGE(xL, xI)		GIMP_OP_DIVIDE (255-xL, xI)
#define GIMP_OP_ADDITION(xL, xI)	CLAMP_PIXEL (xI + xL)
#define GIMP_OP_DARKEN_ONLY(xL, xI)	MIN (xI, xL)
#define GIMP_OP_MULTIPLY(xL, xI)	CLAMP_PIXEL (ADD_ALPHA (xL, xI))
#define GIMP_OP_BURN(xL, xI)		CLAMP_PIXEL (255 - GIMP_OP_DIVIDE (xL, 255 - xI))
#define GIMP_OP_SOFT_LIGHT(xL, xI)	CLAMP_PIXEL (ADD_ALPHA (xI, xI) + 2 * ADD_ALPHA (xL, ADD_ALPHA (xI, 255 - xI)))
#define GIMP_OP_HARD_LIGHT(xL, xI)	CLAMP_PIXEL (xL > 128 ? 255 ^ ADD_ALPHA (255 - xI, 2 * (255 - xL)) : ADD_ALPHA (xI, 2 * xL))
#define GIMP_OP_DIFFERENCE(xL, xI)	CLAMP_PIXEL (ABS_TEMP2 (xI - xL))
#define GIMP_OP_SUBTRACT(xL, xI)	CLAMP_PIXEL (xI - xL)
#define GIMP_OP_GRAIN_EXTRACT(xL, xI)	CLAMP_PIXEL ((int) xI - xL + 128)
#define GIMP_OP_GRAIN_MERGE(xL, xI)	CLAMP_PIXEL ((int) xI + xL - 128)
#define GIMP_OP_DIVIDE(xL, xI)		CLAMP_PIXEL ((int) (xI) * 256 / (1 + (xL)))

extern guchar add_alpha_table[256][256];

void gimp_op_init (void);

#endif /* GIMP_OP_H */
