/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2006-2008 Free Software Foundation, Inc.
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
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include "dom.h"
#include "glib-utils.h"
#include "gth-filterbar.h"
#include "gth-main.h"
#include "gth-user-dir.h"

enum {
	ITEM_TYPE_NONE,
	ITEM_TYPE_SEPARATOR,
	ITEM_TYPE_FILTER,
	ITEM_TYPE_PERSONALIZE
};

enum {
	ICON_COLUMN,
	NAME_COLUMN,
	TYPE_COLUMN,
	FILTER_COLUMN,
	N_COLUMNS
};

enum {
	CHANGED,
	PERSONALIZE,
	CLOSE_BUTTON_CLICKED,
	LAST_SIGNAL
};

struct _GthFilterbarPrivate {
	GtkListStore *model;
	GtkWidget    *test_combo_box;
	GthTest      *test;
	GtkWidget    *control_box;
	GtkWidget    *control;
	GtkWidget    *extra_area;
	gulong        test_changed_id;
};


static guint gth_filterbar_signals[LAST_SIGNAL] = { 0 };


G_DEFINE_TYPE_WITH_CODE (GthFilterbar,
			 gth_filterbar,
			 GTK_TYPE_BOX,
			 G_ADD_PRIVATE (GthFilterbar))


static void
gth_filterbar_finalize (GObject *object)
{
	GthFilterbar *filterbar;

	filterbar = GTH_FILTERBAR (object);

	if (filterbar->priv->test != NULL) {
		g_signal_handler_disconnect (filterbar->priv->test, filterbar->priv->test_changed_id);
		g_object_unref (filterbar->priv->test);
	}

	G_OBJECT_CLASS (gth_filterbar_parent_class)->finalize (object);
}


static void
gth_filterbar_class_init (GthFilterbarClass *class)
{
	GObjectClass   *object_class;

	object_class = (GObjectClass*) class;
	object_class->finalize = gth_filterbar_finalize;

	gth_filterbar_signals[CHANGED] =
		g_signal_new ("changed",
			      G_TYPE_FROM_CLASS (class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (GthFilterbarClass, changed),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__VOID,
			      G_TYPE_NONE,
			      0);
	gth_filterbar_signals[PERSONALIZE] =
		g_signal_new ("personalize",
			      G_TYPE_FROM_CLASS (class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (GthFilterbarClass, personalize),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__VOID,
			      G_TYPE_NONE,
			      0);
	 gth_filterbar_signals[CLOSE_BUTTON_CLICKED] =
		g_signal_new ("close_button_clicked",
			      G_TYPE_FROM_CLASS (class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (GthFilterbarClass, close_button_clicked),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__VOID,
			      G_TYPE_NONE,
			      0);
}


static void
gth_filterbar_init (GthFilterbar *filterbar)
{
	filterbar->priv = gth_filterbar_get_instance_private (filterbar);
	filterbar->priv->model = NULL;
	filterbar->priv->test_combo_box = NULL;
	filterbar->priv->test = NULL;
	filterbar->priv->control_box = NULL;
	filterbar->priv->control = NULL;
	filterbar->priv->extra_area = NULL;
	filterbar->priv->test_changed_id = 0;

	gtk_orientable_set_orientation (GTK_ORIENTABLE (filterbar), GTK_ORIENTATION_HORIZONTAL);
}


static void
gth_filterbar_changed (GthFilterbar *filterbar)
{
	g_signal_emit (filterbar, gth_filterbar_signals[CHANGED], 0);
}


static void
_gth_filterbar_set_test_control (GthFilterbar *filterbar,
				  GtkWidget    *control)
{
	if (filterbar->priv->control != NULL) {
		gtk_container_remove (GTK_CONTAINER (filterbar->priv->control_box),
				      filterbar->priv->control);
		filterbar->priv->control = NULL;
	}

	gth_filterbar_changed (filterbar);

	if (control == NULL)
		return;

	filterbar->priv->control = control;
	gtk_widget_show (control);
	gtk_container_add (GTK_CONTAINER (filterbar->priv->control_box),
			   filterbar->priv->control);
}


static void
test_changed_cb (GthTest      *test,
		 GthFilterbar *filterbar)
{
	gth_filterbar_changed (filterbar);
}


static gboolean
find_test_by_id (GthFilterbar  *filterbar,
		 const char    *id,
		 GthTest      **test,
		 GtkTreeIter   *iter)
{
	g_return_val_if_fail (iter != NULL, FALSE);

	if (! gtk_tree_model_get_iter_first (GTK_TREE_MODEL (filterbar->priv->model), iter))
		return FALSE;

	do {
		int      item_type = ITEM_TYPE_NONE;
		GthTest *local_test;

		gtk_tree_model_get (GTK_TREE_MODEL (filterbar->priv->model),
				    iter,
				    TYPE_COLUMN, &item_type,
				    FILTER_COLUMN, &local_test,
				    -1);

		if ((item_type == ITEM_TYPE_FILTER) && (local_test != NULL) && (g_strcmp0 (gth_test_get_id (local_test), id) == 0)) {
			if (test != NULL)
				*test = local_test;
			else
				g_object_unref (local_test);
			return TRUE;
		}

		_g_object_unref (local_test);
	}
	while (gtk_tree_model_iter_next (GTK_TREE_MODEL (filterbar->priv->model), iter));

	return FALSE;
}


static void
test_combo_box_changed_cb (GtkComboBox  *scope_combo_box,
			   GthFilterbar *filterbar);


static void
_gth_filterbar_set_test (GthFilterbar *filterbar,
			 GthTest      *test,
			 GtkTreeIter  *iter,
			 gboolean      emit_signal)
{
	GtkTreeIter  local_iter;
	GthTest     *old_test;

	if (iter != NULL)
		local_iter = *iter;
	else if (test == NULL)
		gtk_tree_model_get_iter_first (GTK_TREE_MODEL (filterbar->priv->model), &local_iter);
	else if (! find_test_by_id (filterbar,
				    gth_test_get_id (test),
				    NULL,
				    &local_iter))
		return;

	if (! emit_signal)
		g_signal_handlers_block_by_func (filterbar->priv->test_combo_box, test_combo_box_changed_cb, filterbar);
	gtk_combo_box_set_active_iter (GTK_COMBO_BOX (filterbar->priv->test_combo_box), &local_iter);
	if (! emit_signal)
		g_signal_handlers_unblock_by_func (filterbar->priv->test_combo_box, test_combo_box_changed_cb, filterbar);

	if (test == filterbar->priv->test)
		return;

	old_test = filterbar->priv->test;
	if (old_test != NULL) {
		if (filterbar->priv->test_changed_id != 0)
			g_signal_handler_disconnect (old_test, filterbar->priv->test_changed_id);
		filterbar->priv->test = NULL;
	}

	if (test != NULL) {
		filterbar->priv->test = g_object_ref (test);
		filterbar->priv->test_changed_id = g_signal_connect (test,
								     "changed",
								     G_CALLBACK (test_changed_cb),
								     filterbar);
		_gth_filterbar_set_test_control (filterbar, gth_test_create_control (filterbar->priv->test));
		gth_test_focus_control (filterbar->priv->test);
	}
	else
		_gth_filterbar_set_test_control (filterbar, NULL);

	_g_object_unref (old_test);
}


static void
test_combo_box_changed_cb (GtkComboBox  *scope_combo_box,
			   GthFilterbar *filterbar)
{
	GtkTreeIter  iter;
	int          item_type = ITEM_TYPE_NONE;
	GthTest     *test;

	if (! gtk_combo_box_get_active_iter (scope_combo_box, &iter))
		return;

	gtk_tree_model_get (GTK_TREE_MODEL (filterbar->priv->model),
			    &iter,
			    TYPE_COLUMN, &item_type,
			    FILTER_COLUMN, &test,
			    -1);

	switch (item_type) {
	case ITEM_TYPE_FILTER:
		_gth_filterbar_set_test (filterbar, test, &iter, TRUE);
		break;

	case ITEM_TYPE_PERSONALIZE:
		g_signal_emit (filterbar, gth_filterbar_signals[PERSONALIZE], 0);
		_gth_filterbar_set_test (filterbar, filterbar->priv->test, NULL, FALSE);
		break;

	default:
		break;
	}

	if (test != NULL)
		g_object_unref (test);
}


static gboolean
test_combo_box_row_separator_func (GtkTreeModel *model,
				   GtkTreeIter  *iter,
				   gpointer      data)
{
	int item_type = ITEM_TYPE_NONE;

	gtk_tree_model_get (model, iter, TYPE_COLUMN, &item_type, -1);

	return (item_type == ITEM_TYPE_SEPARATOR);
}


static void
gth_filterbar_construct (GthFilterbar *filterbar)
{
	GtkCellRenderer *renderer;
	GtkWidget       *label;

	gtk_box_set_spacing (GTK_BOX (filterbar), 6);
	gtk_container_set_border_width (GTK_CONTAINER (filterbar), 4);

	/* filter combo box */

	filterbar->priv->model = gtk_list_store_new (N_COLUMNS,
							     GDK_TYPE_PIXBUF,
							     G_TYPE_STRING,
						      G_TYPE_INT,
						      G_TYPE_OBJECT);
	filterbar->priv->test_combo_box = gtk_combo_box_new_with_model (GTK_TREE_MODEL (filterbar->priv->model));
	g_object_unref (filterbar->priv->model);
	gtk_combo_box_set_row_separator_func (GTK_COMBO_BOX (filterbar->priv->test_combo_box),
					      test_combo_box_row_separator_func,
					      filterbar,
					      NULL);

	/* icon renderer */

	renderer = gtk_cell_renderer_pixbuf_new ();
	gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (filterbar->priv->test_combo_box),
				    renderer,
				    FALSE);
	gtk_cell_layout_set_attributes  (GTK_CELL_LAYOUT (filterbar->priv->test_combo_box),
					 renderer,
					 "pixbuf", ICON_COLUMN,
					 NULL);

	/* name renderer */

	renderer = gtk_cell_renderer_text_new ();
	gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (filterbar->priv->test_combo_box),
				    renderer,
				    TRUE);
	gtk_cell_layout_set_attributes  (GTK_CELL_LAYOUT (filterbar->priv->test_combo_box),
					 renderer,
					 "text", NAME_COLUMN,
					 NULL);

	/**/

	g_signal_connect (G_OBJECT (filterbar->priv->test_combo_box),
			  "changed",
			  G_CALLBACK (test_combo_box_changed_cb),
			  filterbar);

	gtk_widget_show (filterbar->priv->test_combo_box);

	/* test control box */

	filterbar->priv->control_box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_widget_show (filterbar->priv->control_box);

	/* extra widgets container */

	filterbar->priv->extra_area = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_widget_show (filterbar->priv->extra_area);

	/* view label */

	label = gtk_label_new_with_mnemonic (_("S_how:"));
	gtk_label_set_mnemonic_widget (GTK_LABEL (label), filterbar->priv->test_combo_box);
	gtk_widget_show (label);

	/**/

	gtk_box_pack_start (GTK_BOX (filterbar), label, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (filterbar), filterbar->priv->test_combo_box, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (filterbar), filterbar->priv->control_box, FALSE, FALSE, 0);
	gtk_box_pack_end (GTK_BOX (filterbar), filterbar->priv->extra_area, FALSE, FALSE, 0);
}


GtkWidget*
gth_filterbar_new (void)
{
	GtkWidget *widget;

	widget = GTK_WIDGET (g_object_new (GTH_TYPE_FILTERBAR, NULL));
	gth_filterbar_construct (GTH_FILTERBAR (widget));

	return widget;
}


GthTest *
gth_filterbar_get_test (GthFilterbar *filterbar)
{
	if (filterbar->priv->test != NULL)
		return g_object_ref (filterbar->priv->test);
	else
		return NULL;
}


void
gth_filterbar_save_filter (GthFilterbar *filterbar,
			   const char   *filename)
{
	char  *filter_description;
	gsize  len;
	GFile *filter_file;

	if (filterbar->priv->test != NULL) {
		DomDocument *doc;

		doc = dom_document_new ();
		dom_element_append_child (DOM_ELEMENT (doc), dom_domizable_create_element (DOM_DOMIZABLE (filterbar->priv->test), doc));
		filter_description = dom_document_dump (doc, &len);

		g_object_unref (doc);
	}
	else {
		filter_description = g_strdup ("");
		len = 0;
	}
	filter_file = gth_user_dir_get_file_for_write (GTH_DIR_CONFIG, GTHUMB_DIR, filename, NULL);
	_g_file_write (filter_file, FALSE, 0, filter_description, len, NULL, NULL);

	g_object_unref (filter_file);
	g_free (filter_description);
}


gboolean
gth_filterbar_load_filter (GthFilterbar *filterbar,
			   const char   *filename)
{
	gboolean     loaded = FALSE;
	GFile       *filter_file;
	char        *buffer;
	gsize        len;
	DomDocument *doc;

	filter_file = gth_user_dir_get_file_for_write (GTH_DIR_CONFIG, GTHUMB_DIR, filename, NULL);
	if (! _g_file_load_in_buffer (filter_file, (void **) &buffer, &len, NULL, NULL)) {
		g_object_unref (filter_file);
		return loaded;
	}

	if (buffer == NULL) {
		g_object_unref (filter_file);
		return loaded;
	}

	doc = dom_document_new ();
	if (dom_document_load (doc, buffer, len, NULL)) {
		DomElement *node = DOM_ELEMENT (doc)->first_child;

		if (node != NULL) {
			GthTest     *test;
			GtkTreeIter  iter;

			if (find_test_by_id (filterbar,
					     dom_element_get_attribute (node, "id"),
					     &test,
					     &iter))
			{
				dom_domizable_load_from_element (DOM_DOMIZABLE (test), node);
				_gth_filterbar_set_test (GTH_FILTERBAR (filterbar), test, &iter, TRUE);
				loaded = TRUE;

				g_object_unref (test);
			}
		}
	}

	g_object_unref (doc);
	g_free (buffer);
	g_object_unref (filter_file);

	return loaded;
}


void
gth_filterbar_set_filter_list (GthFilterbar *filterbar,
			       GList        *filters /* GthTest list */)

{
	const char  *current_filter;
	gboolean     no_filter_selected = TRUE;
	GList       *scan;
	GtkTreeIter  iter;

	current_filter = NULL;
	if (filterbar->priv->test != NULL)
		current_filter = gth_test_get_id (filterbar->priv->test);

	gtk_list_store_clear (filterbar->priv->model);

	gtk_list_store_append (filterbar->priv->model, &iter);
	gtk_list_store_set (filterbar->priv->model, &iter,
			    TYPE_COLUMN, ITEM_TYPE_FILTER,
			    FILTER_COLUMN, NULL,
			    NAME_COLUMN, _("All"),
			    -1);

	for (scan = filters; scan; scan = scan->next) {
		GthTest *test = scan->data;

		if (! gth_test_is_visible (test))
			continue;

		gtk_list_store_append (filterbar->priv->model, &iter);
		gtk_list_store_set (filterbar->priv->model, &iter,
				    TYPE_COLUMN, ITEM_TYPE_FILTER,
				    FILTER_COLUMN, test,
				    NAME_COLUMN, gth_test_get_display_name (test),
				    -1);

		if (g_strcmp0 (current_filter, gth_test_get_id (test)) == 0) {
			_gth_filterbar_set_test (GTH_FILTERBAR (filterbar), test, &iter, FALSE);
			no_filter_selected = FALSE;
		}
	}

	gtk_list_store_append (filterbar->priv->model, &iter);
	gtk_list_store_set (filterbar->priv->model, &iter,
			    TYPE_COLUMN, ITEM_TYPE_SEPARATOR,
			    -1);

	gtk_list_store_append (filterbar->priv->model, &iter);
	gtk_list_store_set (filterbar->priv->model, &iter,
			    TYPE_COLUMN, ITEM_TYPE_PERSONALIZE,
			    NAME_COLUMN, _("Personalize…"),
			    -1);

	if (no_filter_selected) {
		gtk_tree_model_get_iter_first (GTK_TREE_MODEL (filterbar->priv->model), &iter);
		gtk_combo_box_set_active_iter (GTK_COMBO_BOX (filterbar->priv->test_combo_box), &iter);
	}
}


gboolean
gth_filterbar_set_test_by_id (GthFilterbar *filterbar,
			      const char   *id)
{
	GthTest     *test;
	GtkTreeIter  iter;

	if (! find_test_by_id (filterbar, id, &test, &iter))
		return FALSE;

	_gth_filterbar_set_test (GTH_FILTERBAR (filterbar), test, &iter, TRUE);

	_g_object_unref (test);

	return TRUE;
}


void
gth_filterbar_set_show_all (GthFilterbar *filterbar)
{
	GtkTreeIter iter;

	gtk_tree_model_get_iter_first (GTK_TREE_MODEL (filterbar->priv->model), &iter);
	gtk_combo_box_set_active_iter (GTK_COMBO_BOX (filterbar->priv->test_combo_box), &iter);
}


GtkWidget *
gth_filterbar_get_extra_area (GthFilterbar *filterbar)
{
	return filterbar->priv->extra_area;
}
