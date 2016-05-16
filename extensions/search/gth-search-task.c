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
#include "gth-search-task.h"


G_DEFINE_TYPE (GthSearchTask, gth_search_task, GTH_TYPE_TASK)


struct _GthSearchTaskPrivate
{
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
};


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

	if (task->priv != NULL) {
		g_object_unref (task->priv->file_source);
		g_object_unref (task->priv->search);
		g_object_unref (task->priv->test);
		g_object_unref (task->priv->search_catalog);
		if (task->priv->browser != NULL)
			g_object_weak_unref (G_OBJECT (task->priv->browser), browser_unref_cb, task);
		g_free (task->priv);
		task->priv = NULL;
	}

	G_OBJECT_CLASS (gth_search_task_parent_class)->finalize (object);
}


typedef struct {
	GthBrowser    *browser;
	GthSearchTask *task;
	gulong         response_id;
} EmbeddedDialogData;


static void
embedded_dialog_response_cb (GthEmbeddedDialog *dialog,
			     int                response_id,
			     gpointer           user_data)
{
	EmbeddedDialogData *data = user_data;

	switch (response_id) {
	case GTK_RESPONSE_CANCEL:
		gth_task_cancel (GTH_TASK (data->task));
		break;

	default:
		break;
	}

	g_signal_handler_disconnect (dialog, data->response_id);
	g_free (data);
}


static void
save_search_result_copy_done_cb (void     **buffer,
				 gsize      count,
				 GError    *error,
				 gpointer   user_data)
{
	GthSearchTask *task = user_data;

	task->priv->io_operation = FALSE;

	gth_browser_update_extra_widget (task->priv->browser);
	gth_monitor_folder_changed (gth_main_get_default_monitor (),
				    task->priv->search_catalog,
				    gth_catalog_get_file_list (GTH_CATALOG (task->priv->search)),
				    GTH_MONITOR_EVENT_CREATED);
	gth_task_completed (GTH_TASK (task), task->priv->error);
}


static void
done_func (GObject  *object,
	   GError   *error,
	   gpointer  user_data)
{
	GthSearchTask *task = user_data;
	DomDocument   *doc;
	char          *data;
	gsize          size;
	GFile         *search_result_real_file;

	gth_embedded_dialog_set_secondary_text (GTH_EMBEDDED_DIALOG (task->priv->dialog), NULL);

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
	}

	/* save the search result */

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
update_secondary_text (GthSearchTask *task)
{
	char *format_str;
	char *msg;

	format_str = g_strdup_printf ("%"G_GSIZE_FORMAT, task->priv->n_files);
	msg = g_strdup_printf (_("Files found until now: %s"), format_str);
	gth_embedded_dialog_set_secondary_text (GTH_EMBEDDED_DIALOG (task->priv->dialog), msg);

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

	if (gth_test_match (GTH_TEST (task->priv->test), file_data)) {
		gth_catalog_insert_file (GTH_CATALOG (task->priv->search), file_data->file, -1);
		task->priv->n_files++;
		update_secondary_text (task);
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
	gth_embedded_dialog_set_primary_text (GTH_EMBEDDED_DIALOG (task->priv->dialog), text);

	g_free (text);
	g_free (uri);

	return DIR_OP_CONTINUE;
}


static void
browser_location_ready_cb (GthBrowser    *browser,
			   GFile         *folder,
			   gboolean       error,
			   GthSearchTask *task)
{
	GtkWidget          *button;
	EmbeddedDialogData *dialog_data;
	GSettings          *settings;
	GString            *attributes;
	const char         *test_attributes;

	g_signal_handler_disconnect (task->priv->browser, task->priv->location_ready_id);

	if (error) {
		gth_task_completed (GTH_TASK (task), NULL);
		return;
	}

	task->priv->n_files = 0;

	task->priv->dialog = gth_browser_get_list_extra_widget (browser);
	gth_embedded_dialog_set_icon (GTH_EMBEDDED_DIALOG (task->priv->dialog), GTK_STOCK_FIND, GTK_ICON_SIZE_DIALOG);
	gth_embedded_dialog_set_primary_text (GTH_EMBEDDED_DIALOG (task->priv->dialog), _("Searching..."));
	update_secondary_text (task);
	gedit_message_area_clear_action_area (GEDIT_MESSAGE_AREA (task->priv->dialog));
	button = gtk_button_new ();
	gtk_container_add (GTK_CONTAINER (button), gtk_image_new_from_stock (GTK_STOCK_STOP, GTK_ICON_SIZE_BUTTON));
	gtk_button_set_relief (GTK_BUTTON (button), GTK_RELIEF_NONE);
	gtk_widget_set_tooltip_text (button, _("Cancel the operation"));
	gtk_widget_show_all (button);
	gedit_message_area_add_action_widget (GEDIT_MESSAGE_AREA (task->priv->dialog),
					      button,
					      GTK_RESPONSE_CANCEL);

	dialog_data = g_new0 (EmbeddedDialogData, 1);
	dialog_data->browser = task->priv->browser;
	dialog_data->task = task;
	dialog_data->response_id = g_signal_connect (task->priv->dialog,
						     "response",
						     G_CALLBACK (embedded_dialog_response_cb),
						     dialog_data);

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

	settings = g_settings_new (GTHUMB_BROWSER_SCHEMA);

	task->priv->show_hidden_files = g_settings_get_boolean (settings, PREF_BROWSER_SHOW_HIDDEN_FILES);
	task->priv->io_operation = TRUE;

	task->priv->file_source = gth_main_get_file_source (gth_search_get_folder (task->priv->search));
	gth_file_source_set_cancellable (task->priv->file_source, gth_task_get_cancellable (GTH_TASK (task)));

	attributes = g_string_new (g_settings_get_boolean (settings, PREF_BROWSER_FAST_FILE_TYPE) ? GFILE_STANDARD_ATTRIBUTES_WITH_FAST_CONTENT_TYPE : GFILE_STANDARD_ATTRIBUTES_WITH_CONTENT_TYPE);
	test_attributes = gth_test_get_attributes (GTH_TEST (task->priv->test));
	if (test_attributes[0] != '\0') {
		g_string_append (attributes, ",");
		g_string_append (attributes, test_attributes);
	}

	gth_file_source_for_each_child (task->priv->file_source,
					gth_search_get_folder (task->priv->search),
					gth_search_is_recursive (task->priv->search),
					attributes->str,
					start_dir_func,
					for_each_file_func,
					done_func,
					task);

	g_object_unref (settings);
	g_string_free (attributes, TRUE);
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
gth_search_task_cancelled (GthTask *task)
{
	if (! GTH_SEARCH_TASK (task)->priv->io_operation)
		gth_task_completed (task, g_error_new_literal (GTH_TASK_ERROR, GTH_TASK_ERROR_CANCELLED, ""));
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
	task->priv = g_new0 (GthSearchTaskPrivate, 1);
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
