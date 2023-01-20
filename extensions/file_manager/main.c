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
	/**
	 * Called when the rename command is invoked on the file list.
	 *
	 * @browser (GthBrowser*): the relative window.
	 **/
	gth_hook_register ("gth-browser-file-list-rename", 1);

	gth_hook_add_callback ("gth-browser-construct", 10, G_CALLBACK (fm__gth_browser_construct_cb), NULL);
	gth_hook_add_callback ("gth-browser-load-location-after", 10, G_CALLBACK (fm__gth_browser_load_location_after_cb), NULL);
	gth_hook_add_callback ("gth-browser-set-current-page", 10, G_CALLBACK (fm__gth_browser_set_current_page_cb), NULL);
	gth_hook_add_callback ("gth-browser-folder-tree-popup-before", 10, G_CALLBACK (fm__gth_browser_folder_tree_popup_before_cb), NULL);
	gth_hook_add_callback ("gth-browser-folder-tree-drag-data-received", 10, G_CALLBACK (fm__gth_browser_folder_tree_drag_data_received_cb), NULL);
	gth_hook_add_callback ("gth-browser-update-sensitivity", 10, G_CALLBACK (fm__gth_browser_update_sensitivity_cb), NULL);
	gth_hook_add_callback ("gth-browser-realize", 10, G_CALLBACK (fm__gth_browser_realize_cb), NULL);
	gth_hook_add_callback ("gth-browser-unrealize", 10, G_CALLBACK (fm__gth_browser_unrealize_cb), NULL);
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
