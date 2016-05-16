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
#include <stdlib.h>
#include <gtk/gtk.h>
#include <gthumb.h>
#include "gth-template-editor-dialog.h"
#include "gth-template-selector.h"


#define GET_WIDGET(name) _gtk_builder_get_widget (self->priv->builder, (name))


G_DEFINE_TYPE (GthTemplateEditorDialog, gth_template_editor_dialog, GTK_TYPE_DIALOG)


struct _GthTemplateEditorDialogPrivate {
	GtkWidget       *content;
	GRegex          *re;
	GthTemplateCode *allowed_codes;
	int              n_codes;
};


static void
gth_template_editor_dialog_finalize (GObject *object)
{
	GthTemplateEditorDialog *dialog;

	dialog = GTH_TEMPLATE_EDITOR_DIALOG (object);

	if (dialog->priv != NULL) {
		if (dialog->priv->re != NULL)
			g_regex_unref (dialog->priv->re);
		g_free (dialog->priv);
		dialog->priv = NULL;
	}

	G_OBJECT_CLASS (gth_template_editor_dialog_parent_class)->finalize (object);
}


static void
gth_template_editor_dialog_class_init (GthTemplateEditorDialogClass *class)
{
	GObjectClass *object_class;

	object_class = (GObjectClass*) class;
	object_class->finalize = gth_template_editor_dialog_finalize;
}


static void
gth_template_editor_dialog_init (GthTemplateEditorDialog *dialog)
{
	dialog->priv = g_new0 (GthTemplateEditorDialogPrivate, 1);
	dialog->priv->re = NULL;
}


static void
_gth_template_editor_update_sensitivity (GthTemplateEditorDialog *self)
{
	GList    *children;
	gboolean  many_selectors;
	GList    *scan;

	children = gtk_container_get_children (GTK_CONTAINER (self->priv->content));
	many_selectors = (children != NULL) && (children->next != NULL);
	for (scan = children; scan; scan = scan->next)
		gth_template_selector_can_remove (GTH_TEMPLATE_SELECTOR (children->data), many_selectors);

	g_list_free (children);
}


static GtkWidget * _gth_template_editor_create_selector (GthTemplateEditorDialog *self);


static void
selector_add_template_cb (GthTemplateSelector     *selector,
			  GthTemplateEditorDialog *self)
{
	GtkWidget *child;

	child = _gth_template_editor_create_selector (self);
	gtk_box_pack_start (GTK_BOX (self->priv->content), child, FALSE, FALSE, 0);
	gtk_box_reorder_child (GTK_BOX (self->priv->content), child, _gtk_container_get_pos (GTK_CONTAINER (self->priv->content), GTK_WIDGET (selector)) + 1);
	_gth_template_editor_update_sensitivity (self);
}


static void
selector_remove_template_cb (GthTemplateSelector     *selector,
			     GthTemplateEditorDialog *self)
{
	gtk_widget_destroy (GTK_WIDGET (selector));
	_gth_template_editor_update_sensitivity (self);
}


static GtkWidget *
_gth_template_editor_create_selector (GthTemplateEditorDialog *self)
{
	GtkWidget *child;

	child = gth_template_selector_new (self->priv->allowed_codes, self->priv->n_codes);
	gtk_widget_show (child);

	g_signal_connect (child,
			  "add_template",
			  G_CALLBACK (selector_add_template_cb),
			  self);
	g_signal_connect (child,
			  "remove_template",
			  G_CALLBACK (selector_remove_template_cb),
			  self);

	return child;
}


static void
gth_template_editor_dialog_construct (GthTemplateEditorDialog *self,
				      GthTemplateCode         *allowed_codes,
				      int                      n_codes,
				      const char              *title,
			              GtkWindow               *parent)
{
	GtkWidget *child;
	GString   *regexp;
	GString   *special_codes;
	int        i;

	self->priv->allowed_codes = allowed_codes;
	self->priv->n_codes = n_codes;

	if (title != NULL)
    		gtk_window_set_title (GTK_WINDOW (self), title);
  	if (parent != NULL)
    		gtk_window_set_transient_for (GTK_WINDOW (self), parent);
    	gtk_window_set_resizable (GTK_WINDOW (self), FALSE);
	gtk_box_set_spacing (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (self))), 5);
	gtk_container_set_border_width (GTK_CONTAINER (self), 5);

	gtk_dialog_add_button (GTK_DIALOG (self), GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL);
	gtk_dialog_add_button (GTK_DIALOG (self), GTK_STOCK_OK, GTK_RESPONSE_OK);

    	self->priv->content = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
    	gtk_container_set_border_width (GTK_CONTAINER (self->priv->content), 5);
    	gtk_widget_show (self->priv->content);
  	gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (self))), self->priv->content, TRUE, TRUE, 0);

	child = _gth_template_editor_create_selector (self);
	gtk_box_pack_start (GTK_BOX (self->priv->content), child, FALSE, FALSE, 0);

	_gth_template_editor_update_sensitivity (self);

	/* build the regular expression to compile the template */

	regexp = g_string_new ("");
	special_codes = g_string_new ("");

	for (i = 0; i < n_codes; i++) {
		GthTemplateCode *code = &allowed_codes[i];

		switch (code->type) {
		case GTH_TEMPLATE_CODE_TYPE_TEXT:
			break;
		case GTH_TEMPLATE_CODE_TYPE_ENUMERATOR:
			if (regexp->len > 0)
				g_string_append (regexp, "|");
			g_string_append (regexp, "(");
			g_string_append_c (regexp, code->code);
			g_string_append (regexp, "+)");
			break;
		case GTH_TEMPLATE_CODE_TYPE_SIMPLE:
		case GTH_TEMPLATE_CODE_TYPE_DATE:
		case GTH_TEMPLATE_CODE_TYPE_FILE_ATTRIBUTE:
			g_string_append_c (special_codes, code->code);
			break;
		}
	}

	if (special_codes->len > 0) {
		/* special code with a custom format */

		if (regexp->len > 0)
			g_string_append (regexp, "|");
		g_string_append (regexp, "(%[");
		g_string_append (regexp, special_codes->str);
		g_string_append (regexp, "]{[^}]+\\})");

		/* special codes without a custom format */

		g_string_append (regexp, "|");
		g_string_append (regexp, "(%[");
		g_string_append (regexp, special_codes->str);
		g_string_append (regexp, "])");

	}

	self->priv->re = g_regex_new (regexp->str, 0, 0, NULL);

	g_string_free (special_codes, TRUE);
	g_string_free (regexp, TRUE);
}


GtkWidget *
gth_template_editor_dialog_new (GthTemplateCode *allowed_codes,
	 	     	     	int              n_codes,
	 	     	     	const char      *title,
			        GtkWindow       *parent)
{
	GthTemplateEditorDialog *self;

	self = g_object_new (GTH_TYPE_TEMPLATE_EDITOR_DIALOG, NULL);
	gth_template_editor_dialog_construct (self, allowed_codes, n_codes, title, parent);

	return (GtkWidget *) self;
}


void
gth_template_editor_dialog_set_template (GthTemplateEditorDialog *self,
					 const char              *template)
{
	char **template_v;
	int    i;

	_gtk_container_remove_children (GTK_CONTAINER (self->priv->content), NULL, NULL);

	template_v = g_regex_split (self->priv->re, template, 0);
	for (i = 0; template_v[i] != NULL; i++) {
		GtkWidget *child;

		if ((template_v[i] == NULL) || g_str_equal (template_v[i], ""))
			continue;

		child = _gth_template_editor_create_selector (self);
		gtk_box_pack_start (GTK_BOX (self->priv->content), child, FALSE, FALSE, 0);
		gth_template_selector_set_value (GTH_TEMPLATE_SELECTOR (child), template_v[i]);
	}

	_gth_template_editor_update_sensitivity (self);

	g_strfreev (template_v);
}


char *
gth_template_editor_dialog_get_template (GthTemplateEditorDialog  *self,
				         GError                  **error)
{
	GString *template;
	GList   *children;
	GList   *scan;

	template = g_string_new ("");
	children = gtk_container_get_children (GTK_CONTAINER (self->priv->content));
	for (scan = children; scan; scan = scan->next) {
		GtkWidget *child = scan->data;
		char      *value;

		value = gth_template_selector_get_value (GTH_TEMPLATE_SELECTOR (child), NULL);
		if (value != NULL) {
			g_string_append (template, value);
			g_free (value);
		}
	}

	g_list_free (children);

	return g_string_free (template, FALSE);
}
