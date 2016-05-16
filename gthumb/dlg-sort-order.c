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
#include "gth-browser.h"
#include "gth-main.h"
#include "gtk-utils.h"


#define GET_WIDGET(name) _gtk_builder_get_widget (data->builder, (name))


enum {
	SELECTION_COLUMN_DATA,
	SELECTION_COLUMN_NAME
};


typedef struct {
	GthBrowser *browser;
	GtkBuilder *builder;
	GtkWidget  *dialog;
	GtkWidget  *sort_by_combobox;
} DialogData;


/* called when the main dialog is closed. */
static void
destroy_cb (GtkWidget  *widget, 
	    DialogData *data)
{
	gth_browser_set_dialog (data->browser, "sort-order", NULL);
	g_object_unref (data->builder);
	g_free (data);
}


static void
apply_sort_order (GtkWidget  *widget,
		  DialogData *data)
{
	GtkTreeIter      iter;
	GtkTreeModel    *tree_model;
	GthFileDataSort *sort_type;
	
	if (! gtk_combo_box_get_active_iter (GTK_COMBO_BOX (data->sort_by_combobox), &iter))
		return;

	tree_model = gtk_combo_box_get_model (GTK_COMBO_BOX (data->sort_by_combobox));
	gtk_tree_model_get (tree_model, &iter, SELECTION_COLUMN_DATA, &sort_type, -1);
	
	gth_browser_set_sort_order (data->browser,
				    sort_type,
				    gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("inverse_checkbutton"))));
}


void
dlg_sort_order (GthBrowser *browser)
{
	DialogData      *data;
	GtkListStore    *selection_model;
	GtkCellRenderer *renderer;
	GthFileData     *file_data;
	GList           *sort_types;
	GList           *scan;
	GthFileDataSort *current_sort_type;
	gboolean         sort_inverse;
	int              i, i_active;
	
	if (gth_browser_get_dialog (browser, "sort-order") != NULL) {
		gtk_window_present (GTK_WINDOW (gth_browser_get_dialog (browser, "sort-order")));
		return;
	}

	data = g_new0 (DialogData, 1);
	data->browser = browser;
	data->builder = _gtk_builder_new_from_file ("sort-order.ui", NULL); 

	/* Get the widgets. */

	data->dialog = _gtk_builder_get_widget (data->builder, "sort_order_dialog");
	gth_browser_set_dialog (browser, "sort-order", data->dialog);
	g_object_set_data (G_OBJECT (data->dialog), "dialog_data", data);

	/* Set widgets data. */

	selection_model = gtk_list_store_new (2, G_TYPE_POINTER, G_TYPE_STRING);
	data->sort_by_combobox = gtk_combo_box_new_with_model (GTK_TREE_MODEL (selection_model));
	g_object_unref (selection_model);
  	
	renderer = gtk_cell_renderer_text_new ();
	gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (data->sort_by_combobox),
				    renderer,
				    TRUE);
	gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (data->sort_by_combobox),
					renderer,
					"text", SELECTION_COLUMN_NAME,
					NULL);
  	
	file_data = gth_browser_get_location_data (data->browser);
	if (file_data != NULL) {
		current_sort_type = gth_main_get_sort_type (g_file_info_get_attribute_string (file_data->info, "sort::type"));
		sort_inverse = g_file_info_get_attribute_boolean (file_data->info, "sort::inverse");
	}
	else
		gth_browser_get_sort_order (data->browser, &current_sort_type, &sort_inverse);

	sort_types = gth_main_get_all_sort_types ();
	for (i = 0, i_active = 0, scan = sort_types; scan; scan = scan->next, i++) {
		GthFileDataSort *sort_type = scan->data;
		GtkTreeIter      iter;
		
		if (strcmp (sort_type->name, current_sort_type->name) == 0)
			i_active = i;
		
		gtk_list_store_append (selection_model, &iter);
		gtk_list_store_set (selection_model, &iter,
				    SELECTION_COLUMN_DATA, sort_type,
				    SELECTION_COLUMN_NAME, _(sort_type->display_name),
				    -1);
	}
	g_list_free (sort_types);

	gtk_combo_box_set_active (GTK_COMBO_BOX (data->sort_by_combobox), i_active);
	gtk_widget_show (data->sort_by_combobox);
	gtk_container_add (GTK_CONTAINER (GET_WIDGET ("sort_by_hbox")), data->sort_by_combobox);

	gtk_label_set_use_underline (GTK_LABEL (GET_WIDGET ("sort_by_label")), TRUE);
	gtk_label_set_mnemonic_widget (GTK_LABEL (GET_WIDGET ("sort_by_label")), data->sort_by_combobox);

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("inverse_checkbutton")), sort_inverse);

	/* Set the signals handlers. */
	
	g_signal_connect (G_OBJECT (data->dialog), 
			  "destroy",
			  G_CALLBACK (destroy_cb),
			  data);
	g_signal_connect_swapped (GET_WIDGET ("close_button"), 
				  "clicked",
				  G_CALLBACK (gtk_widget_destroy),
				  G_OBJECT (data->dialog));
	g_signal_connect (GET_WIDGET ("inverse_checkbutton"), 
			  "toggled",
			  G_CALLBACK (apply_sort_order),
			  data);
	g_signal_connect (data->sort_by_combobox, 
			  "changed",
			  G_CALLBACK (apply_sort_order),
			  data);
			    
	/* run dialog. */

	gtk_window_set_transient_for (GTK_WINDOW (data->dialog), 
				      GTK_WINDOW (browser));
	gtk_window_set_modal (GTK_WINDOW (data->dialog), FALSE);
	gtk_widget_show (data->dialog);
}
