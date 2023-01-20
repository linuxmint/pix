/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2019 Free Software Foundation, Inc.
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

DEF_ACTION_CALLBACK (gth_browser_activate_tool_adjust_contrast)
DEF_ACTION_CALLBACK (gth_browser_activate_tool_flip)
DEF_ACTION_CALLBACK (gth_browser_activate_tool_mirror)
DEF_ACTION_CALLBACK (gth_browser_activate_tool_rotate_right)
DEF_ACTION_CALLBACK (gth_browser_activate_tool_rotate_left)
DEF_ACTION_CALLBACK (gth_browser_activate_tool_crop)
DEF_ACTION_CALLBACK (gth_browser_activate_tool_resize)

#endif /* ACTIONS_H */
