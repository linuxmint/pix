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

#include <gthumb.h>

DEF_ACTION_CALLBACK (gth_browser_activate_create_folder)
DEF_ACTION_CALLBACK (gth_browser_activate_edit_cut)
DEF_ACTION_CALLBACK (gth_browser_activate_edit_copy)
DEF_ACTION_CALLBACK (gth_browser_activate_edit_paste)
DEF_ACTION_CALLBACK (gth_browser_activate_duplicate)
DEF_ACTION_CALLBACK (gth_browser_activate_trash)
DEF_ACTION_CALLBACK (gth_browser_activate_delete)
DEF_ACTION_CALLBACK (gth_browser_activate_remove_from_source)
DEF_ACTION_CALLBACK (gth_browser_activate_remove_from_source_permanently)
DEF_ACTION_CALLBACK (gth_browser_activate_rename)
DEF_ACTION_CALLBACK (gth_browser_activate_file_list_rename)
DEF_ACTION_CALLBACK (gth_browser_activate_copy_to_folder)
DEF_ACTION_CALLBACK (gth_browser_activate_move_to_folder)

DEF_ACTION_CALLBACK (gth_browser_activate_folder_context_open_in_file_manager)
DEF_ACTION_CALLBACK (gth_browser_activate_folder_context_create)
DEF_ACTION_CALLBACK (gth_browser_activate_folder_context_rename)
DEF_ACTION_CALLBACK (gth_browser_activate_folder_context_cut)
DEF_ACTION_CALLBACK (gth_browser_activate_folder_context_copy)
DEF_ACTION_CALLBACK (gth_browser_activate_folder_context_paste_into_folder)
DEF_ACTION_CALLBACK (gth_browser_activate_folder_context_trash)
DEF_ACTION_CALLBACK (gth_browser_activate_folder_context_delete)
DEF_ACTION_CALLBACK (gth_browser_activate_folder_context_copy_to)
DEF_ACTION_CALLBACK (gth_browser_activate_folder_context_move_to)

DEF_ACTION_CALLBACK (gth_browser_activate_open_with_gimp)
DEF_ACTION_CALLBACK (gth_browser_activate_open_with_application)

#endif /* ACTIONS_H */
