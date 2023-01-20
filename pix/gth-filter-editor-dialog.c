/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2008-2009 The Free Software Foundation, Inc.
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
#include <stdlib.h>
#include <gtk/gtk.h>
#include "glib-utils.h"
#include "gth-filter-editor-dialog.h"
#include "gth-main.h"
#include "gth-test-chain.h"
#include "gth-test-selector.h"
#include "gtk-utils.h"


enum {
	SELECTION_COLUMN_DATA,
	SELECTION_COLUMN_NAME
};


#define GET_WIDGET(name) _gtk_builder_get_widget (self->priv->builder, (name))


struct _GthFilterEditorDialogPrivate {
	GtkBuilder *builder;
	GtkWidget  *match_type_combobox;
	GtkWidget  *limit_object_combobox;
	GtkWidget  *selection_combobox;
	GtkWidget  *selection_order_combobox;
	char       *filter_id;
	gboolean    filter_visible;
};


G_DEFINE_TYPE_WITH_CODE (GthFilterEditorDialog,
			 gth_filter_editor_dialog,
			 GTK_TYPE_DIALOG,
			 G_ADD_PRIVATE (GthFilterEditorDialog))


static void
gth_filter_editor_dialog_finalize (GObject *object)
{
	GthFilterEditorDialog *dialog;

	dialog = GTH_FILTER_EDITOR_DIALOG (object);

	g_object_unref (dialog->priv->builder);
	g_free (dialog->priv->filter_id);

	G_OBJECT_CLASS (gth_filter_editor_dialog_parent_class)->finalize (object);
}


static void
gth_filter_editor_dialog_class_init (GthFilterEditorDialogClass *class)
{
	GObjectClass *object_class;

	object_class = (GObjectClass*) class;
	object_class->finalize = gth_filter_editor_dialog_finalize;
}


static void
gth_filter_editor_dialog_init (GthFilterEditorDialog *dialog)
{
	dialog->priv = gth_filter_editor_dialog_get_instance_private (dialog);
	dialog->priv->builder = NULL;
	dialog->priv->match_type_combobox = NULL;
	dialog->priv->limit_object_combobox = NULL;
	dialog->priv->selection_combobox = NULL;
	dialog->priv->selection_order_combobox = NULL;
	dialog->priv->filter_id = NULL;
	dialog->priv->filter_visible = FALSE;
}


static void
update_sensitivity (GthFilterEditorDialog *self)
{
	gboolean  active;
	GList    *test_selectors;
	int       many_selectors;
	GList    *scan;

	active = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("limit_to_checkbutton")));
	gtk_widget_set_sensitive (GET_WIDGET ("limit_options_hbox"), active);
	gtk_widget_set_sensitive (GET_WIDGET ("selection_box"), active);

	active = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("match_checkbutton")));
	gtk_widget_set_sensitive (GET_WIDGET ("match_type_combobox_box"), active);
	gtk_widget_set_sensitive (GET_WIDGET ("tests_box"), active);

	test_selectors = gtk_container_get_children (GTK_CONTAINER (GET_WIDGET ("tests_box")));
	many_selectors = (test_selectors != NULL) && (test_selectors->next != NULL);
	for (scan = test_selectors; scan; scan = scan->next)
		gth_test_selector_can_remove (GTH_TEST_SELECTOR (scan->data), many_selectors);
	g_list_free (test_selectors);
}


static void
match_checkbutton_toggled_cb (GtkToggleButton *button,
			      gpointer         user_data)
{
	GthFilterEditorDialog *self = user_data;
	update_sensitivity (self);
}


static void
limit_to_checkbutton_toggled_cb (GtkToggleButton *button,
			         gpointer         user_data)
{
	GthFilterEditorDialog *self = user_data;
	update_sensitivity (self);
}


static void
gth_filter_editor_dialog_construct (GthFilterEditorDialog *self)
{
	GtkWidget       *content;
	GList           *sort_types;
	GList           *scan;
	GtkListStore    *selection_model;
	GtkCellRenderer *renderer;

    	gtk_window_set_resizable (GTK_WINDOW (self), FALSE);
	gtk_box_set_spacing (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (self))), 5);
	gtk_container_set_border_width (GTK_CONTAINER (self), 5);

	gtk_dialog_add_button (GTK_DIALOG (self), _GTK_LABEL_CANCEL, GTK_RESPONSE_CANCEL);
	gtk_dialog_add_button (GTK_DIALOG (self), _GTK_LABEL_SAVE, GTK_RESPONSE_OK);

	_gtk_dialog_add_class_to_response (GTK_DIALOG (self), GTK_RESPONSE_OK, GTK_STYLE_CLASS_SUGGESTED_ACTION);

    	self->priv->builder = _gtk_builder_new_from_file ("filter-editor.ui", NULL);

    	content = _gtk_builder_get_widget (self->priv->builder, "filter_editor");
    	gtk_container_set_border_width (GTK_CONTAINER (content), 5);
  	gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (self))), content, TRUE, TRUE, 0);

	/**/

  	self->priv->match_type_combobox = gtk_combo_box_text_new ();
  	_gtk_combo_box_append_texts (GTK_COMBO_BOX_TEXT (self->priv->match_type_combobox),
  				     _("all the following rules"),
  				     _("any of the following rules"),
  				     NULL);
	gtk_combo_box_set_active (GTK_COMBO_BOX (self->priv->match_type_combobox), 0);
  	gtk_widget_show (self->priv->match_type_combobox);
  	gtk_container_add (GTK_CONTAINER (GET_WIDGET ("match_type_combobox_box")),
  			   self->priv->match_type_combobox);

	/**/

  	self->priv->limit_object_combobox = gtk_combo_box_text_new ();
  	_gtk_combo_box_append_texts (GTK_COMBO_BOX_TEXT (self->priv->limit_object_combobox),
  				     _("files"),
  				     _("kB"),
  				     _("MB"),
  				     _("GB"),
  				     NULL);
  	gtk_combo_box_set_active (GTK_COMBO_BOX (self->priv->limit_object_combobox), 0);
  	gtk_widget_show (self->priv->limit_object_combobox);
  	gtk_container_add (GTK_CONTAINER (GET_WIDGET ("limit_object_combobox_box")),
  			   self->priv->limit_object_combobox);

	/**/

	selection_model = gtk_list_store_new (2, G_TYPE_POINTER, G_TYPE_STRING);
	self->priv->selection_combobox = gtk_combo_box_new_with_model (GTK_TREE_MODEL (selection_model));
	g_object_unref (selection_model);

	renderer = gtk_cell_renderer_text_new ();
	gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (self->priv->selection_combobox),
				    renderer,
				    TRUE);
	gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (self->priv->selection_combobox),
					renderer,
					"text", SELECTION_COLUMN_NAME,
					NULL);

	sort_types = gth_main_get_all_sort_types ();
	for (scan = sort_types; scan; scan = scan->next) {
		GthFileDataSort *sort_type = scan->data;
		GtkTreeIter      iter;

		gtk_list_store_append (selection_model, &iter);
		gtk_list_store_set (selection_model, &iter,
				    SELECTION_COLUMN_DATA, sort_type,
				    SELECTION_COLUMN_NAME, _(sort_type->display_name),
				    -1);
	}
	g_list_free (sort_types);

	gtk_combo_box_set_active (GTK_COMBO_BOX (self->priv->selection_combobox), 0);
	gtk_widget_show (self->priv->selection_combobox);
	gtk_container_add (GTK_CONTAINER (GET_WIDGET ("selection_combobox_box")),
			   self->priv->selection_combobox);

	/**/

	self->priv->selection_order_combobox = gtk_combo_box_text_new ();
  	_gtk_combo_box_append_texts (GTK_COMBO_BOX_TEXT (self->priv->selection_order_combobox),
  				     _("ascending"),
  				     _("descending"),
  				     NULL);
  	gtk_combo_box_set_active (GTK_COMBO_BOX (self->priv->selection_order_combobox), 0);
  	gtk_widget_show (self->priv->selection_order_combobox);
  	gtk_container_add (GTK_CONTAINER (GET_WIDGET ("selection_order_combobox_box")),
  			   self->priv->selection_order_combobox);

  	/**/

  	g_signal_connect (GET_WIDGET ("match_checkbutton"),
  			  "toggled",
  			  G_CALLBACK (match_checkbutton_toggled_cb),
  			  self);
	g_signal_connect (GET_WIDGET ("limit_to_checkbutton"),
  			  "toggled",
  			  G_CALLBACK (limit_to_checkbutton_toggled_cb),
  			  self);

  	gth_filter_editor_dialog_set_filter (self, NULL);
}


GtkWidget *
gth_filter_editor_dialog_new (const char *title,
			      GtkWindow  *parent)
{
	GthFilterEditorDialog *self;

	self = g_object_new (GTH_TYPE_FILTER_EDITOR_DIALOG,
			     "title", title,
			     "transient-for", parent,
			     "use-header-bar", _gtk_settings_get_dialogs_use_header (),
			     NULL);
	gth_filter_editor_dialog_construct (self);

	return (GtkWidget *) self;
}


static GtkWidget *
_gth_filter_editor_dialog_add_test (GthFilterEditorDialog *self,
				    int                    pos);


static void
test_selector_add_test_cb (GthTestSelector       *selector,
			   GthFilterEditorDialog *self)
{
	int pos;

	pos = _gtk_container_get_pos (GTK_CONTAINER (GET_WIDGET ("tests_box")), (GtkWidget*) selector);
	_gth_filter_editor_dialog_add_test (self, pos == -1 ? -1 : pos + 1);
	update_sensitivity (self);
}


static void
test_selector_remove_test_cb (GthTestSelector       *selector,
			      GthFilterEditorDialog *self)
{
	gtk_container_remove (GTK_CONTAINER (GET_WIDGET ("tests_box")), (GtkWidget*) selector);
	update_sensitivity (self);
}


static GtkWidget *
_gth_filter_editor_dialog_add_test (GthFilterEditorDialog *self,
				    int                    pos)
{
	GtkWidget *test_selector;

	test_selector = gth_test_selector_new ();
	gtk_widget_show (test_selector);

	g_signal_connect (G_OBJECT (test_selector),
			  "add_test",
			  G_CALLBACK (test_selector_add_test_cb),
			  self);
	g_signal_connect (G_OBJECT (test_selector),
			  "remove_test",
			  G_CALLBACK (test_selector_remove_test_cb),
			  self);

	gtk_box_pack_start (GTK_BOX (GET_WIDGET ("tests_box")), test_selector, FALSE, FALSE, 0);

	if (pos >= 0)
		gtk_box_reorder_child (GTK_BOX (GET_WIDGET ("tests_box")),
				       test_selector,
				       pos);

	return test_selector;
}


static void
_gth_filter_editor_dialog_set_new_filter (GthFilterEditorDialog *self)
{
	gtk_entry_set_text (GTK_ENTRY (GET_WIDGET ("name_entry")), "");

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("match_checkbutton")), FALSE);
	gtk_combo_box_set_active (GTK_COMBO_BOX (self->priv->match_type_combobox), 0);
	_gtk_container_remove_children (GTK_CONTAINER (GET_WIDGET ("tests_box")), NULL, NULL);

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("limit_to_checkbutton")), FALSE);
	gtk_entry_set_text (GTK_ENTRY (GET_WIDGET ("limit_size_entry")), "");
	gtk_combo_box_set_active (GTK_COMBO_BOX (self->priv->limit_object_combobox), 0);
	gtk_combo_box_set_active (GTK_COMBO_BOX (self->priv->selection_combobox), 0);
}


static gboolean
get_iter_for_sort_type (GthFilterEditorDialog *self,
			const char            *sort_type_name,
			GtkTreeIter           *iter)
{
	GtkTreeModel *model;

	model = gtk_combo_box_get_model (GTK_COMBO_BOX (self->priv->selection_combobox));

	if (! gtk_tree_model_get_iter_first (model, iter))
		return FALSE;
	do {
		GthFileDataSort *sort_type;

		gtk_tree_model_get (model, iter, SELECTION_COLUMN_DATA, &sort_type, -1);
		if (g_strcmp0 (sort_type->name, sort_type_name) == 0)
			return TRUE;
	}
	while (gtk_tree_model_iter_next (model, iter));

	return FALSE;
}


void
gth_filter_editor_dialog_set_filter (GthFilterEditorDialog *self,
				     GthFilter             *filter)
{
	GthTestChain *chain;
	GthMatchType  match_type;
	GthLimitType  limit_type;
	goffset       limit_value;
	const char   *sort_name;
	GtkSortType   sort_direction;

	g_free (self->priv->filter_id);
	self->priv->filter_id = NULL;
	self->priv->filter_visible = TRUE;

	_gth_filter_editor_dialog_set_new_filter (self);

	if (filter == NULL) {
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("match_checkbutton")), TRUE);
		_gth_filter_editor_dialog_add_test (self, -1);
		update_sensitivity (self);
		return;
	}

	self->priv->filter_id = g_strdup (gth_test_get_id (GTH_TEST (filter)));
	self->priv->filter_visible = gth_test_is_visible (GTH_TEST (filter));

	gtk_entry_set_text (GTK_ENTRY (GET_WIDGET ("name_entry")), gth_test_get_display_name (GTH_TEST (filter)));

	chain = gth_filter_get_test (filter);
	match_type = chain != NULL ? gth_test_chain_get_match_type (chain) : GTH_MATCH_TYPE_NONE;

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("match_checkbutton")), match_type != GTH_MATCH_TYPE_NONE);
	gtk_combo_box_set_active (GTK_COMBO_BOX (self->priv->match_type_combobox), match_type == GTH_MATCH_TYPE_ANY ? 1 : 0);

	_gtk_container_remove_children (GTK_CONTAINER (GET_WIDGET ("tests_box")), NULL, NULL);
	if (match_type != GTH_MATCH_TYPE_NONE) {
		GList *tests;
		GList *scan;

		tests = gth_test_chain_get_tests (chain);
		for (scan = tests; scan; scan = scan->next) {
			GthTest   *test = scan->data;
			GtkWidget *test_selector;

			test_selector = _gth_filter_editor_dialog_add_test (self, -1);
			gth_test_selector_set_test (GTH_TEST_SELECTOR (test_selector), test);
		}
		_g_object_list_unref (tests);
	}
	else
		_gth_filter_editor_dialog_add_test (self, -1);

	gth_filter_get_limit (filter, &limit_type, &limit_value, &sort_name, &sort_direction);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("limit_to_checkbutton")), limit_type != GTH_LIMIT_TYPE_NONE);
	if (limit_type != GTH_LIMIT_TYPE_NONE) {
		char        *value;
		GtkTreeIter  iter;

		gtk_combo_box_set_active (GTK_COMBO_BOX (self->priv->limit_object_combobox), limit_type - 1);
		if (limit_type == GTH_LIMIT_TYPE_SIZE) {
			int sizes[] = { 1024, 1024 * 1024, 1024 * 1024 * 1024 };
			int i;

			for (i = 0; i < G_N_ELEMENTS (sizes); i++) {
				if ((i == G_N_ELEMENTS (sizes) - 1) || (limit_value < sizes[i + 1])) {
					value = g_strdup_printf ("%.2f", (double) limit_value / sizes[i]);
					gtk_entry_set_text (GTK_ENTRY (GET_WIDGET ("limit_size_entry")), value);
					g_free (value);

					gtk_combo_box_set_active (GTK_COMBO_BOX (self->priv->limit_object_combobox), i + 1);
					break;
				}
			}
		}
		else {
			value = g_strdup_printf ("%" G_GOFFSET_FORMAT, limit_value);
			gtk_entry_set_text (GTK_ENTRY (GET_WIDGET ("limit_size_entry")), value);
			g_free (value);

			gtk_combo_box_set_active (GTK_COMBO_BOX (self->priv->limit_object_combobox), 0);
		}
		if (get_iter_for_sort_type (self, sort_name, &iter))
			gtk_combo_box_set_active_iter (GTK_COMBO_BOX (self->priv->selection_combobox), &iter);
		gtk_combo_box_set_active (GTK_COMBO_BOX (self->priv->selection_order_combobox), sort_direction);
	}

	update_sensitivity (self);
}


GthFilter *
gth_filter_editor_dialog_get_filter (GthFilterEditorDialog  *self,
				     GError                **error)
{
	GthFilter *filter;

	filter = gth_filter_new ();
	if (self->priv->filter_id != NULL)
		g_object_set (filter, "id", self->priv->filter_id, NULL);
	g_object_set (filter,
		      "display-name", gtk_entry_get_text (GTK_ENTRY (GET_WIDGET ("name_entry"))),
		      "visible", self->priv->filter_visible,
		      NULL);

	if (g_strcmp0 (gth_test_get_display_name (GTH_TEST (filter)), "") == 0) {
		*error = g_error_new (GTH_TEST_ERROR, 0, _("No name specified"));
		g_object_unref (filter);
		return NULL;
	}

	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("match_checkbutton")))) {
		GthTest *chain;
		GList   *test_selectors;
		GList   *scan;

		chain = gth_test_chain_new (gtk_combo_box_get_active (GTK_COMBO_BOX (self->priv->match_type_combobox)) + 1, NULL);

		test_selectors = gtk_container_get_children (GTK_CONTAINER (GET_WIDGET ("tests_box")));
		for (scan = test_selectors; scan; scan = scan->next) {
			GthTestSelector *test_selector = GTH_TEST_SELECTOR (scan->data);
			GthTest         *test;

			test = gth_test_selector_get_test (test_selector, error);
			if (test == NULL) {
				g_object_unref (filter);
				return NULL;
			}

			gth_test_chain_add_test (GTH_TEST_CHAIN (chain), test);
			g_object_unref (test);
		}
		g_list_free (test_selectors);

		gth_filter_set_test (filter, GTH_TEST_CHAIN (chain));
	}
	else
		gth_filter_set_test (filter, NULL);

	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("limit_to_checkbutton")))) {
		int              size;
		int              idx;
		GthLimitType     limit_type;
		GtkTreeIter      iter;
		GthFileDataSort *sort_type;
		GtkSortType      sort_direction;

		size = atol (gtk_entry_get_text (GTK_ENTRY (GET_WIDGET ("limit_size_entry"))));
		idx = gtk_combo_box_get_active (GTK_COMBO_BOX (self->priv->limit_object_combobox));
		if (idx == 0)
			limit_type = GTH_LIMIT_TYPE_FILES;
		else {
			limit_type = GTH_LIMIT_TYPE_SIZE;
			if (idx == 1)
				size = size * 1024;
			else if (idx == 2)
				size = size * (1024 * 1024);
		}

		if (size == 0) {
			*error = g_error_new (GTH_TEST_ERROR, 0, _("No limit specified"));
			g_object_unref (filter);
			return NULL;
		}

		gtk_combo_box_get_active_iter (GTK_COMBO_BOX (self->priv->selection_combobox), &iter);
		gtk_tree_model_get (gtk_combo_box_get_model (GTK_COMBO_BOX (self->priv->selection_combobox)),
		                    &iter,
		                    SELECTION_COLUMN_DATA, &sort_type,
		                    -1);

		sort_direction = gtk_combo_box_get_active (GTK_COMBO_BOX (self->priv->selection_order_combobox));

		gth_filter_set_limit (filter, limit_type, size, sort_type->name, sort_direction);
	}

	return filter;
}
