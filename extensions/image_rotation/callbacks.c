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
#include <extensions/image_viewer/image-viewer.h>
#include <extensions/list_tools/list-tools.h>
#include <extensions/list_tools/shortcuts.h>
#include "actions.h"
#include "callbacks.h"


#define BROWSER_DATA_KEY "image-rotation-browser-data"


static const GActionEntry actions[] = {
	{ "rotate-right", gth_browser_activate_rotate_right },
	{ "rotate-left", gth_browser_activate_rotate_left },
	{ "apply-orientation", gth_browser_activate_apply_orientation },
	{ "reset-orientation", gth_browser_activate_reset_orientation }
};


static const GthShortcut shortcuts[] = {
	{ "rotate-right", N_("Rotate right"), GTH_SHORTCUT_CONTEXT_BROWSER_VIEWER, GTH_SHORTCUT_CATEGORY_LIST_TOOLS, "bracketright" },
	{ "rotate-left", N_("Rotate left"), GTH_SHORTCUT_CONTEXT_BROWSER_VIEWER, GTH_SHORTCUT_CATEGORY_LIST_TOOLS, "bracketleft" },
};


static const GthMenuEntry tools1_action_entries[] = {
	{ N_("Rotate Left"), "win.rotate-left", NULL, "object-rotate-left-symbolic" },
	{ N_("Rotate Right"), "win.rotate-right", NULL, "object-rotate-right-symbolic" },
};


static const GthMenuEntry tools2_action_entries[] = {
	{ N_("Rotate Physically"), "win.apply-orientation" },
	{ N_("Reset the EXIF Orientation"), "win.reset-orientation" }
};


typedef struct {
	GtkWidget *buttons[2];
	gulong     image_changed_id;
} BrowserData;


static void
browser_data_free (BrowserData *data)
{
	g_free (data);
}


void
ir__gth_browser_construct_cb (GthBrowser *browser)
{
	BrowserData *data;

	g_return_if_fail (GTH_IS_BROWSER (browser));

	data = g_new0 (BrowserData, 1);
	g_object_set_data_full (G_OBJECT (browser), BROWSER_DATA_KEY, data, (GDestroyNotify) browser_data_free);
	data->buttons[0] = NULL;
	data->buttons[1] = NULL;
	data->image_changed_id = 0;

	g_action_map_add_action_entries (G_ACTION_MAP (browser),
					 actions,
					 G_N_ELEMENTS (actions),
					 browser);

	gth_window_add_shortcuts (GTH_WINDOW (browser),
				  shortcuts,
				  G_N_ELEMENTS (shortcuts));

	gth_menu_manager_append_entries (gth_browser_get_menu_manager (browser, GTH_BROWSER_MENU_MANAGER_TOOLS),
					 tools1_action_entries,
					 G_N_ELEMENTS (tools1_action_entries));
	gth_menu_manager_append_entries (gth_browser_get_menu_manager (browser, GTH_BROWSER_MENU_MANAGER_MORE_TOOLS),
					 tools2_action_entries,
					 G_N_ELEMENTS (tools2_action_entries));
}


void
ir__gth_browser_selection_changed_cb (GthBrowser *browser,
				      int         n_selected)
{
	gboolean sensitive;

	sensitive = n_selected > 0;
	gth_window_enable_action (GTH_WINDOW (browser), "rotate-right", sensitive);
	gth_window_enable_action (GTH_WINDOW (browser), "rotate-left", sensitive);
	gth_window_enable_action (GTH_WINDOW (browser), "apply-orientation", sensitive);
	gth_window_enable_action (GTH_WINDOW (browser), "reset-orientation", sensitive);
}


static void
viewer_image_changed_cb  (GtkWidget  *widget,
		          GthBrowser *browser)
{
	BrowserData *data;

	data = g_object_get_data (G_OBJECT (browser), BROWSER_DATA_KEY);
	if (data == NULL)
		return;

	if ((data->buttons[0] != NULL) && (data->buttons[1] != NULL)) {
		GthViewerPage *viewer_page = gth_browser_get_viewer_page (browser);
		gboolean       visible = FALSE;

		if (GTH_IS_IMAGE_VIEWER_PAGE (viewer_page)) {
			GtkWidget *image_viewer = gth_image_viewer_page_get_image_viewer (GTH_IMAGE_VIEWER_PAGE (viewer_page));
			visible = ! gth_image_viewer_is_animation (GTH_IMAGE_VIEWER (image_viewer));
		}

		gtk_widget_set_visible (data->buttons[0], visible);
		gtk_widget_set_visible (data->buttons[1], visible);
	}
}


void
ir__gth_browser_activate_viewer_page_cb (GthBrowser *browser)
{
	BrowserData   *data;
	GthViewerPage *viewer_page;

	data = g_object_get_data (G_OBJECT (browser), BROWSER_DATA_KEY);
	g_return_if_fail (data != NULL);

	viewer_page = gth_browser_get_viewer_page (browser);
	if (GTH_IS_IMAGE_VIEWER_PAGE (viewer_page)) {
		if (data->buttons[0] == NULL)
			data->buttons[0] =
					gth_browser_add_header_bar_button (browser,
									   GTH_BROWSER_HEADER_SECTION_VIEWER_COMMANDS,
									   "object-rotate-left-symbolic",
									   _("Rotate Left"),
									   "win.rotate-left",
									   NULL);
		if (data->buttons[1] == NULL)
			data->buttons[1] =
					gth_browser_add_header_bar_button (browser,
									   GTH_BROWSER_HEADER_SECTION_VIEWER_COMMANDS,
									   "object-rotate-right-symbolic",
									   _("Rotate Right"),
									   "win.rotate-right",
									   NULL);
		if (data->image_changed_id == 0)
			data->image_changed_id = g_signal_connect (gth_image_viewer_page_get_image_viewer (GTH_IMAGE_VIEWER_PAGE (viewer_page)),
								   "image-changed",
								   G_CALLBACK (viewer_image_changed_cb),
								   browser);
	}
}


void
ir__gth_browser_deactivate_viewer_page_cb (GthBrowser *browser)
{
	BrowserData   *data;
	GthViewerPage *viewer_page;

	data = g_object_get_data (G_OBJECT (browser), BROWSER_DATA_KEY);
	g_return_if_fail (data != NULL);

	viewer_page = gth_browser_get_viewer_page (browser);
	if (GTH_IS_IMAGE_VIEWER_PAGE (viewer_page)) {
		if (data->image_changed_id != 0) {
			g_signal_handler_disconnect (gth_image_viewer_page_get_image_viewer (GTH_IMAGE_VIEWER_PAGE (viewer_page)), data->image_changed_id);
			data->image_changed_id = 0;
		}
		if (data->buttons[0] != NULL) {
			gtk_widget_destroy (data->buttons[0]);
			data->buttons[0] = NULL;
		}
		if (data->buttons[1] != NULL) {
			gtk_widget_destroy (data->buttons[1]);
			data->buttons[1] = NULL;
		}
	}
}
