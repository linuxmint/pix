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
#include <glib/gi18n.h>
#include "glib-utils.h"
#include "gth-duplicable.h"
#include "gth-main.h"
#include "gth-test-selector.h"


enum {
	NAME_COLUMN,
	TEST_COLUMN,
	N_COLUMNS
};


enum {
	ADD_TEST,
	REMOVE_TEST,
	LAST_SIGNAL
};


struct _GthTestSelectorPrivate {
	GthTest      *test;
	GtkListStore *model;
	GtkWidget    *test_combo_box;
	GtkWidget    *control_box;
	GtkWidget    *control;
	GtkWidget    *add_button;
	GtkWidget    *remove_button;
};


static guint gth_test_selector_signals[LAST_SIGNAL] = { 0 };


G_DEFINE_TYPE_WITH_CODE (GthTestSelector,
			 gth_test_selector,
			 GTK_TYPE_BOX,
			 G_ADD_PRIVATE (GthTestSelector))


static void
gth_test_selector_finalize (GObject *object)
{
	GthTestSelector *selector;

	selector = GTH_TEST_SELECTOR (object);

	if (selector->priv->test != NULL)
		g_object_unref (selector->priv->test);

	G_OBJECT_CLASS (gth_test_selector_parent_class)->finalize (object);
}


static void
gth_test_selector_class_init (GthTestSelectorClass *klass)
{
	G_OBJECT_CLASS (klass)->finalize = gth_test_selector_finalize;

	/* signals */

	gth_test_selector_signals[ADD_TEST] =
		g_signal_new ("add-test",
			      G_TYPE_FROM_CLASS (klass),
 			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (GthTestSelectorClass, add_test),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__VOID,
			      G_TYPE_NONE,
			      0);
	gth_test_selector_signals[REMOVE_TEST] =
		g_signal_new ("remove-test",
			      G_TYPE_FROM_CLASS (klass),
 			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (GthTestSelectorClass, remove_test),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__VOID,
			      G_TYPE_NONE,
			      0);
}


static void
gth_test_selector_init (GthTestSelector *self)
{
	self->priv = gth_test_selector_get_instance_private (self);
	self->priv->test = NULL;
	self->priv->model = NULL;
	self->priv->test_combo_box = NULL;
	self->priv->control_box = NULL;
	self->priv->control = NULL;
	self->priv->add_button = NULL;
	self->priv->remove_button = NULL;
	gtk_orientable_set_orientation (GTK_ORIENTABLE (self), GTK_ORIENTATION_HORIZONTAL);
}


static void
test_combo_box_changed_cb (GtkComboBox     *scope_combo_box,
                           GthTestSelector *self)
{
	GtkTreeIter  iter;
	const char  *test_name;

	if (! gtk_combo_box_get_active_iter (scope_combo_box, &iter))
		return;

	gtk_tree_model_get (GTK_TREE_MODEL (self->priv->model),
			    &iter,
			    TEST_COLUMN, &test_name,
			    -1);

	if (test_name != NULL) {
		GthTest *test;

		test = gth_main_get_registered_object (GTH_TYPE_TEST, test_name);
		gth_test_selector_set_test (self, test);
		gth_test_selector_focus (self);

		g_object_unref (test);
	}
}


static void
add_button_clicked_cb (GtkButton       *button,
		       GthTestSelector *self)
{
	g_signal_emit (self, gth_test_selector_signals[ADD_TEST], 0);
}


static void
remove_button_clicked_cb (GtkButton       *button,
			  GthTestSelector *self)
{
	g_signal_emit (self, gth_test_selector_signals[REMOVE_TEST], 0);
}


static int
compare_test_by_display_name (gconstpointer a,
                              gconstpointer b)
{
	GthTest *test_a = (GthTest *) a;
	GthTest *test_b = (GthTest *) b;

	return g_utf8_collate (gth_test_get_display_name (test_a), gth_test_get_display_name (test_b));
}


static GList *
get_all_tests (void)
{
	GList *test_ids;
	GList *scan;
	GList *tests = NULL;

	test_ids = gth_main_get_registered_objects_id (GTH_TYPE_TEST);
	for (scan = test_ids; scan; scan = scan->next) {
		char *test_name = scan->data;
		tests = g_list_prepend (tests, gth_main_get_registered_object (GTH_TYPE_TEST, test_name));
	}

	tests = g_list_sort (tests, compare_test_by_display_name);

	_g_string_list_free (test_ids);

	return tests;
}


static void
gth_test_selector_construct (GthTestSelector *self,
			     GthTest         *test)
{
	GtkCellRenderer *renderer;
	GtkTreeIter      iter;
	GtkWidget       *vbox;
	GtkWidget       *hbox;
	GList           *tests;
	GList           *scan;

	gtk_box_set_spacing (GTK_BOX (self), 6);
	gtk_container_set_border_width (GTK_CONTAINER (self), 0);

	/* scope combo box */

	self->priv->model = gtk_list_store_new (N_COLUMNS,
					        G_TYPE_STRING,
					        G_TYPE_STRING);
	self->priv->test_combo_box = gtk_combo_box_new_with_model (GTK_TREE_MODEL (self->priv->model));
	g_object_unref (self->priv->model);

	/* name renderer */

	renderer = gtk_cell_renderer_text_new ();
	gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (self->priv->test_combo_box),
				    renderer,
				    TRUE);
	gtk_cell_layout_set_attributes  (GTK_CELL_LAYOUT (self->priv->test_combo_box),
					 renderer,
					 "text", NAME_COLUMN,
					 NULL);

	/**/

	tests = get_all_tests ();
	for (scan = tests; scan; scan = scan->next) {
		GthTest *test = scan->data;

		gtk_list_store_append (self->priv->model, &iter);
		gtk_list_store_set (self->priv->model, &iter,
				    TEST_COLUMN, gth_test_get_id (test),
				    NAME_COLUMN, gth_test_get_display_name (test),
			    	    -1);
	}
	_g_object_list_unref (tests);

	g_signal_connect (G_OBJECT (self->priv->test_combo_box),
			  "changed",
			  G_CALLBACK (test_combo_box_changed_cb),
			  self);

	gtk_widget_show (self->priv->test_combo_box);

	/* test control box */

	self->priv->control_box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_widget_show (self->priv->control_box);

	/**/

	self->priv->add_button = gtk_button_new_from_icon_name ("list-add-symbolic", GTK_ICON_SIZE_BUTTON);
	gtk_button_set_relief (GTK_BUTTON (self->priv->add_button), GTK_RELIEF_NONE);
	gtk_widget_set_tooltip_text (self->priv->add_button, _("Add a new rule"));
	gtk_widget_show_all (self->priv->add_button);

	g_signal_connect (G_OBJECT (self->priv->add_button),
			  "clicked",
			  G_CALLBACK (add_button_clicked_cb),
			  self);

	self->priv->remove_button = gtk_button_new_from_icon_name ("list-remove-symbolic", GTK_ICON_SIZE_BUTTON);
	gtk_button_set_relief (GTK_BUTTON (self->priv->remove_button), GTK_RELIEF_NONE);
	gtk_widget_set_tooltip_text (self->priv->remove_button, _("Remove this rule"));
	gtk_widget_show_all (self->priv->remove_button);

	g_signal_connect (G_OBJECT (self->priv->remove_button),
			  "clicked",
			  G_CALLBACK (remove_button_clicked_cb),
			  self);

	/**/

	vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
	gtk_widget_show (vbox);

	hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
	gtk_widget_show (hbox);

	gtk_box_pack_start (GTK_BOX (hbox), self->priv->test_combo_box, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (hbox), self->priv->control_box, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (self), vbox, FALSE, FALSE, 0);

	gtk_box_pack_end (GTK_BOX (self), self->priv->add_button, FALSE, FALSE, 0);
	gtk_box_pack_end (GTK_BOX (self), self->priv->remove_button, FALSE, FALSE, 0);

	gth_test_selector_set_test (self, test);
}


GtkWidget *
gth_test_selector_new (void)
{
	GthTestSelector *self;

	self = g_object_new (GTH_TYPE_TEST_SELECTOR, NULL);
	gth_test_selector_construct (self, NULL);

	return (GtkWidget *) self;
}


GtkWidget *
gth_test_selector_new_with_test (GthTest *test)
{
	GthTestSelector *self;

	self = g_object_new (GTH_TYPE_TEST_SELECTOR, NULL);
	gth_test_selector_construct (self, test);

	return (GtkWidget *) self;
}


static int
get_test_index (GthTestSelector *self,
		GthTest         *test)
{
	GtkTreeModel *model = GTK_TREE_MODEL (self->priv->model);
	GtkTreeIter   iter;
	const char   *test_id;
	int           i;
	int           idx;

	if (! gtk_tree_model_get_iter_first (model, &iter))
		return 0;

	g_object_get (test, "id", &test_id, NULL);
	i = idx = -1;
	do {
		char *id;

		i++;
		gtk_tree_model_get (model, &iter, TEST_COLUMN, &id, -1);
		if (g_strcmp0 (test_id, id) == 0)
			idx = i;
		g_free (id);
	} while ((idx == -1) && gtk_tree_model_iter_next (model, &iter));

	return (idx == -1) ? 0 : idx;
}


void
gth_test_selector_set_test (GthTestSelector *self,
			    GthTest         *test)
{
	GthTest   *old_test;
	GtkWidget *control;

	old_test = self->priv->test;
	if (test != NULL)
		self->priv->test = (GthTest *) gth_duplicable_duplicate (GTH_DUPLICABLE (test));
	else
		self->priv->test = gth_main_get_registered_object (GTH_TYPE_TEST, "file::name");

	/* update the active test */

	g_signal_handlers_block_by_func (self->priv->test_combo_box, test_combo_box_changed_cb, self);
	gtk_combo_box_set_active (GTK_COMBO_BOX (self->priv->test_combo_box), get_test_index (self, self->priv->test));
	g_signal_handlers_unblock_by_func (self->priv->test_combo_box, test_combo_box_changed_cb, self);

	/* set the test control */

	if (self->priv->test != NULL)
		control = gth_test_create_control (self->priv->test);
	else
		control = NULL;

	if (self->priv->control != NULL) {
		gtk_container_remove (GTK_CONTAINER (self->priv->control_box),
				      self->priv->control);
		self->priv->control = NULL;
	}

	if (control != NULL) {
		self->priv->control = control;
		gtk_widget_show (control);
		gtk_container_add (GTK_CONTAINER (self->priv->control_box),
				   self->priv->control);
	}

	_g_object_unref (old_test);
}


GthTest *
gth_test_selector_get_test (GthTestSelector  *self,
			    GError          **error)
{
	if (! gth_test_update_from_control (self->priv->test, error))
		return NULL;
	else
		return g_object_ref (self->priv->test);
}


void
gth_test_selector_can_remove (GthTestSelector *self,
			      gboolean         value)
{
	gtk_widget_set_sensitive (self->priv->remove_button, value);
}


void
gth_test_selector_focus (GthTestSelector *self)
{
	if (self->priv->test != NULL)
		gth_test_focus_control (self->priv->test);
}
