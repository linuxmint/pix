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
#include <gthumb.h>
#include "preferences.h"


#define GET_WIDGET(name) _gtk_builder_get_widget (data->builder, (name))
#define BROWSER_DATA_KEY "save-options-preference-data"


enum {
	FILE_TYPE_COLUMN_N,
	FILE_TYPE_COLUMN_OBJ,
	FILE_TYPE_COLUMN_DISPLAY_NAME
};


typedef struct {
	GtkBuilder *builder;
	GList      *image_saver;
} BrowserData;


static void
browser_data_free (BrowserData *data)
{
	_g_object_list_unref (data->image_saver);
	g_object_unref (data->builder);
	g_free (data);
}


static void
treeselection_changed_cb (GtkTreeSelection *treeselection,
			  gpointer          user_data)
{
	GtkWidget      *dialog = user_data;
	BrowserData    *data;
	GtkTreeIter     iter;
	int             page_n;
	GthImageSaver *image_saver;

	data = g_object_get_data (G_OBJECT (dialog), BROWSER_DATA_KEY);
	g_return_if_fail (data != NULL);

	if (! gtk_tree_selection_get_selected (treeselection, NULL, &iter))
		return;

	gtk_tree_model_get (GTK_TREE_MODEL (gtk_builder_get_object (data->builder, "file_type_liststore")), &iter,
			    FILE_TYPE_COLUMN_N, &page_n,
			    FILE_TYPE_COLUMN_OBJ, &image_saver,
			    -1);
	gtk_notebook_set_current_page (GTK_NOTEBOOK (_gtk_builder_get_widget (data->builder, "options_notebook")), page_n);
	gtk_label_set_text (GTK_LABEL (_gtk_builder_get_widget (data->builder, "file_type_label")), gth_image_saver_get_display_name (image_saver));

	g_object_unref (image_saver);
}


void
ci__dlg_preferences_construct_cb (GtkWidget  *dialog,
				  GthBrowser *browser,
				  GtkBuilder *dialog_builder)
{
	BrowserData      *data;
	GtkWidget        *notebook;
	GtkWidget        *page;
	GtkListStore     *model;
	GArray           *image_saver_types;
	int               i;
	GtkTreeSelection *treeselection;
	GtkTreePath      *path;
	GtkWidget        *label;

	data = g_new0 (BrowserData, 1);
	data->builder = _gtk_builder_new_from_file ("save-options-preferences.ui", "cairo_io");

	notebook = _gtk_builder_get_widget (dialog_builder, "notebook");

	page = _gtk_builder_get_widget (data->builder, "preferences_page");
	gtk_widget_show (page);

	model = (GtkListStore *) gtk_builder_get_object (data->builder, "file_type_liststore");
	image_saver_types = gth_main_get_type_set ("image-saver");
	for (i = 0; (image_saver_types != NULL) && (i < image_saver_types->len); i++) {
		GthImageSaver *image_saver;
		GtkTreeIter     iter;
		GtkWidget      *options;

		image_saver = g_object_new (g_array_index (image_saver_types, GType, i), NULL);

		gtk_list_store_append (model, &iter);
		gtk_list_store_set (model, &iter,
				    FILE_TYPE_COLUMN_N, i,
				    FILE_TYPE_COLUMN_OBJ, image_saver,
				    FILE_TYPE_COLUMN_DISPLAY_NAME, gth_image_saver_get_display_name (image_saver),
				    -1);

		options = gth_image_saver_get_control (image_saver);
		gtk_widget_show (options);
		gtk_notebook_append_page (GTK_NOTEBOOK (_gtk_builder_get_widget (data->builder, "options_notebook")), options, NULL);

		data->image_saver = g_list_prepend (data->image_saver, image_saver);
	}

	treeselection = gtk_tree_view_get_selection (GTK_TREE_VIEW (_gtk_builder_get_widget (data->builder, "file_type_treeview")));
	gtk_tree_selection_set_mode (treeselection, GTK_SELECTION_BROWSE);
	g_signal_connect (treeselection,
			  "changed",
			  G_CALLBACK (treeselection_changed_cb),
			  dialog);

	label = gtk_label_new (_("Saving"));
	gtk_widget_show (label);

	gtk_notebook_append_page (GTK_NOTEBOOK (notebook), page, label);
	g_object_set_data_full (G_OBJECT (dialog), BROWSER_DATA_KEY, data, (GDestroyNotify) browser_data_free);

	path = gtk_tree_path_new_first ();
	gtk_tree_selection_select_path (treeselection, path);
	gtk_tree_path_free (path);
}


void
ci__dlg_preferences_apply_cb (GtkWidget  *dialog,
			      GthBrowser *browser,
			      GtkBuilder *builder)
{
	BrowserData *data;
	GList       *scan;

	data = g_object_get_data (G_OBJECT (dialog), BROWSER_DATA_KEY);
	g_return_if_fail (data != NULL);

	for (scan = data->image_saver; scan; scan = scan->next) {
		GthImageSaver *image_saver = scan->data;
		gth_image_saver_save_options (image_saver);
	}
}
