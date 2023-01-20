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
#include "actions.h"
#include "callbacks.h"


static const GActionEntry actions[] = {
	{ "print", gth_browser_activate_print }
};


static const GthShortcut shortcuts[] = {
	{ "print", N_("Print"), GTH_SHORTCUT_CONTEXT_BROWSER_VIEWER, GTH_SHORTCUT_CATEGORY_FILE_MANAGER, "<Primary>p" },
};


void
ip__gth_browser_construct_cb (GthBrowser *browser)
{
	g_return_if_fail (GTH_IS_BROWSER (browser));

	g_action_map_add_action_entries (G_ACTION_MAP (browser),
					 actions,
					 G_N_ELEMENTS (actions),
					 browser);
	gth_menu_manager_append_entry (gth_browser_get_menu_manager (browser, GTH_BROWSER_MENU_MANAGER_FILE_OPEN_ACTIONS),
				       GTH_MENU_MANAGER_NEW_MERGE_ID,
				       _("Print"),
				       "win.print",
				       NULL,
				       NULL);
	gth_menu_manager_append_entry (gth_browser_get_menu_manager (browser, GTH_BROWSER_MENU_MANAGER_FILE_LIST_OPEN_ACTIONS),
				       GTH_MENU_MANAGER_NEW_MERGE_ID,
				       _("Print"),
				       "win.print",
				       NULL,
				       NULL);
	gth_window_add_shortcuts (GTH_WINDOW (browser),
				  shortcuts,
				  G_N_ELEMENTS (shortcuts));
}


void
ip__gth_browser_selection_changed_cb (GthBrowser *browser,
				      int         n_selected)
{
	g_object_set (g_action_map_lookup_action (G_ACTION_MAP (browser), "print"), "enabled", n_selected > 0, NULL);
}
