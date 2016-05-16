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

#ifndef GTH_BROWSER_ACTIONS_CALLBACKS_H
#define GTH_BROWSER_ACTIONS_CALLBACKS_H

#include <gtk/gtk.h>

#define DEFINE_ACTION(x) void x (GtkAction *action, gpointer data);

DEFINE_ACTION(gth_browser_activate_action_bookmarks_add)
DEFINE_ACTION(gth_browser_activate_action_bookmarks_edit)
DEFINE_ACTION(gth_browser_activate_action_browser_mode)
DEFINE_ACTION(gth_browser_activate_action_edit_comment)
DEFINE_ACTION(gth_browser_activate_action_edit_preferences)
DEFINE_ACTION(gth_browser_activate_action_edit_select_all)
DEFINE_ACTION(gth_browser_activate_action_file_open)
DEFINE_ACTION(gth_browser_activate_action_file_new_window)
DEFINE_ACTION(gth_browser_activate_action_file_revert)
DEFINE_ACTION(gth_browser_activate_action_file_save)
DEFINE_ACTION(gth_browser_activate_action_file_save_as)
DEFINE_ACTION(gth_browser_activate_action_folder_open)
DEFINE_ACTION(gth_browser_activate_action_folder_open_in_new_window)
DEFINE_ACTION(gth_browser_activate_action_folder_open_in_file_manager)
DEFINE_ACTION(gth_browser_activate_action_go_back)
DEFINE_ACTION(gth_browser_activate_action_go_forward)
DEFINE_ACTION(gth_browser_activate_action_go_up)
DEFINE_ACTION(gth_browser_activate_action_go_location)
DEFINE_ACTION(gth_browser_activate_action_go_clear_history)
DEFINE_ACTION(gth_browser_activate_action_go_home)
DEFINE_ACTION(gth_browser_activate_action_help_help)
DEFINE_ACTION(gth_browser_activate_action_help_shortcuts)
DEFINE_ACTION(gth_browser_activate_action_help_about)
DEFINE_ACTION(gth_browser_activate_action_view_sort_by)
DEFINE_ACTION(gth_browser_activate_action_view_filter)
DEFINE_ACTION(gth_browser_activate_action_view_filterbar)
DEFINE_ACTION(gth_browser_activate_action_view_fullscreen)
DEFINE_ACTION(gth_browser_activate_action_view_thumbnails)
DEFINE_ACTION(gth_browser_activate_action_view_toolbar)
DEFINE_ACTION(gth_browser_activate_action_view_show_hidden_files)
DEFINE_ACTION(gth_browser_activate_action_view_statusbar)
DEFINE_ACTION(gth_browser_activate_action_view_sidebar)
DEFINE_ACTION(gth_browser_activate_action_view_thumbnail_list)
DEFINE_ACTION(gth_browser_activate_action_view_stop)
DEFINE_ACTION(gth_browser_activate_action_view_reload)
DEFINE_ACTION(gth_browser_activate_action_view_next)
DEFINE_ACTION(gth_browser_activate_action_view_prev)
DEFINE_ACTION(gth_browser_activate_action_viewer_properties)
DEFINE_ACTION(gth_browser_activate_action_browser_tools)
DEFINE_ACTION(gth_browser_activate_action_viewer_tools)
DEFINE_ACTION(gth_browser_activate_action_view_shrink_wrap)

#endif /* GTH_BROWSER_ACTIONS_CALLBACK_H */
