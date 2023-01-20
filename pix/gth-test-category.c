/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2008 Free Software Foundation, Inc.
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
#include <string.h>
#include <glib/gi18n.h>
#include <glib.h>
#include <gtk/gtk.h>
#include "dom.h"
#include "glib-utils.h"
#include "gth-duplicable.h"
#include "gth-enum-types.h"
#include "gth-main.h"
#include "gth-monitor.h"
#include "gth-test.h"
#include "gth-test-category.h"


typedef struct {
	char      *name;
	GthTestOp  op;
	gboolean   negative;
} GthOpData;


GthOpData category_op_data[] = {
	{ N_("is"), GTH_TEST_OP_CONTAINS, FALSE },
	{ N_("is only"), GTH_TEST_OP_CONTAINS_ONLY, FALSE },
	{ N_("is not"), GTH_TEST_OP_CONTAINS, TRUE },
	{ N_("matches"), GTH_TEST_OP_MATCHES, FALSE }
};


struct _GthTestCategoryPrivate {
	char         *category;
	GthTestOp     op;
	gboolean      negative;
	gboolean      has_focus;
	GtkListStore *tag_store;
	GtkWidget    *combo_entry;
	GtkWidget    *text_entry;
	GtkWidget    *op_combo_box;
	guint         monitor_events;
};


static DomDomizableInterface *dom_domizable_parent_iface = NULL;
static GthDuplicableInterface *gth_duplicable_parent_iface = NULL;


static void gth_test_category_dom_domizable_interface_init (DomDomizableInterface * iface);
static void gth_test_category_gth_duplicable_interface_init (GthDuplicableInterface *iface);


G_DEFINE_TYPE_WITH_CODE (GthTestCategory,
			 gth_test_category,
			 GTH_TYPE_TEST,
			 G_ADD_PRIVATE (GthTestCategory)
			 G_IMPLEMENT_INTERFACE (DOM_TYPE_DOMIZABLE,
						gth_test_category_dom_domizable_interface_init)
			 G_IMPLEMENT_INTERFACE (GTH_TYPE_DUPLICABLE,
						gth_test_category_gth_duplicable_interface_init))


static void
gth_test_category_finalize (GObject *object)
{
	GthTestCategory *test;

	test = GTH_TEST_CATEGORY (object);

	if (test->priv->monitor_events != 0) {
		g_signal_handler_disconnect (gth_main_get_default_monitor (), test->priv->monitor_events);
		test->priv->monitor_events = 0;
	}
	g_free (test->priv->category);

	G_OBJECT_CLASS (gth_test_category_parent_class)->finalize (object);
}


static gboolean
text_entry_focus_in_event_cb (GtkEntry        *entry,
			      GdkEventFocus   *event,
                              GthTestCategory *test)
{
	test->priv->has_focus = TRUE;
	return FALSE;
}


static gboolean
text_entry_focus_out_event_cb (GtkEntry        *entry,
			       GdkEventFocus   *event,
                               GthTestCategory *test)
{
	test->priv->has_focus = FALSE;
	return FALSE;
}


static void
combo_entry_changed_cb (GtkComboBox     *widget,
			GthTestCategory *test)
{
	gth_test_update_from_control (GTH_TEST (test), NULL);
	gth_test_changed (GTH_TEST (test));
}


static void
text_entry_activate_cb (GtkEntry        *entry,
                        GthTestCategory *test)
{
	gth_test_update_from_control (GTH_TEST (test), NULL);
	gth_test_changed (GTH_TEST (test));
}


static void
op_combo_box_changed_cb (GtkComboBox     *combo_box,
                         GthTestCategory *test)
{
	gth_test_update_from_control (GTH_TEST (test), NULL);
	gth_test_changed (GTH_TEST (test));
}


static void
update_tag_list (GthTestCategory *test)
{
	char **tags;
	int    i;

	gtk_list_store_clear (GTK_LIST_STORE (test->priv->tag_store));

	tags = g_strdupv (gth_tags_file_get_tags (gth_main_get_default_tag_file ()));
	for (i = 0; tags[i] != NULL; i++) {
		GtkTreeIter iter;

		gtk_list_store_append (GTK_LIST_STORE (test->priv->tag_store), &iter);
		gtk_list_store_set (GTK_LIST_STORE (test->priv->tag_store), &iter,
				    0, tags[i],
				    -1);
	}

	g_strfreev (tags);
}


static void
monitor_tags_changed_cb (GthMonitor *monitor,
		 	 gpointer    user_data)
{
	GthTestCategory *test = user_data;

	if (test->priv->tag_store != NULL)
		update_tag_list (test);
}


static GtkWidget *
gth_test_category_real_create_control (GthTest *base)
{
	GthTestCategory    *test;
	GtkWidget          *control;
	int                 i, op_idx;
	GtkEntryCompletion *completion;

	test = (GthTestCategory *) base;

	control = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
	g_signal_connect (control,
			  "destroy",
			  G_CALLBACK (gtk_widget_destroyed),
			  &test->priv->tag_store);

	/* text operation combo box */

	test->priv->op_combo_box = gtk_combo_box_text_new ();
	gtk_widget_show (test->priv->op_combo_box);

	op_idx = 0;
	for (i = 0; i < G_N_ELEMENTS (category_op_data); i++) {
		gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (test->priv->op_combo_box),
						_(category_op_data[i].name));
		if ((category_op_data[i].op == test->priv->op) && (category_op_data[i].negative == test->priv->negative))
			op_idx = i;
	}
	gtk_combo_box_set_active (GTK_COMBO_BOX (test->priv->op_combo_box), op_idx);

	g_signal_connect (G_OBJECT (test->priv->op_combo_box),
			  "changed",
			  G_CALLBACK (op_combo_box_changed_cb),
			  test);

	/* text entry */

	test->priv->tag_store = gtk_list_store_new (1, G_TYPE_STRING);
	test->priv->combo_entry = gtk_combo_box_new_with_model_and_entry (GTK_TREE_MODEL (test->priv->tag_store));
	gtk_combo_box_set_entry_text_column (GTK_COMBO_BOX (test->priv->combo_entry), 0);
	g_object_unref (test->priv->tag_store);
	update_tag_list (test);

	test->priv->text_entry = gtk_bin_get_child (GTK_BIN (test->priv->combo_entry));
	/*gtk_entry_set_width_chars (GTK_ENTRY (test->priv->text_entry), 6);*/
	if (test->priv->category != NULL)
		gtk_entry_set_text (GTK_ENTRY (test->priv->text_entry), test->priv->category);
	gtk_widget_show (test->priv->combo_entry);

	completion = gtk_entry_completion_new ();
	gtk_entry_completion_set_popup_completion (completion, TRUE);
	gtk_entry_completion_set_popup_single_match (completion, FALSE);
	gtk_entry_completion_set_inline_completion (completion, TRUE);
	gtk_entry_completion_set_model (completion, GTK_TREE_MODEL (test->priv->tag_store));
	gtk_entry_completion_set_text_column (completion, 0);
	gtk_entry_set_completion (GTK_ENTRY (test->priv->text_entry), completion);
	g_object_unref (completion);

	g_signal_connect (G_OBJECT (test->priv->text_entry),
			  "activate",
			  G_CALLBACK (text_entry_activate_cb),
			  test);
	g_signal_connect (G_OBJECT (test->priv->text_entry),
			  "focus-in-event",
			  G_CALLBACK (text_entry_focus_in_event_cb),
			  test);
	g_signal_connect (G_OBJECT (test->priv->text_entry),
			  "focus-out-event",
			  G_CALLBACK (text_entry_focus_out_event_cb),
			  test);
	g_signal_connect (G_OBJECT (test->priv->combo_entry),
			  "changed",
			  G_CALLBACK (combo_entry_changed_cb),
			  test);
	if (test->priv->monitor_events == 0)
		test->priv->monitor_events = g_signal_connect (gth_main_get_default_monitor (),
							       "tags-changed",
							       G_CALLBACK (monitor_tags_changed_cb),
							       test);
	/**/

	gtk_box_pack_start (GTK_BOX (control), test->priv->op_combo_box, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (control), test->priv->combo_entry, FALSE, FALSE, 0);

	return control;
}


static GthMatch
gth_test_category_real_match (GthTest     *test,
			      GthFileData *file)
{
	GthTestCategory *test_category;
	gboolean         result = FALSE;

        test_category = GTH_TEST_CATEGORY (test);

	if (test_category->priv->category != NULL) {
		GthMetadata   *metadata;
		GList         *list, *scan;
		char          *test_category_casefolded = NULL;
		GPatternSpec  *pattern = NULL;

		list = NULL;
		metadata = (GthMetadata *) g_file_info_get_attribute_object (file->info, gth_test_get_attributes (GTH_TEST (test_category)));
		if ((metadata != NULL) && GTH_IS_METADATA (metadata)) {
			GthStringList *string_list;

			string_list = gth_metadata_get_string_list (metadata);
			list = gth_string_list_get_list (string_list);
		}

		if (test_category->priv->op == GTH_TEST_OP_CONTAINS_ONLY) {
			if ((list == NULL) || (list->next != NULL))
				return test_category->priv->negative ? GTH_MATCH_YES : GTH_MATCH_NO;
		}

		for (scan = list; ! result && scan; scan = scan->next) {
			char *category = g_utf8_casefold (scan->data, -1);

			switch (test_category->priv->op) {
			case GTH_TEST_OP_CONTAINS:
			case GTH_TEST_OP_CONTAINS_ONLY:
				if (test_category_casefolded == NULL)
					test_category_casefolded = g_utf8_casefold (test_category->priv->category, -1);
				if (g_utf8_collate (category, test_category_casefolded) == 0)
					result = TRUE;
				break;

			case GTH_TEST_OP_MATCHES:
				if (pattern == NULL)
					pattern = g_pattern_spec_new (test_category->priv->category);
				result = g_pattern_match_string (pattern, scan->data);
				break;

			default:
				break;
			}

			g_free (category);
		}

		if (pattern != NULL)
			g_pattern_spec_free (pattern);
		g_free (test_category_casefolded);
	}

        if (test_category->priv->negative)
		result = ! result;

	return result ? GTH_MATCH_YES : GTH_MATCH_NO;
}


static DomElement*
gth_test_category_real_create_element (DomDomizable *base,
				       DomDocument  *doc)
{
	GthTestCategory *self;
	DomElement      *element;

	g_return_val_if_fail (DOM_IS_DOCUMENT (doc), NULL);

	self = GTH_TEST_CATEGORY (base);

	element = dom_document_create_element (doc, "test",
					       "id", gth_test_get_id (GTH_TEST (self)),
					       NULL);

	if (! gth_test_is_visible (GTH_TEST (self)))
		dom_element_set_attribute (element, "display", "none");

	dom_element_set_attribute (element, "op", _g_enum_type_get_value (GTH_TYPE_TEST_OP, self->priv->op)->value_nick);
	if (self->priv->negative)
		dom_element_set_attribute (element, "negative", self->priv->negative ? "true" : "false");
	if (self->priv->category != NULL)
		dom_element_set_attribute (element, "value", self->priv->category);

	return element;
}


static void
gth_test_category_set_category (GthTestCategory *self,
				const char      *category)
{
	g_free (self->priv->category);
	self->priv->category = NULL;
	if (category != NULL)
		self->priv->category = g_strdup (category);
}


static void
gth_test_category_real_load_from_element (DomDomizable *base,
					  DomElement   *element)
{
	GthTestCategory *self;
	const char    *value;

	g_return_if_fail (DOM_IS_ELEMENT (element));

	self = GTH_TEST_CATEGORY (base);

	g_object_set (self, "visible", (g_strcmp0 (dom_element_get_attribute (element, "display"), "none") != 0), NULL);

	value = dom_element_get_attribute (element, "op");
	if (value != NULL) {
		self->priv->op = _g_enum_type_get_value_by_nick (GTH_TYPE_TEST_OP, value)->value;

		/* convert EQUAL to CONTAINS for backward compatibility */
		if (self->priv->op == GTH_TEST_OP_EQUAL)
			self->priv->op = GTH_TEST_OP_CONTAINS;
	}

	self->priv->negative = g_strcmp0 (dom_element_get_attribute (element, "negative"), "true") == 0;

	gth_test_category_set_category (self, NULL);
	value = dom_element_get_attribute (element, "value");
	if (value != NULL)
		gth_test_category_set_category (self, value);
}


static gboolean
gth_test_category_real_update_from_control (GthTest  *base,
			                    GError  **error)
{
	GthTestCategory *self;
	GthOpData        op_data;

	self = GTH_TEST_CATEGORY (base);

	op_data = category_op_data[gtk_combo_box_get_active (GTK_COMBO_BOX (self->priv->op_combo_box))];
	self->priv->op = op_data.op;
	self->priv->negative = op_data.negative;

	gth_test_category_set_category (self, gtk_entry_get_text (GTK_ENTRY (self->priv->text_entry)));
	if (g_strcmp0 (self->priv->category, "") == 0) {
		if (error != NULL)
			*error = g_error_new (GTH_TEST_ERROR, 0, _("The test definition is incomplete"));
		return FALSE;
	}

	return TRUE;
}


static void
gth_test_category_real_focus_control (GthTest *base)
{
	GthTestCategory *self;

	self = GTH_TEST_CATEGORY (base);
	gtk_widget_grab_focus (self->priv->text_entry);
}


static GObject *
gth_test_category_real_duplicate (GthDuplicable *duplicable)
{
	GthTestCategory *test = GTH_TEST_CATEGORY (duplicable);
	GthTestCategory *new_test;

	new_test = g_object_new (GTH_TYPE_TEST_CATEGORY,
				 "id", gth_test_get_id (GTH_TEST (test)),
				 "attributes", gth_test_get_attributes (GTH_TEST (test)),
				 "display-name", gth_test_get_display_name (GTH_TEST (test)),
				 "visible", gth_test_is_visible (GTH_TEST (test)),
				 NULL);
	new_test->priv->op = test->priv->op;
	new_test->priv->negative = test->priv->negative;
	gth_test_category_set_category (new_test, test->priv->category);

	return (GObject *) new_test;
}


static void
gth_test_category_class_init (GthTestCategoryClass *class)
{
	GObjectClass *object_class;
	GthTestClass *test_class;

	object_class = (GObjectClass*) class;
	object_class->finalize = gth_test_category_finalize;

	test_class = (GthTestClass *) class;
	test_class->create_control = gth_test_category_real_create_control;
	test_class->update_from_control = gth_test_category_real_update_from_control;
	test_class->focus_control = gth_test_category_real_focus_control;
	test_class->match = gth_test_category_real_match;
}


static void
gth_test_category_dom_domizable_interface_init (DomDomizableInterface * iface)
{
	dom_domizable_parent_iface = g_type_interface_peek_parent (iface);
	iface->create_element = gth_test_category_real_create_element;
	iface->load_from_element = gth_test_category_real_load_from_element;
}


static void
gth_test_category_gth_duplicable_interface_init (GthDuplicableInterface *iface)
{
	gth_duplicable_parent_iface = g_type_interface_peek_parent (iface);
	iface->duplicate = gth_test_category_real_duplicate;
}


static void
gth_test_category_init (GthTestCategory *test)
{
	test->priv = gth_test_category_get_instance_private (test);
	test->priv->category = NULL;
	test->priv->op = GTH_TEST_OP_NONE;
	test->priv->negative = FALSE;
	test->priv->has_focus = FALSE;
	test->priv->tag_store = NULL;
	test->priv->combo_entry = NULL;
	test->priv->text_entry = NULL;
	test->priv->op_combo_box = NULL;
	test->priv->monitor_events = 0;
}


void
gth_test_category_set (GthTestCategory *self,
		       GthTestOp        op,
		       gboolean         negative,
		       const char      *value)
{
	self->priv->op = op;
	self->priv->negative = negative;
	gth_test_category_set_category (self, value);
}
