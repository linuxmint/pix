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

DEF_ACTION_CALLBACK (gth_browser_activate_video_screenshot)
DEF_ACTION_CALLBACK (gth_browser_activate_toggle_play)
DEF_ACTION_CALLBACK (gth_browser_activate_toggle_mute)
DEF_ACTION_CALLBACK (gth_browser_activate_play_faster)
DEF_ACTION_CALLBACK (gth_browser_activate_play_slower)
DEF_ACTION_CALLBACK (gth_browser_activate_video_zoom_fit)
DEF_ACTION_CALLBACK (gth_browser_activate_next_video_frame)
DEF_ACTION_CALLBACK (gth_browser_activate_skip_forward_smallest)
DEF_ACTION_CALLBACK (gth_browser_activate_skip_forward_smaller)
DEF_ACTION_CALLBACK (gth_browser_activate_skip_forward_small)
DEF_ACTION_CALLBACK (gth_browser_activate_skip_forward_big)
DEF_ACTION_CALLBACK (gth_browser_activate_skip_forward_bigger)
DEF_ACTION_CALLBACK (gth_browser_activate_skip_back_smallest)
DEF_ACTION_CALLBACK (gth_browser_activate_skip_back_smaller)
DEF_ACTION_CALLBACK (gth_browser_activate_skip_back_small)
DEF_ACTION_CALLBACK (gth_browser_activate_skip_back_big)
DEF_ACTION_CALLBACK (gth_browser_activate_skip_back_bigger)

#endif /* ACTIONS_H */
