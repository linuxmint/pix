/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2008 Free Software Foundation, Inc.
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


#include <config.h>
#include <gtk/gtk.h>
#include <gthumb.h>
#include "callbacks.h"


G_MODULE_EXPORT void
gthumb_extension_activate (void)
{
	gth_hook_add_callback ("gth-catalog-load-from-data", 10, G_CALLBACK (search__gth_catalog_load_from_data_cb), NULL);
	gth_hook_add_callback ("gth-catalog-new-for-uri", 10, G_CALLBACK (search__gth_catalog_new_for_uri_cb), NULL);
	gth_hook_add_callback ("gth-browser-construct", 10, G_CALLBACK (search__gth_browser_construct_cb), NULL);
	gth_hook_add_callback ("gth-browser-update-extra-widget", 20, G_CALLBACK (search__gth_browser_update_extra_widget_cb), NULL);
	gth_hook_add_callback ("gth-browser-load-location-before", 10, G_CALLBACK (search__gth_browser_load_location_before_cb), NULL);
	gth_hook_add_callback ("dlg-catalog-properties", 10, G_CALLBACK (search__dlg_catalog_properties), NULL);
	gth_hook_add_callback ("dlg-catalog-properties-save", 10, G_CALLBACK (search__dlg_catalog_properties_save), NULL);
	gth_hook_add_callback ("dlg-catalog-properties-saved", 10, G_CALLBACK (search__dlg_catalog_properties_saved), NULL);
	gth_hook_add_callback ("gth-organize-task-create-catalog", 10, G_CALLBACK (search__gth_organize_task_create_catalog), NULL);
}


G_MODULE_EXPORT void
gthumb_extension_deactivate (void)
{
}


G_MODULE_EXPORT gboolean
gthumb_extension_is_configurable (void)
{
	return FALSE;
}


G_MODULE_EXPORT void
gthumb_extension_configure (GtkWindow *parent)
{
}
