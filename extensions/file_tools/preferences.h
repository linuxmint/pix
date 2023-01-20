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

#ifndef FILE_TOOLS_PREFERENCES_H
#define FILE_TOOLS_PREFERENCES_H

#include <gthumb.h>

G_BEGIN_DECLS

/* schemas */

#define GTHUMB_CROP_SCHEMA              GTHUMB_SCHEMA ".crop"
#define GTHUMB_RESIZE_SCHEMA            GTHUMB_SCHEMA ".resize"
#define GTHUMB_ROTATE_SCHEMA            GTHUMB_SCHEMA ".rotate"

/* keys: crop */

#define PREF_CROP_GRID_TYPE             "grid-type"
#define PREF_CROP_ASPECT_RATIO          "aspect-ratio"
#define PREF_CROP_ASPECT_RATIO_INVERT   "aspect-ratio-invert"
#define PREF_CROP_ASPECT_RATIO_WIDTH    "aspect-ratio-width"
#define PREF_CROP_ASPECT_RATIO_HEIGHT   "aspect-ratio-height"
#define PREF_CROP_BIND_DIMENSIONS       "bind-dimensions"
#define PREF_CROP_BIND_FACTOR           "bind-factor"

/* keys: resize */

#define PREF_RESIZE_UNIT                "unit"
#define PREF_RESIZE_WIDTH               "width"
#define PREF_RESIZE_HEIGHT              "height"
#define PREF_RESIZE_ASPECT_RATIO_WIDTH  "aspect-ratio-width"
#define PREF_RESIZE_ASPECT_RATIO_HEIGHT "aspect-ratio-height"
#define PREF_RESIZE_ASPECT_RATIO        "aspect-ratio"
#define PREF_RESIZE_ASPECT_RATIO_INVERT "aspect-ratio-invert"
#define PREF_RESIZE_HIGH_QUALITY        "high-quality"

/* keys: rotate */

#define PREF_ROTATE_RESIZE              "resize"
#define PREF_ROTATE_KEEP_ASPECT_RATIO   "keep-aspect-ratio"
#define PREF_ROTATE_GRID_TYPE           "grid-type"
#define PREF_ROTATE_BACKGROUND_COLOR    "background-color"

G_END_DECLS

#endif /* FILE_TOOLS_PREFERENCES_H */
