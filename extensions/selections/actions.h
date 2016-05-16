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

void gth_browser_activate_action_show_selection        (GthBrowser *browser,
							int         n_selection);
void gth_browser_activate_action_add_to_selection      (GthBrowser *browser,
						        int         n_selection);
void gth_browser_activate_action_remove_from_selection (GthBrowser *browser,
						        int         n_selection);

DEFINE_ACTION(gth_browser_activate_action_go_selection_1)
DEFINE_ACTION(gth_browser_activate_action_go_selection_2)
DEFINE_ACTION(gth_browser_activate_action_go_selection_3)
DEFINE_ACTION(gth_browser_activate_action_add_to_selection_1)
DEFINE_ACTION(gth_browser_activate_action_add_to_selection_2)
DEFINE_ACTION(gth_browser_activate_action_add_to_selection_3)
DEFINE_ACTION(gth_browser_activate_action_selection_go_to_container)

#endif /* ACTIONS_H */
