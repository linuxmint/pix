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
#include "gth-file-source-catalogs.h"


G_MODULE_EXPORT void
gthumb_extension_activate (void)
{
	/**
	 * Called to create the catalog class from the given file content.
	 *
	 * @buffer (const char *): the file content.
	 * @return (GthCatalog *): return a pointer to the object that can
	 * handle the catalog data, or NULL if the data type doesn't match.
	 **/
	gth_hook_register ("gth-catalog-load-from-data", 1);

	/**
	 * Called to create the right catalog class for the given uri.
	 *
	 * @uri (const char *): the file uri.
	 * @return (GthCatalog *): return a pointer to the object that can
	 * handle the catalog uri, or NULL if the data type doesn't match.
	 **/
	gth_hook_register ("gth-catalog-new-for-uri", 1);

	/**
	 * Called to update the catalog data from a given file data.
	 *
	 * @catalog (GthCatalog *): the catalog to update
	 * @file_data (GthFileData *): the file data
	 **/
	gth_hook_register ("gth-catalog-read-metadata", 2);

	/**
	 * Called to update the file metadata from a catalog.
	 *
	 * @catalog (GthCatalog *): the catalog
	 * @file_data (GthFileData *): the file data to update
	 **/
	gth_hook_register ("gth-catalog-write-metadata", 2);

	/**
	 * Called to update the catalog from a xml file.
	 *
	 * @catalog (GthCatalog *):
	 * @root (DomElement *):
	 */
	gth_hook_register ("gth-catalog-read-from-doc", 2);

	/**
	 * Called to update a xml file from a catalog.
	 *
	 * @catalog (GthCatalog *):
	 * @doc (DomDocument *):
	 * @root (DomElement *):
	 */
	gth_hook_register ("gth-catalog-write-to-doc", 3);

	/**
	 * Called to add sections to the catalog properties dialog.
	 *
	 * @builder   (GtkBuilder *): the builder relative to the window
	 * @file_data (GthFileData *): the catalog file
	 * @catalog (GthCatalog *): the catalog data
	 **/
	gth_hook_register ("dlg-catalog-properties", 3);

	/**
	 * Called to save the properties dialog changes.
	 *
	 * @builder   (GtkBuilder *): the builder relative to the window
	 * @file_data (GthFileData *): the catalog file
	 * @catalog (GthCatalog *): the catalog data
	 **/
	gth_hook_register ("dlg-catalog-properties-save", 3);

	/**
	 * Called after saving the catalog properties.
	 *
	 * @browser (GthBrowser *): the main window
	 * @file_data (GthFileData *): the catalog file
	 * @catalog (GthCatalog *): the catalog data
	 **/
	gth_hook_register ("dlg-catalog-properties-saved", 3);

	/**
	 * Called to create a catalog when organizing files in catalogs.
	 *
	 * @data (GthGroupPolicyData *): a structure that contains in and out
	 * parameters, as described in gth-organize-task.h
	 **/
	gth_hook_register ("gth-organize-task-create-catalog", 1);

	gth_hook_add_callback ("command-line-files", 10, G_CALLBACK (catalogs__command_line_files_cb), NULL);
	gth_hook_add_callback ("gth-catalog-load-from-data", 10, G_CALLBACK (catalogs__gth_catalog_load_from_data_cb), NULL);
	gth_hook_add_callback ("gth-catalog-new-for-uri", 10, G_CALLBACK (catalogs__gth_catalog_new_for_uri_cb), NULL);

	gth_main_register_file_source (GTH_TYPE_FILE_SOURCE_CATALOGS);
	gth_hook_add_callback ("initialize", 10, G_CALLBACK (catalogs__initialize_cb), NULL);
	gth_hook_add_callback ("gth-browser-construct", 10, G_CALLBACK (catalogs__gth_browser_construct_cb), NULL);
	gth_hook_add_callback ("gth-browser-selection-changed", 10, G_CALLBACK (catalogs__gth_browser_selection_changed_cb), NULL);
	gth_hook_add_callback ("gth-browser-folder-tree-popup-before", 10, G_CALLBACK (catalogs__gth_browser_folder_tree_popup_before_cb), NULL);
	gth_hook_add_callback ("gth-browser-load-location-after", 10, G_CALLBACK (catalogs__gth_browser_load_location_after_cb), NULL);
	gth_hook_add_callback ("gth-browser-update-extra-widget", 10, G_CALLBACK (catalogs__gth_browser_update_extra_widget_cb), NULL);
	gth_hook_add_callback ("gth-browser-file-renamed", 10, G_CALLBACK (catalogs__gth_browser_file_renamed_cb), NULL);
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
