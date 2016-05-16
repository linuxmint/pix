/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
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

	{ "File_NewWindow", "window-new",
	  N_("New _Window"), "<control>N",
	  N_("Open another window"),
	  GTH_ACTION_FLAG_NONE,
	  G_CALLBACK (gth_browser_activate_action_file_new_window) },

	{ "File_Open", GTK_STOCK_OPEN,
	  NULL, NULL,
	  NULL,
	  GTH_ACTION_FLAG_NONE,
	  G_CALLBACK (gth_browser_activate_action_file_open) },

	{ "File_Save", GTK_STOCK_SAVE,
	  NULL, "<control>S",
	  NULL,
	  GTH_ACTION_FLAG_NONE,
	  G_CALLBACK (gth_browser_activate_action_file_save) },

	{ "File_SaveAs", GTK_STOCK_SAVE_AS,
	  NULL, NULL,
	  NULL,
	  GTH_ACTION_FLAG_NONE,
	  G_CALLBACK (gth_browser_activate_action_file_save_as) },

	{ "File_Revert", GTK_STOCK_REVERT_TO_SAVED,
	  NULL, "F4",
	  NULL,
	  GTH_ACTION_FLAG_NONE,
	  G_CALLBACK (gth_browser_activate_action_file_revert) },

	{ "Folder_Open", GTK_STOCK_OPEN,
	  N_("Open"), "",
	  NULL,
	  GTH_ACTION_FLAG_NONE,
	  G_CALLBACK (gth_browser_activate_action_folder_open) },

	{ "Folder_OpenInNewWindow", NULL,
	  N_("Open in New Window"), "",
	  NULL,
	  GTH_ACTION_FLAG_NONE,
	  G_CALLBACK (gth_browser_activate_action_folder_open_in_new_window) },

	{ "Edit_Preferences", GTK_STOCK_PREFERENCES,
	  NULL, NULL,
	  N_("Edit various preferences"),
	  GTH_ACTION_FLAG_NONE,
	  G_CALLBACK (gth_browser_activate_action_edit_preferences) },

	{ "Edit_SelectAll", GTK_STOCK_SELECT_ALL,
	  NULL, "<control>A",
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

	{ "View_Stop", GTK_STOCK_STOP,
	  NULL, "Escape",
	  N_("Stop loading the current location"),
	  GTH_ACTION_FLAG_NONE,
	  G_CALLBACK (gth_browser_activate_action_view_stop) },

	{ "View_Reload", GTK_STOCK_REFRESH,
	  NULL, "<control>R",
	  N_("Reload the current location"),
	  GTH_ACTION_FLAG_NONE,
	  G_CALLBACK (gth_browser_activate_action_view_reload) },

	{ "View_Prev", GTK_STOCK_GO_UP,
	  N_("Previous"), NULL,
	  N_("View previous image"),
	  GTH_ACTION_FLAG_IS_IMPORTANT,
	  G_CALLBACK (gth_browser_activate_action_view_prev) },

	{ "View_Next", GTK_STOCK_GO_DOWN,
	  N_("Next"), NULL,
	  N_("View next image"),
	  GTH_ACTION_FLAG_IS_IMPORTANT,
	  G_CALLBACK (gth_browser_activate_action_view_next) },

	{ "View_Fullscreen", GTK_STOCK_FULLSCREEN,
	  NULL, "F11",
	  N_("Switch to fullscreen"),
	  GTH_ACTION_FLAG_NONE,
	  G_CALLBACK (gth_browser_activate_action_view_fullscreen) },

	{ "View_Leave_Fullscreen", GTK_STOCK_LEAVE_FULLSCREEN,
	  NULL, NULL,
	  N_("Leave Fullscreen"),
	  GTH_ACTION_FLAG_NONE,
	  G_CALLBACK (gth_browser_activate_action_view_fullscreen) },

	{ "Go_Back", GTK_STOCK_GO_BACK,
	  NULL, "<alt>Left",
	  N_("Go to the previous visited location"),
	  GTH_ACTION_FLAG_NONE,
	  G_CALLBACK (gth_browser_activate_action_go_back) },

	{ "Go_Forward", GTK_STOCK_GO_FORWARD,
	  NULL, "<alt>Right",
	  N_("Go to the next visited location"),
	  GTH_ACTION_FLAG_NONE,
	  G_CALLBACK (gth_browser_activate_action_go_forward) },

	{ "Go_Up", GTK_STOCK_GO_UP,
	  NULL, "<alt>Up",
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

	{ "Go_Clear_History", GTK_STOCK_CLEAR,
	  N_("_Delete History"), NULL,
	  N_("Delete the list of visited locations"),
	  GTH_ACTION_FLAG_NONE,
	  G_CALLBACK (gth_browser_activate_action_go_clear_history) },

	{ "View_BrowserMode", GTH_STOCK_BROWSER_MODE,
	  N_("Browser"), "Escape",
	  N_("View the folders"),
	  GTH_ACTION_FLAG_NONE,
	  G_CALLBACK (gth_browser_activate_action_browser_mode) },

	{ "Help_About", GTK_STOCK_ABOUT,
	  NULL, NULL,
	  N_("Show information about gthumb"),
	  GTH_ACTION_FLAG_NONE,
	  G_CALLBACK (gth_browser_activate_action_help_about) },

	{ "Help_Help", GTK_STOCK_HELP,
	  N_("Contents"), "F1",
	  N_("Display the gthumb Manual"),
	  GTH_ACTION_FLAG_NONE,
	  G_CALLBACK (gth_browser_activate_action_help_help) },

	{ "Help_Shortcuts", NULL,
	  N_("_Keyboard Shortcuts"), NULL,
	  " ",
	  GTH_ACTION_FLAG_NONE,
	  G_CALLBACK (gth_browser_activate_action_help_shortcuts) },

	{ "Browser_Tools", "palette",
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
	{ "Viewer_Properties", GTK_STOCK_PROPERTIES,
	  NULL, "<control>I",
	  N_("View file properties"),
	  G_CALLBACK (gth_browser_activate_action_viewer_properties),
	  FALSE },
	{ "Browser_Properties", GTK_STOCK_PROPERTIES,
	  NULL, "<control>I",
	  N_("View file properties"),
	  G_CALLBACK (gth_browser_activate_action_viewer_properties),
	  FALSE },
	{ "Viewer_Tools", "palette",
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
