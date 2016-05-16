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

#define GTHUMB_CONTACT_SHEET_SCHEMA                 GTHUMB_SCHEMA ".contact-sheet.contact-sheet"
#define GTHUMB_IMAGE_WALL_SCHEMA                    GTHUMB_SCHEMA ".contact-sheet.image-wall"

/* keys: contact sheet */

#define PREF_CONTACT_SHEET_HEADER                   "header"
#define PREF_CONTACT_SHEET_FOOTER                   "footer"
#define PREF_CONTACT_SHEET_DESTINATION              "destination"
#define PREF_CONTACT_SHEET_TEMPLATE                 "template"
#define PREF_CONTACT_SHEET_MIME_TYPE                "mime-type"
#define PREF_CONTACT_SHEET_HTML_IMAGE_MAP           "html-image-map"
#define PREF_CONTACT_SHEET_THEME                    "theme"
#define PREF_CONTACT_SHEET_IMAGES_PER_PAGE          "images-per-page"
#define PREF_CONTACT_SHEET_SINGLE_PAGE              "single-page"
#define PREF_CONTACT_SHEET_COLUMNS                  "columns"
#define PREF_CONTACT_SHEET_SORT_TYPE                "sort-type"
#define PREF_CONTACT_SHEET_SORT_INVERSE             "sort-inverse"
#define PREF_CONTACT_SHEET_SAME_SIZE                "same-size"
#define PREF_CONTACT_SHEET_THUMBNAIL_SIZE           "thumbnail-size"
#define PREF_CONTACT_SHEET_SQUARED_THUMBNAIL        "squared-thumbnail"
#define PREF_CONTACT_SHEET_THUMBNAIL_CAPTION        "thumbnail-caption"

/* keys: image wall */

#define PREF_IMAGE_WALL_DESTINATION                 "destination"
#define PREF_IMAGE_WALL_TEMPLATE                    "template"
#define PREF_IMAGE_WALL_MIME_TYPE                   "mime-type"
#define PREF_IMAGE_WALL_IMAGES_PER_PAGE             "images-per-page"
#define PREF_IMAGE_WALL_SINGLE_PAGE                 "single-page"
#define PREF_IMAGE_WALL_COLUMNS                     "columns"
#define PREF_IMAGE_WALL_SORT_TYPE                   "sort-type"
#define PREF_IMAGE_WALL_SORT_INVERSE                "sort-inverse"
#define PREF_IMAGE_WALL_THUMBNAIL_SIZE              "thumbnail-size"

G_END_DECLS

#endif /* PREFERENCES_H */
