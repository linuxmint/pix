/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2008 The Free Software Foundation, Inc.
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
#include "dlg-personalize-filters.h"
#include "glib-utils.h"
#include "gth-filter-editor-dialog.h"
#include "gth-main.h"
#include "gth-preferences.h"
#include "gtk-utils.h"


#define GET_WIDGET(name) _gtk_builder_get_widget (data->builder, (name))
#define ORDER_CHANGED_DELAY 250


enum {
	COLUMN_FILTER,
	COLUMN_NAME,
	COLUMN_VISIBLE,
	COLUMN_STYLE,
	NUM_COLUMNS
};


typedef struct {
	GthBrowser   *browser;
	GtkBuilder   *builder;
	GSettings    *settings;
	GtkWidget    *dialog;
	GtkWidget    *list_view;
	GtkWidget    *general_filter_combobox;
	GtkListStore *list_store;
	gulong        filters_changed_id;
	guint         list_changed_id;
	GList        *general_tests;
} DialogData;


static void
destroy_cb (GtkWidget  *widget,
	    DialogData *data)
{
	if (data->list_changed_id != 0)
		g_source_remove (data->list_changed_id);
	data->list_changed_id = 0;

	gth_browser_set_dialog (data->browser, "personalize_filters", NULL);
	g_signal_handler_disconnect (gth_main_get_default_monitor (), data->filters_changed_id);

	_g_string_list_free (data->general_tests);
	g_object_unref (data->builder);
	g_object_unref (data->settings);
	g_free (data);
}


static gboolean
list_view_row_order_changed_cb (gpointer user_data)
{
	DialogData   *data = user_data;
	GtkTreeModel *model = GTK_TREE_MODEL (data->list_store);
	GtkTreeIter   iter;

	if (data->list_changed_id != 0)
		g_source_remove (data->list_changed_id);
	data->list_changed_id = 0;

	if (gtk_tree_model_get_iter_first (model, &iter)) {
		GthFilterFile *filter_file;

		filter_file = gth_main_get_default_filter_file ();
		gth_filter_file_clear (filter_file);

		do {
			GthTest *test;

			gtk_tree_model_get (model, &iter, COLUMN_FILTER, &test, -1);
			gth_filter_file_add (filter_file, test);
			g_object_unref (test);
		}
		while (gtk_tree_model_iter_next (model, &iter));

		gth_main_filters_changed ();
	}

	return FALSE;
}


static void
row_deleted_cb (GtkTreeModel *tree_model,
		GtkTreePath  *path,
		gpointer      user_data)
{
	DialogData *data = user_data;

	if (data->list_changed_id != 0)
		g_source_remove (data->list_changed_id);
	data->list_changed_id = g_timeout_add (ORDER_CHANGED_DELAY, list_view_row_order_changed_cb, data);
}


static void
row_inserted_cb (GtkTreeModel *tree_model,
		 GtkTreePath  *path,
		 GtkTreeIter  *iter,
		 gpointer      user_data)
{
	DialogData *data = user_data;

	if (data->list_changed_id != 0)
		g_source_remove (data->list_changed_id);
	data->list_changed_id = g_timeout_add (ORDER_CHANGED_DELAY, list_view_row_order_changed_cb, data);
}


static void
set_filter_list (DialogData *data,
		 GList      *filter_list)
{
	GList *scan;

	g_signal_handlers_block_by_func (data->list_store, row_inserted_cb, data);

	for (scan = filter_list; scan; scan = scan->next) {
		GthTest     *test = scan->data;
		GtkTreeIter  iter;

		gtk_list_store_append (data->list_store, &iter);
		gtk_list_store_set (data->list_store, &iter,
				    COLUMN_FILTER, test,
				    COLUMN_NAME, gth_test_get_display_name (test),
				    COLUMN_VISIBLE, gth_test_is_visible (test),
				    COLUMN_STYLE, GTH_IS_FILTER (test) ? PANGO_STYLE_NORMAL : PANGO_STYLE_ITALIC,
				   -1);
	}

	g_signal_handlers_unblock_by_func (data->list_store, row_inserted_cb, data);
}


static void
update_filter_list (DialogData *data)
{
	GList *filter_list;

	g_signal_handlers_block_by_func (data->list_store, row_deleted_cb, data);
	gtk_list_store_clear (data->list_store);
	g_signal_handlers_unblock_by_func (data->list_store, row_deleted_cb, data);

	filter_list = gth_main_get_all_filters ();
	set_filter_list (data, filter_list);
	_g_object_list_unref (filter_list);
}


static void
filters_changed_cb (GthMonitor *monitor,
		    DialogData *data)
{
	update_filter_list (data);
}


static void
cell_renderer_toggle_toggled_cb (GtkCellRendererToggle *cell_renderer,
				 char                  *path,
                                 gpointer               user_data)
{
	DialogData  *data = user_data;
	GtkTreePath *tpath;
	GtkTreeIter  iter;
	gboolean     visible;

	tpath = gtk_tree_path_new_from_string (path);
	if (tpath == NULL)
		return;

	if (gtk_tree_model_get_iter (GTK_TREE_MODEL (data->list_store), &iter, tpath)) {
		GthTest       *filter;
		GthFilterFile *filter_file;

		gtk_tree_model_get (GTK_TREE_MODEL (data->list_store), &iter,
				    COLUMN_FILTER, &filter,
				    COLUMN_VISIBLE, &visible,
				    -1);
		visible = ! visible;
		g_object_set (filter, "visible", visible, NULL);

		g_signal_handlers_block_by_func (gth_main_get_default_monitor (), filters_changed_cb, data);
		filter_file = gth_main_get_default_filter_file ();
		gth_filter_file_add (filter_file, filter);
		gth_main_filters_changed ();
		g_signal_handlers_unblock_by_func (gth_main_get_default_monitor (), filters_changed_cb, data);

		gtk_list_store_set (data->list_store, &iter,
				    COLUMN_VISIBLE, visible,
				    -1);

		g_object_unref (filter);
	}

	gtk_tree_path_free (tpath);
}


static void
add_columns (GtkTreeView *treeview,
	     DialogData  *data)
{
	GtkCellRenderer   *renderer;
	GtkTreeViewColumn *column;

	/* the name column. */

	column = gtk_tree_view_column_new ();
	gtk_tree_view_column_set_title (column, _("Filter"));

	renderer = gtk_cell_renderer_text_new ();
        gtk_tree_view_column_pack_start (column, renderer, TRUE);
        gtk_tree_view_column_set_attributes (column, renderer,
                                             "text", COLUMN_NAME,
                                             "style", COLUMN_STYLE,
                                             NULL);

        gtk_tree_view_column_set_expand (column, TRUE);
        gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);

	/* the checkbox column */

	column = gtk_tree_view_column_new ();
	gtk_tree_view_column_set_title (column, _("Show"));

	renderer = gtk_cell_renderer_toggle_new ();
	g_signal_connect (renderer,
			  "toggled",
			  G_CALLBACK (cell_renderer_toggle_toggled_cb),
			  data);

	gtk_tree_view_column_pack_start (column, renderer, TRUE);
	gtk_tree_view_column_set_attributes (column, renderer,
					     "active", COLUMN_VISIBLE,
					     NULL);

	gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);
}


static gboolean
get_test_iter (DialogData  *data,
	       GthTest     *test,
	       GtkTreeIter *iter)
{
	GtkTreeModel *model = GTK_TREE_MODEL (data->list_store);
	gboolean      found = FALSE;
	const char   *test_id;

	test_id = gth_test_get_id (test);
	if (! gtk_tree_model_get_iter_first (model, iter))
		return FALSE;

	do {
		GthTest *list_test;

		gtk_tree_model_get (model, iter, COLUMN_FILTER, &list_test, -1);
		found = g_strcmp0 (test_id, gth_test_get_id (list_test)) == 0;
		g_object_unref (list_test);
	}
	while (! found && gtk_tree_model_iter_next (model, iter));

	return found;
}


static void
filter_editor_dialog__response_cb (GtkDialog *dialog,
			           int        response,
			           gpointer   user_data)
{
	DialogData    *data = user_data;
	GthFilter     *filter;
	GError        *error = NULL;
	GthFilterFile *filter_file;
	gboolean       new_filter;
	GtkTreeIter    iter;
	gboolean       change_list = TRUE;

	if (response != GTK_RESPONSE_OK) {
		gtk_widget_destroy (GTK_WIDGET (dialog));
		return;
	}

	filter = gth_filter_editor_dialog_get_filter (GTH_FILTER_EDITOR_DIALOG (dialog), &error);
	if (filter == NULL) {
		_gtk_error_dialog_from_gerror_show (GTK_WINDOW (dialog), _("Could not save the filter"), error);
		g_clear_error (&error);
		return;
	}

	/* update the filter file */

	filter_file = gth_main_get_default_filter_file ();
	new_filter = ! gth_filter_file_has_test (filter_file, GTH_TEST (filter));

	g_signal_handlers_block_by_func (gth_main_get_default_monitor (), filters_changed_cb, data);
	gth_filter_file_add (filter_file, GTH_TEST (filter));
	gth_main_filters_changed ();
	g_signal_handlers_unblock_by_func (gth_main_get_default_monitor (), filters_changed_cb, data);

	/* update the filter list */

	if (new_filter) {
		g_signal_handlers_block_by_func (data->list_store, row_inserted_cb, data);
		gtk_list_store_append (data->list_store, &iter);
		g_signal_handlers_unblock_by_func (data->list_store, row_inserted_cb, data);
	}
	else
		change_list = get_test_iter (data, GTH_TEST (filter), &iter);

	if (change_list)
		gtk_list_store_set (data->list_store, &iter,
				    COLUMN_FILTER, filter,
				    COLUMN_NAME, gth_test_get_display_name (GTH_TEST (filter)),
				    COLUMN_VISIBLE, gth_test_is_visible (GTH_TEST (filter)),
				    -1);

	g_object_unref (filter);
	gtk_widget_destroy (GTK_WIDGET (dialog));
}


static void
new_filter_cb (GtkButton  *button,
	       DialogData *data)
{
	GtkWidget *dialog;

	dialog = gth_filter_editor_dialog_new (_("New Filter"), GTK_WINDOW (data->dialog));
	g_signal_connect (dialog, "response",
			  G_CALLBACK (filter_editor_dialog__response_cb),
			  data);
	gtk_window_set_modal (GTK_WINDOW (dialog), TRUE);
	gtk_window_present (GTK_WINDOW (dialog));
}


static void
edit_filter_cb (GtkButton  *button,
	        DialogData *data)
{
	GtkTreeSelection *selection;
	GtkTreeModel     *model = GTK_TREE_MODEL (data->list_store);
	GtkTreeIter       iter;
	GthTest          *filter;

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (data->list_view));
	if (! gtk_tree_selection_get_selected (selection, &model, &iter))
		return;

	gtk_tree_model_get (model, &iter, COLUMN_FILTER, &filter, -1);
	if (filter == NULL)
		return;

	if (GTH_IS_FILTER (filter)) {
		GtkWidget *dialog;

		dialog = gth_filter_editor_dialog_new (_("Edit Filter"), GTK_WINDOW (data->dialog));
		gth_filter_editor_dialog_set_filter (GTH_FILTER_EDITOR_DIALOG (dialog), GTH_FILTER (filter));
		g_signal_connect (dialog, "response",
				  G_CALLBACK (filter_editor_dialog__response_cb),
				  data);
		gtk_window_set_modal (GTK_WINDOW (dialog), TRUE);
		gtk_window_present (GTK_WINDOW (dialog));
	}

	g_object_unref (filter);
}


static void
delete_filter_cb (GtkButton  *button,
	          DialogData *data)
{
	GtkTreeSelection *selection;
	GtkTreeModel     *model = GTK_TREE_MODEL (data->list_store);
	GtkTreeIter       iter;
	GthTest          *filter;

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (data->list_view));
	if (! gtk_tree_selection_get_selected (selection, &model, &iter))
		return;

	gtk_tree_model_get (model, &iter, COLUMN_FILTER, &filter, -1);
	if (filter == NULL)
		return;

	if (GTH_IS_FILTER (filter)) {
		GthFilterFile *filter_file;

		/* update the filter file */

		g_signal_handlers_block_by_func (gth_main_get_default_monitor (), filters_changed_cb, data);

		filter_file = gth_main_get_default_filter_file ();
		gth_filter_file_remove (filter_file, filter);
		gth_main_filters_changed ();

		g_signal_handlers_unblock_by_func (gth_main_get_default_monitor (), filters_changed_cb, data);

		/* update the filter list */

		g_signal_handlers_block_by_func (data->list_store, row_deleted_cb, data);
		gtk_list_store_remove (data->list_store, &iter);
		g_signal_handlers_unblock_by_func (data->list_store, row_deleted_cb, data);
	}

	g_object_unref (filter);
}


static void
update_sensitivity (DialogData *data)
{
	GtkTreeModel     *model;
	GtkTreeSelection *selection;
	GtkTreeIter       iter;
	GthTest          *test;
	gboolean          is_filter;

	model = GTK_TREE_MODEL (data->list_store);
	is_filter = FALSE;

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (data->list_view));
	if (gtk_tree_selection_get_selected (selection, &model, &iter)) {
		gtk_tree_model_get (model, &iter, COLUMN_FILTER, &test, -1);
		if (test != NULL) {
			is_filter = GTH_IS_FILTER (test);
			g_object_unref (test);
		}
	}

	gtk_widget_set_sensitive (GET_WIDGET ("edit_button"), is_filter);
	gtk_widget_set_sensitive (GET_WIDGET ("delete_button"), is_filter);
}


static void
list_view_selection_changed_cb (GtkTreeSelection *treeselection,
				gpointer          user_data)
{
	update_sensitivity ((DialogData *) user_data);
}


static void
list_view_row_activated_cb (GtkTreeView       *tree_view,
                            GtkTreePath       *path,
                            GtkTreeViewColumn *column,
                            gpointer           user_data)
{
	edit_filter_cb (NULL, user_data);
}


static void
general_filter_changed_cb (GtkComboBox *widget,
			   gpointer     user_data)
{
	DialogData *data = user_data;
	int         idx;

	idx = gtk_combo_box_get_active (widget);
	g_settings_set_string (data->settings, PREF_BROWSER_GENERAL_FILTER, g_list_nth (data->general_tests, idx)->data);
}


void
dlg_personalize_filters (GthBrowser *browser)
{
	DialogData *data;
	GList      *tests, *scan;
	char       *general_filter;
	int         i, active_filter;
	int         i_general;

	if (gth_browser_get_dialog (browser, "personalize_filters") != NULL) {
		gtk_window_present (GTK_WINDOW (gth_browser_get_dialog (browser, "personalize_filters")));
		return;
	}

	data = g_new0 (DialogData, 1);
	data->browser = browser;
	data->builder = _gtk_builder_new_from_file ("personalize-filters.ui", NULL);
	data->settings = g_settings_new (GTHUMB_BROWSER_SCHEMA);

	/* Get the widgets. */

	data->dialog = g_object_new (GTK_TYPE_DIALOG,
				     "title", _("Filters"),
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

	gth_browser_set_dialog (browser, "personalize_filters", data->dialog);
	g_object_set_data (G_OBJECT (data->dialog), "dialog_data", data);

	/* Set widgets data. */

	tests = gth_main_get_registered_objects_id (GTH_TYPE_TEST);
	general_filter = g_settings_get_string (data->settings, PREF_BROWSER_GENERAL_FILTER);
	active_filter = 0;

	data->general_filter_combobox = gtk_combo_box_text_new ();
	for (i = 0, i_general = -1, scan = tests; scan; scan = scan->next, i++) {
		const char *registered_test_id = scan->data;
		GthTest    *test;

		if (strncmp (registered_test_id, "file::type::", 12) != 0)
			continue;

		i_general += 1;

		if (strcmp (registered_test_id, general_filter) == 0)
			active_filter = i_general;

		test = gth_main_get_registered_object (GTH_TYPE_TEST, registered_test_id);
		data->general_tests = g_list_prepend (data->general_tests, g_strdup (gth_test_get_id (test)));
		gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (data->general_filter_combobox), gth_test_get_display_name (test));
		g_object_unref (test);
	}
	data->general_tests = g_list_reverse (data->general_tests);

	gtk_combo_box_set_active (GTK_COMBO_BOX (data->general_filter_combobox), active_filter);
	gtk_widget_show (data->general_filter_combobox);
	gtk_container_add (GTK_CONTAINER (GET_WIDGET ("general_filter_box")), data->general_filter_combobox);

	gtk_label_set_mnemonic_widget (GTK_LABEL (GET_WIDGET ("general_filter_label")), data->general_filter_combobox);
	gtk_label_set_use_underline (GTK_LABEL (GET_WIDGET ("general_filter_label")), TRUE);

	g_free (general_filter);
	_g_string_list_free (tests);

	/**/

	data->list_store = gtk_list_store_new (NUM_COLUMNS,
					       G_TYPE_OBJECT,
					       G_TYPE_STRING,
					       G_TYPE_BOOLEAN,
					       PANGO_TYPE_STYLE);
	data->list_view = gtk_tree_view_new_with_model (GTK_TREE_MODEL (data->list_store));
	g_object_unref (data->list_store);
        gtk_tree_view_set_reorderable (GTK_TREE_VIEW (data->list_view), TRUE);
        gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (data->list_view), TRUE);

	add_columns (GTK_TREE_VIEW (data->list_view), data);

	gtk_widget_show (data->list_view);
	gtk_container_add (GTK_CONTAINER (GET_WIDGET ("filters_scrolledwindow")), data->list_view);

	gtk_label_set_mnemonic_widget (GTK_LABEL (GET_WIDGET ("filters_label")), data->list_view);
	gtk_label_set_use_underline (GTK_LABEL (GET_WIDGET ("filters_label")), TRUE);

	update_filter_list (data);
	update_sensitivity (data);

	/* Set the signals handlers. */

	g_signal_connect (G_OBJECT (data->dialog),
			  "destroy",
			  G_CALLBACK (destroy_cb),
			  data);
	g_signal_connect_swapped (gtk_dialog_get_widget_for_response (GTK_DIALOG (data->dialog), GTK_RESPONSE_CLOSE),
				  "clicked",
				  G_CALLBACK (gtk_widget_destroy),
				  G_OBJECT (data->dialog));
	g_signal_connect (G_OBJECT (GET_WIDGET ("new_button")),
			  "clicked",
			  G_CALLBACK (new_filter_cb),
			  data);
	g_signal_connect (G_OBJECT (GET_WIDGET ("edit_button")),
			  "clicked",
			  G_CALLBACK (edit_filter_cb),
			  data);
	g_signal_connect (G_OBJECT (GET_WIDGET ("delete_button")),
			  "clicked",
			  G_CALLBACK (delete_filter_cb),
			  data);
	g_signal_connect (gtk_tree_view_get_selection (GTK_TREE_VIEW (data->list_view)),
			  "changed",
			  G_CALLBACK (list_view_selection_changed_cb),
			  data);
	g_signal_connect (GTK_TREE_VIEW (data->list_view),
			  "row-activated",
			  G_CALLBACK (list_view_row_activated_cb),
			  data);
	g_signal_connect (data->list_store,
			  "row-deleted",
			  G_CALLBACK (row_deleted_cb),
			  data);
	g_signal_connect (data->list_store,
			  "row-inserted",
			  G_CALLBACK (row_inserted_cb),
			  data);
	g_signal_connect (G_OBJECT (data->general_filter_combobox),
			  "changed",
			  G_CALLBACK (general_filter_changed_cb),
			  data);

	data->filters_changed_id = g_signal_connect (gth_main_get_default_monitor (),
				                     "filters-changed",
				                     G_CALLBACK (filters_changed_cb),
				                     data);

	/* run dialog. */

	gtk_widget_show (data->dialog);
}
