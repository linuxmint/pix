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
#include <glib.h>
#include <glib/gi18n.h>
#include <gthumb.h>
#include <extensions/catalogs/gth-catalog.h>
#include "gth-search-source.h"
#include "gth-search-task.h"


struct _GthSearchTaskPrivate {
	GthBrowser    *browser;
	GthSearch     *search;
	GthTestChain  *test;
	GFile         *search_catalog;
	gboolean       show_hidden_files;
	gboolean       io_operation;
	GError        *error;
	gulong         location_ready_id;
	GtkWidget     *dialog;
	GthFileSource *file_source;
	gsize          n_files;
	GList         *current_location;
	gulong         info_bar_response_id;
};


G_DEFINE_TYPE_WITH_CODE (GthSearchTask,
			 gth_search_task,
			 GTH_TYPE_TASK,
			 G_ADD_PRIVATE (GthSearchTask))


static void
browser_unref_cb (gpointer  data,
		  GObject  *browser)
{
	((GthSearchTask *) data)->priv->browser = NULL;
}


static void
gth_search_task_finalize (GObject *object)
{
	GthSearchTask *task;

	task = GTH_SEARCH_TASK (object);

	_g_object_unref (task->priv->file_source);
	_g_object_unref (task->priv->search);
	_g_object_unref (task->priv->test);
	_g_object_unref (task->priv->search_catalog);
	if (task->priv->browser != NULL)
		g_object_weak_unref (G_OBJECT (task->priv->browser), browser_unref_cb, task);

	G_OBJECT_CLASS (gth_search_task_parent_class)->finalize (object);
}


static void
info_bar_response_cb (GtkInfoBar *info_bar,
		      int         response_id,
		      gpointer    user_data)
{
	GthSearchTask *task = user_data;

	switch (response_id) {
	case GTK_RESPONSE_CANCEL:
		if (task->priv->info_bar_response_id != 0) {
			g_signal_handler_disconnect (task->priv->dialog, task->priv->info_bar_response_id);
			task->priv->info_bar_response_id = 0;
		}
		gth_task_cancel (GTH_TASK (task));
		break;

	default:
		break;
	}
}


static void
save_search_result_copy_done_cb (void     **buffer,
				 gsize      count,
				 GError    *error,
				 gpointer   user_data)
{
	GthSearchTask *task = user_data;

	task->priv->io_operation = FALSE;

	if (task->priv->n_files == 0)
		gth_file_list_clear (GTH_FILE_LIST (gth_browser_get_file_list (task->priv->browser)), _("No file found"));
	gth_browser_update_extra_widget (task->priv->browser);
	gtk_widget_hide (task->priv->dialog);
	gth_task_completed (GTH_TASK (task), task->priv->error);
}


static void
_gth_search_task_save_search_result (GthSearchTask *task)
{
	DomDocument   *doc;
	char          *data;
	gsize          size;
	GFile         *search_result_real_file;

	if (task->priv->info_bar_response_id != 0) {
		g_signal_handler_disconnect (task->priv->dialog, task->priv->info_bar_response_id);
		task->priv->info_bar_response_id = 0;
	}

	doc = dom_document_new ();
	dom_element_append_child (DOM_ELEMENT (doc), dom_domizable_create_element (DOM_DOMIZABLE (task->priv->search), doc));
	data = dom_document_dump (doc, &size);

	search_result_real_file = gth_catalog_file_to_gio_file (task->priv->search_catalog);
	_g_file_write_async (search_result_real_file,
			     data,
			     size,
			     TRUE,
			     G_PRIORITY_DEFAULT,
			     gth_task_get_cancellable (GTH_TASK (task)),
			     save_search_result_copy_done_cb,
			     task);

	g_object_unref (search_result_real_file);
	g_object_unref (doc);
}


static void
_gth_search_task_search_current_location (GthSearchTask *task);


static void
done_func (GObject  *object,
	   GError   *error,
	   gpointer  user_data)
{
	GthSearchTask *task = user_data;

	task->priv->error = NULL;
	if (error != NULL) {
		if (g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED)) {
			task->priv->error = g_error_new_literal (GTH_TASK_ERROR, GTH_TASK_ERROR_CANCELLED, "");
			g_error_free (error);

			/* reset the cancellable because it's re-used below to
			 * save the partial result. */
			g_cancellable_reset (gth_task_get_cancellable (GTH_TASK (task)));
		}
		else
			task->priv->error = error;
		_gth_search_task_save_search_result (task);
		return;
	}

	task->priv->current_location = g_list_next (task->priv->current_location);
	_gth_search_task_search_current_location (task);
}


static void
update_secondary_text (GthSearchTask *task)
{
	char *format_str;
	char *msg;

	format_str = g_strdup_printf ("%"G_GSIZE_FORMAT, task->priv->n_files);
	msg = g_strdup_printf (_("Files found so far: %s"), format_str);
	gth_info_bar_set_secondary_text (GTH_INFO_BAR (task->priv->dialog), msg);

	g_free (format_str);
	g_free (msg);
}


static void
for_each_file_func (GFile     *file,
		    GFileInfo *info,
		    gpointer   user_data)
{
	GthSearchTask *task = user_data;
	GthFileData   *file_data;

	if (g_file_info_get_file_type (info) != G_FILE_TYPE_REGULAR)
		return;

	file_data = gth_file_data_new (file, info);

	if (gth_test_match (GTH_TEST (task->priv->test), file_data)
	    && gth_catalog_insert_file (GTH_CATALOG (task->priv->search), file_data->file, -1))
	{
		GList *file_list;

		task->priv->n_files++;
		update_secondary_text (task);

		file_list = g_list_prepend (NULL, file_data->file);
		gth_monitor_folder_changed (gth_main_get_default_monitor (),
					    task->priv->search_catalog,
					    file_list,
					    GTH_MONITOR_EVENT_CREATED);

		g_list_free (file_list);
	}

	g_object_unref (file_data);
}


static gboolean
file_is_visible (GthSearchTask *task,
		 GFileInfo     *info)
{
	if (task->priv->show_hidden_files)
		return TRUE;
	else
		return ! g_file_info_get_is_hidden (info);
}


static DirOp
start_dir_func (GFile      *directory,
		GFileInfo  *info,
		GError    **error,
		gpointer    user_data)
{
	GthSearchTask *task = user_data;
	char          *uri;
	char          *text;

	if (! file_is_visible (task, info))
		return DIR_OP_SKIP;

	uri = g_file_get_parse_name (directory);
	text = g_strdup_printf ("Searching in %s", uri);
	gth_info_bar_set_primary_text (GTH_INFO_BAR (task->priv->dialog), text);

	g_free (text);
	g_free (uri);

	return DIR_OP_CONTINUE;
}


static void
_gth_search_task_search_current_location (GthSearchTask *task)
{
	GthSearchSource *search_location;
	GSettings       *settings;
	GString         *attributes;
	const char      *test_attributes;

	if (task->priv->current_location == NULL) {
		gth_info_bar_set_secondary_text (GTH_INFO_BAR (task->priv->dialog), NULL);
		_gth_search_task_save_search_result (task);
		return;
	}

	settings = g_settings_new (GTHUMB_BROWSER_SCHEMA);
	task->priv->show_hidden_files = g_settings_get_boolean (settings, PREF_BROWSER_SHOW_HIDDEN_FILES);

	search_location = GTH_SEARCH_SOURCE (task->priv->current_location->data);
	task->priv->file_source = gth_main_get_file_source (gth_search_source_get_folder (search_location));
	gth_file_source_set_cancellable (task->priv->file_source, gth_task_get_cancellable (GTH_TASK (task)));

	attributes = g_string_new (g_settings_get_boolean (settings, PREF_BROWSER_FAST_FILE_TYPE) ? GFILE_STANDARD_ATTRIBUTES_WITH_FAST_CONTENT_TYPE : GFILE_STANDARD_ATTRIBUTES_WITH_CONTENT_TYPE);
	test_attributes = gth_test_get_attributes (GTH_TEST (task->priv->test));
	if (test_attributes[0] != '\0') {
		g_string_append (attributes, ",");
		g_string_append (attributes, test_attributes);
	}

	task->priv->io_operation = TRUE;
	gth_file_source_for_each_child (task->priv->file_source,
					gth_search_source_get_folder (search_location),
					gth_search_source_is_recursive (search_location),
					attributes->str,
					start_dir_func,
					for_each_file_func,
					done_func,
					task);

	g_string_free (attributes, TRUE);
	g_object_unref (settings);
}


static void
browser_location_ready_cb (GthBrowser    *browser,
			   GFile         *folder,
			   gboolean       error,
			   GthSearchTask *task)
{
	GtkWidget *button;

	if (! _g_file_equal (folder, task->priv->search_catalog))
		return;

	g_signal_handler_disconnect (task->priv->browser, task->priv->location_ready_id);

	if (error) {
		if (task->priv->dialog != NULL)
			gtk_widget_hide (task->priv->dialog);
		gth_task_completed (GTH_TASK (task), NULL);
		return;
	}

	task->priv->n_files = 0;

	gth_file_list_clear (GTH_FILE_LIST (gth_browser_get_file_list (browser)), _("Searching…"));

	task->priv->dialog = gth_browser_get_list_info_bar (browser);
	gth_info_bar_set_icon_name (GTH_INFO_BAR (task->priv->dialog), "edit-find-symbolic", GTK_ICON_SIZE_BUTTON);
	gth_info_bar_set_primary_text (GTH_INFO_BAR (task->priv->dialog), _("Searching…"));
	update_secondary_text (task);
	_gtk_info_bar_clear_action_area (GTK_INFO_BAR (task->priv->dialog));
	gtk_widget_show (task->priv->dialog);

	button = gtk_button_new ();
	gtk_container_add (GTK_CONTAINER (button), gtk_image_new_from_icon_name ("process-stop-symbolic", GTK_ICON_SIZE_BUTTON));
	gtk_widget_set_tooltip_text (button, _("Cancel the operation"));
	gtk_widget_show_all (button);
	gtk_info_bar_add_action_widget (GTK_INFO_BAR (task->priv->dialog),
					button,
					GTK_RESPONSE_CANCEL);

	task->priv->info_bar_response_id = g_signal_connect (task->priv->dialog,
							     "response",
							     G_CALLBACK (info_bar_response_cb),
							     task);

	/**/

	if (gth_search_get_test (task->priv->search) != NULL)
		task->priv->test = (GthTestChain*) gth_duplicable_duplicate (GTH_DUPLICABLE (gth_search_get_test (task->priv->search)));
	else
		task->priv->test = (GthTestChain*) gth_test_chain_new (GTH_MATCH_TYPE_ALL, NULL);

	if (! gth_test_chain_has_type_test (task->priv->test)) {
		GthTest *general_filter;
		GthTest *test_with_general_filter;

		general_filter = gth_main_get_general_filter ();
		test_with_general_filter = gth_test_chain_new (GTH_MATCH_TYPE_ALL,
							       general_filter,
							       task->priv->test,
							       NULL);

		g_object_unref (task->priv->test);
		task->priv->test = (GthTestChain *) test_with_general_filter;

		g_object_unref (general_filter);
	}

	task->priv->current_location = gth_search_get_sources (task->priv->search);
	_gth_search_task_search_current_location (task);
}


static void
clear_search_result_copy_done_cb (void     **buffer,
				  gsize      count,
				  GError    *error,
				  gpointer   user_data)
{
	GthSearchTask *task = user_data;
	GFile         *parent;
	GList         *files;

	task->priv->io_operation = FALSE;

	if (error != NULL) {
		_gtk_error_dialog_from_gerror_show (GTK_WINDOW (task->priv->browser), _("Could not create the catalog"), error);
		return;
	}

	parent = g_file_get_parent (task->priv->search_catalog);
	files = g_list_prepend (NULL, g_object_ref (task->priv->search_catalog));
	gth_monitor_folder_changed (gth_main_get_default_monitor (),
				    parent,
				    files,
				    GTH_MONITOR_EVENT_CREATED);

	_g_object_list_unref (files);
	g_object_unref (parent);

	task->priv->location_ready_id = g_signal_connect (task->priv->browser,
							  "location-ready",
							  G_CALLBACK (browser_location_ready_cb),
							  task);
	gth_browser_go_to (task->priv->browser, task->priv->search_catalog, NULL);
}


static void
gth_search_task_exec (GthTask *base)
{
	GthSearchTask *task = (GthSearchTask *) base;
	DomDocument   *doc;
	char          *data;
	gsize          size;
	GFile         *search_result_real_file;

	gth_catalog_set_file_list (GTH_CATALOG (task->priv->search), NULL);

	/* save the search result */

	task->priv->io_operation = TRUE;

	doc = dom_document_new ();
	dom_element_append_child (DOM_ELEMENT (doc), dom_domizable_create_element (DOM_DOMIZABLE (task->priv->search), doc));
	data = dom_document_dump (doc, &size);

	search_result_real_file = gth_catalog_file_to_gio_file (task->priv->search_catalog);
	_g_file_write_async (search_result_real_file,
			     data,
			     size,
			     TRUE,
			     G_PRIORITY_DEFAULT,
			     gth_task_get_cancellable (GTH_TASK (task)),
			     clear_search_result_copy_done_cb,
			     task);

	g_object_unref (search_result_real_file);
	g_object_unref (doc);
}


static void
gth_search_task_cancelled (GthTask *base)
{
	GthSearchTask *task = (GthSearchTask *) base;

	if (! task->priv->io_operation) {
		if (task->priv->dialog != NULL)
			gtk_widget_hide (task->priv->dialog);
		gth_task_completed (GTH_TASK (task), g_error_new_literal (GTH_TASK_ERROR, GTH_TASK_ERROR_CANCELLED, ""));
	}
}


static void
gth_search_task_class_init (GthSearchTaskClass *class)
{
	GObjectClass *object_class;
	GthTaskClass *task_class;

	object_class = (GObjectClass*) class;
	object_class->finalize = gth_search_task_finalize;

	task_class = (GthTaskClass*) class;
	task_class->exec = gth_search_task_exec;
	task_class->cancelled = gth_search_task_cancelled;
}


static void
gth_search_task_init (GthSearchTask *task)
{
	task->priv = gth_search_task_get_instance_private (task);
	task->priv->browser = NULL;
	task->priv->search = NULL;
	task->priv->test = NULL;
	task->priv->search_catalog = NULL;
	task->priv->show_hidden_files = FALSE;
	task->priv->io_operation = FALSE;
	task->priv->error = NULL;
	task->priv->location_ready_id = 0;
	task->priv->dialog = NULL;
	task->priv->file_source = NULL;
	task->priv->n_files = 0;
	task->priv->current_location = NULL;
	task->priv->info_bar_response_id = 0;
}


GthTask *
gth_search_task_new (GthBrowser *browser,
		     GthSearch  *search,
		     GFile      *search_catalog)
{
	GthSearchTask *task;

	task = (GthSearchTask *) g_object_new (GTH_TYPE_SEARCH_TASK, NULL);

	task->priv->browser = browser;
	g_object_weak_ref (G_OBJECT (task->priv->browser), browser_unref_cb, task);

	task->priv->search = g_object_ref (search);
	task->priv->search_catalog = g_object_ref (search_catalog);

	return (GthTask*) task;
}


GFile *
gth_search_task_get_catalog (GthSearchTask *task)
{
	g_return_val_if_fail (GTH_IS_SEARCH_TASK (task), NULL);
	return task->priv->search_catalog;
}
