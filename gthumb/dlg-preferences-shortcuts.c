/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2019 The Free Software Foundation, Inc.
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
#include "dlg-preferences-shortcuts.h"
#include "glib-utils.h"
#include "gth-accel-dialog.h"
#include "gth-browser.h"
#include "gth-main.h"
#include "gth-preferences.h"
#include "gtk-utils.h"
#include "main.h"


#define GET_WIDGET(name) _gtk_builder_get_widget (data->builder, (name))
#define BROWSER_DATA_KEY "shortcuts-preference-data"
#define GTH_SHORTCUT_CATEGORY_ALL "all"
#define GTH_SHORTCUT_CATEGORY_MODIFIED "modified"


enum {
	ACTION_NAME_COLUMN,
	DESCRIPTION_COLUMN,
	ACCELERATOR_LABEL_COLUMN
};


typedef struct {
	GthBrowser *browser;
	GtkBuilder *builder;
	GtkWidget  *preferences_dialog;
	GPtrArray  *rows;
	char       *show_category;
} BrowserData;


static void
browser_data_free (BrowserData *data)
{
	g_free (data->show_category);
	g_ptr_array_unref (data->rows);
	g_object_unref (data->builder);
	g_free (data);
}


typedef struct {
	BrowserData *browser_data;
	GthShortcut *shortcut;
	GtkWidget   *accel_label;
	GtkWidget   *revert_button;
	gboolean     modified;
} RowData;


static RowData *
row_data_new (BrowserData *data,
	      GthShortcut *shortcut)
{
	RowData *row_data;

	row_data = g_new0 (RowData, 1);
	row_data->browser_data = data;
	row_data->shortcut = shortcut;
	row_data->accel_label = NULL;

	g_ptr_array_add (row_data->browser_data->rows, row_data);

	return row_data;
}


static void
row_data_free (RowData *row_data)
{
	if (row_data == NULL)
		return;
	g_free (row_data);
}


static void
row_data_update_accel_label (RowData *row_data)
{
	if (row_data == NULL)
		return;

	row_data->modified = g_strcmp0 (row_data->shortcut->default_accelerator, row_data->shortcut->accelerator) != 0;
	if (row_data->modified) {
		char *esc_text;
		char *markup_text;

		esc_text = g_markup_escape_text (row_data->shortcut->label, -1);
		markup_text = g_strdup_printf ("<b>%s</b>", esc_text);
		gtk_label_set_markup (GTK_LABEL (row_data->accel_label), markup_text);

		g_free (markup_text);
		g_free (esc_text);
	}
	else
		gtk_label_set_text (GTK_LABEL (row_data->accel_label), row_data->shortcut->label);

	gtk_widget_set_sensitive (row_data->revert_button, row_data->modified);
	gtk_widget_set_child_visible (row_data->revert_button, row_data->modified);
}


static RowData *
find_row_by_shortcut (BrowserData *browser_data,
		      GthShortcut *shortcut)
{
	int i;

	for (i = 0; i < browser_data->rows->len; i++) {
		RowData *row_data = g_ptr_array_index (browser_data->rows, i);
		if (g_strcmp0 (row_data->shortcut->detailed_action, shortcut->detailed_action) == 0)
			return row_data;
	}

	return NULL;
}


static void
update_filter_if_required (BrowserData *data)
{
	if (g_strcmp0 (data->show_category, GTH_SHORTCUT_CATEGORY_MODIFIED) == 0)
		gtk_list_box_invalidate_filter (GTK_LIST_BOX (_gtk_builder_get_widget (data->builder, "shortcuts_list")));
}


static gboolean
row_data_update_shortcut (RowData         *row_data,
			  guint            keycode,
			  GdkModifierType  modifiers,
			  GtkWindow       *parent)
{
	gboolean change;

	change = gth_window_can_change_shortcut (GTH_WINDOW (row_data->browser_data->browser),
						 row_data->shortcut->detailed_action,
						 row_data->shortcut->context,
						 row_data->shortcut->viewer_context,
						 keycode,
						 modifiers,
						 parent);

	if (change) {
		GPtrArray   *shortcuts_v;
		GthShortcut *shortcut;

		shortcuts_v = gth_window_get_shortcuts (GTH_WINDOW (row_data->browser_data->browser));
		shortcut = gth_shortcut_array_find (shortcuts_v,
						    row_data->shortcut->context,
						    row_data->shortcut->viewer_context,
						    keycode,
						    modifiers);
		if (shortcut != NULL) {
			gth_shortcut_set_key (shortcut, 0, 0);
			row_data_update_accel_label (find_row_by_shortcut (row_data->browser_data, shortcut));
		}

		gth_shortcut_set_key (row_data->shortcut, keycode, modifiers);
		row_data_update_accel_label (row_data);
		update_filter_if_required (row_data->browser_data);

		gth_main_shortcuts_changed (gth_window_get_shortcuts (GTH_WINDOW (row_data->browser_data->browser)));
	}

	return change;
}


static void
accel_dialog_response_cb (GtkDialog *dialog,
			  gint       response_id,
			  gpointer   user_data)
{
	RowData         *row_data = user_data;
	guint            keycode;
	GdkModifierType  modifiers;

	switch (response_id) {
	case GTK_RESPONSE_OK:
		if (gth_accel_dialog_get_accel (GTH_ACCEL_DIALOG (dialog), &keycode, &modifiers))
			if (row_data_update_shortcut (row_data, keycode, modifiers, GTK_WINDOW (dialog)))
				gtk_widget_destroy (GTK_WIDGET (dialog));
		break;

	case GTK_RESPONSE_CANCEL:
		gtk_widget_destroy (GTK_WIDGET (dialog));
		break;

	case GTH_ACCEL_BUTTON_RESPONSE_DELETE:
		row_data_update_shortcut (row_data, 0, 0, GTK_WINDOW (dialog));
		gtk_widget_destroy (GTK_WIDGET (dialog));
		break;
	}
}


static void
shortcuts_list_row_activated_cb (GtkListBox    *box,
				 GtkListBoxRow *row,
				 gpointer       user_data)
{
	RowData   *row_data;
	GtkWidget *dialog;

	row_data = g_object_get_data (G_OBJECT (row), "shortcut-row-data");

	dialog = gth_accel_dialog_new (_("Shortcut"),
				       _gtk_widget_get_toplevel_if_window (GTK_WIDGET (box)),
				       row_data->shortcut->keyval,
				       row_data->shortcut->modifiers);
	g_signal_connect (dialog,
			  "response",
			  G_CALLBACK (accel_dialog_response_cb),
			  row_data);
	gtk_widget_show (dialog);
}


static void
revert_button_clicked_cb (GtkButton *button,
			  gpointer   user_data)
{
	RowData         *row_data = user_data;
	guint            keycode;
	GdkModifierType  modifiers;

	gtk_accelerator_parse (row_data->shortcut->default_accelerator, &keycode, &modifiers);
	row_data_update_shortcut (row_data, keycode, modifiers, GTK_WINDOW (row_data->browser_data->preferences_dialog));
}


static GtkWidget *
_new_shortcut_row (GthShortcut *shortcut,
		   BrowserData *data)
{
	GtkWidget *row;
	RowData   *row_data;
	GtkWidget *box;
	GtkWidget *label;
	GtkWidget *button_box;
	GtkWidget *button;

	row = gtk_list_box_row_new ();
	row_data = row_data_new (data, shortcut);
	g_object_set_data_full (G_OBJECT (row), "shortcut-row-data", row_data, (GDestroyNotify) row_data_free);

	box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_container_set_border_width (GTK_CONTAINER (box), 5);
	gtk_container_add (GTK_CONTAINER (row), box);

	label = gtk_label_new (_(shortcut->description));
	gtk_label_set_xalign (GTK_LABEL (label), 0.0);
	gtk_widget_set_margin_end (label, 12);
	gtk_size_group_add_widget (GTK_SIZE_GROUP (gtk_builder_get_object (data->builder, "column1_size_group")), label);
	gtk_box_pack_start (GTK_BOX (box), label, TRUE, TRUE, 0);

	row_data->accel_label = label = gtk_label_new ("");
	gtk_label_set_xalign (GTK_LABEL (label), 0.0);
	gtk_widget_set_margin_end (label, 12);
	gtk_style_context_add_class (gtk_widget_get_style_context (label), GTK_STYLE_CLASS_DIM_LABEL);
	gtk_size_group_add_widget (GTK_SIZE_GROUP (gtk_builder_get_object (data->builder, "column2_size_group")), label);
	gtk_box_pack_start (GTK_BOX (box), label, FALSE, FALSE, 0);

	button_box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_widget_set_margin_start (button_box, 12);
	gtk_widget_set_margin_end (button_box, 12);
	gtk_size_group_add_widget (GTK_SIZE_GROUP (gtk_builder_get_object (data->builder, "column3_size_group")), button_box);
	gtk_box_pack_start (GTK_BOX (box), button_box, FALSE, FALSE, 0);

	row_data->revert_button = button = gtk_button_new_from_icon_name ("edit-clear-symbolic", GTK_ICON_SIZE_MENU);
	gtk_button_set_relief (GTK_BUTTON (button), GTK_RELIEF_NONE);
	gtk_widget_set_tooltip_text (button, _("Revert"));
	gtk_style_context_add_class (gtk_widget_get_style_context (button), "circular");
	gtk_style_context_add_class (gtk_widget_get_style_context (button), "revert-shortcut-button");
	gtk_box_pack_start (GTK_BOX (button_box), button, FALSE, FALSE, 0);

	gtk_widget_show_all (row);
	row_data_update_accel_label (row_data);

	g_signal_connect (row_data->revert_button,
			  "clicked",
			  G_CALLBACK (revert_button_clicked_cb),
			  row_data);

	return row;
}


static GtkWidget *
_new_shortcut_category_row (const char *category_id,
			    int         n_category)
{
	GtkWidget           *row;
	GtkWidget           *box;
	GthShortcutCategory *category;
	const char          *text;
	char                *esc_text;
	char                *markup_text;
	GtkWidget           *label;

	row = gtk_list_box_row_new ();
	gtk_list_box_row_set_activatable (GTK_LIST_BOX_ROW (row), FALSE);
	gtk_list_box_row_set_selectable (GTK_LIST_BOX_ROW (row), FALSE);

	box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
	if (n_category > 1)
		gtk_widget_set_margin_top (box, 15);
	gtk_container_add (GTK_CONTAINER (row), box);

	category = gth_main_get_shortcut_category (category_id);
	text = (category != NULL) ? _(category->display_name) : _("Other");
	esc_text = g_markup_escape_text (text, -1);
	markup_text = g_strdup_printf ("<b>%s</b>", esc_text);

	label =  gtk_label_new (NULL);
	gtk_label_set_markup (GTK_LABEL (label), markup_text);
	gtk_label_set_xalign (GTK_LABEL (label), 0.0);
	gtk_widget_set_margin_start (label, 5);
	gtk_widget_set_margin_end (label, 5);
	gtk_widget_set_margin_top (label, 5);
	gtk_widget_set_margin_bottom (label, 5);
	gtk_box_pack_start (GTK_BOX (box), label, FALSE, FALSE, 0);
	gtk_style_context_add_class (gtk_widget_get_style_context (label), GTK_STYLE_CLASS_DIM_LABEL);
	gtk_box_pack_start (GTK_BOX (box), gtk_separator_new (GTK_ORIENTATION_HORIZONTAL), FALSE, FALSE, 0);

	gtk_widget_show_all (row);

	g_free (markup_text);
	g_free (esc_text);

	return row;
}


static void
restore_all_button_clicked_cb (GtkButton *button,
			       gpointer   user_data)
{
	BrowserData *data = user_data;
	GtkWidget   *dialog;
	gboolean     reassign;

	dialog = _gtk_yesno_dialog_new (GTK_WINDOW (data->preferences_dialog),
					GTK_DIALOG_MODAL,
					_("Do you want to revert all the changes and use the default shortcuts?"),
					_GTK_LABEL_CANCEL,
					_("Revert"));
	_gtk_dialog_add_class_to_response (GTK_DIALOG (dialog), GTK_RESPONSE_YES, GTK_STYLE_CLASS_DESTRUCTIVE_ACTION);

	reassign = gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_YES;
	gtk_widget_destroy (GTK_WIDGET (dialog));

	if (reassign) {
		int i;

		for (i = 0; i < data->rows->len; i++) {
			RowData *row_data = g_ptr_array_index (data->rows, i);

			gth_shortcut_set_accelerator (row_data->shortcut, row_data->shortcut->default_accelerator);
			row_data_update_accel_label (row_data);
		}

		gth_main_shortcuts_changed (gth_window_get_shortcuts (GTH_WINDOW (data->browser)));
	}
}


static int
cmp_category (gconstpointer a,
	      gconstpointer b)
{
	GthShortcutCategory *cat_a = * (GthShortcutCategory **) a;
	GthShortcutCategory *cat_b = * (GthShortcutCategory **) b;

	if (cat_a->sort_order == cat_b->sort_order)
		return 0;
	else if (cat_a->sort_order > cat_b->sort_order)
		return 1;
	else
		return -1;
}


static void
category_combo_box_changed_cb (GtkComboBox *combo_box,
			       gpointer     user_data)
{
	BrowserData *data = user_data;
	GtkTreeIter  iter;

	if (gtk_combo_box_get_active_iter (combo_box, &iter)) {
		char *category_id;

		gtk_tree_model_get (GTK_TREE_MODEL (gtk_builder_get_object (data->builder, "category_liststore")),
				    &iter,
				    0, &category_id,
				    -1);

		g_free (data->show_category);
		data->show_category = g_strdup (category_id);

		gtk_list_box_invalidate_filter (GTK_LIST_BOX (_gtk_builder_get_widget (data->builder, "shortcuts_list")));

		g_free (category_id);
	}
}


static gboolean
shortcut_filter_func (GtkListBoxRow *row,
		      gpointer       user_data)
{
	BrowserData *data = user_data;
	RowData     *row_data;

	if (g_strcmp0 (data->show_category, GTH_SHORTCUT_CATEGORY_ALL) == 0)
		return TRUE;

	row_data = g_object_get_data (G_OBJECT (row), "shortcut-row-data");
	if ((row_data == NULL) || (row_data->shortcut == NULL))
		return FALSE;

	if (g_strcmp0 (data->show_category, GTH_SHORTCUT_CATEGORY_MODIFIED) == 0)
		return row_data->modified;

	return g_strcmp0 (data->show_category, row_data->shortcut->category) == 0;
}


void
shortcuts__dlg_preferences_construct_cb (GtkWidget  *dialog,
					 GthBrowser *browser,
					 GtkBuilder *dialog_builder)
{
	BrowserData *data;
	GHashTable  *visible_categories;
	GtkWidget   *shortcuts_list;
	GPtrArray   *shortcuts_v;
	const char  *last_category;
	int          n_category;
	int          i;
	GtkWidget   *label;
	GtkWidget   *page;

	data = g_new0 (BrowserData, 1);
	data->browser = browser;
	data->builder = _gtk_builder_new_from_file ("shortcuts-preferences.ui", NULL);
	data->preferences_dialog = dialog;
	data->rows = g_ptr_array_new ();
	data->show_category = g_strdup (GTH_SHORTCUT_CATEGORY_ALL);

	g_object_set_data_full (G_OBJECT (dialog), BROWSER_DATA_KEY, data, (GDestroyNotify) browser_data_free);

	/* shortcut list */

	visible_categories = g_hash_table_new (g_str_hash, g_str_equal);
	shortcuts_list = _gtk_builder_get_widget (data->builder, "shortcuts_list");
	shortcuts_v = gth_window_get_shortcuts_by_category (GTH_WINDOW (browser));
	last_category = NULL;
	n_category = 0;
	for (i = 0; i < shortcuts_v->len; i++) {
		GthShortcut *shortcut = g_ptr_array_index (shortcuts_v, i);

		if (g_strcmp0 (shortcut->category, GTH_SHORTCUT_CATEGORY_HIDDEN) == 0)
			continue;

		if ((shortcut->context & GTH_SHORTCUT_CONTEXT_INTERNAL) != 0)
			continue;

		if ((shortcut->context & GTH_SHORTCUT_CONTEXT_DOC) != 0)
			continue;

		if ((shortcut->context & GTH_SHORTCUT_CONTEXT_FIXED) != 0)
			continue;

		if (g_strcmp0 (shortcut->category,last_category) != 0) {
			last_category = shortcut->category;
			n_category++;
			g_hash_table_add (visible_categories, shortcut->category);
			gtk_list_box_insert (GTK_LIST_BOX (shortcuts_list),
					     _new_shortcut_category_row (shortcut->category, n_category),
					     -1);
		}
		gtk_list_box_insert (GTK_LIST_BOX (shortcuts_list),
				     _new_shortcut_row (shortcut, data),
				     -1);
	}
	gtk_list_box_set_filter_func (GTK_LIST_BOX (shortcuts_list),
				      shortcut_filter_func,
				      data,
				      NULL);

	g_signal_connect (shortcuts_list,
			  "row-activated",
			  G_CALLBACK (shortcuts_list_row_activated_cb),
			  data);
	g_signal_connect (_gtk_builder_get_widget (data->builder, "restore_all_button"),
			  "clicked",
			  G_CALLBACK (restore_all_button_clicked_cb),
			  data);

	/* shortcut categories */

	{
		GPtrArray    *category_v;
		GtkListStore *list_store;
		GtkTreeIter   iter;
		int           i;

		category_v = _g_ptr_array_dup (gth_main_get_shortcut_categories (), NULL, NULL);
		g_ptr_array_sort (category_v, cmp_category);

		list_store = (GtkListStore *) gtk_builder_get_object (data->builder, "category_liststore");

		gtk_list_store_append (list_store, &iter);
		gtk_list_store_set (list_store, &iter,
				    0, GTH_SHORTCUT_CATEGORY_ALL,
				    1, C_("Shortcuts", "All"),
				    -1);

		gtk_list_store_append (list_store, &iter);
		gtk_list_store_set (list_store, &iter,
				    0, GTH_SHORTCUT_CATEGORY_MODIFIED,
				    1, C_("Shortcuts", "Modified"),
				    -1);

		for (i = 0; i < category_v->len; i++) {
			GthShortcutCategory *category = g_ptr_array_index (category_v, i);

			if (! g_hash_table_contains (visible_categories, category->id))
				continue;

			gtk_list_store_append (list_store, &iter);
			gtk_list_store_set (list_store, &iter,
					    0, category->id,
					    1, _(category->display_name),
					    -1);
		}

		gtk_combo_box_set_active (GTK_COMBO_BOX (_gtk_builder_get_widget (data->builder, "category_combobox")), 0);
		g_signal_connect (_gtk_builder_get_widget (data->builder, "category_combobox"),
				  "changed",
				  G_CALLBACK (category_combo_box_changed_cb),
				  data);

		g_ptr_array_unref (category_v);
	}

	g_hash_table_unref (visible_categories);

	/* add the page to the preferences dialog */

	label = gtk_label_new (_("Shortcuts"));
	gtk_widget_show (label);

	page = _gtk_builder_get_widget (data->builder, "preferences_page");
	gtk_widget_show (page);
	gtk_notebook_append_page (GTK_NOTEBOOK (_gtk_builder_get_widget (dialog_builder, "notebook")), page, label);
}
