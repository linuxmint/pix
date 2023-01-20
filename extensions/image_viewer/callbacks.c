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


#include <config.h>
#include <glib/gi18n.h>
#include <glib-object.h>
#include <gthumb.h>
#include "callbacks.h"
#include "shortcuts.h"


static const GthShortcut shortcuts[] = {
	{ "image-zoom-in", N_("Zoom in"), GTH_SHORTCUT_CONTEXT_VIEWER, GTH_SHORTCUT_CATEGORY_IMAGE_VIEWER, "plus" },
	{ "image-zoom-out", N_("Zoom out"), GTH_SHORTCUT_CONTEXT_VIEWER, GTH_SHORTCUT_CATEGORY_IMAGE_VIEWER, "minus" },
	{ "image-zoom-100", N_("Zoom 100%"), GTH_SHORTCUT_CONTEXT_VIEWER, GTH_SHORTCUT_CATEGORY_IMAGE_VIEWER, "1" },
	{ "image-zoom-200", N_("Zoom 200%"), GTH_SHORTCUT_CONTEXT_VIEWER, GTH_SHORTCUT_CATEGORY_IMAGE_VIEWER, "2" },
	{ "image-zoom-300", N_("Zoom 300%"), GTH_SHORTCUT_CONTEXT_VIEWER, GTH_SHORTCUT_CATEGORY_IMAGE_VIEWER, "3" },

	{ "image-zoom-fit", N_("Zoom to fit"), GTH_SHORTCUT_CONTEXT_VIEWER, GTH_SHORTCUT_CATEGORY_IMAGE_VIEWER, "<Shift>x" },
	{ "image-zoom-fit-if-larger", N_("Zoom to fit if larger"), GTH_SHORTCUT_CONTEXT_VIEWER, GTH_SHORTCUT_CATEGORY_IMAGE_VIEWER, "x" },
	{ "image-zoom-fit-width", N_("Zoom to fit width"), GTH_SHORTCUT_CONTEXT_VIEWER, GTH_SHORTCUT_CATEGORY_IMAGE_VIEWER, "<Shift>w" },
	{ "image-zoom-fit-width-if-larger", N_("Zoom to fit width if larger"), GTH_SHORTCUT_CONTEXT_VIEWER, GTH_SHORTCUT_CATEGORY_IMAGE_VIEWER, "w" },
	{ "image-zoom-fit-height", N_("Zoom to fit height"), GTH_SHORTCUT_CONTEXT_VIEWER, GTH_SHORTCUT_CATEGORY_IMAGE_VIEWER, "<Shift>h" },
	{ "image-zoom-fit-height-if-larger", N_("Zoom to fit height if larger"), GTH_SHORTCUT_CONTEXT_VIEWER, GTH_SHORTCUT_CATEGORY_IMAGE_VIEWER, "h" },

	{ "image-undo", N_("Undo edit"), GTH_SHORTCUT_CONTEXT_VIEWER, GTH_SHORTCUT_CATEGORY_IMAGE_EDITOR, "<Primary>z" },
	{ "image-redo", N_("Redo edit"), GTH_SHORTCUT_CONTEXT_VIEWER, GTH_SHORTCUT_CATEGORY_IMAGE_EDITOR, "<Primary><Shift>z" },

	{ "scroll-step-left", N_("Scroll left"), GTH_SHORTCUT_CONTEXT_VIEWER, GTH_SHORTCUT_CATEGORY_SCROLL_IMAGE, "Left" },
	{ "scroll-step-right", N_("Scroll right"), GTH_SHORTCUT_CONTEXT_VIEWER, GTH_SHORTCUT_CATEGORY_SCROLL_IMAGE, "Right" },
	{ "scroll-step-up", N_("Scroll up"), GTH_SHORTCUT_CONTEXT_VIEWER, GTH_SHORTCUT_CATEGORY_SCROLL_IMAGE, "Up" },
	{ "scroll-step-down", N_("Scroll down"), GTH_SHORTCUT_CONTEXT_VIEWER, GTH_SHORTCUT_CATEGORY_SCROLL_IMAGE, "Down" },

	{ "scroll-page-left", N_("Scroll left fast"), GTH_SHORTCUT_CONTEXT_VIEWER, GTH_SHORTCUT_CATEGORY_SCROLL_IMAGE, "<Shift>Left" },
	{ "scroll-page-right", N_("Scroll right fast"), GTH_SHORTCUT_CONTEXT_VIEWER, GTH_SHORTCUT_CATEGORY_SCROLL_IMAGE, "<Shift>Right" },
	{ "scroll-page-up", N_("Scroll up fast"), GTH_SHORTCUT_CONTEXT_VIEWER, GTH_SHORTCUT_CATEGORY_SCROLL_IMAGE, "<Shift>Up" },
	{ "scroll-page-down", N_("Scroll down fast"), GTH_SHORTCUT_CONTEXT_VIEWER, GTH_SHORTCUT_CATEGORY_SCROLL_IMAGE, "<Shift>Down" },

	{ "scroll-to-center", N_("Scroll to center"), GTH_SHORTCUT_CONTEXT_VIEWER, GTH_SHORTCUT_CATEGORY_SCROLL_IMAGE, "<Alt>Down" },
};


void
image_viewer__gth_browser_construct_cb (GthBrowser *browser)
{
	g_return_if_fail (GTH_IS_BROWSER (browser));

	gth_window_add_viewer_shortcuts (GTH_WINDOW (browser),
					 GTH_SHORTCUT_VIEWER_CONTEXT_IMAGE,
					 shortcuts,
					 G_N_ELEMENTS (shortcuts));
}
