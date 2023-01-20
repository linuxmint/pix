/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2017 Free Software Foundation, Inc.
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
#include <math.h>
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
#include "gth-test-aspect-ratio.h"
#include "gtk-utils.h"


#define RATIO_EQUAL(a, b) (fabs((a) - (b)) < 0.005)


static struct {
	const char *name;
	double      value;
} aspect_ratio_values[] = {
	{ N_("Square"), 1.0 },
	{ N_("5∶4"), 5.0 / 4.0 },
	{ N_("4∶3 (DVD, Book)"), 4.0 / 3.0 },
	{ N_("7∶5"), 7.0 / 5.0 },
	{ N_("3∶2 (Postcard)"), 3.0 / 2.0 },
	{ N_("16∶10"), 16.0 / 10.0 },
	{ N_("16∶9 (DVD)"), 16.0 / 9.0 },
	{ N_("1.85∶1"), 1.85 },
	{ N_("2.39∶1"), 2.39 }
};


typedef struct {
	char      *name;
	GthTestOp  op;
	gboolean   negative;
} GthOpData;


GthOpData aspect_ratio_op_data[] = {
	{ N_("is lower than"), GTH_TEST_OP_LOWER, FALSE },
	{ N_("is greater than"), GTH_TEST_OP_GREATER, FALSE },
	{ N_("is equal to"), GTH_TEST_OP_EQUAL, FALSE },
	{ N_("is greater than or equal to"), GTH_TEST_OP_LOWER, TRUE },
	{ N_("is lower than or equal to"), GTH_TEST_OP_GREATER, TRUE }
};


struct _GthTestAspectRatioPrivate {
	GthTestOp     op;
	gboolean      negative;
	double        aspect_ratio;
	int           custom_ratio_idx;
	GtkWidget    *op_combo_box;
	GtkWidget    *ratio_combobox;
	GtkWidget    *ratio_spinbutton;
	GtkWidget    *custom_ratio_box;
};


static DomDomizableInterface *dom_domizable_parent_iface = NULL;
static GthDuplicableInterface *gth_duplicable_parent_iface = NULL;


static void gth_test_aspect_ratio_dom_domizable_interface_init (DomDomizableInterface * iface);
static void gth_test_aspect_ratio_gth_duplicable_interface_init (GthDuplicableInterface *iface);


G_DEFINE_TYPE_WITH_CODE (GthTestAspectRatio,
			 gth_test_aspect_ratio,
			 GTH_TYPE_TEST,
			 G_ADD_PRIVATE (GthTestAspectRatio)
			 G_IMPLEMENT_INTERFACE (DOM_TYPE_DOMIZABLE,
						gth_test_aspect_ratio_dom_domizable_interface_init)
			 G_IMPLEMENT_INTERFACE (GTH_TYPE_DUPLICABLE,
						gth_test_aspect_ratio_gth_duplicable_interface_init))


static void
update_visibility (GthTestAspectRatio *self)
{
	if (gtk_combo_box_get_active (GTK_COMBO_BOX (self->priv->ratio_combobox)) == self->priv->custom_ratio_idx) {
		gtk_widget_show (self->priv->custom_ratio_box);
		gtk_widget_hide (self->priv->ratio_combobox);
	}
	else {
		gtk_widget_hide (self->priv->custom_ratio_box);
		gtk_widget_show (self->priv->ratio_combobox);
	}
}


static void
combo_box_changed_cb (GtkComboBox        *widget,
		      GthTestAspectRatio *self)
{
	gth_test_update_from_control (GTH_TEST (self), NULL);
	gth_test_changed (GTH_TEST (self));
	update_visibility (self);
}


static void
spinbutton_changed_cb (GtkSpinButton      *spinbutton,
		       GthTestAspectRatio *self)
{
	gth_test_update_from_control (GTH_TEST (self), NULL);
	gth_test_changed (GTH_TEST (self));
}


static void
reset_button_clicked_cb (GtkButton          *button,
			 GthTestAspectRatio *self)
{
	gtk_combo_box_set_active (GTK_COMBO_BOX (self->priv->ratio_combobox), 0);
}


static GtkWidget *
gth_test_aspect_ratio_real_create_control (GthTest *base)
{
	GthTestAspectRatio *self;
	GtkWidget          *control;
	GtkWidget          *reset_button;
	int                 i, active_idx, current_idx;

	self = (GthTestAspectRatio *) base;

	control = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);

	/* operation combo box */

	self->priv->op_combo_box = gtk_combo_box_text_new ();
	gtk_widget_show (self->priv->op_combo_box);

	active_idx = 0;
	for (i = 0; i < G_N_ELEMENTS (aspect_ratio_op_data); i++) {
		gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (self->priv->op_combo_box),
						_(aspect_ratio_op_data[i].name));
		if ((aspect_ratio_op_data[i].op == self->priv->op) && (aspect_ratio_op_data[i].negative == self->priv->negative))
			active_idx = i;
	}
	gtk_combo_box_set_active (GTK_COMBO_BOX (self->priv->op_combo_box), active_idx);

	g_signal_connect (G_OBJECT (self->priv->op_combo_box),
			  "changed",
			  G_CALLBACK (combo_box_changed_cb),
			  self);

	/* aspect ratio list combo box */

	self->priv->ratio_combobox = gtk_combo_box_text_new ();
	gtk_widget_show (self->priv->ratio_combobox);

	current_idx = -1;
	active_idx = -1;

	for (i = 0; i < G_N_ELEMENTS (aspect_ratio_values); i++) {
		gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (self->priv->ratio_combobox),
						_(aspect_ratio_values[i].name));
		current_idx++;
		if ((active_idx == -1) && RATIO_EQUAL (aspect_ratio_values[i].value, self->priv->aspect_ratio))
			active_idx = current_idx;
	}

	gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (self->priv->ratio_combobox), _("Personalize…"));
	current_idx++;
	self->priv->custom_ratio_idx = current_idx;

	if (active_idx == -1)
		active_idx = self->priv->custom_ratio_idx;

	gtk_combo_box_set_active (GTK_COMBO_BOX (self->priv->ratio_combobox), active_idx);

	g_signal_connect (G_OBJECT (self->priv->ratio_combobox),
			  "changed",
			  G_CALLBACK (combo_box_changed_cb),
			  self);

	/* custom ratio widgets */

	self->priv->custom_ratio_box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
	gtk_widget_show (self->priv->custom_ratio_box);

	self->priv->ratio_spinbutton = gtk_spin_button_new_with_range (0, 10.0, 0.01);
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (self->priv->ratio_spinbutton), self->priv->aspect_ratio);
	gtk_widget_show (self->priv->ratio_spinbutton);

	reset_button = gtk_button_new_from_icon_name ("edit-undo-symbolic", GTK_ICON_SIZE_BUTTON);
	gtk_button_set_relief (GTK_BUTTON (reset_button), GTK_RELIEF_NONE);
	gtk_widget_set_tooltip_text (reset_button, _("Reset"));
	gtk_widget_show (reset_button);

	g_signal_connect (G_OBJECT (reset_button),
			  "clicked",
			  G_CALLBACK (reset_button_clicked_cb),
			  self);

	gtk_box_pack_start (GTK_BOX (self->priv->custom_ratio_box), self->priv->ratio_spinbutton, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (self->priv->custom_ratio_box), reset_button, FALSE, FALSE, 0);

	g_signal_connect (G_OBJECT (self->priv->ratio_spinbutton),
			  "value-changed",
			  G_CALLBACK (spinbutton_changed_cb),
			  self);

	/**/

	gtk_box_pack_start (GTK_BOX (control), self->priv->op_combo_box, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (control), self->priv->ratio_combobox, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (control), self->priv->custom_ratio_box, FALSE, FALSE, 0);
	update_visibility (self);

	return control;
}


static GthMatch
gth_test_aspect_ratio_real_match (GthTest     *test,
			          GthFileData *file)
{
	GthTestAspectRatio *self;
	int                 width, height;
	double              ratio;
	gboolean            result = FALSE;

        self = GTH_TEST_ASPECT_RATIO (test);

	width = g_file_info_get_attribute_int32 (file->info, "frame::width");
	height = g_file_info_get_attribute_int32 (file->info, "frame::height");
	ratio = (height > 0) ? (double) width / height : 0;

	switch (self->priv->op) {
	case GTH_TEST_OP_EQUAL:
		result = RATIO_EQUAL (ratio, self->priv->aspect_ratio);
		break;

	case GTH_TEST_OP_LOWER:
		result = (ratio < self->priv->aspect_ratio);
		break;

	case GTH_TEST_OP_GREATER:
		result = (ratio > self->priv->aspect_ratio);
		break;

	default:
		break;
	}

        if (self->priv->negative)
		result = ! result;

	return result ? GTH_MATCH_YES : GTH_MATCH_NO;
}


static DomElement*
gth_test_aspect_ratio_real_create_element (DomDomizable *base,
					   DomDocument  *doc)
{
	GthTestAspectRatio *self;
	DomElement         *element;
	char               *value;

	g_return_val_if_fail (DOM_IS_DOCUMENT (doc), NULL);

	self = GTH_TEST_ASPECT_RATIO (base);

	element = dom_document_create_element (doc, "test",
					       "id", gth_test_get_id (GTH_TEST (self)),
					       NULL);

	if (! gth_test_is_visible (GTH_TEST (self)))
		dom_element_set_attribute (element, "display", "none");

	dom_element_set_attribute (element, "op", _g_enum_type_get_value (GTH_TYPE_TEST_OP, self->priv->op)->value_nick);
	if (self->priv->negative)
		dom_element_set_attribute (element, "negative", self->priv->negative ? "true" : "false");

	value = g_strdup_printf ("%f", self->priv->aspect_ratio);
	dom_element_set_attribute (element, "value", value);
	g_free (value);

	return element;
}


static void
gth_test_aspect_ratio_set_aspect_ratio (GthTestAspectRatio *self,
					double              value)
{
	self->priv->aspect_ratio = value;
}


static void
gth_test_aspect_ratio_real_load_from_element (DomDomizable *base,
					      DomElement   *element)
{
	GthTestAspectRatio *self;
	const char         *value;

	g_return_if_fail (DOM_IS_ELEMENT (element));

	self = GTH_TEST_ASPECT_RATIO (base);

	g_object_set (self, "visible", (g_strcmp0 (dom_element_get_attribute (element, "display"), "none") != 0), NULL);

	value = dom_element_get_attribute (element, "op");
	if (value != NULL)
		self->priv->op = _g_enum_type_get_value_by_nick (GTH_TYPE_TEST_OP, value)->value;

	self->priv->negative = g_strcmp0 (dom_element_get_attribute (element, "negative"), "true") == 0;

	value = dom_element_get_attribute (element, "value");
	gth_test_aspect_ratio_set_aspect_ratio (self, (value != NULL) ? atof (value) : 0.0);
}


static gboolean
gth_test_aspect_ratio_real_update_from_control (GthTest  *base,
						GError  **error)
{
	GthTestAspectRatio *self;
	GthOpData           op_data;
	int                 active_idx;

	self = GTH_TEST_ASPECT_RATIO (base);

	op_data = aspect_ratio_op_data[gtk_combo_box_get_active (GTK_COMBO_BOX (self->priv->op_combo_box))];
	self->priv->op = op_data.op;
	self->priv->negative = op_data.negative;

	active_idx = gtk_combo_box_get_active (GTK_COMBO_BOX (self->priv->ratio_combobox));
	if (active_idx == self->priv->custom_ratio_idx)
		self->priv->aspect_ratio = gtk_spin_button_get_value (GTK_SPIN_BUTTON (self->priv->ratio_spinbutton));
	else
		self->priv->aspect_ratio = aspect_ratio_values[active_idx].value;

	if (self->priv->aspect_ratio == 0) {
		if (error != NULL)
			*error = g_error_new (GTH_TEST_ERROR, 0, _("The test definition is incomplete"));
		return FALSE;
	}

	return TRUE;
}


static void
gth_test_aspect_ratio_real_focus_control (GthTest *base)
{
	GthTestAspectRatio *self;

	self = GTH_TEST_ASPECT_RATIO (base);
	if (gtk_combo_box_get_active (GTK_COMBO_BOX (self->priv->ratio_combobox)) == self->priv->custom_ratio_idx)
		gtk_widget_grab_focus (self->priv->ratio_spinbutton);
	else
		gtk_widget_grab_focus (self->priv->ratio_combobox);
}


static GObject *
gth_test_aspect_ratio_real_duplicate (GthDuplicable *duplicable)
{
	GthTestAspectRatio *test = GTH_TEST_ASPECT_RATIO (duplicable);
	GthTestAspectRatio *new_test;

	new_test = g_object_new (GTH_TYPE_TEST_ASPECT_RATIO,
				 "id", gth_test_get_id (GTH_TEST (test)),
				 "attributes", gth_test_get_attributes (GTH_TEST (test)),
				 "display-name", gth_test_get_display_name (GTH_TEST (test)),
				 "visible", gth_test_is_visible (GTH_TEST (test)),
				 NULL);
	new_test->priv->op = test->priv->op;
	new_test->priv->negative = test->priv->negative;
	gth_test_aspect_ratio_set_aspect_ratio (new_test, test->priv->aspect_ratio);

	return (GObject *) new_test;
}


static void
gth_test_aspect_ratio_class_init (GthTestAspectRatioClass *class)
{
	GthTestClass *test_class;

	test_class = (GthTestClass *) class;
	test_class->create_control = gth_test_aspect_ratio_real_create_control;
	test_class->update_from_control = gth_test_aspect_ratio_real_update_from_control;
	test_class->focus_control = gth_test_aspect_ratio_real_focus_control;
	test_class->match = gth_test_aspect_ratio_real_match;
}


static void
gth_test_aspect_ratio_dom_domizable_interface_init (DomDomizableInterface * iface)
{
	dom_domizable_parent_iface = g_type_interface_peek_parent (iface);
	iface->create_element = gth_test_aspect_ratio_real_create_element;
	iface->load_from_element = gth_test_aspect_ratio_real_load_from_element;
}


static void
gth_test_aspect_ratio_gth_duplicable_interface_init (GthDuplicableInterface *iface)
{
	gth_duplicable_parent_iface = g_type_interface_peek_parent (iface);
	iface->duplicate = gth_test_aspect_ratio_real_duplicate;
}


static void
gth_test_aspect_ratio_init (GthTestAspectRatio *self)
{
	self->priv = gth_test_aspect_ratio_get_instance_private (self);
	self->priv->op = GTH_TEST_OP_EQUAL;
	self->priv->negative = FALSE;
	self->priv->aspect_ratio = 1.0;
	self->priv->custom_ratio_idx = 0;
	self->priv->op_combo_box = NULL;
	self->priv->ratio_combobox = NULL;
	self->priv->ratio_spinbutton = NULL;
	self->priv->custom_ratio_box = NULL;
}
