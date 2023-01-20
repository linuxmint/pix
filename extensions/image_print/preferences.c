/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2011 Free Software Foundation, Inc.
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
#include <gthumb.h>
#include "preferences.h"


#define GET_WIDGET(name) _gtk_builder_get_widget (data->builder, (name))
#define BROWSER_DATA_KEY "print-options-preference-data"


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


void
ip__dlg_preferences_construct_cb (GtkWidget  *dialog,
				  GthBrowser *browser,
				  GtkBuilder *dialog_builder)
{
	BrowserData *data;
	GtkWidget   *notebook;
	GtkWidget   *page;
	GtkWidget   *label;
	char        *font_name;

	data = g_new0 (BrowserData, 1);
	data->builder = _gtk_builder_new_from_file ("print-preferences.ui", "image_print");
	data->settings = g_settings_new (GTHUMB_IMAGE_PRINT_SCHEMA);

	notebook = _gtk_builder_get_widget (dialog_builder, "notebook");

	page = _gtk_builder_get_widget (data->builder, "preferences_page");
	gtk_widget_show (page);

	font_name = g_settings_get_string (data->settings, PREF_IMAGE_PRINT_FONT_NAME);
	gtk_font_chooser_set_font (GTK_FONT_CHOOSER (GET_WIDGET ("caption_fontbutton")), font_name);
	g_free (font_name);

	font_name = g_settings_get_string (data->settings, PREF_IMAGE_PRINT_HEADER_FONT_NAME);
	gtk_font_chooser_set_font (GTK_FONT_CHOOSER (GET_WIDGET ("header_fontbutton")), font_name);
	g_free (font_name);

	font_name = g_settings_get_string (data->settings, PREF_IMAGE_PRINT_FOOTER_FONT_NAME);
	gtk_font_chooser_set_font (GTK_FONT_CHOOSER (GET_WIDGET ("footer_fontbutton")), font_name);
	g_free (font_name);

	label = gtk_label_new (_("Print"));
	gtk_widget_show (label);

	gtk_notebook_append_page (GTK_NOTEBOOK (notebook), page, label);
	g_object_set_data_full (G_OBJECT (dialog), BROWSER_DATA_KEY, data, (GDestroyNotify) browser_data_free);
}


void
ip__dlg_preferences_apply_cb (GtkWidget  *dialog,
			      GthBrowser *browser,
			      GtkBuilder *builder)
{
	BrowserData *data;

	data = g_object_get_data (G_OBJECT (dialog), BROWSER_DATA_KEY);
	g_return_if_fail (data != NULL);

	g_settings_set_string (data->settings, PREF_IMAGE_PRINT_FONT_NAME, gtk_font_chooser_get_font (GTK_FONT_CHOOSER (GET_WIDGET ("caption_fontbutton"))));
	g_settings_set_string (data->settings, PREF_IMAGE_PRINT_HEADER_FONT_NAME, gtk_font_chooser_get_font (GTK_FONT_CHOOSER (GET_WIDGET ("header_fontbutton"))));
	g_settings_set_string (data->settings, PREF_IMAGE_PRINT_FOOTER_FONT_NAME, gtk_font_chooser_get_font (GTK_FONT_CHOOSER (GET_WIDGET ("footer_fontbutton"))));
}
