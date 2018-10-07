/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  Pix
 *
 *  Copyright (C) 2005-2009 Free Software Foundation, Inc.
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

#ifndef GTH_BROWSER_ACTION_ENTRIES_H
#define GTH_BROWSER_ACTION_ENTRIES_H

#include <config.h>
#include <glib/gi18n.h>
#include "gth-stock.h"
#include "gtk-utils.h"

static GthActionEntryExt gth_browser_action_entries[] = {
	{ "FileMenu", NULL, N_("_File") },
	{ "EditMenu", NULL, N_("_Edit") },
	{ "ViewMenu", NULL, N_("_View") },
	{ "GoMenu", NULL, N_("_Go") },
	{ "HelpMenu", NULL, N_("_Help") },
	{ "OpenWithMenu", NULL, N_("Open _With") },
	{ "ImportMenu", NULL, N_("I_mport From") },
	{ "ExportMenu", NULL, N_("E_xport To") },

	{ "File_NewWindow", NULL,
	  N_("New _Window"), "<control>N",
	  N_("Open another window"),
	  GTH_ACTION_FLAG_NONE,
	  G_CALLBACK (gth_browser_activate_action_file_new_window) },

	{ "File_Open", "document-open-symbolic",
	  N_("Open"), NULL,
	  NULL,
	  GTH_ACTION_FLAG_NONE,
	  G_CALLBACK (gth_browser_activate_action_file_open) },

	{ "File_Save", "document-save-symbolic",
	  N_("Save"), "<control>S",
	  NULL,
	  GTH_ACTION_FLAG_NONE,
	  G_CALLBACK (gth_browser_activate_action_file_save) },

	{ "File_SaveAs", "document-save-as-generic",
	  N_("Save As..."), NULL,
	  NULL,
	  GTH_ACTION_FLAG_NONE,
	  G_CALLBACK (gth_browser_activate_action_file_save_as) },

	{ "File_Revert", "document-revert-symbolic",
	  N_("Revert"), "F4",
	  NULL,
	  GTH_ACTION_FLAG_NONE,
	  G_CALLBACK (gth_browser_activate_action_file_revert) },

	{ "Folder_Open", "document-open-symbolic",
	  N_("Open"), "",
	  NULL,
	  GTH_ACTION_FLAG_NONE,
	  G_CALLBACK (gth_browser_activate_action_folder_open) },

	{ "Folder_OpenInNewWindow", NULL,
	  N_("Open in New Window"), "",
	  NULL,
	  GTH_ACTION_FLAG_NONE,
	  G_CALLBACK (gth_browser_activate_action_folder_open_in_new_window) },

	{ "Edit_Preferences", "preferences-other-symbolic",
	  N_("Preferences"), NULL,
	  N_("Edit various preferences"),
	  GTH_ACTION_FLAG_NONE,
	  G_CALLBACK (gth_browser_activate_action_edit_preferences) },

	{ "Edit_SelectAll", "edit-select-all-symbolic",
	  N_("Select All"), "<control>A",
	  NULL,
	  GTH_ACTION_FLAG_NONE,
	  G_CALLBACK (gth_browser_activate_action_edit_select_all) },

	{ "View_Sort_By", NULL,
	  N_("_Sort By..."), NULL,
	  NULL,
	  GTH_ACTION_FLAG_NONE,
	  G_CALLBACK (gth_browser_activate_action_view_sort_by) },

	{ "View_Filters", NULL,
	  N_("_Filter..."), NULL,
	  NULL,
	  GTH_ACTION_FLAG_NONE,
	  G_CALLBACK (gth_browser_activate_action_view_filter) },

	{ "View_Stop", "process-stop-symbolic",
	  N_("Stop"), "Escape",
	  N_("Stop loading the current location"),
	  GTH_ACTION_FLAG_NONE,
	  G_CALLBACK (gth_browser_activate_action_view_stop) },

	{ "View_Reload", "view-refresh-symbolic",
	  N_("Reload"), "<control>R",
	  N_("Reload the current location"),
	  GTH_ACTION_FLAG_NONE,
	  G_CALLBACK (gth_browser_activate_action_view_reload) },

	{ "View_Prev", "go-previous-symbolic",
	  N_("Previous"), NULL,
	  N_("View previous image"),
	  GTH_ACTION_FLAG_IS_IMPORTANT,
	  G_CALLBACK (gth_browser_activate_action_view_prev) },

	{ "View_Next", "go-next-symbolic",
	  N_("Next"), NULL,
	  N_("View next image"),
	  GTH_ACTION_FLAG_IS_IMPORTANT,
	  G_CALLBACK (gth_browser_activate_action_view_next) },

	{ "View_Fullscreen", "view-fullscreen-symbolic",
	  N_("Fullscreen"), "F11",
	  N_("Switch to fullscreen"),
	  GTH_ACTION_FLAG_NONE,
	  G_CALLBACK (gth_browser_activate_action_view_fullscreen) },

	{ "View_Leave_Fullscreen", "view-restore-symbolic",
	  N_("Leave Fullscreen"), NULL,
	  N_("Leave Fullscreen"),
	  GTH_ACTION_FLAG_NONE,
	  G_CALLBACK (gth_browser_activate_action_view_fullscreen) },

	{ "Go_Back", "go-previous-symbolic",
	  N_("Back"), "<alt>Left",
	  N_("Go to the previous visited location"),
	  GTH_ACTION_FLAG_NONE,
	  G_CALLBACK (gth_browser_activate_action_go_back) },

	{ "Go_Forward", "go-next-symbolic",
	  N_("Forward"), "<alt>Right",
	  N_("Go to the next visited location"),
	  GTH_ACTION_FLAG_NONE,
	  G_CALLBACK (gth_browser_activate_action_go_forward) },

	{ "Go_Up", "go-up-symbolic",
	  N_("Up"), "<alt>Up",
	  N_("Go up one level"),
	  GTH_ACTION_FLAG_NONE,
	  G_CALLBACK (gth_browser_activate_action_go_up) },

	{ "Go_Location", NULL,
	  N_("_Location..."), "<control>L",
	  N_("Specify a location to open"),
	  GTH_ACTION_FLAG_NONE,
	  G_CALLBACK (gth_browser_activate_action_go_location) },

	{ "Go_Home", NULL,
	  NULL, "<alt>Home",
	  NULL,
	  GTH_ACTION_FLAG_NONE,
	  G_CALLBACK (gth_browser_activate_action_go_home) },

	{ "Go_Clear_History", "list-remove-all-symbolic",
	  N_("_Delete History"), NULL,
	  N_("Delete the list of visited locations"),
	  GTH_ACTION_FLAG_NONE,
	  G_CALLBACK (gth_browser_activate_action_go_clear_history) },

	{ "View_BrowserMode", "view-grid-symbolic",
	  N_("Browser"), "Escape",
	  N_("View the folders"),
	  GTH_ACTION_FLAG_NONE,
	  G_CALLBACK (gth_browser_activate_action_browser_mode) },

	{ "Help_About", "help-about-symbolic",
	  N_("About"), NULL,
	  N_("Show information about pix"),
	  GTH_ACTION_FLAG_NONE,
	  G_CALLBACK (gth_browser_activate_action_help_about) },

	{ "Help_Help", "help-contents-symbolic",
	  N_("Contents"), "F1",
	  N_("Display the pix Manual"),
	  GTH_ACTION_FLAG_NONE,
	  G_CALLBACK (gth_browser_activate_action_help_help) },

	{ "Help_Shortcuts", "preferences-desktop-keyboard-shortcuts-symbolic",
	  N_("_Keyboard Shortcuts"), NULL,
	  " ",
	  GTH_ACTION_FLAG_NONE,
	  G_CALLBACK (gth_browser_activate_action_help_shortcuts) },

	{ "Browser_Tools", "image-x-generic-symbolic",
	  N_("Edit"), NULL,
	  N_("Edit file"),
	  GTH_ACTION_FLAG_NONE,
	  G_CALLBACK (gth_browser_activate_action_browser_tools) },
};


static GtkToggleActionEntry gth_browser_action_toggle_entries[] = {
	{ "View_Toolbar", NULL,
	  N_("_Toolbar"), NULL,
	  N_("View or hide the toolbar of this window"),
	  G_CALLBACK (gth_browser_activate_action_view_toolbar),
	  TRUE },
	{ "View_Statusbar", NULL,
	  N_("_Statusbar"), NULL,
	  N_("View or hide the statusbar of this window"),
	  G_CALLBACK (gth_browser_activate_action_view_statusbar),
	  TRUE },
	{ "View_Filterbar", NULL,
	  N_("_Filterbar"), NULL,
	  N_("View or hide the filterbar of this window"),
	  G_CALLBACK (gth_browser_activate_action_view_filterbar),
	  TRUE },
	{ "View_Sidebar", NULL,
	  N_("_Sidebar"), "F9",
	  N_("View or hide the sidebar of this window"),
	  G_CALLBACK (gth_browser_activate_action_view_sidebar),
	  TRUE },
	{ "View_Thumbnail_List", NULL,
	  N_("_Thumbnail Pane"), "F8",
	  N_("View or hide the thumbnail pane in viewer mode"),
	  G_CALLBACK (gth_browser_activate_action_view_thumbnail_list),
	  TRUE },
	{ "View_Thumbnails", NULL,
	  N_("_Thumbnails"), "<control>T",
	  N_("View thumbnails"),
	  G_CALLBACK (gth_browser_activate_action_view_thumbnails),
	  TRUE },
	{ "View_ShowHiddenFiles", NULL,
	  N_("_Hidden Files"), "<control>H",
	  N_("Show hidden files and folders"),
	  G_CALLBACK (gth_browser_activate_action_view_show_hidden_files),
	  FALSE },
	{ "Viewer_Properties", "document-properties-symbolic",
	  N_("Properties"), "<control>I",
	  N_("View file properties"),
	  G_CALLBACK (gth_browser_activate_action_viewer_properties),
	  FALSE },
	{ "Browser_Properties", "document-properties-symbolic",
	  N_("View file properties"), "<control>I",
	  N_("View file properties"),
	  G_CALLBACK (gth_browser_activate_action_viewer_properties),
	  FALSE },
	{ "Viewer_Tools", "image-x-generic-symbolic",
	  N_("Edit"), NULL,
	  N_("Edit file"),
	  G_CALLBACK (gth_browser_activate_action_viewer_tools),
	  FALSE },
	{ "View_ShrinkWrap", NULL,
	  N_("_Fit Window to Image"), "<control>e",
	  N_("Resize the window to the size of the image"),
	  G_CALLBACK (gth_browser_activate_action_view_shrink_wrap),
	  FALSE },
};

#endif /* GTH_BROWSER_ACTION_ENTRIES_H */
