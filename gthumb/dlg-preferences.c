/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2001-2008 The Free Software Foundation, Inc.
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
#include "dlg-preferences.h"
#include "gth-browser.h"
#include "gth-enum-types.h"
#include "gth-file-source-vfs.h"
#include "gth-main.h"
#include "gth-metadata-chooser.h"
#include "gth-preferences.h"
#include "gtk-utils.h"
#include "glib-utils.h"
#include "typedefs.h"


#define GET_WIDGET(name) _gtk_builder_get_widget (data->builder, (name))


typedef struct {
	GthBrowser *browser;
	GtkBuilder *builder;
	GtkWidget  *dialog;
	GtkWidget  *thumbnail_caption_chooser;
	GSettings  *general_settings;
	GSettings  *browser_settings;
	GSettings  *messages_settings;
} DialogData;

static int thumb_size[] = { 48, 64, 85, 95, 112, 128, 164, 200, 256 };
static int thumb_sizes = sizeof (thumb_size) / sizeof (int);


static int
get_idx_from_size (gint size)
{
	int i;

	for (i = 0; i < thumb_sizes; i++)
		if (size == thumb_size[i])
			return i;
	return -1;
}


static void
apply_changes (DialogData *data)
{
	/* Startup dir. */

	g_settings_set_boolean (data->browser_settings, PREF_BROWSER_GO_TO_LAST_LOCATION, gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("go_to_last_location_radiobutton"))));
	g_settings_set_boolean (data->browser_settings, PREF_BROWSER_USE_STARTUP_LOCATION, gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("use_startup_location_radiobutton"))));
	g_settings_set_boolean (data->general_settings, PREF_GENERAL_STORE_METADATA_IN_FILES, gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("embed_metadata_checkbutton"))));

	if (g_settings_get_boolean (data->browser_settings, PREF_BROWSER_USE_STARTUP_LOCATION)) {
		char *location;

		location = gtk_file_chooser_get_uri (GTK_FILE_CHOOSER (GET_WIDGET ("startup_dir_filechooserbutton")));
		_g_settings_set_uri (data->browser_settings, PREF_BROWSER_STARTUP_LOCATION, location);
		gth_pref_set_startup_location (location);
		g_free (location);
	}

	gth_hook_invoke ("dlg-preferences-apply", data->dialog, data->browser, data->builder);
}


static void
destroy_cb (GtkWidget *widget,
	    DialogData *data)
{
	apply_changes (data);
	gth_browser_set_dialog (data->browser, "preferences", NULL);

	g_object_unref (data->general_settings);
	g_object_unref (data->browser_settings);
	g_object_unref (data->messages_settings);
	g_object_unref (data->builder);
	g_free (data);
}


static void
close_button_clicked_cb (GtkWidget  *widget,
			 DialogData *data)
{
	gtk_widget_destroy (data->dialog);
}


static void
help_button_clicked_cb (GtkWidget  *widget,
			DialogData *data)
{
	show_help_dialog (GTK_WINDOW (data->dialog), NULL);
}


static void
use_startup_toggled_cb (GtkWidget *widget,
			DialogData *data)
{
	gtk_widget_set_sensitive (GET_WIDGET ("startup_dir_filechooserbutton"), gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget)));
	gtk_widget_set_sensitive (GET_WIDGET ("set_to_current_button"), gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget)));
}


static void
set_to_current_cb (GtkWidget  *widget,
		   DialogData *data)
{
	GthFileSource *file_source;

	file_source = gth_main_get_file_source (gth_browser_get_location (data->browser));
	if (GTH_IS_FILE_SOURCE_VFS (file_source)) {
		GFile *gio_file;
		char  *uri;

		gio_file = gth_file_source_to_gio_file (file_source, gth_browser_get_location (data->browser));
		uri = g_file_get_uri (gio_file);
		gtk_file_chooser_set_uri (GTK_FILE_CHOOSER (GET_WIDGET ("startup_dir_filechooserbutton")), uri);

		g_free (uri);
		g_object_unref (gio_file);
	}
	g_object_unref (file_source);
}


static void
toolbar_style_changed_cb (GtkWidget  *widget,
			  DialogData *data)
{
	g_settings_set_enum (data->browser_settings, PREF_BROWSER_TOOLBAR_STYLE, gtk_combo_box_get_active (GTK_COMBO_BOX (GET_WIDGET ("toolbar_style_combobox"))));
}


static void
thumbnails_pane_orientation_changed_cb (GtkWidget  *widget,
					DialogData *data)
{
	g_settings_set_enum (data->browser_settings, PREF_BROWSER_VIEWER_THUMBNAILS_ORIENT, gtk_combo_box_get_active (GTK_COMBO_BOX (GET_WIDGET ("thumbnails_pane_orient_combobox"))));
}


static void
file_properties_position_combobox_changed_cb (GtkWidget  *widget,
					      DialogData *data)
{
	g_settings_set_boolean (data->browser_settings, PREF_BROWSER_PROPERTIES_ON_THE_RIGHT, gtk_combo_box_get_active (GTK_COMBO_BOX (GET_WIDGET ("file_properties_position_combobox"))) == 1);
}


static void
reuse_active_window_checkbutton_toggled_cb (GtkToggleButton *button,
					    DialogData      *data)
{
	g_settings_set_boolean (data->browser_settings, PREF_BROWSER_REUSE_ACTIVE_WINDOW, gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("reuse_active_window_checkbutton"))));
}


static void
confirm_deletion_toggled_cb (GtkToggleButton *button,
			     DialogData      *data)
{
	g_settings_set_boolean (data->messages_settings, PREF_MSG_CONFIRM_DELETION, gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("confirm_deletion_checkbutton"))));
}


static void
ask_to_save_toggled_cb (GtkToggleButton *button,
			DialogData      *data)
{
	g_settings_set_boolean (data->messages_settings, PREF_MSG_SAVE_MODIFIED_IMAGE, gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("ask_to_save_checkbutton"))));
}


static void
thumbnail_size_changed_cb (GtkWidget  *widget,
			   DialogData *data)
{
	g_settings_set_int (data->browser_settings, PREF_BROWSER_THUMBNAIL_SIZE, thumb_size[gtk_combo_box_get_active (GTK_COMBO_BOX (GET_WIDGET ("thumbnail_size_combobox")))]);
}


static void
fast_file_type_toggled_cb (GtkToggleButton *button,
			   DialogData      *data)
{
	g_settings_set_boolean (data->browser_settings, PREF_BROWSER_FAST_FILE_TYPE, ! gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("slow_mime_type_checkbutton"))));
}


static void
thumbnail_caption_chooser_changed_cb (GthMetadataChooser *chooser,
				      DialogData         *data)
{
	char *attributes;

	attributes = gth_metadata_chooser_get_selection (chooser);
	g_settings_set_string (data->browser_settings, PREF_BROWSER_THUMBNAIL_CAPTION, attributes);

	g_free (attributes);
}


void
dlg_preferences (GthBrowser *browser)
{
	DialogData       *data;
	char             *startup_location;
	GthFileSource    *file_source;
	char             *current_caption;

	if (gth_browser_get_dialog (browser, "preferences") != NULL) {
		gtk_window_present (GTK_WINDOW (gth_browser_get_dialog (browser, "preferences")));
		return;
	}

	data = g_new0 (DialogData, 1);
	data->browser = browser;
	data->builder = _gtk_builder_new_from_file ("preferences.ui", NULL);
	data->dialog = GET_WIDGET ("preferences_dialog");
	data->general_settings = g_settings_new (GTHUMB_GENERAL_SCHEMA);
	data->browser_settings = g_settings_new (GTHUMB_BROWSER_SCHEMA);
	data->messages_settings = g_settings_new (GTHUMB_MESSAGES_SCHEMA);

	gth_browser_set_dialog (browser, "preferences", data->dialog);

	/* caption list */

	data->thumbnail_caption_chooser = gth_metadata_chooser_new (GTH_METADATA_ALLOW_IN_FILE_LIST);
	gtk_widget_show (data->thumbnail_caption_chooser);
	gtk_container_add (GTK_CONTAINER (GET_WIDGET ("caption_scrolledwindow")), data->thumbnail_caption_chooser);

	current_caption = g_settings_get_string (data->browser_settings, PREF_BROWSER_THUMBNAIL_CAPTION);
	gth_metadata_chooser_set_selection (GTH_METADATA_CHOOSER (data->thumbnail_caption_chooser), current_caption);
	g_free (current_caption);

	/* * general */

	if (g_settings_get_boolean (data->browser_settings, PREF_BROWSER_USE_STARTUP_LOCATION))
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("use_startup_location_radiobutton")), TRUE);
	else
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("go_to_last_location_radiobutton")), TRUE);

	if (! gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("use_startup_location_radiobutton")))) {
		gtk_widget_set_sensitive (GET_WIDGET ("startup_dir_filechooserbutton"), FALSE);
		gtk_widget_set_sensitive (GET_WIDGET ("set_to_current_button"), FALSE);
	}

	startup_location = _g_settings_get_uri (data->browser_settings, PREF_BROWSER_STARTUP_LOCATION);
	if (startup_location == NULL)
		startup_location = g_strdup (get_home_uri ());
	file_source = gth_main_get_file_source_for_uri (startup_location);
	if (GTH_IS_FILE_SOURCE_VFS (file_source)) {
		GFile *location;
		GFile *folder;
		char  *folder_uri;

		location = g_file_new_for_uri (startup_location);
		folder = gth_file_source_to_gio_file (file_source, location);
		folder_uri = g_file_get_uri (folder);
		gtk_file_chooser_set_uri (GTK_FILE_CHOOSER (GET_WIDGET ("startup_dir_filechooserbutton")), folder_uri);

		g_free (folder_uri);
		g_object_unref (folder);
		g_object_unref (location);
	}
	g_object_unref (file_source);
	g_free (startup_location);

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("reuse_active_window_checkbutton")),
				      g_settings_get_boolean (data->browser_settings, PREF_BROWSER_REUSE_ACTIVE_WINDOW));
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("confirm_deletion_checkbutton")),
				      g_settings_get_boolean (data->messages_settings, PREF_MSG_CONFIRM_DELETION));
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("ask_to_save_checkbutton")),
				      g_settings_get_boolean (data->messages_settings, PREF_MSG_SAVE_MODIFIED_IMAGE));
	gtk_combo_box_set_active (GTK_COMBO_BOX (GET_WIDGET ("toolbar_style_combobox")),
				  g_settings_get_enum (data->browser_settings, PREF_BROWSER_TOOLBAR_STYLE));
	gtk_combo_box_set_active (GTK_COMBO_BOX (GET_WIDGET ("thumbnails_pane_orient_combobox")),
				  g_settings_get_enum (data->browser_settings, PREF_BROWSER_VIEWER_THUMBNAILS_ORIENT));

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("embed_metadata_checkbutton")),
				      g_settings_get_boolean (data->general_settings, PREF_GENERAL_STORE_METADATA_IN_FILES));

	/* * browser */

	gtk_combo_box_set_active (GTK_COMBO_BOX (GET_WIDGET ("thumbnail_size_combobox")),
				  get_idx_from_size (g_settings_get_int (data->browser_settings, PREF_BROWSER_THUMBNAIL_SIZE)));
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("slow_mime_type_checkbutton")),
				      ! g_settings_get_boolean (data->browser_settings, PREF_BROWSER_FAST_FILE_TYPE));
	gtk_combo_box_set_active (GTK_COMBO_BOX (GET_WIDGET ("file_properties_position_combobox")),
				  g_settings_get_boolean (data->browser_settings, PREF_BROWSER_PROPERTIES_ON_THE_RIGHT) ? 1 : 0);

	gth_hook_invoke ("dlg-preferences-construct", data->dialog, data->browser, data->builder);

	/* Set the signals handlers. */

	g_signal_connect (G_OBJECT (data->dialog),
			  "destroy",
			  G_CALLBACK (destroy_cb),
			  data);
	g_signal_connect (G_OBJECT (GET_WIDGET ("close_button")),
			  "clicked",
			  G_CALLBACK (close_button_clicked_cb),
			  data);
	g_signal_connect (G_OBJECT (GET_WIDGET ("help_button")),
			  "clicked",
			  G_CALLBACK (help_button_clicked_cb),
			  data);

	/* general */

	g_signal_connect (G_OBJECT (GET_WIDGET ("toolbar_style_combobox")),
			  "changed",
			  G_CALLBACK (toolbar_style_changed_cb),
			  data);
	g_signal_connect (GET_WIDGET ("thumbnails_pane_orient_combobox"),
			  "changed",
			  G_CALLBACK (thumbnails_pane_orientation_changed_cb),
			  data);
	g_signal_connect (GET_WIDGET ("file_properties_position_combobox"),
			  "changed",
			  G_CALLBACK (file_properties_position_combobox_changed_cb),
			  data);
	g_signal_connect (G_OBJECT (GET_WIDGET ("use_startup_location_radiobutton")),
			  "toggled",
			  G_CALLBACK (use_startup_toggled_cb),
			  data);
	g_signal_connect (G_OBJECT (GET_WIDGET ("set_to_current_button")),
			  "clicked",
			  G_CALLBACK (set_to_current_cb),
			  data);
	g_signal_connect (G_OBJECT (GET_WIDGET ("reuse_active_window_checkbutton")),
			  "toggled",
			  G_CALLBACK (reuse_active_window_checkbutton_toggled_cb),
			  data);
	g_signal_connect (G_OBJECT (GET_WIDGET ("confirm_deletion_checkbutton")),
			  "toggled",
			  G_CALLBACK (confirm_deletion_toggled_cb),
			  data);
	g_signal_connect (G_OBJECT (GET_WIDGET ("ask_to_save_checkbutton")),
			  "toggled",
			  G_CALLBACK (ask_to_save_toggled_cb),
			  data);

	/* browser */

	g_signal_connect (G_OBJECT (GET_WIDGET ("thumbnail_size_combobox")),
			  "changed",
			  G_CALLBACK (thumbnail_size_changed_cb),
			  data);
	g_signal_connect (G_OBJECT (GET_WIDGET ("slow_mime_type_checkbutton")),
			  "toggled",
			  G_CALLBACK (fast_file_type_toggled_cb),
			  data);
	g_signal_connect (G_OBJECT (data->thumbnail_caption_chooser),
			  "changed",
			  G_CALLBACK (thumbnail_caption_chooser_changed_cb),
			  data);

	/* run dialog. */

	gtk_window_set_transient_for (GTK_WINDOW (data->dialog), GTK_WINDOW (browser));
	gtk_window_set_modal (GTK_WINDOW (data->dialog), FALSE);
	gtk_widget_show (data->dialog);
}
