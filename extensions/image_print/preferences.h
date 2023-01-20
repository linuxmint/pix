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

/* schemas */

#define GTHUMB_IMAGE_PRINT_SCHEMA          GTHUMB_SCHEMA ".image-print"

/* keys */

#define PREF_IMAGE_PRINT_CAPTION           "caption"
#define PREF_IMAGE_PRINT_FONT_NAME         "font-name"
#define PREF_IMAGE_PRINT_HEADER_FONT_NAME  "header-font-name"
#define PREF_IMAGE_PRINT_FOOTER_FONT_NAME  "footer-font-name"
#define PREF_IMAGE_PRINT_HEADER            "header"
#define PREF_IMAGE_PRINT_FOOTER            "footer"
#define PREF_IMAGE_PRINT_N_ROWS            "n-rows"
#define PREF_IMAGE_PRINT_N_COLUMNS         "n-columns"
#define PREF_IMAGE_PRINT_UNIT              "unit"


void ip__dlg_preferences_construct_cb (GtkWidget  *dialog,
				       GthBrowser *browser,
				       GtkBuilder *builder);
void ip__dlg_preferences_apply_cb     (GtkWidget  *dialog,
				       GthBrowser *browser,
				       GtkBuilder *builder);

G_END_DECLS

#endif /* PREFERENCES_H */
