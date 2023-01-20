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
#include "glib-utils.h"

DEF_ACTION_CALLBACK (gth_browser_activate_new_window)
DEF_ACTION_CALLBACK (gth_browser_activate_preferences)
DEF_ACTION_CALLBACK (gth_browser_activate_show_shortcuts)
DEF_ACTION_CALLBACK (gth_browser_activate_show_help)
DEF_ACTION_CALLBACK (gth_browser_activate_about)
DEF_ACTION_CALLBACK (gth_browser_activate_quit)
DEF_ACTION_CALLBACK (gth_browser_activate_browser_mode)
DEF_ACTION_CALLBACK (gth_browser_activate_browser_edit_file)
DEF_ACTION_CALLBACK (gth_browser_activate_browser_properties)
DEF_ACTION_CALLBACK (gth_browser_activate_clear_history)
DEF_ACTION_CALLBACK (gth_browser_activate_close)
DEF_ACTION_CALLBACK (gth_browser_activate_fullscreen)
DEF_ACTION_CALLBACK (gth_browser_activate_go_back)
DEF_ACTION_CALLBACK (gth_browser_activate_go_forward)
DEF_ACTION_CALLBACK (gth_browser_activate_go_to_history_pos)
DEF_ACTION_CALLBACK (gth_browser_activate_go_to_location)
DEF_ACTION_CALLBACK (gth_browser_activate_go_home)
DEF_ACTION_CALLBACK (gth_browser_activate_go_up)
DEF_ACTION_CALLBACK (gth_browser_activate_reload)
DEF_ACTION_CALLBACK (gth_browser_activate_open_location)
DEF_ACTION_CALLBACK (gth_browser_activate_revert_to_saved)
DEF_ACTION_CALLBACK (gth_browser_activate_save)
DEF_ACTION_CALLBACK (gth_browser_activate_save_as)
DEF_ACTION_CALLBACK (gth_browser_activate_toggle_edit_file)
DEF_ACTION_CALLBACK (gth_browser_activate_toggle_file_properties)
DEF_ACTION_CALLBACK (gth_browser_activate_viewer_edit_file)
DEF_ACTION_CALLBACK (gth_browser_activate_viewer_properties)
DEF_ACTION_CALLBACK (gth_browser_activate_unfullscreen)
DEF_ACTION_CALLBACK (gth_browser_activate_open_folder_in_new_window)
DEF_ACTION_CALLBACK (gth_browser_activate_show_progress_dialog)
DEF_ACTION_CALLBACK (gth_browser_activate_show_hidden_files)
DEF_ACTION_CALLBACK (gth_browser_activate_sort_by)
DEF_ACTION_CALLBACK (gth_browser_activate_show_statusbar)
DEF_ACTION_CALLBACK (gth_browser_activate_show_sidebar)
DEF_ACTION_CALLBACK (gth_browser_activate_show_thumbnail_list)
DEF_ACTION_CALLBACK (gth_browser_activate_toggle_statusbar)
DEF_ACTION_CALLBACK (gth_browser_activate_toggle_sidebar)
DEF_ACTION_CALLBACK (gth_browser_activate_toggle_thumbnail_list)
DEF_ACTION_CALLBACK (gth_browser_activate_show_first_image)
DEF_ACTION_CALLBACK (gth_browser_activate_show_last_image)
DEF_ACTION_CALLBACK (gth_browser_activate_show_previous_image)
DEF_ACTION_CALLBACK (gth_browser_activate_show_next_image)
DEF_ACTION_CALLBACK (gth_browser_activate_apply_editor_changes)
DEF_ACTION_CALLBACK (gth_browser_activate_select_all)
DEF_ACTION_CALLBACK (gth_browser_activate_unselect_all)
DEF_ACTION_CALLBACK (gth_browser_activate_show_menu)
DEF_ACTION_CALLBACK (gth_browser_activate_set_filter)
DEF_ACTION_CALLBACK (gth_browser_activate_set_filter_all)

#endif /* GTH_BROWSER_ACTIONS_CALLBACK_H */
