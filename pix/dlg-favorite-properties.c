/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2001-2020 The Free Software Foundation, Inc.
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
#include "dlg-favorite-properties.h"
#include "gth-main.h"
#include "gth-metadata-chooser.h"
#include "gth-preferences.h"
#include "str-utils.h"


#define GET_WIDGET(name) _gtk_builder_get_widget (data->builder, (name))


typedef struct {
	GthBrowser *browser;
	GtkBuilder *builder;
	GSettings  *settings;
	GtkWidget  *dialog;
	GtkWidget  *metadata_chooser;
} DialogData;


static void
dialog_data_free (DialogData *data)
{
	g_object_unref (data->builder);
	g_object_unref (data->settings);
	g_free (data);
}


static void
destroy_cb (GtkWidget  *widget,
	    DialogData *data)
{
	gth_browser_set_dialog (data->browser, "favorite_properties", NULL);
	dialog_data_free (data);
}


static void
cancel_button_clicked_cb (GtkWidget  *widget,
			  DialogData *data)
{
	gtk_widget_destroy (data->dialog);
}


static void
ok_button_clicked_cb (GtkWidget  *widget,
		      DialogData *data)
{
	char *favourite_properties;

	favourite_properties = gth_metadata_chooser_get_selection (GTH_METADATA_CHOOSER (data->metadata_chooser));
	g_settings_set_string (data->settings, PREF_BROWSER_FAVORITE_PROPERTIES, favourite_properties);
	g_free (favourite_properties);

	gtk_widget_destroy (data->dialog);
}


void
dlg_favorite_properties (GthBrowser *browser)
{
	DialogData *data;
	char       *favourite_properties;

	if (gth_browser_get_dialog (browser, "favorite_properties") != NULL) {
		gtk_window_present (GTK_WINDOW (gth_browser_get_dialog (browser, "favorite_properties")));
		return;
	}

	data = g_new0 (DialogData, 1);
	data->browser = browser;
	data->builder = _gtk_builder_new_from_file ("favorite-properties.ui", NULL);
	data->settings = g_settings_new (GTHUMB_BROWSER_SCHEMA);
	data->dialog = g_object_new (GTK_TYPE_DIALOG,
				     "title", _("Preferences"),
				     "transient-for", GTK_WINDOW (browser),
				     "modal", FALSE,
				     "destroy-with-parent", FALSE,
				     "use-header-bar", _gtk_settings_get_dialogs_use_header (),
				     NULL);
	gtk_container_add (GTK_CONTAINER (gtk_dialog_get_content_area (GTK_DIALOG (data->dialog))),
			   _gtk_builder_get_widget (data->builder, "dialog_content"));
	gtk_dialog_add_buttons (GTK_DIALOG (data->dialog),
				_GTK_LABEL_CANCEL, GTK_RESPONSE_CANCEL,
				_GTK_LABEL_OK, GTK_RESPONSE_OK,
				NULL);
	_gtk_dialog_add_class_to_response (GTK_DIALOG (data->dialog), GTK_RESPONSE_OK, GTK_STYLE_CLASS_SUGGESTED_ACTION);
	gtk_widget_grab_default (gtk_dialog_get_widget_for_response (GTK_DIALOG (data->dialog), GTK_RESPONSE_OK));
	gth_browser_set_dialog (browser, "favorite_properties", data->dialog);

	/* TODO: set the widget data */

	data->metadata_chooser = gth_metadata_chooser_new (GTH_METADATA_ALLOW_IN_PROPERTIES_VIEW, FALSE);
	gtk_widget_show (data->metadata_chooser);
	gtk_container_add (GTK_CONTAINER (_gtk_builder_get_widget (data->builder, "chooser_scrolled_window")), data->metadata_chooser);

	favourite_properties = g_settings_get_string (data->settings, PREF_BROWSER_FAVORITE_PROPERTIES);
	if (_g_str_equal (favourite_properties, "default")) {
		GString *attributes;
		GList   *metadata_info;
		GList   *scan;

		attributes = g_string_new ("");
		metadata_info = gth_main_get_all_metadata_info ();
		for (scan = metadata_info; scan; scan = scan->next) {
			GthMetadataInfo *info = scan->data;

			if ((info->flags & GTH_METADATA_ALLOW_IN_PROPERTIES_VIEW) != GTH_METADATA_ALLOW_IN_PROPERTIES_VIEW)
				continue;

			if (info->id == NULL)
				continue;

			if (g_str_has_prefix (info->id, "Exif"))
				continue;
			else if (g_str_has_prefix (info->id, "Iptc"))
				continue;
			else if (g_str_has_prefix (info->id, "Xmp"))
				continue;

			g_string_append (attributes, info->id);
			g_string_append_c (attributes, ',');
		}

		gth_metadata_chooser_set_selection (GTH_METADATA_CHOOSER (data->metadata_chooser), attributes->str);

		g_string_free (attributes, TRUE);
	}
	else
		gth_metadata_chooser_set_selection (GTH_METADATA_CHOOSER (data->metadata_chooser), favourite_properties);

	g_free (favourite_properties);

	/* set the signals handlers. */

	g_signal_connect (G_OBJECT (data->dialog),
			  "destroy",
			  G_CALLBACK (destroy_cb),
			  data);
	g_signal_connect (gtk_dialog_get_widget_for_response (GTK_DIALOG (data->dialog), GTK_RESPONSE_CANCEL),
			  "clicked",
			  G_CALLBACK (cancel_button_clicked_cb),
			  data);
	g_signal_connect (gtk_dialog_get_widget_for_response (GTK_DIALOG (data->dialog), GTK_RESPONSE_OK),
			  "clicked",
			  G_CALLBACK (ok_button_clicked_cb),
			  data);

	/* run dialog. */

	gtk_widget_show (data->dialog);
}
