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
#include "dlg-preferences-browser.h"
#include "glib-utils.h"
#include "gth-browser.h"
#include "gth-main.h"
#include "gth-metadata-chooser.h"
#include "gth-preferences.h"
#include "gtk-utils.h"
#include "main.h"


#define BROWSER_DATA_KEY "browser-preference-data"
#define GET_WIDGET(name) _gtk_builder_get_widget (data->builder, (name))


typedef struct {
	GthBrowser   *browser;
	GtkBuilder   *builder;
	GSettings    *browser_settings;
	GtkWidget    *dialog;
	GtkWidget    *thumbnail_caption_chooser;
} BrowserData;


static int thumb_size[] = { 48, 64, 85, 95, 112, 128, 164, 200, 256 };
static int thumb_sizes = sizeof (thumb_size) / sizeof (int);


static void
browser_data_free (BrowserData *data)
{
	g_object_unref (data->browser_settings);
	g_object_unref (data->builder);
	g_free (data);
}


static int
get_idx_from_size (int size)
{
	int i;

	for (i = 0; i < thumb_sizes; i++)
		if (size == thumb_size[i])
			return i;
	return -1;
}


static void
thumbnail_size_changed_cb (GtkWidget   *widget,
			   BrowserData *data)
{
	g_settings_set_int (data->browser_settings, PREF_BROWSER_THUMBNAIL_SIZE, thumb_size[gtk_combo_box_get_active (GTK_COMBO_BOX (GET_WIDGET ("thumbnail_size_combobox")))]);
}


static void
fast_file_type_toggled_cb (GtkToggleButton *button,
			   BrowserData     *data)
{
	g_settings_set_boolean (data->browser_settings, PREF_BROWSER_FAST_FILE_TYPE, ! gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("slow_mime_type_checkbutton"))));
}


static void
thumbnail_caption_chooser_changed_cb (GthMetadataChooser *chooser,
				      BrowserData        *data)
{
	char *attributes;

	attributes = gth_metadata_chooser_get_selection (chooser);
	g_settings_set_string (data->browser_settings, PREF_BROWSER_THUMBNAIL_CAPTION, attributes);

	g_free (attributes);
}


static void
click_behavior_changed_cb (GtkToggleButton *button,
			   BrowserData     *data)
{
	g_settings_set_boolean (data->browser_settings, PREF_BROWSER_SINGLE_CLICK_ACTIVATION, gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("single_click_radiobutton"))));
}


static void
open_in_fullscreen_toggled_cb (GtkToggleButton *button,
			       BrowserData     *data)
{
	g_settings_set_boolean (data->browser_settings, PREF_BROWSER_OPEN_FILES_IN_FULLSCREEN, gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("open_in_fullscreen_checkbutton"))));
}


void
browser__dlg_preferences_construct_cb (GtkWidget  *dialog,
				       GthBrowser *browser,
				       GtkBuilder *dialog_builder)
{
	BrowserData   *data;
	char          *current_caption;
	GtkWidget     *label;
	GtkWidget     *page;

	data = g_new0 (BrowserData, 1);
	data->builder = _gtk_builder_new_from_file ("browser-preferences.ui", NULL);
	data->browser_settings = g_settings_new (GTHUMB_BROWSER_SCHEMA);
	data->dialog = dialog;

	/* caption list */

	data->thumbnail_caption_chooser = gth_metadata_chooser_new (GTH_METADATA_ALLOW_IN_FILE_LIST, TRUE);
	gtk_widget_show (data->thumbnail_caption_chooser);
	gtk_container_add (GTK_CONTAINER (GET_WIDGET ("caption_scrolledwindow")), data->thumbnail_caption_chooser);

	current_caption = g_settings_get_string (data->browser_settings, PREF_BROWSER_THUMBNAIL_CAPTION);
	gth_metadata_chooser_set_selection (GTH_METADATA_CHOOSER (data->thumbnail_caption_chooser), current_caption);
	g_free (current_caption);

	/* behavior */

	if (g_settings_get_boolean (data->browser_settings, PREF_BROWSER_SINGLE_CLICK_ACTIVATION))
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("single_click_radiobutton")), TRUE);
	else
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("double_click_radiobutton")), TRUE);

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("open_in_fullscreen_checkbutton")),
				      g_settings_get_boolean (data->browser_settings, PREF_BROWSER_OPEN_FILES_IN_FULLSCREEN));

	/* other */

	gtk_combo_box_set_active (GTK_COMBO_BOX (GET_WIDGET ("thumbnail_size_combobox")),
				  get_idx_from_size (g_settings_get_int (data->browser_settings, PREF_BROWSER_THUMBNAIL_SIZE)));
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("slow_mime_type_checkbutton")),
				      ! g_settings_get_boolean (data->browser_settings, PREF_BROWSER_FAST_FILE_TYPE));

	/* signal handlers */

	g_signal_connect (G_OBJECT (GET_WIDGET ("thumbnail_size_combobox")),
			  "changed",
			  G_CALLBACK (thumbnail_size_changed_cb),
			  data);
	g_signal_connect (G_OBJECT (GET_WIDGET ("slow_mime_type_checkbutton")),
			  "toggled",
			  G_CALLBACK (fast_file_type_toggled_cb),
			  data);
	g_signal_connect (G_OBJECT (GET_WIDGET ("single_click_radiobutton")),
			  "toggled",
			  G_CALLBACK (click_behavior_changed_cb),
			  data);
	g_signal_connect (G_OBJECT (GET_WIDGET ("double_click_radiobutton")),
			  "toggled",
			  G_CALLBACK (click_behavior_changed_cb),
			  data);
	g_signal_connect (G_OBJECT (GET_WIDGET ("open_in_fullscreen_checkbutton")),
			  "toggled",
			  G_CALLBACK (open_in_fullscreen_toggled_cb),
			  data);
	g_signal_connect (G_OBJECT (data->thumbnail_caption_chooser),
			  "changed",
			  G_CALLBACK (thumbnail_caption_chooser_changed_cb),
			  data);

	/* add the page to the preferences dialog */

	label = gtk_label_new (_("Browser"));
	gtk_widget_show (label);

	page = _gtk_builder_get_widget (data->builder, "browser_page");
	gtk_widget_show (page);
	gtk_notebook_append_page (GTK_NOTEBOOK (_gtk_builder_get_widget (dialog_builder, "notebook")), page, label);

	g_object_set_data_full (G_OBJECT (dialog), BROWSER_DATA_KEY, data, (GDestroyNotify) browser_data_free);
}


void
browser__dlg_preferences_apply (GtkWidget  *dialog,
		  	  	GthBrowser *browser,
		  	  	GtkBuilder *dialog_builder)
{
	/* void */
}
