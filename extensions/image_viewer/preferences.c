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
} BrowserData;


static void
browser_data_free (BrowserData *data)
{
	g_object_unref (data->builder);
	g_object_unref (data->settings);
	g_free (data);
}


static void
zoom_quality_changed_cb (GtkComboBox *combo_box,
			 BrowserData *data)
{
	g_settings_set_enum (data->settings, PREF_IMAGE_VIEWER_ZOOM_QUALITY, gtk_combo_box_get_active (combo_box));
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
transp_type_changed_cb (GtkComboBox *combo_box,
			BrowserData *data)
{
	g_settings_set_enum (data->settings, PREF_IMAGE_VIEWER_TRANSP_TYPE, gtk_combo_box_get_active (combo_box));
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

	notebook = _gtk_builder_get_widget (dialog_builder, "notebook");

	page = _gtk_builder_get_widget (data->builder, "preferences_page");
	g_object_set_data (G_OBJECT (page), "extension-name", "image_viewer");
	gtk_widget_show (page);

	gtk_combo_box_set_active (GTK_COMBO_BOX (GET_WIDGET ("change_zoom_combobox")),
				  g_settings_get_enum (data->settings, PREF_IMAGE_VIEWER_ZOOM_CHANGE));
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("toggle_reset_scrollbars")),
				      g_settings_get_boolean (data->settings, PREF_IMAGE_VIEWER_RESET_SCROLLBARS));
	gtk_combo_box_set_active (GTK_COMBO_BOX (GET_WIDGET ("transp_type_combobox")),
				  g_settings_get_enum (data->settings, PREF_IMAGE_VIEWER_TRANSP_TYPE));
	gtk_combo_box_set_active (GTK_COMBO_BOX (GET_WIDGET ("zoom_quality_combobox")),
				  g_settings_get_enum (data->settings, PREF_IMAGE_VIEWER_ZOOM_QUALITY));

	g_signal_connect (GET_WIDGET ("zoom_quality_combobox"),
			  "changed",
			  G_CALLBACK (zoom_quality_changed_cb),
			  data);
	g_signal_connect (GET_WIDGET ("change_zoom_combobox"),
			  "changed",
			  G_CALLBACK (zoom_change_changed_cb),
			  data);
	g_signal_connect (GET_WIDGET ("transp_type_combobox"),
			  "changed",
			  G_CALLBACK (transp_type_changed_cb),
			  data);
	g_signal_connect (GET_WIDGET ("toggle_reset_scrollbars"),
			  "toggled",
			  G_CALLBACK (reset_scrollbars_toggled_cb),
			  data);

	label = gtk_label_new (_("Viewer"));
	gtk_widget_show (label);

	gtk_notebook_append_page (GTK_NOTEBOOK (notebook), page, label);

	g_object_set_data_full (G_OBJECT (dialog), BROWSER_DATA_KEY, data, (GDestroyNotify) browser_data_free);
}
