/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2009 The Free Software Foundation, Inc.
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
#include <gdk/gdkkeysyms.h>
#include <gthumb.h>
#include "gth-script-editor-dialog.h"
#include "gth-script-file.h"


#define GET_WIDGET(name) _gtk_builder_get_widget (self->priv->builder, (name))


static GthTemplateCode Command_Special_Codes[] = {
	{ GTH_TEMPLATE_CODE_TYPE_QUOTED, N_("Quoted text"), GTH_SCRIPT_CODE_QUOTE, 1 },
	{ GTH_TEMPLATE_CODE_TYPE_SIMPLE, N_("File URI"), GTH_SCRIPT_CODE_URI, 0 },
	{ GTH_TEMPLATE_CODE_TYPE_SIMPLE, N_("File path"), GTH_SCRIPT_CODE_PATH, 0 },
	{ GTH_TEMPLATE_CODE_TYPE_SIMPLE, N_("File name"), GTH_SCRIPT_CODE_BASENAME, 0 },
	{ GTH_TEMPLATE_CODE_TYPE_SIMPLE, N_("File name, no extension"), GTH_SCRIPT_CODE_BASENAME_NO_EXTENSION, 0 },
	{ GTH_TEMPLATE_CODE_TYPE_SIMPLE, N_("File extension"), GTH_SCRIPT_CODE_EXTENSION, 0 },
	{ GTH_TEMPLATE_CODE_TYPE_SIMPLE, N_("Folder path"), GTH_SCRIPT_CODE_PARENT_PATH, 0 },
	{ GTH_TEMPLATE_CODE_TYPE_DATE, N_("Current date"), GTH_SCRIPT_CODE_TIMESTAMP, 1 },
	{ GTH_TEMPLATE_CODE_TYPE_ASK_VALUE, N_("Ask a value"), GTH_SCRIPT_CODE_ASK_VALUE, 2 },
	{ GTH_TEMPLATE_CODE_TYPE_FILE_ATTRIBUTE, N_("File attribute"), GTH_SCRIPT_CODE_FILE_ATTRIBUTE, 1 },
};


enum {
	SHORTCUT_NAME_COLUMN = 0,
	SHORTCUT_SENSITIVE_COLUMN
};

struct _GthScriptEditorDialogPrivate {
	GthWindow   *shortcut_window;
	GtkBuilder  *builder;
	GtkWidget   *accel_button;
	char        *script_id;
	gboolean     script_visible;
	gboolean     wait_command;
	gboolean     shell_script;
	gboolean     for_each_file;
	GthShortcut *shortcut;
};


G_DEFINE_TYPE_WITH_CODE (GthScriptEditorDialog,
			 gth_script_editor_dialog,
			 GTK_TYPE_DIALOG,
			 G_ADD_PRIVATE (GthScriptEditorDialog))


static void
gth_script_editor_dialog_finalize (GObject *object)
{
	GthScriptEditorDialog *dialog;

	dialog = GTH_SCRIPT_EDITOR_DIALOG (object);

	g_object_unref (dialog->priv->builder);
	g_free (dialog->priv->script_id);

	G_OBJECT_CLASS (gth_script_editor_dialog_parent_class)->finalize (object);
}


static void
gth_script_editor_dialog_class_init (GthScriptEditorDialogClass *class)
{
	GObjectClass *object_class;

	object_class = (GObjectClass*) class;
	object_class->finalize = gth_script_editor_dialog_finalize;
}


static void
gth_script_editor_dialog_init (GthScriptEditorDialog *dialog)
{
	dialog->priv = gth_script_editor_dialog_get_instance_private (dialog);
	dialog->priv->builder = NULL;
	dialog->priv->accel_button = NULL;
	dialog->priv->script_id = NULL;
	dialog->priv->script_visible = FALSE;
	dialog->priv->wait_command = FALSE;
	dialog->priv->shell_script = FALSE;
	dialog->priv->for_each_file = FALSE;
	dialog->priv->shortcut = NULL;
	dialog->priv->shortcut_window = NULL;
}


static void
update_sensitivity (GthScriptEditorDialog *self)
{
	/* FIXME */
}


static void
edit_command_button_clicked_cb (GtkButton *button,
				gpointer   user_data)
{
	GthScriptEditorDialog *self = user_data;
	GtkWidget             *dialog;

	dialog = gth_template_editor_dialog_new (Command_Special_Codes,
						 G_N_ELEMENTS (Command_Special_Codes),
						 0,
						 _("Edit Command"),
						 GTK_WINDOW (self));
	gth_template_editor_dialog_set_preview_func (GTH_TEMPLATE_EDITOR_DIALOG (dialog),
						     (TemplatePreviewFunc) gth_script_get_preview,
						     NULL);
	gth_template_editor_dialog_set_template (GTH_TEMPLATE_EDITOR_DIALOG (dialog),
						 gtk_entry_get_text (GTK_ENTRY (GET_WIDGET ("command_entry"))));
	g_signal_connect (dialog,
			  "response",
			  G_CALLBACK (gth_template_editor_dialog_default_response),
			  GET_WIDGET ("command_entry"));
	gtk_widget_show (dialog);
}


static gboolean
accel_button_change_value_cb (GthAccelButton  *button,
			      guint            keycode,
			      GdkModifierType  modifiers,
			      gpointer         user_data)
{
	GthScriptEditorDialog *self = user_data;
	gboolean               change;

	change = gth_window_can_change_shortcut (self->priv->shortcut_window,
						 self->priv->shortcut != NULL ? self->priv->shortcut->detailed_action : NULL,
						 GTH_SHORTCUT_CONTEXT_BROWSER_VIEWER,
						 GTH_SHORTCUT_VIEWER_CONTEXT_ANY,
						 keycode,
						 modifiers,
						 GTK_WINDOW (self));

	return change ? GDK_EVENT_PROPAGATE : GDK_EVENT_STOP;
}


static void
gth_script_editor_dialog_construct (GthScriptEditorDialog *self,
				    const char            *title,
			            GtkWindow             *parent)
{
	if (title != NULL)
		gtk_window_set_title (GTK_WINDOW (self), title);
	if (parent != NULL) {
		gtk_window_set_transient_for (GTK_WINDOW (self), parent);
		_gtk_dialog_add_to_window_group (GTK_DIALOG (self));
	}

	gtk_dialog_add_buttons (GTK_DIALOG (self),
			        _GTK_LABEL_CANCEL, GTK_RESPONSE_CANCEL,
				_GTK_LABEL_SAVE, GTK_RESPONSE_OK,
				NULL);
	_gtk_dialog_add_class_to_response (GTK_DIALOG (self), GTK_RESPONSE_OK, GTK_STYLE_CLASS_SUGGESTED_ACTION);

	self->priv->builder = gtk_builder_new_from_resource ("/org/gnome/gThumb/list_tools/data/ui/script-editor.ui");
	gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (self))), _gtk_builder_get_widget (self->priv->builder, "script_editor"), TRUE, TRUE, 0);

	self->priv->accel_button = gth_accel_button_new ();
	g_signal_connect (self->priv->accel_button,
			  "change-value",
			  G_CALLBACK (accel_button_change_value_cb),
			  self);

	gtk_widget_show (self->priv->accel_button);
	gtk_box_pack_start (GTK_BOX (GET_WIDGET ("accel_box")), self->priv->accel_button, FALSE, FALSE, 0);

	g_signal_connect (GET_WIDGET ("edit_command_button"),
			  "clicked",
			  G_CALLBACK (edit_command_button_clicked_cb),
			  self);

	/**/

	gth_script_editor_dialog_set_script (self, NULL);
}


GtkWidget *
gth_script_editor_dialog_new (const char *title,
			      GthWindow  *shortcut_window,
			      GtkWindow  *parent)
{
	GthScriptEditorDialog *self;

	self = g_object_new (GTH_TYPE_SCRIPT_EDITOR_DIALOG,
			     "use-header-bar", _gtk_settings_get_dialogs_use_header (),
			     "resizable", TRUE,
			     NULL);
	self->priv->shortcut_window = shortcut_window;
	gth_script_editor_dialog_construct (self, title, parent);

	return (GtkWidget *) self;
}


static void
_gth_script_editor_dialog_set_new_script (GthScriptEditorDialog *self)
{
	gtk_entry_set_text (GTK_ENTRY (GET_WIDGET ("name_entry")), "");
	gtk_entry_set_text (GTK_ENTRY (GET_WIDGET ("command_entry")), "");
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("shell_script_checkbutton")), TRUE);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("for_each_file_checkbutton")), FALSE);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("wait_command_checkbutton")), FALSE);
	gth_accel_button_set_accelerator (GTH_ACCEL_BUTTON (self->priv->accel_button), 0, 0);
}


void
gth_script_editor_dialog_set_script (GthScriptEditorDialog *self,
				     GthScript             *script)
{
	g_free (self->priv->script_id);
	self->priv->script_id = NULL;
	self->priv->script_visible = TRUE;
	self->priv->shortcut = NULL;

	_gth_script_editor_dialog_set_new_script (self);

	if (script != NULL) {
		self->priv->script_id = g_strdup (gth_script_get_id (script));
		self->priv->script_visible = gth_script_is_visible (script);

		gtk_entry_set_text (GTK_ENTRY (GET_WIDGET ("name_entry")), gth_script_get_display_name (script));
		gtk_entry_set_text (GTK_ENTRY (GET_WIDGET ("command_entry")), gth_script_get_command (script));
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("shell_script_checkbutton")), gth_script_is_shell_script (script));
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("for_each_file_checkbutton")), gth_script_for_each_file (script));
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("wait_command_checkbutton")), gth_script_wait_command (script));

		self->priv->shortcut = gth_window_get_shortcut (self->priv->shortcut_window, gth_script_get_detailed_action (script));
		if (self->priv->shortcut != NULL) {
			gth_accel_button_set_accelerator (GTH_ACCEL_BUTTON (self->priv->accel_button),
							  self->priv->shortcut->keyval,
							  self->priv->shortcut->modifiers);
		}
	}

	update_sensitivity (self);
}


GthScript *
gth_script_editor_dialog_get_script (GthScriptEditorDialog  *self,
				     GError                **error)
{
	GthScript       *script;
	guint            keyval;
	GdkModifierType  modifiers;
	char            *accelerator;

	script = gth_script_new ();
	if (self->priv->script_id != NULL)
		g_object_set (script, "id", self->priv->script_id, NULL);

	gth_accel_button_get_accelerator (GTH_ACCEL_BUTTON (self->priv->accel_button), &keyval, &modifiers);
	accelerator = gtk_accelerator_name (keyval, modifiers);

	g_object_set (script,
		      "display-name", gtk_entry_get_text (GTK_ENTRY (GET_WIDGET ("name_entry"))),
		      "command", gtk_entry_get_text (GTK_ENTRY (GET_WIDGET ("command_entry"))),
		      "visible", self->priv->script_visible,
		      "shell-script", gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("shell_script_checkbutton"))),
		      "for-each-file", gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("for_each_file_checkbutton"))),
		      "wait-command", gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("wait_command_checkbutton"))),
		      "accelerator", accelerator,
		      NULL);

	if (g_strcmp0 (gth_script_get_display_name (script), "") == 0) {
		*error = g_error_new (GTH_ERROR, 0, _("No name specified"));
		g_object_unref (script);
		return NULL;
	}

	if (g_strcmp0 (gth_script_get_command (script), "") == 0) {
		*error = g_error_new (GTH_ERROR, 0, _("No command specified"));
		g_object_unref (script);
		return NULL;
	}

	g_free (accelerator);

	return script;
}
