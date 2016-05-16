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
#include "gth-file-source-selections.h"
#include "gth-selections-manager.h"


#define BROWSER_DATA_KEY "selections-browser-data"


static const char *fixed_ui_info =
"<ui>"
"  <popup name='FileListPopup'>"
"    <placeholder name='Folder_Actions2'>"
"      <menu action='Edit_AddToSelection'>"
"        <menuitem action='Edit_AddToSelection_1'/>"
"        <menuitem action='Edit_AddToSelection_2'/>"
"        <menuitem action='Edit_AddToSelection_3'/>"
"      </menu>"
"    </placeholder>"
"  </popup>"
"  <popup name='FilePopup'>"
"    <placeholder name='Folder_Actions2'>"
"      <menu action='Edit_AddToSelection'>"
"        <menuitem action='Edit_AddToSelection_1'/>"
"        <menuitem action='Edit_AddToSelection_2'/>"
"        <menuitem action='Edit_AddToSelection_3'/>"
"      </menu>"
"    </placeholder>"
"  </popup>"
"  <accelerator action=\"Go_Selection_1\" />"
"  <accelerator action=\"Go_Selection_2\" />"
"  <accelerator action=\"Go_Selection_3\" />"
"</ui>";


static const char *vfs_ui_info =
"<ui>"
"  <popup name='FileListPopup'>"
"    <placeholder name='Open_Actions'>"
"      <menuitem action='Go_FileContainer'/>"
"    </placeholder>"
"  </popup>"
"  <popup name='FilePopup'>"
"    <placeholder name='Open_Actions'>"
"      <menuitem action='Go_FileContainer'/>"
"    </placeholder>"
"  </popup>"
"</ui>";


static GthActionEntryExt selections_action_entries[] = {
	{ "Edit_AddToSelection", GTK_STOCK_ADD, N_("Add to _Selection") },

        { "Go_FileContainer", GTK_STOCK_JUMP_TO,
          N_("Open _Folder"), "<alt>End",
          N_("Go to the folder that contains the selected file"),
       	  GTH_ACTION_FLAG_NONE,
          G_CALLBACK (gth_browser_activate_action_selection_go_to_container) },

	{ "Edit_AddToSelection_1", "selection1",
	  N_("Selection 1"), NULL,
	  NULL,
	  GTH_ACTION_FLAG_ALWAYS_SHOW_IMAGE,
	  G_CALLBACK (gth_browser_activate_action_add_to_selection_1) },
	{ "Edit_AddToSelection_2", "selection2",
	  N_("Selection 2"), NULL,
	  NULL,
	  GTH_ACTION_FLAG_ALWAYS_SHOW_IMAGE,
	  G_CALLBACK (gth_browser_activate_action_add_to_selection_2) },
	{ "Edit_AddToSelection_3", "selection3",
	  N_("Selection 3"), NULL,
	  NULL,
	  GTH_ACTION_FLAG_ALWAYS_SHOW_IMAGE,
	  G_CALLBACK (gth_browser_activate_action_add_to_selection_3) },
	{ "Go_Selection_1", NULL,
	  NULL, "<control>1",
	  NULL,
	  GTH_ACTION_FLAG_NONE,
	  G_CALLBACK (gth_browser_activate_action_go_selection_1) },
	{ "Go_Selection_2", NULL,
	  NULL, "<control>2",
	  NULL,
	  GTH_ACTION_FLAG_NONE,
	  G_CALLBACK (gth_browser_activate_action_go_selection_2) },
	{ "Go_Selection_3", NULL,
	  NULL, "<control>3",
	  NULL,
	  GTH_ACTION_FLAG_NONE,
	  G_CALLBACK (gth_browser_activate_action_go_selection_3) }

};
static guint selections_action_entries_size = G_N_ELEMENTS (selections_action_entries);


typedef struct {
	GthBrowser     *browser;
	GtkActionGroup *actions;
	guint           vfs_merge_id;
} BrowserData;


static void
browser_data_free (BrowserData *data)
{
	g_free (data);
}


void
selections__gth_browser_construct_cb (GthBrowser *browser)
{
	BrowserData *data;
	GError      *error = NULL;

	g_return_if_fail (GTH_IS_BROWSER (browser));

	data = g_new0 (BrowserData, 1);
	g_object_set_data_full (G_OBJECT (browser), BROWSER_DATA_KEY, data, (GDestroyNotify) browser_data_free);

	data->browser = browser;

	data->actions = gtk_action_group_new ("Selections Actions");
	gtk_action_group_set_translation_domain (data->actions, NULL);
	_gtk_action_group_add_actions_with_flags (data->actions,
						  selections_action_entries,
						  selections_action_entries_size,
						  browser);
	gtk_ui_manager_insert_action_group (gth_browser_get_ui_manager (browser), data->actions, 0);

	if (! gtk_ui_manager_add_ui_from_string (gth_browser_get_ui_manager (browser), fixed_ui_info, -1, &error)) {
		g_message ("building menus failed: %s", error->message);
		g_error_free (error);
	}
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
				gth_browser_activate_action_remove_from_selection (browser, keyval - GDK_KEY_1 + 1);
			else /* Alt+n => add to selection n */
				gth_browser_activate_action_add_to_selection (browser, keyval - GDK_KEY_1 + 1);
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
			gth_browser_activate_action_show_selection (browser, keyval - GDK_KEY_1 + 1);
			result = GINT_TO_POINTER (1);
			break;
		}
	}

	return result;
}


void
selections__gth_browser_load_location_after_cb (GthBrowser   *browser,
						GthFileData  *location_data,
						const GError *error)
{
	BrowserData *data;

	if ((location_data == NULL) || (error != NULL))
		return;

	data = g_object_get_data (G_OBJECT (browser), BROWSER_DATA_KEY);

	if (GTH_IS_FILE_SOURCE_SELECTIONS (gth_browser_get_location_source (browser))) {
		if (data->vfs_merge_id == 0) {
			GError *error = NULL;

			data->vfs_merge_id = gtk_ui_manager_add_ui_from_string (gth_browser_get_ui_manager (browser), vfs_ui_info, -1, &error);
			if (data->vfs_merge_id == 0) {
				g_message ("building menus failed: %s", error->message);
				g_error_free (error);
			}
		}
	}
	else {
		if (data->vfs_merge_id != 0) {
			gtk_ui_manager_remove_ui (gth_browser_get_ui_manager (browser), data->vfs_merge_id);
			data->vfs_merge_id = 0;
		}
	}
}


void
selections__gth_browser_update_extra_widget_cb (GthBrowser *browser)
{
	GthFileData *location_data;
	GtkWidget   *extra_widget;
	int          n_selection;
	char        *msg;

	location_data = gth_browser_get_location_data (browser);
	if (! _g_content_type_is_a (g_file_info_get_content_type (location_data->info), "gthumb/selection"))
		return;

	n_selection = g_file_info_get_attribute_int32 (location_data->info, "gthumb::n-selection");
	extra_widget = gth_browser_get_list_extra_widget (browser);
	gth_embedded_dialog_set_gicon (GTH_EMBEDDED_DIALOG (extra_widget), g_file_info_get_icon (location_data->info), GTK_ICON_SIZE_DIALOG);
	gth_embedded_dialog_set_primary_text (GTH_EMBEDDED_DIALOG (extra_widget), g_file_info_get_display_name (location_data->info));
	if (n_selection > 0)
		msg = g_strdup_printf (_("Use Alt-%d to add files to this selection, Ctrl-%d to view this selection."), n_selection, n_selection);
	else
		msg = NULL;
	gth_embedded_dialog_set_secondary_text (GTH_EMBEDDED_DIALOG (extra_widget), msg);

	g_free (msg);
}
