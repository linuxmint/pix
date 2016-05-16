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
#include "gth-file-tool-adjust-contrast.h"
#include "gth-file-tool-crop.h"
#include "gth-file-tool-flip.h"
#include "gth-file-tool-mirror.h"
#include "gth-file-tool-resize.h"
#include "gth-file-tool-rotate-left.h"
#include "gth-file-tool-rotate-right.h"


gpointer
file_tools__gth_browser_file_list_key_press_cb (GthBrowser  *browser,
						GdkEventKey *event)
{
	gpointer     result = NULL;
	GtkWidget   *sidebar;
	GtkWidget   *toolbox;
	GthFileTool *tool = NULL;
	guint        modifiers;
	GtkWidget   *page;

	sidebar = gth_browser_get_viewer_sidebar (browser);
	toolbox = gth_sidebar_get_toolbox (GTH_SIDEBAR (sidebar));
	if (gth_toolbox_tool_is_active (GTH_TOOLBOX (toolbox)))
		return NULL;

	modifiers = gtk_accelerator_get_default_mod_mask ();
	if (((event->state & modifiers) != 0) && ((event->state & modifiers) != GDK_SHIFT_MASK))
		return NULL;

	page = gth_browser_get_viewer_page (browser);
	if (! GTH_IS_IMAGE_VIEWER_PAGE (page))
		return NULL;

	if (gth_window_get_current_page (GTH_WINDOW (browser)) == GTH_BROWSER_PAGE_VIEWER
	    && ! gtk_widget_has_focus (gth_image_viewer_page_get_image_viewer (GTH_IMAGE_VIEWER_PAGE (page))))
	{
		return NULL;
	}

	switch (event->keyval) {
	case GDK_KEY_h:
		tool = (GthFileTool *) gth_toolbox_get_tool (GTH_TOOLBOX (toolbox), GTH_TYPE_FILE_TOOL_ADJUST_CONTRAST);
		break;
	case GDK_KEY_l:
		tool = (GthFileTool *) gth_toolbox_get_tool (GTH_TOOLBOX (toolbox), GTH_TYPE_FILE_TOOL_FLIP);
		break;
	case GDK_KEY_m:
		tool = (GthFileTool *) gth_toolbox_get_tool (GTH_TOOLBOX (toolbox), GTH_TYPE_FILE_TOOL_MIRROR);
		break;
	case GDK_KEY_r:
		tool = (GthFileTool *) gth_toolbox_get_tool (GTH_TOOLBOX (toolbox), GTH_TYPE_FILE_TOOL_ROTATE_RIGHT);
		break;
	case GDK_KEY_R:
		tool = (GthFileTool *) gth_toolbox_get_tool (GTH_TOOLBOX (toolbox), GTH_TYPE_FILE_TOOL_ROTATE_LEFT);
		break;
	case GDK_KEY_C:
		gth_browser_show_viewer_tools (browser);
		tool = (GthFileTool *) gth_toolbox_get_tool (GTH_TOOLBOX (toolbox), GTH_TYPE_FILE_TOOL_CROP);
		break;
	case GDK_KEY_S:
		gth_browser_show_viewer_tools (browser);
		tool = (GthFileTool *) gth_toolbox_get_tool (GTH_TOOLBOX (toolbox), GTH_TYPE_FILE_TOOL_RESIZE);
		break;
	}

	if (tool != NULL) {
		if (gth_window_get_current_page (GTH_WINDOW (browser)) == GTH_BROWSER_PAGE_BROWSER)
			gth_window_set_current_page (GTH_WINDOW (browser), GTH_BROWSER_PAGE_VIEWER);
		gth_file_tool_activate (tool);
		result = GINT_TO_POINTER (1);
	}

	return result;
}
