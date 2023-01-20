/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2020 The Free Software Foundation, Inc.
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
#include "dlg-terminal-preferences.h"
#include "preferences.h"


typedef struct {
	GtkBuilder *builder;
	GSettings  *settings;
	GtkWidget  *dialog;
} DialogData;


static void
update_settings (DialogData *data)
{
	const char *command;

	command = gtk_entry_get_text (GTK_ENTRY (gtk_builder_get_object (data->builder, "command_entry")));
	if (command != NULL)
		g_settings_set_string (data->settings, PREF_TERMINAL_COMMAND, command);
}


static void
destroy_cb (GtkWidget  *widget,
	    DialogData *data)
{
	g_object_unref (data->builder);
	g_object_unref (data->settings);
	g_free (data);
}


static void
dialog_response_cb (GtkDialog *dialog,
		    int        response_id,
		    gpointer   user_data)
{
	DialogData *data = user_data;

	if (response_id == GTK_RESPONSE_ACCEPT)
		update_settings (data);
	gtk_widget_destroy (data->dialog);
}


void
dlg_terminal_preferences (GtkWindow *parent)
{
	DialogData *data;
	char       *command;

	data = g_new0 (DialogData, 1);
	data->builder = _gtk_builder_new_from_file ("terminal-preferences.ui", "terminal");
	data->settings = g_settings_new (GTHUMB_TERMINAL_SCHEMA);

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
				_GTK_LABEL_CANCEL, GTK_RESPONSE_CANCEL,
				_GTK_LABEL_SAVE, GTK_RESPONSE_ACCEPT,
				NULL);
	_gtk_dialog_add_class_to_response (GTK_DIALOG (data->dialog), GTK_RESPONSE_ACCEPT, GTK_STYLE_CLASS_SUGGESTED_ACTION);

	/* Set widgets data. */

	command = g_settings_get_string (data->settings, PREF_TERMINAL_COMMAND);
	gtk_entry_set_text (GTK_ENTRY (gtk_builder_get_object (data->builder, "command_entry")), command);
	g_free (command);

	/* Set the signals handlers. */

	g_signal_connect (G_OBJECT (data->dialog),
			  "destroy",
			  G_CALLBACK (destroy_cb),
			  data);
	g_signal_connect (data->dialog,
			  "response",
			  G_CALLBACK (dialog_response_cb),
			  data);

	/* run dialog. */

	gtk_widget_show (data->dialog);
}
