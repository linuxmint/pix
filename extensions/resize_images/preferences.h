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

G_BEGIN_DECLS

/* schema */

#define GTHUMB_RESIZE_IMAGES_SCHEMA        GTHUMB_SCHEMA ".resize-images"

/* keys */

#define  PREF_RESIZE_IMAGES_SERIES_WIDTH   "width"
#define  PREF_RESIZE_IMAGES_SERIES_HEIGHT  "height"
#define  PREF_RESIZE_IMAGES_UNIT           "unit"
#define  PREF_RESIZE_IMAGES_KEEP_RATIO     "keep-aspect-ratio"
#define  PREF_RESIZE_IMAGES_MIME_TYPE      "mime-type"

G_END_DECLS

#endif /* PREFERENCES_H */
