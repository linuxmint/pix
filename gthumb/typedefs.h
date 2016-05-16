/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2001-2008 The Free Software Foundation, Inc.
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

#ifndef TYPEDEFS_H
#define TYPEDEFS_H

#include <gtk/gtk.h>

G_BEGIN_DECLS


#define BOOKMARKS_FILE "bookmarks.xbel"
#define FILTERS_FILE   "filters.xml"
#define TAGS_FILE      "tags.xml"
#define FILE_CACHE     "cache"


typedef enum {
	GTH_CLICK_POLICY_NAUTILUS,
	GTH_CLICK_POLICY_SINGLE,
	GTH_CLICK_POLICY_DOUBLE
} GthClickPolicy;


typedef enum {
	GTH_DIRECTION_FORWARD,
	GTH_DIRECTION_REVERSE,
	GTH_DIRECTION_RANDOM
} GthDirection;


typedef enum {
	GTH_TOOLBAR_STYLE_SYSTEM,
	GTH_TOOLBAR_STYLE_TEXT_BELOW,
	GTH_TOOLBAR_STYLE_TEXT_BESIDE,
	GTH_TOOLBAR_STYLE_ICONS,
	GTH_TOOLBAR_STYLE_TEXT
} GthToolbarStyle;


/* The GthTransform numeric values range from 1 to 8, corresponding to
 * the valid range of Exif orientation tags.  The name associated with each
 * numeric valid describes the data transformation required that will allow
 * the orientation value to be reset to "1" without changing the displayed
 * image.
 * GthTransform and ExifShort values are interchangeably in a number of
 * places.  The main difference is that ExifShort can have a value of zero,
 * corresponding to an error or an absence of an Exif orientation tag.
 * See bug 361913 for additional details. */
typedef enum {
	GTH_TRANSFORM_NONE = 1,         /* no transformation */
	GTH_TRANSFORM_FLIP_H,           /* horizontal flip */
	GTH_TRANSFORM_ROTATE_180,       /* 180-degree rotation */
	GTH_TRANSFORM_FLIP_V,           /* vertical flip */
	GTH_TRANSFORM_TRANSPOSE,        /* transpose across UL-to-LR axis (= rotate_90 + flip_h) */
	GTH_TRANSFORM_ROTATE_90,        /* 90-degree clockwise rotation */
	GTH_TRANSFORM_TRANSVERSE,       /* transpose across UR-to-LL axis (= rotate_90 + flip_v) */
	GTH_TRANSFORM_ROTATE_270        /* 270-degree clockwise */
} GthTransform;


typedef enum {
	GTH_UNIT_PIXELS,
        GTH_UNIT_PERCENTAGE
} GthUnit;


typedef enum {
	GTH_METRIC_PIXELS,
        GTH_METRIC_MILLIMETERS,
        GTH_METRIC_INCHES
} GthMetric;


typedef enum {
	GTH_ASPECT_RATIO_NONE = 0,
	GTH_ASPECT_RATIO_SQUARE,
	GTH_ASPECT_RATIO_IMAGE,
	GTH_ASPECT_RATIO_DISPLAY,
	GTH_ASPECT_RATIO_5x4,
	GTH_ASPECT_RATIO_4x3,
	GTH_ASPECT_RATIO_7x5,
	GTH_ASPECT_RATIO_3x2,
	GTH_ASPECT_RATIO_16x10,
	GTH_ASPECT_RATIO_16x9,
	GTH_ASPECT_RATIO_185x100,
	GTH_ASPECT_RATIO_239x100,
	GTH_ASPECT_RATIO_CUSTOM
} GthAspectRatio;


typedef enum {
	GTH_OVERWRITE_SKIP,
	GTH_OVERWRITE_RENAME,
	GTH_OVERWRITE_ASK,
	GTH_OVERWRITE_OVERWRITE
} GthOverwriteMode;


typedef enum {
	GTH_GRID_NONE = 0,
	GTH_GRID_THIRDS,
	GTH_GRID_GOLDEN,
	GTH_GRID_CENTER,
	GTH_GRID_UNIFORM
} GthGridType;


typedef enum /*< skip >*/ {
        GTH_CHANNEL_RED   = 0,
        GTH_CHANNEL_GREEN = 1,
        GTH_CHANNEL_BLUE  = 2,
        GTH_CHANNEL_ALPHA = 3
} GthChannel;


typedef void (*DataFunc)         (gpointer    user_data);
typedef void (*ReadyFunc)        (GError     *error,
			 	  gpointer    user_data);
typedef void (*ReadyCallback)    (GObject    *object,
				  GError     *error,
			   	  gpointer    user_data);
typedef void (*ProgressCallback) (GObject    *object,
				  const char *description,
				  const char *details,
			          gboolean    pulse,
			          double      fraction,
			   	  gpointer    user_data);
typedef void (*DialogCallback)   (gboolean    opened,
				  GtkWidget  *dialog,
				  gpointer    user_data);


G_END_DECLS

#endif /* TYPEDEFS_H */
