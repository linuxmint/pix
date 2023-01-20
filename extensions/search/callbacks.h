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
#include <extensions/catalogs/gth-catalog.h>
#include <extensions/catalogs/gth-organize-task.h>

void         search__gth_browser_construct_cb            (GthBrowser         *browser);
void         search__gth_browser_update_sensitivity_cb   (GthBrowser         *browser);
void         search__gth_browser_update_extra_widget_cb  (GthBrowser         *browser);
void         search__gth_browser_load_location_before_cb (GthBrowser         *browser,
							  GFile              *location);
GthCatalog * search__gth_catalog_load_from_data_cb       (const void         *buffer);
GthCatalog * search__gth_catalog_new_for_uri_cb          (const char         *uri);
void         search__dlg_catalog_properties              (GtkBuilder         *builder,
							  GthFileData        *file_data,
							  GthCatalog         *catalog);
void         search__dlg_catalog_properties_save         (GtkBuilder         *builder,
							  GthFileData        *file_data,
							  GthCatalog         *catalog);
void         search__dlg_catalog_properties_saved        (GthBrowser         *browser,
							  GthFileData        *file_data,
							  GthCatalog         *catalog);
void         search__gth_organize_task_create_catalog    (GthGroupPolicyData *data);

#endif /* CALLBACKS_H */
