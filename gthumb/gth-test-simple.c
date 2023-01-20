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

#include <math.h>
#include <config.h>
#include <stdlib.h>
#include <string.h>
#include <glib/gi18n.h>
#include <glib.h>
#include <gtk/gtk.h>
#include "dom.h"
#include "glib-utils.h"
#include "gth-enum-types.h"
#include "gth-duplicable.h"
#include "gth-test.h"
#include "gth-test-simple.h"
#include "gth-time.h"
#include "gth-time-selector.h"


typedef struct {
	char      *name;
	GthTestOp  op;
	gboolean   negative;
} GthOpData;


GthOpData text_op_data[] = {
	{ N_("contains"), GTH_TEST_OP_CONTAINS, FALSE },
	{ N_("starts with"), GTH_TEST_OP_STARTS_WITH, FALSE },
	{ N_("ends with"), GTH_TEST_OP_ENDS_WITH, FALSE },
	{ N_("is"), GTH_TEST_OP_EQUAL, FALSE },
	{ N_("is not"), GTH_TEST_OP_EQUAL, TRUE },
	{ N_("does not contain"), GTH_TEST_OP_CONTAINS, TRUE },
	{ N_("matches"), GTH_TEST_OP_MATCHES, FALSE }
};

GthOpData int_op_data[] = {
	{ N_("is lower than"), GTH_TEST_OP_LOWER, FALSE },
	{ N_("is greater than"), GTH_TEST_OP_GREATER, FALSE },
	{ N_("is equal to"), GTH_TEST_OP_EQUAL, FALSE },
	{ N_("is greater than or equal to"), GTH_TEST_OP_LOWER, TRUE },
	{ N_("is lower than or equal to"), GTH_TEST_OP_GREATER, TRUE }
};

GthOpData date_op_data[] = {
	{ N_("is before"), GTH_TEST_OP_BEFORE, FALSE },
	{ N_("is after"), GTH_TEST_OP_AFTER, FALSE },
	{ N_("is"), GTH_TEST_OP_EQUAL, FALSE },
	{ N_("is not"), GTH_TEST_OP_EQUAL, TRUE }
};

typedef struct {
	char    *name;
	goffset  size;
} GthSizeData;


static GthSizeData size_data[] = {
	{ N_("kB"), 1024 },
	{ N_("MB"), 1024*1024 },
	{ N_("GB"), 1024*1024*1024 }
};


enum {
	PROP_0,
	PROP_DATA_TYPE,
	PROP_DATA_AS_STRING,
	PROP_DATA_AS_INT,
	PROP_DATA_AS_SIZE,
	PROP_DATA_AS_DOUBLE,
	PROP_DATA_AS_DATE,
	PROP_GET_DATA,
	PROP_OP,
	PROP_NEGATIVE,
	PROP_MAX_INT,
	PROP_MAX_DOUBLE
};


struct _GthTestSimplePrivate {
	GthTestDataType  data_type;
	union {
		char    *s;
		int      i;
		guint64  size;
		GDate   *date;
		gdouble  f;
	} data;
	GthTestGetData   get_data;
	GthTestOp        op;
	gboolean         negative;
	gint64           max_int;
	double           max_double;
	GPatternSpec    *pattern;
	gboolean         has_focus;
	GtkWidget       *text_entry;
	GtkWidget       *text_op_combo_box;
	GtkWidget       *size_op_combo_box;
	GtkWidget       *date_op_combo_box;
	GtkWidget       *size_combo_box;
	GtkWidget       *time_selector;
	GtkWidget       *spinbutton;
};


static DomDomizableInterface *dom_domizable_parent_iface = NULL;
static GthDuplicableInterface *gth_duplicable_parent_iface = NULL;


static void gth_test_simple_dom_domizable_interface_init (DomDomizableInterface *iface);
static void gth_test_simple_gth_duplicable_interface_init (GthDuplicableInterface *iface);


G_DEFINE_TYPE_WITH_CODE (GthTestSimple,
			 gth_test_simple,
			 GTH_TYPE_TEST,
			 G_ADD_PRIVATE (GthTestSimple)
			 G_IMPLEMENT_INTERFACE (DOM_TYPE_DOMIZABLE,
						gth_test_simple_dom_domizable_interface_init)
			 G_IMPLEMENT_INTERFACE (GTH_TYPE_DUPLICABLE,
						gth_test_simple_gth_duplicable_interface_init))


static void
_gth_test_simple_free_data (GthTestSimple *test)
{
	switch (test->priv->data_type) {
	case GTH_TEST_DATA_TYPE_STRING:
		g_free (test->priv->data.s);
		test->priv->data.s = NULL;
		break;

	case GTH_TEST_DATA_TYPE_DATE:
		if (test->priv->data.date != NULL)
			g_date_free (test->priv->data.date);
		test->priv->data.date = NULL;
		break;

	default:
		break;
	}

	if (test->priv->pattern != NULL) {
		g_pattern_spec_free (test->priv->pattern);
		test->priv->pattern = NULL;
	}
}


static void
gth_test_simple_finalize (GObject *object)
{
	GthTestSimple *test;

	test = GTH_TEST_SIMPLE (object);

	_gth_test_simple_free_data (test);
	if (test->priv->pattern != NULL)
		g_pattern_spec_free (test->priv->pattern);

	G_OBJECT_CLASS (gth_test_simple_parent_class)->finalize (object);
}


static gboolean
text_entry_focus_in_event_cb (GtkEntry      *entry,
			      GdkEventFocus *event,
                              GthTestSimple *test)
{
	test->priv->has_focus = TRUE;
	return FALSE;
}


static gboolean
text_entry_focus_out_event_cb (GtkEntry      *entry,
			       GdkEventFocus *event,
                               GthTestSimple *test)
{
	test->priv->has_focus = FALSE;
	return FALSE;
}


static void
size_text_entry_activate_cb (GtkEntry      *entry,
                             GthTestSimple *test)
{
	gth_test_update_from_control (GTH_TEST (test), NULL);
	gth_test_changed (GTH_TEST (test));
}


static void
size_op_combo_box_changed_cb (GtkComboBox   *combo_box,
                              GthTestSimple *test)
{
	gth_test_update_from_control (GTH_TEST (test), NULL);
	gth_test_changed (GTH_TEST (test));
}


static void
size_combo_box_changed_cb (GtkComboBox   *combo_box,
                           GthTestSimple *test)
{
	gth_test_update_from_control (GTH_TEST (test), NULL);
	gth_test_changed (GTH_TEST (test));
}


static void
spinbutton_changed_cb (GtkSpinButton *spinbutton,
                       GthTestSimple *test)
{
	gth_test_update_from_control (GTH_TEST (test), NULL);
	gth_test_changed (GTH_TEST (test));
}


static void
focus_event_cb (GtkWidget     *widget,
                GdkEvent      *event,
                GthTestSimple *test)
{
	gth_test_update_from_control (GTH_TEST (test), NULL);
	gth_test_changed (GTH_TEST (test));
}


static GtkWidget *
create_control_for_integer (GthTestSimple *test)
{
	GtkWidget *control;
	int        i, op_idx;

	control = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);

	/* text operation combo box */

	test->priv->text_op_combo_box = gtk_combo_box_text_new ();
	gtk_widget_show (test->priv->text_op_combo_box);

	op_idx = 0;
	for (i = 0; i < G_N_ELEMENTS (int_op_data); i++) {
		gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (test->priv->text_op_combo_box),
						_(int_op_data[i].name));
		if ((int_op_data[i].op == test->priv->op) && (int_op_data[i].negative == test->priv->negative))
			op_idx = i;
	}
	gtk_combo_box_set_active (GTK_COMBO_BOX (test->priv->text_op_combo_box), op_idx);

	g_signal_connect (G_OBJECT (test->priv->text_op_combo_box),
			  "changed",
			  G_CALLBACK (size_op_combo_box_changed_cb),
			  test);

	/* spin button */

	test->priv->spinbutton = gtk_spin_button_new_with_range (0, test->priv->max_int, 1);
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (test->priv->spinbutton), 0);
	gtk_widget_show (test->priv->spinbutton);

	g_signal_connect (G_OBJECT (test->priv->spinbutton),
			  "value-changed",
			  G_CALLBACK (spinbutton_changed_cb),
			  test);
	g_signal_connect (G_OBJECT (test->priv->spinbutton),
			  "activate",
			  G_CALLBACK (size_text_entry_activate_cb),
			  test);
	g_signal_connect (G_OBJECT (test->priv->spinbutton),
			  "focus-in-event",
			  G_CALLBACK (focus_event_cb),
			  test);
	g_signal_connect (G_OBJECT (test->priv->spinbutton),
			  "focus-out-event",
			  G_CALLBACK (focus_event_cb),
			  test);

	/**/

	gtk_box_pack_start (GTK_BOX (control), test->priv->text_op_combo_box, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (control), test->priv->spinbutton, FALSE, FALSE, 0);

	return control;
}

static GtkWidget *
create_control_for_double (GthTestSimple *test)
{
	GtkWidget *control;
	int        i, op_idx;

	control = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);

	/* text operation combo box */

	test->priv->text_op_combo_box = gtk_combo_box_text_new ();
	gtk_widget_show (test->priv->text_op_combo_box);

	op_idx = 0;
	for (i = 0; i < G_N_ELEMENTS (int_op_data); i++) {
		gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (test->priv->text_op_combo_box),
						_(int_op_data[i].name));
		if ((int_op_data[i].op == test->priv->op) && (int_op_data[i].negative == test->priv->negative))
			op_idx = i;
	}
	gtk_combo_box_set_active (GTK_COMBO_BOX (test->priv->text_op_combo_box), op_idx);

	g_signal_connect (G_OBJECT (test->priv->text_op_combo_box),
			  "changed",
			  G_CALLBACK (size_op_combo_box_changed_cb),
			  test);

	/* spin button */

	test->priv->spinbutton = gtk_spin_button_new_with_range (0, test->priv->max_double, 0.01);
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (test->priv->spinbutton), test->priv->data.f);
	gtk_widget_show (test->priv->spinbutton);

	g_signal_connect (G_OBJECT (test->priv->spinbutton),
			  "value-changed",
			  G_CALLBACK (spinbutton_changed_cb),
			  test);

	/**/

	gtk_box_pack_start (GTK_BOX (control), test->priv->text_op_combo_box, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (control), test->priv->spinbutton, FALSE, FALSE, 0);

	return control;
}

static GtkWidget *
create_control_for_size (GthTestSimple *test)
{
	GtkWidget *control;
	int        i, op_idx, size_idx;
	gboolean   size_set = FALSE;

	control = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);

	/* text operation combo box */

	test->priv->size_op_combo_box = gtk_combo_box_text_new ();
	gtk_widget_show (test->priv->size_op_combo_box);

	op_idx = 0;
	for (i = 0; i < G_N_ELEMENTS (int_op_data); i++) {
		gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (test->priv->size_op_combo_box),
						_(int_op_data[i].name));
		if ((int_op_data[i].op == test->priv->op) && (int_op_data[i].negative == test->priv->negative))
			op_idx = i;
	}
	gtk_combo_box_set_active (GTK_COMBO_BOX (test->priv->size_op_combo_box), op_idx);

	g_signal_connect (G_OBJECT (test->priv->size_op_combo_box),
			  "changed",
			  G_CALLBACK (size_op_combo_box_changed_cb),
			  test);

	/* text entry */

	test->priv->text_entry = gtk_entry_new ();
	gtk_entry_set_width_chars (GTK_ENTRY (test->priv->text_entry), 6);
	gtk_widget_show (test->priv->text_entry);

	g_signal_connect (G_OBJECT (test->priv->text_entry),
			  "activate",
			  G_CALLBACK (size_text_entry_activate_cb),
			  test);
	g_signal_connect (G_OBJECT (test->priv->text_entry),
			  "focus-in-event",
			  G_CALLBACK (text_entry_focus_in_event_cb),
			  test);
	g_signal_connect (G_OBJECT (test->priv->text_entry),
			  "focus-out-event",
			  G_CALLBACK (text_entry_focus_out_event_cb),
			  test);

	/* size combo box */

	test->priv->size_combo_box = gtk_combo_box_text_new ();
	gtk_widget_show (test->priv->size_combo_box);

	size_idx = 0;
	for (i = 0; i < G_N_ELEMENTS (size_data); i++) {
		gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (test->priv->size_combo_box),
						_(size_data[i].name));
		if (! size_set && ((i == G_N_ELEMENTS (size_data) - 1) || (test->priv->data.size < size_data[i + 1].size))) {
			char *value;

			size_idx = i;
			value = g_strdup_printf ("%.2f", (double) test->priv->data.size / size_data[i].size);
			gtk_entry_set_text (GTK_ENTRY (test->priv->text_entry), value);
			g_free (value);
			size_set = TRUE;
		}
	}
	gtk_combo_box_set_active (GTK_COMBO_BOX (test->priv->size_combo_box), size_idx);

	g_signal_connect (G_OBJECT (test->priv->size_combo_box),
			  "changed",
			  G_CALLBACK (size_combo_box_changed_cb),
			  test);

	/**/

	gtk_box_pack_start (GTK_BOX (control), test->priv->size_op_combo_box, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (control), test->priv->text_entry, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (control), test->priv->size_combo_box, FALSE, FALSE, 0);

	return control;
}


static void
string_text_entry_activate_cb (GtkEntry      *entry,
                               GthTestSimple *test)
{
	gth_test_update_from_control (GTH_TEST (test), NULL);
	gth_test_changed (GTH_TEST (test));
}


static void
text_op_combo_box_changed_cb (GtkComboBox   *combo_box,
                              GthTestSimple *test)
{
	gth_test_update_from_control (GTH_TEST (test), NULL);
	gth_test_changed (GTH_TEST (test));
}


static GtkWidget *
create_control_for_string (GthTestSimple *test)
{
	GtkWidget *control;
	int        i, op_idx;

	control = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);

	/* text operation combo box */

	test->priv->text_op_combo_box = gtk_combo_box_text_new ();
	gtk_widget_show (test->priv->text_op_combo_box);

	op_idx = 0;
	for (i = 0; i < G_N_ELEMENTS (text_op_data); i++) {
		gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (test->priv->text_op_combo_box),
						_(text_op_data[i].name));
		if ((text_op_data[i].op == test->priv->op) && (text_op_data[i].negative == test->priv->negative))
			op_idx = i;
	}
	gtk_combo_box_set_active (GTK_COMBO_BOX (test->priv->text_op_combo_box), op_idx);

	g_signal_connect (G_OBJECT (test->priv->text_op_combo_box),
			  "changed",
			  G_CALLBACK (text_op_combo_box_changed_cb),
			  test);

	/* text entry */

	test->priv->text_entry = gtk_entry_new ();
	if (test->priv->data.s != NULL)
		gtk_entry_set_text (GTK_ENTRY (test->priv->text_entry), test->priv->data.s);
	gtk_widget_show (test->priv->text_entry);

	g_signal_connect (G_OBJECT (test->priv->text_entry),
			  "activate",
			  G_CALLBACK (string_text_entry_activate_cb),
			  test);
	g_signal_connect (G_OBJECT (test->priv->text_entry),
			  "focus-in-event",
			  G_CALLBACK (text_entry_focus_in_event_cb),
			  test);
	g_signal_connect (G_OBJECT (test->priv->text_entry),
			  "focus-out-event",
			  G_CALLBACK (text_entry_focus_out_event_cb),
			  test);
	gtk_widget_set_size_request (test->priv->text_entry, 150, -1);

	/**/

	gtk_box_pack_start (GTK_BOX (control), test->priv->text_op_combo_box, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (control), test->priv->text_entry, FALSE, FALSE, 0);

	return control;
}


static void
date_op_combo_box_changed_cb (GtkComboBox   *combo_box,
			      GthTestSimple *test)
{
	gth_test_update_from_control (GTH_TEST (test), NULL);
	gth_test_changed (GTH_TEST (test));
}


static void
time_selector_changed_cb (GthTimeSelector *selctor,
			  GthTestSimple   *test)
{
	gth_test_update_from_control (GTH_TEST (test), NULL);
	gth_test_changed (GTH_TEST (test));
}


static GtkWidget *
create_control_for_date (GthTestSimple *test)
{
	GtkWidget *control;
	int        i, op_idx;

	control = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);

	/* date operation combo box */

	test->priv->date_op_combo_box = gtk_combo_box_text_new ();
	gtk_widget_show (test->priv->date_op_combo_box);

	op_idx = 0;
	for (i = 0; i < G_N_ELEMENTS (date_op_data); i++) {
		gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (test->priv->date_op_combo_box),
						_(date_op_data[i].name));
		if ((date_op_data[i].op == test->priv->op) && (date_op_data[i].negative == test->priv->negative))
			op_idx = i;
	}
	gtk_combo_box_set_active (GTK_COMBO_BOX (test->priv->date_op_combo_box), op_idx);

	g_signal_connect (G_OBJECT (test->priv->date_op_combo_box),
			  "changed",
			  G_CALLBACK (date_op_combo_box_changed_cb),
			  test);

	/* date selector */

	test->priv->time_selector = gth_time_selector_new ();
	gth_time_selector_show_time (GTH_TIME_SELECTOR (test->priv->time_selector), FALSE, FALSE);
	gtk_widget_show (test->priv->time_selector);

	if (test->priv->data.date != NULL) {
		GthDateTime *dt;

		dt = gth_datetime_new ();
		gth_datetime_from_gdate (dt, test->priv->data.date);
		gth_time_selector_set_value (GTH_TIME_SELECTOR (test->priv->time_selector), dt);

		gth_datetime_free (dt);
	}

	g_signal_connect (G_OBJECT (test->priv->time_selector),
			  "changed",
			  G_CALLBACK (time_selector_changed_cb),
			  test);

	/**/

	gtk_box_pack_start (GTK_BOX (control), test->priv->date_op_combo_box, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (control), test->priv->time_selector, FALSE, FALSE, 0);

	return control;
}


static GtkWidget *
gth_test_simple_real_create_control (GthTest *test)
{
	GthTestSimple *test_simple = (GthTestSimple *) test;
	GtkWidget     *control = NULL;

	switch (test_simple->priv->data_type) {
	case GTH_TEST_DATA_TYPE_NONE:
		control = NULL;
		break;

	case GTH_TEST_DATA_TYPE_INT:
		control = create_control_for_integer (GTH_TEST_SIMPLE (test));
		break;

	case GTH_TEST_DATA_TYPE_SIZE:
		control = create_control_for_size (GTH_TEST_SIMPLE (test));
		break;

	case GTH_TEST_DATA_TYPE_STRING:
		control = create_control_for_string (GTH_TEST_SIMPLE (test));
		break;

	case GTH_TEST_DATA_TYPE_DATE:
		control = create_control_for_date (GTH_TEST_SIMPLE (test));
		break;

	case GTH_TEST_DATA_TYPE_DOUBLE:
		control = create_control_for_double (GTH_TEST_SIMPLE (test));
		break;
	}

	return control;
}


static gboolean
test_string (GthTestSimple *test,
	     char          *value)
{
	gboolean  result = FALSE;
	char     *value1;
	char     *value2;

	if ((test->priv->data.s == NULL) || (value == NULL))
		return FALSE;

	value1 = g_utf8_casefold (test->priv->data.s, -1);
	value2 = g_utf8_casefold (value, -1);

	switch (test->priv->op) {
	case GTH_TEST_OP_EQUAL:
		result = g_utf8_collate (value2, value1) == 0;
		break;

	case GTH_TEST_OP_LOWER:
		result = g_utf8_collate (value2, value1) < 0;
		break;

	case GTH_TEST_OP_GREATER:
		result = g_utf8_collate (value2, value1) > 0;
		break;

	case GTH_TEST_OP_CONTAINS:
		result = g_strstr_len (value2, -1, value1) != NULL;
		break;

	case GTH_TEST_OP_STARTS_WITH:
		result = g_str_has_prefix (value2, value1);
		break;

	case GTH_TEST_OP_ENDS_WITH:
		result = g_str_has_suffix (value2, value1);
		break;

	case GTH_TEST_OP_MATCHES:
		if (test->priv->pattern == NULL)
			test->priv->pattern = g_pattern_spec_new (test->priv->data.s);
		result = g_pattern_match_string (test->priv->pattern, value2);
		break;

	default:
		break;
	}

	g_free (value2);
	g_free (value1);

	return result;
}


static gboolean
test_integer (GthTestSimple *test,
              int            value)
{
	gboolean result = FALSE;

	switch (test->priv->op) {
	case GTH_TEST_OP_EQUAL:
		result = (value == test->priv->data.i);
		break;

	case GTH_TEST_OP_LOWER:
		result = (value < test->priv->data.i);
		break;

	case GTH_TEST_OP_GREATER:
		result = (value > test->priv->data.i);
		break;

	default:
		break;
	}

	return result;
}


static gboolean
test_size (GthTestSimple *test,
	   guint64        value)
{
	gboolean result = FALSE;

	switch (test->priv->op) {
	case GTH_TEST_OP_EQUAL:
		result = (value == test->priv->data.size);
		break;

	case GTH_TEST_OP_LOWER:
		result = (value < test->priv->data.size);
		break;

	case GTH_TEST_OP_GREATER:
		result = (value > test->priv->data.size);
		break;

	default:
		break;
	}

	return result;
}


static gboolean
test_double (GthTestSimple *test,
             gdouble        value)
{
	gboolean result = FALSE;

	switch (test->priv->op) {
	case GTH_TEST_OP_EQUAL:
		result = FLOAT_EQUAL (value, test->priv->data.f);
		break;

	case GTH_TEST_OP_LOWER:
		result = (value < test->priv->data.f);
		break;

	case GTH_TEST_OP_GREATER:
		result = (value > test->priv->data.f);
		break;

	default:
		break;
	}
	return result;
}


static gboolean
test_date (GthTestSimple *test,
           GDate         *date)
{
	gboolean result = FALSE;
	int       compare;

	if (! g_date_valid (date) || ! g_date_valid (test->priv->data.date))
		return FALSE;

	compare = g_date_compare (date, test->priv->data.date);

	switch (test->priv->op) {
	case GTH_TEST_OP_EQUAL:
		result = (compare == 0);
		break;

	case GTH_TEST_OP_BEFORE:
		result = (compare < 0);
		break;

	case GTH_TEST_OP_AFTER:
		result = (compare > 0);
		break;
	default:
		break;
	}

	return result;
}


static gpointer
_gth_test_simple_get_pointer (GthTestSimple  *test,
			      GthFileData    *file,
			      GDestroyNotify *data_destroy_func)
{
	gpointer value;

	test->priv->get_data (GTH_TEST (test), file, &value, data_destroy_func);

	return value;
}


static gint64
_gth_test_simple_get_int (GthTestSimple *test,
			  GthFileData   *file)
{
        return test->priv->get_data (GTH_TEST (test), file, NULL, NULL);
}


static guint64
_gth_test_simple_get_size (GthTestSimple *test,
			   GthFileData   *file)
{
	guint64 value;

	test->priv->get_data (GTH_TEST (test), file, (gpointer)&value, NULL);

	return value;
}


static gdouble
_gth_test_simple_get_double (GthTestSimple *test,
			     GthFileData   *file)
{
	gdouble value;

	test->priv->get_data (GTH_TEST (test), file, (gpointer)&value, NULL);

	return value;
}


static GthMatch
gth_test_simple_real_match (GthTest   *test,
			    GthFileData   *file)
{
	GthTestSimple  *test_simple;
	gboolean        result = FALSE;
	gpointer        data;
	GDestroyNotify  data_destroy_func = NULL;

        test_simple = GTH_TEST_SIMPLE (test);

	switch (test_simple->priv->data_type) {
	case GTH_TEST_DATA_TYPE_NONE:
		result = _gth_test_simple_get_int (test_simple, file) == TRUE;
		break;

	case GTH_TEST_DATA_TYPE_INT:
		result = test_integer (test_simple, _gth_test_simple_get_int (test_simple, file));
		break;

	case GTH_TEST_DATA_TYPE_SIZE:
		result = test_size (test_simple, _gth_test_simple_get_size (test_simple, file));
		break;

	case GTH_TEST_DATA_TYPE_DOUBLE:
		result = test_double (test_simple, _gth_test_simple_get_double (test_simple, file));
		break;

	case GTH_TEST_DATA_TYPE_STRING:
		data = _gth_test_simple_get_pointer (test_simple, file, &data_destroy_func);
		result = test_string (test_simple, data);
		if (data_destroy_func != NULL)
			data_destroy_func (data);
		break;

	case GTH_TEST_DATA_TYPE_DATE:
		data = _gth_test_simple_get_pointer (test_simple, file, &data_destroy_func);
		result = test_date (test_simple, data);
		if (data_destroy_func != NULL)
			data_destroy_func (data);
		break;
	}

	if (test_simple->priv->negative)
		result = ! result;

	return result ? GTH_MATCH_YES : GTH_MATCH_NO;
}


static DomElement*
gth_test_simple_real_create_element (DomDomizable *base,
				     DomDocument  *doc)
{
	GthTestSimple *self;
	DomElement    *element;
	char          *value;

	g_return_val_if_fail (DOM_IS_DOCUMENT (doc), NULL);

	self = GTH_TEST_SIMPLE (base);

	element = dom_document_create_element (doc, "test",
					       "id", gth_test_get_id (GTH_TEST (self)),
					       NULL);

	if (! gth_test_is_visible (GTH_TEST (self)))
		dom_element_set_attribute (element, "display", "none");

	switch (self->priv->data_type) {
	case GTH_TEST_DATA_TYPE_NONE:
		break;

	case GTH_TEST_DATA_TYPE_INT:
		dom_element_set_attribute (element, "op", _g_enum_type_get_value (GTH_TYPE_TEST_OP, self->priv->op)->value_nick);
		if (self->priv->op != GTH_TEST_OP_NONE) {
			if (self->priv->negative)
				dom_element_set_attribute (element, "negative", self->priv->negative ? "true" : "false");
			value = g_strdup_printf ("%d", self->priv->data.i);
			dom_element_set_attribute (element, "value", value);
			g_free (value);
		}
		break;

	case GTH_TEST_DATA_TYPE_SIZE:
		dom_element_set_attribute (element, "op", _g_enum_type_get_value (GTH_TYPE_TEST_OP, self->priv->op)->value_nick);
		if (self->priv->op != GTH_TEST_OP_NONE) {
			if (self->priv->negative)
				dom_element_set_attribute (element, "negative", self->priv->negative ? "true" : "false");
			value = g_strdup_printf ("%" G_GUINT64_FORMAT, self->priv->data.size);
			dom_element_set_attribute (element, "value", value);
			g_free (value);
		}
		break;

	case GTH_TEST_DATA_TYPE_DOUBLE:
		dom_element_set_attribute (element, "op", _g_enum_type_get_value (GTH_TYPE_TEST_OP, self->priv->op)->value_nick);
		if (self->priv->op != GTH_TEST_OP_NONE) {
			if (self->priv->negative)
				dom_element_set_attribute (element, "negative", self->priv->negative ? "true" : "false");
			value = g_strdup_printf ("%f", self->priv->data.f);
			dom_element_set_attribute (element, "value", value);
			g_free (value);
		}
		break;

	case GTH_TEST_DATA_TYPE_STRING:
		dom_element_set_attribute (element, "op", _g_enum_type_get_value (GTH_TYPE_TEST_OP, self->priv->op)->value_nick);
		if (self->priv->op != GTH_TEST_OP_NONE) {
			if (self->priv->negative)
				dom_element_set_attribute (element, "negative", self->priv->negative ? "true" : "false");
			if (self->priv->data.s != NULL)
				dom_element_set_attribute (element, "value", self->priv->data.s);
		}
		break;

	case GTH_TEST_DATA_TYPE_DATE:
		dom_element_set_attribute (element, "op", _g_enum_type_get_value (GTH_TYPE_TEST_OP, self->priv->op)->value_nick);
		if (self->priv->op != GTH_TEST_OP_NONE) {
			if (self->priv->negative)
				dom_element_set_attribute (element, "negative", self->priv->negative ? "true" : "false");
			if (self->priv->data.date != NULL) {
				GthDateTime *dt;
				char        *exif_date;

				dt = gth_datetime_new ();
				gth_datetime_from_gdate (dt, self->priv->data.date);
				exif_date = gth_datetime_to_exif_date (dt);
				dom_element_set_attribute (element, "value", exif_date);

				g_free (exif_date);
				gth_datetime_free (dt);
			}
		}
		break;
	}

	return element;
}


static void
gth_test_simple_real_load_from_element (DomDomizable *base,
					DomElement   *element)
{
	GthTestSimple *self;
	const char    *value;

	g_return_if_fail (DOM_IS_ELEMENT (element));

	self = GTH_TEST_SIMPLE (base);

	g_object_set (self, "visible", (g_strcmp0 (dom_element_get_attribute (element, "display"), "none") != 0), NULL);

	value = dom_element_get_attribute (element, "op");
	if (value != NULL)
		self->priv->op = _g_enum_type_get_value_by_nick (GTH_TYPE_TEST_OP, value)->value;

	self->priv->negative = g_strcmp0 (dom_element_get_attribute (element, "negative"), "true") == 0;

	value = dom_element_get_attribute (element, "value");
	if (value == NULL)
		return;

	switch (self->priv->data_type) {
	case GTH_TEST_DATA_TYPE_INT:
		gth_test_simple_set_data_as_int (self, atol (value));
		break;

	case GTH_TEST_DATA_TYPE_DOUBLE:
		gth_test_simple_set_data_as_double (self, atof (value));
		break;

	case GTH_TEST_DATA_TYPE_SIZE:
		gth_test_simple_set_data_as_size (self, atol (value));
		break;

	case GTH_TEST_DATA_TYPE_STRING:
		gth_test_simple_set_data_as_string (self, value);
		break;

	case GTH_TEST_DATA_TYPE_DATE:
		{
			GthDateTime *dt;

			dt = gth_datetime_new ();
			gth_datetime_from_exif_date (dt, value);
			gth_test_simple_set_data_as_date (self, dt->date);

			gth_datetime_free (dt);
		}
		break;

	default:
		break;
	}
}


static gboolean
update_from_control_for_integer (GthTestSimple  *self,
				 GError        **error)
{
	GthOpData op_data;

	op_data = int_op_data[gtk_combo_box_get_active (GTK_COMBO_BOX (self->priv->text_op_combo_box))];
	self->priv->op = op_data.op;
	self->priv->negative = op_data.negative;
	gth_test_simple_set_data_as_int (self, gtk_spin_button_get_value_as_int  (GTK_SPIN_BUTTON (self->priv->spinbutton)));

	return TRUE;
}


static gboolean
update_from_control_for_double (GthTestSimple  *self,
				GError        **error)
{
	GthOpData op_data;

	op_data = int_op_data[gtk_combo_box_get_active (GTK_COMBO_BOX (self->priv->text_op_combo_box))];
	self->priv->op = op_data.op;
	self->priv->negative = op_data.negative;
	gth_test_simple_set_data_as_double (self, gtk_spin_button_get_value  (GTK_SPIN_BUTTON (self->priv->spinbutton)));

	return TRUE;
}


static gboolean
update_from_control_for_size (GthTestSimple  *self,
			      GError        **error)
{
	GthOpData op_data;
	double    value;
	guint64   size;

	op_data = int_op_data[gtk_combo_box_get_active (GTK_COMBO_BOX (self->priv->size_op_combo_box))];
	self->priv->op = op_data.op;
	self->priv->negative = op_data.negative;

	value = atol (gtk_entry_get_text (GTK_ENTRY (self->priv->text_entry)));
	size = value * size_data[gtk_combo_box_get_active (GTK_COMBO_BOX (self->priv->size_combo_box))].size;
	gth_test_simple_set_data_as_size (self, size);

	if ((self->priv->data.size == 0) && (self->priv->op == GTH_TEST_OP_LOWER)) {
		if (error != NULL)
			*error = g_error_new (GTH_TEST_ERROR, 0, _("The test definition is incomplete"));
		return FALSE;
	}

	return TRUE;
}


static gboolean
update_from_control_for_string (GthTestSimple  *self,
				GError        **error)
{
	GthOpData op_data;

	op_data = text_op_data[gtk_combo_box_get_active (GTK_COMBO_BOX (self->priv->text_op_combo_box))];
	self->priv->op = op_data.op;
	self->priv->negative = op_data.negative;
	gth_test_simple_set_data_as_string (self, gtk_entry_get_text (GTK_ENTRY (self->priv->text_entry)));

	if (g_strcmp0 (self->priv->data.s, "") == 0) {
		if (error != NULL)
			*error = g_error_new (GTH_TEST_ERROR, 0, _("The test definition is incomplete"));
		return FALSE;
	}

	return TRUE;
}


static gboolean
update_from_control_for_date (GthTestSimple  *self,
			      GError        **error)
{
	GthOpData    op_data;
	GthDateTime *dt;

	op_data = date_op_data[gtk_combo_box_get_active (GTK_COMBO_BOX (self->priv->date_op_combo_box))];
	self->priv->op = op_data.op;
	self->priv->negative = op_data.negative;

	dt = gth_datetime_new ();
	gth_time_selector_get_value (GTH_TIME_SELECTOR (self->priv->time_selector), dt);
	gth_test_simple_set_data_as_date (self, dt->date);
	gth_datetime_free (dt);

	if (! g_date_valid (self->priv->data.date)) {
		if (error != NULL)
			*error = g_error_new (GTH_TEST_ERROR, 0, _("The test definition is incomplete"));
		return FALSE;
	}

	return TRUE;
}


static gboolean
gth_test_simple_real_update_from_control (GthTest  *base,
			                  GError  **error)
{
	GthTestSimple *self;
	gboolean       retval;

	self = GTH_TEST_SIMPLE (base);

	switch (self->priv->data_type) {
	case GTH_TEST_DATA_TYPE_INT:
		retval = update_from_control_for_integer (self, error);
		break;

	case GTH_TEST_DATA_TYPE_DOUBLE:
		retval = update_from_control_for_double (self, error);
		break;

	case GTH_TEST_DATA_TYPE_SIZE:
		retval = update_from_control_for_size (self, error);
		break;

	case GTH_TEST_DATA_TYPE_STRING:
		retval = update_from_control_for_string (self, error);
		break;

	case GTH_TEST_DATA_TYPE_DATE:
		retval = update_from_control_for_date (self, error);
		break;

	case GTH_TEST_DATA_TYPE_NONE:
	default:
		retval = TRUE;
		break;
	}

	return retval;
}


static void
gth_test_simple_real_focus_control (GthTest *base)
{
	GthTestSimple *self;

	self = GTH_TEST_SIMPLE (base);

	switch (self->priv->data_type) {
	case GTH_TEST_DATA_TYPE_INT:
	case GTH_TEST_DATA_TYPE_DOUBLE:
		gtk_widget_grab_focus (self->priv->spinbutton);
		break;

	case GTH_TEST_DATA_TYPE_STRING:
	case GTH_TEST_DATA_TYPE_SIZE:
		gtk_widget_grab_focus (self->priv->text_entry);
		break;

	case GTH_TEST_DATA_TYPE_DATE:
		gth_time_selector_focus (GTH_TIME_SELECTOR (self->priv->time_selector));
		break;

	default:
		break;
	}
}


static GObject *
gth_test_simple_real_duplicate (GthDuplicable *duplicable)
{
	GthTestSimple *test = GTH_TEST_SIMPLE (duplicable);
	GthTestSimple *new_test;

	new_test = g_object_new (GTH_TYPE_TEST_SIMPLE,
				 "id", gth_test_get_id (GTH_TEST (test)),
				 "attributes", gth_test_get_attributes (GTH_TEST (test)),
				 "display-name", gth_test_get_display_name (GTH_TEST (test)),
				 "visible", gth_test_is_visible (GTH_TEST (test)),
				 NULL);

	switch (test->priv->data_type) {
	case GTH_TEST_DATA_TYPE_NONE:
		break;

	case GTH_TEST_DATA_TYPE_INT:
		gth_test_simple_set_data_as_int (new_test, test->priv->data.i);
		break;

	case GTH_TEST_DATA_TYPE_DOUBLE:
		gth_test_simple_set_data_as_double (new_test, test->priv->data.f);
		break;

	case GTH_TEST_DATA_TYPE_SIZE:
		gth_test_simple_set_data_as_size (new_test, test->priv->data.size);
		break;

	case GTH_TEST_DATA_TYPE_STRING:
		gth_test_simple_set_data_as_string (new_test, test->priv->data.s);
		break;

	case GTH_TEST_DATA_TYPE_DATE:
		gth_test_simple_set_data_as_date (new_test, test->priv->data.date);
		break;
	}

	new_test->priv->get_data = test->priv->get_data;
	new_test->priv->op = test->priv->op;
	new_test->priv->negative = test->priv->negative;
	new_test->priv->max_int = test->priv->max_int;
	new_test->priv->max_double = test->priv->max_double;

	return (GObject *) new_test;
}


static void
gth_test_simple_set_property (GObject      *object,
			      guint         property_id,
			      const GValue *value,
			      GParamSpec   *pspec)
{
	GthTestSimple *test;

        test = GTH_TEST_SIMPLE (object);

	switch (property_id) {
	case PROP_DATA_TYPE:
		test->priv->data_type = g_value_get_enum (value);
		break;

	case PROP_DATA_AS_STRING:
		_gth_test_simple_free_data (test);
		test->priv->data.s = g_value_dup_string (value);
		break;

	case PROP_DATA_AS_INT:
		_gth_test_simple_free_data (test);
		test->priv->data.i = g_value_get_int (value);
		break;

	case PROP_DATA_AS_SIZE:
		_gth_test_simple_free_data (test);
		test->priv->data.size = g_value_get_uint64 (value);
		break;

	case PROP_DATA_AS_DOUBLE:
		_gth_test_simple_free_data (test);
		test->priv->data.f = g_value_get_double (value);
		break;

	case PROP_DATA_AS_DATE:
		_gth_test_simple_free_data (test);
		test->priv->data.date = g_value_dup_boxed (value);
		break;

	case PROP_GET_DATA:
		test->priv->get_data = g_value_get_pointer (value);
		break;

	case PROP_OP:
		test->priv->op = g_value_get_enum (value);
		break;

	case PROP_NEGATIVE:
		test->priv->negative = g_value_get_boolean (value);
		break;

	case PROP_MAX_INT:
		test->priv->max_int = g_value_get_int (value);
		break;

	case PROP_MAX_DOUBLE:
		test->priv->max_double = g_value_get_double (value);
		break;

	default:
		break;
	}
}


static void
gth_test_simple_get_property (GObject    *object,
			      guint       property_id,
			      GValue     *value,
			      GParamSpec *pspec)
{
	GthTestSimple *test;

        test = GTH_TEST_SIMPLE (object);

	switch (property_id) {
	case PROP_DATA_TYPE:
		g_value_set_enum (value, test->priv->data_type);
		break;

	case PROP_DATA_AS_STRING:
		g_value_set_string (value, test->priv->data.s);
		break;

	case PROP_DATA_AS_INT:
		g_value_set_int (value, test->priv->data.i);
		break;

	case PROP_DATA_AS_SIZE:
		g_value_set_uint64 (value, test->priv->data.size);
		break;

	case PROP_DATA_AS_DOUBLE:
		g_value_set_double (value, test->priv->data.f);
		break;

	case PROP_DATA_AS_DATE:
		g_value_set_boxed (value, test->priv->data.date);
		break;

	case PROP_GET_DATA:
		g_value_set_pointer (value, test->priv->get_data);
		break;

	case PROP_OP:
		g_value_set_enum (value, test->priv->op);
		break;

	case PROP_NEGATIVE:
		g_value_set_boolean (value, test->priv->negative);
		break;

	case PROP_MAX_INT:
		g_value_set_int (value, test->priv->max_int);
		break;

	case PROP_MAX_DOUBLE:
		g_value_set_double (value, test->priv->max_double);
		break;

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
		break;
	}
}


static void
gth_test_simple_class_init (GthTestSimpleClass *class)
{
	GObjectClass *object_class;
	GthTestClass *test_class;

	object_class = (GObjectClass*) class;
	object_class->set_property = gth_test_simple_set_property;
	object_class->get_property = gth_test_simple_get_property;
	object_class->finalize = gth_test_simple_finalize;

	test_class = (GthTestClass *) class;
	test_class->create_control = gth_test_simple_real_create_control;
	test_class->update_from_control = gth_test_simple_real_update_from_control;
	test_class->focus_control = gth_test_simple_real_focus_control;
	test_class->match = gth_test_simple_real_match;

	/* properties */

	g_object_class_install_property (object_class,
					 PROP_DATA_TYPE,
					 g_param_spec_enum ("data-type",
							    "Data type",
							    "The type of the data to test",
							    GTH_TYPE_TEST_DATA_TYPE,
							    GTH_TEST_DATA_TYPE_NONE,
							    G_PARAM_READWRITE));
	g_object_class_install_property (object_class,
					 PROP_DATA_AS_STRING,
					 g_param_spec_string ("data-as-string",
                                                              "Data as string",
                                                              "The data value as a string",
                                                              NULL,
                                                              G_PARAM_READWRITE));
	g_object_class_install_property (object_class,
					 PROP_DATA_AS_INT,
					 g_param_spec_int ("data-as-int",
                                                           "Data as integer",
                                                           "The data value as an integer",
                                                           G_MININT,
                                                           G_MAXINT,
                                                           0,
                                                           G_PARAM_READWRITE));
	g_object_class_install_property (object_class,
					 PROP_DATA_AS_SIZE,
					 g_param_spec_uint64 ("data-as-size",
                                                              "Data as size",
							      "The data value as an unsigned long integer",
							      0,
							      G_MAXUINT64,
							      0,
							      G_PARAM_READWRITE));
	g_object_class_install_property (object_class,
					 PROP_DATA_AS_DOUBLE,
					 g_param_spec_double ("data-as-double",
                                                              "Data as double",
							      "The data value as a double precision real number",
							      G_MINDOUBLE,
							      G_MAXDOUBLE,
							      1.0,
							      G_PARAM_READWRITE));
	g_object_class_install_property (object_class,
					 PROP_DATA_AS_DATE,
					 g_param_spec_boxed ("data-as-date",
                                                             "Data as date",
                                                             "The data value as a date",
                                                             G_TYPE_DATE,
                                                             G_PARAM_READWRITE));
	g_object_class_install_property (object_class,
					 PROP_GET_DATA,
					 g_param_spec_pointer ("get-data-func",
                                                               "Get Data Function",
                                                               "The function used to get the value to test",
                                                               G_PARAM_READWRITE));
	g_object_class_install_property (object_class,
					 PROP_OP,
					 g_param_spec_enum ("op",
                                                            "Operation",
                                                            "The operation to use to compare the values",
                                                            GTH_TYPE_TEST_OP,
                                                            GTH_TEST_OP_NONE,
                                                            G_PARAM_READWRITE));
	g_object_class_install_property (object_class,
					 PROP_NEGATIVE,
					 g_param_spec_boolean ("negative",
                                                               "Negative",
                                                               "Whether to negate the test result",
                                                               FALSE,
                                                               G_PARAM_READWRITE));
	g_object_class_install_property (object_class,
					 PROP_MAX_INT,
					 g_param_spec_int ("max-int",
                                                           "Max integer",
                                                           "Max value for integers",
                                                           G_MININT,
                                                           G_MAXINT,
                                                           0,
                                                           G_PARAM_READWRITE));
	g_object_class_install_property (object_class,
					 PROP_MAX_DOUBLE,
					 g_param_spec_int ("max-double",
                                                           "Max double",
                                                           "Max value for doubles",
                                                           G_MININT,
                                                           G_MAXINT,
                                                           0.0,
                                                           G_PARAM_READWRITE));
}


static void
gth_test_simple_dom_domizable_interface_init (DomDomizableInterface * iface)
{
	dom_domizable_parent_iface = g_type_interface_peek_parent (iface);
	iface->create_element = gth_test_simple_real_create_element;
	iface->load_from_element = gth_test_simple_real_load_from_element;
}


static void
gth_test_simple_gth_duplicable_interface_init (GthDuplicableInterface *iface)
{
	gth_duplicable_parent_iface = g_type_interface_peek_parent (iface);
	iface->duplicate = gth_test_simple_real_duplicate;
}


static void
gth_test_simple_init (GthTestSimple *test)
{
	test->priv = gth_test_simple_get_instance_private (test);
	test->priv->data_type = GTH_TEST_DATA_TYPE_NONE;
	test->priv->get_data = NULL;
	test->priv->op = GTH_TEST_OP_NONE;
	test->priv->negative = FALSE;
	test->priv->max_int = 0;
	test->priv->max_double = 0;
	test->priv->pattern = NULL;
	test->priv->has_focus = FALSE;
	test->priv->text_entry = NULL;
	test->priv->text_op_combo_box = NULL;
	test->priv->size_op_combo_box = NULL;
	test->priv->date_op_combo_box = NULL;
	test->priv->size_combo_box = NULL;
	test->priv->time_selector = NULL;
	test->priv->spinbutton = NULL;
}


void
gth_test_simple_set_data_as_string (GthTestSimple *test,
			            const char    *s)
{
	_gth_test_simple_free_data (test);
	test->priv->data_type = GTH_TEST_DATA_TYPE_STRING;
	test->priv->data.s = g_strdup (s);
}


void
gth_test_simple_set_data_as_int (GthTestSimple *test,
			         gint64         i)
{
	_gth_test_simple_free_data (test);
	test->priv->data_type = GTH_TEST_DATA_TYPE_INT;
	test->priv->data.i = i;
}


void
gth_test_simple_set_data_as_double (GthTestSimple *test,
			            gdouble        f)
{
	_gth_test_simple_free_data (test);
	test->priv->data_type = GTH_TEST_DATA_TYPE_DOUBLE;
	test->priv->data.f = f;
}


void
gth_test_simple_set_data_as_size (GthTestSimple *test,
			          guint64        i)
{
	_gth_test_simple_free_data (test);
	test->priv->data_type = GTH_TEST_DATA_TYPE_SIZE;
	test->priv->data.size = i;
}


void
gth_test_simple_set_data_as_date (GthTestSimple *test,
				  GDate         *date)
{
	_gth_test_simple_free_data (test);
	test->priv->data_type = GTH_TEST_DATA_TYPE_DATE;
	test->priv->data.date = g_date_new ();
	if (date != NULL)
		*test->priv->data.date = *date;
	else
		g_date_clear (test->priv->data.date, 1);
}
