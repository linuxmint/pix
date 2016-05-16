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


#define GET_WIDGET(x) (_gtk_builder_get_widget (self->priv->builder, (x)))


G_DEFINE_TYPE (GthImportPreferencesDialog, gth_import_preferences_dialog, GTK_TYPE_DIALOG)


/* Signals */
enum {
	DESTINATION_CHANGED,
	LAST_SIGNAL
};


static guint signals[LAST_SIGNAL] = { 0 };


struct _GthImportPreferencesDialogPrivate {
	GtkBuilder *builder;
	GSettings  *settings;
	GtkWidget  *subfolder_type_list;
	GtkWidget  *subfolder_format_list;
	char       *event;
	gboolean    help_visible;
};


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

	g_type_class_add_private (klass, sizeof (GthImportPreferencesDialogPrivate));

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


static GthSubfolderType
get_subfolder_type (GthImportPreferencesDialog *self)
{
	if (! gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("autosubfolder_checkbutton"))))
		return GTH_SUBFOLDER_TYPE_NONE;
	else
		return gtk_combo_box_get_active (GTK_COMBO_BOX (self->priv->subfolder_type_list)) + 1;
}


static void
save_options (GthImportPreferencesDialog *self)
{
	GFile              *destination;
	gboolean            single_subfolder;
	GthSubfolderType    subfolder_type;
	GthSubfolderFormat  subfolder_format;
	const char         *custom_format;

	destination = gth_import_preferences_dialog_get_destination (self);
	if (destination != NULL) {
		char *uri;

		uri = g_file_get_uri (destination);
		g_settings_set_string (self->priv->settings, PREF_IMPORTER_DESTINATION, uri);

		g_free (uri);
	}

	single_subfolder = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("single_subfolder_checkbutton")));
	g_settings_set_boolean (self->priv->settings, PREF_IMPORTER_SUBFOLDER_SINGLE, single_subfolder);

	subfolder_type = get_subfolder_type (self);
	g_settings_set_enum (self->priv->settings, PREF_IMPORTER_SUBFOLDER_TYPE, subfolder_type);

	subfolder_format = gtk_combo_box_get_active (GTK_COMBO_BOX (self->priv->subfolder_format_list));
	g_settings_set_enum (self->priv->settings, PREF_IMPORTER_SUBFOLDER_FORMAT, subfolder_format);

	custom_format = gtk_entry_get_text (GTK_ENTRY (GET_WIDGET ("custom_format_entry")));
	g_settings_set_string (self->priv->settings, PREF_IMPORTER_SUBFOLDER_CUSTOM_FORMAT, custom_format);

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
	GFile              *destination_example;
	char               *uri;
	char               *example;
	GthSubfolderType    subfolder_type;
	GthSubfolderFormat  subfolder_format;

	destination_example = gth_import_preferences_dialog_get_destination_example (self);
	if (destination_example == NULL)
		return;

	uri = g_file_get_parse_name (destination_example);
	example = g_strdup_printf (_("example: %s"), uri);
	gtk_label_set_text (GTK_LABEL (GET_WIDGET ("example_label")), example);

	subfolder_type = get_subfolder_type (self);
	gtk_widget_set_sensitive (GET_WIDGET ("single_subfolder_checkbutton"), subfolder_type != GTH_SUBFOLDER_TYPE_NONE);
	gtk_widget_set_sensitive (self->priv->subfolder_type_list, subfolder_type != GTH_SUBFOLDER_TYPE_NONE);
	gtk_widget_set_sensitive (self->priv->subfolder_format_list, subfolder_type != GTH_SUBFOLDER_TYPE_NONE);
	gtk_widget_set_sensitive (GET_WIDGET ("subfolder_options_notebook"), subfolder_type != GTH_SUBFOLDER_TYPE_NONE);

	subfolder_format = gtk_combo_box_get_active (GTK_COMBO_BOX (self->priv->subfolder_format_list));
	gtk_notebook_set_current_page (GTK_NOTEBOOK (GET_WIDGET ("subfolder_options_notebook")), (subfolder_format == GTH_SUBFOLDER_FORMAT_CUSTOM) ? 1 : 0);

	g_signal_emit (self, signals[DESTINATION_CHANGED], 0);

	g_free (example);
	g_free (uri);
	g_object_unref (destination_example);
}


static gboolean
preferences_dialog_map_event_cb (GtkWidget *widget,
				 GdkEvent  *event,
				 gpointer   user_data)
{
	update_destination ((GthImportPreferencesDialog *) user_data);
	return FALSE;
}


static void
subfolder_type_list_changed_cb (GtkWidget *widget,
				gpointer   user_data)
{
	update_destination ((GthImportPreferencesDialog *) user_data);
}


static void
subfolder_format_list_changed_cb (GtkWidget *widget,
				  gpointer   user_data)
{
	update_destination ((GthImportPreferencesDialog *) user_data);
}


static void
destination_selection_changed_cb (GtkWidget *widget,
				  gpointer  *user_data)
{
	update_destination ((GthImportPreferencesDialog *) user_data);
}


static void
subfolder_hierarchy_checkbutton_toggled_cb (GtkWidget *widget,
					    gpointer  *user_data)
{
	update_destination ((GthImportPreferencesDialog *) user_data);
}


static void
autosubfolder_checkbutton_toggled_cb (GtkToggleButton *togglebutton,
				      gpointer        *user_data)
{
	update_destination ((GthImportPreferencesDialog *) user_data);
}


static void
custom_format_entry_changed_cb (GtkEditable *editable,
				gpointer    *user_data)
{
	update_destination ((GthImportPreferencesDialog *) user_data);
}


static void
custom_format_entry_icon_press_cb (GtkEntry             *entry,
				   GtkEntryIconPosition  icon_pos,
				   GdkEvent             *event,
				   gpointer              user_data)
{
	GthImportPreferencesDialog *self = user_data;

	self->priv->help_visible = ! self->priv->help_visible;

	if (self->priv->help_visible)
		gtk_widget_show (GET_WIDGET ("template_help_alignment"));
	else
		gtk_widget_hide (GET_WIDGET ("template_help_alignment"));
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
	GthSubfolderType  subfolder_type;
	char             *custom_format;

	self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, GTH_TYPE_IMPORT_PREFERENCES_DIALOG, GthImportPreferencesDialogPrivate);
	self->priv->builder = _gtk_builder_new_from_file ("import-preferences.ui", "importer");
	self->priv->settings = g_settings_new (GTHUMB_IMPORTER_SCHEMA);
	self->priv->help_visible = FALSE;

	gtk_window_set_title (GTK_WINDOW (self), _("Preferences"));
	gtk_window_set_resizable (GTK_WINDOW (self), FALSE);
	gtk_window_set_modal (GTK_WINDOW (self), TRUE);
	gtk_box_set_spacing (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (self))), 5);
	gtk_container_set_border_width (GTK_CONTAINER (self), 5);

	content = _gtk_builder_get_widget (self->priv->builder, "import_preferences");
	gtk_container_set_border_width (GTK_CONTAINER (content), 5);
	gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (self))), content, TRUE, TRUE, 0);

	/* subfolder type  */

	self->priv->subfolder_type_list = _gtk_combo_box_new_with_texts (_("File date"),
									 _("Current date"),
									 NULL);
	gtk_combo_box_set_active (GTK_COMBO_BOX (self->priv->subfolder_type_list), 0);
	gtk_widget_show (self->priv->subfolder_type_list);
	gtk_box_pack_start (GTK_BOX (GET_WIDGET ("subfolder_type_box")), self->priv->subfolder_type_list, TRUE, TRUE, 0);
	/*gtk_label_set_mnemonic_widget (GTK_LABEL (GET_WIDGET ("subfolder_label")), self->priv->subfolder_type_list);*/

	/* subfolder format */

	self->priv->subfolder_format_list = _gtk_combo_box_new_with_texts (_("year-month-day"),
									   _("year-month"),
									   _("year"),
									   _("custom format"),
									   NULL);
	gtk_combo_box_set_active (GTK_COMBO_BOX (self->priv->subfolder_format_list), 0);
	gtk_widget_show (self->priv->subfolder_format_list);
	gtk_box_pack_start (GTK_BOX (GET_WIDGET ("subfolder_type_box")), self->priv->subfolder_format_list, TRUE, TRUE, 0);

	gtk_dialog_add_button (GTK_DIALOG (self),
			       GTK_STOCK_CLOSE,
			       GTK_RESPONSE_CLOSE);

	/* set widget data */

	destination = gth_import_preferences_get_destination ();
	gtk_file_chooser_set_current_folder_file (GTK_FILE_CHOOSER (GET_WIDGET ("destination_filechooserbutton")),
						  destination,
						  NULL);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("single_subfolder_checkbutton")),
				      g_settings_get_boolean (self->priv->settings, PREF_IMPORTER_SUBFOLDER_SINGLE));
	subfolder_type = g_settings_get_enum (self->priv->settings, PREF_IMPORTER_SUBFOLDER_TYPE);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("autosubfolder_checkbutton")), subfolder_type != GTH_SUBFOLDER_TYPE_NONE);
	gtk_combo_box_set_active (GTK_COMBO_BOX (self->priv->subfolder_type_list),
				  (subfolder_type == 0) ? 0 : subfolder_type - 1);
	gtk_combo_box_set_active (GTK_COMBO_BOX (self->priv->subfolder_format_list),
				  g_settings_get_enum (self->priv->settings, PREF_IMPORTER_SUBFOLDER_FORMAT));

	custom_format = g_settings_get_string (self->priv->settings, PREF_IMPORTER_SUBFOLDER_CUSTOM_FORMAT);
	if (custom_format != NULL) {
		gtk_entry_set_text (GTK_ENTRY (GET_WIDGET ("custom_format_entry")), custom_format);
		g_free (custom_format);
	}

	update_destination (self);

	g_signal_connect (self->priv->subfolder_type_list,
			  "changed",
			  G_CALLBACK (subfolder_type_list_changed_cb),
			  self);
	g_signal_connect (self->priv->subfolder_format_list,
			  "changed",
			  G_CALLBACK (subfolder_format_list_changed_cb),
			  self);
	g_signal_connect (GET_WIDGET ("destination_filechooserbutton"),
			  "selection_changed",
			  G_CALLBACK (destination_selection_changed_cb),
			  self);
	g_signal_connect (GET_WIDGET ("single_subfolder_checkbutton"),
			  "toggled",
			  G_CALLBACK (subfolder_hierarchy_checkbutton_toggled_cb),
			  self);
	g_signal_connect (self,
			  "map-event",
			  G_CALLBACK (preferences_dialog_map_event_cb),
			  self);
	g_signal_connect (self,
			  "delete-event",
			  G_CALLBACK (preferences_dialog_delete_event_cb),
			  self);
	g_signal_connect (GET_WIDGET ("autosubfolder_checkbutton"),
			  "toggled",
			  G_CALLBACK (autosubfolder_checkbutton_toggled_cb),
			  self);
	g_signal_connect (GET_WIDGET ("custom_format_entry"),
			  "changed",
			  G_CALLBACK (custom_format_entry_changed_cb),
			  self);
	g_signal_connect (GET_WIDGET ("custom_format_entry"),
			  "icon-press",
			  G_CALLBACK (custom_format_entry_icon_press_cb),
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
	return (GtkWidget *) g_object_new (GTH_TYPE_IMPORT_PREFERENCES_DIALOG, NULL);
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
gth_import_preferences_dialog_get_destination_example (GthImportPreferencesDialog *self)
{
	GFile              *destination;
	GthSubfolderType    subfolder_type;
	GthSubfolderFormat  subfolder_format;
	gboolean            single_subfolder;
	const char         *custom_format;
	GthFileData        *example_data;
	GTimeVal            timeval;
	GFile              *destination_example;

	destination = gth_import_preferences_dialog_get_destination (self);
	if (destination == NULL)
		return NULL;

	subfolder_type = get_subfolder_type (self);
	subfolder_format = gtk_combo_box_get_active (GTK_COMBO_BOX (self->priv->subfolder_format_list));
	single_subfolder = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("single_subfolder_checkbutton")));
	custom_format = gtk_entry_get_text (GTK_ENTRY (GET_WIDGET ("custom_format_entry")));

	example_data = create_example_file_data ();
	g_get_current_time (&timeval);

	destination_example = gth_import_utils_get_file_destination (example_data,
								     destination,
								     subfolder_type,
								     subfolder_format,
								     single_subfolder,
								     custom_format,
								     self->priv->event,
								     timeval);

	g_object_unref (example_data);
	g_object_unref (destination);

	return destination_example;
}
