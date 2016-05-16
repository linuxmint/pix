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
#include <gthumb.h>
#include "actions.h"
#include "dlg-photo-importer.h"
#include "preferences.h"


#define BROWSER_DATA_KEY "photo-importer-browser-data"


static const char *ui_info =
"<ui>"
"  <menubar name='MenuBar'>"
"    <menu name='File' action='FileMenu'>"
"      <menu name='Import' action='ImportMenu'>"
"        <placeholder name='Misc_Actions'>"
"          <menuitem action='File_ImportFromDevice'/>"
"          <menuitem action='File_ImportFromFolder'/>"
"        </placeholder>"
"      </menu>"
"    </menu>"
"  </menubar>"
"</ui>";


static GtkActionEntry action_entries[] = {
	{ "File_ImportFromDevice", "camera-photo",
	  N_("_Removable Device..."), NULL,
	  N_("Import photos and other files from a removable device"),
	  G_CALLBACK (gth_browser_activate_action_import_from_device) },
	{ "File_ImportFromFolder", "folder",
	  N_("F_older..."), NULL,
	  N_("Import photos and other files from a folder"),
	  G_CALLBACK (gth_browser_activate_action_import_from_folder) }
};


typedef struct {
	GtkActionGroup *action_group;
} BrowserData;


static void
browser_data_free (BrowserData *data)
{
	g_free (data);
}


void
pi__gth_browser_construct_cb (GthBrowser *browser)
{
	BrowserData *data;
	GError      *error = NULL;
	guint        merge_id;

	g_return_if_fail (GTH_IS_BROWSER (browser));

	data = g_new0 (BrowserData, 1);

	data->action_group = gtk_action_group_new ("Photo Importer Actions");
	gtk_action_group_set_translation_domain (data->action_group, NULL);
	gtk_action_group_add_actions (data->action_group,
				      action_entries,
				      G_N_ELEMENTS (action_entries),
				      browser);
	gtk_ui_manager_insert_action_group (gth_browser_get_ui_manager (browser), data->action_group, 0);

	merge_id = gtk_ui_manager_add_ui_from_string (gth_browser_get_ui_manager (browser), ui_info, -1, &error);
	if (merge_id == 0) {
		g_warning ("building ui failed: %s", error->message);
		g_clear_error (&error);
	}

	g_object_set_data_full (G_OBJECT (browser), BROWSER_DATA_KEY, data, (GDestroyNotify) browser_data_free);
}


/* -- pi__import_photos_cb -- */


typedef struct {
	GthBrowser *browser;
	GFile      *source;
} ImportData;


static void
import_data_unref (gpointer user_data)
{
	ImportData *data = user_data;

	g_object_unref (data->browser);
	_g_object_unref (data->source);
	g_free (data);
}


static gboolean
import_photos_idle_cb (gpointer user_data)
{
	ImportData *data = user_data;

	dlg_photo_importer_from_device (data->browser, data->source);

	return FALSE;
}


void
pi__import_photos_cb (GthBrowser *browser,
		      GFile      *source)
{
	ImportData *data;

	data = g_new0 (ImportData, 1);
	data->browser = g_object_ref (browser);
	data->source = _g_object_ref (source);
	g_idle_add_full (G_PRIORITY_DEFAULT_IDLE,
			 import_photos_idle_cb,
			 data,
			 import_data_unref);
}


/* -- pi__dlg_preferences_construct_cb -- */


#define PREFERENCES_DATA_KEY "photo-import-preference-data"


typedef struct {
	GtkBuilder *builder;
	GSettings  *settings;
} PreferencesData;


static void
preferences_data_free (PreferencesData *data)
{
	g_object_unref (data->settings);
	g_object_unref (data->builder);
	g_free (data);
}


static void
adjust_orientation_checkbutton_toggled_cb (GtkToggleButton *button,
					   PreferencesData *data)
{
	g_settings_set_boolean (data->settings, PREF_PHOTO_IMPORTER_ADJUST_ORIENTATION, gtk_toggle_button_get_active (button));
}


void
pi__dlg_preferences_construct_cb (GtkWidget  *dialog,
				  GthBrowser *browser,
				  GtkBuilder *dialog_builder)
{
	PreferencesData *data;
	GtkWidget       *general_vbox;
	GtkWidget       *importer_options;
	GtkWidget       *widget;

	data = g_new0 (PreferencesData, 1);
	data->builder = _gtk_builder_new_from_file("photo-importer-options.ui", "photo_importer");
	data->settings = g_settings_new (GTHUMB_PHOTO_IMPORTER_SCHEMA);

	general_vbox = _gtk_builder_get_widget (dialog_builder, "general_vbox");
	importer_options = _gtk_builder_get_widget (data->builder, "importer_options");
	gtk_box_pack_start (GTK_BOX (general_vbox),
			    importer_options,
			    FALSE,
			    FALSE,
			    0);
	/* move the options before the 'other' options */
	gtk_box_reorder_child (GTK_BOX (general_vbox),
			       importer_options,
			       _gtk_container_get_n_children (GTK_CONTAINER (general_vbox)) - 2);

	widget = _gtk_builder_get_widget (data->builder, "adjust_orientation_checkbutton");
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget), g_settings_get_boolean (data->settings, PREF_PHOTO_IMPORTER_ADJUST_ORIENTATION));
	g_signal_connect (widget,
			  "toggled",
			  G_CALLBACK (adjust_orientation_checkbutton_toggled_cb),
			  data);

	g_object_set_data_full (G_OBJECT (dialog), PREFERENCES_DATA_KEY, data, (GDestroyNotify) preferences_data_free);
}
