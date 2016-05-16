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
#define NO_SHORTCUT 0


G_DEFINE_TYPE (GthScriptEditorDialog, gth_script_editor_dialog, GTK_TYPE_DIALOG)


enum {
	SHORTCUT_NAME_COLUMN = 0,
	SHORTCUT_SENSITIVE_COLUMN
};

struct _GthScriptEditorDialogPrivate {
	GtkBuilder *builder;
  	char       *script_id;
  	gboolean    script_visible;
  	gboolean    wait_command;
  	gboolean    shell_script;
  	gboolean    for_each_file;
  	gboolean    help_visible;
};


static void
gth_script_editor_dialog_finalize (GObject *object)
{
	GthScriptEditorDialog *dialog;

	dialog = GTH_SCRIPT_EDITOR_DIALOG (object);

	if (dialog->priv != NULL) {
		g_object_unref (dialog->priv->builder);
		g_free (dialog->priv->script_id);
		g_free (dialog->priv);
		dialog->priv = NULL;
	}

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
	dialog->priv = g_new0 (GthScriptEditorDialogPrivate, 1);
	dialog->priv->help_visible = FALSE;
}


static void
update_sensitivity (GthScriptEditorDialog *self)
{
	/* FIXME */
}


static void
command_entry_icon_press_cb (GtkEntry             *entry,
                             GtkEntryIconPosition  icon_pos,
                             GdkEvent             *event,
                             gpointer              user_data)
{
	GthScriptEditorDialog *self = user_data;

	self->priv->help_visible = ! self->priv->help_visible;

	if (self->priv->help_visible)
		gtk_widget_show (GET_WIDGET("command_help_box"));
	else
		gtk_widget_hide (GET_WIDGET("command_help_box"));
}


static void
gth_script_editor_dialog_construct (GthScriptEditorDialog *self,
				    const char            *title,
			            GtkWindow             *parent)
{
	GtkWidget   *content;
	GtkTreeIter  iter;
	int          i;

	if (title != NULL)
		gtk_window_set_title (GTK_WINDOW (self), title);
	if (parent != NULL)
		gtk_window_set_transient_for (GTK_WINDOW (self), parent);
	gtk_window_set_resizable (GTK_WINDOW (self), FALSE);
	gtk_box_set_spacing (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (self))), 5);
	gtk_container_set_border_width (GTK_CONTAINER (self), 5);

	gtk_dialog_add_button (GTK_DIALOG (self), GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL);
	gtk_dialog_add_button (GTK_DIALOG (self), GTK_STOCK_SAVE, GTK_RESPONSE_OK);
	gtk_dialog_add_button (GTK_DIALOG (self), GTK_STOCK_HELP, GTK_RESPONSE_HELP);

	self->priv->builder = _gtk_builder_new_from_file ("script-editor.ui", "list_tools");

	content = _gtk_builder_get_widget (self->priv->builder, "script_editor");
	gtk_container_set_border_width (GTK_CONTAINER (content), 5);
	gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (self))), content, TRUE, TRUE, 0);

	g_signal_connect (GET_WIDGET ("command_entry"),
			  "icon-press",
			  G_CALLBACK (command_entry_icon_press_cb),
			  self);

	gtk_list_store_append (GTK_LIST_STORE (GET_WIDGET ("shortcut_liststore")), &iter);
	gtk_list_store_set (GTK_LIST_STORE (GET_WIDGET ("shortcut_liststore")), &iter,
			    SHORTCUT_NAME_COLUMN, _("none"),
			    -1);

	for (i = 0; i <= 9; i++) {
		char *value;

		value = g_strdup_printf (_("key %d on the numeric keypad"), i);
		gtk_list_store_append (GTK_LIST_STORE (GET_WIDGET ("shortcut_liststore")), &iter);
		gtk_list_store_set (GTK_LIST_STORE (GET_WIDGET ("shortcut_liststore")), &iter,
				    SHORTCUT_NAME_COLUMN, value,
				    -1);

		g_free (value);
	}

	/**/

	gth_script_editor_dialog_set_script (self, NULL);
}


GtkWidget *
gth_script_editor_dialog_new (const char *title,
			      GtkWindow  *parent)
{
	GthScriptEditorDialog *self;

	self = g_object_new (GTH_TYPE_SCRIPT_EDITOR_DIALOG, NULL);
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
	gtk_combo_box_set_active (GTK_COMBO_BOX (GET_WIDGET ("shortcut_combobox")), NO_SHORTCUT);
}


void
gth_script_editor_dialog_set_script (GthScriptEditorDialog *self,
				     GthScript             *script)
{
	GtkTreeIter  iter;
	GtkTreePath *path;
	GList       *script_list;
	GList       *scan;

	g_free (self->priv->script_id);
	self->priv->script_id = NULL;
	self->priv->script_visible = TRUE;

	_gth_script_editor_dialog_set_new_script (self);

	if (script != NULL) {
		guint keyval;

		self->priv->script_id = g_strdup (gth_script_get_id (script));
		self->priv->script_visible = gth_script_is_visible (script);

		gtk_entry_set_text (GTK_ENTRY (GET_WIDGET ("name_entry")), gth_script_get_display_name (script));
		gtk_entry_set_text (GTK_ENTRY (GET_WIDGET ("command_entry")), gth_script_get_command (script));
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("shell_script_checkbutton")), gth_script_is_shell_script (script));
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("for_each_file_checkbutton")), gth_script_for_each_file (script));
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("wait_command_checkbutton")), gth_script_wait_command (script));

		keyval = gth_script_get_shortcut (script);
		if ((keyval >= GDK_KEY_KP_0) && (keyval <= GDK_KEY_KP_9))
			gtk_combo_box_set_active (GTK_COMBO_BOX (GET_WIDGET ("shortcut_combobox")), (keyval - GDK_KEY_KP_0) + 1);
		else
			gtk_combo_box_set_active (GTK_COMBO_BOX (GET_WIDGET ("shortcut_combobox")), NO_SHORTCUT);
	}

	if (gtk_tree_model_get_iter_first (GTK_TREE_MODEL (GET_WIDGET ("shortcut_liststore")), &iter)) {
		do {
			gtk_list_store_set (GTK_LIST_STORE (GET_WIDGET ("shortcut_liststore")), &iter,
					    SHORTCUT_SENSITIVE_COLUMN, TRUE,
					    -1);
		}
		while (gtk_tree_model_iter_next (GTK_TREE_MODEL (GET_WIDGET ("shortcut_liststore")), &iter));
	}

	script_list = gth_script_file_get_scripts (gth_script_file_get ());
	for (scan = script_list; scan; scan = scan->next) {
		GthScript *other_script = scan->data;
		guint      keyval;

		keyval = gth_script_get_shortcut (other_script);
		if ((keyval >= GDK_KEY_KP_0) && (keyval <= GDK_KEY_KP_9)) {
			if (g_strcmp0 (gth_script_get_id (other_script), self->priv->script_id) == 0)
				continue;

			path = gtk_tree_path_new_from_indices (keyval - GDK_KEY_KP_0 + 1, -1);
			gtk_tree_model_get_iter (GTK_TREE_MODEL (GET_WIDGET ("shortcut_liststore")), &iter, path);
			gtk_list_store_set (GTK_LIST_STORE (GET_WIDGET ("shortcut_liststore")), &iter,
					    SHORTCUT_SENSITIVE_COLUMN, FALSE,
					    -1);

			gtk_tree_path_free (path);
		}
	}
	_g_object_list_unref (script_list);

	update_sensitivity (self);
}


GthScript *
gth_script_editor_dialog_get_script (GthScriptEditorDialog  *self,
				     GError                **error)
{
	GthScript *script;
	int        keyval_index;
	guint      keyval;

	script = gth_script_new ();
	if (self->priv->script_id != NULL)
		g_object_set (script, "id", self->priv->script_id, NULL);

	keyval_index = gtk_combo_box_get_active (GTK_COMBO_BOX (GET_WIDGET ("shortcut_combobox")));
	if ((keyval_index >= 1) && (keyval_index <= 10))
		keyval = GDK_KEY_KP_0 + (keyval_index - 1);
	else
		keyval = GDK_KEY_VoidSymbol;

	g_object_set (script,
		      "display-name", gtk_entry_get_text (GTK_ENTRY (GET_WIDGET ("name_entry"))),
		      "command", gtk_entry_get_text (GTK_ENTRY (GET_WIDGET ("command_entry"))),
		      "visible", self->priv->script_visible,
		      "shell-script", gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("shell_script_checkbutton"))),
		      "for-each-file", gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("for_each_file_checkbutton"))),
		      "wait-command", gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("wait_command_checkbutton"))),
		      "shortcut", keyval,
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

	return script;
}
