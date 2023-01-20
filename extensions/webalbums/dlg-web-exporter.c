/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2003-2010 Free Software Foundation, Inc.
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
#include "dlg-web-exporter.h"
#include "gth-web-exporter.h"
#include "preferences.h"


#define GET_WIDGET(name) _gtk_builder_get_widget (data->builder, (name))
#define STRING_IS_VOID(x) (((x) == NULL) || (*(x) == 0))


static GthTemplateCode Album_Header_Special_Codes[] = {
	{ GTH_TEMPLATE_CODE_TYPE_SIMPLE, N_("Current page number"), 'p', 0 },
	{ GTH_TEMPLATE_CODE_TYPE_SIMPLE, N_("Total number of pages"), 'P', 0 },
	{ GTH_TEMPLATE_CODE_TYPE_SIMPLE, N_("Location"), 'L', 0 },
	{ GTH_TEMPLATE_CODE_TYPE_DATE, N_("Current date"), 'D', 1 },
};


static GthTemplateCode Image_Header_Special_Codes[] = {
	{ GTH_TEMPLATE_CODE_TYPE_SIMPLE, N_("Current image number"), 'i', 0 },
	{ GTH_TEMPLATE_CODE_TYPE_SIMPLE, N_("Total number of images"), 'I', 0 },
	{ GTH_TEMPLATE_CODE_TYPE_SIMPLE, N_("Original filename"), 'F', 0 },
	{ GTH_TEMPLATE_CODE_TYPE_SIMPLE, N_("Comment"), 'C', 0 },
	{ GTH_TEMPLATE_CODE_TYPE_DATE, N_("Current date"), 'D', 1 },
};


enum {
	THEME_COLUMN_ID,
	THEME_COLUMN_NAME,
	THEME_COLUMN_PREVIEW
};

enum {
	SORT_TYPE_COLUMN_DATA,
	SORT_TYPE_COLUMN_NAME
};

typedef struct {
	GthBrowser  *browser;
	GthFileData *location;
	GSettings   *settings;
	GList       *file_list;
	GtkBuilder  *builder;
	GtkWidget   *dialog;
	GtkWidget   *thumbnail_caption_chooser;
	GtkWidget   *image_attributes_chooser;
} DialogData;


static void
destroy_cb (GtkWidget  *widget,
	    DialogData *data)
{
	gth_browser_set_dialog (data->browser, "web_exporter", NULL);
	_g_object_list_unref (data->file_list);
	g_object_unref (data->settings);
	g_object_unref (data->location);
	g_object_unref (data->builder);
	g_free (data);
}


static void
ok_clicked_cb (GtkWidget  *widget,
	       DialogData *data)
{
	char            *s_value;
	GFile           *destination;
	int              i_value;
	const char      *header;
	const char      *footer;
	const char      *image_page_header;
	const char      *image_page_footer;
	char            *thumbnail_caption;
	char            *image_attributes;
	GtkTreeIter      iter;
	char            *theme_name;
	GthFileDataSort *sort_type;
	GthTask         *task;

	/* save the options */

	s_value = gtk_file_chooser_get_uri (GTK_FILE_CHOOSER (GET_WIDGET ("destination_filechooserbutton")));
	destination = g_file_new_for_uri (s_value);
	_g_settings_set_uri (data->settings, PREF_WEBALBUMS_DESTINATION, s_value);
	g_free (s_value);

	g_settings_set_boolean (data->settings, PREF_WEBALBUMS_COPY_IMAGES, gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("copy_images_checkbutton"))));
	g_settings_set_boolean (data->settings, PREF_WEBALBUMS_RESIZE_IMAGES, gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("resize_images_checkbutton"))));

	i_value = gtk_combo_box_get_active (GTK_COMBO_BOX (GET_WIDGET ("resize_images_combobox")));
	g_settings_set_int (data->settings, PREF_WEBALBUMS_RESIZE_WIDTH, ImageSizeValues[i_value].width);
	g_settings_set_int (data->settings, PREF_WEBALBUMS_RESIZE_HEIGHT, ImageSizeValues[i_value].height);

	g_settings_set_int (data->settings, PREF_WEBALBUMS_IMAGES_PER_INDEX, gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (GET_WIDGET ("images_per_index_spinbutton"))));
	g_settings_set_boolean (data->settings, PREF_WEBALBUMS_SINGLE_INDEX, gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("single_index_checkbutton"))));
	g_settings_set_int (data->settings, PREF_WEBALBUMS_COLUMNS, gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (GET_WIDGET ("cols_spinbutton"))));
	g_settings_set_boolean (data->settings, PREF_WEBALBUMS_ADAPT_TO_WIDTH, gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("adapt_column_checkbutton"))));

	if (gtk_combo_box_get_active_iter (GTK_COMBO_BOX (GET_WIDGET ("sort_combobox")), &iter)) {
		GthFileDataSort *sort_type;

		gtk_tree_model_get (GTK_TREE_MODEL (GET_WIDGET ("sort_liststore")),
				    &iter,
				    SORT_TYPE_COLUMN_DATA, &sort_type,
				    -1);
		g_settings_set_string (data->settings, PREF_WEBALBUMS_SORT_TYPE, sort_type->name);
	}

	g_settings_set_boolean (data->settings, PREF_WEBALBUMS_SORT_INVERSE, gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("reverse_order_checkbutton"))));

	header = gtk_entry_get_text (GTK_ENTRY (GET_WIDGET ("header_entry")));
	g_settings_set_string (data->settings, PREF_WEBALBUMS_HEADER, header);

	footer = gtk_entry_get_text (GTK_ENTRY (GET_WIDGET ("footer_entry")));
	g_settings_set_string (data->settings, PREF_WEBALBUMS_FOOTER, footer);

	image_page_header = gtk_entry_get_text (GTK_ENTRY (GET_WIDGET ("image_page_header_entry")));
	g_settings_set_string (data->settings, PREF_WEBALBUMS_IMAGE_PAGE_HEADER, image_page_header);

	image_page_footer = gtk_entry_get_text (GTK_ENTRY (GET_WIDGET ("image_page_footer_entry")));
	g_settings_set_string (data->settings, PREF_WEBALBUMS_IMAGE_PAGE_FOOTER, image_page_footer);

	theme_name = NULL;
	{
		GList *list;

		list = gtk_icon_view_get_selected_items (GTK_ICON_VIEW (GET_WIDGET ("theme_iconview")));
		if (list != NULL) {
			GtkTreePath *path;
			GtkTreeIter  iter;

			path = g_list_first (list)->data;
			gtk_tree_model_get_iter (GTK_TREE_MODEL (GET_WIDGET ("theme_liststore")), &iter, path);
			gtk_tree_model_get (GTK_TREE_MODEL (GET_WIDGET ("theme_liststore")), &iter,
					    THEME_COLUMN_NAME, &theme_name,
					    -1);
		}

		g_list_foreach (list, (GFunc) gtk_tree_path_free, NULL);
		g_list_free (list);
	}
	g_return_if_fail (theme_name != NULL);
	g_settings_set_string (data->settings, PREF_WEBALBUMS_THEME, theme_name);

	g_settings_set_boolean (data->settings, PREF_WEBALBUMS_ENABLE_THUMBNAIL_CAPTION, gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("thumbnail_caption_checkbutton"))));

	thumbnail_caption = gth_metadata_chooser_get_selection (GTH_METADATA_CHOOSER (data->thumbnail_caption_chooser));
	g_settings_set_string (data->settings, PREF_WEBALBUMS_THUMBNAIL_CAPTION, thumbnail_caption);

	g_settings_set_boolean (data->settings, PREF_WEBALBUMS_ENABLE_IMAGE_ATTRIBUTES, gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("image_attributes_checkbutton"))));

	g_settings_set_boolean (data->settings, PREF_WEBALBUMS_ENABLE_IMAGE_DESCRIPTION, gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("image_description_checkbutton"))));

	image_attributes = gth_metadata_chooser_get_selection (GTH_METADATA_CHOOSER (data->image_attributes_chooser));
	g_settings_set_string (data->settings, PREF_WEBALBUMS_IMAGE_ATTRIBUTES, image_attributes);

	/* exec the task */

	task = gth_web_exporter_new (data->browser, data->file_list);

	gth_web_exporter_set_header (GTH_WEB_EXPORTER (task), header);
	gth_web_exporter_set_footer (GTH_WEB_EXPORTER (task), footer);
	gth_web_exporter_set_image_page_header (GTH_WEB_EXPORTER (task), image_page_header);
	gth_web_exporter_set_image_page_footer (GTH_WEB_EXPORTER (task), image_page_footer);
	gth_web_exporter_set_style (GTH_WEB_EXPORTER (task), theme_name);
	gth_web_exporter_set_destination (GTH_WEB_EXPORTER (task), destination);
	gth_web_exporter_set_copy_images (GTH_WEB_EXPORTER (task),
					  g_settings_get_boolean (data->settings, PREF_WEBALBUMS_COPY_IMAGES));
	gth_web_exporter_set_resize_images (GTH_WEB_EXPORTER (task),
					    g_settings_get_boolean (data->settings, PREF_WEBALBUMS_RESIZE_IMAGES),
					    g_settings_get_int (data->settings, PREF_WEBALBUMS_RESIZE_WIDTH),
					    g_settings_get_int (data->settings, PREF_WEBALBUMS_RESIZE_HEIGHT));

	s_value = g_settings_get_string (data->settings, PREF_WEBALBUMS_SORT_TYPE);
	sort_type = gth_main_get_sort_type (s_value);
	gth_web_exporter_set_sort_order (GTH_WEB_EXPORTER (task),
					 sort_type,
					 g_settings_get_boolean (data->settings, PREF_WEBALBUMS_SORT_INVERSE));
	g_free (s_value);

	gth_web_exporter_set_images_per_index (GTH_WEB_EXPORTER (task),
					       g_settings_get_int (data->settings, PREF_WEBALBUMS_IMAGES_PER_INDEX));
	gth_web_exporter_set_single_index (GTH_WEB_EXPORTER (task),
					   g_settings_get_boolean (data->settings, PREF_WEBALBUMS_SINGLE_INDEX));
	gth_web_exporter_set_columns (GTH_WEB_EXPORTER (task),
				      g_settings_get_int (data->settings, PREF_WEBALBUMS_COLUMNS));
	gth_web_exporter_set_adapt_to_width (GTH_WEB_EXPORTER (task),
					     g_settings_get_boolean (data->settings, PREF_WEBALBUMS_ADAPT_TO_WIDTH));
	gth_web_exporter_set_thumbnail_caption (GTH_WEB_EXPORTER (task),
						gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("thumbnail_caption_checkbutton"))) ? thumbnail_caption : "");
	gth_web_exporter_set_image_attributes (GTH_WEB_EXPORTER (task),
					       gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("image_description_checkbutton"))),
					       gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("image_attributes_checkbutton"))) ? image_attributes : "");

	gth_browser_exec_task (data->browser, task, GTH_TASK_FLAGS_DEFAULT);
	gtk_widget_destroy (data->dialog);

	g_object_unref (task);
	g_free (image_attributes);
	g_free (thumbnail_caption);
	g_free (theme_name);
	g_object_unref (destination);
}


static void
update_sensitivity (DialogData *data)
{
	gtk_widget_set_sensitive (GET_WIDGET ("resize_images_combobox"), gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("resize_images_checkbutton"))));
	gtk_widget_set_sensitive (GET_WIDGET ("resize_images_hbox"), gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("copy_images_checkbutton"))));
	gtk_widget_set_sensitive (GET_WIDGET ("images_per_index_spinbutton"), ! gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("single_index_checkbutton"))));
	gtk_widget_set_sensitive (GET_WIDGET ("cols_spinbutton"), ! gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("adapt_column_checkbutton"))));
	gtk_widget_set_sensitive (data->image_attributes_chooser, gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("image_attributes_checkbutton"))));
	gtk_widget_set_sensitive (data->thumbnail_caption_chooser, gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("thumbnail_caption_checkbutton"))));
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

	case 'P':
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

	case 'i':
		g_string_append (result, "1");
		break;

	case 'I':
		g_string_append_printf (result, "%d", g_list_length (data->file_list));
		break;

	case 'F':
		_g_string_append_markup_escaped (result, "%s", "filename.jpeg");
		break;

	case 'C':
		_g_string_append_markup_escaped (result, "%s", _("Comment"));
		break;

	case 'L':
		_g_string_append_markup_escaped (result, "%s", g_file_info_get_edit_name (data->location->info));
		break;

	default:
		break;
	}

	if (code != 0)
		g_string_append (result, "</span>");

	return FALSE;
}


static void
album_header_button_clicked_cb (DialogData *data,
				const char *entry_name)
{
	GtkWidget *dialog;

	dialog = gth_template_editor_dialog_new (Album_Header_Special_Codes,
						 G_N_ELEMENTS (Album_Header_Special_Codes),
						 0,
						 _("Edit Template"),
						 GTK_WINDOW (data->dialog));
	gth_template_editor_dialog_set_preview_cb (GTH_TEMPLATE_EDITOR_DIALOG (dialog),
						   text_preview_cb,
						   data);
	gth_template_editor_dialog_set_template (GTH_TEMPLATE_EDITOR_DIALOG (dialog),
						 gtk_entry_get_text (GTK_ENTRY (GET_WIDGET (entry_name))));
	g_signal_connect (dialog,
			  "response",
			  G_CALLBACK (gth_template_editor_dialog_default_response),
			  GET_WIDGET (entry_name));
	gtk_window_set_modal (GTK_WINDOW (dialog), TRUE);
	gtk_window_present (GTK_WINDOW (dialog));
}


static void
edit_header_button_clicked_cb (GtkButton *button,
			       gpointer   user_data)
{
	album_header_button_clicked_cb ((DialogData *) user_data, "header_entry");
}


static void
edit_footer_button_clicked_cb (GtkButton *button,
			       gpointer   user_data)
{
	album_header_button_clicked_cb ((DialogData *) user_data, "footer_entry");
}


static void
album_image_page_header_button_clicked_cb (DialogData *data,
					   const char *entry_name)
{
	GtkWidget *dialog;

	dialog = gth_template_editor_dialog_new (Image_Header_Special_Codes,
						 G_N_ELEMENTS (Image_Header_Special_Codes),
						 0,
						 _("Edit Template"),
						 GTK_WINDOW (data->dialog));
	gth_template_editor_dialog_set_preview_cb (GTH_TEMPLATE_EDITOR_DIALOG (dialog),
						   text_preview_cb,
						   data);
	gth_template_editor_dialog_set_template (GTH_TEMPLATE_EDITOR_DIALOG (dialog),
						 gtk_entry_get_text (GTK_ENTRY (GET_WIDGET (entry_name))));
	g_signal_connect (dialog,
			  "response",
			  G_CALLBACK (gth_template_editor_dialog_default_response),
			  GET_WIDGET (entry_name));
	gtk_window_set_modal (GTK_WINDOW (dialog), TRUE);
	gtk_window_present (GTK_WINDOW (dialog));
}


static void
edit_image_page_header_button_clicked_cb (GtkButton *button,
					  gpointer   user_data)
{
	album_image_page_header_button_clicked_cb ((DialogData *) user_data, "image_page_header_entry");
}


static void
edit_image_page_footer_button_clicked_cb (GtkButton *button,
					  gpointer   user_data)
{
	album_image_page_header_button_clicked_cb ((DialogData *) user_data, "image_page_footer_entry");
}


static void
add_themes_from_dir (DialogData *data,
		     GFile      *dir)
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
		GFile     *file;
		char      *filename;
		GdkPixbuf *preview;

		if (g_file_info_get_file_type (file_info) != G_FILE_TYPE_DIRECTORY) {
			g_object_unref (file_info);
			continue;
		}

		file = _g_file_get_child (dir, g_file_info_get_name (file_info), "preview.png", NULL);
		filename = g_file_get_path (file);
		preview = gdk_pixbuf_new_from_file_at_size (filename, 128, 128, NULL);
		if (preview != NULL) {
			GtkTreeIter iter;

			gtk_list_store_append (GTK_LIST_STORE (GET_WIDGET ("theme_liststore")), &iter);
			gtk_list_store_set (GTK_LIST_STORE (GET_WIDGET ("theme_liststore")), &iter,
					    THEME_COLUMN_ID, g_file_info_get_name (file_info),
					    THEME_COLUMN_NAME, g_file_info_get_display_name (file_info),
					    THEME_COLUMN_PREVIEW, preview,
					    -1);
		}

		g_object_unref (preview);
		g_free (filename);
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

	style_dir = gth_user_dir_get_file_for_read (GTH_DIR_DATA, GTHUMB_DIR, "albumthemes", NULL);
	add_themes_from_dir (data, style_dir);
	g_object_unref (style_dir);

	/* system themes */

	data_dir = g_file_new_for_path (WEBALBUM_DATADIR);
	style_dir = _g_file_get_child (data_dir, "albumthemes", NULL);
	add_themes_from_dir (data, style_dir);
	g_object_unref (style_dir);
	g_object_unref (data_dir);

	/**/

	gtk_widget_set_size_request (GET_WIDGET ("theme_iconview"), (150 * 3), 140);
	gtk_widget_realize (GET_WIDGET ("theme_iconview"));

	default_theme = g_settings_get_string (data->settings, PREF_WEBALBUMS_THEME);

	model = GTK_TREE_MODEL (GET_WIDGET ("theme_liststore"));
	if (gtk_tree_model_get_iter_first (model, &iter)) {
		do {
			char *name;

			gtk_tree_model_get(model, &iter, THEME_COLUMN_ID, &name, -1);

			if (g_strcmp0 (name, default_theme) == 0) {
				GtkTreePath *path;

				path = gtk_tree_model_get_path (model, &iter);
				gtk_icon_view_select_path (GTK_ICON_VIEW (GET_WIDGET ("theme_iconview")), path);
				gtk_icon_view_scroll_to_path (GTK_ICON_VIEW (GET_WIDGET ("theme_iconview")), path, TRUE, 0.5, 0.5);

				gtk_tree_path_free (path);
				g_free (name);
				break;
			}

			g_free (name);
		}
		while (gtk_tree_model_iter_next (model, &iter));
	}

	g_free (default_theme);
}


void
dlg_web_exporter (GthBrowser *browser,
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

	if (gth_browser_get_dialog (browser, "web_exporter") != NULL) {
		gtk_window_present (GTK_WINDOW (gth_browser_get_dialog (browser, "web_exporter")));
		return;
	}

	data = g_new0 (DialogData, 1);
	data->browser = browser;
	data->location = gth_file_data_dup (gth_browser_get_location_data (browser));
	data->file_list = _g_object_list_ref (file_list);
	data->builder = _gtk_builder_new_from_file ("web-album-exporter.ui", "webalbums");
	data->settings = g_settings_new (GTHUMB_WEBALBUMS_SCHEMA);

	data->dialog = g_object_new (GTK_TYPE_DIALOG,
				     "title", _("Web Album"),
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

	gth_browser_set_dialog (browser, "web_exporter", data->dialog);
	g_object_set_data (G_OBJECT (data->dialog), "dialog_data", data);

	data->thumbnail_caption_chooser = gth_metadata_chooser_new (GTH_METADATA_ALLOW_IN_FILE_LIST, TRUE);
	gtk_widget_show (data->thumbnail_caption_chooser);
	gtk_container_add (GTK_CONTAINER (GET_WIDGET ("thumbnail_caption_scrolledwindow")), data->thumbnail_caption_chooser);

	data->image_attributes_chooser = gth_metadata_chooser_new (GTH_METADATA_ALLOW_IN_PROPERTIES_VIEW, TRUE);
	gtk_widget_show (data->image_attributes_chooser);
	gtk_container_add (GTK_CONTAINER (GET_WIDGET ("image_caption_scrolledwindow")), data->image_attributes_chooser);

	/* Set widgets data. */

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("copy_images_checkbutton")),
				      g_settings_get_boolean (data->settings, PREF_WEBALBUMS_COPY_IMAGES));
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("resize_images_checkbutton")),
				      g_settings_get_boolean (data->settings, PREF_WEBALBUMS_RESIZE_IMAGES));
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (GET_WIDGET ("images_per_index_spinbutton")), g_settings_get_int (data->settings, PREF_WEBALBUMS_IMAGES_PER_INDEX));
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("single_index_checkbutton")),
				      g_settings_get_boolean (data->settings, PREF_WEBALBUMS_SINGLE_INDEX));
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (GET_WIDGET ("cols_spinbutton")),
				   g_settings_get_int (data->settings, PREF_WEBALBUMS_COLUMNS));
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("adapt_column_checkbutton")),
				      g_settings_get_boolean (data->settings, PREF_WEBALBUMS_ADAPT_TO_WIDTH));

	_gtk_combo_box_add_image_sizes (GTK_COMBO_BOX (GET_WIDGET ("resize_images_combobox")),
					g_settings_get_int (data->settings, PREF_WEBALBUMS_RESIZE_WIDTH),
					g_settings_get_int (data->settings, PREF_WEBALBUMS_RESIZE_HEIGHT));

	default_sort_type = g_settings_get_string (data->settings, PREF_WEBALBUMS_SORT_TYPE);
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

	gtk_combo_box_set_active (GTK_COMBO_BOX (GET_WIDGET ("sort_combobox")), active_index);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("reverse_order_checkbutton")), g_settings_get_boolean (data->settings, PREF_WEBALBUMS_SORT_INVERSE));

	g_free (default_sort_type);

	s_value = g_settings_get_string (data->settings, PREF_WEBALBUMS_HEADER);
	gtk_entry_set_text (GTK_ENTRY (GET_WIDGET ("header_entry")), s_value);
	g_free (s_value);

	s_value = g_settings_get_string (data->settings, PREF_WEBALBUMS_FOOTER);
	gtk_entry_set_text (GTK_ENTRY (GET_WIDGET ("footer_entry")), s_value);
	g_free (s_value);

	s_value = g_settings_get_string (data->settings, PREF_WEBALBUMS_IMAGE_PAGE_HEADER);
	gtk_entry_set_text (GTK_ENTRY (GET_WIDGET ("image_page_header_entry")), s_value);
	g_free (s_value);

	s_value = g_settings_get_string (data->settings, PREF_WEBALBUMS_IMAGE_PAGE_FOOTER);
	gtk_entry_set_text (GTK_ENTRY (GET_WIDGET ("image_page_footer_entry")), s_value);
	g_free (s_value);

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("thumbnail_caption_checkbutton")), g_settings_get_boolean (data->settings, PREF_WEBALBUMS_ENABLE_THUMBNAIL_CAPTION));

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("image_description_checkbutton")), g_settings_get_boolean (data->settings, PREF_WEBALBUMS_ENABLE_IMAGE_DESCRIPTION));

	caption = g_settings_get_string (data->settings, PREF_WEBALBUMS_THUMBNAIL_CAPTION);
	gth_metadata_chooser_set_selection (GTH_METADATA_CHOOSER (data->thumbnail_caption_chooser), caption);
	g_free (caption);

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("image_attributes_checkbutton")), g_settings_get_boolean (data->settings, PREF_WEBALBUMS_ENABLE_IMAGE_ATTRIBUTES));

	caption = g_settings_get_string (data->settings, PREF_WEBALBUMS_IMAGE_ATTRIBUTES);
	gth_metadata_chooser_set_selection (GTH_METADATA_CHOOSER (data->image_attributes_chooser), caption);
	g_free (caption);

	gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (GET_WIDGET ("theme_liststore")),
					      THEME_COLUMN_NAME,
					      GTK_SORT_ASCENDING);

	load_themes (data);
	update_sensitivity (data);

	{
		char *destination;

		destination = _g_settings_get_uri (data->settings, PREF_WEBALBUMS_DESTINATION);
		if (destination == NULL)
			destination = g_strdup (_g_uri_get_home ());
		gtk_file_chooser_set_uri (GTK_FILE_CHOOSER (GET_WIDGET ("destination_filechooserbutton")), destination);

		g_free (destination);
	}

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
	g_signal_connect_swapped (GET_WIDGET ("copy_images_checkbutton"),
				  "clicked",
				  G_CALLBACK (update_sensitivity),
				  data);
	g_signal_connect_swapped (GET_WIDGET ("resize_images_checkbutton"),
				  "clicked",
				  G_CALLBACK (update_sensitivity),
				  data);
	g_signal_connect_swapped (GET_WIDGET ("single_index_checkbutton"),
				  "toggled",
				  G_CALLBACK (update_sensitivity),
				  data);
	g_signal_connect_swapped (GET_WIDGET ("adapt_column_checkbutton"),
				  "toggled",
				  G_CALLBACK (update_sensitivity),
				  data);
	g_signal_connect_swapped (GET_WIDGET ("image_attributes_checkbutton"),
				  "toggled",
				  G_CALLBACK (update_sensitivity),
				  data);
	g_signal_connect_swapped (GET_WIDGET ("thumbnail_caption_checkbutton"),
				  "toggled",
				  G_CALLBACK (update_sensitivity),
				  data);
	g_signal_connect (GET_WIDGET ("edit_header_button"),
			  "clicked",
			  G_CALLBACK (edit_header_button_clicked_cb),
			  data);
	g_signal_connect (GET_WIDGET ("edit_footer_button"),
			  "clicked",
			  G_CALLBACK (edit_footer_button_clicked_cb),
			  data);
	g_signal_connect (GET_WIDGET ("edit_image_page_header_button"),
			  "clicked",
			  G_CALLBACK (edit_image_page_header_button_clicked_cb),
			  data);
	g_signal_connect (GET_WIDGET ("edit_image_page_footer_button"),
			  "clicked",
			  G_CALLBACK (edit_image_page_footer_button_clicked_cb),
			  data);

	/* Run dialog. */

	gtk_widget_show (data->dialog);
}
