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
#include <string.h>
#include <glib/gi18n.h>
#include <glib.h>
#include <gtk/gtk.h>
#include "gth-duplicable.h"
#include "gth-test.h"
#include "glib-utils.h"


/* Properties */
enum {
	PROP_0,
	PROP_ID,
	PROP_ATTRIBUTES,
	PROP_DISPLAY_NAME,
	PROP_VISIBLE
};

/* Signals */
enum {
	CHANGED,
	LAST_SIGNAL
};

struct _GthTestPrivate {
	char      *id;
	char      *attributes;
	char      *display_name;
	gboolean   visible;
};


static GthDuplicableInterface *gth_duplicable_parent_iface = NULL;
static guint gth_test_signals[LAST_SIGNAL] = { 0 };


static void gth_test_gth_duplicable_interface_init (GthDuplicableInterface *iface);


G_DEFINE_TYPE_WITH_CODE (GthTest,
			 gth_test,
			 G_TYPE_OBJECT,
			 G_ADD_PRIVATE (GthTest)
			 G_IMPLEMENT_INTERFACE (GTH_TYPE_DUPLICABLE,
						gth_test_gth_duplicable_interface_init))


GQuark
gth_test_error_quark (void)
{
	return g_quark_from_static_string ("gth-test-error-quark");
}


static void
gth_test_finalize (GObject *object)
{
	GthTest *self;

	self = GTH_TEST (object);

	g_free (self->priv->id);
	g_free (self->priv->attributes);
	g_free (self->priv->display_name);
	g_free (self->files);

	G_OBJECT_CLASS (gth_test_parent_class)->finalize (object);
}


static const char *
base_get_attributes (GthTest *self)
{
	return self->priv->attributes;
}


static GtkWidget *
base_create_control (GthTest *self)
{
	return NULL;
}


static gboolean
base_update_from_control (GthTest   *self,
			  GError   **error)
{
	return TRUE;
}


static void
base_focus_control (GthTest *self)
{
	/* void */
}


static GthMatch
base_match (GthTest     *self,
	    GthFileData *fdata)
{
	return GTH_MATCH_YES;
}


static void
base_set_file_list (GthTest *self,
	 	    GList   *files)
{
	GList *scan;
	int    i;

	self->n_files = g_list_length (files);

	g_free (self->files);
	self->files = g_malloc (sizeof (GthFileData*) * (self->n_files + 1));

	for (scan = files, i = 0; scan; scan = scan->next)
		self->files[i++] = scan->data;
	self->files[i++] = NULL;

	self->iterator = 0;
}


static GthFileData *
base_get_next (GthTest *self)
{
	GthFileData *file = NULL;
	GthMatch     match = GTH_MATCH_NO;

	if (self->files == NULL)
		return NULL;

	while (match == GTH_MATCH_NO) {
		file = self->files[self->iterator];
		if (file != NULL) {
			match = gth_test_match (self, file);
			self->iterator++;
		}
		else
			match = GTH_MATCH_LIMIT_REACHED;
	}

	if (match != GTH_MATCH_YES)
		file = NULL;

	if (file == NULL) {
		g_free (self->files);
		self->files = NULL;
	}

	return file;
}


static GObject *
gth_test_real_duplicate (GthDuplicable *duplicable)
{
	GthTest *self = GTH_TEST (duplicable);
	GthTest *new_test;

	new_test = g_object_new (GTH_TYPE_TEST,
				 "id", gth_test_get_id (self),
				 "attributes", gth_test_get_attributes (self),
				 "display-name", gth_test_get_display_name (self),
				 "visible", gth_test_is_visible (self),
				 NULL);

	return (GObject *) new_test;
}


static void
gth_test_set_property (GObject      *object,
		       guint         property_id,
		       const GValue *value,
		       GParamSpec   *pspec)
{
	GthTest *self;

        self = GTH_TEST (object);

	switch (property_id) {
	case PROP_ID:
		g_free (self->priv->id);
		self->priv->id = g_value_dup_string (value);
		if (self->priv->id == NULL)
			self->priv->id = g_strdup ("");
		break;
	case PROP_ATTRIBUTES:
		g_free (self->priv->attributes);
		self->priv->attributes = g_value_dup_string (value);
		if (self->priv->attributes == NULL)
			self->priv->attributes = g_strdup ("");
		break;
	case PROP_DISPLAY_NAME:
		g_free (self->priv->display_name);
		self->priv->display_name = g_value_dup_string (value);
		if (self->priv->display_name == NULL)
			self->priv->display_name = g_strdup ("");
		break;
	case PROP_VISIBLE:
		self->priv->visible = g_value_get_boolean (value);
		break;
	default:
		break;
	}
}


static void
gth_test_get_property (GObject    *object,
		       guint       property_id,
		       GValue     *value,
		       GParamSpec *pspec)
{
	GthTest *self;

        self = GTH_TEST (object);

	switch (property_id) {
	case PROP_ID:
		g_value_set_string (value, self->priv->id);
		break;
	case PROP_ATTRIBUTES:
		g_value_set_string (value, self->priv->attributes);
		break;
	case PROP_DISPLAY_NAME:
		g_value_set_string (value, self->priv->display_name);
		break;
	case PROP_VISIBLE:
		g_value_set_boolean (value, self->priv->visible);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
		break;
	}
}


static void
gth_test_class_init (GthTestClass *klass)
{
	GObjectClass *object_class;

	object_class = (GObjectClass*) klass;
	object_class->set_property = gth_test_set_property;
	object_class->get_property = gth_test_get_property;
	object_class->finalize = gth_test_finalize;

	klass->get_attributes = base_get_attributes;
	klass->create_control = base_create_control;
	klass->update_from_control = base_update_from_control;
	klass->focus_control = base_focus_control;
	klass->match = base_match;
	klass->set_file_list = base_set_file_list;
	klass->get_next = base_get_next;

	/* properties */

	g_object_class_install_property (object_class,
					 PROP_ID,
					 g_param_spec_string ("id",
                                                              "ID",
                                                              "The object id",
                                                              NULL,
                                                              G_PARAM_READWRITE));
	g_object_class_install_property (object_class,
					 PROP_ATTRIBUTES,
					 g_param_spec_string ("attributes",
                                                              "Attributes",
                                                              "The attributes required to perform this test",
                                                              NULL,
                                                              G_PARAM_READWRITE));
	g_object_class_install_property (object_class,
					 PROP_DISPLAY_NAME,
					 g_param_spec_string ("display-name",
                                                              "Display name",
                                                              "The user visible name",
                                                              NULL,
                                                              G_PARAM_READWRITE));
	g_object_class_install_property (object_class,
					 PROP_VISIBLE,
					 g_param_spec_boolean ("visible",
							       "Visible",
							       "Whether this test should be visible in the filter bar",
							       FALSE,
							       G_PARAM_READWRITE));

	/* signals */

	gth_test_signals[CHANGED] =
                g_signal_new ("changed",
                              G_TYPE_FROM_CLASS (klass),
                              G_SIGNAL_RUN_LAST,
                              G_STRUCT_OFFSET (GthTestClass, changed),
                              NULL, NULL,
                              g_cclosure_marshal_VOID__VOID,
                              G_TYPE_NONE,
                              0);
}


static void
gth_test_gth_duplicable_interface_init (GthDuplicableInterface *iface)
{
	gth_duplicable_parent_iface = g_type_interface_peek_parent (iface);
	iface->duplicate = gth_test_real_duplicate;
}


static void
gth_test_init (GthTest *self)
{
	self->priv = gth_test_get_instance_private (self);
	self->priv->id = g_strdup ("");
	self->priv->attributes = g_strdup ("");
	self->priv->display_name = g_strdup ("");
	self->priv->visible = FALSE;
}


GthTest *
gth_test_new (void)
{
	return (GthTest*) g_object_new (GTH_TYPE_TEST, NULL);
}


const char *
gth_test_get_id (GthTest *self)
{
	return self->priv->id;
}


const char *
gth_test_get_display_name (GthTest *self)
{
	return self->priv->display_name;
}


gboolean
gth_test_is_visible (GthTest *self)
{
	return self->priv->visible;
}


const char *
gth_test_get_attributes (GthTest *self)
{
	return GTH_TEST_GET_CLASS (self)->get_attributes (self);
}


GtkWidget *
gth_test_create_control (GthTest *self)
{
	return GTH_TEST_GET_CLASS (self)->create_control (self);
}


gboolean
gth_test_update_from_control (GthTest   *self,
			      GError   **error)
{
	return GTH_TEST_GET_CLASS (self)->update_from_control (self, error);
}


void
gth_test_focus_control (GthTest *self)
{
	GTH_TEST_GET_CLASS (self)->focus_control (self);
}


void
gth_test_changed (GthTest *self)
{
	g_signal_emit (self, gth_test_signals[CHANGED], 0);
}


GthMatch
gth_test_match (GthTest     *self,
		GthFileData *fdata)
{
	return GTH_TEST_GET_CLASS (self)->match (self, fdata);
}

void
gth_test_set_file_list (GthTest *self,
		        GList   *files)
{
	GTH_TEST_GET_CLASS (self)->set_file_list (self, files);
}


GthFileData *
gth_test_get_next (GthTest *self)
{
	return GTH_TEST_GET_CLASS (self)->get_next (self);
}
