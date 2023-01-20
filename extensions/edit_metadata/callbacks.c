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


#include <config.h>
#include <glib/gi18n.h>
#include <glib-object.h>
#include <gdk/gdkkeysyms.h>
#include <gthumb.h>
#include <extensions/list_tools/list-tools.h>
#include "actions.h"
#include "callbacks.h"
#include "gth-tag-task.h"


#define BROWSER_DATA_KEY "edit-metadata-data"


static const GActionEntry actions[] = {
	{ "edit-metadata", gth_browser_activate_edit_metadata },
	{ "edit-tags", gth_browser_activate_edit_tags },
	{ "delete-metadata", gth_browser_activate_delete_metadata },
};


static const GthShortcut shortcuts[] = {
	{ "edit-metadata", N_("Edit comment"), GTH_SHORTCUT_CONTEXT_BROWSER_VIEWER, GTH_SHORTCUT_CATEGORY_FILE_MANAGER, "c" },
	{ "edit-tags", N_("Edit tags"), GTH_SHORTCUT_CONTEXT_BROWSER_VIEWER, GTH_SHORTCUT_CATEGORY_FILE_MANAGER, "t" },
};


static const GthMenuEntry tools_actions[] = {
	{ N_("Delete Metadata"), "win.delete-metadata" }
};


static const GthMenuEntry file_list_actions[] = {
	{ N_("Comment"), "win.edit-metadata" },
	{ N_("Tags"), "win.edit-tags" }
};


void
edit_metadata__gth_browser_construct_cb (GthBrowser *browser)
{
	g_return_if_fail (GTH_IS_BROWSER (browser));

	g_action_map_add_action_entries (G_ACTION_MAP (browser),
					 actions,
					 G_N_ELEMENTS (actions),
					 browser);
	gth_window_add_shortcuts (GTH_WINDOW (browser),
				  shortcuts,
				  G_N_ELEMENTS (shortcuts));

	if (gth_main_extension_is_active ("list_tools"))
		gth_menu_manager_append_entries (gth_browser_get_menu_manager (browser, GTH_BROWSER_MENU_MANAGER_MORE_TOOLS),
						 tools_actions,
						 G_N_ELEMENTS (tools_actions));
	gth_menu_manager_append_entries (gth_browser_get_menu_manager (browser, GTH_BROWSER_MENU_MANAGER_FILE_LIST_OTHER_ACTIONS),
					 file_list_actions,
					 G_N_ELEMENTS (file_list_actions));
	gth_menu_manager_append_entries (gth_browser_get_menu_manager (browser, GTH_BROWSER_MENU_MANAGER_FILE_OTHER_ACTIONS),
					 file_list_actions,
					 G_N_ELEMENTS (file_list_actions));

	gth_browser_add_header_bar_button (browser,
					   GTH_BROWSER_HEADER_SECTION_VIEWER_EDIT_METADATA,
					   "comment-symbolic",
					   _("Comment"),
					   "win.edit-metadata",
					   NULL);
	gth_browser_add_header_bar_button (browser,
					   GTH_BROWSER_HEADER_SECTION_VIEWER_EDIT_METADATA,
					   "tag-symbolic",
					   _("Tags"),
					   "win.edit-tags",
					   NULL);

	gth_browser_add_header_bar_button (browser,
					   GTH_BROWSER_HEADER_SECTION_BROWSER_METADATA_TOOLS,
					   "comment-symbolic",
					   _("Comment"),
					   "win.edit-metadata",
					   NULL);
	gth_browser_add_header_bar_button (browser,
					   GTH_BROWSER_HEADER_SECTION_BROWSER_METADATA_TOOLS,
					   "tag-symbolic",
					   _("Tags"),
					   "win.edit-tags",
					   NULL);
}


void
edit_metadata__gth_browser_selection_changed_cb (GthBrowser *browser,
						 int         n_selected)
{
	gboolean sensitive;

	sensitive = (n_selected > 0);
	g_object_set (g_action_map_lookup_action (G_ACTION_MAP (browser), "edit-metadata"), "enabled", sensitive, NULL);
	g_object_set (g_action_map_lookup_action (G_ACTION_MAP (browser), "edit-tags"), "enabled", sensitive, NULL);
	g_object_set (g_action_map_lookup_action (G_ACTION_MAP (browser), "delete-metadata"), "enabled", sensitive, NULL);
}
