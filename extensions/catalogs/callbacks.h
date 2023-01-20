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

#ifndef CALLBACKS_H
#define CALLBACKS_H

#include <gthumb.h>
#include "gth-catalog.h"

void catalogs__initialize_cb                              (void);
void catalogs__gth_browser_construct_cb                   (GthBrowser    *browser);
void catalogs__gth_browser_selection_changed_cb           (GthBrowser    *browser,
							   int            n_selected);
void catalogs__gth_browser_folder_tree_popup_before_cb    (GthBrowser    *browser,
							   GthFileSource *file_source,
							   GthFileData   *folder);
GFile *      catalogs__command_line_files_cb              (GList         *files);
GthCatalog * catalogs__gth_catalog_load_from_data_cb      (const void    *buffer);
GthCatalog * catalogs__gth_catalog_new_for_uri_cb         (const char    *uri);
void         catalogs__gth_browser_load_location_after_cb (GthBrowser    *browser,
							   GthFileData   *location);
void         catalogs__gth_browser_update_extra_widget_cb (GthBrowser    *browser);
void         catalogs__gth_browser_file_renamed_cb        (GthBrowser    *browser,
							   GFile         *file,
							   GFile         *new_file);

#endif /* CALLBACKS_H */
