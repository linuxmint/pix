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

#ifndef ACTIONS_H
#define ACTIONS_H

#include <gtk/gtk.h>

#define DEFINE_ACTION(x) void x (GtkAction *action, gpointer data);

DEFINE_ACTION(gth_browser_action_new_folder)
DEFINE_ACTION(gth_browser_action_rename_folder)
DEFINE_ACTION(gth_browser_activate_action_edit_cut_files)
DEFINE_ACTION(gth_browser_activate_action_edit_copy_files)
DEFINE_ACTION(gth_browser_activate_action_edit_paste)
DEFINE_ACTION(gth_browser_activate_action_edit_duplicate)
DEFINE_ACTION(gth_browser_activate_action_edit_trash)
DEFINE_ACTION(gth_browser_activate_action_edit_delete)
DEFINE_ACTION(gth_browser_activate_action_edit_rename)
DEFINE_ACTION(gth_browser_activate_action_folder_open_in_file_manager)
DEFINE_ACTION(gth_browser_activate_action_folder_create)
DEFINE_ACTION(gth_browser_activate_action_folder_rename)
DEFINE_ACTION(gth_browser_activate_action_folder_cut)
DEFINE_ACTION(gth_browser_activate_action_folder_copy)
DEFINE_ACTION(gth_browser_activate_action_folder_paste)
DEFINE_ACTION(gth_browser_activate_action_folder_trash)
DEFINE_ACTION(gth_browser_activate_action_folder_delete)
DEFINE_ACTION(gth_browser_activate_action_folder_copy_to_folder)
DEFINE_ACTION(gth_browser_activate_action_folder_move_to_folder)
DEFINE_ACTION(gth_browser_activate_action_tool_copy_to_folder)
DEFINE_ACTION(gth_browser_activate_action_tool_move_to_folder)

#endif /* ACTIONS_H */
