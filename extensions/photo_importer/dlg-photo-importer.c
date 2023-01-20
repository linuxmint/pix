/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2009 Free Software Foundation, Inc.
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
 *  along with this program. If not, see <http://www.gnu.org/licenses/>
 */

#include <config.h>
#include <gtk/gtk.h>
#include <gthumb.h>
#include <extensions/importer/importer.h>
#include "dlg-photo-importer.h"
#include "preferences.h"


typedef enum {
	DLG_IMPORTER_SOURCE_TYPE_DEVICE,
	DLG_IMPORTER_SOURCE_TYPE_FOLDER
} DlgImporterSourceType;


enum {
	SOURCE_LIST_COLUMN_MOUNT,
	SOURCE_LIST_COLUMN_ICON,
	SOURCE_LIST_COLUMN_NAME,
	SOURCE_LIST_COLUMNS
};

#define GET_WIDGET(name) _gtk_builder_get_widget (data->builder, (name))


typedef struct {
	GthBrowser    *browser;
	DlgImporterSourceType  selector_type;
	GtkWidget     *dialog;
	GtkWidget     *preferences_dialog;
	GtkBuilder    *builder;
	GSettings     *settings;
	GFile         *source;
	GFile         *last_source;
	GtkListStore  *device_list_store;
	GtkWidget     *device_chooser;
	GtkWidget     *folder_chooser;
	GtkWidget     *file_list;
	GCancellable  *cancellable;
	GList         *files;
	gboolean       loading_list;
	gboolean       import;
	GthFileSource *vfs_source;
	DataFunc       done_func;
	gboolean       cancelling;
	gulong         entry_points_changed_id;
	GtkWidget     *filter_combobox;
	GtkWidget     *tags_entry;
	GList         *general_tests;
} DialogData;


static GList *
get_selected_file_list (DialogData *data)
{
	GList     *file_list = NULL;
	GtkWidget *file_view;
	GList     *items;

	file_view = gth_file_list_get_view (GTH_FILE_LIST (data->file_list));
	items = gth_file_selection_get_selected (GTH_FILE_SELECTION (file_view));
	if (items != NULL)
		file_list = gth_file_list_get_files (GTH_FILE_LIST (data->file_list), items);
	else
		file_list = gth_file_store_get_visibles (GTH_FILE_STORE (gth_file_view_get_model (GTH_FILE_VIEW (file_view))));

	_gtk_tree_path_list_free (items);

	return file_list;
}


static void
destroy_dialog (gpointer user_data)
{
	DialogData *data = user_data;
	gboolean    delete_imported;

	g_signal_handler_disconnect (gth_main_get_default_monitor (), data->entry_points_changed_id);

	delete_imported = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("delete_checkbutton")));
	g_settings_set_boolean (data->settings, PREF_PHOTO_IMPORTER_DELETE_FROM_DEVICE, delete_imported);

	if (data->import) {
		GSettings *importer_settings;
		GFile     *destination;
		char      *subfolder_template;
		GList     *file_list;

		importer_settings = g_settings_new (GTHUMB_IMPORTER_SCHEMA);
		destination = gth_import_preferences_get_destination ();
		subfolder_template = g_settings_get_string (importer_settings, PREF_IMPORTER_SUBFOLDER_TEMPLATE);

		file_list = get_selected_file_list (data);
		if (file_list != NULL) {
			char    **tags;
			GthTask  *task;

			tags = gth_tags_entry_get_tags (GTH_TAGS_ENTRY (data->tags_entry), TRUE);
			task = gth_import_task_new (data->browser,
						    file_list,
						    destination,
						    subfolder_template,
						    gtk_entry_get_text (GTK_ENTRY (GET_WIDGET ("event_entry"))),
						    tags,
						    delete_imported,
						    FALSE,
						    g_settings_get_boolean (data->settings, PREF_PHOTO_IMPORTER_ADJUST_ORIENTATION));
			gth_browser_exec_task (data->browser, task, GTH_TASK_FLAGS_DEFAULT);

			g_strfreev (tags);
			g_object_unref (task);
		}

		_g_object_list_unref (file_list);
		g_free (subfolder_template);
		_g_object_unref (destination);
		g_object_unref (importer_settings);
	}

	gtk_widget_destroy (data->dialog);
	gth_browser_set_dialog (data->browser, "photo_importer", NULL);

	g_object_unref (data->vfs_source);
	g_object_unref (data->settings);
	g_object_unref (data->builder);
	_g_object_unref (data->source);
	_g_object_unref (data->last_source);
	_g_object_unref (data->cancellable);
	_g_object_list_unref (data->files);
	_g_string_list_free (data->general_tests);

	if (! data->import && gth_browser_get_close_with_task (data->browser))
		gth_window_close (GTH_WINDOW (data->browser));

	g_free (data);
}


static void
cancel_done (gpointer user_data)
{
	DialogData *data = user_data;

	g_cancellable_reset (data->cancellable);
	data->cancelling = FALSE;
	data->done_func (data);
}


static void
cancel (DialogData *data,
	DataFunc    done_func)
{
	if (data->cancelling)
		return;

	data->done_func = done_func;
	data->cancelling = TRUE;
	if (data->loading_list)
		g_cancellable_cancel (data->cancellable);
	else
		gth_file_list_cancel (GTH_FILE_LIST (data->file_list), cancel_done, data);
}


static void
close_dialog (gpointer unused,
	      DialogData *data)
{
	cancel (data, destroy_dialog);
}


static gboolean
dialog_delete_event_cb (GtkWidget *widget,
			GdkEvent  *event,
			gpointer   user_data)
{
	close_dialog (NULL, (DialogData *) user_data);
	return TRUE;
}


static void
ok_clicked_cb (GtkWidget  *widget,
	       DialogData *data)
{
	GList    *file_list;
	GFile    *destination;
	GError   *error = NULL;
	gboolean  import;

	file_list = get_selected_file_list (data);
	destination = gth_import_preferences_get_destination ();
	if (! gth_import_task_check_free_space (destination, file_list, &error)) {
		_gtk_error_dialog_from_gerror_show (GTK_WINDOW (data->dialog),
						    _("Could not import the files"),
						    error);
		g_clear_error (&error);
		import = FALSE;
	}
	else
		import = TRUE;

	_g_object_unref (destination);
	_g_object_list_unref (file_list);

	if (! import)
		return;

	data->import = TRUE;
	close_dialog (NULL, data);
}


static void
update_sensitivity (DialogData *data)
{
	gboolean can_import;

	if (data->selector_type == DLG_IMPORTER_SOURCE_TYPE_DEVICE)
		can_import = data->source != NULL;
	else
		can_import = TRUE;
	gtk_dialog_set_response_sensitive (GTK_DIALOG (data->dialog), GTK_RESPONSE_OK, can_import);
	gtk_widget_set_sensitive (GET_WIDGET ("source_selector_box"), can_import);
	gtk_widget_set_sensitive (GET_WIDGET ("tags_box"), can_import);
	gtk_widget_set_sensitive (GET_WIDGET ("delete_checkbutton"), can_import);
}


static void
update_status (DialogData *data)
{
	GtkWidget *file_view;
	int        n_selected;
	goffset    size;
	GList     *selected;
	GList     *file_list;
	GList     *scan;
	char      *ssize;
	char      *status;

	file_view = gth_file_list_get_view (GTH_FILE_LIST (data->file_list));
	selected = gth_file_selection_get_selected (GTH_FILE_SELECTION (file_view));
	if (selected != NULL)
		file_list = gth_file_list_get_files (GTH_FILE_LIST (data->file_list), selected);
	else
		file_list = gth_file_store_get_visibles (GTH_FILE_STORE (gth_file_view_get_model (GTH_FILE_VIEW (file_view))));

	n_selected = 0;
	size = 0;
	for (scan = file_list; scan; scan = scan->next) {
		GthFileData *file_data = scan->data;

		size += g_file_info_get_size (file_data->info);
		n_selected += 1;
	}
	ssize = g_format_size (size);
	/* translators: %d is the number of files, %s the total size */
	status = g_strdup_printf (_("Files to import: %d (%s)"), n_selected, ssize);
	gtk_label_set_text (GTK_LABEL (GET_WIDGET ("status_label")), status);

	g_free (status);
	g_free (ssize);

	_g_object_list_unref (file_list);
	_gtk_tree_path_list_free (selected);
}


static void
file_store_changed_cb (GthFileStore *file_store,
		       DialogData   *data)
{
	update_status (data);
}


static void
file_view_selection_changed_cb (GtkIconView *iconview,
				DialogData  *data)
{
	update_status (data);
}


static void
list_ready_cb (GList    *files,
	       GError   *error,
	       gpointer  user_data)
{
	DialogData *data = user_data;

	data->loading_list = FALSE;

	if (data->cancelling) {
		gth_file_list_cancel (GTH_FILE_LIST (data->file_list), cancel_done, data);
	}
	else if (error != NULL) {
		_gtk_error_dialog_from_gerror_show (GTK_WINDOW (data->dialog), _("Could not load the folder"), error);
	}
	else {
		_g_object_unref (data->last_source);
		data->last_source = g_file_dup (data->source);

		data->files = _g_object_list_ref (files);
		gth_file_list_set_files (GTH_FILE_LIST (data->file_list), data->files);
	}

	update_sensitivity (data);
}


static void
list_source_files (gpointer user_data)
{
	DialogData *data = user_data;
	GList      *list;

	_g_clear_object (&data->last_source);
	_g_object_list_unref (data->files);
	data->files = NULL;

	if (data->source == NULL) {
		gth_file_list_clear (GTH_FILE_LIST (data->file_list), NULL);
		update_sensitivity (data);
		return;
	}

	gth_file_list_clear (GTH_FILE_LIST (data->file_list), _("Getting the folder content…"));

	data->loading_list = TRUE;
	list = g_list_prepend (NULL, data->source);
	_g_query_all_metadata_async (list,
				     GTH_LIST_RECURSIVE | GTH_LIST_NO_HIDDEN_FILES | GTH_LIST_NO_BACKUP_FILES,
				     DEFINE_STANDARD_ATTRIBUTES (",preview::icon,standard::fast-content-type,gth::file::display-size"),
				     data->cancellable,
				     list_ready_cb,
				     data);

	g_list_free (list);
}


static void
load_file_list (DialogData *data)
{
	update_sensitivity (data);
	if (_g_file_equal (data->source, data->last_source))
		return;
	cancel (data, list_source_files);
}


static void
device_chooser_changed_cb (GtkWidget  *widget,
			   DialogData *data)
{
	GtkTreeIter  iter;
	GMount      *mount;

	if (! gtk_combo_box_get_active_iter (GTK_COMBO_BOX (data->device_chooser), &iter)) {
		_g_clear_object (&data->source);
		_g_clear_object (&data->last_source);
		gth_file_list_clear (GTH_FILE_LIST (data->file_list), NULL);
		return;
	}

	gtk_tree_model_get (GTK_TREE_MODEL (data->device_list_store), &iter,
			    SOURCE_LIST_COLUMN_MOUNT, &mount,
			    -1);

	if (mount == NULL) {
		_g_clear_object (&data->source);
		_g_clear_object (&data->last_source);
		gth_file_list_clear (GTH_FILE_LIST (data->file_list), NULL);
		return;
	}

	_g_object_unref (data->source);
	data->source = g_mount_get_root (mount);
	load_file_list (data);

	g_object_unref (mount);
}


static void
folder_chooser_file_set_cb (GtkFileChooserButton *widget,
			    gpointer              user_data)
{
	DialogData *data = user_data;
	GFile      *folder;

	folder = gtk_file_chooser_get_file (GTK_FILE_CHOOSER (widget));
	if (folder == NULL)
		return;

	_g_object_unref (data->source);
	data->source = g_object_ref (folder);;
	load_file_list (data);

	g_object_unref (folder);
}


static void
update_device_source_list (DialogData *data)
{
	gboolean  source_available = FALSE;
	GList    *mounts;
	GList    *scan;

	gtk_list_store_clear (data->device_list_store);

	mounts = g_volume_monitor_get_mounts (g_volume_monitor_get ());
	for (scan = mounts; scan; scan = scan->next) {
		GMount      *mount = scan->data;
		GtkTreeIter  iter;
		GFile       *root;
		GIcon       *icon;
		char        *name;
		GDrive      *drive;

		if (g_mount_is_shadowed (mount))
			continue;

		gtk_list_store_append (data->device_list_store, &iter);

		root = g_mount_get_root (mount);
		if (data->source == NULL)
			data->source = g_file_dup (root);

		icon = g_mount_get_icon (mount);
		name = g_mount_get_name (mount);

		drive = g_mount_get_drive (mount);
		if (drive != NULL) {
			char *drive_name;
			char *tmp;

			drive_name = g_drive_get_name (drive);
			tmp = g_strconcat (drive_name, ": ", name, NULL);
			g_free (name);
			g_object_unref (drive);
			name = tmp;

			g_free (drive_name);
		}

		gtk_list_store_set (data->device_list_store, &iter,
				    SOURCE_LIST_COLUMN_MOUNT, mount,
				    SOURCE_LIST_COLUMN_ICON, icon,
				    SOURCE_LIST_COLUMN_NAME, name,
				    -1);

		if (g_file_equal (data->source, root)) {
			source_available = TRUE;
			gtk_combo_box_set_active_iter (GTK_COMBO_BOX (data->device_chooser), &iter);
		}

		g_free (name);
		g_object_unref (icon);
		g_object_unref (root);
	}

	if (! source_available) {
		_g_object_unref (data->source);
		data->source = NULL;
		load_file_list (data);
	}

	_g_object_list_unref (mounts);
}


static void
entry_points_changed_cb (GthMonitor *monitor,
			 DialogData *data)
{
	update_device_source_list (data);
}


static void
filter_combobox_changed_cb (GtkComboBox *widget,
			    DialogData  *data)
{
	int         idx;
	const char *test_id;
	GthTest    *test;

	idx = gtk_combo_box_get_active (widget);
	test_id = g_list_nth (data->general_tests, idx)->data;
	test = gth_main_get_registered_object (GTH_TYPE_TEST, test_id);
	gth_file_list_set_filter (GTH_FILE_LIST (data->file_list), test);

	g_settings_set_string (data->settings, PREF_PHOTO_IMPORTER_FILTER, test_id);

	g_object_unref (test);
}


static void
event_entry_changed_cb (GtkEditable *editable,
			DialogData  *data)
{
	gth_import_preferences_dialog_set_event (GTH_IMPORT_PREFERENCES_DIALOG (data->preferences_dialog),
						 gtk_entry_get_text (GTK_ENTRY (GET_WIDGET ("event_entry"))));
}


static void
dlg_photo_importer (GthBrowser            *browser,
		    GFile                 *source,
		    DlgImporterSourceType  selector_type)
{
	DialogData       *data;
	GtkCellRenderer  *renderer;
	GthFileDataSort  *sort_type;
	GList            *tests, *scan;
	char             *default_filter;
	int               i, active_filter;
	int               i_general;

	if (gth_browser_get_dialog (browser, "photo_importer") != NULL) {
		gtk_window_present (GTK_WINDOW (gth_browser_get_dialog (browser, "photo_importer")));
		return;
	}

	data = g_new0 (DialogData, 1);
	data->browser = browser;
	data->builder = gtk_builder_new_from_resource ("/org/gnome/gThumb/photo_importer/data/ui/photo-importer.ui");
	data->settings = g_settings_new (GTHUMB_PHOTO_IMPORTER_SCHEMA);
	data->selector_type = selector_type;
	data->source = _g_object_ref (source);
	data->cancellable = g_cancellable_new ();
	data->vfs_source = g_object_new (GTH_TYPE_FILE_SOURCE_VFS, NULL);
	gth_file_source_monitor_entry_points (GTH_FILE_SOURCE (data->vfs_source));

	/* Get the widgets. */

	data->dialog = g_object_new (GTK_TYPE_DIALOG,
				     "transient-for", GTK_WINDOW (browser),
				     "modal", FALSE,
				     "destroy-with-parent", FALSE,
				     "use-header-bar", _gtk_settings_get_dialogs_use_header (),
				     NULL);
	gtk_container_add (GTK_CONTAINER (gtk_dialog_get_content_area (GTK_DIALOG (data->dialog))),
			   _gtk_builder_get_widget (data->builder, "dialog_content"));
	gtk_dialog_add_buttons (GTK_DIALOG (data->dialog),
				_GTK_LABEL_CANCEL, GTK_RESPONSE_CANCEL,
				_("_Import"), GTK_RESPONSE_OK,
				NULL);
	_gtk_dialog_add_class_to_response (GTK_DIALOG (data->dialog), GTK_RESPONSE_OK, GTK_STYLE_CLASS_SUGGESTED_ACTION);


	_gtk_window_resize_to_fit_screen_height (data->dialog, 580);
	gth_browser_set_dialog (browser, "photo_importer", data->dialog);
	g_object_set_data (G_OBJECT (data->dialog), "dialog_data", data);

	if (data->selector_type == DLG_IMPORTER_SOURCE_TYPE_DEVICE) {
		gtk_window_set_title (GTK_WINDOW (data->dialog), _("Import from Removable Device"));

		data->device_list_store = gtk_list_store_new (SOURCE_LIST_COLUMNS, G_TYPE_OBJECT, G_TYPE_ICON, G_TYPE_STRING);
		data->device_chooser = gtk_combo_box_new_with_model (GTK_TREE_MODEL (data->device_list_store));
		gtk_widget_show (data->device_chooser);
		gtk_box_pack_start (GTK_BOX (GET_WIDGET ("source_box")), data->device_chooser, TRUE, TRUE, 0);
		gtk_label_set_mnemonic_widget (GTK_LABEL (GET_WIDGET ("source_label")), data->device_chooser);

		renderer = gtk_cell_renderer_pixbuf_new ();
		gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (data->device_chooser), renderer, FALSE);
		gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (data->device_chooser),
						renderer,
						"gicon", SOURCE_LIST_COLUMN_ICON,
						NULL);

		renderer = gtk_cell_renderer_text_new ();
		gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (data->device_chooser), renderer, TRUE);
		gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (data->device_chooser),
						renderer,
						"text", SOURCE_LIST_COLUMN_NAME,
						NULL);

		g_object_unref (data->device_list_store);
	}
	else {
		if (data->source == NULL) {
			if (GTH_IS_FILE_SOURCE_VFS (gth_browser_get_location_source (browser)))
				data->source = _g_object_ref (gth_browser_get_location (browser));
			if (data->source == NULL)
				data->source = g_file_new_for_uri (_g_uri_get_home ());
		}

		gtk_window_set_title (GTK_WINDOW (data->dialog), _("Import from Folder"));

		data->folder_chooser = gtk_file_chooser_button_new (_("Choose a folder"), GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER);
		gtk_label_set_mnemonic_widget (GTK_LABEL (GET_WIDGET ("source_label")), data->folder_chooser);
		gtk_file_chooser_set_file (GTK_FILE_CHOOSER (data->folder_chooser), data->source, NULL);
		gtk_file_chooser_set_local_only (GTK_FILE_CHOOSER (data->folder_chooser), FALSE);
		gtk_widget_show (data->folder_chooser);
		gtk_box_pack_start (GTK_BOX (GET_WIDGET ("source_box")), data->folder_chooser, TRUE, TRUE, 0);
	}

	data->file_list = gth_file_list_new (gth_grid_view_new (), GTH_FILE_LIST_MODE_NORMAL, FALSE);
	sort_type = gth_main_get_sort_type ("file::mtime");
	gth_file_list_set_sort_func (GTH_FILE_LIST (data->file_list), sort_type->cmp_func, FALSE);
	gth_file_list_enable_thumbs (GTH_FILE_LIST (data->file_list), TRUE);
	gth_file_list_set_ignore_hidden (GTH_FILE_LIST (data->file_list), TRUE);
	gth_file_list_set_thumb_size (GTH_FILE_LIST (data->file_list), 128);
	gth_file_list_set_caption (GTH_FILE_LIST (data->file_list), "standard::display-name,gth::file::display-size");

	gtk_widget_show (data->file_list);
	gtk_box_pack_start (GTK_BOX (GET_WIDGET ("filelist_box")), data->file_list, TRUE, TRUE, 0);

	tests = gth_main_get_registered_objects_id (GTH_TYPE_TEST);
	default_filter = g_settings_get_string (data->settings, PREF_PHOTO_IMPORTER_FILTER); /* default value */
	active_filter = 0;

	data->filter_combobox = gtk_combo_box_text_new ();
	for (i = 0, i_general = -1, scan = tests; scan; scan = scan->next, i++) {
		const char *registered_test_id = scan->data;
		GthTest    *test;

		if (strncmp (registered_test_id, "file::type::", 12) != 0)
			continue;

		i_general += 1;
		test = gth_main_get_registered_object (GTH_TYPE_TEST, registered_test_id);
		if (strcmp (registered_test_id, default_filter) == 0) {
			active_filter = i_general;
			gth_file_list_set_filter (GTH_FILE_LIST (data->file_list), test);
		}

		data->general_tests = g_list_prepend (data->general_tests, g_strdup (gth_test_get_id (test)));
		gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (data->filter_combobox),
						gth_test_get_display_name (test));
		g_object_unref (test);
	}
	data->general_tests = g_list_reverse (data->general_tests);

	gtk_combo_box_set_active (GTK_COMBO_BOX (data->filter_combobox), active_filter);
	gtk_widget_show (data->filter_combobox);
	gtk_container_add (GTK_CONTAINER (GET_WIDGET ("filter_box")), data->filter_combobox);

	gtk_label_set_mnemonic_widget (GTK_LABEL (GET_WIDGET ("filter_label")), data->filter_combobox);
	gtk_label_set_use_underline (GTK_LABEL (GET_WIDGET ("filter_label")), TRUE);

	_g_string_list_free (tests);
	g_free (default_filter);

	data->tags_entry = gth_tags_entry_new (GTH_TAGS_ENTRY_MODE_POPUP);
	gtk_widget_show (data->tags_entry);
	gtk_box_pack_start (GTK_BOX (GET_WIDGET ("tags_entry_box")), data->tags_entry, TRUE, TRUE, 0);
	gtk_label_set_mnemonic_widget (GTK_LABEL (GET_WIDGET ("tags_label")), data->tags_entry);

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("delete_checkbutton")),
				      g_settings_get_boolean (data->settings, PREF_PHOTO_IMPORTER_DELETE_FROM_DEVICE));

	data->preferences_dialog = gth_import_preferences_dialog_new ();
	gtk_window_set_transient_for (GTK_WINDOW (data->preferences_dialog), GTK_WINDOW (data->dialog));

	gtk_box_pack_start (GTK_BOX (GET_WIDGET ("destination_button_box")),
			    gth_import_destination_button_new (GTH_IMPORT_PREFERENCES_DIALOG (data->preferences_dialog)),
			    TRUE,
			    TRUE,
			    0);
	gtk_widget_show_all (GET_WIDGET ("destination_button_box"));

	/* Set the signals handlers. */

	g_signal_connect (G_OBJECT (data->dialog),
			  "delete-event",
			  G_CALLBACK (dialog_delete_event_cb),
			  data);
	g_signal_connect (gtk_dialog_get_widget_for_response (GTK_DIALOG (data->dialog), GTK_RESPONSE_OK),
			  "clicked",
			  G_CALLBACK (ok_clicked_cb),
			  data);
	g_signal_connect (gtk_dialog_get_widget_for_response (GTK_DIALOG (data->dialog), GTK_RESPONSE_CANCEL),
			  "clicked",
			  G_CALLBACK (close_dialog),
			  data);
        if (data->selector_type == DLG_IMPORTER_SOURCE_TYPE_DEVICE)
		g_signal_connect (data->device_chooser,
				  "changed",
				  G_CALLBACK (device_chooser_changed_cb),
				  data);
        else
		g_signal_connect (data->folder_chooser,
				  "selection-changed",
				  G_CALLBACK (folder_chooser_file_set_cb),
				  data);
	g_signal_connect (data->filter_combobox,
			  "changed",
			  G_CALLBACK (filter_combobox_changed_cb),
			  data);
	g_signal_connect (gth_file_view_get_model (GTH_FILE_VIEW (gth_file_list_get_view (GTH_FILE_LIST (data->file_list)))),
			  "visibility_changed",
			  G_CALLBACK (file_store_changed_cb),
			  data);
	g_signal_connect (gth_file_list_get_view (GTH_FILE_LIST (data->file_list)),
			  "file-selection-changed",
			  G_CALLBACK (file_view_selection_changed_cb),
			  data);
	data->entry_points_changed_id = g_signal_connect (gth_main_get_default_monitor (),
							  "entry-points-changed",
							  G_CALLBACK (entry_points_changed_cb),
							  data);
	g_signal_connect_after (GET_WIDGET ("event_entry"),
				"changed",
				G_CALLBACK (event_entry_changed_cb),
				data);

	/* Run dialog. */

	gtk_widget_show (data->dialog);

	gth_import_preferences_dialog_set_event (GTH_IMPORT_PREFERENCES_DIALOG (data->preferences_dialog),
						 gtk_entry_get_text (GTK_ENTRY (GET_WIDGET ("event_entry"))));

	if (data->selector_type == DLG_IMPORTER_SOURCE_TYPE_DEVICE)
		update_device_source_list (data);
	else
		load_file_list (data);
}


void
dlg_photo_importer_from_device (GthBrowser *browser,
				GFile      *source)
{
	dlg_photo_importer (browser, source, DLG_IMPORTER_SOURCE_TYPE_DEVICE);
}


void
dlg_photo_importer_from_folder (GthBrowser *browser,
				GFile      *source)
{
	dlg_photo_importer (browser, source, DLG_IMPORTER_SOURCE_TYPE_FOLDER);
}
