/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2011 The Free Software Foundation, Inc.
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
#include "gth-template-selector.h"


#define GET_WIDGET(x) (_gtk_builder_get_widget (self->priv->builder, (x)))


G_DEFINE_TYPE (GthTemplateSelector, gth_template_selector, GTK_TYPE_BOX)


enum {
	TYPE_DATA_COLUMN,
	TYPE_NAME_COLUMN,
	TYPE_N_COLUMNS
};

enum {
	DATE_FORMAT_FORMAT_COLUMN,
	DATE_FORMAT_NAME_COLUMN,
	DATE_FORMAT_N_COLUMNS
};

enum {
	ATTRIBUTE_ID_COLUMN,
	ATTRIBUTE_NAME_COLUMN,
	ATTRIBUTE_SORT_ORDER_COLUMN,
	ATTRIBUTE_N_COLUMNS,
};

enum {
        ADD_TEMPLATE,
        REMOVE_TEMPLATE,
        LAST_SIGNAL
};


struct _GthTemplateSelectorPrivate {
	GtkBuilder *builder;
};

static char * Date_Formats[] = { "%Y-%m-%d--%H.%M.%S", "%Y-%m-%d", "%x %X", "%x", NULL };
static guint  gth_template_selector_signals[LAST_SIGNAL] = { 0 };


static void
gth_template_selector_finalize (GObject *object)
{
	GthTemplateSelector *selector;

	selector = GTH_TEMPLATE_SELECTOR (object);

	if (selector->priv != NULL) {
		_g_object_unref (selector->priv->builder);
		g_free (selector->priv);
		selector->priv = NULL;
	}

	G_OBJECT_CLASS (gth_template_selector_parent_class)->finalize (object);
}


static void
gth_template_selector_class_init (GthTemplateSelectorClass *klass)
{
	G_OBJECT_CLASS (klass)->finalize = gth_template_selector_finalize;

	/* signals */

	gth_template_selector_signals[ADD_TEMPLATE] =
		g_signal_new ("add-template",
			      G_TYPE_FROM_CLASS (klass),
 			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (GthTemplateSelectorClass, add_template),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__VOID,
			      G_TYPE_NONE,
			      0);
	gth_template_selector_signals[REMOVE_TEMPLATE] =
		g_signal_new ("remove-template",
			      G_TYPE_FROM_CLASS (klass),
 			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (GthTemplateSelectorClass, remove_template),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__VOID,
			      G_TYPE_NONE,
			      0);
}


static void
gth_template_selector_init (GthTemplateSelector *self)
{
	self->priv = g_new0 (GthTemplateSelectorPrivate, 1);
	self->priv->builder = NULL;

	gtk_orientable_set_orientation (GTK_ORIENTABLE (self), GTK_ORIENTATION_HORIZONTAL);
}


static void
add_button_clicked_cb (GtkButton           *button,
		       GthTemplateSelector *self)
{
	g_signal_emit (self, gth_template_selector_signals[ADD_TEMPLATE], 0);
}


static void
remove_button_clicked_cb (GtkButton           *button,
			  GthTemplateSelector *self)
{
	g_signal_emit (self, gth_template_selector_signals[REMOVE_TEMPLATE], 0);
}


static void
type_combobox_changed_cb (GtkComboBox         *combo_box,
			  GthTemplateSelector *self)
{
	GtkTreeIter      iter;
	GthTemplateCode *code;

	if (! gtk_combo_box_get_active_iter (combo_box, &iter))
		return;

	gtk_tree_model_get (GTK_TREE_MODEL (GET_WIDGET ("type_liststore")),
			    &iter,
			    TYPE_DATA_COLUMN, &code,
			    -1);

	gtk_notebook_set_current_page (GTK_NOTEBOOK (GET_WIDGET ("type_notebook")), code->type);
}


static void
date_format_combobox_changed_cb (GtkComboBox         *combo_box,
				 GthTemplateSelector *self)
{
	gboolean custom_format;

	custom_format = gtk_combo_box_get_active (GTK_COMBO_BOX (GET_WIDGET ("date_format_combobox"))) == G_N_ELEMENTS (Date_Formats) - 1;
	if (custom_format) {
		gtk_widget_hide (GET_WIDGET ("date_format_combobox"));
		gtk_widget_show (GET_WIDGET ("custom_date_format_entry"));
		gtk_widget_grab_focus (GET_WIDGET ("custom_date_format_entry"));
	}
	else {
		gtk_widget_show (GET_WIDGET ("date_format_combobox"));
		gtk_widget_hide (GET_WIDGET ("custom_date_format_entry"));
	}
}


static void
gth_template_selector_construct (GthTemplateSelector *self,
				 GthTemplateCode     *allowed_codes,
				 int                  n_codes)
{
	GtkListStore  *list_store;
	GtkTreeIter    iter;
	int            i;
	GTimeVal       timeval;
	GHashTable    *category_root;
	GtkTreeStore  *tree_store;
	char         **attributes_v;

	gtk_box_set_spacing (GTK_BOX (self), 6);
	gtk_container_set_border_width (GTK_CONTAINER (self), 0);

	self->priv->builder = _gtk_builder_new_from_file("code-selector.ui", "rename_series");
	gtk_container_add (GTK_CONTAINER (self), GET_WIDGET ("code_selector"));

	gtk_spin_button_set_value (GTK_SPIN_BUTTON (GET_WIDGET ("enumerator_digits_spinbutton")), 1.0);

	/* code types */

	list_store = (GtkListStore *) GET_WIDGET ("type_liststore");
	for (i = 0; i < n_codes; i++) {
		GthTemplateCode *code = &allowed_codes[i];

		gtk_list_store_append (list_store, &iter);
		gtk_list_store_set (list_store, &iter,
				    TYPE_DATA_COLUMN, code,
				    TYPE_NAME_COLUMN, _(code->description),
				    -1);
	}

	gtk_notebook_set_current_page (GTK_NOTEBOOK (GET_WIDGET ("type_notebook")), GTH_TEMPLATE_CODE_TYPE_SIMPLE);

	/* date formats */

	g_get_current_time (&timeval);
	list_store = (GtkListStore *) GET_WIDGET ("date_format_liststore");
	for (i = 0; Date_Formats[i] != NULL; i++) {
		char *example;

		example = _g_time_val_strftime (&timeval, Date_Formats[i]);

		gtk_list_store_append (list_store, &iter);
		gtk_list_store_set (list_store, &iter,
				    DATE_FORMAT_FORMAT_COLUMN, Date_Formats[i],
				    DATE_FORMAT_NAME_COLUMN, example,
				    -1);

		g_free (example);
	}

	gtk_list_store_append (list_store, &iter);
	gtk_list_store_set (list_store, &iter,
			    DATE_FORMAT_FORMAT_COLUMN, "",
			    DATE_FORMAT_NAME_COLUMN, _("Custom"),
			    -1);

	gtk_combo_box_set_active (GTK_COMBO_BOX (GET_WIDGET ("date_format_combobox")), 0);

	/* attributes */

	gtk_combo_box_set_model (GTK_COMBO_BOX (GET_WIDGET ("attribute_combobox")), NULL);
	tree_store = (GtkTreeStore *) GET_WIDGET ("attribute_treestore");
	category_root = g_hash_table_new_full (g_str_hash, g_str_equal, (GDestroyNotify) g_free, (GDestroyNotify) gtk_tree_row_reference_free);
	attributes_v = gth_main_get_metadata_attributes ("*");
	for (i = 0; attributes_v[i] != NULL; i++) {
		GthMetadataInfo     *info;
		const char          *name;
		GthMetadataCategory *category;
		GtkTreeRowReference *parent_row;
		GtkTreePath         *path;
		GtkTreeIter          root_iter;

		info = gth_main_get_metadata_info (attributes_v[i]);
		if (info == NULL)
			continue;
		if ((info->flags & GTH_METADATA_ALLOW_IN_PROPERTIES_VIEW) == 0)
			continue;

		name = info->display_name;
		if (name == NULL)
			name = info->id;

		category = gth_main_get_metadata_category (info->category);
		parent_row = g_hash_table_lookup (category_root, category->id);
		if (parent_row == NULL) {
			gtk_tree_store_append (tree_store, &iter, NULL);
			gtk_tree_store_set (tree_store,
					    &iter,
					    ATTRIBUTE_ID_COLUMN, category->id,
					    ATTRIBUTE_NAME_COLUMN, _(category->display_name),
					    ATTRIBUTE_SORT_ORDER_COLUMN, category->sort_order,
					    -1);

			path = gtk_tree_model_get_path (GTK_TREE_MODEL (tree_store), &iter);
			parent_row = gtk_tree_row_reference_new (GTK_TREE_MODEL (tree_store), path);
			g_hash_table_insert (category_root, g_strdup (info->category), parent_row);

			gtk_tree_path_free (path);
		}

		path = gtk_tree_row_reference_get_path (parent_row);
		gtk_tree_model_get_iter (GTK_TREE_MODEL (tree_store), &root_iter, path);
		gtk_tree_path_free (path);

		gtk_tree_store_append (tree_store, &iter, &root_iter);
		gtk_tree_store_set (tree_store, &iter,
				    ATTRIBUTE_ID_COLUMN, info->id,
				    ATTRIBUTE_NAME_COLUMN, name,
				    ATTRIBUTE_SORT_ORDER_COLUMN, info->sort_order,
				    -1);
	}
	g_strfreev (attributes_v);
	g_hash_table_destroy (category_root);

	gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (tree_store), ATTRIBUTE_SORT_ORDER_COLUMN, GTK_SORT_ASCENDING);
	gtk_combo_box_set_model (GTK_COMBO_BOX (GET_WIDGET ("attribute_combobox")), GTK_TREE_MODEL (tree_store));

	/* signals */

	g_signal_connect (GET_WIDGET ("add_button"),
			  "clicked",
			  G_CALLBACK (add_button_clicked_cb),
			  self);
	g_signal_connect (GET_WIDGET ("remove_button"),
			  "clicked",
			  G_CALLBACK (remove_button_clicked_cb),
			  self);
	g_signal_connect (GET_WIDGET ("type_combobox"),
			  "changed",
			  G_CALLBACK (type_combobox_changed_cb),
			  self);
	g_signal_connect (GET_WIDGET ("date_format_combobox"),
			  "changed",
			  G_CALLBACK (date_format_combobox_changed_cb),
			  self);
}


GtkWidget *
gth_template_selector_new (GthTemplateCode *allowed_codes,
		           int              n_codes)
{
	GthTemplateSelector *self;

	self = g_object_new (GTH_TYPE_TEMPLATE_SELECTOR, NULL);
	gth_template_selector_construct (self, allowed_codes, n_codes);

	return (GtkWidget *) self;
}


static char *
get_format_from_value (const char *value)
{
	char    *format = NULL;
	GRegex  *re;
	char   **a;
	int      i;

	re = g_regex_new ("%.\\{([^}]+)\\}", 0, 0, NULL);
	a = g_regex_split (re, value, 0);
	for (i = 1; i < g_strv_length (a); i += 2)
		format = g_strstrip (g_strdup (a[i]));

	g_strfreev (a);
	g_regex_unref (re);

	return format;
}


static gboolean
_gtk_tree_model_get_iter_from_attribute_id (GtkTreeModel *tree_model,
					    GtkTreeIter  *root,
					    const char   *attribute_id,
					    GtkTreeIter  *result)
{
	GtkTreeIter  iter;
	char        *iter_id;

	if (root != NULL) {
		gtk_tree_model_get (tree_model,
				    root,
				    ATTRIBUTE_ID_COLUMN, &iter_id,
				    -1);
		if (g_strcmp0 (attribute_id, iter_id) == 0) {
			g_free (iter_id);
			*result = *root;
			return TRUE;
		}

		g_free (iter_id);
	}

	if (! gtk_tree_model_iter_children (tree_model, &iter, root))
		return FALSE;

	do {
		if (_gtk_tree_model_get_iter_from_attribute_id (tree_model, &iter, attribute_id, result))
			return TRUE;
	}
	while (gtk_tree_model_iter_next (tree_model, &iter));

	return FALSE;
}


void
gth_template_selector_set_value (GthTemplateSelector *self,
				 const char          *value)
{
	GthTemplateCode *code = NULL;
	GtkTreeModel    *tree_model;
	GtkTreeIter      iter;
	GtkTreeIter      text_iter;
	gboolean         allows_text = FALSE;
	int              i;
	gboolean         predefined_format;
	char            *format;
	char            *attribute_id;

	tree_model = (GtkTreeModel *) GET_WIDGET ("type_liststore");
	if (gtk_tree_model_get_iter_first (tree_model, &iter)) {
		do {
			GthTemplateCode *iter_code = NULL;

			gtk_tree_model_get (tree_model,
					    &iter,
					    TYPE_DATA_COLUMN, &iter_code,
					    -1);

			if (iter_code->type == GTH_TEMPLATE_CODE_TYPE_TEXT) {
				allows_text = TRUE;
				text_iter = iter;
			}

			if ((value[0] == '%')
			    && ((iter_code->type == GTH_TEMPLATE_CODE_TYPE_SIMPLE)
				|| (iter_code->type == GTH_TEMPLATE_CODE_TYPE_DATE)
				|| (iter_code->type == GTH_TEMPLATE_CODE_TYPE_FILE_ATTRIBUTE))
			    && (value[1] == iter_code->code))
			{
				code = iter_code;
			}
			else if ((iter_code->type == GTH_TEMPLATE_CODE_TYPE_ENUMERATOR) && (value[0] == iter_code->code)) {
				code = iter_code;
			}
		}
		while ((code == NULL) && gtk_tree_model_iter_next (tree_model, &iter));
	}

	if ((code == NULL) && ! allows_text)
		return;

	if ((code == NULL) && allows_text) {
		gtk_combo_box_set_active_iter (GTK_COMBO_BOX (GET_WIDGET ("type_combobox")), &text_iter);
		gtk_notebook_set_current_page (GTK_NOTEBOOK (GET_WIDGET ("type_notebook")), GTH_TEMPLATE_CODE_TYPE_TEXT);
		gtk_entry_set_text (GTK_ENTRY (GET_WIDGET ("text_entry")), value);
		return;
	}

	gtk_combo_box_set_active_iter (GTK_COMBO_BOX (GET_WIDGET ("type_combobox")), &iter);
	gtk_notebook_set_current_page (GTK_NOTEBOOK (GET_WIDGET ("type_notebook")), code->type);

	switch (code->type) {
	case GTH_TEMPLATE_CODE_TYPE_ENUMERATOR:
		gtk_spin_button_set_value (GTK_SPIN_BUTTON (GET_WIDGET ("enumerator_digits_spinbutton")), strlen (value));
		break;

	case GTH_TEMPLATE_CODE_TYPE_DATE:
		predefined_format = FALSE;
		format = get_format_from_value (value);
		if (format == NULL)
			format = g_strdup (DEFAULT_STRFTIME_FORMAT);
		for (i = 0; Date_Formats[i] != NULL; i++) {
			if (g_str_equal (format, Date_Formats[i])) {
				gtk_combo_box_set_active (GTK_COMBO_BOX (GET_WIDGET ("date_format_combobox")), i);
				predefined_format = TRUE;
				break;
			}
		}
		if (! predefined_format) {
			gtk_combo_box_set_active (GTK_COMBO_BOX (GET_WIDGET ("date_format_combobox")), G_N_ELEMENTS (Date_Formats) - 1);
			gtk_entry_set_text (GTK_ENTRY (GET_WIDGET ("custom_date_format_entry")), format);
		}
		g_free (format);
		break;

	case GTH_TEMPLATE_CODE_TYPE_FILE_ATTRIBUTE:
		attribute_id = get_format_from_value (value);
		if (attribute_id != NULL) {
			GtkTreeModel *tree_model;
			GtkTreeIter   iter;

			tree_model = (GtkTreeModel *) GET_WIDGET ("attribute_treestore");
			if (_gtk_tree_model_get_iter_from_attribute_id (tree_model, NULL, attribute_id, &iter))
				gtk_combo_box_set_active_iter (GTK_COMBO_BOX (GET_WIDGET ("attribute_combobox")), &iter);
		}
		g_free (attribute_id);
		break;

	default:
		break;
	}
}


char *
gth_template_selector_get_value (GthTemplateSelector  *self,
				 GError              **error)
{
	GString         *value;
	GtkTreeIter      iter;
	GthTemplateCode *code;
	int              i;

	if (! gtk_combo_box_get_active_iter (GTK_COMBO_BOX (GET_WIDGET ("type_combobox")), &iter))
		return NULL;

	gtk_tree_model_get (GTK_TREE_MODEL (GET_WIDGET ("type_liststore")),
			    &iter,
			    TYPE_DATA_COLUMN, &code,
			    -1);

	value = g_string_new ("");

	switch (code->type) {
	case GTH_TEMPLATE_CODE_TYPE_TEXT:
		g_string_append (value, gtk_entry_get_text (GTK_ENTRY (GET_WIDGET ("text_entry"))));
		break;

	case GTH_TEMPLATE_CODE_TYPE_ENUMERATOR:
		for (i = 0; i < gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (GET_WIDGET ("enumerator_digits_spinbutton"))); i++)
			g_string_append_c (value, code->code);
		break;

	case GTH_TEMPLATE_CODE_TYPE_SIMPLE:
		g_string_append (value, "%");
		g_string_append_c (value, code->code);
		break;

	case GTH_TEMPLATE_CODE_TYPE_DATE:
		g_string_append (value, "%");
		g_string_append_c (value, code->code);
		if (gtk_combo_box_get_active_iter (GTK_COMBO_BOX (GET_WIDGET ("date_format_combobox")), &iter)) {
			char *format;

			gtk_tree_model_get (GTK_TREE_MODEL (GET_WIDGET ("date_format_liststore")),
					    &iter,
					    DATE_FORMAT_FORMAT_COLUMN, &format,
					    -1);

			if ((format == NULL) || (strcmp (format, "") == 0))
				format = g_strdup (gtk_entry_get_text (GTK_ENTRY (GET_WIDGET ("custom_date_format_entry"))));

			if ((format != NULL) && (strcmp (format, "") != 0) && (strcmp (format, DEFAULT_STRFTIME_FORMAT) != 0))
				g_string_append_printf (value, "{ %s }", format);

			g_free (format);
		}
		break;

	case GTH_TEMPLATE_CODE_TYPE_FILE_ATTRIBUTE:
		if (gtk_combo_box_get_active_iter (GTK_COMBO_BOX (GET_WIDGET ("attribute_combobox")), &iter)) {
			char *attribute_id;

			gtk_tree_model_get (GTK_TREE_MODEL (GET_WIDGET ("attribute_treestore")),
					    &iter,
					    ATTRIBUTE_ID_COLUMN, &attribute_id,
					    -1);
			if ((attribute_id != NULL) && (strcmp (attribute_id, "") != 0)) {
				g_string_append_printf (value, "%%%c{ %s }", code->code, attribute_id);
			}

			g_free (attribute_id);
		}
		break;
	}

	return g_string_free (value, FALSE);
}


void
gth_template_selector_can_remove (GthTemplateSelector *self,
				  gboolean             value)
{
	gtk_widget_set_sensitive (GET_WIDGET ("remove_button"), value);
}
