/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2009 Free Software Foundation, Inc.
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

#ifndef PREFERENCES_H
#define PREFERENCES_H

#include <gthumb.h>


typedef enum {
	GTH_TIFF_COMPRESSION_NONE,
	GTH_TIFF_COMPRESSION_DEFLATE,
	GTH_TIFF_COMPRESSION_JPEG
} GthTiffCompression;


/* schemas */

#define GTHUMB_IMAGE_SAVERS               GTHUMB_SCHEMA ".pixbuf-savers"
#define GTHUMB_IMAGE_SAVERS_AVIF_SCHEMA   GTHUMB_IMAGE_SAVERS ".avif"
#define GTHUMB_IMAGE_SAVERS_JPEG_SCHEMA   GTHUMB_IMAGE_SAVERS ".jpeg"
#define GTHUMB_IMAGE_SAVERS_PNG_SCHEMA    GTHUMB_IMAGE_SAVERS ".png"
#define GTHUMB_IMAGE_SAVERS_TGA_SCHEMA    GTHUMB_IMAGE_SAVERS ".tga"
#define GTHUMB_IMAGE_SAVERS_TIFF_SCHEMA   GTHUMB_IMAGE_SAVERS ".tiff"
#define GTHUMB_IMAGE_SAVERS_WEBP_SCHEMA   GTHUMB_IMAGE_SAVERS ".webp"

/* keys: avif */

#define  PREF_AVIF_LOSSLESS               "lossless"
#define  PREF_AVIF_QUALITY                "quality"

/* keys: jpeg */

#define  PREF_JPEG_DEFAULT_EXT            "default-ext"
#define  PREF_JPEG_QUALITY                "quality"
#define  PREF_JPEG_SMOOTHING              "smoothing"
#define  PREF_JPEG_OPTIMIZE               "optimize"
#define  PREF_JPEG_PROGRESSIVE            "progressive"

/* keys: png */

#define  PREF_PNG_COMPRESSION_LEVEL       "compression-level"

/* keys: tga */

#define  PREF_TGA_RLE_COMPRESSION         "rle-compression"

/* keys: tiff */

#define  PREF_TIFF_DEFAULT_EXT            "default-ext"
#define  PREF_TIFF_COMPRESSION            "compression"
#define  PREF_TIFF_HORIZONTAL_RES         "horizontal-resolution"
#define  PREF_TIFF_VERTICAL_RES           "vertical-resolution"

/* keys: webp */

#define  PREF_WEBP_LOSSLESS               "lossless"
#define  PREF_WEBP_QUALITY                "quality"
#define  PREF_WEBP_METHOD                 "method"


void ci__dlg_preferences_construct_cb (GtkWidget  *dialog,
				       GthBrowser *browser,
				       GtkBuilder *builder);
void ci__dlg_preferences_apply_cb     (GtkWidget  *dialog,
				       GthBrowser *browser,
				       GtkBuilder *builder);

#endif /* PREFERENCES_H */
