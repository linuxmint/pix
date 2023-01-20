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
#include <gtk/gtk.h>
#include "dlg-image-wall.h"
#include "gth-contact-sheet-creator.h"
#include "preferences.h"

#define GET_WIDGET(name) _gtk_builder_get_widget (data->builder, (name))
#define STRING_IS_VOID(x) (((x) == NULL) || (*(x) == 0))


static GthTemplateCode Filename_Special_Codes[] = {
	{ GTH_TEMPLATE_CODE_TYPE_ENUMERATOR, N_("Enumerator") },
	{ GTH_TEMPLATE_CODE_TYPE_DATE, N_("Current date"), 'D', 1 },
};


enum {
	SORT_TYPE_COLUMN_DATA,
	SORT_TYPE_COLUMN_NAME
};

enum {
	THUMBNAIL_SIZE_TYPE_COLUMN_SIZE,
	THUMBNAIL_SIZE_TYPE_COLUMN_NAME
};

enum {
	FILE_TYPE_COLUMN_DEFAULT_EXTENSION,
	FILE_TYPE_COLUMN_MIME_TYPE
};

typedef struct {
	GthBrowser *browser;
	GSettings  *settings;
	GList      *file_list;
	GtkBuilder *builder;
	GtkWidget  *dialog;
} DialogData;


static int thumb_size[] = { 64, 112, 128, 164, 200, 256, 312, 512 };
static int thumb_sizes = sizeof (thumb_size) / sizeof (int);


static int
get_idx_from_size (int size)
{
	int i;

	for (i = 0; i < thumb_sizes; i++)
		if (size == thumb_size[i])
			return i;
	return -1;
}


static void
destroy_cb (GtkWidget  *widget,
	    DialogData *data)
{
	gth_browser_set_dialog (data->browser, "image_wall", NULL);
	_g_object_list_unref (data->file_list);
	g_object_unref (data->settings);
	g_object_unref (data->builder);
	g_free (data);
}


static void
ok_clicked_cb (GtkWidget  *widget,
	       DialogData *data)
{
	char                 *s_value;
	GFile                *destination;
	const char           *template;
	char                 *mime_type;
	char                 *file_extension;
	GthContactSheetTheme *theme;
	int                   images_per_index;
	int                   single_page;
	int                   columns;
	GthFileDataSort      *sort_type;
	gboolean              sort_inverse;
	int                   thumbnail_size;
	GtkTreeIter           iter;
	GthTask              *task;

	/* save the options */

	s_value = gtk_file_chooser_get_uri (GTK_FILE_CHOOSER (GET_WIDGET ("destination_filechooserbutton")));
	destination = g_file_new_for_uri (s_value);
	_g_settings_set_uri (data->settings, PREF_IMAGE_WALL_DESTINATION, s_value);
	g_free (s_value);

	template = gtk_entry_get_text (GTK_ENTRY (GET_WIDGET ("template_entry")));
	g_settings_set_string (data->settings, PREF_IMAGE_WALL_TEMPLATE, template);

	mime_type = NULL;
	if (gtk_combo_box_get_active_iter (GTK_COMBO_BOX (GET_WIDGET ("filetype_combobox")), &iter)) {
		gtk_tree_model_get (GTK_TREE_MODEL (GET_WIDGET ("filetype_liststore")),
				    &iter,
				    FILE_TYPE_COLUMN_MIME_TYPE, &mime_type,
				    FILE_TYPE_COLUMN_DEFAULT_EXTENSION, &file_extension,
				    -1);
		g_settings_set_string (data->settings, PREF_IMAGE_WALL_MIME_TYPE, mime_type);
	}

	images_per_index = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (GET_WIDGET ("images_per_index_spinbutton")));
	g_settings_set_int (data->settings, PREF_IMAGE_WALL_IMAGES_PER_PAGE, images_per_index);

	single_page = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("single_index_checkbutton")));
	g_settings_set_boolean (data->settings, PREF_IMAGE_WALL_SINGLE_PAGE, single_page);

	columns = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (GET_WIDGET ("cols_spinbutton")));
	g_settings_set_int (data->settings, PREF_IMAGE_WALL_COLUMNS, columns);

	if (gtk_combo_box_get_active_iter (GTK_COMBO_BOX (GET_WIDGET ("sort_combobox")), &iter)) {
		gtk_tree_model_get (GTK_TREE_MODEL (GET_WIDGET ("sort_liststore")),
				    &iter,
				    SORT_TYPE_COLUMN_DATA, &sort_type,
				    -1);
		g_settings_set_string (data->settings, PREF_IMAGE_WALL_SORT_TYPE, sort_type->name);
	}

	sort_inverse = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("reverse_order_checkbutton")));
	g_settings_set_boolean (data->settings, PREF_IMAGE_WALL_SORT_INVERSE, sort_inverse);

	thumbnail_size = thumb_size[gtk_combo_box_get_active (GTK_COMBO_BOX (GET_WIDGET ("thumbnail_size_combobox")))];
	g_settings_set_int (data->settings, PREF_IMAGE_WALL_THUMBNAIL_SIZE, thumbnail_size);

	theme = gth_contact_sheet_theme_new ();
	theme->background_type = GTH_CONTACT_SHEET_BACKGROUND_TYPE_SOLID;
	gdk_rgba_parse (&theme->background_color1, "#000");
	theme->frame_style = GTH_CONTACT_SHEET_FRAME_STYLE_NONE;
	theme->frame_hpadding = 0;
	theme->frame_vpadding = 0;
	theme->frame_border = 0;
	theme->row_spacing = 0;
	theme->col_spacing = 0;

	/* exec the task */

	task = gth_contact_sheet_creator_new (data->browser, data->file_list);

	gth_contact_sheet_creator_set_header (GTH_CONTACT_SHEET_CREATOR (task), "");
	gth_contact_sheet_creator_set_footer (GTH_CONTACT_SHEET_CREATOR (task), "");
	gth_contact_sheet_creator_set_destination (GTH_CONTACT_SHEET_CREATOR (task), destination);
	gth_contact_sheet_creator_set_filename_template (GTH_CONTACT_SHEET_CREATOR (task), template);
	gth_contact_sheet_creator_set_mime_type (GTH_CONTACT_SHEET_CREATOR (task), mime_type, file_extension);
	gth_contact_sheet_creator_set_write_image_map (GTH_CONTACT_SHEET_CREATOR (task), FALSE);
	gth_contact_sheet_creator_set_theme (GTH_CONTACT_SHEET_CREATOR (task), theme);
	gth_contact_sheet_creator_set_images_per_index (GTH_CONTACT_SHEET_CREATOR (task), images_per_index);
	gth_contact_sheet_creator_set_single_index (GTH_CONTACT_SHEET_CREATOR (task), single_page);
	gth_contact_sheet_creator_set_columns (GTH_CONTACT_SHEET_CREATOR (task), columns);
	gth_contact_sheet_creator_set_sort_order (GTH_CONTACT_SHEET_CREATOR (task), sort_type, sort_inverse);
	gth_contact_sheet_creator_set_same_size (GTH_CONTACT_SHEET_CREATOR (task), FALSE);
	gth_contact_sheet_creator_set_thumb_size (GTH_CONTACT_SHEET_CREATOR (task), TRUE, thumbnail_size, thumbnail_size);
	gth_contact_sheet_creator_set_thumbnail_caption (GTH_CONTACT_SHEET_CREATOR (task), "");

	gth_browser_exec_task (data->browser, task, GTH_TASK_FLAGS_DEFAULT);
	gtk_widget_destroy (data->dialog);

	g_object_unref (task);
	gth_contact_sheet_theme_unref (theme);
	g_free (file_extension);
	g_free (mime_type);
	g_object_unref (destination);
}


static void
update_sensitivity (DialogData *data)
{
	gtk_widget_set_sensitive (GET_WIDGET ("images_per_index_spinbutton"), ! gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("single_index_checkbutton"))));
}


static void
edit_template_entry_button_clicked_cb (GtkWidget  *widget,
				       DialogData *data)
{
	GtkWidget *dialog;

	dialog = gth_template_editor_dialog_new (Filename_Special_Codes,
						 G_N_ELEMENTS (Filename_Special_Codes),
						 0,
						 _("Edit Template"),
						 GTK_WINDOW (data->dialog));
	gth_template_editor_dialog_set_template (GTH_TEMPLATE_EDITOR_DIALOG (dialog),
						 gtk_entry_get_text (GTK_ENTRY (GET_WIDGET ("template_entry"))));
	g_signal_connect (dialog,
			  "response",
			  G_CALLBACK (gth_template_editor_dialog_default_response),
			  GET_WIDGET ("template_entry"));
	gtk_widget_show (dialog);
}


void
dlg_image_wall (GthBrowser *browser,
		GList      *file_list)
{
	DialogData *data;
	int         i;
	int         active_index;
	char       *default_sort_type;
	GList      *sort_types;
	GList      *scan;
	GFile      *location;
	char       *s_value;
	char       *default_mime_type;
	GArray     *savers;

	if (gth_browser_get_dialog (browser, "image_wall") != NULL) {
		gtk_window_present (GTK_WINDOW (gth_browser_get_dialog (browser, "image_wall")));
		return;
	}

	data = g_new0 (DialogData, 1);
	data->browser = browser;
	data->file_list = _g_object_list_ref (file_list);
	data->builder = _gtk_builder_new_from_file ("image-wall.ui", "contact_sheet");
	data->settings = g_settings_new (GTHUMB_IMAGE_WALL_SCHEMA);

	data->dialog = g_object_new (GTK_TYPE_DIALOG,
				     "title", _("Image Wall"),
				     "transient-for", GTK_WINDOW (browser),
				     "modal", FALSE,
				     "destroy-with-parent", FALSE,
				     "use-header-bar", _gtk_settings_get_dialogs_use_header (),
				     NULL);
	gtk_container_add (GTK_CONTAINER (gtk_dialog_get_content_area (GTK_DIALOG (data->dialog))),
			   _gtk_builder_get_widget (data->builder, "dialog_content"));
	gtk_dialog_add_buttons (GTK_DIALOG (data->dialog),
				_GTK_LABEL_CANCEL, GTK_RESPONSE_CANCEL,
				_GTK_LABEL_SAVE, GTK_RESPONSE_OK,
				NULL);
	_gtk_dialog_add_class_to_response (GTK_DIALOG (data->dialog), GTK_RESPONSE_OK, GTK_STYLE_CLASS_SUGGESTED_ACTION);

	gth_browser_set_dialog (browser, "image_wall", data->dialog);
	g_object_set_data (G_OBJECT (data->dialog), "dialog_data", data);

	/* Set widgets data. */

	location = gth_browser_get_location (data->browser);
	if ((location != NULL) && g_file_has_uri_scheme (location, "file"))
		s_value = g_file_get_uri (location);
	else
		s_value = _g_settings_get_uri (data->settings, PREF_IMAGE_WALL_DESTINATION);
	if (s_value == NULL)
		s_value = g_strdup (_g_uri_get_home ());
	gtk_file_chooser_set_uri (GTK_FILE_CHOOSER (GET_WIDGET ("destination_filechooserbutton")), s_value);
	g_free (s_value);

	s_value = _g_settings_get_uri (data->settings, PREF_IMAGE_WALL_TEMPLATE);
	gtk_entry_set_text (GTK_ENTRY (GET_WIDGET ("template_entry")), s_value);
	g_free (s_value);

	default_mime_type = g_settings_get_string (data->settings, PREF_IMAGE_WALL_MIME_TYPE);
	active_index = 0;
	savers = gth_main_get_type_set ("image-saver");
	for (i = 0; (savers != NULL) && (i < savers->len); i++) {
		GthImageSaver *saver;
		GtkTreeIter     iter;

		saver = g_object_new (g_array_index (savers, GType, i), NULL);

		if (g_str_equal (default_mime_type, gth_image_saver_get_mime_type (saver)))
			active_index = i;

		gtk_list_store_append (GTK_LIST_STORE (GET_WIDGET ("filetype_liststore")), &iter);
		gtk_list_store_set (GTK_LIST_STORE (GET_WIDGET ("filetype_liststore")), &iter,
				    FILE_TYPE_COLUMN_MIME_TYPE, gth_image_saver_get_mime_type (saver),
				    FILE_TYPE_COLUMN_DEFAULT_EXTENSION, gth_image_saver_get_default_ext (saver),
				    -1);

		g_object_unref (saver);
	}
	g_free (default_mime_type);

	gtk_combo_box_set_active (GTK_COMBO_BOX (GET_WIDGET ("filetype_combobox")), active_index);

	gtk_spin_button_set_value (GTK_SPIN_BUTTON (GET_WIDGET ("images_per_index_spinbutton")),
				   g_settings_get_int (data->settings, PREF_IMAGE_WALL_IMAGES_PER_PAGE));
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("single_index_checkbutton")),
				      g_settings_get_boolean (data->settings, PREF_IMAGE_WALL_SINGLE_PAGE));
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (GET_WIDGET ("cols_spinbutton")),
				   g_settings_get_int (data->settings, PREF_IMAGE_WALL_COLUMNS));

	default_sort_type = g_settings_get_string (data->settings, PREF_IMAGE_WALL_SORT_TYPE);
	active_index = 0;
	sort_types = gth_main_get_all_sort_types ();
	for (i = 0, scan = sort_types; scan; scan = scan->next, i++) {
		GthFileDataSort *sort_type = scan->data;
		GtkTreeIter      iter;

		if (g_str_equal (sort_type->name, default_sort_type))
			active_index = i;

		gtk_list_store_append (GTK_LIST_STORE (GET_WIDGET ("sort_liststore")), &iter);
		gtk_list_store_set (GTK_LIST_STORE (GET_WIDGET ("sort_liststore")), &iter,
				    SORT_TYPE_COLUMN_DATA, sort_type,
				    SORT_TYPE_COLUMN_NAME, _(sort_type->display_name),
				    -1);
	}
	g_list_free (sort_types);
	g_free (default_sort_type);

	gtk_combo_box_set_active (GTK_COMBO_BOX (GET_WIDGET ("sort_combobox")), active_index);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("reverse_order_checkbutton")),
				      g_settings_get_boolean (data->settings, PREF_IMAGE_WALL_SORT_INVERSE));

	for (i = 0; i < thumb_sizes; i++) {
		char        *name;
		GtkTreeIter  iter;

		name = g_strdup_printf ("%d", thumb_size[i]);

		gtk_list_store_append (GTK_LIST_STORE (GET_WIDGET ("thumbnail_size_liststore")), &iter);
		gtk_list_store_set (GTK_LIST_STORE (GET_WIDGET ("thumbnail_size_liststore")), &iter,
				    THUMBNAIL_SIZE_TYPE_COLUMN_SIZE, thumb_size[i],
				    THUMBNAIL_SIZE_TYPE_COLUMN_NAME, name,
				    -1);

		g_free (name);
	}
	gtk_combo_box_set_active (GTK_COMBO_BOX (GET_WIDGET ("thumbnail_size_combobox")),
				  get_idx_from_size (g_settings_get_int (data->settings, PREF_IMAGE_WALL_THUMBNAIL_SIZE)));

	update_sensitivity (data);

	/* Set the signals handlers. */

	g_signal_connect (G_OBJECT (data->dialog),
			  "destroy",
			  G_CALLBACK (destroy_cb),
			  data);
	g_signal_connect (gtk_dialog_get_widget_for_response (GTK_DIALOG (data->dialog), GTK_RESPONSE_OK),
			  "clicked",
			  G_CALLBACK (ok_clicked_cb),
			  data);
	g_signal_connect_swapped (gtk_dialog_get_widget_for_response (GTK_DIALOG (data->dialog), GTK_RESPONSE_CANCEL),
				  "clicked",
				  G_CALLBACK (gtk_widget_destroy),
				  data->dialog);
	g_signal_connect_swapped (GET_WIDGET ("single_index_checkbutton"),
				  "toggled",
				  G_CALLBACK (update_sensitivity),
				  data);
	g_signal_connect (GET_WIDGET ("edit_template_entry_button"),
			  "clicked",
			  G_CALLBACK (edit_template_entry_button_clicked_cb),
			  data);

	/* Run dialog. */

	gtk_widget_show (data->dialog);
}
