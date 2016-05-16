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

#ifndef PREFERENCES_H
#define PREFERENCES_H

#include <gthumb.h>

G_BEGIN_DECLS

/* schemas */

#define GTHUMB_WEBALBUMS_SCHEMA                 GTHUMB_SCHEMA ".webalbums"
#define GTHUMB_WEBALBUMS_DIRECTORIES_SCHEMA     GTHUMB_WEBALBUMS_SCHEMA ".directories"

/* keys: webalbums */

#define PREF_WEBALBUMS_INDEX_FILE               "index-file"
#define PREF_WEBALBUMS_DESTINATION              "destination"
#define PREF_WEBALBUMS_COPY_IMAGES              "copy-images"
#define PREF_WEBALBUMS_RESIZE_IMAGES            "resize-images"
#define PREF_WEBALBUMS_RESIZE_WIDTH             "resize-width"
#define PREF_WEBALBUMS_RESIZE_HEIGHT            "resize-height"
#define PREF_WEBALBUMS_IMAGES_PER_INDEX         "images-per-index"
#define PREF_WEBALBUMS_SINGLE_INDEX             "single-index"
#define PREF_WEBALBUMS_COLUMNS                  "columns"
#define PREF_WEBALBUMS_ADAPT_TO_WIDTH           "adapt-to-width"
#define PREF_WEBALBUMS_SORT_TYPE                "sort-type"
#define PREF_WEBALBUMS_SORT_INVERSE             "sort-inverse"
#define PREF_WEBALBUMS_HEADER                   "header"
#define PREF_WEBALBUMS_FOOTER                   "footer"
#define PREF_WEBALBUMS_IMAGE_PAGE_HEADER        "image-page-header"
#define PREF_WEBALBUMS_IMAGE_PAGE_FOOTER        "image-page-footer"
#define PREF_WEBALBUMS_THEME                    "theme"
#define PREF_WEBALBUMS_ENABLE_THUMBNAIL_CAPTION "enable-thumbnail-caption"
#define PREF_WEBALBUMS_THUMBNAIL_CAPTION        "thumbnail-caption"
#define PREF_WEBALBUMS_ENABLE_IMAGE_DESCRIPTION "enable-image-description"
#define PREF_WEBALBUMS_ENABLE_IMAGE_ATTRIBUTES  "enable-image-attributes"
#define PREF_WEBALBUMS_IMAGE_ATTRIBUTES         "image-attributes"

/* keys: webalbums directories */

#define PREF_WEBALBUMS_DIR_PREVIEWS             "previews"
#define PREF_WEBALBUMS_DIR_THUMBNAILS           "thumbnails"
#define PREF_WEBALBUMS_DIR_IMAGES               "images"
#define PREF_WEBALBUMS_DIR_HTML_IMAGES          "html-images"
#define PREF_WEBALBUMS_DIR_HTML_INDEXES         "html-indexes"
#define PREF_WEBALBUMS_DIR_THEME_FILES          "theme-files"

G_END_DECLS

#endif /* PREFERENCES_H */
