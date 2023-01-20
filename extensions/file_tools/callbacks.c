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


#include <config.h>
#include <glib/gi18n.h>
#include <glib-object.h>
#include <gdk/gdkkeysyms.h>
#include <gthumb.h>
#include <extensions/image_viewer/gth-image-viewer-page.h>
#include <extensions/image_viewer/shortcuts.h>
#include "actions.h"
#include "callbacks.h"
#include "gth-file-tool-adjust-contrast.h"
#include "gth-file-tool-crop.h"
#include "gth-file-tool-flip.h"
#include "gth-file-tool-mirror.h"
#include "gth-file-tool-resize.h"
#include "gth-file-tool-rotate-left.h"
#include "gth-file-tool-rotate-right.h"


static const GActionEntry actions[] = {
	{ "file-tool-adjust-contrast", gth_browser_activate_tool_adjust_contrast },
	{ "file-tool-flip", gth_browser_activate_tool_flip },
	{ "file-tool-mirror", gth_browser_activate_tool_mirror },
	{ "file-tool-rotate-right", gth_browser_activate_tool_rotate_right },
	{ "file-tool-rotate-left", gth_browser_activate_tool_rotate_left },
	{ "file-tool-crop", gth_browser_activate_tool_crop },
	{ "file-tool-resize", gth_browser_activate_tool_resize },
};


static const GthShortcut shortcuts[] = {
	{ "file-tool-adjust-contrast", N_("Adjust contrast"), GTH_SHORTCUT_CONTEXT_BROWSER_VIEWER, GTH_SHORTCUT_CATEGORY_IMAGE_EDITOR, "a" },
	{ "file-tool-flip", N_("Flip"), GTH_SHORTCUT_CONTEXT_BROWSER_VIEWER, GTH_SHORTCUT_CATEGORY_IMAGE_EDITOR, "l" },
	{ "file-tool-mirror", N_("Mirror"), GTH_SHORTCUT_CONTEXT_BROWSER_VIEWER, GTH_SHORTCUT_CATEGORY_IMAGE_EDITOR, "m" },
	{ "file-tool-rotate-right", N_("Rotate right"), GTH_SHORTCUT_CONTEXT_BROWSER_VIEWER, GTH_SHORTCUT_CATEGORY_IMAGE_EDITOR, "r" },
	{ "file-tool-rotate-left", N_("Rotate left"), GTH_SHORTCUT_CONTEXT_BROWSER_VIEWER, GTH_SHORTCUT_CATEGORY_IMAGE_EDITOR, "<Shift>r" },
	{ "file-tool-crop", N_("Crop"), GTH_SHORTCUT_CONTEXT_BROWSER_VIEWER, GTH_SHORTCUT_CATEGORY_IMAGE_EDITOR, "<Shift>c" },
	{ "file-tool-resize", N_("Resize"), GTH_SHORTCUT_CONTEXT_BROWSER_VIEWER, GTH_SHORTCUT_CATEGORY_IMAGE_EDITOR, "<Shift>s" },
};


void
file_tools__gth_browser_construct_cb (GthBrowser *browser)
{
	g_action_map_add_action_entries (G_ACTION_MAP (browser),
					 actions,
					 G_N_ELEMENTS (actions),
					 browser);

	gth_window_add_viewer_shortcuts (GTH_WINDOW (browser),
					 GTH_SHORTCUT_VIEWER_CONTEXT_IMAGE,
					 shortcuts,
					 G_N_ELEMENTS (shortcuts));
}
