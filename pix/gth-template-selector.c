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
#include "gth-template-selector.h"
#include "gth-template-editor-dialog.h"
#include "glib-utils.h"
#include "gtk-utils.h"
#include "gth-main.h"


#define GET_WIDGET(x) (_gtk_builder_get_widget (self->priv->builder, (x)))


static GthTemplateCode Date_Format_Special_Codes[] = {
	{ GTH_TEMPLATE_CODE_TYPE_SIMPLE, N_("Year"), 'Y' },
	{ GTH_TEMPLATE_CODE_TYPE_SIMPLE, N_("Month"), 'm' },
	{ GTH_TEMPLATE_CODE_TYPE_SIMPLE, N_("Day of the month"), 'd' },
	{ GTH_TEMPLATE_CODE_TYPE_SIMPLE, N_("Hour"), 'H' },
	{ GTH_TEMPLATE_CODE_TYPE_SIMPLE, N_("Minute"), 'M' },
	/* Translators: the time second, not the second place. */
	{ GTH_TEMPLATE_CODE_TYPE_SIMPLE, N_("Second"), 'S' },
};


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
	EDIT_TEMPLATE,
	CHANGED,
	LAST_SIGNAL
};


struct _GthTemplateSelectorPrivate {
	GtkBuilder   *builder;
	GtkTreeModel *date_format_liststore;
	int           n_date_formats;
};

static char *TypeName[] = {
	"space",
	"text",
	"enumerator",
	"simple",
	"date",
	"file_attribute",
	"ask_value",
	"quoted"
};

static char *Default_Date_Formats[] = {
	"%Y-%m-%d--%H.%M.%S",
	"%x %X",
	"%Y%m%d%H%M%S",
	"%Y-%m-%d",
	"%x",
	"%Y%m%d",
	"%H.%M.%S",
	"%X",
	"%H%M%S",
	NULL
};


G_DEFINE_TYPE_WITH_CODE (GthTemplateSelector,
			 gth_template_selector,
			 GTK_TYPE_BOX,
			 G_ADD_PRIVATE (GthTemplateSelector))


static guint  gth_template_selector_signals[LAST_SIGNAL] = { 0 };


static void
gth_template_selector_finalize (GObject *object)
{
	GthTemplateSelector *selector;

	selector = GTH_TEMPLATE_SELECTOR (object);

	_g_object_unref (selector->priv->builder);

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
	gth_template_selector_signals[EDIT_TEMPLATE] =
		g_signal_new ("edit-template",
			      G_TYPE_FROM_CLASS (klass),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (GthTemplateSelectorClass, edit_template),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__OBJECT,
			      G_TYPE_NONE,
			      1,
			      GTK_TYPE_ENTRY);
	gth_template_selector_signals[CHANGED] =
		g_signal_new ("changed",
			      G_TYPE_FROM_CLASS (klass),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (GthTemplateSelectorClass, changed),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__VOID,
			      G_TYPE_NONE,
			      0);
}


static void
gth_template_selector_init (GthTemplateSelector *self)
{
	self->priv = gth_template_selector_get_instance_private (self);
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
edit_quoted_text_button_clicked_cb (GtkButton           *button,
				    GthTemplateSelector *self)
{
	GtkWidget *entry = GET_WIDGET ("quoted_entry");

	gtk_widget_grab_focus (entry);
	g_signal_emit (self, gth_template_selector_signals[EDIT_TEMPLATE], 0, entry);
}


static void
_gth_template_selector_changed (GthTemplateSelector *self)
{
	g_signal_emit (self, gth_template_selector_signals[CHANGED], 0);
}


static void
type_combobox_changed_cb (GtkComboBox         *combo_box,
			  GthTemplateSelector *self)
{
	GthTemplateCode *code;

	code = gth_template_selector_get_code (self);
	if (code == NULL)
		return;

	gtk_stack_set_visible_child_name (GTK_STACK (GET_WIDGET ("type_stack")), TypeName[code->type]);

	gth_template_selector_focus (self);
	_gth_template_selector_changed (self);
}


static void
date_format_combobox_changed_cb (GtkComboBox         *combo_box,
				 GthTemplateSelector *self)
{
	gboolean custom_format;

	custom_format = gtk_combo_box_get_active (GTK_COMBO_BOX (GET_WIDGET ("date_format_combobox"))) == self->priv->n_date_formats - 1;
	if (custom_format) {
		gtk_widget_hide (GET_WIDGET ("date_format_combobox"));
		gtk_widget_show (GET_WIDGET ("custom_date_format_box"));
		gtk_widget_grab_focus (GET_WIDGET ("custom_date_format_entry"));
	}
	else {
		gtk_widget_show (GET_WIDGET ("date_format_combobox"));
		gtk_widget_hide (GET_WIDGET ("custom_date_format_box"));
	}

	_gth_template_selector_changed (self);
}


static void
custom_date_format_entry_icon_release_cb (GtkEntry            *entry,
					  GtkEntryIconPosition icon_pos,
					  GdkEvent            *event,
					  gpointer             user_data)
{
	GthTemplateSelector *self = user_data;

	if (icon_pos == GTK_ENTRY_ICON_SECONDARY) {
		gtk_combo_box_set_active (GTK_COMBO_BOX (GET_WIDGET ("date_format_combobox")), 0);
		_gth_template_selector_changed (self);
	}
}


static void
editable_changed_cb (GtkEditable *editable,
		     gpointer     user_data)
{
	_gth_template_selector_changed (GTH_TEMPLATE_SELECTOR (user_data));
}


static void
attribute_combobox_changed_cb (GtkComboBox *combo_box,
			       gpointer     user_data)
{
	_gth_template_selector_changed (GTH_TEMPLATE_SELECTOR (user_data));
}


static void
edit_default_value_button_clicked_cb (GtkButton           *button,
				      GthTemplateSelector *self)
{
	GtkWidget *entry = GET_WIDGET ("default_value_entry");

	gtk_widget_grab_focus (entry);
	g_signal_emit (self, gth_template_selector_signals[EDIT_TEMPLATE], 0, entry);
}


static gboolean
date_template_eval_cb (TemplateFlags   flags,
		       gunichar        parent_code,
		       gunichar        code,
		       char          **args,
		       GString        *result,
		       gpointer        user_data)
{
	gboolean   preview;
	GDateTime *timestamp;
	char      *format;
	char      *text;

	preview = flags & TEMPLATE_FLAGS_PREVIEW;

	if (preview && (code != 0))
		g_string_append (result, "<span foreground=\"#4696f8\">");

	switch (code) {
	case 'Y': /* Year */
	case 'm': /* Month */
	case 'd': /* Day of the month  */
	case 'H': /* Hour */
	case 'M': /* Minutes */
	case 'S': /* Seconds */
		timestamp = g_date_time_new_now_local ();
		format = g_strdup_printf ("%%%c", code);
		text = g_date_time_format (timestamp, format);
		g_string_append (result, text);

		g_free (text);
		g_free (format);
		g_date_time_unref (timestamp);
		break;

	default:
		break;
	}

	if (preview && (code != 0))
		g_string_append (result, "</span>");

	return FALSE;
}


static void
edit_custom_date_format_button_clicked_cb (GtkButton           *button,
					   GthTemplateSelector *self)
{
	GtkWidget *toplevel;
	GtkWidget *dialog;

	toplevel = gtk_widget_get_toplevel (GTK_WIDGET (self));
	if (! GTK_IS_WINDOW (toplevel))
		toplevel = NULL;

	dialog = gth_template_editor_dialog_new (Date_Format_Special_Codes,
						 G_N_ELEMENTS (Date_Format_Special_Codes),
						 0,
						 _("Edit Template"),
						 GTK_WINDOW (toplevel));
	gth_template_editor_dialog_set_preview_cb (GTH_TEMPLATE_EDITOR_DIALOG (dialog),
						   date_template_eval_cb,
						   self);
	gth_template_editor_dialog_set_template (GTH_TEMPLATE_EDITOR_DIALOG (dialog),
						 gtk_entry_get_text (GTK_ENTRY (GET_WIDGET ("custom_date_format_entry"))));
	g_signal_connect (dialog,
			  "response",
			  G_CALLBACK (gth_template_editor_dialog_default_response),
			  GET_WIDGET ("custom_date_format_entry"));
	gtk_widget_show (dialog);
}


static void
gth_template_selector_construct (GthTemplateSelector  *self,
				 GthTemplateCode      *allowed_codes,
				 int                   n_codes,
				 char                **date_formats)
{
	GtkListStore  *list_store;
	GtkTreeIter    iter;
	int            i;
	GTimeVal       timeval;
	GHashTable    *category_root;
	GtkTreeStore  *tree_store;
	char         **attributes_v;

	if (date_formats == NULL)
		date_formats = Default_Date_Formats;

	gtk_box_set_spacing (GTK_BOX (self), 6);
	gtk_container_set_border_width (GTK_CONTAINER (self), 0);

	self->priv->builder = _gtk_builder_new_from_file("code-selector.ui", NULL);
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

	gtk_stack_set_visible_child_name (GTK_STACK (GET_WIDGET ("type_stack")), TypeName[GTH_TEMPLATE_CODE_TYPE_SIMPLE]);

	/* date formats */

	self->priv->date_format_liststore = (GtkTreeModel *) GET_WIDGET ("date_format_liststore");
	g_get_current_time (&timeval);
	list_store = (GtkListStore *) self->priv->date_format_liststore;
	for (i = 0; date_formats[i] != NULL; i++) {
		char *example;

		example = _g_time_val_strftime (&timeval, date_formats[i]);

		gtk_list_store_append (list_store, &iter);
		gtk_list_store_set (list_store, &iter,
				    DATE_FORMAT_FORMAT_COLUMN, date_formats[i],
				    DATE_FORMAT_NAME_COLUMN, example,
				    -1);

		g_free (example);
	}

	gtk_list_store_append (list_store, &iter);
	gtk_list_store_set (list_store, &iter,
			    DATE_FORMAT_FORMAT_COLUMN, "",
			    DATE_FORMAT_NAME_COLUMN, _("Other…"),
			    -1);
	self->priv->n_date_formats = gtk_tree_model_iter_n_children (self->priv->date_format_liststore, NULL);

	gtk_combo_box_set_active (GTK_COMBO_BOX (GET_WIDGET ("date_format_combobox")), 0);

	/* attributes */

	gtk_combo_box_set_model (GTK_COMBO_BOX (GET_WIDGET ("attribute_combobox")), NULL);
	tree_store = (GtkTreeStore *) GET_WIDGET ("attribute_treestore");
	category_root = g_hash_table_new_full (g_str_hash, g_str_equal, (GDestroyNotify) g_free, (GDestroyNotify) gtk_tree_iter_free);
	attributes_v = gth_main_get_metadata_attributes ("*");
	for (i = 0; attributes_v[i] != NULL; i++) {
		GthMetadataInfo     *info;
		const char          *name;
		GthMetadataCategory *category;
		GtkTreeIter         *root_iter;

		info = gth_main_get_metadata_info (attributes_v[i]);
		if (info == NULL)
			continue;

		if ((info->flags & GTH_METADATA_ALLOW_EVERYWHERE) == 0)
			continue;

		category = gth_main_get_metadata_category (info->category);
		if (category == NULL)
			continue;

		if (info->display_name != NULL)
			name = _(info->display_name);
		else
			name = info->id;

		root_iter = g_hash_table_lookup (category_root, category->id);
		if (root_iter == NULL) {
			gtk_tree_store_append (tree_store, &iter, NULL);
			gtk_tree_store_set (tree_store,
					    &iter,
					    ATTRIBUTE_ID_COLUMN, category->id,
					    ATTRIBUTE_NAME_COLUMN, _(category->display_name),
					    ATTRIBUTE_SORT_ORDER_COLUMN, category->sort_order,
					    -1);
			root_iter = gtk_tree_iter_copy (&iter);
			g_hash_table_insert (category_root, g_strdup (info->category), root_iter);
		}

		gtk_tree_store_append (tree_store, &iter, root_iter);
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
	g_signal_connect (GET_WIDGET ("custom_date_format_entry"),
			  "icon-release",
			  G_CALLBACK (custom_date_format_entry_icon_release_cb),
			  self);
	g_signal_connect (GET_WIDGET ("custom_date_format_entry"),
			  "changed",
			  G_CALLBACK (editable_changed_cb),
			  self);
	g_signal_connect (GET_WIDGET ("edit_quoted_text_button"),
			  "clicked",
			  G_CALLBACK (edit_quoted_text_button_clicked_cb),
			  self);
	g_signal_connect (GET_WIDGET ("quoted_entry"),
			  "changed",
			  G_CALLBACK (editable_changed_cb),
			  self);
	g_signal_connect (GET_WIDGET ("text_entry"),
			  "changed",
			  G_CALLBACK (editable_changed_cb),
			  self);
	g_signal_connect (GET_WIDGET ("enumerator_digits_spinbutton"),
			  "changed",
			  G_CALLBACK (editable_changed_cb),
			  self);
	g_signal_connect (GET_WIDGET ("attribute_combobox"),
			  "changed",
			  G_CALLBACK (attribute_combobox_changed_cb),
			  self);
	g_signal_connect (GET_WIDGET ("prompt_entry"),
			  "changed",
			  G_CALLBACK (editable_changed_cb),
			  self);
	g_signal_connect (GET_WIDGET ("default_value_entry"),
			  "changed",
			  G_CALLBACK (editable_changed_cb),
			  self);
	g_signal_connect (GET_WIDGET ("edit_default_value_button"),
			  "clicked",
			  G_CALLBACK (edit_default_value_button_clicked_cb),
			  self);
	g_signal_connect (GET_WIDGET ("edit_custom_date_format_button"),
			  "clicked",
			  G_CALLBACK (edit_custom_date_format_button_clicked_cb),
			  self);
}


GtkWidget *
gth_template_selector_new (GthTemplateCode  *allowed_codes,
			   int               n_codes,
			   char            **date_formats)
{
	GthTemplateSelector *self;

	self = g_object_new (GTH_TYPE_TEMPLATE_SELECTOR, NULL);
	gth_template_selector_construct (self, allowed_codes, n_codes, date_formats);

	return (GtkWidget *) self;
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
	GthTemplateCode  *code = NULL;
	GtkTreeModel     *tree_model;
	GtkTreeIter       iter;
	GtkTreeIter       text_iter, space_iter;
	gboolean          predefined_format;
	char            **args;
	char             *arg;

	tree_model = (GtkTreeModel *) GET_WIDGET ("type_liststore");
	if (gtk_tree_model_get_iter_first (tree_model, &iter)) {
		do {
			GthTemplateCode *iter_code = NULL;

			gtk_tree_model_get (tree_model,
					    &iter,
					    TYPE_DATA_COLUMN, &iter_code,
					    -1);

			switch (iter_code->type) {
			case GTH_TEMPLATE_CODE_TYPE_TEXT:
				text_iter = iter;
				break;

			case GTH_TEMPLATE_CODE_TYPE_SPACE:
				space_iter = iter;
				break;

			case GTH_TEMPLATE_CODE_TYPE_ENUMERATOR:
				if (g_str_has_prefix (value, "#"))
					code = iter_code;
				break;

			default:
				if (_g_template_token_is (value, iter_code->code))
					code = iter_code;
				break;
			}
		}
		while ((code == NULL) && gtk_tree_model_iter_next (tree_model, &iter));
	}

	if (code == NULL) {
		if (_g_str_equal (value, " ")) {
			gtk_combo_box_set_active_iter (GTK_COMBO_BOX (GET_WIDGET ("type_combobox")), &space_iter);
			gtk_stack_set_visible_child_name (GTK_STACK (GET_WIDGET ("type_stack")), TypeName[GTH_TEMPLATE_CODE_TYPE_SPACE]);
		}
		else {
			gtk_combo_box_set_active_iter (GTK_COMBO_BOX (GET_WIDGET ("type_combobox")), &text_iter);
			gtk_stack_set_visible_child_name (GTK_STACK (GET_WIDGET ("type_stack")), TypeName[GTH_TEMPLATE_CODE_TYPE_TEXT]);
			gtk_entry_set_text (GTK_ENTRY (GET_WIDGET ("text_entry")), value);
		}
		return;
	}

	gtk_combo_box_set_active_iter (GTK_COMBO_BOX (GET_WIDGET ("type_combobox")), &iter);
	gtk_stack_set_visible_child_name (GTK_STACK (GET_WIDGET ("type_stack")), TypeName[code->type]);

	args = _g_template_get_token_args (value);

	switch (code->type) {
	case GTH_TEMPLATE_CODE_TYPE_ENUMERATOR:
		gtk_spin_button_set_value (GTK_SPIN_BUTTON (GET_WIDGET ("enumerator_digits_spinbutton")), strlen (value));
		break;

	case GTH_TEMPLATE_CODE_TYPE_DATE:
		predefined_format = FALSE;
		arg = g_strdup (args[0]);
		if (arg == NULL)
			arg = g_strdup (DEFAULT_STRFTIME_FORMAT);

		if (gtk_tree_model_get_iter_first (self->priv->date_format_liststore, &iter)) {
			do {
				char *format;

				gtk_tree_model_get (self->priv->date_format_liststore,
						    &iter,
						    DATE_FORMAT_FORMAT_COLUMN, &format,
						    -1);
				if (g_str_equal (arg, format)) {
					gtk_combo_box_set_active_iter (GTK_COMBO_BOX (GET_WIDGET ("date_format_combobox")), &iter);
					predefined_format = TRUE;
				}

				g_free (format);
			}
			while (! predefined_format && gtk_tree_model_iter_next (self->priv->date_format_liststore, &iter));
		}

		if (! predefined_format) {
			gtk_combo_box_set_active (GTK_COMBO_BOX (GET_WIDGET ("date_format_combobox")), self->priv->n_date_formats - 1);
			gtk_entry_set_text (GTK_ENTRY (GET_WIDGET ("custom_date_format_entry")), arg);
		}
		g_free (arg);
		break;

	case GTH_TEMPLATE_CODE_TYPE_FILE_ATTRIBUTE:
		if (args[0] != NULL) {
			if (_gtk_tree_model_get_iter_from_attribute_id (GTK_TREE_MODEL (GET_WIDGET ("attribute_treestore")), NULL, args[0], &iter))
				gtk_combo_box_set_active_iter (GTK_COMBO_BOX (GET_WIDGET ("attribute_combobox")), &iter);
		}
		break;

	case GTH_TEMPLATE_CODE_TYPE_ASK_VALUE:
		if (args[0] != NULL)
			gtk_entry_set_text (GTK_ENTRY (GET_WIDGET ("prompt_entry")), args[0]);
		if (args[1] != NULL)
			gtk_entry_set_text (GTK_ENTRY (GET_WIDGET ("default_value_entry")), args[1]);
		break;

	case GTH_TEMPLATE_CODE_TYPE_QUOTED:
		if (args[0] != NULL)
			gtk_entry_set_text (GTK_ENTRY (GET_WIDGET ("quoted_entry")), args[0]);
		break;

	default:
		break;
	}

	g_strfreev (args);
}


char *
gth_template_selector_get_value (GthTemplateSelector  *self)
{
	GthTemplateCode *code;
	GString         *value;
	GtkTreeIter      iter;
	int              i;

	code = gth_template_selector_get_code (self);
	if (code == NULL)
		return NULL;

	value = g_string_new ("");

	switch (code->type) {
	case GTH_TEMPLATE_CODE_TYPE_SPACE:
		g_string_append_c (value, ' ');
		break;

	case GTH_TEMPLATE_CODE_TYPE_TEXT:
		g_string_append (value, gtk_entry_get_text (GTK_ENTRY (GET_WIDGET ("text_entry"))));
		break;

	case GTH_TEMPLATE_CODE_TYPE_ENUMERATOR:
		for (i = 0; i < gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (GET_WIDGET ("enumerator_digits_spinbutton"))); i++)
			g_string_append_c (value, '#');
		break;

	case GTH_TEMPLATE_CODE_TYPE_SIMPLE:
		g_string_append_c (value, '%');
		g_string_append_unichar (value, code->code);
		break;

	case GTH_TEMPLATE_CODE_TYPE_DATE:
		g_string_append_c (value, '%');
		g_string_append_unichar (value, code->code);
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
				g_string_append_c (value, '%');
				g_string_append_unichar (value, code->code);
				g_string_append_printf (value, "{ %s }", attribute_id);
			}

			g_free (attribute_id);
		}
		break;

	case GTH_TEMPLATE_CODE_TYPE_QUOTED:
		g_string_append_c (value, '%');
		g_string_append_unichar (value, code->code);
		g_string_append_printf (value, "{ %s }", gtk_entry_get_text (GTK_ENTRY (GET_WIDGET ("quoted_entry"))));
		break;

	case GTH_TEMPLATE_CODE_TYPE_ASK_VALUE:
		g_string_append_c (value, '%');
		g_string_append_unichar (value, code->code);
		g_string_append_printf (value, "{ %s }", gtk_entry_get_text (GTK_ENTRY (GET_WIDGET ("prompt_entry"))));
		g_string_append_printf (value, "{ %s }", gtk_entry_get_text (GTK_ENTRY (GET_WIDGET ("default_value_entry"))));
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


void
gth_template_selector_set_quoted (GthTemplateSelector *self,
				  const char          *value)
{
	gtk_entry_set_text (GTK_ENTRY (GET_WIDGET ("quoted_entry")), value);
	_gth_template_selector_changed (self);
}


void
gth_template_selector_focus (GthTemplateSelector *self)
{
	GthTemplateCode *code;
	GtkWidget       *focused = NULL;

	code = gth_template_selector_get_code (self);
	if (code == NULL)
		return;

	focused = GET_WIDGET ("type_combobox");

	switch (code->type) {
	case GTH_TEMPLATE_CODE_TYPE_SPACE:
		break;

	case GTH_TEMPLATE_CODE_TYPE_TEXT:
		focused = GET_WIDGET ("text_entry");
		break;

	case GTH_TEMPLATE_CODE_TYPE_ENUMERATOR:
		focused = GET_WIDGET ("enumerator_digits_spinbutton");
		break;

	case GTH_TEMPLATE_CODE_TYPE_SIMPLE:
		break;

	case GTH_TEMPLATE_CODE_TYPE_DATE:
		focused = GET_WIDGET ("date_format_combobox");
		break;

	case GTH_TEMPLATE_CODE_TYPE_FILE_ATTRIBUTE:
		focused = GET_WIDGET ("attribute_combobox");
		break;

	case GTH_TEMPLATE_CODE_TYPE_QUOTED:
		focused = GET_WIDGET ("quoted_entry");
		break;

	case GTH_TEMPLATE_CODE_TYPE_ASK_VALUE:
		focused = GET_WIDGET ("prompt_entry");
		break;
	}

	if (focused != NULL)
		gtk_widget_grab_focus (focused);
}


GthTemplateCode *
gth_template_selector_get_code (GthTemplateSelector *self)
{
	GtkTreeIter      iter;
	GthTemplateCode *code = NULL;

	if (! gtk_combo_box_get_active_iter (GTK_COMBO_BOX (GET_WIDGET ("type_combobox")), &iter))
		return NULL;

	gtk_tree_model_get (GTK_TREE_MODEL (GET_WIDGET ("type_liststore")),
			    &iter,
			    TYPE_DATA_COLUMN, &code,
			    -1);

	return code;
}
