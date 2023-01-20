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
#include "glib-utils.h"
#include "gtk-utils.h"
#include "gth-template-editor-dialog.h"
#include "gth-template-selector.h"
#include "str-utils.h"


#define UPDATE_PREVIEW_DELAY 100
#define GET_WIDGET(name) _gtk_builder_get_widget (self->priv->builder, (name))


struct _GthTemplateEditorDialogPrivate {
	GtkBuilder          *builder;
	GtkWidget           *selectors;
	GtkWidget           *preview;
	GthTemplateCode     *allowed_codes;
	int                  n_codes;
	gulong               update_id;
	TemplatePreviewFunc  preview_func;
	gpointer             preview_data;
	TemplateEvalFunc     preview_cb;
	TemplateFlags        template_flags;
	char               **date_formats;
};


G_DEFINE_TYPE_WITH_CODE (GthTemplateEditorDialog,
			 gth_template_editor_dialog,
			 GTK_TYPE_DIALOG,
			 G_ADD_PRIVATE (GthTemplateEditorDialog))


static void
gth_template_editor_dialog_finalize (GObject *object)
{
	GthTemplateEditorDialog *self = GTH_TEMPLATE_EDITOR_DIALOG (object);

	if (self->priv->update_id != 0) {
		g_source_remove (self->priv->update_id);
		self->priv->update_id = 0;
	}
	_g_object_unref (self->priv->builder);
	g_free (self->priv->allowed_codes);

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
gth_template_editor_dialog_init (GthTemplateEditorDialog *self)
{
	self->priv = gth_template_editor_dialog_get_instance_private (self);
	self->priv->selectors = NULL;
	self->priv->allowed_codes = NULL;
	self->priv->n_codes = 0;
	self->priv->update_id = 0;
	self->priv->preview_func = NULL;
	self->priv->preview_data = NULL;
	self->priv->preview_cb = NULL;
	self->priv->template_flags = 0;
}


static void
_gth_template_editor_update_sensitivity (GthTemplateEditorDialog *self)
{
	GList    *children;
	gboolean  many_selectors;
	GList    *scan;

	children = gtk_container_get_children (GTK_CONTAINER (self->priv->selectors));
	many_selectors = (children != NULL) && (children->next != NULL);
	for (scan = children; scan; scan = scan->next)
		gth_template_selector_can_remove (GTH_TEMPLATE_SELECTOR (scan->data), many_selectors);

	g_list_free (children);
}


static char *
_get_preview_from_template (GthTemplateEditorDialog *self,
			    const char              *template,
			    gboolean                 highlight_code)
{
	GString   *preview;
	char     **template_v;
	int        i, j;
	GTimeVal   timeval;

	if (template == NULL)
		return g_strdup ("");

	preview = g_string_new ("");
	template_v = _g_template_tokenize (template, self->priv->template_flags);
	for (i = 0; template_v[i] != NULL; i++) {
		const char      *token = template_v[i];
		GthTemplateCode *code = NULL;

		for (j = 0; (code == NULL) && (j < self->priv->n_codes); j++) {
			GthTemplateCode *allowed_code = self->priv->allowed_codes + j;

			switch (allowed_code->type) {
			case GTH_TEMPLATE_CODE_TYPE_ENUMERATOR:
				if (token[0] == '#')
					code = allowed_code;
				break;

			case GTH_TEMPLATE_CODE_TYPE_TEXT:
			case GTH_TEMPLATE_CODE_TYPE_SPACE:
				/* ignore */
				break;

			default:
				if (_g_template_token_is (token, allowed_code->code))
					code = allowed_code;
				break;
			}
		}

		if (code != NULL) {
			char **args;
			char  *text;

			if (highlight_code)
				g_string_append (preview, "<span foreground=\"#4696f8\">");

			args = _g_template_get_token_args (template_v[i]);

			switch (code->type) {
			case GTH_TEMPLATE_CODE_TYPE_ENUMERATOR:
				text = _g_template_replace_enumerator (token, 1);
				_g_string_append_markup_escaped (preview, "%s", text);
				g_free (text);
				break;

			case GTH_TEMPLATE_CODE_TYPE_DATE:
				g_get_current_time (&timeval);
				text = _g_time_val_strftime (&timeval, (args[0] != NULL) ? args[0] : DEFAULT_STRFTIME_FORMAT);
				_g_string_append_markup_escaped (preview, "%s", text);
				g_free (text);
				break;

			case GTH_TEMPLATE_CODE_TYPE_FILE_ATTRIBUTE:
				if (args[0] != NULL)
					_g_string_append_markup_escaped (preview, "{ %s }", args[0]);
				break;

			case GTH_TEMPLATE_CODE_TYPE_ASK_VALUE:
				if ((args[0] != NULL) && (args[1] != NULL)) {
					text = _get_preview_from_template (self, args[1], FALSE);
					g_string_append (preview, text);
					g_free (text);
				}
				else if (args[0] != NULL)
					_g_string_append_markup_escaped (preview, "{ %s }", args[0]);
				else
					g_string_append_unichar (preview, code->code);
				break;

			case GTH_TEMPLATE_CODE_TYPE_QUOTED:
				text = _get_preview_from_template (self, args[0], FALSE);
				g_string_append_printf (preview, "'%s'", text);
				g_free (text);
				break;

			default:
				_g_string_append_markup_escaped (preview, "%s", token);
				break;
			}

			if (highlight_code)
				g_string_append (preview, "</span>");

			g_strfreev (args);
		}
		else
			_g_string_append_markup_escaped (preview, "%s", token);
	}

	g_strfreev (template_v);

	return g_string_free (preview, FALSE);
}


static void
_gth_template_editor_update_preview (GthTemplateEditorDialog *self)
{
	char *template;
	char *preview;

	template = gth_template_editor_dialog_get_template (self);
	if (self->priv->preview_cb != NULL)
		preview = _g_template_eval (template,
					    self->priv->template_flags | TEMPLATE_FLAGS_PREVIEW,
					    self->priv->preview_cb,
					    self->priv->preview_data);
	else if (self->priv->preview_func != NULL)
		preview = self->priv->preview_func (template,
						    self->priv->template_flags,
						    self->priv->preview_data);
	else
		preview = _get_preview_from_template (self, template, TRUE);

	if (_g_str_empty (preview)) {
		g_free (preview);
		preview = g_strdup (" ");
	}
	gtk_label_set_markup (GTK_LABEL (self->priv->preview), preview);

	g_free (preview);
	g_free (template);
}


static gboolean
update_preview_cb (gpointer user_data)
{
	GthTemplateEditorDialog *self = user_data;

	if (self->priv->update_id != 0) {
		g_source_remove (self->priv->update_id);
		self->priv->update_id = 0;
	}

	_gth_template_editor_update_preview (self);

	return FALSE;
}


static void
_gth_template_editor_queue_update_preview (GthTemplateEditorDialog *self)
{
	if (self->priv->update_id != 0)
		g_source_remove (self->priv->update_id);
	self->priv->update_id = g_timeout_add (UPDATE_PREVIEW_DELAY, update_preview_cb, self);
}


static GtkWidget * _gth_template_editor_create_selector (GthTemplateEditorDialog *self);


static void
selector_add_template_cb (GthTemplateSelector     *selector,
			  GthTemplateEditorDialog *self)
{
	GtkWidget *child;

	child = _gth_template_editor_create_selector (self);
	gtk_box_pack_start (GTK_BOX (self->priv->selectors), child, FALSE, FALSE, 0);
	gtk_box_reorder_child (GTK_BOX (self->priv->selectors),
			       child,
			       _gtk_container_get_pos (GTK_CONTAINER (self->priv->selectors),
						       GTK_WIDGET (selector)) + 1);
	_gth_template_editor_update_sensitivity (self);
	_gth_template_editor_queue_update_preview (self);
	gth_template_selector_focus (GTH_TEMPLATE_SELECTOR (child));
}


static void
selector_remove_template_cb (GthTemplateSelector     *selector,
			     GthTemplateEditorDialog *self)
{
	gtk_widget_destroy (GTK_WIDGET (selector));
	_gth_template_editor_update_sensitivity (self);
	_gth_template_editor_queue_update_preview (self);
}


static void
selector_editor_dialog_response_cb (GtkDialog *dialog,
				    int        response_id,
				    gpointer   user_data)
{
	GtkEntry *entry = user_data;
	char     *value;

	switch (response_id) {
	case GTK_RESPONSE_OK:
		value = gth_template_editor_dialog_get_template (GTH_TEMPLATE_EDITOR_DIALOG (dialog));
		if (value != NULL) {
			gtk_entry_set_text (entry, value);
			gtk_widget_destroy (GTK_WIDGET (dialog));

			g_free (value);
		}
		break;

	default:
		gtk_widget_destroy (GTK_WIDGET (dialog));
		break;
	}
}


static void
selector_edit_template_cb (GthTemplateSelector     *selector,
			   GtkEntry                *entry,
			   GthTemplateEditorDialog *self)
{
	GthTemplateCode *code;
	GtkWidget       *dialog;

	code = gth_template_selector_get_code (selector);
	if (code == NULL)
		return;

	dialog = gth_template_editor_dialog_new (self->priv->allowed_codes,
						 self->priv->n_codes,
						 self->priv->template_flags | TEMPLATE_FLAGS_PARTIAL,
						 gtk_window_get_title (GTK_WINDOW (self)),
						 GTK_WINDOW (self));
	gth_template_editor_dialog_set_preview_func (GTH_TEMPLATE_EDITOR_DIALOG (dialog),
						     self->priv->preview_func,
						     self->priv->preview_data);
	gth_template_editor_dialog_set_template (GTH_TEMPLATE_EDITOR_DIALOG (dialog),
						 gtk_entry_get_text (entry));

	g_signal_connect (dialog,
			  "response",
			  G_CALLBACK (selector_editor_dialog_response_cb),
			  entry);
	gtk_widget_show (dialog);
}


static void
selector_changed_cb (GthTemplateSelector     *selector,
		     GthTemplateEditorDialog *self)
{
	_gth_template_editor_queue_update_preview (self);
}


static GtkWidget *
_gth_template_editor_create_selector (GthTemplateEditorDialog *self)
{
	GtkWidget *child;

	child = gth_template_selector_new (self->priv->allowed_codes, self->priv->n_codes, self->priv->date_formats);
	gth_template_selector_set_value (GTH_TEMPLATE_SELECTOR (child), "");
	gtk_widget_show (child);

	g_signal_connect (child,
			  "add_template",
			  G_CALLBACK (selector_add_template_cb),
			  self);
	g_signal_connect (child,
			  "remove_template",
			  G_CALLBACK (selector_remove_template_cb),
			  self);
	g_signal_connect (child,
			  "edit_template",
			  G_CALLBACK (selector_edit_template_cb),
			  self);
	g_signal_connect (child,
			  "changed",
			  G_CALLBACK (selector_changed_cb),
			  self);

	return child;
}


static void
gth_template_editor_dialog_construct (GthTemplateEditorDialog *self,
				      GthTemplateCode         *allowed_codes,
				      int                      n_codes,
				      TemplateFlags            template_flags,
				      const char              *title,
				      GtkWindow               *parent)
{
	GtkWidget *content;
	int        i, j;
	gboolean   has_enumerator;

	self->priv->n_codes = n_codes + 2;
	self->priv->allowed_codes = g_new0 (GthTemplateCode, self->priv->n_codes);

	j = 0;

	self->priv->allowed_codes[j].type = GTH_TEMPLATE_CODE_TYPE_TEXT;
	self->priv->allowed_codes[j].description = N_("Text");
	j++;

	self->priv->allowed_codes[j].type = GTH_TEMPLATE_CODE_TYPE_SPACE;
	/* Translators: The space character not the Astronomy space. */
	self->priv->allowed_codes[j].description = N_("Space");
	j++;

	has_enumerator = FALSE;
	for (i = 0; i < n_codes; i++) {
		if ((allowed_codes[i].type == GTH_TEMPLATE_CODE_TYPE_TEXT)
		    || (allowed_codes[i].type == GTH_TEMPLATE_CODE_TYPE_SPACE))
		{
			self->priv->n_codes--;
			continue;
		}

		self->priv->allowed_codes[j] = allowed_codes[i];
		if (self->priv->allowed_codes[j].type == GTH_TEMPLATE_CODE_TYPE_ENUMERATOR)
			has_enumerator = TRUE;

		j++;
	}

	self->priv->template_flags = template_flags;
	if (! has_enumerator)
		self->priv->template_flags |= TEMPLATE_FLAGS_NO_ENUMERATOR;

	if (title != NULL)
		gtk_window_set_title (GTK_WINDOW (self), title);
	if (parent != NULL) {
		gtk_window_set_transient_for (GTK_WINDOW (self), parent);
		_gtk_dialog_add_to_window_group (GTK_DIALOG (self));
		gtk_window_set_modal (GTK_WINDOW (self), TRUE);
	}
	gtk_window_set_resizable (GTK_WINDOW (self), FALSE);

	gtk_dialog_add_button (GTK_DIALOG (self), _GTK_LABEL_CANCEL, GTK_RESPONSE_CANCEL);
	gtk_dialog_add_button (GTK_DIALOG (self), _GTK_LABEL_OK, GTK_RESPONSE_OK);

	_gtk_dialog_add_class_to_response (GTK_DIALOG (self), GTK_RESPONSE_OK, GTK_STYLE_CLASS_SUGGESTED_ACTION);

	self->priv->builder = _gtk_builder_new_from_file ("template-editor-dialog.ui", NULL);

	content = _gtk_builder_get_widget (self->priv->builder, "content");
	gtk_widget_show (content);
	gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (self))), content, TRUE, TRUE, 0);

	self->priv->preview = _gtk_builder_get_widget (self->priv->builder, "preview_label");
	self->priv->selectors = _gtk_builder_get_widget (self->priv->builder, "selectors");

	gtk_box_pack_start (GTK_BOX (self->priv->selectors),
			    _gth_template_editor_create_selector (self),
			    FALSE,
			    FALSE,
			    0);

	_gth_template_editor_update_sensitivity (self);
}


GtkWidget *
gth_template_editor_dialog_new (GthTemplateCode *allowed_codes,
				int              n_codes,
				TemplateFlags    template_flags,
				const char      *title,
				GtkWindow       *parent)
{
	GthTemplateEditorDialog *self;

	self = g_object_new (GTH_TYPE_TEMPLATE_EDITOR_DIALOG,
			     "use-header-bar", _gtk_settings_get_dialogs_use_header (),
			     NULL);
	gth_template_editor_dialog_construct (self, allowed_codes, n_codes, template_flags, title, parent);

	return (GtkWidget *) self;
}


void
gth_template_editor_dialog_set_template (GthTemplateEditorDialog *self,
					 const char              *template)
{
	char **template_v;
	int    i;

	_gtk_container_remove_children (GTK_CONTAINER (self->priv->selectors), NULL, NULL);

	template_v = _g_template_tokenize (template, self->priv->template_flags);
	for (i = 0; template_v[i] != NULL; i++) {
		GtkWidget *child;

		if ((template_v[i] == NULL) || g_str_equal (template_v[i], ""))
			continue;

		child = _gth_template_editor_create_selector (self);
		gtk_box_pack_start (GTK_BOX (self->priv->selectors), child, FALSE, FALSE, 0);
		gth_template_selector_set_value (GTH_TEMPLATE_SELECTOR (child), template_v[i]);
	}

	if (template_v[0] == NULL) {
		GtkWidget *child;

		/* Empty template. */

		child = _gth_template_editor_create_selector (self);
		gtk_box_pack_start (GTK_BOX (self->priv->selectors), child, FALSE, FALSE, 0);
		gth_template_selector_set_value (GTH_TEMPLATE_SELECTOR (child), "");
	}

	_gth_template_editor_update_sensitivity (self);
	_gth_template_editor_update_preview (self);

	g_strfreev (template_v);
}


char *
gth_template_editor_dialog_get_template (GthTemplateEditorDialog *self)
{
	GString *template;
	GList   *children;
	GList   *scan;

	template = g_string_new ("");
	children = gtk_container_get_children (GTK_CONTAINER (self->priv->selectors));
	for (scan = children; scan; scan = scan->next) {
		GtkWidget *child = scan->data;
		char      *value;

		value = gth_template_selector_get_value (GTH_TEMPLATE_SELECTOR (child));
		if (value != NULL) {
			g_string_append (template, value);
			g_free (value);
		}
	}

	g_list_free (children);

	return g_string_free (template, FALSE);
}


void
gth_template_editor_dialog_set_preview_func (GthTemplateEditorDialog  *self,
					     TemplatePreviewFunc       func,
					     gpointer                  user_data)
{
	self->priv->preview_func = func;
	self->priv->preview_data = user_data;
	self->priv->preview_cb = NULL;
}


void
gth_template_editor_dialog_set_preview_cb (GthTemplateEditorDialog  *self,
					   TemplateEvalFunc          func,
					   gpointer                  user_data)
{
	self->priv->preview_func = NULL;
	self->priv->preview_cb = func;
	self->priv->preview_data = user_data;
}


void
gth_template_editor_dialog_set_date_formats (GthTemplateEditorDialog  *self,
					     char                    **formats)
{
	self->priv->date_formats = formats;
}


void
gth_template_editor_dialog_default_response (GtkDialog *dialog,
					     int        response_id,
					     gpointer   user_data)
{
	GtkEntry *entry = GTK_ENTRY (user_data);
	char     *value;

	switch (response_id) {
	case GTK_RESPONSE_OK:
		value = gth_template_editor_dialog_get_template (GTH_TEMPLATE_EDITOR_DIALOG (dialog));
		if (value != NULL) {
			gtk_entry_set_text (entry, value);
			gtk_widget_destroy (GTK_WIDGET (dialog));

			g_free (value);
		}
		break;

	default:
		gtk_widget_destroy (GTK_WIDGET (dialog));
		break;
	}
}
