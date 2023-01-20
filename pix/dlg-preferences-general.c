/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2014 The Free Software Foundation, Inc.
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
#include <gtk/gtk.h>
#include "dlg-preferences-general.h"
#include "glib-utils.h"
#include "gth-browser.h"
#include "gth-file-source-vfs.h"
#include "gth-location-chooser.h"
#include "gth-main.h"
#include "gth-preferences.h"
#include "gtk-utils.h"
#include "main.h"


#define BROWSER_DATA_KEY "general-preference-data"
#define GET_WIDGET(name) _gtk_builder_get_widget (data->builder, (name))


typedef struct {
	GthBrowser *browser;
	GtkBuilder *builder;
	GSettings  *general_settings;
	GSettings  *browser_settings;
	GSettings  *messages_settings;
	GtkWidget  *dialog;
	GtkWidget  *startup_location_chooser;
} BrowserData;


static void
browser_data_free (BrowserData *data)
{
	g_object_unref (data->general_settings);
	g_object_unref (data->browser_settings);
	g_object_unref (data->messages_settings);
	g_object_unref (data->builder);
	g_object_unref (data->browser);
	g_free (data);
}


static void
statusbar_toggled_cb (GtkWidget   *widget,
		      BrowserData *data)
{
	g_settings_set_boolean (data->browser_settings, PREF_BROWSER_STATUSBAR_VISIBLE, gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("statusbar_checkbutton"))));
}


static void
use_startup_toggled_cb (GtkWidget   *widget,
			BrowserData *data)
{
	gtk_widget_set_sensitive (data->startup_location_chooser, gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget)));
	gtk_widget_set_sensitive (GET_WIDGET ("set_to_current_button"), gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget)));
}


static void
set_to_current_cb (GtkWidget   *widget,
		   BrowserData *data)
{
	gth_location_chooser_set_current (GTH_LOCATION_CHOOSER (data->startup_location_chooser),
			gth_browser_get_location (data->browser));
}


static void
thumbnails_pane_orientation_changed_cb (GtkWidget   *widget,
					BrowserData *data)
{
	g_settings_set_enum (data->browser_settings, PREF_BROWSER_VIEWER_THUMBNAILS_ORIENT, gtk_combo_box_get_active (GTK_COMBO_BOX (GET_WIDGET ("thumbnails_pane_orient_combobox"))));
}


static void
file_properties_position_combobox_changed_cb (GtkWidget   *widget,
					      BrowserData *data)
{
	g_settings_set_boolean (data->browser_settings, PREF_BROWSER_PROPERTIES_ON_THE_RIGHT, gtk_combo_box_get_active (GTK_COMBO_BOX (GET_WIDGET ("file_properties_position_combobox"))) == 1);
}


static void
reuse_active_window_checkbutton_toggled_cb (GtkToggleButton *button,
					    BrowserData     *data)
{
	g_settings_set_boolean (data->browser_settings, PREF_BROWSER_REUSE_ACTIVE_WINDOW, gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("reuse_active_window_checkbutton"))));
}


static void
confirm_deletion_toggled_cb (GtkToggleButton *button,
			     BrowserData     *data)
{
	g_settings_set_boolean (data->messages_settings, PREF_MSG_CONFIRM_DELETION, gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("confirm_deletion_checkbutton"))));
}


static void
ask_to_save_toggled_cb (GtkToggleButton *button,
			BrowserData     *data)
{
	g_settings_set_boolean (data->messages_settings, PREF_MSG_SAVE_MODIFIED_IMAGE, gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("ask_to_save_checkbutton"))));
}


void
general__dlg_preferences_construct_cb (GtkWidget  *dialog,
				       GthBrowser *browser,
				       GtkBuilder *dialog_builder)
{
	BrowserData *data;

	data = g_new0 (BrowserData, 1);
	data->browser = g_object_ref (browser);
	data->builder = g_object_ref (dialog_builder);
	data->general_settings = g_settings_new (GTHUMB_GENERAL_SCHEMA);
	data->browser_settings = g_settings_new (GTHUMB_BROWSER_SCHEMA);
	data->messages_settings = g_settings_new (GTHUMB_MESSAGES_SCHEMA);
	data->dialog = dialog;

	/* widgets */

	gtk_combo_box_set_active (GTK_COMBO_BOX (GET_WIDGET ("file_properties_position_combobox")),
				  g_settings_get_boolean (data->browser_settings, PREF_BROWSER_PROPERTIES_ON_THE_RIGHT) ? 1 : 0);

	if (g_settings_get_boolean (data->browser_settings, PREF_BROWSER_USE_STARTUP_LOCATION))
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("use_startup_location_radiobutton")), TRUE);
	else
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("go_to_last_location_radiobutton")), TRUE);

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("statusbar_checkbutton")),
				      g_settings_get_boolean (data->browser_settings, PREF_BROWSER_STATUSBAR_VISIBLE));

	/* starup location */
	{
		char  *uri;
		GFile *location;

		data->startup_location_chooser = g_object_new (
				GTH_TYPE_LOCATION_CHOOSER,
				"show-entry-points", FALSE,
				"show-other", TRUE,
				"relief", GTK_RELIEF_NORMAL,
				NULL);
		gtk_widget_show (data->startup_location_chooser);
		gtk_box_pack_start (GTK_BOX (GET_WIDGET ("startup_location_chooser_box")),
				data->startup_location_chooser,
				TRUE,
				TRUE,
				0);

		uri = _g_settings_get_uri (data->browser_settings, PREF_BROWSER_STARTUP_LOCATION);
		if (uri == NULL)
			uri = g_strdup (_g_uri_get_home ());
		location = g_file_new_for_uri (uri);
		gth_location_chooser_set_current (GTH_LOCATION_CHOOSER (data->startup_location_chooser), location);

		g_object_unref (location);
		g_free (uri);
	}

	if (! gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("use_startup_location_radiobutton")))) {
		gtk_widget_set_sensitive (data->startup_location_chooser, FALSE);
		gtk_widget_set_sensitive (GET_WIDGET ("set_to_current_button"), FALSE);
	}

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("reuse_active_window_checkbutton")),
				      g_settings_get_boolean (data->browser_settings, PREF_BROWSER_REUSE_ACTIVE_WINDOW));
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("confirm_deletion_checkbutton")),
				      g_settings_get_boolean (data->messages_settings, PREF_MSG_CONFIRM_DELETION));
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("ask_to_save_checkbutton")),
				      g_settings_get_boolean (data->messages_settings, PREF_MSG_SAVE_MODIFIED_IMAGE));
	gtk_combo_box_set_active (GTK_COMBO_BOX (GET_WIDGET ("thumbnails_pane_orient_combobox")),
				  g_settings_get_enum (data->browser_settings, PREF_BROWSER_VIEWER_THUMBNAILS_ORIENT));

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("embed_metadata_checkbutton")),
				      g_settings_get_boolean (data->general_settings, PREF_GENERAL_STORE_METADATA_IN_FILES));

	/* signal handlers */

	g_signal_connect (GET_WIDGET ("thumbnails_pane_orient_combobox"),
			  "changed",
			  G_CALLBACK (thumbnails_pane_orientation_changed_cb),
			  data);
	g_signal_connect (GET_WIDGET ("file_properties_position_combobox"),
			  "changed",
			  G_CALLBACK (file_properties_position_combobox_changed_cb),
			  data);
	g_signal_connect (G_OBJECT (GET_WIDGET ("statusbar_checkbutton")),
			  "toggled",
			  G_CALLBACK (statusbar_toggled_cb),
			  data);
	g_signal_connect (G_OBJECT (GET_WIDGET ("use_startup_location_radiobutton")),
			  "toggled",
			  G_CALLBACK (use_startup_toggled_cb),
			  data);
	g_signal_connect (G_OBJECT (GET_WIDGET ("set_to_current_button")),
			  "clicked",
			  G_CALLBACK (set_to_current_cb),
			  data);
	g_signal_connect (G_OBJECT (GET_WIDGET ("reuse_active_window_checkbutton")),
			  "toggled",
			  G_CALLBACK (reuse_active_window_checkbutton_toggled_cb),
			  data);
	g_signal_connect (G_OBJECT (GET_WIDGET ("confirm_deletion_checkbutton")),
			  "toggled",
			  G_CALLBACK (confirm_deletion_toggled_cb),
			  data);
	g_signal_connect (G_OBJECT (GET_WIDGET ("ask_to_save_checkbutton")),
			  "toggled",
			  G_CALLBACK (ask_to_save_toggled_cb),
			  data);

	g_object_set_data_full (G_OBJECT (dialog), BROWSER_DATA_KEY, data, (GDestroyNotify) browser_data_free);
}


void
general__dlg_preferences_apply (GtkWidget  *dialog,
		  	  	GthBrowser *browser,
		  	  	GtkBuilder *dialog_builder)
{
	BrowserData *data;

	data = g_object_get_data (G_OBJECT (dialog), BROWSER_DATA_KEY);
	g_return_if_fail (data != NULL);

	/* Startup dir. */

	g_settings_set_boolean (data->browser_settings, PREF_BROWSER_GO_TO_LAST_LOCATION, gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("go_to_last_location_radiobutton"))));
	g_settings_set_boolean (data->browser_settings, PREF_BROWSER_USE_STARTUP_LOCATION, gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("use_startup_location_radiobutton"))));
	g_settings_set_boolean (data->general_settings, PREF_GENERAL_STORE_METADATA_IN_FILES, gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("embed_metadata_checkbutton"))));

	if (g_settings_get_boolean (data->browser_settings, PREF_BROWSER_USE_STARTUP_LOCATION)) {
		GFile *location;

		location = gth_location_chooser_get_current (GTH_LOCATION_CHOOSER (data->startup_location_chooser));
		if (location != NULL) {
			char  *uri;

			uri = g_file_get_uri (location);
			_g_settings_set_uri (data->browser_settings, PREF_BROWSER_STARTUP_LOCATION, uri);
			gth_pref_set_startup_location (uri);
			g_free (uri);
		}
	}

}
