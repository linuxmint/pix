/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2001-2009 The Free Software Foundation, Inc.
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
#include <gthumb.h>
#include "dlg-add-to-catalog.h"
#include "gth-catalog.h"
#include "gth-file-source-catalogs.h"
#include "preferences.h"


#define GET_WIDGET(name) _gtk_builder_get_widget (data->builder, (name))
#define ADD_TO_CATALOG_DIALOG_NAME "add-to-catalog"
#define UPDATE_SELECTION_DELAY 50


typedef struct {
	int            ref;
	GthBrowser    *browser;
	GtkWidget     *parent_window;
	GtkWidget     *dialog;
	GList         *files;
	gboolean       view_destination;
	gboolean       close_after_adding;
	GFile         *catalog_file;
	GthCatalog    *catalog;
} AddData;



static AddData *
add_data_new (void)
{
	AddData *add_data;

	add_data = g_new0 (AddData, 1);
	add_data->ref = 0;
	add_data->view_destination = FALSE;
	add_data->close_after_adding = TRUE;

	return add_data;
}


static void
add_data_ref (AddData *add_data)
{
	add_data->ref++;
}


static void
add_data_unref (AddData *add_data)
{
	add_data->ref--;
	if (add_data->ref > 0)
		return;

	_g_object_unref (add_data->catalog);
	_g_object_list_unref (add_data->files);
	_g_object_unref (add_data->catalog_file);
	g_free (add_data);
}


typedef struct {
	GthBrowser    *browser;
	GtkBuilder    *builder;
	GtkWidget     *dialog;
	GtkWidget     *keep_open_checkbutton;
	GtkWidget     *source_tree;
	GtkWidget     *info;
	AddData       *add_data;
	GthFileSource *catalog_source;
	GthFileData   *new_catalog;
	GthFileData   *new_library;
	gulong         file_selection_changed_event;
	guint          update_selection_event;
	GSettings     *settings;
} DialogData;


static void
destroy_cb (GtkWidget  *widget,
	    DialogData *data)
{
	gth_browser_set_dialog (data->browser, ADD_TO_CATALOG_DIALOG_NAME, NULL);
	if (data->file_selection_changed_event != 0) {
		g_signal_handler_disconnect (gth_browser_get_file_list_view (data->browser),
					     data->file_selection_changed_event);
		data->file_selection_changed_event = 0;
	}
	if (data->update_selection_event != 0) {
		g_source_remove (data->update_selection_event);
		data->update_selection_event = 0;
	}
	add_data_unref (data->add_data);
	_g_object_unref (data->catalog_source);
	_g_object_unref (data->new_catalog);
	_g_object_unref (data->new_library);
	_g_object_unref (data->settings);
	g_object_unref (data->builder);
	g_free (data);
}


static GFile *
get_selected_catalog (DialogData *data)
{
	GthFileData *file_data;
	GFile       *file;

	file_data = gth_folder_tree_get_selected_or_parent (GTH_FOLDER_TREE (data->source_tree));
	if ((file_data != NULL) && ! g_file_info_get_attribute_boolean (file_data->info, "gthumb::no-child")) {
		_g_object_unref (file_data);
		file_data = NULL;
	}
	if (file_data != NULL)
		file = g_object_ref (file_data->file);
	else
		file = NULL;

	_g_object_unref (file_data);

	return file;
}


static void
catalog_save_done_cb (void     **buffer,
		      gsize      count,
		      GError    *error,
		      gpointer   user_data)
{
	AddData *add_data = user_data;

	if (error != NULL) {
		_gtk_error_dialog_from_gerror_show (GTK_WINDOW (add_data->parent_window), _("Could not add the files to the catalog"), error);
		add_data_unref (add_data);
		return;
	}

	gth_monitor_folder_changed (gth_main_get_default_monitor (),
				    add_data->catalog_file,
				    add_data->files,
				    GTH_MONITOR_EVENT_CREATED);

	if (add_data->close_after_adding) {
		if (add_data->view_destination)
			gth_browser_go_to (add_data->browser, add_data->catalog_file, NULL);

		if (add_data->dialog != NULL)
			gtk_widget_destroy (add_data->dialog);
	}
	else
		gth_browser_show_next_image (add_data->browser, FALSE, FALSE);

	add_data_unref (add_data);
}


static void
catalog_ready_cb (GObject  *catalog,
		  GError   *error,
		  gpointer  user_data)
{
	AddData *add_data = user_data;
	GList   *scan;
	char    *buffer;
	gsize    length;
	GFile   *gio_file;

	_g_object_unref (add_data->catalog);
	if (catalog != NULL)
		add_data->catalog = (GthCatalog *) catalog;
	else
		add_data->catalog = gth_catalog_new_for_file (add_data->catalog_file);

	for (scan = add_data->files; scan; scan = scan->next)
		gth_catalog_insert_file (add_data->catalog, (GFile *) scan->data, -1);

	buffer = gth_catalog_to_data (add_data->catalog, &length);
	gio_file = gth_catalog_file_to_gio_file (add_data->catalog_file);
	_g_file_write_async (gio_file,
			     buffer,
			     length,
			     TRUE,
			     G_PRIORITY_DEFAULT,
			     NULL,
			     catalog_save_done_cb,
			     add_data);

	g_object_unref (gio_file);
}


static void
add_data_exec (AddData *add_data)
{
	add_data_ref (add_data);
	gth_catalog_load_from_file_async (add_data->catalog_file,
					  NULL,
					  catalog_ready_cb,
					  add_data);
}


static void
add_selection_to_catalog (DialogData *data,
			  gboolean    close_after_adding)
{
	char  *last_catalog;
	GList *items;
	GList *file_list;

	_g_clear_object (&data->add_data->catalog_file);
	data->add_data->catalog_file = get_selected_catalog (data);
	if (data->add_data->catalog_file == NULL)
		return;

	last_catalog = g_file_get_uri (data->add_data->catalog_file);
	g_settings_set_string (data->settings, PREF_CATALOGS_LAST_CATALOG, last_catalog);
	g_free (last_catalog);

	_g_object_list_unref (data->add_data->files);
	data->add_data->files = NULL;

	items = gth_file_selection_get_selected (GTH_FILE_SELECTION (gth_browser_get_file_list_view (data->browser)));
	file_list = gth_file_list_get_files (GTH_FILE_LIST (gth_browser_get_file_list (data->browser)), items);
	data->add_data->files = gth_file_data_list_to_file_list (file_list);
	if (data->add_data->files != NULL) {
		data->add_data->close_after_adding = close_after_adding;
		data->add_data->view_destination = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("view_destination_checkbutton")));
		add_data_exec (data->add_data);
	}

	_g_object_list_unref (file_list);
	_gtk_tree_path_list_free (items);
}


static void
add_button_clicked_cb (GtkWidget  *widget,
		       DialogData *data)
{
	add_selection_to_catalog (data, ! gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (data->keep_open_checkbutton)));
}


static void
update_sensitivity (DialogData *data)
{
	GFile    *selected_catalog;
	GList    *items;
	GList    *file_data_list;
	gboolean  can_add;

	selected_catalog = get_selected_catalog (data);
	items = gth_file_selection_get_selected (GTH_FILE_SELECTION (gth_browser_get_file_list_view (data->browser)));
	can_add = (items != NULL) && (selected_catalog != NULL);
	gtk_dialog_set_response_sensitive (GTK_DIALOG (data->dialog), GTK_RESPONSE_OK, can_add);
	gtk_toggle_button_set_inconsistent (GTK_TOGGLE_BUTTON (GET_WIDGET ("view_destination_checkbutton")), gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (data->keep_open_checkbutton)));
	gtk_widget_set_sensitive (GET_WIDGET ("view_destination_checkbutton"), ! gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (data->keep_open_checkbutton)));

	file_data_list = gth_file_list_get_files (GTH_FILE_LIST (gth_browser_get_file_list (data->browser)), items);
	gth_file_selection_info_set_file_list (GTH_FILE_SELECTION_INFO (data->info), file_data_list);

	_g_object_list_unref (file_data_list);
	_gtk_tree_path_list_free (items);
	_g_object_unref (selected_catalog);
}


static void
source_tree_changed_cb (GthVfsTree *folder_tree,
			gpointer    user_data)
{
	update_sensitivity ((DialogData *) user_data);
}


static void
source_tree_selection_changed_cb (GtkTreeSelection *treeselection,
				  gpointer          user_data)
{
	update_sensitivity ((DialogData *) user_data);
}


static void
new_catalog_metadata_ready_cb (GObject  *object,
			       GError   *error,
			       gpointer  user_data)
{
	DialogData  *data = user_data;
	GFile       *parent;
	GList       *file_list;
	GtkTreePath *tree_path;

	if (error != NULL) {
		_gtk_error_dialog_from_gerror_show (GTK_WINDOW (data->dialog), _("Could not create the catalog"), error);
		return;
	}

	/* add the new catalog to the tree and select it */

	parent = g_file_get_parent (data->new_catalog->file);
	file_list = g_list_append (NULL, g_object_ref (data->new_catalog));
	gth_folder_tree_add_children (GTH_FOLDER_TREE (data->source_tree),
				      parent,
				      file_list);

	tree_path = gth_folder_tree_get_path (GTH_FOLDER_TREE (data->source_tree), data->new_catalog->file);
	if (tree_path != NULL) {
		gth_folder_tree_select_path (GTH_FOLDER_TREE (data->source_tree), tree_path);
		gtk_tree_view_scroll_to_cell (GTK_TREE_VIEW (data->source_tree), tree_path, NULL, TRUE, 0.5, 0.0);
		gtk_tree_path_free (tree_path);
	}

	_g_object_list_unref (file_list);

	/* notify the catalog creation */

	file_list = g_list_prepend (NULL, g_object_ref (data->new_catalog->file));
	gth_monitor_folder_changed (gth_main_get_default_monitor (),
				    parent,
				    file_list,
				    GTH_MONITOR_EVENT_CREATED);

	_g_object_list_unref (file_list);

	g_object_unref (parent);
}


static void
new_catalog_dialog_response_cb (GtkWidget *dialog,
				int        response_id,
				gpointer   user_data)
{
	DialogData  *data = user_data;
	char        *name;
	GthFileData *selected_parent;
	GFile       *parent;
	GFile       *gio_parent;
	char        *display_name;
	GError      *error = NULL;
	GFile       *gio_file;

	if (response_id != GTK_RESPONSE_OK) {
		gtk_widget_destroy (dialog);
		return;
	}

	name = gth_request_dialog_get_normalized_text (GTH_REQUEST_DIALOG (dialog));
	if (_g_utf8_all_spaces (name)) {
		g_free (name);
		gth_request_dialog_set_info_text (GTH_REQUEST_DIALOG (dialog), GTK_MESSAGE_ERROR, _("No name specified"));
		return;
	}

	if (g_regex_match_simple ("/", name, 0, 0)) {
		char *message;

		message = g_strdup_printf (_("Invalid name. The following characters are not allowed: %s"), "/");
		gth_request_dialog_set_info_text (GTH_REQUEST_DIALOG (dialog), GTK_MESSAGE_ERROR, message);

		g_free (message);
		g_free (name);

		return;
	}

	selected_parent = gth_folder_tree_get_selected_or_parent (GTH_FOLDER_TREE (data->source_tree));
	if (selected_parent != NULL) {
		GthFileSource *file_source;
		GFileInfo     *info;

		file_source = gth_main_get_file_source (selected_parent->file);
		info = gth_file_source_get_file_info (file_source, selected_parent->file, GFILE_BASIC_ATTRIBUTES);
		if (g_file_info_get_attribute_boolean (info, "gthumb::no-child"))
			parent = g_file_get_parent (selected_parent->file);
		else
			parent = g_file_dup (selected_parent->file);

		g_object_unref (info);
		g_object_unref (file_source);
	}
	else
		parent = g_file_new_for_uri ("catalog:///");

	_g_object_unref (data->catalog_source);
	data->catalog_source = gth_main_get_file_source (parent);
	gio_parent = gth_file_source_to_gio_file (data->catalog_source, parent);
	display_name = g_strconcat (name, ".catalog", NULL);
	gio_file = g_file_get_child_for_display_name (gio_parent, display_name, &error);
	if (gio_file != NULL) {
		GFileOutputStream *stream;

		stream = g_file_create (gio_file, G_FILE_CREATE_NONE, NULL, &error);
		if (stream != NULL) {
			GFile *file;

			_g_object_unref (data->new_catalog);
			file = gth_catalog_file_from_gio_file (gio_file, NULL);
			data->new_catalog = gth_file_data_new (file, NULL);
			gth_file_source_read_metadata (data->catalog_source,
						       data->new_catalog,
						       "*",
						       new_catalog_metadata_ready_cb,
						       data);

			g_object_unref (file);
			g_object_unref (stream);
		}

		g_object_unref (gio_file);
	}

	if (error != NULL) {
		if (g_error_matches (error, G_IO_ERROR, G_IO_ERROR_EXISTS))
			gth_request_dialog_set_info_text (GTH_REQUEST_DIALOG (dialog), GTK_MESSAGE_ERROR, _("Name already used"));
		else
			gth_request_dialog_set_info_text (GTH_REQUEST_DIALOG (dialog), GTK_MESSAGE_ERROR, error->message);
		g_clear_error (&error);
	}
	else
		gtk_widget_destroy (dialog);

	g_free (display_name);
	g_object_unref (gio_parent);
}


static void
new_catalog_button_clicked_cb (GtkWidget  *widget,
			       DialogData *data)
{
	GtkWidget *dialog;

	dialog = gth_request_dialog_new (GTK_WINDOW (data->dialog),
					 GTK_DIALOG_MODAL,
					 _("New Catalog"),
					 _("Enter the catalog name:"),
					 _GTK_LABEL_CANCEL,
					 _("C_reate"));
	g_signal_connect (dialog,
			  "response",
			  G_CALLBACK (new_catalog_dialog_response_cb),
			  data);

	gtk_widget_show (dialog);
}


static void
new_library_metadata_ready_cb (GObject  *object,
			       GError   *error,
			       gpointer  user_data)
{
	DialogData  *data = user_data;
	GFile       *parent;
	GList       *file_list;
	GtkTreePath *tree_path;

	if (error != NULL)
		return;

	/* add the new library to the tree and select it */

	parent = g_file_get_parent (data->new_library->file);
	file_list = g_list_append (NULL, g_object_ref (data->new_library));
	gth_folder_tree_add_children (GTH_FOLDER_TREE (data->source_tree),
				      parent,
				      file_list);

	_g_object_list_unref (file_list);

	/* select the new library */

	tree_path = gth_folder_tree_get_path (GTH_FOLDER_TREE (data->source_tree), data->new_library->file);
	if (tree_path != NULL) {
		gth_folder_tree_select_path (GTH_FOLDER_TREE (data->source_tree), tree_path);
		gtk_tree_view_scroll_to_cell (GTK_TREE_VIEW (data->source_tree), tree_path, NULL, TRUE, 0.5, 0.0);
		gtk_tree_path_free (tree_path);
	}

	/* notify the library creation */

	file_list = g_list_prepend (NULL, g_object_ref (data->new_library->file));
	gth_monitor_folder_changed (gth_main_get_default_monitor (),
				    parent,
				    file_list,
				    GTH_MONITOR_EVENT_CREATED);

	_g_object_list_unref (file_list);
	g_object_unref (parent);
}


static void
new_library_dialog_response_cb (GtkWidget *dialog,
				int        response_id,
				gpointer   user_data)
{
	DialogData    *data = user_data;
	char          *name;
	GthFileData   *selected_parent;
	GFile         *parent;
	GthFileSource *file_source;
	GFile         *gio_parent;
	GError        *error = NULL;
	GFile         *gio_file;

	if (response_id != GTK_RESPONSE_OK) {
		gtk_widget_destroy (dialog);
		return;
	}

	name = gth_request_dialog_get_normalized_text (GTH_REQUEST_DIALOG (dialog));
	if (_g_utf8_all_spaces (name)) {
		g_free (name);
		gth_request_dialog_set_info_text (GTH_REQUEST_DIALOG (dialog), GTK_MESSAGE_ERROR, _("No name specified"));
		return;
	}

	if (g_regex_match_simple ("/", name, 0, 0)) {
		char *message;

		message = g_strdup_printf (_("Invalid name. The following characters are not allowed: %s"), "/");
		gth_request_dialog_set_info_text (GTH_REQUEST_DIALOG (dialog), GTK_MESSAGE_ERROR, message);

		g_free (message);
		g_free (name);

		return;
	}

	selected_parent = gth_folder_tree_get_selected_or_parent (GTH_FOLDER_TREE (data->source_tree));
	if (selected_parent != NULL) {
		GthFileSource *file_source;
		GFileInfo     *info;

		file_source = gth_main_get_file_source (selected_parent->file);
		info = gth_file_source_get_file_info (file_source, selected_parent->file, GFILE_BASIC_ATTRIBUTES);
		if (g_file_info_get_attribute_boolean (info, "gthumb::no-child"))
			parent = g_file_get_parent (selected_parent->file);
		else
			parent = g_file_dup (selected_parent->file);

		g_object_unref (info);
		g_object_unref (file_source);
	}
	else
		parent = g_file_new_for_uri ("catalog:///");

	file_source = gth_main_get_file_source (parent);
	gio_parent = gth_file_source_to_gio_file (file_source, parent);
	gio_file = g_file_get_child_for_display_name (gio_parent, name, &error);
	if ((gio_file != NULL) && g_file_make_directory (gio_file, NULL, &error)) {
		GFile *file;

		data->catalog_source = gth_main_get_file_source (parent);

		_g_object_unref (data->new_library);
		file = gth_catalog_file_from_gio_file (gio_file, NULL);
		data->new_library = gth_file_data_new (file, NULL);
		gth_file_source_read_metadata (data->catalog_source,
					       data->new_library,
					       "*",
					       new_library_metadata_ready_cb,
					       data);

		g_object_unref (file);
	}

	if (error != NULL) {
		if (g_error_matches (error, G_IO_ERROR, G_IO_ERROR_EXISTS))
			gth_request_dialog_set_info_text (GTH_REQUEST_DIALOG (dialog), GTK_MESSAGE_ERROR, _("Name already used"));
		else
			gth_request_dialog_set_info_text (GTH_REQUEST_DIALOG (dialog), GTK_MESSAGE_ERROR, error->message);
		g_clear_error (&error);
	}
	else
		gtk_widget_destroy (dialog);

	g_object_unref (gio_file);
	g_object_unref (gio_parent);
	g_object_unref (file_source);
	g_free (name);
}


static void
new_library_button_clicked_cb (GtkWidget  *widget,
		       	       DialogData *data)
{
	GtkWidget *dialog;

	dialog = gth_request_dialog_new (GTK_WINDOW (data->dialog),
					 GTK_DIALOG_MODAL,
					 _("New Library"),
					 _("Enter the library name:"),
					 _GTK_LABEL_CANCEL,
					 _("C_reate"));
	g_signal_connect (dialog,
			  "response",
			  G_CALLBACK (new_library_dialog_response_cb),
			  data);

	gtk_widget_show (dialog);
}


static gboolean
update_sensitivity_cb (gpointer user_data)
{
	DialogData *data = user_data;

	if (data->update_selection_event != 0) {
		g_source_remove (data->update_selection_event);
		data->update_selection_event = 0;
	}

	update_sensitivity (data);

	return FALSE;
}


static void
file_selection_changed_cb (GthFileSelection *self,
			   DialogData       *data)
{
	if (data->update_selection_event != 0)
		g_source_remove (data->update_selection_event);
	data->update_selection_event = g_timeout_add (UPDATE_SELECTION_DELAY, update_sensitivity_cb, data);
}


static void
keep_open_button_toggled_cb (GtkToggleButton *button,
			     DialogData      *data)
{
	gth_file_selection_info_set_visible (GTH_FILE_SELECTION_INFO (data->info),
					     gtk_toggle_button_get_active (button));
	update_sensitivity (data);
}


void
dlg_add_to_catalog (GthBrowser *browser)
{
	DialogData       *data;
	GtkTreeSelection *selection;
	char             *last_catalog;

	if (gth_browser_get_dialog (browser, ADD_TO_CATALOG_DIALOG_NAME)) {
		gtk_window_present (GTK_WINDOW (gth_browser_get_dialog (browser, ADD_TO_CATALOG_DIALOG_NAME)));
		return;
	}

	data = g_new0 (DialogData, 1);
	data->browser = browser;
	data->builder = _gtk_builder_new_from_file ("add-to-catalog.ui", "catalogs");
	data->settings = g_settings_new (GTHUMB_CATALOGS_SCHEMA);

	data->dialog = g_object_new (GTK_TYPE_DIALOG,
				     "title", _("Add to Catalog"),
				     "transient-for", GTK_WINDOW (browser),
				     "modal", FALSE,
				     "use-header-bar", _gtk_settings_get_dialogs_use_header (),
				     NULL);
	gtk_container_set_border_width (GTK_CONTAINER (data->dialog), 5);

	data->info = gth_file_selection_info_new ();
	gtk_widget_show (data->info);
	gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (data->dialog))),
			    data->info,
			    FALSE,
			    FALSE,
			    0);

	gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (data->dialog))),
			    GET_WIDGET ("dialog_content"),
			    TRUE,
			    TRUE,
			    0);

	gtk_dialog_add_buttons (GTK_DIALOG (data->dialog),
				_GTK_LABEL_CLOSE, GTK_RESPONSE_CANCEL,
				_("_Add"), GTK_RESPONSE_OK,
				NULL);

	data->keep_open_checkbutton = _gtk_toggle_image_button_new_for_header_bar ("pinned-symbolic");
	gtk_widget_set_tooltip_text (data->keep_open_checkbutton, _("Keep the dialog open"));
	gtk_widget_show (data->keep_open_checkbutton);
	_gtk_dialog_add_action_widget (GTK_DIALOG (data->dialog), data->keep_open_checkbutton);

	_gtk_dialog_add_class_to_response (GTK_DIALOG (data->dialog), GTK_RESPONSE_OK, GTK_STYLE_CLASS_SUGGESTED_ACTION);

	gth_browser_set_dialog (browser, ADD_TO_CATALOG_DIALOG_NAME, data->dialog);

	data->add_data = add_data_new ();
	data->add_data->browser = browser;
	data->add_data->parent_window = data->add_data->dialog = data->dialog;
	add_data_ref (data->add_data);

	last_catalog = g_settings_get_string (data->settings, PREF_CATALOGS_LAST_CATALOG);
	data->source_tree = gth_vfs_tree_new ("catalog:///", last_catalog);
	gtk_widget_show (data->source_tree);
	gtk_container_add (GTK_CONTAINER (GET_WIDGET ("catalog_list_scrolled_window")), data->source_tree);

	gtk_label_set_mnemonic_widget (GTK_LABEL (GET_WIDGET ("catalogs_label")), data->source_tree);
	update_sensitivity (data);

	g_free (last_catalog);

	/* Set the signals handlers. */

	g_signal_connect (G_OBJECT (data->dialog),
			  "destroy",
			  G_CALLBACK (destroy_cb),
			  data);
	g_signal_connect_swapped (gtk_dialog_get_widget_for_response (GTK_DIALOG (data->dialog), GTK_RESPONSE_CANCEL),
				  "clicked",
				  G_CALLBACK (gtk_widget_destroy),
				  G_OBJECT (data->dialog));
	g_signal_connect (G_OBJECT (data->source_tree),
			  "changed",
			  G_CALLBACK (source_tree_changed_cb),
			  data);
	g_signal_connect (gtk_dialog_get_widget_for_response (GTK_DIALOG (data->dialog), GTK_RESPONSE_OK),
			  "clicked",
			  G_CALLBACK (add_button_clicked_cb),
			  data);
	g_signal_connect (G_OBJECT (GET_WIDGET ("new_catalog_button")),
			  "clicked",
			  G_CALLBACK (new_catalog_button_clicked_cb),
			  data);
	g_signal_connect (G_OBJECT (GET_WIDGET ("new_library_button")),
			  "clicked",
			  G_CALLBACK (new_library_button_clicked_cb),
			  data);
	g_signal_connect (data->keep_open_checkbutton,
			  "toggled",
			  G_CALLBACK (keep_open_button_toggled_cb),
			  data);
	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (data->source_tree));
	g_signal_connect (selection,
			  "changed",
			  G_CALLBACK (source_tree_selection_changed_cb),
			  data);
	data->file_selection_changed_event =
			g_signal_connect (gth_browser_get_file_list_view (data->browser),
					  "file-selection-changed",
					  G_CALLBACK (file_selection_changed_cb),
					  data);

	/* run dialog. */

	gtk_window_set_transient_for (GTK_WINDOW (data->dialog), GTK_WINDOW (browser));
	gtk_window_set_modal (GTK_WINDOW (data->dialog), FALSE);
	gtk_widget_show (data->dialog);
}


void
add_to_catalog (GthBrowser *browser,
		GFile      *catalog,
		GList      *list)
{
	AddData *add_data;

	add_data = add_data_new ();
	add_data->browser = browser;
	add_data->parent_window = (GtkWidget *) browser;
	add_data->catalog_file = g_object_ref (catalog);
	add_data->files = _g_object_list_ref (list);

	add_data_exec (add_data);
}
