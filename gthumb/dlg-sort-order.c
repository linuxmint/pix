/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2009 The Free Software Foundation, Inc.
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
#include "dlg-sort-order.h"
#include "gth-browser.h"
#include "gth-main.h"
#include "gtk-utils.h"


#define GET_WIDGET(name) _gtk_builder_get_widget (data->builder, (name))


typedef struct {
	GthBrowser	*browser;
	GtkBuilder	*builder;
	GtkWidget	*dialog;
	GthFileDataSort	*current_sort_type;
	GList		*sort_types;
} DialogData;


static void
destroy_cb (GtkWidget  *widget,
	    DialogData *data)
{
	gth_browser_set_dialog (data->browser, "sort-order", NULL);
	g_list_free (data->sort_types);
	g_object_unref (data->builder);
	g_free (data);
}


static void
apply_sort_order (GtkWidget  *widget,
		  DialogData *data)
{
	gth_browser_set_sort_order (data->browser,
				    data->current_sort_type,
				    gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("inverse_checkbutton"))));
}


typedef struct {
	DialogData      *data;
	GthFileDataSort *sort_type;
} ButtonData;


static void
button_data_free (ButtonData *button_data)
{
	g_return_if_fail (button_data != NULL);
	g_free (button_data);
}


static void
order_button_toggled_cb (GtkToggleButton *button,
			 gpointer         user_data)
{
	ButtonData *button_data = user_data;
	DialogData *data = button_data->data;

	if (! gtk_toggle_button_get_active (button))
		return;

	data->current_sort_type = button_data->sort_type;
	apply_sort_order (NULL, data);
}


void
dlg_sort_order (GthBrowser *browser)
{
	DialogData      *data;
	GtkWidget       *first_button;
	GthFileData     *file_data;
	GList           *scan;
	gboolean         sort_inverse;

	if (gth_browser_get_dialog (browser, "sort-order") != NULL) {
		gtk_window_present (GTK_WINDOW (gth_browser_get_dialog (browser, "sort-order")));
		return;
	}

	data = g_new0 (DialogData, 1);
	data->browser = browser;
	data->builder = _gtk_builder_new_from_file ("sort-order.ui", NULL);

	/* Get the widgets. */

	data->dialog = g_object_new (GTK_TYPE_DIALOG,
				     "title", _("Sort By"),
				     "transient-for", GTK_WINDOW (browser),
				     "modal", FALSE,
				     "destroy-with-parent", FALSE,
				     "use-header-bar", _gtk_settings_get_dialogs_use_header (),
				     NULL);
	gtk_container_add (GTK_CONTAINER (gtk_dialog_get_content_area (GTK_DIALOG (data->dialog))),
			   _gtk_builder_get_widget (data->builder, "dialog_content"));
	gtk_dialog_add_buttons (GTK_DIALOG (data->dialog),
				_GTK_LABEL_OK, GTK_RESPONSE_CLOSE,
				NULL);

	gth_browser_set_dialog (browser, "sort-order", data->dialog);
	g_object_set_data (G_OBJECT (data->dialog), "dialog_data", data);

	/* Set widgets data. */

	file_data = gth_browser_get_location_data (data->browser);
	if (file_data != NULL) {
		data->current_sort_type = gth_main_get_sort_type (g_file_info_get_attribute_string (file_data->info, "sort::type"));
		sort_inverse = g_file_info_get_attribute_boolean (file_data->info, "sort::inverse");
	}
	else
		gth_browser_get_sort_order (data->browser, &data->current_sort_type, &sort_inverse);

	first_button = NULL;
	data->sort_types = gth_main_get_all_sort_types ();
	for (scan = data->sort_types; scan; scan = scan->next) {
		GthFileDataSort *sort_type = scan->data;
		GtkWidget       *button;
		ButtonData      *button_data;

		button = gtk_radio_button_new_with_label_from_widget (GTK_RADIO_BUTTON (first_button), _(sort_type->display_name));
		if (scan == data->sort_types)
			first_button = button;
		gtk_widget_show (button);
		gtk_box_pack_start (GTK_BOX (GET_WIDGET ("sort_order_box")), button, FALSE, FALSE, 0);
		if (strcmp (sort_type->name, data->current_sort_type->name) == 0)
			gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), TRUE);

		button_data = g_new0 (ButtonData, 1);
		button_data->data = data;
		button_data->sort_type = sort_type;

		g_signal_connect_data (button,
				       "toggled",
				       G_CALLBACK (order_button_toggled_cb),
				       button_data,
				       (GClosureNotify) button_data_free,
				       0);
	}

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("inverse_checkbutton")), sort_inverse);

	/* Set the signals handlers. */

	g_signal_connect (G_OBJECT (data->dialog),
			  "destroy",
			  G_CALLBACK (destroy_cb),
			  data);
	g_signal_connect_swapped (gtk_dialog_get_widget_for_response (GTK_DIALOG (data->dialog), GTK_RESPONSE_CLOSE),
				  "clicked",
				  G_CALLBACK (gtk_widget_destroy),
				  G_OBJECT (data->dialog));
	g_signal_connect (GET_WIDGET ("inverse_checkbutton"),
			  "toggled",
			  G_CALLBACK (apply_sort_order),
			  data);

	/* run dialog. */

	gtk_window_set_transient_for (GTK_WINDOW (data->dialog),
				      GTK_WINDOW (browser));
	gtk_window_set_modal (GTK_WINDOW (data->dialog), FALSE);
	gtk_widget_show (data->dialog);
}
