/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2010 Free Software Foundation, Inc.
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
#include "gth-import-enum-types.h"
#include "gth-import-preferences-dialog.h"
#include "preferences.h"
#include "utils.h"


#define EXAMPLE_FILE_DATE "2005-03-09T13:23:51Z"
#define GET_WIDGET(x) (_gtk_builder_get_widget (self->priv->builder, (x)))


static char * Importer_Date_Formats[] = {
	"%Y-%m-%d",
	"%Y-%m",
	"%Y",
	"%Y/%m/%d",
	"%Y/%m",
	NULL
};


static GthTemplateCode Subfolder_Special_Codes[] = {
	{ GTH_TEMPLATE_CODE_TYPE_DATE, N_("File date"), 'D' },
	{ GTH_TEMPLATE_CODE_TYPE_DATE, N_("Current date"), 'T' },
	{ GTH_TEMPLATE_CODE_TYPE_SIMPLE, N_("Event description"), 'E' },
};


/* Signals */
enum {
	DESTINATION_CHANGED,
	LAST_SIGNAL
};


static guint signals[LAST_SIGNAL] = { 0 };


struct _GthImportPreferencesDialogPrivate {
	GtkBuilder *builder;
	GSettings  *settings;
	char       *event;
};


G_DEFINE_TYPE_WITH_CODE (GthImportPreferencesDialog,
			 gth_import_preferences_dialog,
			 GTK_TYPE_DIALOG,
			 G_ADD_PRIVATE (GthImportPreferencesDialog))


static void
gth_import_preferences_dialog_finalize (GObject *object)
{
	GthImportPreferencesDialog *self;

	self = GTH_IMPORT_PREFERENCES_DIALOG (object);

	_g_object_unref (self->priv->settings);
	_g_object_unref (self->priv->builder);
	g_free (self->priv->event);

	G_OBJECT_CLASS (gth_import_preferences_dialog_parent_class)->finalize (object);
}


static void
gth_import_preferences_dialog_class_init (GthImportPreferencesDialogClass *klass)
{
	GObjectClass *object_class;

	object_class = (GObjectClass*) klass;
	object_class->finalize = gth_import_preferences_dialog_finalize;

	/* signals */

	signals[DESTINATION_CHANGED] =
		g_signal_new ("destination-changed",
			      G_TYPE_FROM_CLASS (klass),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (GthImportPreferencesDialogClass, destination_changed),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__VOID,
			      G_TYPE_NONE,
			      0);
}


static void
save_options (GthImportPreferencesDialog *self)
{
	GFile *destination;

	destination = gth_import_preferences_dialog_get_destination (self);
	if (destination != NULL) {
		char *uri;

		uri = g_file_get_uri (destination);
		g_settings_set_string (self->priv->settings, PREF_IMPORTER_DESTINATION, uri);

		g_free (uri);
	}

	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("automatic_subfolder_checkbutton")))) {
		const char *subfolder_template = gtk_entry_get_text (GTK_ENTRY (GET_WIDGET ("subfolder_template_entry")));
		g_settings_set_string (self->priv->settings, PREF_IMPORTER_SUBFOLDER_TEMPLATE, subfolder_template);
	}
	else
		g_settings_set_string (self->priv->settings, PREF_IMPORTER_SUBFOLDER_TEMPLATE, "");

	_g_object_unref (destination);
}


static void
save_and_hide (GthImportPreferencesDialog *self)
{
	save_options (self);
	gtk_widget_hide (GTK_WIDGET (self));
}


static gboolean
preferences_dialog_delete_event_cb (GtkWidget *widget,
				    GdkEvent  *event,
				    gpointer   user_data)
{
	save_and_hide ((GthImportPreferencesDialog *) user_data);
	return TRUE;
}


static GthFileData *
create_example_file_data (void)
{
	GFile       *file;
	GFileInfo   *info;
	GthFileData *file_data;
	GthMetadata *metadata;

	file = g_file_new_for_uri ("file://home/user/document.txt");
	info = g_file_info_new ();
	file_data = gth_file_data_new (file, info);

	metadata = g_object_new (GTH_TYPE_METADATA,
				 "raw", "2005:03:09 13:23:51",
				 "formatted", "2005:03:09 13:23:51",
				 NULL);
	g_file_info_set_attribute_object (info, "Embedded::Photo::DateTimeOriginal", G_OBJECT (metadata));

	g_object_unref (metadata);
	g_object_unref (info);
	g_object_unref (file);

	return file_data;
}


static void
update_destination (GthImportPreferencesDialog *self)
{
	GFile *destination;

	destination = gth_import_preferences_dialog_get_destination (self);
	if (destination == NULL) {
		gtk_label_set_text (GTK_LABEL (GET_WIDGET ("example_label")), "");
	}
	else {
		GString *example;
		char    *destination_name;
		GFile   *subfolder;

		example = g_string_new ("");
		destination_name = g_file_get_parse_name (destination);
		_g_string_append_markup_escaped (example, "%s", destination_name);

		subfolder = gth_import_preferences_dialog_get_subfolder_example (self);
		if (subfolder != NULL) {
			char *subfolder_name = g_file_get_parse_name (subfolder);
			if (! _g_str_empty (subfolder_name) && ! _g_str_equal (subfolder_name, "/")) {
				const char *name = g_str_has_suffix (destination_name, "/") ? subfolder_name + 1 : subfolder_name;
				_g_string_append_markup_escaped (example, "<span foreground=\"#4696f8\">%s</span>", name);
			}

			g_free (subfolder_name);
			g_object_unref (subfolder);
		}

		gtk_label_set_markup (GTK_LABEL (GET_WIDGET ("example_label")), example->str);

		g_free (destination_name);
		g_string_free (example, TRUE);
		g_object_unref (destination);
	}

	g_signal_emit (self, signals[DESTINATION_CHANGED], 0);
}


static void
destination_selection_changed_cb (GtkWidget *widget,
				  gpointer  *user_data)
{
	update_destination ((GthImportPreferencesDialog *) user_data);
}


static void
automatic_subfolder_checkbutton_toggled_cb (GtkToggleButton *togglebutton,
					    gpointer         user_data)
{
	GthImportPreferencesDialog *self = user_data;
	gboolean                    active;

	active = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("automatic_subfolder_checkbutton")));
	gtk_widget_set_visible (GET_WIDGET ("subfolder_section_box"), active);
	if (active)
		gtk_widget_grab_focus (GET_WIDGET ("subfolder_template_entry"));
	update_destination (self);
}


static void
subfolder_template_entry_changed_cb (GtkEditable *editable,
				     gpointer    *user_data)
{
	update_destination ((GthImportPreferencesDialog *) user_data);
}


static gboolean
template_eval_cb (TemplateFlags   flags,
		  gunichar        parent_code,
		  gunichar        code,
		  char          **args,
		  GString        *result,
		  gpointer        user_data)
{
	GthImportPreferencesDialog *self = user_data;
	gboolean                    preview;
	char                       *text = NULL;
	GDateTime                  *timestamp;

	preview = flags & TEMPLATE_FLAGS_PREVIEW;

	if ((parent_code == 'D') || (parent_code == 'T')) {
		/* strftime code, return the code itself. */
		_g_string_append_template_code (result, code, args);
		return FALSE;
	}

	if (preview && (code != 0))
		g_string_append (result, "<span foreground=\"#4696f8\">");

	/**/

	switch (code) {
	case 'D': /* File date */
		timestamp = g_date_time_new_from_iso8601 (EXAMPLE_FILE_DATE, NULL);
		text = g_date_time_format (timestamp,
					   (args[0] != NULL) ? args[0] : DEFAULT_STRFTIME_FORMAT);
		g_date_time_unref (timestamp);
		break;

	case 'T': /* Timestamp */
		timestamp = g_date_time_new_now_local ();
		text = g_date_time_format (timestamp,
					   (args[0] != NULL) ? args[0] : DEFAULT_STRFTIME_FORMAT);
		g_date_time_unref (timestamp);
		break;

	case 'E': /* Event description */
		if (self->priv->event != NULL)
			g_string_append (result, self->priv->event);
		break;

	default:
		break;
	}

	if (text != NULL) {
		g_string_append (result, text);
		g_free (text);
	}

	if (preview && (code != 0))
		g_string_append (result, "</span>");

	return FALSE;
}


static void
edit_subfolder_template_button_clicked_cb (GtkButton *button,
					   gpointer   user_data)
{
	GthImportPreferencesDialog *self = user_data;
	GtkWidget                  *dialog;

	dialog = gth_template_editor_dialog_new (Subfolder_Special_Codes,
						 G_N_ELEMENTS (Subfolder_Special_Codes),
						 0,
						 _("Edit Template"),
						 GTK_WINDOW (self));
	gth_template_editor_dialog_set_preview_cb (GTH_TEMPLATE_EDITOR_DIALOG (dialog),
						   template_eval_cb,
						   self);
	gth_template_editor_dialog_set_date_formats (GTH_TEMPLATE_EDITOR_DIALOG (dialog),
						     Importer_Date_Formats);
	gth_template_editor_dialog_set_template (GTH_TEMPLATE_EDITOR_DIALOG (dialog),
						 gtk_entry_get_text (GTK_ENTRY (GET_WIDGET ("subfolder_template_entry"))));
	g_signal_connect (dialog,
			  "response",
			  G_CALLBACK (gth_template_editor_dialog_default_response),
			  GET_WIDGET ("subfolder_template_entry"));
	gtk_widget_show (dialog);
}


static void
dialog_response_cb (GtkDialog *dialog,
                    int        response_id,
                    gpointer   user_data)
{
	GthImportPreferencesDialog *self = user_data;

	if ((response_id == GTK_RESPONSE_DELETE_EVENT) || (response_id == GTK_RESPONSE_CLOSE))
		save_and_hide (self);
}


static void
gth_import_preferences_dialog_init (GthImportPreferencesDialog *self)
{
	GtkWidget        *content;
	GFile            *destination;
	char             *subfolder_template;

	self->priv = gth_import_preferences_dialog_get_instance_private (self);
	self->priv->builder = _gtk_builder_new_from_file ("import-preferences.ui", "importer");
	self->priv->settings = g_settings_new (GTHUMB_IMPORTER_SCHEMA);

	content = _gtk_builder_get_widget (self->priv->builder, "import_preferences");
	gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (self))), content, TRUE, TRUE, 0);
	gtk_dialog_add_button (GTK_DIALOG (self), _GTK_LABEL_CLOSE, GTK_RESPONSE_CLOSE);

	/* set widget data */

	destination = gth_import_preferences_get_destination ();
	gtk_file_chooser_set_file (GTK_FILE_CHOOSER (GET_WIDGET ("destination_filechooserbutton")),
				   destination,
				   NULL);

	subfolder_template = g_settings_get_string (self->priv->settings, PREF_IMPORTER_SUBFOLDER_TEMPLATE);
	if (subfolder_template != NULL) {
		gtk_entry_set_text (GTK_ENTRY (GET_WIDGET ("subfolder_template_entry")), subfolder_template);
		if (! _g_str_empty (subfolder_template)) {
			gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("automatic_subfolder_checkbutton")), TRUE);
			automatic_subfolder_checkbutton_toggled_cb (NULL, self);
		}
		g_free (subfolder_template);
	}

	update_destination (self);

	g_signal_connect (GET_WIDGET ("destination_filechooserbutton"),
			  "selection_changed",
			  G_CALLBACK (destination_selection_changed_cb),
			  self);
	g_signal_connect (self,
			  "delete-event",
			  G_CALLBACK (preferences_dialog_delete_event_cb),
			  self);
	g_signal_connect (GET_WIDGET ("automatic_subfolder_checkbutton"),
			  "toggled",
			  G_CALLBACK (automatic_subfolder_checkbutton_toggled_cb),
			  self);
	g_signal_connect (GET_WIDGET ("subfolder_template_entry"),
			  "changed",
			  G_CALLBACK (subfolder_template_entry_changed_cb),
			  self);
	g_signal_connect (GET_WIDGET ("edit_subfolder_template_button"),
			  "clicked",
			  G_CALLBACK (edit_subfolder_template_button_clicked_cb),
			  self);
	g_signal_connect (self,
			  "response",
			  G_CALLBACK (dialog_response_cb),
			  self);

	g_object_unref (destination);
}


GtkWidget *
gth_import_preferences_dialog_new (void)
{
	return (GtkWidget *) g_object_new (GTH_TYPE_IMPORT_PREFERENCES_DIALOG,
					   "title", _("Preferences"),
					   "resizable", FALSE,
					   "modal", TRUE,
					   "use-header-bar", _gtk_settings_get_dialogs_use_header (),
					   NULL);
}


void
gth_import_preferences_dialog_set_event (GthImportPreferencesDialog *self,
					 const char                 *event)
{
	g_free (self->priv->event);
	self->priv->event = g_strdup (event);

	g_signal_emit (self, signals[DESTINATION_CHANGED], 0);
}


GFile *
gth_import_preferences_dialog_get_destination (GthImportPreferencesDialog *self)
{
	return gtk_file_chooser_get_file (GTK_FILE_CHOOSER (GET_WIDGET ("destination_filechooserbutton")));
}


GFile *
gth_import_preferences_dialog_get_subfolder_example (GthImportPreferencesDialog *self)
{
	GFile       *destination;
	GthFileData *example_file_data;
	const char  *subfolder_template;
	GTimeVal     timestamp;
	GFile       *destination_example;

	destination = g_file_new_for_path("/");

	example_file_data = create_example_file_data ();
	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("automatic_subfolder_checkbutton"))))
		subfolder_template = gtk_entry_get_text (GTK_ENTRY (GET_WIDGET ("subfolder_template_entry")));
	else
		subfolder_template = NULL;
	g_get_current_time (&timestamp);
	destination_example = gth_import_utils_get_file_destination (example_file_data,
								     destination,
								     subfolder_template,
								     self->priv->event,
								     timestamp);

	g_object_unref (example_file_data);
	g_object_unref (destination);

	return destination_example;
}
