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
#include <stdlib.h>
#include <gtk/gtk.h>
#include <gthumb.h>
#include "gth-search-editor.h"
#include "gth-search-source-selector.h"


#define GET_WIDGET(name) _gtk_builder_get_widget (self->priv->builder, (name))


struct _GthSearchEditorPrivate {
	GtkBuilder *builder;
	GtkWidget  *match_type_combobox;
};


G_DEFINE_TYPE_WITH_CODE (GthSearchEditor,
			 gth_search_editor,
			 GTK_TYPE_BOX,
			 G_ADD_PRIVATE (GthSearchEditor))


static void
gth_search_editor_finalize (GObject *object)
{
	GthSearchEditor *dialog;

	dialog = GTH_SEARCH_EDITOR (object);

	_g_object_unref (dialog->priv->builder);

	G_OBJECT_CLASS (gth_search_editor_parent_class)->finalize (object);
}

static void
gth_search_editor_class_init (GthSearchEditorClass *class)
{
	GObjectClass *object_class;

	object_class = (GObjectClass*) class;
	object_class->finalize = gth_search_editor_finalize;
}


static void
gth_search_editor_init (GthSearchEditor *dialog)
{
	dialog->priv = gth_search_editor_get_instance_private (dialog);
	dialog->priv->builder = NULL;
	dialog->priv->match_type_combobox = NULL;
	gtk_orientable_set_orientation (GTK_ORIENTABLE (dialog), GTK_ORIENTATION_VERTICAL);
}


static void
update_sensitivity (GthSearchEditor *self)
{
	GList *selectors;
	int    more_selectors;
	GList *scan;

	/* sources */

	selectors = gtk_container_get_children (GTK_CONTAINER (GET_WIDGET ("sources_box")));
	more_selectors = (selectors != NULL) && (selectors->next != NULL);
	for (scan = selectors; scan; scan = scan->next)
		gth_search_source_selector_can_remove (GTH_SEARCH_SOURCE_SELECTOR (scan->data), more_selectors);
	g_list_free (selectors);

	/* tests */

	selectors = gtk_container_get_children (GTK_CONTAINER (GET_WIDGET ("tests_box")));
	more_selectors = (selectors != NULL) && (selectors->next != NULL);
	for (scan = selectors; scan; scan = scan->next)
		gth_test_selector_can_remove (GTH_TEST_SELECTOR (scan->data), more_selectors);
	g_list_free (selectors);

}


static void
gth_search_editor_construct (GthSearchEditor *self,
			     GthSearch       *search)
{
	GtkWidget *content;

    	self->priv->builder = _gtk_builder_new_from_file ("search-editor.ui", "search");

    	content = _gtk_builder_get_widget (self->priv->builder, "search_editor");
    	gtk_container_set_border_width (GTK_CONTAINER (content), 0);
  	gtk_box_pack_start (GTK_BOX (self), content, TRUE, TRUE, 0);

	self->priv->match_type_combobox = gtk_combo_box_text_new ();
  	_gtk_combo_box_append_texts (GTK_COMBO_BOX_TEXT (self->priv->match_type_combobox),
  				     _("all the following rules"),
  				     _("any of the following rules"),
  				     NULL);
	gtk_combo_box_set_active (GTK_COMBO_BOX (self->priv->match_type_combobox), 0);
  	gtk_widget_show (self->priv->match_type_combobox);
  	gtk_container_add (GTK_CONTAINER (GET_WIDGET ("match_type_combobox_box")),
  			   self->priv->match_type_combobox);

	gtk_label_set_use_underline (GTK_LABEL (GET_WIDGET ("match_label")), TRUE);
	gtk_label_set_mnemonic_widget (GTK_LABEL (GET_WIDGET ("match_label")), self->priv->match_type_combobox);

  	gth_search_editor_set_search (self, search);
}


GtkWidget *
gth_search_editor_new (GthSearch *search)
{
	GthSearchEditor *self;

	self = g_object_new (GTH_TYPE_SEARCH_EDITOR, NULL);
	gth_search_editor_construct (self, search);

	return (GtkWidget *) self;
}


static GtkWidget *
_gth_search_editor_add_test (GthSearchEditor *self,
			     int              pos);


static void
test_selector_add_test_cb (GthTestSelector *selector,
			   GthSearchEditor *self)
{
	int        pos;
	GtkWidget *new_selector;

	pos = _gtk_container_get_pos (GTK_CONTAINER (GET_WIDGET ("tests_box")), (GtkWidget*) selector);
	new_selector = _gth_search_editor_add_test (self, pos == -1 ? -1 : pos + 1);
	gth_test_selector_focus (GTH_TEST_SELECTOR (new_selector));

	update_sensitivity (self);
}


static void
test_selector_remove_test_cb (GthTestSelector *selector,
			      GthSearchEditor *self)
{
	gtk_container_remove (GTK_CONTAINER (GET_WIDGET ("tests_box")), (GtkWidget*) selector);
	update_sensitivity (self);
}


static GtkWidget *
_gth_search_editor_add_test (GthSearchEditor *self,
			     int              pos)
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


static GtkWidget *
_gth_search_editor_add_source (GthSearchEditor *self,
			       int              pos);


static void
test_selector_add_source_cb (GthTestSelector *selector,
			     GthSearchEditor *self)
{
	int pos;

	pos = _gtk_container_get_pos (GTK_CONTAINER (GET_WIDGET ("sources_box")), (GtkWidget*) selector);
	_gth_search_editor_add_source (self, pos == -1 ? -1 : pos + 1);
	update_sensitivity (self);
}


static void
test_selector_remove_source_cb (GthTestSelector *selector,
				GthSearchEditor *self)
{
	gtk_container_remove (GTK_CONTAINER (GET_WIDGET ("sources_box")), (GtkWidget*) selector);
	update_sensitivity (self);
}


static GtkWidget *
_gth_search_editor_add_source (GthSearchEditor *self,
			       int              pos)
{
	GthSearchSource *source;
	GtkWindow       *window;
	GtkWidget       *source_selector;

	source = NULL;
	window = _gtk_widget_get_toplevel_if_window (GTK_WIDGET (self));
	if (window != NULL)
		window = gtk_window_get_transient_for (window);
	if ((window != NULL) && GTH_IS_BROWSER (window)) {
		source = gth_search_source_new ();
		gth_search_source_set_folder (source, gth_browser_get_location (GTH_BROWSER (window)));
		gth_search_source_set_recursive (source, TRUE);
	}
	source_selector = gth_search_source_selector_new_with_source (source);
	gtk_widget_show (source_selector);

	g_signal_connect (G_OBJECT (source_selector),
			  "add_source",
			  G_CALLBACK (test_selector_add_source_cb),
			  self);
	g_signal_connect (G_OBJECT (source_selector),
			  "remove_source",
			  G_CALLBACK (test_selector_remove_source_cb),
			  self);

	gtk_box_pack_start (GTK_BOX (GET_WIDGET ("sources_box")), source_selector, FALSE, FALSE, 0);

	if (pos >= 0)
		gtk_box_reorder_child (GTK_BOX (GET_WIDGET ("sources_box")),
				       source_selector,
				       pos);

	_g_object_unref (source);

	return source_selector;
}


void
gth_search_editor_set_search (GthSearchEditor *self,
			      GthSearch       *search)
{
	int           n_sources;
	int           n_tests;
	GthMatchType  match_type = GTH_MATCH_TYPE_NONE;
	GList        *scan;

	/* sources */

	_gtk_container_remove_children (GTK_CONTAINER (GET_WIDGET ("sources_box")), NULL, NULL);
	n_sources = 0;

	if (search != NULL) {
		for (scan = gth_search_get_sources (search); scan; scan = scan->next) {
			GthSearchSource *source = scan->data;
			GtkWidget       *source_selector;

			source_selector = _gth_search_editor_add_source (self, -1);
			gth_search_source_selector_set_source (GTH_SEARCH_SOURCE_SELECTOR (source_selector), source);
			n_sources += 1;
		}
	}

	/* tests */

	_gtk_container_remove_children (GTK_CONTAINER (GET_WIDGET ("tests_box")), NULL, NULL);
	n_tests = 0;

	if (search != NULL) {
		GthTestChain *test;

		test = gth_search_get_test (search);
		if (test != NULL)
			match_type = gth_test_chain_get_match_type (test);

		if (match_type != GTH_MATCH_TYPE_NONE) {
			GList *tests;
			GList *scan;

			tests = gth_test_chain_get_tests (test);
			for (scan = tests; scan; scan = scan->next) {
				GthTest   *test = scan->data;
				GtkWidget *test_selector;

				test_selector = _gth_search_editor_add_test (self, -1);
				gth_test_selector_set_test (GTH_TEST_SELECTOR (test_selector), test);
				n_tests += 1;
			}
			_g_object_list_unref (tests);
		}
	}

	if (n_sources == 0)
		_gth_search_editor_add_source (self, -1);
	if (n_tests == 0)
		_gth_search_editor_add_test (self, -1);
	gtk_combo_box_set_active (GTK_COMBO_BOX (self->priv->match_type_combobox), (match_type == GTH_MATCH_TYPE_ANY) ? 1 : 0);

	update_sensitivity (self);
}


GthSearch *
gth_search_editor_get_search (GthSearchEditor  *self,
			      GError          **error)
{
	GthSearch *search;
	GList     *sources;
	GthTest   *test;
	GList     *selectors;
	GList     *scan;

	search = gth_search_new ();

	/* sources */

	sources = NULL;
	selectors = gtk_container_get_children (GTK_CONTAINER (GET_WIDGET ("sources_box")));
	for (scan = selectors; scan; scan = scan->next) {
		GthSearchSourceSelector *selector = GTH_SEARCH_SOURCE_SELECTOR (scan->data);
		sources = g_list_prepend (sources, gth_search_source_selector_get_source (selector));
	}
	g_list_free (selectors);
	sources = g_list_reverse (sources);
	gth_search_set_sources (search, sources);
	_g_object_list_unref (sources);

	/* tests */

	test = gth_test_chain_new (gtk_combo_box_get_active (GTK_COMBO_BOX (self->priv->match_type_combobox)) + 1, NULL);
	selectors = gtk_container_get_children (GTK_CONTAINER (GET_WIDGET ("tests_box")));
	for (scan = selectors; scan; scan = scan->next) {
		GthTestSelector *test_selector = GTH_TEST_SELECTOR (scan->data);
		GthTest         *sub_test;

		sub_test = gth_test_selector_get_test (test_selector, error);
		if (sub_test == NULL) {
			g_object_unref (search);
			return NULL;
		}

		gth_test_chain_add_test (GTH_TEST_CHAIN (test), sub_test);
		g_object_unref (sub_test);
	}
	g_list_free (selectors);
	gth_search_set_test (search, GTH_TEST_CHAIN (test));
	g_object_unref (test);

	return search;
}


void
gth_search_editor_focus_first_rule (GthSearchEditor *self)
{
	GList *test_selectors;

	test_selectors = gtk_container_get_children (GTK_CONTAINER (GET_WIDGET ("tests_box")));
	if (test_selectors == NULL)
		return;

	gth_test_selector_focus (GTH_TEST_SELECTOR (test_selectors->data));
}
