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
#include "dlg-contact-sheet.h"
#include "gth-contact-sheet-creator.h"
#include "gth-contact-sheet-theme-dialog.h"
#include "preferences.h"

#define GET_WIDGET(name) _gtk_builder_get_widget (data->builder, (name))
#define STRING_IS_VOID(x) (((x) == NULL) || (*(x) == 0))
#define PREVIEW_SIZE 112

static GthTemplateCode Filename_Special_Codes[] = {
	{ GTH_TEMPLATE_CODE_TYPE_ENUMERATOR, N_("Enumerator") },
	{ GTH_TEMPLATE_CODE_TYPE_DATE, N_("Current date"), 'D', 1 },
};

static GthTemplateCode Text_Special_Codes[] = {
	{ GTH_TEMPLATE_CODE_TYPE_SIMPLE, N_("Current page number"), 'p', 0 },
	{ GTH_TEMPLATE_CODE_TYPE_SIMPLE, N_("Total number of pages"), 'n', 0 },
	{ GTH_TEMPLATE_CODE_TYPE_DATE, N_("Current date"), 'D', 1 },
	{ GTH_TEMPLATE_CODE_TYPE_SIMPLE, N_("Location"), 'L', 0 },
};

enum {
	THEME_COLUMN_THEME,
	THEME_COLUMN_DISPLAY_NAME,
	THEME_COLUMN_PREVIEW
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
	GthBrowser  *browser;
	GthFileData *location;
	GSettings   *settings;
	GList       *file_list;
	GtkBuilder  *builder;
	GtkWidget   *dialog;
	GtkWidget   *thumbnail_caption_chooser;
	gulong       theme_selection_changed_event;
} DialogData;


static int thumb_size[] = { 112, 128, 164, 200, 256, 312 };
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
	gth_browser_set_dialog (data->browser, "contact_sheet", NULL);
	_g_object_list_unref (data->file_list);
	g_object_unref (data->settings);
	g_object_unref (data->builder);
	_g_object_unref (data->location);
	g_free (data);
}


static void
close_dialog (DialogData *data)
{
	if (data->theme_selection_changed_event != 0) {
		g_signal_handler_disconnect (GET_WIDGET ("theme_iconview"), data->theme_selection_changed_event);
		data->theme_selection_changed_event = 0;
	}
	gtk_widget_destroy (data->dialog);
}


static void
dialog_delete_event_cb (GtkWidget *widget,
			GdkEvent  *event,
			gpointer   user_data)
{
	close_dialog (user_data);
}


static GthContactSheetTheme *
get_selected_theme (DialogData *data)
{
	GthContactSheetTheme *theme;
	GList                *list;

	theme = NULL;
	list = gtk_icon_view_get_selected_items (GTK_ICON_VIEW (GET_WIDGET ("theme_iconview")));
	if (list != NULL) {
		GtkTreePath *path;
		GtkTreeIter  iter;

		path = g_list_first (list)->data;
		gtk_tree_model_get_iter (GTK_TREE_MODEL (GET_WIDGET ("theme_liststore")), &iter, path);
		gtk_tree_model_get (GTK_TREE_MODEL (GET_WIDGET ("theme_liststore")), &iter,
				    THEME_COLUMN_THEME, &theme,
				    -1);

		g_list_foreach (list, (GFunc) gtk_tree_path_free, NULL);
		g_list_free (list);
	}

	return theme;
}


static void
ok_clicked_cb (GtkWidget  *widget,
	       DialogData *data)
{
	const char           *header;
	const char           *footer;
	char                 *s_value;
	GFile                *destination;
	const char           *template;
	char                 *mime_type;
	char                 *file_extension;
	gboolean              create_image_map;
	GthContactSheetTheme *theme;
	char                 *theme_name;
	int                   images_per_index;
	int                   single_page;
	int                   columns;
	GthFileDataSort      *sort_type;
	gboolean              sort_inverse;
	gboolean              same_size;
	int                   thumbnail_size;
	gboolean              squared_thumbnail;
	char                 *thumbnail_caption;
	GtkTreeIter           iter;
	GthTask              *task;

	/* save the options */

	header = gtk_entry_get_text (GTK_ENTRY (GET_WIDGET ("header_entry")));
	g_settings_set_string (data->settings, PREF_CONTACT_SHEET_HEADER, header);

	footer = gtk_entry_get_text (GTK_ENTRY (GET_WIDGET ("footer_entry")));
	g_settings_set_string (data->settings, PREF_CONTACT_SHEET_FOOTER, footer);

	s_value = gtk_file_chooser_get_uri (GTK_FILE_CHOOSER (GET_WIDGET ("destination_filechooserbutton")));
	destination = g_file_new_for_uri (s_value);
	_g_settings_set_uri (data->settings, PREF_CONTACT_SHEET_DESTINATION, s_value);
	g_free (s_value);

	template = gtk_entry_get_text (GTK_ENTRY (GET_WIDGET ("template_entry")));
	g_settings_set_string (data->settings, PREF_CONTACT_SHEET_TEMPLATE, template);

	mime_type = NULL;
	file_extension = NULL;
	if (gtk_combo_box_get_active_iter (GTK_COMBO_BOX (GET_WIDGET ("filetype_combobox")), &iter)) {
		gtk_tree_model_get (GTK_TREE_MODEL (GET_WIDGET ("filetype_liststore")),
				    &iter,
				    FILE_TYPE_COLUMN_MIME_TYPE, &mime_type,
				    FILE_TYPE_COLUMN_DEFAULT_EXTENSION, &file_extension,
				    -1);
		g_settings_set_string (data->settings, PREF_CONTACT_SHEET_MIME_TYPE, mime_type);
	}

	create_image_map = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("image_map_checkbutton")));
	g_settings_set_boolean (data->settings, PREF_CONTACT_SHEET_HTML_IMAGE_MAP, create_image_map);

	theme = get_selected_theme (data);
	g_return_if_fail (theme != NULL);
	theme_name = g_file_get_basename (theme->file);
	g_settings_set_string (data->settings, PREF_CONTACT_SHEET_THEME, theme_name);

	images_per_index = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (GET_WIDGET ("images_per_index_spinbutton")));
	g_settings_set_int (data->settings, PREF_CONTACT_SHEET_IMAGES_PER_PAGE, images_per_index);

	single_page = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("single_index_checkbutton")));
	g_settings_set_boolean (data->settings, PREF_CONTACT_SHEET_SINGLE_PAGE, single_page);

	columns = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (GET_WIDGET ("cols_spinbutton")));
	g_settings_set_int (data->settings, PREF_CONTACT_SHEET_COLUMNS, columns);

	sort_type = NULL;
	if (gtk_combo_box_get_active_iter (GTK_COMBO_BOX (GET_WIDGET ("sort_combobox")), &iter)) {
		gtk_tree_model_get (GTK_TREE_MODEL (GET_WIDGET ("sort_liststore")),
				    &iter,
				    SORT_TYPE_COLUMN_DATA, &sort_type,
				    -1);
		g_settings_set_string (data->settings, PREF_CONTACT_SHEET_SORT_TYPE, sort_type->name);
	}

	sort_inverse = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("reverse_order_checkbutton")));
	g_settings_set_boolean (data->settings, PREF_CONTACT_SHEET_SORT_INVERSE, sort_inverse);

	same_size = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("same_size_checkbutton")));
	g_settings_set_boolean (data->settings, PREF_CONTACT_SHEET_SAME_SIZE, same_size);

	thumbnail_size = thumb_size[gtk_combo_box_get_active (GTK_COMBO_BOX (GET_WIDGET ("thumbnail_size_combobox")))];
	g_settings_set_int (data->settings, PREF_CONTACT_SHEET_THUMBNAIL_SIZE, thumbnail_size);

	squared_thumbnail = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("squared_thumbnail_checkbutton")));
	g_settings_set_boolean (data->settings, PREF_CONTACT_SHEET_SQUARED_THUMBNAIL, squared_thumbnail);

	thumbnail_caption = gth_metadata_chooser_get_selection (GTH_METADATA_CHOOSER (data->thumbnail_caption_chooser));
	g_settings_set_string (data->settings, PREF_CONTACT_SHEET_THUMBNAIL_CAPTION, thumbnail_caption);

	/* exec the task */

	task = gth_contact_sheet_creator_new (data->browser, data->file_list);

	gth_contact_sheet_creator_set_header (GTH_CONTACT_SHEET_CREATOR (task), header);
	gth_contact_sheet_creator_set_footer (GTH_CONTACT_SHEET_CREATOR (task), footer);
	gth_contact_sheet_creator_set_destination (GTH_CONTACT_SHEET_CREATOR (task), destination);
	gth_contact_sheet_creator_set_filename_template (GTH_CONTACT_SHEET_CREATOR (task), template);
	gth_contact_sheet_creator_set_mime_type (GTH_CONTACT_SHEET_CREATOR (task), mime_type, file_extension);
	gth_contact_sheet_creator_set_write_image_map (GTH_CONTACT_SHEET_CREATOR (task), create_image_map);
	gth_contact_sheet_creator_set_theme (GTH_CONTACT_SHEET_CREATOR (task), theme);
	gth_contact_sheet_creator_set_images_per_index (GTH_CONTACT_SHEET_CREATOR (task), images_per_index);
	gth_contact_sheet_creator_set_single_index (GTH_CONTACT_SHEET_CREATOR (task), single_page);
	gth_contact_sheet_creator_set_columns (GTH_CONTACT_SHEET_CREATOR (task), columns);
	gth_contact_sheet_creator_set_sort_order (GTH_CONTACT_SHEET_CREATOR (task), sort_type, sort_inverse);
	gth_contact_sheet_creator_set_same_size (GTH_CONTACT_SHEET_CREATOR (task), same_size);
	gth_contact_sheet_creator_set_thumb_size (GTH_CONTACT_SHEET_CREATOR (task), squared_thumbnail, thumbnail_size, thumbnail_size);
	gth_contact_sheet_creator_set_thumbnail_caption (GTH_CONTACT_SHEET_CREATOR (task), thumbnail_caption);
	gth_contact_sheet_creator_set_location_name (GTH_CONTACT_SHEET_CREATOR (task), g_file_info_get_edit_name (gth_browser_get_location_data (data->browser)->info));

	gth_browser_exec_task (data->browser, task, GTH_TASK_FLAGS_DEFAULT);
	close_dialog (data);

	g_object_unref (task);
	g_free (thumbnail_caption);
	g_free (theme_name);
	g_free (file_extension);
	g_free (mime_type);
	g_object_unref (destination);
}


static void
update_sensitivity (DialogData *data)
{
	GthContactSheetTheme *theme;
	gboolean              sensitive;

	theme = get_selected_theme (data);
	sensitive = (theme != NULL) && theme->editable;
	gtk_widget_set_sensitive (GET_WIDGET ("edit_theme_button"), sensitive);
	gtk_widget_set_sensitive (GET_WIDGET ("delete_theme_button"), sensitive);

	gtk_widget_set_sensitive (GET_WIDGET ("images_per_index_spinbutton"), ! gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("single_index_checkbutton"))));
}


static void
entry_help_icon_press_cb (GtkEntry             *entry,
			  GtkEntryIconPosition  icon_pos,
			  GdkEvent             *event,
			  gpointer              user_data)
{
	DialogData *data = user_data;
	GtkWidget  *help_box = NULL;

	if (GTK_WIDGET (entry) == GET_WIDGET ("footer_entry"))
		help_box = GET_WIDGET ("page_footer_help_table");
	else if (GTK_WIDGET (entry) == GET_WIDGET ("template_entry"))
		help_box = GET_WIDGET ("template_help_table");

	if (help_box == NULL)
		return;

	if (gtk_widget_get_visible (help_box))
		gtk_widget_hide (help_box);
	else
		gtk_widget_show (help_box);
}


static void
add_themes_from_dir (DialogData *data,
		     GFile      *dir,
		     gboolean    editable)
{
	GFileEnumerator *enumerator;
	GFileInfo       *file_info;

	enumerator = g_file_enumerate_children (dir,
						(G_FILE_ATTRIBUTE_STANDARD_NAME ","
						 G_FILE_ATTRIBUTE_STANDARD_TYPE ","
						 G_FILE_ATTRIBUTE_STANDARD_DISPLAY_NAME),
						G_FILE_QUERY_INFO_NONE,
						NULL,
						NULL);
	if (enumerator == NULL)
		return;

	while ((file_info = g_file_enumerator_next_file (enumerator, NULL, NULL)) != NULL) {
		GthContactSheetTheme *theme;
		GFile                *file;
		char                 *buffer;
		gsize                 size;
		GKeyFile             *key_file;
		GtkTreeIter           iter;
		GdkPixbuf            *preview;

		if (g_file_info_get_file_type (file_info) != G_FILE_TYPE_REGULAR) {
			g_object_unref (file_info);
			continue;
		}

		if (g_strcmp0 (_g_path_get_extension (g_file_info_get_name (file_info)), ".cst") != 0) {
			g_object_unref (file_info);
			continue;
		}

		file = g_file_get_child (dir, g_file_info_get_name (file_info));
		if (! _g_file_load_in_buffer (file,
					      (void **) &buffer,
					      &size,
					      NULL,
					      NULL))
		{
			g_object_unref (file);
			g_object_unref (file_info);
			continue;
		}

		key_file = g_key_file_new ();
		if (! g_key_file_load_from_data (key_file, buffer, size, G_KEY_FILE_NONE, NULL)) {
			g_key_file_free (key_file);
			g_free (buffer);
			g_object_unref (file);
			g_object_unref (file_info);
			continue;
		}

		theme = gth_contact_sheet_theme_new_from_key_file (key_file);
		theme->file = g_object_ref (file);
		theme->editable = editable;

		preview = gth_contact_sheet_theme_create_preview (theme, PREVIEW_SIZE);
		gtk_list_store_append (GTK_LIST_STORE (GET_WIDGET ("theme_liststore")), &iter);
		gtk_list_store_set (GTK_LIST_STORE (GET_WIDGET ("theme_liststore")), &iter,
				    THEME_COLUMN_THEME, theme,
				    THEME_COLUMN_DISPLAY_NAME, theme->display_name,
				    THEME_COLUMN_PREVIEW, preview,
				    -1);

		_g_object_unref (preview);
		g_key_file_free (key_file);
		g_free (buffer);
		g_object_unref (file);
		g_object_unref (file_info);
	}

	g_object_unref (enumerator);
}


static void
load_themes (DialogData *data)
{
	GFile        *style_dir;
	GFile        *data_dir;
	char         *default_theme;
	GtkTreeModel *model;
	GtkTreeIter   iter;

	/* local themes */

	style_dir = gth_user_dir_get_file_for_read (GTH_DIR_DATA, GTHUMB_DIR, "contact_sheet_themes", NULL);
	add_themes_from_dir (data, style_dir, TRUE);
	g_object_unref (style_dir);

	/* system themes */

	data_dir = g_file_new_for_path (CONTACT_SHEET_DATADIR);
	style_dir = _g_file_get_child (data_dir, "contact_sheet_themes", NULL);
	add_themes_from_dir (data, style_dir, FALSE);
	g_object_unref (style_dir);
	g_object_unref (data_dir);

	/**/

	gtk_widget_realize (GET_WIDGET ("theme_iconview"));

	default_theme = g_settings_get_string (data->settings, PREF_CONTACT_SHEET_THEME);
	model = GTK_TREE_MODEL (GET_WIDGET ("theme_liststore"));
	if (gtk_tree_model_get_iter_first (model, &iter)) {
		gboolean theme_selected = FALSE;

		do {
			GthContactSheetTheme *theme;
			char                 *basename;

			gtk_tree_model_get (model, &iter, THEME_COLUMN_THEME, &theme, -1);
			basename = g_file_get_basename (theme->file);

			if (g_strcmp0 (basename, default_theme) == 0) {
				GtkTreePath *path;

				path = gtk_tree_model_get_path (model, &iter);
				gtk_icon_view_select_path (GTK_ICON_VIEW (GET_WIDGET ("theme_iconview")), path);
				gtk_icon_view_scroll_to_path (GTK_ICON_VIEW (GET_WIDGET ("theme_iconview")), path, TRUE, 0.5, 0.5);
				theme_selected = TRUE;

				gtk_tree_path_free (path);
				g_free (basename);
				break;
			}

			g_free (basename);
		}
		while (gtk_tree_model_iter_next (model, &iter));

		if (! theme_selected) {
			GtkTreePath *path;

			path = gtk_tree_path_new_first ();
			gtk_icon_view_select_path (GTK_ICON_VIEW (GET_WIDGET ("theme_iconview")), path);

			gtk_tree_path_free (path);
		}
	}

	g_free (default_theme);
}


static void
theme_dialog_response_cb (GtkDialog *dialog,
                	  int        response_id,
                	  gpointer   user_data)
{
	DialogData           *data = user_data;
	GthContactSheetTheme *theme;
	gboolean              new_theme;
	void                 *buffer;
	gsize                 buffer_size;
	GtkTreeIter           iter;
	GdkPixbuf            *preview;
	GtkTreePath          *path;
	GError               *error = NULL;

	if (response_id == GTK_RESPONSE_CANCEL) {
		gtk_widget_destroy (GTK_WIDGET (dialog));
		return;
	}

	if (response_id != GTK_RESPONSE_OK)
		return;

	theme = gth_contact_sheet_theme_dialog_get_theme (GTH_CONTACT_SHEET_THEME_DIALOG (dialog));
	new_theme = theme->file == NULL;

	if (theme->file == NULL) {
		GFile *themes_dir;

		gth_user_dir_mkdir_with_parents (GTH_DIR_DATA, GTHUMB_DIR, "contact_sheet_themes", NULL);
		themes_dir = gth_user_dir_get_file_for_read (GTH_DIR_DATA, GTHUMB_DIR, "contact_sheet_themes", NULL);
		theme->file = _g_file_create_unique (themes_dir,
						     theme->display_name,
						     ".cst",
						     &error);
		if (theme->file == NULL) {
			_gtk_error_dialog_from_gerror_run (GTK_WINDOW (data->dialog), _("Could not save the theme"), error);
			g_clear_error (&error);
		}

		g_object_unref (themes_dir);

		if (theme->file == NULL)
			return;
	}

	if (! gth_contact_sheet_theme_to_data (theme, &buffer, &buffer_size, &error)) {
		_gtk_error_dialog_from_gerror_run (GTK_WINDOW (data->dialog), _("Could not save the theme"), error);
		g_clear_error (&error);
		g_free (buffer);
		return;
	}

	if (! _g_file_write (theme->file,
			     FALSE,
			     G_FILE_CREATE_NONE,
			     buffer,
			     buffer_size,
			     NULL,
			     &error))
	{
		_gtk_error_dialog_from_gerror_run (GTK_WINDOW (data->dialog), _("Could not save the theme"), error);
		g_clear_error (&error);
		g_free (buffer);
		return;
	}

	g_free (buffer);

	if (! new_theme) {
		GList *list;

		list = gtk_icon_view_get_selected_items (GTK_ICON_VIEW (GET_WIDGET ("theme_iconview")));
		if (list != NULL) {
			GthContactSheetTheme *old_theme;

			path = g_list_first (list)->data;
			gtk_tree_model_get_iter (GTK_TREE_MODEL (GET_WIDGET ("theme_liststore")), &iter, path);
			gtk_tree_model_get (GTK_TREE_MODEL (GET_WIDGET ("theme_liststore")), &iter,
					    THEME_COLUMN_THEME, &old_theme,
					    -1);
			gth_contact_sheet_theme_unref (old_theme);
			gtk_list_store_remove (GTK_LIST_STORE (GET_WIDGET ("theme_liststore")), &iter);

			g_list_foreach (list, (GFunc) gtk_tree_path_free, NULL);
			g_list_free (list);
		}
	}

	preview = gth_contact_sheet_theme_create_preview (theme, PREVIEW_SIZE);
	gtk_list_store_append (GTK_LIST_STORE (GET_WIDGET ("theme_liststore")), &iter);
	gtk_list_store_set (GTK_LIST_STORE (GET_WIDGET ("theme_liststore")), &iter,
			    THEME_COLUMN_THEME, theme,
			    THEME_COLUMN_DISPLAY_NAME, theme->display_name,
			    THEME_COLUMN_PREVIEW, preview,
			    -1);
	path = gtk_tree_model_get_path (GTK_TREE_MODEL (GET_WIDGET ("theme_liststore")), &iter);
	gtk_icon_view_select_path (GTK_ICON_VIEW (GET_WIDGET ("theme_iconview")), path);

	gtk_tree_path_free (path);
	g_object_unref (preview);

	gtk_widget_destroy (GTK_WIDGET (dialog));
}


static GList *
get_all_themes (DialogData *data)
{
	GList        *list = NULL;
	GtkTreeModel *model;
	GtkTreeIter   iter;

	model = GTK_TREE_MODEL (GET_WIDGET ("theme_liststore"));
	if (gtk_tree_model_get_iter_first (model, &iter))
		do {
			GthContactSheetTheme *theme;

			gtk_tree_model_get (model, &iter,
					    THEME_COLUMN_THEME, &theme,
					    -1);
			if (theme != NULL)
				list = g_list_prepend (list, gth_contact_sheet_theme_ref (theme));
		}
		while (gtk_tree_model_iter_next (model, &iter));

	return g_list_reverse (list);
}


static void
edit_theme_button_clicked_cb (GtkButton *button,
			      gpointer   user_data)
{
	DialogData           *data = user_data;
	GthContactSheetTheme *theme;
	GList                *all_themes;
	GtkWidget            *theme_dialog;

	theme = get_selected_theme (data);
	if ((theme == NULL) || ! theme->editable)
		return;

	all_themes = get_all_themes (data);
	theme_dialog = gth_contact_sheet_theme_dialog_new (theme, all_themes);
	g_signal_connect (theme_dialog,
			  "response",
			  G_CALLBACK (theme_dialog_response_cb),
			  data);
	gtk_window_set_transient_for (GTK_WINDOW (theme_dialog), GTK_WINDOW (data->dialog));
	gtk_window_set_modal (GTK_WINDOW (theme_dialog), TRUE);
	gtk_widget_show (theme_dialog);

	gth_contact_sheet_theme_list_free (all_themes);
}


static void
add_theme_button_clicked_cb (GtkButton *button,
			     gpointer   user_data)
{
	DialogData *data = user_data;
	GList      *all_themes;
	GtkWidget  *theme_dialog;

	all_themes = get_all_themes (data);
	theme_dialog = gth_contact_sheet_theme_dialog_new (NULL, all_themes);
	g_signal_connect (theme_dialog,
			  "response",
			  G_CALLBACK (theme_dialog_response_cb),
			  data);
	gtk_window_set_transient_for (GTK_WINDOW (theme_dialog), GTK_WINDOW (data->dialog));
	gtk_window_set_modal (GTK_WINDOW (theme_dialog), TRUE);
	gtk_widget_show (theme_dialog);

	gth_contact_sheet_theme_list_free (all_themes);
}


static void
delete_theme_button_clicked_cb (GtkButton *button,
		     	        gpointer   user_data)
{
	DialogData           *data = user_data;
	GList                *list;
	GtkTreePath          *path;
	GtkTreeIter           iter;
	GthContactSheetTheme *theme;

	list = gtk_icon_view_get_selected_items (GTK_ICON_VIEW (GET_WIDGET ("theme_iconview")));
	if (list == NULL)
		return;

	path = g_list_first (list)->data;
	gtk_tree_model_get_iter (GTK_TREE_MODEL (GET_WIDGET ("theme_liststore")), &iter, path);
	gtk_tree_model_get (GTK_TREE_MODEL (GET_WIDGET ("theme_liststore")), &iter,
			    THEME_COLUMN_THEME, &theme,
			    -1);

	if (! theme->editable)
		return;

	if (theme->file != NULL) {
		GError *error = NULL;

		if (! g_file_delete (theme->file, NULL, &error)) {
			_gtk_error_dialog_from_gerror_run (GTK_WINDOW (data->dialog), _("Could not delete the theme"), error);
			g_clear_error (&error);
		}
	}

	gth_contact_sheet_theme_unref (theme);
	gtk_list_store_remove (GTK_LIST_STORE (GET_WIDGET ("theme_liststore")), &iter);

	g_list_foreach (list, (GFunc) gtk_tree_path_free, NULL);
	g_list_free (list);
}

static void
theme_iconview_selection_changed_cb (GtkIconView *iconview,
                		     gpointer     user_data)
{
	DialogData *data = user_data;
	update_sensitivity (data);
}


static gboolean
text_preview_cb (TemplateFlags   flags,
		 gunichar        parent_code,
		 gunichar        code,
		 char          **args,
		 GString        *result,
		 gpointer        user_data)
{
	DialogData *data = user_data;
	GDateTime  *timestamp;
	char       *text;

	if (parent_code == 'D') {
		/* strftime code, return the code itself. */
		_g_string_append_template_code (result, code, args);
		return FALSE;
	}

	if (code != 0)
		g_string_append (result, "<span foreground=\"#4696f8\">");

	switch (code) {
	case 'p':
		g_string_append (result, "1");
		break;

	case 'n':
		g_string_append (result, "5");
		break;

	case 'D':
		timestamp = g_date_time_new_now_local ();
		text = g_date_time_format (timestamp,
					   (args[0] != NULL) ? args[0] : DEFAULT_STRFTIME_FORMAT);
		g_string_append (result, text);

		g_free (text);
		g_date_time_unref (timestamp);
		break;

	case 'L':
		g_string_append (result, g_file_info_get_edit_name (data->location->info));
		break;

	default:
		break;
	}

	if (code != 0)
		g_string_append (result, "</span>");

	return FALSE;
}


static void
edit_header_button_clicked_cb (GtkButton *button,
			       gpointer   user_data)
{
	DialogData *data = user_data;
	GtkWidget  *dialog;

	dialog = gth_template_editor_dialog_new (Text_Special_Codes,
						 G_N_ELEMENTS (Text_Special_Codes),
						 0,
						 _("Edit Template"),
						 GTK_WINDOW (data->dialog));
	gth_template_editor_dialog_set_preview_cb (GTH_TEMPLATE_EDITOR_DIALOG (dialog),
						   text_preview_cb,
						   data);
	gth_template_editor_dialog_set_template (GTH_TEMPLATE_EDITOR_DIALOG (dialog),
						 gtk_entry_get_text (GTK_ENTRY (GET_WIDGET ("header_entry"))));
	g_signal_connect (dialog,
			  "response",
			  G_CALLBACK (gth_template_editor_dialog_default_response),
			  GET_WIDGET ("header_entry"));
	gtk_widget_show (dialog);
}


static void
edit_footer_button_clicked_cb (GtkButton *button,
			       gpointer   user_data)
{
	DialogData *data = user_data;
	GtkWidget  *dialog;

	dialog = gth_template_editor_dialog_new (Text_Special_Codes,
						 G_N_ELEMENTS (Text_Special_Codes),
						 0,
						 _("Edit Template"),
						 GTK_WINDOW (data->dialog));
	gth_template_editor_dialog_set_preview_cb (GTH_TEMPLATE_EDITOR_DIALOG (dialog),
						   text_preview_cb,
						   data);
	gth_template_editor_dialog_set_template (GTH_TEMPLATE_EDITOR_DIALOG (dialog),
						 gtk_entry_get_text (GTK_ENTRY (GET_WIDGET ("footer_entry"))));
	g_signal_connect (dialog,
			  "response",
			  G_CALLBACK (gth_template_editor_dialog_default_response),
			  GET_WIDGET ("footer_entry"));
	gtk_widget_show (dialog);
}


static void
edit_filename_button_clicked_cb (GtkButton *button,
				 gpointer   user_data)
{
	DialogData *data = user_data;
	GtkWidget  *dialog;

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
dlg_contact_sheet (GthBrowser *browser,
		   GList      *file_list)
{
	DialogData *data;
	int         i;
	int         active_index;
	char       *default_sort_type;
	GList      *sort_types;
	GList      *scan;
	char       *caption;
	char       *s_value;
	char       *default_mime_type;
	GArray     *savers;

	if (gth_browser_get_dialog (browser, "contact_sheet") != NULL) {
		gtk_window_present (GTK_WINDOW (gth_browser_get_dialog (browser, "contact_sheet")));
		return;
	}

	data = g_new0 (DialogData, 1);
	data->browser = browser;
	data->location = gth_file_data_dup (gth_browser_get_location_data (data->browser));
	data->file_list = _g_object_list_ref (file_list);
	data->builder = _gtk_builder_new_from_file ("contact-sheet.ui", "contact_sheet");
	data->settings = g_settings_new (GTHUMB_CONTACT_SHEET_SCHEMA);

	data->dialog = g_object_new (GTK_TYPE_DIALOG,
				     "title", _("Contact Sheet"),
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

	gth_browser_set_dialog (browser, "contact_sheet", data->dialog);
	g_object_set_data (G_OBJECT (data->dialog), "dialog_data", data);

	data->thumbnail_caption_chooser = gth_metadata_chooser_new (GTH_METADATA_ALLOW_IN_PRINT, TRUE);
	gtk_widget_show (data->thumbnail_caption_chooser);
	gtk_container_add (GTK_CONTAINER (GET_WIDGET ("thumbnail_caption_scrolledwindow")), data->thumbnail_caption_chooser);

	/* Set widgets data. */

	s_value = g_settings_get_string (data->settings, PREF_CONTACT_SHEET_HEADER);
	gtk_entry_set_text (GTK_ENTRY (GET_WIDGET ("header_entry")), s_value);
	g_free (s_value);

	s_value = g_settings_get_string (data->settings, PREF_CONTACT_SHEET_FOOTER);
	gtk_entry_set_text (GTK_ENTRY (GET_WIDGET ("footer_entry")), s_value);
	g_free (s_value);

	if ((data->location != NULL) && g_file_has_uri_scheme (data->location->file, "file"))
		s_value = g_file_get_uri (data->location->file);
	else
		s_value = _g_settings_get_uri (data->settings, PREF_CONTACT_SHEET_DESTINATION);
	if (s_value == NULL)
		s_value = g_strdup (_g_uri_get_home ());
	gtk_file_chooser_set_uri (GTK_FILE_CHOOSER (GET_WIDGET ("destination_filechooserbutton")), s_value);
	g_free (s_value);

	s_value = _g_settings_get_uri (data->settings, PREF_CONTACT_SHEET_TEMPLATE);
	gtk_entry_set_text (GTK_ENTRY (GET_WIDGET ("template_entry")), s_value);
	g_free (s_value);

	default_mime_type = g_settings_get_string (data->settings, PREF_CONTACT_SHEET_MIME_TYPE);
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

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(GET_WIDGET("image_map_checkbutton")), g_settings_get_boolean (data->settings, PREF_CONTACT_SHEET_HTML_IMAGE_MAP));

	load_themes (data);
	gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (GET_WIDGET ("theme_liststore")),
					      THEME_COLUMN_DISPLAY_NAME,
					      GTK_SORT_ASCENDING);

	gtk_spin_button_set_value (GTK_SPIN_BUTTON (GET_WIDGET ("images_per_index_spinbutton")), g_settings_get_int (data->settings, PREF_CONTACT_SHEET_IMAGES_PER_PAGE));
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("single_index_checkbutton")), g_settings_get_boolean (data->settings, PREF_CONTACT_SHEET_SINGLE_PAGE));
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (GET_WIDGET ("cols_spinbutton")), g_settings_get_int (data->settings, PREF_CONTACT_SHEET_COLUMNS));
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("same_size_checkbutton")), g_settings_get_boolean (data->settings, PREF_CONTACT_SHEET_SAME_SIZE));

	default_sort_type = g_settings_get_string (data->settings, PREF_CONTACT_SHEET_SORT_TYPE);
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
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("reverse_order_checkbutton")), g_settings_get_boolean (data->settings, PREF_CONTACT_SHEET_SORT_INVERSE));

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
	gtk_combo_box_set_active (GTK_COMBO_BOX (GET_WIDGET ("thumbnail_size_combobox")), get_idx_from_size (g_settings_get_int (data->settings, PREF_CONTACT_SHEET_THUMBNAIL_SIZE)));
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("squared_thumbnail_checkbutton")), g_settings_get_boolean (data->settings, PREF_CONTACT_SHEET_SQUARED_THUMBNAIL));

	caption = g_settings_get_string (data->settings, PREF_CONTACT_SHEET_THUMBNAIL_CAPTION);
	gth_metadata_chooser_set_selection (GTH_METADATA_CHOOSER (data->thumbnail_caption_chooser), caption);
	g_free (caption);

	update_sensitivity (data);

	/* Set the signals handlers. */

	g_signal_connect (G_OBJECT (data->dialog),
			  "destroy",
			  G_CALLBACK (destroy_cb),
			  data);
	g_signal_connect (G_OBJECT (data->dialog),
			  "delete-event",
			  G_CALLBACK (dialog_delete_event_cb),
			  data);
	g_signal_connect (gtk_dialog_get_widget_for_response (GTK_DIALOG (data->dialog), GTK_RESPONSE_OK),
			  "clicked",
			  G_CALLBACK (ok_clicked_cb),
			  data);
	g_signal_connect_swapped (gtk_dialog_get_widget_for_response (GTK_DIALOG (data->dialog), GTK_RESPONSE_CANCEL),
				  "clicked",
				  G_CALLBACK (close_dialog),
				  data);
	g_signal_connect (GET_WIDGET ("footer_entry"),
			  "icon-press",
			  G_CALLBACK (entry_help_icon_press_cb),
			  data);
	g_signal_connect (GET_WIDGET ("template_entry"),
			  "icon-press",
			  G_CALLBACK (entry_help_icon_press_cb),
			  data);
	g_signal_connect_swapped (GET_WIDGET ("single_index_checkbutton"),
				  "toggled",
				  G_CALLBACK (update_sensitivity),
				  data);
	g_signal_connect (GET_WIDGET ("edit_theme_button"),
			  "clicked",
			  G_CALLBACK (edit_theme_button_clicked_cb),
			  data);
	g_signal_connect (GET_WIDGET ("add_theme_button"),
			  "clicked",
			  G_CALLBACK (add_theme_button_clicked_cb),
			  data);
	g_signal_connect (GET_WIDGET ("delete_theme_button"),
			  "clicked",
			  G_CALLBACK (delete_theme_button_clicked_cb),
			  data);
	data->theme_selection_changed_event =
			g_signal_connect (GET_WIDGET ("theme_iconview"),
					  "selection-changed",
					  G_CALLBACK (theme_iconview_selection_changed_cb),
					  data);
	g_signal_connect (GET_WIDGET ("edit_header_button"),
			  "clicked",
			  G_CALLBACK (edit_header_button_clicked_cb),
			  data);
	g_signal_connect (GET_WIDGET ("edit_footer_button"),
			  "clicked",
			  G_CALLBACK (edit_footer_button_clicked_cb),
			  data);
	g_signal_connect (GET_WIDGET ("edit_filename_button"),
			  "clicked",
			  G_CALLBACK (edit_filename_button_clicked_cb),
			  data);

	/* Run dialog. */

	gtk_widget_show (data->dialog);
}
