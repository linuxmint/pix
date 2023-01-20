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
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <config.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <gthumb.h>
#include "dlg-rename-series.h"
#include "gth-template-editor-dialog.h"
#include "gth-template-selector.h"
#include "gth-rename-task.h"
#include "preferences.h"


enum {
	SORT_DATA_COLUMN,
	SORT_NAME_COLUMN,
	SORT_NUM_COLUMNS
};

enum {
	PREVIEW_OLD_NAME_COLUMN,
	PREVIEW_NEW_NAME_COLUMN,
	PREVIEW_NUM_COLUMNS
};


#define GET_WIDGET(name) _gtk_builder_get_widget (data->builder, (name))
#define DEFAULT_START_AT 1
#define DEFAULT_CHANGE_CASE GTH_CHANGE_CASE_NONE
#define UPDATE_DELAY 250


static GthTemplateCode Rename_Special_Codes[] = {
	{ GTH_TEMPLATE_CODE_TYPE_ENUMERATOR, N_("Enumerator") },
	{ GTH_TEMPLATE_CODE_TYPE_SIMPLE, N_("Original filename"), 'F', 0 },
	{ GTH_TEMPLATE_CODE_TYPE_SIMPLE, N_("Original extension"), 'E', 0 },
	{ GTH_TEMPLATE_CODE_TYPE_SIMPLE, N_("Original enumerator"), 'N', 0 },
	{ GTH_TEMPLATE_CODE_TYPE_DATE, N_("Modification date"), 'M', 0 },
	{ GTH_TEMPLATE_CODE_TYPE_DATE, N_("Digitalization date"), 'D', 0 },
	{ GTH_TEMPLATE_CODE_TYPE_FILE_ATTRIBUTE, N_("File attribute"), 'A', 0 }
};


typedef struct {
	GthBrowser    *browser;
	GSettings     *settings;
	GList         *file_list;
	GList         *file_data_list;
	GList         *new_file_list;
	GList         *new_names_list;
	gboolean       single_file;
	gboolean       first_update;
	GtkBuilder    *builder;
	GtkWidget     *dialog;
	GtkWidget     *list_view;
	GtkWidget     *sort_combobox;
	GtkWidget     *change_case_combobox;
	GtkListStore  *list_store;
	GtkListStore  *sort_model;
	char          *required_attributes;
	guint          update_id;
	gboolean       template_changed;
	GList         *tasks;
	gboolean       closing;
} DialogData;


static void
destroy_dialog (DialogData *data)
{
	if (data->dialog != NULL)
		gtk_widget_destroy (data->dialog);
	data->dialog = NULL;
	gth_browser_set_dialog (data->browser, "rename_series", NULL);

	if (data->update_id != 0) {
		g_source_remove (data->update_id);
		data->update_id = 0;
	}

	g_free (data->required_attributes);
	g_object_unref (data->builder);
	_g_object_list_unref (data->file_data_list);
	_g_object_list_unref (data->file_list);
	_g_string_list_free (data->new_names_list);
	g_list_free (data->new_file_list);
	g_object_unref (data->settings);
	g_free (data);
}


typedef struct {
	GthFileData *file_data;
	int          n;
} TemplateData;


static char *
get_original_enum (GthFileData *file_data)
{
	char *basename;
	char  *value = NULL;

	basename = g_file_get_basename (file_data->file);
	value = _g_utf8_find_expr (basename, "[0-9]+");

	g_free (basename);

	return value;
}


static char *
get_attribute_value (GthFileData *file_data,
		     const char  *attribute)
{
	char *value;

	if (_g_str_empty (attribute))
		return NULL;

	value = gth_file_data_get_attribute_as_string (file_data, attribute);
	if (value != NULL) {
		char *tmp = _g_utf8_replace_pattern (value, "[\r\n]", " ");
		g_free (value);
		value = tmp;
	}

	return value;
}


static gboolean
template_eval_cb (TemplateFlags   flags,
		  gunichar        parent_code,
		  gunichar        code,
		  char          **args,
		  GString        *result,
		  gpointer        user_data)
{
	TemplateData *template_data = user_data;
	char         *text = NULL;
	char         *path;
	GTimeVal      timeval;

	switch (code) {
	case '#':
		text = _g_template_replace_enumerator (args[0], template_data->n);
		break;

	case 'A':
		text = get_attribute_value (template_data->file_data, args[0]);
		break;

	case 'E':
		path = g_file_get_path (template_data->file_data->file);
		text = g_strdup (_g_path_get_extension (path));
		g_free (path);
		break;

	case 'F':
		path = g_file_get_path (template_data->file_data->file);
		text = g_strdup (_g_path_get_basename (path));
		g_free (path);
		break;

	case 'N':
		text = get_original_enum (template_data->file_data);
		break;

	case 'D':
		if (gth_file_data_get_digitalization_time (template_data->file_data, &timeval))
			text = _g_time_val_strftime (&timeval, (args[0] != NULL) ? args[0] : DEFAULT_STRFTIME_FORMAT);
		break;

	case 'M':
		timeval = *gth_file_data_get_modification_time (template_data->file_data);
		text = _g_time_val_strftime (&timeval, (args[0] != NULL) ? args[0] : DEFAULT_STRFTIME_FORMAT);
		break;
	}

	if (text != NULL) {
		g_string_append (result, text);
		g_free (text);
	}

	return FALSE;
}


static gboolean
collect_file_attributes_cb (gunichar   parent_code,
			    gunichar   code,
			    char     **args,
			    gpointer   user_data)
{
	GHashTable *attributes = user_data;
	int         i;

	switch (code) {
	case 'A':
		g_hash_table_add (attributes, g_strdup (args[0]));
		break;

	case 'D':
		for (i = 0; FileDataDigitalizationTags[i] != NULL; i++)
			g_hash_table_add (attributes, g_strdup (FileDataDigitalizationTags[i]));
		break;

	case 'M':
		g_hash_table_add (attributes, g_strdup (G_FILE_ATTRIBUTE_TIME_MODIFIED "," G_FILE_ATTRIBUTE_TIME_MODIFIED_USEC));
		break;
	}

	return FALSE;
}


static char *
get_required_attributes (DialogData *data)
{
	GHashTable   *attributes;
	GtkTreeIter   iter;
	char        **attributev;
	char         *result;

	attributes = g_hash_table_new (g_str_hash, g_str_equal);
	g_hash_table_add (attributes, g_strdup (GFILE_STANDARD_ATTRIBUTES));

	/* attributes required for sorting */

	if (gtk_combo_box_get_active_iter (GTK_COMBO_BOX (data->sort_combobox), &iter)) {
		GthFileDataSort *sort_type;

		gtk_tree_model_get (GTK_TREE_MODEL (data->sort_model),
				    &iter,
				    SORT_DATA_COLUMN, &sort_type,
				    -1);

		if (! _g_str_empty (sort_type->required_attributes))
			g_hash_table_add (attributes, g_strdup (sort_type->required_attributes));
	}

	/* attributes required for renaming */

	_g_template_for_each_token (gtk_entry_get_text (GTK_ENTRY (GET_WIDGET ("template_entry"))),
				    0,
				    collect_file_attributes_cb,
				    attributes);

	attributev = (char **) g_hash_table_get_keys_as_array (attributes, NULL);
	result = g_strjoinv (",", attributev);

	g_free (attributev);
	g_hash_table_unref (attributes);

	return result;
}


/* update_file_list */


typedef struct {
	DialogData *data;
	ReadyFunc   ready_func;
	GthTask    *task;
	gulong      task_completed_id;
} UpdateData;


static void
update_data_free (UpdateData *update_data)
{
	g_free(update_data);
}


static void update_preview_cb (GtkWidget  *widget, DialogData *data);


static void
update_file_list__step2 (gpointer user_data)
{
	UpdateData   *update_data = user_data;
	DialogData   *data = update_data->data;
	GtkTreeIter   iter;
	int           change_case;
	const char   *template;
	TemplateData  template_data;
	GList        *scan;
	GError       *error = NULL;

	if (data->first_update) {
		if (data->file_data_list->next == NULL) {
			GthFileData *file_data = data->file_data_list->data;
			const char  *edit_name;
			const char  *end_pos;

			g_signal_handlers_block_by_func (GET_WIDGET ("template_entry"), update_preview_cb, data);
			gtk_entry_set_text (GTK_ENTRY (GET_WIDGET ("template_entry")), g_file_info_get_attribute_string (file_data->info, G_FILE_ATTRIBUTE_STANDARD_EDIT_NAME));
			g_signal_handlers_unblock_by_func (GET_WIDGET ("template_entry"), update_preview_cb, data);

			gtk_widget_grab_focus (GET_WIDGET ("template_entry"));

			edit_name = gtk_entry_get_text (GTK_ENTRY (GET_WIDGET ("template_entry")));
			end_pos = g_utf8_strrchr (edit_name, -1, '.');
			if (end_pos != NULL) {
				glong nchars;

				nchars = g_utf8_strlen (edit_name, (gssize) (end_pos - edit_name));
				gtk_editable_select_region (GTK_EDITABLE (GET_WIDGET ("template_entry")), 0, nchars);
			}
		}
		else {
			gtk_widget_grab_focus (GET_WIDGET ("template_entry"));
			gtk_editable_select_region (GTK_EDITABLE (GET_WIDGET ("template_entry")), 0, -1);
		}
	}

	data->first_update = FALSE;

	if (data->new_names_list != NULL) {
		_g_string_list_free (data->new_names_list);
		data->new_names_list = NULL;
	}

	if (data->new_file_list != NULL) {
		g_list_free (data->new_file_list);
		data->new_file_list = NULL;
	}

	data->new_file_list = g_list_copy (data->file_data_list);
	if (gtk_combo_box_get_active_iter (GTK_COMBO_BOX (data->sort_combobox), &iter)) {
		GthFileDataSort *sort_type;

		gtk_tree_model_get (GTK_TREE_MODEL (data->sort_model),
				    &iter,
				    SORT_DATA_COLUMN, &sort_type,
				    -1);

		if (sort_type->cmp_func != NULL)
			data->new_file_list = g_list_sort (data->new_file_list, (GCompareFunc) sort_type->cmp_func);
	}
	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("reverse_order_checkbutton"))))
		data->new_file_list = g_list_reverse (data->new_file_list);

	change_case = gtk_combo_box_get_active (GTK_COMBO_BOX (data->change_case_combobox));

	template = gtk_entry_get_text (GTK_ENTRY (GET_WIDGET ("template_entry")));
	template_data.n = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (GET_WIDGET ("start_at_spinbutton")));

	for (scan = data->new_file_list; scan; scan = scan->next) {
		char *new_name;
		char *tmp;

		template_data.file_data = (GthFileData *) scan->data;
		new_name = _g_template_eval (template,
					     0,
					     template_eval_cb,
					     &template_data);

		switch (change_case) {
		case GTH_CHANGE_CASE_LOWER:
			tmp = g_utf8_strdown (new_name, -1);
			g_free (new_name);
			new_name = tmp;
			break;

		case GTH_CHANGE_CASE_UPPER:
			tmp = g_utf8_strup (new_name, -1);
			g_free (new_name);
			new_name = tmp;
			break;

		default:
			break;
		}

		data->new_names_list = g_list_prepend (data->new_names_list, new_name);
		template_data.n++;
	}
	data->new_names_list = g_list_reverse (data->new_names_list);

	if (update_data->ready_func)
		update_data->ready_func (error, update_data->data);

	update_data_free (update_data);
}


static void
load_file_data_task_completed_cb (GthTask  *task,
				  GError   *error,
				  gpointer  user_data)
{
	UpdateData *update_data = user_data;
	DialogData *data = update_data->data;

	gtk_widget_hide (GET_WIDGET ("task_box"));
	gtk_widget_set_sensitive (GET_WIDGET ("options_table"), TRUE);
	gtk_dialog_set_response_sensitive (GTK_DIALOG (data->dialog), GTK_RESPONSE_OK, TRUE);

	data->tasks = g_list_remove (data->tasks, update_data->task);

	g_object_unref (update_data->task);
	update_data->task = NULL;
	update_data->task_completed_id = 0;

	if (error != NULL) {
		if (! data->closing && update_data->ready_func)
			update_data->ready_func (error, update_data->data);
		update_data_free (update_data);

		if (data->tasks == NULL)
			destroy_dialog (data);

		return;
	}

	_g_object_list_unref (data->file_data_list);
	data->file_data_list = _g_object_list_ref (gth_load_file_data_task_get_result (GTH_LOAD_FILE_DATA_TASK (task)));
	data->template_changed = FALSE;

	update_file_list__step2 (update_data);
}


static void
update_file_list (DialogData *data,
		  ReadyFunc   ready_func)
{
	UpdateData *update_data;

	update_data = g_new (UpdateData, 1);
	update_data->data = data;
	update_data->ready_func = ready_func;

	if (data->template_changed) {
		char     *required_attributes;
		gboolean  reload_required;

		required_attributes = get_required_attributes (data);
		reload_required = attribute_list_reload_required (data->required_attributes, required_attributes);

		g_free (data->required_attributes);
		data->required_attributes = required_attributes;

		if (reload_required) {
			GtkWidget *child;

			gtk_widget_set_sensitive (GET_WIDGET ("options_table"), FALSE);
			gtk_dialog_set_response_sensitive (GTK_DIALOG (data->dialog), GTK_RESPONSE_OK, FALSE);
			gtk_widget_show (GET_WIDGET ("task_box"));

			update_data->task = gth_load_file_data_task_new (data->file_list, data->required_attributes);
			update_data->task_completed_id = g_signal_connect (update_data->task,
									   "completed",
									   G_CALLBACK (load_file_data_task_completed_cb),
									   update_data);

			data->tasks = g_list_prepend (data->tasks, update_data->task);

			child = gth_task_progress_new (update_data->task);
			gtk_widget_show (child);
			gtk_box_pack_start (GTK_BOX (GET_WIDGET ("task_box")), child, TRUE, TRUE, 0);
			gth_task_exec (update_data->task, NULL);

			return;
		}
	}

	call_when_idle (update_file_list__step2, update_data);
}


static void
ok_button_clicked__step2 (GError   *error,
			  gpointer  user_data)
{
	DialogData  *data = user_data;
	GtkTreeIter  iter;
	GList       *old_files;
	GList       *new_files;
	GList       *scan1;
	GList       *scan2;
	GthTask     *task;

	if (error != NULL) {
		_gtk_error_dialog_from_gerror_show (GTK_WINDOW (data->dialog), _("Could not rename the files"), error);
		return;
	}

	/* -- save preferences -- */

	if (data->file_list->next != NULL)
		g_settings_set_string (data->settings, PREF_RENAME_SERIES_TEMPLATE, gtk_entry_get_text (GTK_ENTRY (GET_WIDGET ("template_entry"))));

	g_settings_set_int (data->settings, PREF_RENAME_SERIES_START_AT, gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (GET_WIDGET ("start_at_spinbutton"))));

	if (gtk_combo_box_get_active_iter (GTK_COMBO_BOX (data->sort_combobox), &iter)) {
		GthFileDataSort *sort_type;

		gtk_tree_model_get (GTK_TREE_MODEL (data->sort_model),
				    &iter,
				    SORT_DATA_COLUMN, &sort_type,
				    -1);
		g_settings_set_string (data->settings, PREF_RENAME_SERIES_SORT_BY, sort_type->name);
	}

	g_settings_set_boolean (data->settings, PREF_RENAME_SERIES_REVERSE_ORDER, gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("reverse_order_checkbutton"))));
	g_settings_set_enum (data->settings, PREF_RENAME_SERIES_CHANGE_CASE, gtk_combo_box_get_active (GTK_COMBO_BOX (data->change_case_combobox)));

	/* -- prepare and exec rename task -- */

	old_files = NULL;
	new_files = NULL;

	for (scan1 = data->new_file_list, scan2 = data->new_names_list;
	     scan1 && scan2;
	     scan1 = scan1->next, scan2 = scan2->next)
	{
		GthFileData *file_data = scan1->data;
		char        *new_name  = scan2->data;
		GFile       *parent;
		GFile       *new_file;

		parent = g_file_get_parent (file_data->file);
		new_file = g_file_get_child (parent, new_name);

		old_files = g_list_prepend (old_files, g_object_ref (file_data->file));
		new_files = g_list_prepend (new_files, new_file);

		g_object_unref (parent);
	}
	old_files = g_list_reverse (old_files);
	new_files = g_list_reverse (new_files);

	task = gth_rename_task_new (old_files, new_files);
	gth_browser_exec_task (data->browser, task, GTH_TASK_FLAGS_DEFAULT);

	g_object_unref (task);
	destroy_dialog (data);
}


static void
ok_button_clicked (DialogData *data)
{
	if (data->update_id != 0) {
		g_source_remove (data->update_id);
		data->update_id = 0;
	}

	update_file_list (data, ok_button_clicked__step2);
}


static void
dialog_response_cb (GtkDialog *dialog,
		    int        response_id,
		    gpointer   user_data)
{
	DialogData *data = user_data;

	switch (response_id) {
	case GTK_RESPONSE_DELETE_EVENT:
	case GTK_RESPONSE_CANCEL:
		if (data->tasks != NULL) {
			GList *tasks;

			data->closing = TRUE;

			tasks = g_list_copy (data->tasks);
			g_list_foreach (tasks, (GFunc) gth_task_cancel, NULL);
			g_list_free (tasks);
		}
		else
			destroy_dialog (data);
		break;

	case GTK_RESPONSE_OK:
		ok_button_clicked (data);
		break;

	default:
		break;
	}
}


static void
error_dialog_response_cb (GtkDialog *dialog,
			  int        response,
			  gpointer   user_data)
{
	DialogData *data = user_data;

	gtk_widget_destroy (GTK_WIDGET (dialog));
	destroy_dialog (data);
}


static void
update_preview__step2 (GError   *error,
		       gpointer  user_data)
{
	DialogData *data = user_data;
	GList      *scan1, *scan2;

	if (error != NULL) {
		GtkWidget *d;

		d = _gtk_message_dialog_new (GTK_WINDOW (data->dialog),
					     GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
					     _GTK_ICON_NAME_DIALOG_ERROR,
					     _("Could not rename the files"),
					     error->message,
					     _GTK_LABEL_OK, GTK_RESPONSE_OK,
					     NULL);
		g_signal_connect (d, "response", G_CALLBACK (error_dialog_response_cb), data);
		gtk_window_present (GTK_WINDOW (d));

		return;
	}

	/* -- update the list view -- */

	gtk_list_store_clear (data->list_store);
	for (scan1 = data->new_file_list, scan2 = data->new_names_list;
	     scan1 && scan2;
	     scan1 = scan1->next, scan2 = scan2->next)
	{
		GthFileData *file_data1 = scan1->data;
		GthFileData *new_name = scan2->data;
		GtkTreeIter  iter;

		gtk_list_store_append (data->list_store, &iter);
		gtk_list_store_set (data->list_store, &iter,
				    PREVIEW_OLD_NAME_COLUMN, g_file_info_get_display_name (file_data1->info),
				    PREVIEW_NEW_NAME_COLUMN, new_name,
				    -1);
	}
}


static void
dlg_rename_series_update_preview (DialogData *data)
{
	update_file_list (data, update_preview__step2);
}


static gboolean
update_preview_after_delay_cb (gpointer user_data)
{
	DialogData *data = user_data;

	if (data->update_id != 0) {
		g_source_remove (data->update_id);
		data->update_id = 0;
	}

	dlg_rename_series_update_preview (data);

	return FALSE;
}


static void
update_preview_cb (GtkWidget  *widget,
		   DialogData *data)
{
	data->template_changed = TRUE;
	if (data->update_id != 0)
		g_source_remove (data->update_id);
	data->update_id = g_timeout_add (UPDATE_DELAY, update_preview_after_delay_cb, data);
}


static void
edit_template_button_clicked_cb (GtkWidget  *widget,
				 DialogData *data)
{
	GtkWidget *dialog;

	dialog = gth_template_editor_dialog_new (Rename_Special_Codes,
						 G_N_ELEMENTS (Rename_Special_Codes),
						 0,
						 _("Edit Template"),
						 GTK_WINDOW (data->dialog));
	gth_template_editor_dialog_set_template (GTH_TEMPLATE_EDITOR_DIALOG (dialog),
						 gtk_entry_get_text (GTK_ENTRY (GET_WIDGET ("template_entry"))));
	g_signal_connect (dialog,
			  "response",
			  G_CALLBACK (gth_template_editor_dialog_default_response),
			  GET_WIDGET ("template_entry"));
	gtk_window_set_modal (GTK_WINDOW (dialog), TRUE);
	gtk_window_present (GTK_WINDOW (dialog));
}


static void
revert_template_button_clicked_cb (GtkWidget  *widget,
				   DialogData *data)
{
	gtk_entry_set_text (GTK_ENTRY (GET_WIDGET ("template_entry")),
			    g_settings_get_string (data->settings, PREF_RENAME_SERIES_TEMPLATE));
}


static void
return_pressed_callback (GtkDialog *dialog,
			 gpointer   user_data)
{
	ok_button_clicked (user_data);
}


void
dlg_rename_series (GthBrowser *browser,
		   GList      *file_list)
{
	DialogData        *data;
	GtkCellRenderer   *renderer;
	GtkTreeViewColumn *column;
	int                i;
	GList             *sort_types;
	GList             *scan;
	int                change_case;
	int                start_at;
	char              *sort_by;
	gboolean           found;

	if (gth_browser_get_dialog (browser, "rename_series") != NULL) {
		gtk_window_present (GTK_WINDOW (gth_browser_get_dialog (browser, "rename_series")));
		return;
	}

	data = g_new0 (DialogData, 1);
	data->browser = browser;
	data->builder = _gtk_builder_new_from_file ("rename-series.ui", "rename_series");
	data->settings = g_settings_new (GTHUMB_RENAME_SERIES_SCHEMA);
	data->file_list = _g_file_list_dup (file_list);
	data->first_update = TRUE;
	data->template_changed = TRUE;
	data->closing = FALSE;

	/* Get the widgets. */

	data->dialog = g_object_new (GTK_TYPE_DIALOG,
				     "title", _("Rename"),
				     "transient-for", GTK_WINDOW (browser),
				     "modal", FALSE,
				     "destroy-with-parent", FALSE,
				     "use-header-bar", _gtk_settings_get_dialogs_use_header (),
				     NULL);
	gtk_container_add (GTK_CONTAINER (gtk_dialog_get_content_area (GTK_DIALOG (data->dialog))),
			   _gtk_builder_get_widget (data->builder, "dialog_content"));
	gtk_dialog_add_buttons (GTK_DIALOG (data->dialog),
				_GTK_LABEL_CANCEL, GTK_RESPONSE_CANCEL,
				_("_Rename"), GTK_RESPONSE_OK,
				NULL);
	_gtk_dialog_add_class_to_response (GTK_DIALOG (data->dialog), GTK_RESPONSE_OK, GTK_STYLE_CLASS_SUGGESTED_ACTION);

	gth_browser_set_dialog (browser, "rename_series", data->dialog);
	g_object_set_data (G_OBJECT (data->dialog), "dialog_data", data);

	/* Set widgets data. */

	data->list_store = gtk_list_store_new (PREVIEW_NUM_COLUMNS,
					       G_TYPE_STRING,
					       G_TYPE_STRING);
	data->list_view = gtk_tree_view_new_with_model (GTK_TREE_MODEL (data->list_store));
	g_object_unref (data->list_store);

	renderer = gtk_cell_renderer_text_new ();
	g_object_set (renderer, "ellipsize", PANGO_ELLIPSIZE_END, NULL);
	column = gtk_tree_view_column_new_with_attributes (_("Old Name"),
							   renderer,
							   "text", PREVIEW_OLD_NAME_COLUMN,
							   NULL);
	gtk_tree_view_column_set_expand (GTK_TREE_VIEW_COLUMN (column), TRUE);
	gtk_tree_view_column_set_resizable (GTK_TREE_VIEW_COLUMN (column), TRUE);
	gtk_tree_view_column_set_sizing (GTK_TREE_VIEW_COLUMN (column), GTK_TREE_VIEW_COLUMN_AUTOSIZE);
	gtk_tree_view_append_column (GTK_TREE_VIEW (data->list_view), column);

	renderer = gtk_cell_renderer_text_new ();
	g_object_set (renderer, "ellipsize", PANGO_ELLIPSIZE_END, NULL);
	column = gtk_tree_view_column_new_with_attributes (_("New Name"),
							   renderer,
							   "text", PREVIEW_NEW_NAME_COLUMN,
							   NULL);
	gtk_tree_view_column_set_expand (GTK_TREE_VIEW_COLUMN (column), TRUE);
	gtk_tree_view_column_set_resizable (GTK_TREE_VIEW_COLUMN (column), TRUE);
	gtk_tree_view_column_set_sizing (GTK_TREE_VIEW_COLUMN (column), GTK_TREE_VIEW_COLUMN_AUTOSIZE);
	gtk_tree_view_append_column (GTK_TREE_VIEW (data->list_view), column);

	gtk_widget_show (data->list_view);
	gtk_widget_set_vexpand (data->list_view, TRUE);
	gtk_container_add (GTK_CONTAINER (GET_WIDGET ("preview_scrolledwindow")), data->list_view);

	gtk_label_set_mnemonic_widget (GTK_LABEL (GET_WIDGET ("preview_label")), data->list_view);

	if (data->file_list->next != NULL)
		gtk_entry_set_text (GTK_ENTRY (GET_WIDGET ("template_entry")),
				    g_settings_get_string (data->settings, PREF_RENAME_SERIES_TEMPLATE));

	start_at = g_settings_get_int (data->settings, PREF_RENAME_SERIES_START_AT);
	if (start_at < 0)
		start_at = DEFAULT_START_AT;
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (GET_WIDGET ("start_at_spinbutton")),  start_at * 1.0);

	/* sort by */

	data->sort_model = gtk_list_store_new (SORT_NUM_COLUMNS,
					       G_TYPE_POINTER,
					       G_TYPE_STRING);
	data->sort_combobox = gtk_combo_box_new_with_model (GTK_TREE_MODEL (data->sort_model));
	g_object_unref (data->sort_model);
	renderer = gtk_cell_renderer_text_new ();
	gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (data->sort_combobox), renderer, TRUE);
	gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (data->sort_combobox),
					renderer,
					"text", SORT_NAME_COLUMN,
					NULL);

	sort_by = g_settings_get_string (data->settings, PREF_RENAME_SERIES_SORT_BY);
	found = FALSE;

	sort_types = gth_main_get_all_sort_types ();
	for (i = 0, scan = sort_types; scan; scan = scan->next, i++) {
		GthFileDataSort *sort_type = scan->data;
		GtkTreeIter      iter;

		gtk_list_store_append (data->sort_model, &iter);
		gtk_list_store_set (data->sort_model, &iter,
				    SORT_DATA_COLUMN, sort_type,
				    SORT_NAME_COLUMN, sort_type->display_name,
				    -1);

		if (strcmp (sort_by, sort_type->name) == 0) {
			gtk_combo_box_set_active_iter (GTK_COMBO_BOX (data->sort_combobox), &iter);
			found = TRUE;
		}
	}
	g_list_free (sort_types);
	g_free (sort_by);

	if (!found)
		gtk_combo_box_set_active (GTK_COMBO_BOX (data->sort_combobox), 0);

	gtk_widget_show (data->sort_combobox);
	gtk_box_pack_start (GTK_BOX (GET_WIDGET ("sort_by_box")), data->sort_combobox, FALSE, FALSE, 0);
	gtk_label_set_mnemonic_widget (GTK_LABEL (GET_WIDGET ("sort_by_label")), data->sort_combobox);

	/* reverse order */

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("reverse_order_checkbutton")),
				      g_settings_get_boolean (data->settings, PREF_RENAME_SERIES_REVERSE_ORDER));

	/* change case */

	change_case = g_settings_get_enum (data->settings, PREF_RENAME_SERIES_CHANGE_CASE);
	if ((change_case < GTH_CHANGE_CASE_NONE) || (change_case > GTH_CHANGE_CASE_UPPER))
		change_case = DEFAULT_CHANGE_CASE;

	data->change_case_combobox = _gtk_combo_box_new_with_texts (_("Keep original case"),
								    _("Convert to lower-case"),
								    _("Convert to upper-case"),
								    NULL);
	gtk_combo_box_set_active (GTK_COMBO_BOX (data->change_case_combobox), change_case);
	gtk_widget_show (data->change_case_combobox);
	gtk_box_pack_start (GTK_BOX (GET_WIDGET ("change_case_box")), data->change_case_combobox, FALSE, FALSE, 0);
	gtk_label_set_mnemonic_widget (GTK_LABEL (GET_WIDGET ("change_case_label")), data->change_case_combobox);

	/* Set the signals handlers. */

	g_signal_connect (data->dialog,
			  "delete-event",
			  G_CALLBACK (gtk_true),
			  NULL);
	g_signal_connect (data->dialog,
			  "response",
			  G_CALLBACK (dialog_response_cb),
			  data);
	g_signal_connect (GET_WIDGET ("template_entry"),
			  "changed",
			  G_CALLBACK (update_preview_cb),
			  data);
	g_signal_connect (GET_WIDGET("template_entry"),
			  "activate",
			  G_CALLBACK (return_pressed_callback),
			  data);
	g_signal_connect (GET_WIDGET ("start_at_spinbutton"),
			  "value_changed",
			  G_CALLBACK (update_preview_cb),
			  data);
	g_signal_connect (data->sort_combobox,
			  "changed",
			  G_CALLBACK (update_preview_cb),
			  data);
	g_signal_connect (data->change_case_combobox,
			  "changed",
			  G_CALLBACK (update_preview_cb),
			  data);
	g_signal_connect (GET_WIDGET ("reverse_order_checkbutton"),
			  "toggled",
			  G_CALLBACK (update_preview_cb),
			  data);
	g_signal_connect (GET_WIDGET ("edit_template_button"),
			  "clicked",
			  G_CALLBACK (edit_template_button_clicked_cb),
			  data);
	g_signal_connect (GET_WIDGET ("revert_template_button"),
			  "clicked",
			  G_CALLBACK (revert_template_button_clicked_cb),
			  data);

	/* Run dialog. */

	gtk_window_set_transient_for (GTK_WINDOW (data->dialog), GTK_WINDOW (browser));
	gtk_window_set_modal (GTK_WINDOW (data->dialog), FALSE);
	gtk_widget_show (data->dialog);

	dlg_rename_series_update_preview (data);
}
