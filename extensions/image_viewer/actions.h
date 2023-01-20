/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2013 Free Software Foundation, Inc.
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

DEF_ACTION_CALLBACK (gth_browser_activate_image_zoom_in)
DEF_ACTION_CALLBACK (gth_browser_activate_image_zoom_out)
DEF_ACTION_CALLBACK (gth_browser_activate_image_zoom_100)
DEF_ACTION_CALLBACK (gth_browser_activate_image_zoom_200)
DEF_ACTION_CALLBACK (gth_browser_activate_image_zoom_300)
DEF_ACTION_CALLBACK (gth_browser_activate_image_zoom_fit)
DEF_ACTION_CALLBACK (gth_browser_activate_image_zoom_fit_if_larger)
DEF_ACTION_CALLBACK (gth_browser_activate_image_zoom_fit_width)
DEF_ACTION_CALLBACK (gth_browser_activate_image_zoom_fit_width_if_larger)
DEF_ACTION_CALLBACK (gth_browser_activate_image_zoom_fit_height)
DEF_ACTION_CALLBACK (gth_browser_activate_image_zoom_fit_height_if_larger)
DEF_ACTION_CALLBACK (gth_browser_activate_image_undo)
DEF_ACTION_CALLBACK (gth_browser_activate_image_redo)
DEF_ACTION_CALLBACK (gth_browser_activate_copy_image)
DEF_ACTION_CALLBACK (gth_browser_activate_paste_image)
DEF_ACTION_CALLBACK (gth_browser_activate_apply_icc_profile)
DEF_ACTION_CALLBACK (gth_browser_activate_image_zoom)
DEF_ACTION_CALLBACK (gth_browser_activate_toggle_animation)
DEF_ACTION_CALLBACK (gth_browser_activate_step_animation)
DEF_ACTION_CALLBACK (gth_browser_activate_transparency_style)
DEF_ACTION_CALLBACK (gth_browser_activate_scroll_step_left)
DEF_ACTION_CALLBACK (gth_browser_activate_scroll_step_right)
DEF_ACTION_CALLBACK (gth_browser_activate_scroll_step_up)
DEF_ACTION_CALLBACK (gth_browser_activate_scroll_step_down)
DEF_ACTION_CALLBACK (gth_browser_activate_scroll_page_left)
DEF_ACTION_CALLBACK (gth_browser_activate_scroll_page_right)
DEF_ACTION_CALLBACK (gth_browser_activate_scroll_page_up)
DEF_ACTION_CALLBACK (gth_browser_activate_scroll_page_down)
DEF_ACTION_CALLBACK (gth_browser_activate_scroll_to_center)

#endif /* ACTIONS_H */
