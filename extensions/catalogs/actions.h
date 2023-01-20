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

DEF_ACTION_CALLBACK (gth_browser_activate_add_to_catalog)
DEF_ACTION_CALLBACK (gth_browser_activate_remove_from_catalog)
DEF_ACTION_CALLBACK (gth_browser_activate_create_catalog)
DEF_ACTION_CALLBACK (gth_browser_activate_create_library)
DEF_ACTION_CALLBACK (gth_browser_activate_remove_catalog)
DEF_ACTION_CALLBACK (gth_browser_activate_rename_catalog)
DEF_ACTION_CALLBACK (gth_browser_activate_catalog_properties)
DEF_ACTION_CALLBACK (gth_browser_activate_go_to_container_from_catalog)

void gth_browser_add_to_catalog (GthBrowser *browser,
				 GFile      *catalog);

#endif /* ACTIONS_H */
