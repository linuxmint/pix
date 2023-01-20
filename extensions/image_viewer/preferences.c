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
#include "preferences.h"


#define BROWSER_DATA_KEY "image-viewer-preference-data"
#define GET_WIDGET(x) (_gtk_builder_get_widget (data->builder, (x)))


typedef struct {
	GtkBuilder *builder;
	GSettings  *settings;
	GSettings  *browser_settings;
} BrowserData;


static void
browser_data_free (BrowserData *data)
{
	g_object_unref (data->builder);
	g_object_unref (data->settings);
	g_object_unref (data->browser_settings);
	g_free (data);
}


static void
zoom_change_changed_cb (GtkComboBox *combo_box,
			BrowserData *data)
{
	g_settings_set_enum (data->settings, PREF_IMAGE_VIEWER_ZOOM_CHANGE, gtk_combo_box_get_active (combo_box));
}


static void
reset_scrollbars_toggled_cb (GtkToggleButton *button,
			     BrowserData     *data)
{
	g_settings_set_boolean (data->settings, PREF_IMAGE_VIEWER_RESET_SCROLLBARS, gtk_toggle_button_get_active (button));
}


static void
scroll_event_toggled_cb (GtkToggleButton *button,
			 BrowserData     *data)
{
	g_settings_set_enum (data->browser_settings,
			     PREF_VIEWER_SCROLL_ACTION,
			     (GTK_WIDGET (button) == GET_WIDGET ("scroll_event_change_image_radiobutton")) ? GTH_SCROLL_ACTION_CHANGE_FILE : GTH_SCROLL_ACTION_ZOOM);
}

static void
zoom_quality_radiobutton_toggled_cb (GtkToggleButton *button,
				     BrowserData     *data)
{
	g_settings_set_enum (data->settings, PREF_IMAGE_VIEWER_ZOOM_QUALITY, (GTK_WIDGET (button) == GET_WIDGET ("zoom_quality_high_radiobutton")) ? GTH_ZOOM_QUALITY_HIGH : GTH_ZOOM_QUALITY_LOW);
}


static void
transparency_style_changed_cb (GtkComboBox *combo_box,
			       BrowserData *data)
{
	g_settings_set_enum (data->settings,
			     PREF_IMAGE_VIEWER_TRANSPARENCY_STYLE,
			     gtk_combo_box_get_active (combo_box));
}


void
image_viewer__dlg_preferences_construct_cb (GtkWidget  *dialog,
					    GthBrowser *browser,
					    GtkBuilder *dialog_builder)
{
	BrowserData *data;
	GtkWidget   *notebook;
	GtkWidget   *page;
	GtkWidget   *label;

	data = g_new0 (BrowserData, 1);
	data->builder = _gtk_builder_new_from_file ("image-viewer-preferences.ui", "image_viewer");
	data->settings = g_settings_new (GTHUMB_IMAGE_VIEWER_SCHEMA);
	data->browser_settings = g_settings_new (GTHUMB_BROWSER_SCHEMA);

	notebook = _gtk_builder_get_widget (dialog_builder, "notebook");

	page = _gtk_builder_get_widget (data->builder, "preferences_page");
	gtk_widget_show (page);

	gtk_combo_box_set_active (GTK_COMBO_BOX (GET_WIDGET ("change_zoom_combobox")),
				  g_settings_get_enum (data->settings, PREF_IMAGE_VIEWER_ZOOM_CHANGE));
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("toggle_reset_scrollbars")),
				      g_settings_get_boolean (data->settings, PREF_IMAGE_VIEWER_RESET_SCROLLBARS));

	if (g_settings_get_enum (data->settings, PREF_IMAGE_VIEWER_ZOOM_QUALITY) == GTH_ZOOM_QUALITY_LOW)
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("zoom_quality_low_radiobutton")), TRUE);
	else
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("zoom_quality_high_radiobutton")), TRUE);

	if (g_settings_get_enum (data->browser_settings, PREF_VIEWER_SCROLL_ACTION) == GTH_SCROLL_ACTION_CHANGE_FILE)
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("scroll_event_change_image_radiobutton")), TRUE);
	else
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("scroll_event_zoom_radiobutton")), TRUE);

	gtk_combo_box_set_active (GTK_COMBO_BOX (GET_WIDGET ("transparency_style_combobox")),
				  g_settings_get_enum (data->settings, PREF_IMAGE_VIEWER_TRANSPARENCY_STYLE));

	g_signal_connect (GET_WIDGET ("change_zoom_combobox"),
			  "changed",
			  G_CALLBACK (zoom_change_changed_cb),
			  data);
	g_signal_connect (GET_WIDGET ("scroll_event_change_image_radiobutton"),
			  "toggled",
			  G_CALLBACK (scroll_event_toggled_cb),
			  data);
	g_signal_connect (GET_WIDGET ("scroll_event_zoom_radiobutton"),
			  "toggled",
			  G_CALLBACK (scroll_event_toggled_cb),
			  data);
	g_signal_connect (GET_WIDGET ("zoom_quality_low_radiobutton"),
			  "toggled",
			  G_CALLBACK (zoom_quality_radiobutton_toggled_cb),
			  data);
	g_signal_connect (GET_WIDGET ("zoom_quality_high_radiobutton"),
			  "toggled",
			  G_CALLBACK (zoom_quality_radiobutton_toggled_cb),
			  data);
	g_signal_connect (GET_WIDGET ("toggle_reset_scrollbars"),
			  "toggled",
			  G_CALLBACK (reset_scrollbars_toggled_cb),
			  data);
	g_signal_connect (GET_WIDGET ("transparency_style_combobox"),
			  "changed",
			  G_CALLBACK (transparency_style_changed_cb),
			  data);

	label = gtk_label_new (_("Viewer"));
	gtk_widget_show (label);

	gtk_notebook_append_page (GTK_NOTEBOOK (notebook), page, label);

	g_object_set_data_full (G_OBJECT (dialog), BROWSER_DATA_KEY, data, (GDestroyNotify) browser_data_free);
}
