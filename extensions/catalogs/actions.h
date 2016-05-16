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

DEFINE_ACTION(gth_browser_activate_action_edit_add_to_catalog)
DEFINE_ACTION(gth_browser_activate_action_edit_remove_from_catalog)
DEFINE_ACTION(gth_browser_activate_action_catalog_new)
DEFINE_ACTION(gth_browser_activate_action_catalog_new_library)
DEFINE_ACTION(gth_browser_activate_action_catalog_remove)
DEFINE_ACTION(gth_browser_activate_action_catalog_rename)
DEFINE_ACTION(gth_browser_activate_action_catalog_properties)
DEFINE_ACTION(gth_browser_activate_action_go_to_container)

void gth_browser_add_to_catalog (GthBrowser *browser,
				 GFile      *catalog);

#endif /* ACTIONS_H */
