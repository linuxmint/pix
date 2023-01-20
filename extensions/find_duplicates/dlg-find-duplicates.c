/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2005-2009 Free Software Foundation, Inc.
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
#include "dlg-find-duplicates.h"
#include "gth-find-duplicates.h"


#define GET_WIDGET(name) _gtk_builder_get_widget (data->builder, (name))


typedef struct {
	GthBrowser *browser;
	GtkBuilder *builder;
	GtkWidget  *dialog;
	GtkWidget  *location_chooser;
	GList      *general_tests;
} DialogData;


static void
destroy_cb (GtkWidget  *widget,
	    DialogData *data)
{
	gth_browser_set_dialog (data->browser, "find_duplicates", NULL);

	_g_string_list_free (data->general_tests);
	g_object_unref (data->builder);
	g_free (data);
}


static void
ok_clicked_cb (GtkWidget  *widget,
	       DialogData *data)
{
	GFile *folder;

	folder = gth_location_chooser_get_current (GTH_LOCATION_CHOOSER (data->location_chooser));
	if (folder == NULL)
		return;

	gth_find_duplicates_exec (data->browser,
				  folder,
				  gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("include_subfolder_checkbutton"))),
				  g_list_nth_data (data->general_tests, gtk_combo_box_get_active (GTK_COMBO_BOX (GET_WIDGET ("file_type_combobox")))));

	gtk_widget_destroy (data->dialog);
}


void
dlg_find_duplicates (GthBrowser *browser)
{
	DialogData *data;
	GSettings  *settings;
	GList      *tests;
	char       *general_filter;
	int         active_filter;
	int         i;
	int         i_general;
	GList      *scan;

	if (gth_browser_get_dialog (browser, "find_duplicates") != NULL) {
		gtk_window_present (GTK_WINDOW (gth_browser_get_dialog (browser, "find_duplicates")));
		return;
	}

	data = g_new0 (DialogData, 1);
	data->browser = browser;
	data->builder = _gtk_builder_new_from_file ("find-duplicates.ui", "find_duplicates");

	settings = g_settings_new (GTHUMB_BROWSER_SCHEMA);

	/* Get the widgets. */

	data->dialog = g_object_new (GTK_TYPE_DIALOG,
				     "title", _("Find Duplicates"),
				     "use-header-bar", _gtk_settings_get_dialogs_use_header (),
				     "resizable", FALSE,
				     NULL);

	gtk_container_add (GTK_CONTAINER (gtk_dialog_get_content_area (GTK_DIALOG (data->dialog))),
	                   _gtk_builder_get_widget (data->builder, "dialog_content"));
	gtk_dialog_add_buttons (GTK_DIALOG (data->dialog),
				_("_Find"), GTK_RESPONSE_OK,
				_GTK_LABEL_CANCEL, GTK_RESPONSE_CANCEL,
				NULL);
	_gtk_dialog_add_class_to_response (GTK_DIALOG (data->dialog), GTK_RESPONSE_OK, GTK_STYLE_CLASS_SUGGESTED_ACTION);

	gth_browser_set_dialog (browser, "find_duplicates", data->dialog);
	g_object_set_data (G_OBJECT (data->dialog), "dialog_data", data);

	data->location_chooser = g_object_new (GTH_TYPE_LOCATION_CHOOSER,
					       "show-entry-points", TRUE,
					       "show-other", TRUE,
					       "relief", GTK_RELIEF_NORMAL,
					       NULL);
	gtk_widget_show (data->location_chooser);
	gtk_container_add (GTK_CONTAINER (GET_WIDGET ("location_chooser_container")), data->location_chooser);

	/* Set widgets data. */

	gth_location_chooser_set_current (GTH_LOCATION_CHOOSER (data->location_chooser), gth_browser_get_location (browser));

	tests = gth_main_get_registered_objects_id (GTH_TYPE_TEST);
	general_filter = g_settings_get_string (settings, PREF_BROWSER_GENERAL_FILTER);
	active_filter = 0;
	for (i = 0, i_general = -1, scan = tests; scan; scan = scan->next, i++) {
		const char  *registered_test_id = scan->data;
		GthTest     *test;
		GtkTreeIter  iter;

		if (strncmp (registered_test_id, "file::type::", 12) != 0)
			continue;

		i_general += 1;

		if (strcmp (registered_test_id, general_filter) == 0)
			active_filter = i_general;

		test = gth_main_get_registered_object (GTH_TYPE_TEST, registered_test_id);
		data->general_tests = g_list_prepend (data->general_tests, g_strdup (gth_test_get_id (test)));

		gtk_list_store_append (GTK_LIST_STORE (GET_WIDGET ("file_type_liststore")), &iter);
		gtk_list_store_set (GTK_LIST_STORE (GET_WIDGET ("file_type_liststore")), &iter,
				    0, gth_test_get_display_name (test),
				    -1);

		g_object_unref (test);
	}
	data->general_tests = g_list_reverse (data->general_tests);

	gtk_combo_box_set_active (GTK_COMBO_BOX (GET_WIDGET ("file_type_combobox")), active_filter);

	g_free (general_filter);
	_g_string_list_free (tests);
	g_object_unref (settings);

	/* Set the signals handlers. */

	g_signal_connect (G_OBJECT (data->dialog),
			  "destroy",
			  G_CALLBACK (destroy_cb),
			  data);
	g_signal_connect (gtk_dialog_get_widget_for_response (GTK_DIALOG (data->dialog), GTK_RESPONSE_OK),
			  "clicked",
			  G_CALLBACK (ok_clicked_cb),
			  data);
	g_signal_connect_swapped (gtk_dialog_get_widget_for_response (GTK_DIALOG (data->dialog), GTK_RESPONSE_CANCEL),
				  "clicked",
				  G_CALLBACK (gtk_widget_destroy),
				  G_OBJECT (data->dialog));

	/* Run dialog. */

	gtk_window_set_transient_for (GTK_WINDOW (data->dialog), GTK_WINDOW (browser));
	gtk_window_set_modal (GTK_WINDOW (data->dialog), FALSE);
	gtk_widget_show (data->dialog);
}
