/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  Pix
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

#include <pix.h>

void gth_browser_show_selection        (GthBrowser *browser,
					int         n_selection);
void gth_browser_add_to_selection      (GthBrowser *browser,
					int         n_selection);
void gth_browser_remove_from_selection (GthBrowser *browser,
					int         n_selection);

DEF_ACTION_CALLBACK (gth_browser_activate_go_to_selection)
DEF_ACTION_CALLBACK (gth_browser_activate_add_to_selection)
DEF_ACTION_CALLBACK (gth_browser_activate_remove_from_selection)
DEF_ACTION_CALLBACK (gth_browser_activate_remove_from_current_selection)
DEF_ACTION_CALLBACK (gth_browser_activate_go_to_file_container)

#endif /* ACTIONS_H */
