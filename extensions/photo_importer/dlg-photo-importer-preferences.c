/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2015 The Free Software Foundation, Inc.
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
#include <gthumb.h>
#include "dlg-photo-importer-preferences.h"
#include "preferences.h"


typedef struct {
	GtkBuilder *builder;
	GSettings  *settings;
	GtkWidget  *dialog;
} DialogData;


static void
destroy_cb (GtkWidget  *widget,
	    DialogData *data)
{
	g_object_unref (data->builder);
	g_object_unref (data->settings);
	g_free (data);
}


static void
adjust_orientation_checkbutton_toggled_cb (GtkToggleButton *button,
					   DialogData      *data)
{
	g_settings_set_boolean (data->settings,
				PREF_PHOTO_IMPORTER_ADJUST_ORIENTATION,
				gtk_toggle_button_get_active (button));
}


static void
delete_files_checkbutton_toggled_cb (GtkToggleButton *button,
					   DialogData      *data)
{
	g_settings_set_boolean (data->settings,
				PREF_PHOTO_IMPORTER_DELETE_FROM_DEVICE,
				gtk_toggle_button_get_active (button));
}


void
dlg_photo_importer_preferences (GtkWindow *parent)
{
	DialogData *data;

	data = g_new0 (DialogData, 1);
	data->builder = gtk_builder_new_from_resource ("/org/gnome/gThumb/photo_importer/data/ui/photo-importer-options.ui");
	data->settings = g_settings_new (GTHUMB_PHOTO_IMPORTER_SCHEMA);

	/* Get the widgets. */

	data->dialog = g_object_new (GTK_TYPE_DIALOG,
				     "title", _("Preferences"),
				     "transient-for", GTK_WINDOW (parent),
				     "modal", TRUE,
				     "destroy-with-parent", FALSE,
				     "use-header-bar", _gtk_settings_get_dialogs_use_header (),
				     NULL);
	gtk_container_add (GTK_CONTAINER (gtk_dialog_get_content_area (GTK_DIALOG (data->dialog))),
			   _gtk_builder_get_widget (data->builder, "dialog_content"));
	gtk_dialog_add_buttons (GTK_DIALOG (data->dialog),
				_GTK_LABEL_OK, GTK_RESPONSE_CLOSE,
				NULL);

	/* Set widgets data. */

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (gtk_builder_get_object (data->builder, "adjust_orientation_checkbutton")),
				      g_settings_get_boolean (data->settings, PREF_PHOTO_IMPORTER_ADJUST_ORIENTATION));
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (gtk_builder_get_object (data->builder, "delete_files_checkbutton")),
				      g_settings_get_boolean (data->settings, PREF_PHOTO_IMPORTER_DELETE_FROM_DEVICE));

	/* Set the signals handlers. */

	g_signal_connect (G_OBJECT (data->dialog),
			  "destroy",
			  G_CALLBACK (destroy_cb),
			  data);
	g_signal_connect_swapped (gtk_dialog_get_widget_for_response (GTK_DIALOG (data->dialog), GTK_RESPONSE_CLOSE),
				  "clicked",
				  G_CALLBACK (gtk_widget_destroy),
				  G_OBJECT (data->dialog));
	g_signal_connect (gtk_builder_get_object (data->builder, "adjust_orientation_checkbutton"),
			  "clicked",
			  G_CALLBACK (adjust_orientation_checkbutton_toggled_cb),
			  data);
	g_signal_connect (gtk_builder_get_object (data->builder, "delete_files_checkbutton"),
			  "clicked",
			  G_CALLBACK (delete_files_checkbutton_toggled_cb),
			  data);

	/* run dialog. */

	gtk_widget_show (data->dialog);
}
