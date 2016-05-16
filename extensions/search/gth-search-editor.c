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


#define GET_WIDGET(name) _gtk_builder_get_widget (self->priv->builder, (name))


G_DEFINE_TYPE (GthSearchEditor, gth_search_editor, GTK_TYPE_BOX)


struct _GthSearchEditorPrivate {
	GtkBuilder *builder;
	GtkWidget  *location_chooser;
	GtkWidget  *match_type_combobox;
};


static void
gth_search_editor_finalize (GObject *object)
{
	GthSearchEditor *dialog;

	dialog = GTH_SEARCH_EDITOR (object);

	if (dialog->priv != NULL) {
		g_object_unref (dialog->priv->builder);
		g_free (dialog->priv);
		dialog->priv = NULL;
	}

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
	dialog->priv = g_new0 (GthSearchEditorPrivate, 1);
	gtk_orientable_set_orientation (GTK_ORIENTABLE (dialog), GTK_ORIENTATION_VERTICAL);
}


static void
update_sensitivity (GthSearchEditor *self)
{
	GList *test_selectors;
	int    more_selectors;
	GList *scan;

	test_selectors = gtk_container_get_children (GTK_CONTAINER (GET_WIDGET ("tests_box")));
	more_selectors = (test_selectors != NULL) && (test_selectors->next != NULL);
	for (scan = test_selectors; scan; scan = scan->next)
		gth_test_selector_can_remove (GTH_TEST_SELECTOR (scan->data), more_selectors);
	g_list_free (test_selectors);
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

	self->priv->location_chooser = g_object_new (GTH_TYPE_LOCATION_CHOOSER,
						     "show-entry-points", TRUE,
						     "relief", GTK_RELIEF_NORMAL,
						     NULL);
	gtk_widget_show (self->priv->location_chooser);
  	gtk_box_pack_start (GTK_BOX (GET_WIDGET ("location_box")), self->priv->location_chooser, TRUE, TRUE, 0);

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
	gtk_label_set_mnemonic_widget (GTK_LABEL (GET_WIDGET ("start_at_label")), self->priv->location_chooser);

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
	int pos;

	pos = _gtk_container_get_pos (GTK_CONTAINER (GET_WIDGET ("tests_box")), (GtkWidget*) selector);
	_gth_search_editor_add_test (self, pos == -1 ? -1 : pos + 1);
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


static void
_gth_search_editor_set_new_search (GthSearchEditor *self)
{
	GFile *home_location;

	home_location = g_file_new_for_uri (get_home_uri ());
	gth_location_chooser_set_current (GTH_LOCATION_CHOOSER (self->priv->location_chooser), home_location);
	g_object_unref (home_location);

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("include_subfolders_checkbutton")), TRUE);
	_gtk_container_remove_children (GTK_CONTAINER (GET_WIDGET ("tests_box")), NULL, NULL);
}


void
gth_search_editor_set_search (GthSearchEditor *self,
			      GthSearch       *search)
{
	GthTestChain *test;
	GthMatchType  match_type;

	_gth_search_editor_set_new_search (self);

	if (search == NULL) {
		_gth_search_editor_add_test (self, -1);
		update_sensitivity (self);
		return;
	}

	gth_location_chooser_set_current (GTH_LOCATION_CHOOSER (self->priv->location_chooser), gth_search_get_folder (search));
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("include_subfolders_checkbutton")), gth_search_is_recursive (search));

	test = gth_search_get_test (search);
	match_type = (test != NULL) ? gth_test_chain_get_match_type (test) : GTH_MATCH_TYPE_NONE;
	_gtk_container_remove_children (GTK_CONTAINER (GET_WIDGET ("tests_box")), NULL, NULL);
	if (match_type != GTH_MATCH_TYPE_NONE) {
		GList *tests;
		GList *scan;

		tests = gth_test_chain_get_tests (test);
		for (scan = tests; scan; scan = scan->next) {
			GthTest   *test = scan->data;
			GtkWidget *test_selector;

			test_selector = _gth_search_editor_add_test (self, -1);
			gth_test_selector_set_test (GTH_TEST_SELECTOR (test_selector), test);
		}
		_g_object_list_unref (tests);
	}
	else
		_gth_search_editor_add_test (self, -1);

	gtk_combo_box_set_active (GTK_COMBO_BOX (self->priv->match_type_combobox), (match_type == GTH_MATCH_TYPE_ANY) ? 1 : 0);

	update_sensitivity (self);
}


GthSearch *
gth_search_editor_get_search (GthSearchEditor  *self,
			      GError          **error)
{
	GthSearch *search;
	GFile     *folder;
	GthTest   *test;
	GList     *test_selectors;
	GList     *scan;

	search = gth_search_new ();

	folder = gth_location_chooser_get_current (GTH_LOCATION_CHOOSER (self->priv->location_chooser));
	if (folder != NULL)
		gth_search_set_folder (search, folder);

	gth_search_set_recursive (search, gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("include_subfolders_checkbutton"))));

	test = gth_test_chain_new (gtk_combo_box_get_active (GTK_COMBO_BOX (self->priv->match_type_combobox)) + 1, NULL);
	test_selectors = gtk_container_get_children (GTK_CONTAINER (GET_WIDGET ("tests_box")));
	for (scan = test_selectors; scan; scan = scan->next) {
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
	g_list_free (test_selectors);
	gth_search_set_test (search, GTH_TEST_CHAIN (test));

	return search;
}
