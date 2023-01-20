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
#include "gth-file-source-selections.h"
#include "gth-metadata-provider-selections.h"
#include "shortcuts.h"


static GthShortcutCategory shortcut_categories[] = {
	{ GTH_SHORTCUT_CATEGORY_SELECTIONS, N_("Selections"), 17 },
};


G_MODULE_EXPORT void
gthumb_extension_activate (void)
{
	gth_main_register_file_source (GTH_TYPE_FILE_SOURCE_SELECTIONS);
	gth_main_register_metadata_provider (GTH_TYPE_METADATA_PROVIDER_SELECTIONS);
	gth_main_register_shortcut_category (shortcut_categories, G_N_ELEMENTS (shortcut_categories));
	gth_hook_add_callback ("gth-browser-construct", 10, G_CALLBACK (selections__gth_browser_construct_cb), NULL);
	gth_hook_add_callback ("gth-browser-selection-changed", 10, G_CALLBACK (selections__gth_browser_selection_changed_cb), NULL);
	gth_hook_add_callback ("gth-browser-file-list-key-press", 10, G_CALLBACK (selections__gth_browser_file_list_key_press_cb), NULL);
	gth_hook_add_callback ("gth-browser-load-location-after", 10, G_CALLBACK (selections__gth_browser_load_location_after_cb), NULL);
	gth_hook_add_callback ("gth-browser-update-extra-widget", 20, G_CALLBACK (selections__gth_browser_update_extra_widget_cb), NULL);
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
