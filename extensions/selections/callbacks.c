/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2012 Free Software Foundation, Inc.
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
#include "actions.h"
#include "callbacks.h"
#include "gth-file-source-selections.h"
#include "gth-selections-manager.h"
#include "shortcuts.h"


#define BROWSER_DATA_KEY "selections-browser-data"
#define N_SELECTIONS 3


static const GActionEntry actions[] = {
	{ "add-to-selection-1", gth_browser_activate_add_to_selection, "i", "1" },
	{ "add-to-selection-2", gth_browser_activate_add_to_selection, "i", "2" },
	{ "add-to-selection-3", gth_browser_activate_add_to_selection, "i", "3" },

	{ "go-to-selection-1", gth_browser_activate_go_to_selection, "i", "1" },
	{ "go-to-selection-2", gth_browser_activate_go_to_selection, "i", "2" },
	{ "go-to-selection-3", gth_browser_activate_go_to_selection, "i", "3" },

	{ "remove-to-selection-1", gth_browser_activate_remove_from_selection, "i", "1" },
	{ "remove-to-selection-2", gth_browser_activate_remove_from_selection, "i", "2" },
	{ "remove-to-selection-3", gth_browser_activate_remove_from_selection, "i", "3" },

	{ "go-to-container-from-selection", gth_browser_activate_go_to_file_container },
	{ "remove-from-selection", gth_browser_activate_remove_from_current_selection }
};


static const GthShortcut shortcuts[] = {
	{ "add-to-selection-1", N_("Add to selection"), GTH_SHORTCUT_CONTEXT_BROWSER_VIEWER | GTH_SHORTCUT_CONTEXT_INTERNAL, GTH_SHORTCUT_CATEGORY_HIDDEN, "<Alt>1" },
	{ "add-to-selection-2", N_("Add to selection"), GTH_SHORTCUT_CONTEXT_BROWSER_VIEWER | GTH_SHORTCUT_CONTEXT_INTERNAL, GTH_SHORTCUT_CATEGORY_HIDDEN, "<Alt>2" },
	{ "add-to-selection-3", N_("Add to selection"), GTH_SHORTCUT_CONTEXT_BROWSER_VIEWER | GTH_SHORTCUT_CONTEXT_INTERNAL, GTH_SHORTCUT_CATEGORY_HIDDEN, "<Alt>3" },

	{ "remove-from-selection-1", N_("Remove from selection"), GTH_SHORTCUT_CONTEXT_BROWSER_VIEWER | GTH_SHORTCUT_CONTEXT_INTERNAL, GTH_SHORTCUT_CATEGORY_HIDDEN, "<Shift><Alt>1" },
	{ "remove-from-selection-2", N_("Remove from selection"), GTH_SHORTCUT_CONTEXT_BROWSER_VIEWER | GTH_SHORTCUT_CONTEXT_INTERNAL, GTH_SHORTCUT_CATEGORY_HIDDEN, "<Shift><Alt>2" },
	{ "remove-from-selection-3", N_("Remove from selection"), GTH_SHORTCUT_CONTEXT_BROWSER_VIEWER | GTH_SHORTCUT_CONTEXT_INTERNAL, GTH_SHORTCUT_CATEGORY_HIDDEN, "<Shift><Alt>3" },

	{ "go-to-selection-1", N_("Show selection"), GTH_SHORTCUT_CONTEXT_BROWSER_VIEWER | GTH_SHORTCUT_CONTEXT_INTERNAL, GTH_SHORTCUT_CATEGORY_HIDDEN, "<Primary>1" },
	{ "go-to-selection-2", N_("Show selection"), GTH_SHORTCUT_CONTEXT_BROWSER_VIEWER | GTH_SHORTCUT_CONTEXT_INTERNAL, GTH_SHORTCUT_CATEGORY_HIDDEN, "<Primary>2" },
	{ "go-to-selection-3", N_("Show selection"), GTH_SHORTCUT_CONTEXT_BROWSER_VIEWER | GTH_SHORTCUT_CONTEXT_INTERNAL, GTH_SHORTCUT_CATEGORY_HIDDEN, "<Primary>3" },

	/* Not real actions, used in the shorcut window for documentation. */

	{ "add-to-selection-doc", N_("Add to selection"), GTH_SHORTCUT_CONTEXT_BROWSER_VIEWER | GTH_SHORTCUT_CONTEXT_DOC, GTH_SHORTCUT_CATEGORY_SELECTIONS, "<Alt>1...3" },
	{ "remove-from-selection-doc", N_("Remove from selection"), GTH_SHORTCUT_CONTEXT_BROWSER_VIEWER | GTH_SHORTCUT_CONTEXT_DOC, GTH_SHORTCUT_CATEGORY_SELECTIONS, "<Shift><Alt>1...3" },
	{ "go-to-selection-doc", N_("Show selection"), GTH_SHORTCUT_CONTEXT_BROWSER_VIEWER | GTH_SHORTCUT_CONTEXT_DOC, GTH_SHORTCUT_CATEGORY_SELECTIONS, "<Primary>1...3" },
};


static const GthMenuEntry file_list_popup_open_entries[] = {
	{ N_("Open Folder"), "win.go-to-container-from-selection" },
};


static const GthMenuEntry file_list_popup_delete_entries[] = {
	{ N_("Remove from Selection"), "win.remove-from-selection" },
};


typedef struct {
	GthBrowser     *browser;
	guint           vfs_merge_open_id;
	guint           vfs_merge_delete_id;
	GtkWidget      *selection_buttons[N_SELECTIONS];
	gulong          folder_changed_id;
} BrowserData;


static void
browser_data_free (BrowserData *data)
{
	g_signal_handler_disconnect (gth_main_get_default_monitor (), data->folder_changed_id);
	g_free (data);
}


static void
selection_clicked_cb (GtkWidget *button,
		      gpointer   user_data)
{
	BrowserData *data = user_data;
	int          n_selection = 0;

	for (n_selection = 0; n_selection < N_SELECTIONS; n_selection++)
		if (button == data->selection_buttons[n_selection])
			break;

	g_return_if_fail (n_selection >= 0 && n_selection <= N_SELECTIONS - 1);

	gth_browser_show_selection (data->browser, n_selection + 1);
}


static GtkWidget *
_selection_button_new (int      n_selection,
		       gpointer user_data)
{
	GtkWidget *button;
	char      *tooltip;

	tooltip = g_strdup_printf (_("Show selection %d"), n_selection);

	button = gtk_button_new ();
	gtk_container_add (GTK_CONTAINER (button), gtk_image_new_from_icon_name (gth_selection_get_icon_name (n_selection), GTK_ICON_SIZE_MENU));
	gtk_button_set_relief (GTK_BUTTON (button), GTK_RELIEF_NONE);
	gtk_widget_show_all (button);
	gtk_widget_set_sensitive (button, ! gth_selections_manager_get_is_empty (n_selection));
	gtk_widget_set_tooltip_text (button, tooltip);

	g_signal_connect (button,
			  "clicked",
			  G_CALLBACK (selection_clicked_cb),
			  user_data);

	g_free (tooltip);

	return button;
}


static GtkWidget *
create_selection_buttons (BrowserData *data)
{
	GtkWidget *box;
	int        i;

	box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_widget_show (box);

	for (i = 0; i < N_SELECTIONS; i++) {
		data->selection_buttons[i] = _selection_button_new (i + 1, data);
		gtk_box_pack_start (GTK_BOX (box), data->selection_buttons[i], FALSE, FALSE, 0);
	}

	return box;
}


static void
folder_changed_cb (GthMonitor      *monitor,
		   GFile           *parent,
		   GList           *list /* GFile list */,
		   int              position,
		   GthMonitorEvent  event,
		   gpointer         user_data)
{
	BrowserData *data = user_data;
	int          n_selection;

	if (event == GTH_MONITOR_EVENT_CHANGED)
		return;

	if (! g_file_has_uri_scheme (parent, "selection"))
		return;

	n_selection = _g_file_get_n_selection (parent);
	if (n_selection <= 0)
		return;

	gtk_widget_set_sensitive (data->selection_buttons[n_selection - 1], ! gth_selections_manager_get_is_empty (n_selection));
}


void
selections__gth_browser_construct_cb (GthBrowser *browser)
{
	BrowserData *data;
	GtkWidget   *filter_bar;
	GtkWidget   *filter_bar_extra_area;
	GtkWidget   *selection_buttons;

	g_return_if_fail (GTH_IS_BROWSER (browser));

	data = g_new0 (BrowserData, 1);
	g_object_set_data_full (G_OBJECT (browser), BROWSER_DATA_KEY, data, (GDestroyNotify) browser_data_free);

	data->browser = browser;

	g_action_map_add_action_entries (G_ACTION_MAP (browser),
					 actions,
					 G_N_ELEMENTS (actions),
					 browser);
	gth_window_add_shortcuts (GTH_WINDOW (browser),
				  shortcuts,
				  G_N_ELEMENTS (shortcuts));

	filter_bar = gth_browser_get_filterbar (browser);
	filter_bar_extra_area = gth_filterbar_get_extra_area (GTH_FILTERBAR (filter_bar));
	selection_buttons = create_selection_buttons (data);
	gtk_box_pack_start (GTK_BOX (filter_bar_extra_area), selection_buttons, FALSE, FALSE, 0);

	data->folder_changed_id = g_signal_connect (gth_main_get_default_monitor (),
						    "folder-changed",
						    G_CALLBACK (folder_changed_cb),
						    data);
}


void
selections__gth_browser_selection_changed_cb (GthBrowser *browser,
					      int         n_selected)
{
	BrowserData *data;

	if (! GTH_IS_FILE_SOURCE_SELECTIONS (gth_browser_get_location_source (browser)))
		return;

	data = g_object_get_data (G_OBJECT (browser), BROWSER_DATA_KEY);
	g_return_if_fail (data != NULL);

	gth_window_enable_action (GTH_WINDOW (browser), "go-to-container-from-selection", n_selected == 1);
}


static guint
get_numeric_keyval (GthBrowser  *browser,
		    GdkEventKey *event)
{
	guint keyval;

	gdk_keymap_translate_keyboard_state (gdk_keymap_get_for_display (gtk_widget_get_display (GTK_WIDGET (browser))),
					     event->hardware_keycode,
	                                     event->state & ~GDK_SHIFT_MASK,
	                                     event->group,
	                                     &keyval,
	                                     NULL, NULL, NULL);

	/* This fixes the keyboard shortcuts for French keyboards (and
	 * maybe others as well) where the number keys are shifted. */
	if ((keyval < GDK_KEY_1) || (keyval > GDK_KEY_3))
		gdk_keymap_translate_keyboard_state (gdk_keymap_get_for_display (gtk_widget_get_display (GTK_WIDGET (browser))),
						     event->hardware_keycode,
		                                     event->state | GDK_SHIFT_MASK,
		                                     event->group,
		                                     &keyval,
		                                     NULL, NULL, NULL);

	return keyval;
}


gpointer
selections__gth_browser_file_list_key_press_cb (GthBrowser  *browser,
						GdkEventKey *event)
{
	gpointer result = NULL;
	guint    modifiers;

	modifiers = gtk_accelerator_get_default_mod_mask ();
	if (((event->state & modifiers) == GDK_MOD1_MASK)
	    || ((event->state & modifiers) == (GDK_SHIFT_MASK|GDK_MOD1_MASK)))
	{
		guint keyval;

		keyval = get_numeric_keyval (browser, event);
		switch (keyval) {
		case GDK_KEY_1:
		case GDK_KEY_2:
		case GDK_KEY_3:
			/* Alt+Shift+n => remove from selection n */
			if ((event->state & modifiers) == (GDK_SHIFT_MASK|GDK_MOD1_MASK))
				gth_browser_remove_from_selection (browser, keyval - GDK_KEY_1 + 1);
			else /* Alt+n => add to selection n */
				gth_browser_add_to_selection (browser, keyval - GDK_KEY_1 + 1);
			result = GINT_TO_POINTER (1);
			break;
		}
	}

	if ((event->state & modifiers) == GDK_CONTROL_MASK) {
		guint keyval;

		keyval = get_numeric_keyval (browser, event);
		switch (keyval) {
		case GDK_KEY_1:
		case GDK_KEY_2:
		case GDK_KEY_3:
			/* Control+n => go to selection n */
			gth_browser_show_selection (browser, keyval - GDK_KEY_1 + 1);
			result = GINT_TO_POINTER (1);
			break;
		}
	}

	return result;
}


void
selections__gth_browser_load_location_after_cb (GthBrowser   *browser,
						GthFileData  *location_data)
{
	BrowserData *data;

	data = g_object_get_data (G_OBJECT (browser), BROWSER_DATA_KEY);

	if (GTH_IS_FILE_SOURCE_SELECTIONS (gth_browser_get_location_source (browser))) {
		if (data->vfs_merge_open_id == 0)
			data->vfs_merge_open_id =
					gth_menu_manager_append_entries (gth_browser_get_menu_manager (browser, GTH_BROWSER_MENU_MANAGER_FILE_LIST_OPEN_ACTIONS),
									 file_list_popup_open_entries,
									 G_N_ELEMENTS (file_list_popup_open_entries));
		if (data->vfs_merge_delete_id == 0)
			data->vfs_merge_delete_id =
					gth_menu_manager_append_entries (gth_browser_get_menu_manager (browser, GTH_BROWSER_MENU_MANAGER_FILE_LIST_DELETE_ACTIONS),
									 file_list_popup_delete_entries,
									 G_N_ELEMENTS (file_list_popup_delete_entries));
	}
	else {
		gth_menu_manager_remove_entries (gth_browser_get_menu_manager (browser, GTH_BROWSER_MENU_MANAGER_FILE_LIST_OPEN_ACTIONS), data->vfs_merge_open_id);
		gth_menu_manager_remove_entries (gth_browser_get_menu_manager (browser, GTH_BROWSER_MENU_MANAGER_FILE_LIST_DELETE_ACTIONS), data->vfs_merge_delete_id);
		data->vfs_merge_open_id = 0;
		data->vfs_merge_delete_id = 0;
	}
}


void
selections__gth_browser_update_extra_widget_cb (GthBrowser *browser)
{
	GthFileData *location_data;
	GtkWidget   *info_bar;
	int          n_selection;
	char        *msg;

	location_data = gth_browser_get_location_data (browser);
	if (! _g_content_type_is_a (g_file_info_get_content_type (location_data->info), "gthumb/selection"))
		return;

	n_selection = g_file_info_get_attribute_int32 (location_data->info, "gthumb::n-selection");
	if (n_selection <= 0)
		return;

	info_bar = gth_browser_get_list_info_bar (browser);
	gtk_info_bar_set_message_type (GTK_INFO_BAR(info_bar), GTK_MESSAGE_INFO);
	gth_info_bar_set_icon_name (GTH_INFO_BAR (info_bar), "dialog-information-symbolic", GTK_ICON_SIZE_MENU);
	gth_info_bar_set_primary_text (GTH_INFO_BAR (info_bar), NULL);
	msg = g_strdup_printf (_("Use Alt-%d to add files to this selection, Ctrl-%d to view this selection."), n_selection, n_selection);
	gth_info_bar_set_secondary_text (GTH_INFO_BAR (info_bar), msg);
	gtk_widget_show (info_bar);

	g_free (msg);
}
