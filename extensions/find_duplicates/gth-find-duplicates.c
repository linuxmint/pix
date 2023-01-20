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
#include <glib.h>
#include <glib/gi18n.h>
#include <gthumb.h>
#include <extensions/catalogs/gth-catalog.h>
#include <extensions/file_manager/actions.h>
#include "gth-find-duplicates.h"
#include "gth-folder-chooser-dialog.h"


#define GET_WIDGET(x) (_gtk_builder_get_widget (self->priv->builder, (x)))
#define BUFFER_SIZE 4096
#define SELECT_COMMAND_ID_DATA "delete-command-id"
#define PULSE_DELAY 50


enum {
	FILE_LIST_COLUMN_FILE = 0,
	FILE_LIST_COLUMN_CHECKED,
	FILE_LIST_COLUMN_FILENAME,
	FILE_LIST_COLUMN_POSITION,
	FILE_LIST_COLUMN_LAST_MODIFIED,
	FILE_LIST_COLUMN_VISIBLE,
	FILE_LIST_COLUMN_LAST_MODIFIED_TIME,
};


typedef enum {
	SELECT_LEAVE_NEWEST,
	SELECT_LEAVE_OLDEST,
	SELECT_BY_FOLDER,
	SELECT_ALL,
	SELECT_NONE
} SelectID;


typedef struct {
	const char *display_name;
	SelectID    id;
} SelectCommand;


SelectCommand select_commands[] = {
	{ N_("leave the newest duplicates"), SELECT_LEAVE_NEWEST },
	{ N_("leave the oldest duplicates"), SELECT_LEAVE_OLDEST },
	{ N_("by folder…"), SELECT_BY_FOLDER },
	{ N_("all files"), SELECT_ALL },
	{ N_("no file"), SELECT_NONE }
};


struct _GthFindDuplicatesPrivate {
	GthBrowser    *browser;
	GtkWidget     *dialog;
	GFile         *location;
	gboolean       recursive;
	GthTest       *test;
	GtkBuilder    *builder;
	GtkWidget     *duplicates_list;
	GtkWidget     *select_button;
	GtkWidget     *select_menu;
	GString       *attributes;
	GCancellable  *cancellable;
	gboolean       io_operation;
	gboolean       closing;
	GthFileSource *file_source;
	int            n_duplicates;
	goffset        duplicates_size;
	int            n_files;
	int            n_file;
	GList         *files;
	GList         *directories;
	GFile         *current_directory;
	GthFileData   *current_file;
	guchar         buffer[BUFFER_SIZE];
	GChecksum     *checksum;
	GInputStream  *file_stream;
	GHashTable    *duplicated;
	gulong         folder_changed_id;
	guint          pulse_event_id;
};


G_DEFINE_TYPE_WITH_CODE (GthFindDuplicates,
			 gth_find_duplicates,
			 G_TYPE_OBJECT,
			 G_ADD_PRIVATE (GthFindDuplicates))


typedef struct {
	GthFileData *file_data;
	GList       *files;
	goffset      total_size;
	int          n_files;
} DuplicatedData;


static DuplicatedData *
duplicated_data_new (void)
{
	DuplicatedData *d_data;

	d_data = g_new0 (DuplicatedData, 1);
	d_data->file_data = NULL;
	d_data->files = 0;
	d_data->total_size = 0;
	d_data->n_files = 0;

	return d_data;
}


static void
duplicated_data_free (DuplicatedData *d_data)
{
	_g_object_list_unref (d_data->files);
	_g_object_unref (d_data->file_data);
	g_free (d_data);
}


static void
gth_find_duplicates_finalize (GObject *object)
{
	GthFindDuplicates *self;

	self = GTH_FIND_DUPLICATES (object);

	if (self->priv->pulse_event_id != 0)
		g_source_remove (self->priv->pulse_event_id);
	if (self->priv->folder_changed_id != 0)
		g_signal_handler_disconnect (gth_main_get_default_monitor (),
					     self->priv->folder_changed_id);
	g_object_unref (self->priv->location);
	_g_object_unref (self->priv->test);
	_g_object_unref (self->priv->builder);
	if (self->priv->attributes != NULL)
		g_string_free (self->priv->attributes, TRUE);
	g_object_unref (self->priv->cancellable);
	_g_object_unref (self->priv->file_source);
	_g_object_list_unref (self->priv->files);
	_g_object_list_unref (self->priv->directories);
	_g_object_unref (self->priv->current_file);
	_g_object_unref (self->priv->current_directory);
	if (self->priv->checksum != NULL)
		g_checksum_free (self->priv->checksum);
	_g_object_unref (self->priv->file_stream);
	g_hash_table_unref (self->priv->duplicated);

	G_OBJECT_CLASS (gth_find_duplicates_parent_class)->finalize (object);
}


static void
gth_find_duplicates_class_init (GthFindDuplicatesClass *klass)
{
	GObjectClass *object_class;

	object_class = (GObjectClass*) klass;
	object_class->finalize = gth_find_duplicates_finalize;
}


static void
gth_find_duplicates_init (GthFindDuplicates *self)
{
	self->priv = gth_find_duplicates_get_instance_private (self);
	self->priv->test = NULL;
	self->priv->builder = NULL;
	self->priv->attributes = NULL;
	self->priv->io_operation = FALSE;
	self->priv->n_duplicates = 0;
	self->priv->duplicates_size = 0;
	self->priv->file_source = NULL;
	self->priv->files = NULL;
	self->priv->directories = NULL;
	self->priv->current_directory = NULL;
	self->priv->current_file = NULL;
	self->priv->checksum = NULL;
	self->priv->file_stream = NULL;
	self->priv->duplicated = g_hash_table_new_full (g_str_hash,
							g_str_equal,
							g_free,
							(GDestroyNotify) duplicated_data_free);
	self->priv->cancellable = g_cancellable_new ();
	self->priv->folder_changed_id = 0;
}


static void
update_file_list_sensitivity (GthFindDuplicates *self)
{
	GtkTreeModel *model;
	GtkTreeIter   iter;
	gboolean      one_active = FALSE;

	model = GTK_TREE_MODEL (GET_WIDGET ("files_liststore"));
	if (gtk_tree_model_get_iter_first (model, &iter)) {
		do {
			gboolean active;
			gboolean visible;

			gtk_tree_model_get (model, &iter,
					    FILE_LIST_COLUMN_CHECKED, &active,
					    FILE_LIST_COLUMN_VISIBLE, &visible,
					    -1);
			if (active && visible) {
				one_active = TRUE;
				break;
			}
		}
		while (gtk_tree_model_iter_next (model, &iter));
	}

	gtk_widget_set_sensitive (GET_WIDGET ("view_button"), one_active);
	gtk_widget_set_sensitive (GET_WIDGET ("delete_button"), one_active);
}


static void
update_file_list_selection_info (GthFindDuplicates *self)
{
	GtkTreeModel *model;
	GtkTreeIter   iter;
	int           n_files;
	goffset       total_size;
	char         *size_formatted;
	char         *text;

	n_files = 0;
	total_size = 0;

	model = GTK_TREE_MODEL (GET_WIDGET ("files_liststore"));
	if (gtk_tree_model_get_iter_first (model, &iter)) {
		do {
			GthFileData *file_data;
			gboolean     active;
			gboolean     visible;

			gtk_tree_model_get (model, &iter,
					    FILE_LIST_COLUMN_FILE, &file_data,
					    FILE_LIST_COLUMN_CHECKED, &active,
					    FILE_LIST_COLUMN_VISIBLE, &visible,
					    -1);

			if (active && visible) {
				n_files += 1;
				total_size += g_file_info_get_size (file_data->info);
			}

			_g_object_unref (file_data);
		}
		while (gtk_tree_model_iter_next (model, &iter));
	}

	size_formatted = g_format_size (total_size);
	text = g_strdup_printf (g_dngettext (NULL, "%d file (%s)", "%d files (%s)", n_files), n_files, size_formatted);
	gtk_label_set_text (GTK_LABEL (GET_WIDGET ("total_files_label")), text);

	g_free (text);
	g_free (size_formatted);
}


static GList *
get_duplicates_file_data_list (GthFindDuplicates *self)
{
	GtkWidget *duplicates_view;
	GList     *items;
	GList     *file_data_list;

	duplicates_view = gth_file_list_get_view (GTH_FILE_LIST (self->priv->duplicates_list));
	items = gth_file_selection_get_selected (GTH_FILE_SELECTION (duplicates_view));
	file_data_list = gth_file_list_get_files (GTH_FILE_LIST (self->priv->duplicates_list), items);
	if (file_data_list == NULL)
		file_data_list = gth_file_store_get_visibles (GTH_FILE_STORE (gth_file_view_get_model (GTH_FILE_VIEW (duplicates_view))));

	_gtk_tree_path_list_free (items);

	return file_data_list;
}


#if 0


static void
duplicates_list_view_selection_changed_cb (GthFileView *fileview,
					   gpointer     user_data)
{
	GthFindDuplicates *self = user_data;
	GList             *file_data_list;
	GList             *scan;

	file_data_list = get_duplicates_file_data_list (self);

	gtk_list_store_clear (GTK_LIST_STORE (GET_WIDGET ("files_liststore")));

	for (scan = file_data_list; scan; scan = scan->next) {
		GthFileData    *selected_file_data = (GthFileData *) scan->data;
		const char     *checksum;
		DuplicatedData *d_data;
		GList          *scan_duplicated;

		checksum = g_file_info_get_attribute_string (selected_file_data->info, "find-duplicates::checksum");
		d_data = g_hash_table_lookup (self->priv->duplicated, checksum);

		g_return_if_fail (d_data != NULL);

		for (scan_duplicated = d_data->files; scan_duplicated; scan_duplicated = scan_duplicated->next) {
			GthFileData *file_data = scan_duplicated->data;
			GFile       *parent;
			char        *parent_name;
			GtkTreeIter  iter;

			parent = g_file_get_parent (file_data->file);
			if (parent != NULL)
				parent_name = g_file_get_parse_name (parent);
			else
				parent_name = NULL;
			gtk_list_store_append (GTK_LIST_STORE (GET_WIDGET ("files_liststore")), &iter);
			gtk_list_store_set (GTK_LIST_STORE (GET_WIDGET ("files_liststore")), &iter,
					    FILE_LIST_COLUMN_FILE, file_data,
					    FILE_LIST_COLUMN_CHECKED, TRUE,
					    FILE_LIST_COLUMN_FILENAME, g_file_info_get_display_name (file_data->info),
					    FILE_LIST_COLUMN_POSITION, parent_name,
					    FILE_LIST_COLUMN_LAST_MODIFIED, g_file_info_get_attribute_string (file_data->info, "gth::file::display-mtime"),
					    -1);

			g_free (parent_name);
			g_object_unref (parent);
		}
	}

	update_file_list_sensitivity (self);
	update_file_list_selection_info (self);

	_g_object_list_unref (file_data_list);
}


#endif


static void
duplicates_list_view_selection_changed_cb (GthFileView *fileview,
					   gpointer     user_data)
{

	GthFindDuplicates *self = user_data;
	GList             *file_data_list;
	GHashTable        *selected_files;
	GtkTreeModel      *files_treemodel;
	GtkTreeModel      *files_treemodelfilter;
	GtkTreeIter        iter;
	GList             *scan;

	selected_files = g_hash_table_new_full (g_file_hash, (GEqualFunc) g_file_equal, g_object_unref, NULL);
	file_data_list = get_duplicates_file_data_list (self);
	for (scan = file_data_list; scan; scan = scan->next) {
		GthFileData    *selected_file_data = (GthFileData *) scan->data;
		const char     *checksum;
		DuplicatedData *d_data;
		GList          *scan_duplicated;

		checksum = g_file_info_get_attribute_string (selected_file_data->info, "find-duplicates::checksum");
		d_data = g_hash_table_lookup (self->priv->duplicated, checksum);

		g_return_if_fail (d_data != NULL);

		for (scan_duplicated = d_data->files; scan_duplicated; scan_duplicated = scan_duplicated->next) {
			GthFileData *file_data = scan_duplicated->data;
			g_hash_table_insert (selected_files, g_object_ref (file_data->file), GINT_TO_POINTER (1));
		}
	}

	files_treemodel = GTK_TREE_MODEL (GET_WIDGET ("files_liststore"));

	files_treemodelfilter = GTK_TREE_MODEL (GET_WIDGET ("files_treemodelfilter"));
	g_object_ref (files_treemodelfilter);
	gtk_tree_view_set_model (GTK_TREE_VIEW (GET_WIDGET ("files_treeview")), NULL); /* to avoid excessive recomputation */

	if (gtk_tree_model_get_iter_first (files_treemodel, &iter)) {
		do {
			GthFileData *file_data;

			gtk_tree_model_get (files_treemodel, &iter,
					    FILE_LIST_COLUMN_FILE, &file_data,
					    -1);
			gtk_list_store_set (GTK_LIST_STORE (files_treemodel), &iter,
					    FILE_LIST_COLUMN_VISIBLE, (g_hash_table_lookup (selected_files, file_data->file) != NULL),
					    -1);

			g_object_unref (file_data);
		}
		while (gtk_tree_model_iter_next (files_treemodel, &iter));
	}

	gtk_tree_view_set_model (GTK_TREE_VIEW (GET_WIDGET ("files_treeview")), files_treemodelfilter);
	g_object_unref (files_treemodelfilter);

	update_file_list_sensitivity (self);
	update_file_list_selection_info (self);

	_g_object_list_unref (file_data_list);
	g_hash_table_unref (selected_files);
}


static void
files_tree_view_selection_changed_cb (GtkTreeSelection *tree_selection,
				      gpointer          user_data)
{
	GthFindDuplicates *self = user_data;
	GtkTreeModel      *model;
	GtkTreeIter        iter;
	GthFileData       *file_data;
	const char        *checksum;
	DuplicatedData    *d_data;

	if (! gtk_tree_selection_get_selected (tree_selection, &model, &iter))
		return;

	gtk_tree_model_get (model, &iter,
			    FILE_LIST_COLUMN_FILE, &file_data,
			    -1);

	checksum = g_file_info_get_attribute_string (file_data->info, "find-duplicates::checksum");
	d_data = g_hash_table_lookup (self->priv->duplicated, checksum);
	if (d_data != NULL) {
		GtkWidget *duplicates_view;
		int        pos;

		duplicates_view = gth_file_list_get_view (GTH_FILE_LIST (self->priv->duplicates_list));
		pos = gth_file_store_get_pos (GTH_FILE_STORE (gth_file_view_get_model (GTH_FILE_VIEW (duplicates_view))), d_data->file_data->file);
		if (pos >= 0) {
			/* FIXME */
			/*g_signal_handlers_block_by_data (duplicates_view, self);
			gth_file_selection_select (GTH_FILE_SELECTION (duplicates_view), pos);*/
			gth_file_view_scroll_to (GTH_FILE_VIEW (duplicates_view), pos, 0.5);
			/*g_signal_handlers_unblock_by_data (duplicates_view, self);*/
		}
	}

	g_object_unref (file_data);
}


static void
update_total_duplicates_label (GthFindDuplicates *self)
{
	char *size_formatted;
	char *text;

	size_formatted = g_format_size (self->priv->duplicates_size);
	text = g_strdup_printf (g_dngettext (NULL, "%d file (%s)", "%d files (%s)", self->priv->n_duplicates), self->priv->n_duplicates, size_formatted);
	gtk_label_set_text (GTK_LABEL (GET_WIDGET ("total_duplicates_label")), text);

	g_free (text);
	g_free (size_formatted);
}


static void
folder_changed_cb (GthMonitor      *monitor,
		   GFile           *parent,
		   GList           *list,
		   int              position,
		   GthMonitorEvent  event,
		   gpointer         user_data)
{
	GthFindDuplicates *self = user_data;
	GList             *file_scan;

	if (event != GTH_MONITOR_EVENT_DELETED)
		return;

	for (file_scan = list; file_scan; file_scan = file_scan->next) {
		GFile *file = file_scan->data;
		GList *values;
		GList *scan;

		values = g_hash_table_get_values (self->priv->duplicated);
		for (scan = values; scan; scan = scan->next) {
			DuplicatedData *d_data = scan->data;
			GList          *link;
			char           *text;
			GList          *singleton;

			link = gth_file_data_list_find_file (d_data->files, file);
			if (link == NULL)
				continue;

			d_data->files = g_list_remove_link (d_data->files, link);
			d_data->n_files -= 1;
			d_data->total_size -= g_file_info_get_size (d_data->file_data->info);

			text = g_strdup_printf (g_dngettext (NULL, "%d duplicate", "%d duplicates", d_data->n_files - 1), d_data->n_files - 1);
			g_file_info_set_attribute_string (d_data->file_data->info,
							  "find-duplicates::n-duplicates",
							  text);
			g_free (text);

			singleton = g_list_append (NULL, d_data->file_data);
			if (d_data->n_files <= 1)
				gth_file_list_delete_files (GTH_FILE_LIST (self->priv->duplicates_list), singleton);
			else
				gth_file_list_update_files (GTH_FILE_LIST (self->priv->duplicates_list), singleton);
			g_list_free (singleton);

			self->priv->n_duplicates -= 1;
			self->priv->duplicates_size -= g_file_info_get_size (d_data->file_data->info);
			update_total_duplicates_label (self);

			_g_object_list_unref (link);
		}

		g_list_free (values);
	}

	duplicates_list_view_selection_changed_cb (NULL, self);
	update_file_list_sensitivity (self);
	update_file_list_selection_info (self);
}


static void
after_checksums (GthFindDuplicates *self)
{
	self->priv->folder_changed_id = g_signal_connect (gth_main_get_default_monitor (),
							  "folder-changed",
							  G_CALLBACK (folder_changed_cb),
							  self);

	gtk_notebook_set_current_page (GTK_NOTEBOOK (GET_WIDGET ("pages_notebook")), (self->priv->n_duplicates > 0) ? 0 : 1);
	gtk_label_set_text (GTK_LABEL (GET_WIDGET ("progress_label")), _("Search completed"));
	gtk_label_set_text (GTK_LABEL (GET_WIDGET ("search_details_label")), "");
	gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (GET_WIDGET ("search_progressbar")), 1.0);
	gtk_widget_set_sensitive (GET_WIDGET ("stop_button"), FALSE);
	duplicates_list_view_selection_changed_cb (NULL, self);
}


static void start_next_checksum (GthFindDuplicates *self);


static void
_file_list_add_file (GthFindDuplicates *self,
		     GthFileData       *file_data)
{
	GFile       *parent;
	char        *parent_name;
	GTimeVal     timeval;
	GtkTreeIter  iter;

	parent = g_file_get_parent (file_data->file);
	if (parent != NULL)
		parent_name = g_file_get_parse_name (parent);
	else
		parent_name = NULL;

	g_file_info_get_modification_time (file_data->info, &timeval);

	gtk_list_store_append (GTK_LIST_STORE (GET_WIDGET ("files_liststore")), &iter);
	gtk_list_store_set (GTK_LIST_STORE (GET_WIDGET ("files_liststore")), &iter,
			    FILE_LIST_COLUMN_FILE, file_data,
			    FILE_LIST_COLUMN_CHECKED, TRUE,
			    FILE_LIST_COLUMN_FILENAME, g_file_info_get_display_name (file_data->info),
			    FILE_LIST_COLUMN_POSITION, parent_name,
			    FILE_LIST_COLUMN_LAST_MODIFIED, g_file_info_get_attribute_string (file_data->info, "gth::file::display-mtime"),
			    FILE_LIST_COLUMN_VISIBLE, TRUE,
			    FILE_LIST_COLUMN_LAST_MODIFIED_TIME, timeval.tv_sec,
			    -1);

	g_free (parent_name);
	g_object_unref (parent);
}


static void
file_input_stream_read_ready_cb (GObject      *source,
		    	    	 GAsyncResult *result,
		    	    	 gpointer      user_data)
{
	GthFindDuplicates *self = user_data;
	GError            *error = NULL;
	gssize             buffer_size;

	self->priv->io_operation = FALSE;
	if (self->priv->closing) {
		gtk_widget_destroy (self->priv->dialog);
		return;
	}

	buffer_size = g_input_stream_read_finish (G_INPUT_STREAM (source), result, &error);

	if (buffer_size < 0) {
		start_next_checksum (self);
		return;
	}
	else if (buffer_size == 0) {
		const char     *checksum;
		DuplicatedData *d_data;

		self->priv->n_file += 1;

		g_object_unref (self->priv->file_stream);
		self->priv->file_stream = NULL;

		checksum = g_checksum_get_string (self->priv->checksum);
		g_file_info_set_attribute_string (self->priv->current_file->info,
						  "find-duplicates::checksum",
						  checksum);

		d_data = g_hash_table_lookup (self->priv->duplicated, checksum);
		if (d_data == NULL) {
			d_data = duplicated_data_new ();
			g_hash_table_insert (self->priv->duplicated, g_strdup (checksum), d_data);
		}
		if (d_data->file_data == NULL)
			d_data->file_data = g_object_ref (self->priv->current_file);
		d_data->files = g_list_prepend (d_data->files, g_object_ref (self->priv->current_file));
		d_data->n_files += 1;
		d_data->total_size += g_file_info_get_size (self->priv->current_file->info);
		if (d_data->n_files > 1) {
			char  *text;
			GList *singleton;

			text = g_strdup_printf (g_dngettext (NULL, "%d duplicate", "%d duplicates", d_data->n_files - 1), d_data->n_files - 1);
			g_file_info_set_attribute_string (d_data->file_data->info,
							  "find-duplicates::n-duplicates",
							  text);
			g_free (text);

			singleton = g_list_append (NULL, d_data->file_data);
			if (d_data->n_files == 2) {
				gth_file_list_add_files (GTH_FILE_LIST (self->priv->duplicates_list), singleton, -1);
				_file_list_add_file (self, d_data->file_data); /* add the first one as well */
			}
			else
				gth_file_list_update_files (GTH_FILE_LIST (self->priv->duplicates_list), singleton);
			_file_list_add_file (self, self->priv->current_file);
			g_list_free (singleton);

			self->priv->n_duplicates += 1;
			self->priv->duplicates_size += g_file_info_get_size (d_data->file_data->info);
			update_total_duplicates_label (self);
		}

		duplicates_list_view_selection_changed_cb (NULL, self);
		start_next_checksum (self);

		return;
	}

	self->priv->io_operation = TRUE;

	g_checksum_update (self->priv->checksum, self->priv->buffer, buffer_size);
	g_input_stream_read_async (self->priv->file_stream,
				   self->priv->buffer,
				   BUFFER_SIZE,
				   G_PRIORITY_DEFAULT,
				   self->priv->cancellable,
				   file_input_stream_read_ready_cb,
				   self);
}


static void
read_current_file_ready_cb (GObject      *source,
			    GAsyncResult *result,
			    gpointer      user_data)
{
	GthFindDuplicates *self = user_data;
	GError            *error = NULL;

	self->priv->io_operation = FALSE;
	if (self->priv->closing) {
		gtk_widget_destroy (self->priv->dialog);
		return;
	}

	if (self->priv->file_stream != NULL)
		g_object_unref (self->priv->file_stream);
	self->priv->file_stream = (GInputStream *) g_file_read_finish (G_FILE (source), result, &error);
	if (self->priv->file_stream == NULL) {
		start_next_checksum (self);
		return;
	}

	self->priv->io_operation = TRUE;
	g_input_stream_read_async (self->priv->file_stream,
				   self->priv->buffer,
				   BUFFER_SIZE,
				   G_PRIORITY_DEFAULT,
				   self->priv->cancellable,
				   file_input_stream_read_ready_cb,
				   self);
}


static void
start_next_checksum (GthFindDuplicates *self)
{
	GList *link;
	char  *text;
	int    n_remaining;

	link = self->priv->files;
	if (link == NULL) {
		after_checksums (self);
		return;
	}

	self->priv->files = g_list_remove_link (self->priv->files, link);
	_g_object_unref (self->priv->current_file);
	self->priv->current_file = (GthFileData *) link->data;
	g_list_free (link);

	gtk_label_set_text (GTK_LABEL (GET_WIDGET ("progress_label")), _("Searching for duplicates"));

	n_remaining = self->priv->n_files - self->priv->n_file;
	text = g_strdup_printf (g_dngettext (NULL, "%d file remaining", "%d files remaining", n_remaining), n_remaining);
	gtk_label_set_text (GTK_LABEL (GET_WIDGET ("search_details_label")), text);
	g_free (text);

	gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (GET_WIDGET ("search_progressbar")),
				       (double) (self->priv->n_file + 1) / (self->priv->n_files + 1));

	if (self->priv->checksum == NULL)
		self->priv->checksum = g_checksum_new (G_CHECKSUM_MD5);
	else
		g_checksum_reset (self->priv->checksum);

	self->priv->io_operation = TRUE;
	g_file_read_async (self->priv->current_file->file,
			   G_PRIORITY_DEFAULT,
			   self->priv->cancellable,
			   read_current_file_ready_cb,
			   self);
}


/* -- search_directory -- */


static void
done_func (GObject  *object,
	   GError   *error,
	   gpointer  user_data)
{
	GthFindDuplicates *self = user_data;
	GList             *scan;
	GHashTable        *file_sizes;
	GList             *possible_duplicates;

	g_source_remove (self->priv->pulse_event_id);
	self->priv->pulse_event_id = 0;
	self->priv->io_operation = FALSE;

	if (self->priv->closing) {
		gtk_widget_destroy (self->priv->dialog);
		return;
	}

	if ((error != NULL) && ! g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED)) {
		_gtk_error_dialog_from_gerror_show (GTK_WINDOW (self->priv->browser), _("Could not perform the operation"), error);
		gtk_widget_destroy (self->priv->dialog);
		return;
	}

	/* ignore files with an unique size */

	file_sizes = g_hash_table_new_full (g_int64_hash, g_int64_equal, NULL, NULL);
	for (scan = self->priv->files; scan; scan = scan->next) {
		GthFileData *file_data = scan->data;
		gpointer     value;
		int          n_files;
		gint64       size;

		size = g_file_info_get_size (file_data->info);
		value = g_hash_table_lookup (file_sizes, &size);
		n_files = (value == NULL) ? 0 : GPOINTER_TO_INT (value);
		n_files += 1;
		g_hash_table_insert (file_sizes, &size, GINT_TO_POINTER (n_files));
	}

	possible_duplicates = NULL;
	for (scan = self->priv->files; scan; scan = scan->next) {
		GthFileData *file_data = scan->data;
		gint64       size;
		gpointer     value;
		int          n_files;

		size = g_file_info_get_size (file_data->info);
		value = g_hash_table_lookup (file_sizes, &size);
		if (value == NULL)
			continue;

		n_files = GPOINTER_TO_INT (value);
		if (n_files > 1)
			possible_duplicates = g_list_prepend (possible_duplicates, g_object_ref (file_data));
	}

	g_hash_table_destroy (file_sizes);

	/* start computing checksums */

	_g_object_list_unref (self->priv->files);
	self->priv->files = possible_duplicates;
	self->priv->n_files = g_list_length (self->priv->files);
	self->priv->n_file = 0;
	start_next_checksum (self);
}


static void
for_each_file_func (GFile     *file,
		    GFileInfo *info,
		    gpointer   user_data)
{
	GthFindDuplicates *self = user_data;
	GthFileData       *file_data;

	if (g_file_info_get_file_type (info) != G_FILE_TYPE_REGULAR)
		return;

	file_data = gth_file_data_new (file, info);
	if (gth_test_match (self->priv->test, file_data))
		self->priv->files = g_list_prepend (self->priv->files, g_object_ref (file_data));

	g_object_unref (file_data);
}


static DirOp
start_dir_func (GFile      *directory,
		GFileInfo  *info,
		GError    **error,
		gpointer    user_data)
{
	GthFindDuplicates *self = user_data;

	_g_object_unref (self->priv->current_directory);
	self->priv->current_directory = g_object_ref (directory);

	return DIR_OP_CONTINUE;
}


static gboolean
pulse_progressbar_cb (gpointer user_data)
{
	GthFindDuplicates *self = user_data;

	gtk_progress_bar_pulse (GTK_PROGRESS_BAR (GET_WIDGET ("search_progressbar")));

	return TRUE;
}


static void
search_directory (GthFindDuplicates *self,
		  GFile             *directory)
{
	gtk_widget_set_sensitive (GET_WIDGET ("stop_button"), TRUE);
	self->priv->io_operation = TRUE;

	gtk_label_set_text (GTK_LABEL (GET_WIDGET ("progress_label")), _("Getting the file list"));
	gtk_label_set_text (GTK_LABEL (GET_WIDGET ("search_details_label")), "");
	gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (GET_WIDGET ("search_progressbar")), 0.0);
	self->priv->pulse_event_id = g_timeout_add (PULSE_DELAY, pulse_progressbar_cb, self);

	gth_file_source_for_each_child (self->priv->file_source,
					directory,
					self->priv->recursive,
					self->priv->attributes->str,
					start_dir_func,
					for_each_file_func,
					done_func,
					self);
}


static void
find_duplicates_dialog_destroy_cb (GtkWidget *dialog,
				   gpointer   user_data)
{
	g_object_unref (GTH_FIND_DUPLICATES (user_data));
}


static void
close_button_clicked_cb (GtkButton *button,
			 gpointer   user_data)
{
	GthFindDuplicates *self = user_data;

	if (! self->priv->io_operation) {
		gtk_widget_destroy (self->priv->dialog);
	}
	else {
		self->priv->closing = TRUE;
		g_cancellable_cancel (self->priv->cancellable);
	}
}


static void
file_cellrenderertoggle_toggled_cb (GtkCellRendererToggle *cell_renderer,
                		    char                  *path,
                		    gpointer               user_data)
{
	GthFindDuplicates *self = user_data;
	GtkTreeModel      *model;
	GtkTreePath       *filter_path;
	GtkTreePath       *child_path;
	GtkTreeIter        iter;
	gboolean           active;

	model = GTK_TREE_MODEL (GET_WIDGET ("files_liststore"));
	filter_path = gtk_tree_path_new_from_string (path);
	child_path = gtk_tree_model_filter_convert_path_to_child_path (GTK_TREE_MODEL_FILTER (GET_WIDGET ("files_treemodelfilter")), filter_path);
	if (! gtk_tree_model_get_iter (model, &iter, child_path)) {
		gtk_tree_path_free (child_path);
		gtk_tree_path_free (filter_path);
		return;
	}

	gtk_tree_model_get (model, &iter, FILE_LIST_COLUMN_CHECKED, &active, -1);
	gtk_list_store_set (GTK_LIST_STORE (model), &iter, FILE_LIST_COLUMN_CHECKED, ! active, -1);

	update_file_list_sensitivity (self);
	update_file_list_selection_info (self);

	gtk_tree_path_free (child_path);
	gtk_tree_path_free (filter_path);
}


static void
_file_list_set_sort_column_id (GthFindDuplicates *self,
			       GtkTreeViewColumn *treeviewcolumn,
			       int                column)
{
	int          old_column;
	GtkSortType  order;
	GList       *columns;
	GList       *scan;

	gtk_tree_sortable_get_sort_column_id (GTK_TREE_SORTABLE (GET_WIDGET ("files_liststore")), &old_column, &order);
	if (column == old_column)
		order = (order == GTK_SORT_ASCENDING) ? GTK_SORT_DESCENDING : GTK_SORT_ASCENDING;
	else
		order = GTK_SORT_ASCENDING;

	gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (GET_WIDGET ("files_liststore")), column, order);

	columns = gtk_tree_view_get_columns (GTK_TREE_VIEW (GET_WIDGET ("files_treeview")));
	for (scan = columns; scan; scan = scan->next) {
		GtkTreeViewColumn *c = scan->data;
		gtk_tree_view_column_set_sort_indicator (c, c == treeviewcolumn);
	}
	g_list_free (columns);

	gtk_tree_view_column_set_sort_order (treeviewcolumn, order);
}


static void
file_treeviewcolumn_clicked_cb (GtkTreeViewColumn *treeviewcolumn,
				gpointer           user_data)
{
	GthFindDuplicates *self = user_data;
	_file_list_set_sort_column_id (self, treeviewcolumn, FILE_LIST_COLUMN_FILENAME);
}


static void
modified_treeviewcolumn_clicked_cb (GtkTreeViewColumn *treeviewcolumn,
				    gpointer           user_data)
{
	GthFindDuplicates *self = user_data;
	_file_list_set_sort_column_id (self, treeviewcolumn, FILE_LIST_COLUMN_LAST_MODIFIED_TIME);
}


static void
position_treeviewcolumn_clicked_cb (GtkTreeViewColumn *treeviewcolumn,
				    gpointer           user_data)
{
	GthFindDuplicates *self = user_data;
	_file_list_set_sort_column_id (self, treeviewcolumn, FILE_LIST_COLUMN_POSITION);
}


static GList *
get_selected_files (GthFindDuplicates *self)
{
	GtkTreeModel *model;
	GtkTreeIter   iter;
	GList        *list;

	model = GTK_TREE_MODEL (GET_WIDGET ("files_liststore"));
	if (! gtk_tree_model_get_iter_first (model, &iter))
		return NULL;

	list = NULL;
	do {
		GthFileData *file_data;
		gboolean     active;
		gboolean     visible;

		gtk_tree_model_get (model, &iter,
				    FILE_LIST_COLUMN_FILE, &file_data,
				    FILE_LIST_COLUMN_CHECKED, &active,
				    FILE_LIST_COLUMN_VISIBLE, &visible,
				    -1);
		if (active && visible)
			list = g_list_prepend (list, g_object_ref (file_data));

		g_object_unref (file_data);
	}
	while (gtk_tree_model_iter_next (model, &iter));

	return g_list_reverse (list);
}


static void
view_button_clicked_cb (GtkWidget *button,
			gpointer   user_data)
{
	GthFindDuplicates *self = user_data;
	GList             *file_data_list;
	GList             *file_list;
	GthCatalog        *catalog;
	GFile             *catalog_file;

	file_data_list = get_selected_files (self);
	if (file_data_list == NULL)
		return;

	file_list = gth_file_data_list_to_file_list (file_data_list);
	catalog = gth_catalog_new ();
	catalog_file = gth_catalog_file_from_relative_path (_("Duplicates"), ".catalog");
	gth_catalog_set_file (catalog, catalog_file);
	gth_catalog_set_file_list (catalog, file_list);
	gth_catalog_save (catalog);
	gth_browser_go_to (self->priv->browser, catalog_file, NULL);

	g_object_unref (catalog_file);
	g_object_unref (catalog);
	_g_object_list_unref (file_list);
	_g_object_list_unref (file_data_list);
}


static void
delete_button_clicked_cb (GtkWidget *button,
			  gpointer   user_data)
{
	GthFindDuplicates *self = user_data;
	GList             *file_data_list;

	file_data_list = get_selected_files (self);
	if (file_data_list != NULL) {
		gth_file_mananger_delete_files (GTK_WINDOW (self->priv->dialog), file_data_list);
		_g_object_list_unref (file_data_list);
	}
}


static void
select_files_leaving_one (GthFindDuplicates *self,
			  GtkTreeModel      *model,
			  SelectID           selection_type)
{
	GHashTable  *newest_files;
	GList       *file_data_list;
	GList       *scan;
	GtkTreeIter  iter;

	newest_files = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_object_unref);
	file_data_list = get_duplicates_file_data_list (self);

	for (scan = file_data_list; scan; scan = scan->next) {
		GthFileData    *selected_file_data = (GthFileData *) scan->data;
		const char     *checksum;
		DuplicatedData *d_data;
		GList          *scan_duplicated;
		GthFileData    *newest_file = NULL;

		checksum = g_file_info_get_attribute_string (selected_file_data->info, "find-duplicates::checksum");
		d_data = g_hash_table_lookup (self->priv->duplicated, checksum);

		g_return_if_fail (d_data != NULL);

		for (scan_duplicated = d_data->files; scan_duplicated; scan_duplicated = scan_duplicated->next) {
			GthFileData *file_data = scan_duplicated->data;

			if (newest_file != NULL) {
				GTimeVal *t_newest_file;
				GTimeVal *t_file_data;
				gboolean  is_newest = FALSE;

				t_newest_file = gth_file_data_get_modification_time (newest_file);
				t_file_data = gth_file_data_get_modification_time (file_data);

				switch (selection_type) {
				case SELECT_LEAVE_NEWEST:
					is_newest = _g_time_val_cmp (t_file_data, t_newest_file) > 0;
					break;
				case SELECT_LEAVE_OLDEST:
					is_newest = _g_time_val_cmp (t_file_data, t_newest_file) < 0;
					break;
				default:
					break;
				}

				if (is_newest) {
					g_object_unref (newest_file);
					newest_file = g_object_ref (file_data);
				}
			}
			else
				newest_file = g_object_ref (file_data);
		}

		g_hash_table_insert (newest_files, g_strdup (checksum), newest_file);
	}

	if (gtk_tree_model_get_iter_first (model, &iter)) {
		do {
			GthFileData *file_data;
			gboolean     visible;
			const char  *checksum;
			GthFileData *newest_file;
			gboolean     active;

			gtk_tree_model_get (model, &iter,
					    FILE_LIST_COLUMN_FILE, &file_data,
					    FILE_LIST_COLUMN_VISIBLE, &visible,
					    -1);

			if (visible) {
				checksum = g_file_info_get_attribute_string (file_data->info, "find-duplicates::checksum");
				newest_file = g_hash_table_lookup (newest_files, checksum);
				active = ((newest_file == NULL) || ! g_file_equal (newest_file->file, file_data->file));
				gtk_list_store_set (GTK_LIST_STORE (model), &iter,
						    FILE_LIST_COLUMN_CHECKED, active,
						    -1);
			}

			g_object_unref (file_data);
		}
		while (gtk_tree_model_iter_next (model, &iter));
	}

	_g_object_list_unref (file_data_list);
	g_hash_table_unref (newest_files);
}


static void
select_menu_item_activate_cb (GtkMenuItem *menu_item,
         		      gpointer     user_data)
{
	GthFindDuplicates *self = user_data;
	SelectID           id;
	GtkTreeModel      *model;
	GtkTreeIter        iter;

	model = GTK_TREE_MODEL (GET_WIDGET ("files_liststore"));

	id = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (menu_item), SELECT_COMMAND_ID_DATA));
	switch (id) {
	case SELECT_ALL:
	case SELECT_NONE:
		if (gtk_tree_model_get_iter_first (model, &iter)) {
			do {
				gboolean visible;

				gtk_tree_model_get (GTK_TREE_MODEL (model), &iter,
						    FILE_LIST_COLUMN_VISIBLE, &visible,
						    -1);
				if (visible)
					gtk_list_store_set (GTK_LIST_STORE (model), &iter,
							    FILE_LIST_COLUMN_CHECKED, (id == SELECT_ALL),
							    -1);
			}
			while (gtk_tree_model_iter_next (model, &iter));
		}
		break;

	case SELECT_BY_FOLDER:
		{
			GHashTable *folders_table;
			GList      *folders = NULL;
			GtkWidget  *dialog;
			GHashTable *selected_folders = NULL;

			folders_table = g_hash_table_new_full (g_file_hash, (GEqualFunc) g_file_equal, g_object_unref, NULL);

			if (gtk_tree_model_get_iter_first (model, &iter)) {
				do {
					GthFileData *file_data;
					gboolean     visible;
					GFile       *folder;

					gtk_tree_model_get (model, &iter,
							    FILE_LIST_COLUMN_FILE, &file_data,
							    FILE_LIST_COLUMN_VISIBLE, &visible,
							    -1);

					if (visible) {
						folder = g_file_get_parent (file_data->file);
						if (folder != NULL) {
							if (g_hash_table_lookup (folders_table, folder) == NULL)
								g_hash_table_insert (folders_table, g_object_ref (folder), GINT_TO_POINTER (1));

							g_object_unref (folder);
						}
					}

					g_object_unref (file_data);
				}
				while (gtk_tree_model_iter_next (model, &iter));

				folders = g_hash_table_get_keys (folders_table);
			}

			dialog = gth_folder_chooser_dialog_new (folders);
			gtk_window_set_transient_for (GTK_WINDOW (dialog), GTK_WINDOW (self->priv->dialog));
			gtk_window_set_modal (GTK_WINDOW (dialog), TRUE);
			gtk_widget_show (dialog);

			switch (gtk_dialog_run (GTK_DIALOG (dialog))) {
			case GTK_RESPONSE_OK:
				selected_folders = gth_folder_chooser_dialog_get_selected (GTH_FOLDER_CHOOSER_DIALOG (dialog));
				break;
			default:
				break;
			}
			gtk_widget_destroy (dialog);

			if (selected_folders != NULL) {
				if (gtk_tree_model_get_iter_first (model, &iter)) {
					do {
						GthFileData *file_data;
						gboolean     visible;

						gtk_tree_model_get (model, &iter,
								    FILE_LIST_COLUMN_FILE, &file_data,
								    FILE_LIST_COLUMN_VISIBLE, &visible,
								    -1);
						if (visible) {
							GFile    *parent;
							gboolean  active;

							parent = g_file_get_parent (file_data->file);
							active = (parent != NULL) && g_hash_table_lookup (selected_folders, parent) != NULL;
							gtk_list_store_set (GTK_LIST_STORE (model), &iter, 1, active, -1);

							_g_object_unref (parent);
						}

						g_object_unref (file_data);
					}
					while (gtk_tree_model_iter_next (model, &iter));
				}

				g_hash_table_unref (selected_folders);
			}

			g_list_free (folders);
			g_hash_table_unref (folders_table);
		}
		break;

	case SELECT_LEAVE_NEWEST:
	case SELECT_LEAVE_OLDEST:
		select_files_leaving_one (self, model, id);
		break;
	}

	update_file_list_sensitivity (self);
	update_file_list_selection_info (self);
}


void
gth_find_duplicates_exec (GthBrowser *browser,
		     	  GFile      *location,
		     	  gboolean    recursive,
		     	  const char *filter)
{
	GthFindDuplicates *self;
	GSettings         *settings;
	const char        *test_attributes;
	int                i;

	g_return_if_fail (location != NULL);

	self = (GthFindDuplicates *) g_object_new (GTH_TYPE_FIND_DUPLICATES, NULL);

	settings = g_settings_new (GTHUMB_BROWSER_SCHEMA);

	self->priv->browser = browser;
	self->priv->location = g_object_ref (location);
	self->priv->recursive = recursive;
	if (filter != NULL)
		self->priv->test = gth_main_get_registered_object (GTH_TYPE_TEST, filter);

	self->priv->file_source = gth_main_get_file_source (self->priv->location);
	gth_file_source_set_cancellable (self->priv->file_source, self->priv->cancellable);

	self->priv->attributes = g_string_new (g_settings_get_boolean (settings, PREF_BROWSER_FAST_FILE_TYPE) ? GFILE_STANDARD_ATTRIBUTES_WITH_FAST_CONTENT_TYPE : GFILE_STANDARD_ATTRIBUTES_WITH_CONTENT_TYPE);
	g_string_append (self->priv->attributes, ",gth::file::display-size");
	test_attributes = gth_test_get_attributes (self->priv->test);
	if (test_attributes[0] != '\0') {
		g_string_append (self->priv->attributes, ",");
		g_string_append (self->priv->attributes, test_attributes);
	}

	self->priv->builder = _gtk_builder_new_from_file ("find-duplicates-dialog.ui", "find_duplicates");

	self->priv->dialog = g_object_new (GTK_TYPE_DIALOG,
			     	     	   "title", _("Find Duplicates"),
					   "transient-for", GTK_WINDOW (self->priv->browser),
					   "modal", FALSE,
					   "destroy-with-parent", FALSE,
					   "use-header-bar", _gtk_settings_get_dialogs_use_header (),
					   NULL);
	gtk_container_add (GTK_CONTAINER (gtk_dialog_get_content_area (GTK_DIALOG (self->priv->dialog))),
			   _gtk_builder_get_widget (self->priv->builder, "dialog_content"));
	gtk_dialog_add_buttons (GTK_DIALOG (self->priv->dialog),
				_GTK_LABEL_CLOSE, GTK_RESPONSE_CLOSE,
				NULL);

	self->priv->duplicates_list = gth_file_list_new (gth_grid_view_new (), GTH_FILE_LIST_MODE_NORMAL, FALSE);
	gtk_tree_model_filter_set_visible_column (GTK_TREE_MODEL_FILTER (GET_WIDGET ("files_treemodelfilter")), FILE_LIST_COLUMN_VISIBLE);
	gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (GET_WIDGET ("files_liststore")), FILE_LIST_COLUMN_FILENAME, GTK_SORT_ASCENDING);
	gth_file_selection_set_selection_mode (GTH_FILE_SELECTION (gth_file_list_get_view (GTH_FILE_LIST (self->priv->duplicates_list))), GTK_SELECTION_MULTIPLE);
	gth_file_list_set_caption (GTH_FILE_LIST (self->priv->duplicates_list), "find-duplicates::n-duplicates,gth::file::display-size");
	gth_file_list_set_thumb_size (GTH_FILE_LIST (self->priv->duplicates_list), 112);
	gtk_widget_set_size_request (self->priv->duplicates_list, 750, 300);
	gtk_widget_show (self->priv->duplicates_list);
	gtk_box_pack_start (GTK_BOX (GET_WIDGET ("duplicates_list_box")), self->priv->duplicates_list, TRUE, TRUE, 0);

	self->priv->select_button = gtk_menu_button_new ();
	gtk_container_add (GTK_CONTAINER (self->priv->select_button), gtk_label_new (_("Select")));
	gtk_widget_show_all (self->priv->select_button);
	gtk_box_pack_start (GTK_BOX (GET_WIDGET ("select_button_box")), self->priv->select_button, FALSE, FALSE, 0);

	self->priv->select_menu = gtk_menu_new ();
	for (i = 0; i < G_N_ELEMENTS (select_commands); i++) {
		SelectCommand *command = &select_commands[i];
		GtkWidget     *menu_item;

		menu_item = gtk_menu_item_new_with_label (_(command->display_name));
		g_object_set_data (G_OBJECT (menu_item), SELECT_COMMAND_ID_DATA, GINT_TO_POINTER (command->id));
		gtk_widget_show (menu_item);
		g_signal_connect (menu_item,
				  "activate",
				  G_CALLBACK (select_menu_item_activate_cb),
				  self);

		gtk_menu_shell_append (GTK_MENU_SHELL (self->priv->select_menu), menu_item);
	}
	gtk_menu_button_set_popup (GTK_MENU_BUTTON (self->priv->select_button), self->priv->select_menu);

	g_object_unref (settings);

	g_signal_connect (self->priv->dialog,
			  "destroy",
			  G_CALLBACK (find_duplicates_dialog_destroy_cb),
			  self);
	g_signal_connect (gtk_dialog_get_widget_for_response (GTK_DIALOG (self->priv->dialog), GTK_RESPONSE_CLOSE),
			  "clicked",
			  G_CALLBACK (close_button_clicked_cb),
			  self);
	g_signal_connect_swapped (GET_WIDGET ("stop_button"),
				  "clicked",
				  G_CALLBACK (g_cancellable_cancel),
				  self->priv->cancellable);
	g_signal_connect (gth_file_list_get_view (GTH_FILE_LIST (self->priv->duplicates_list)),
			  "file-selection-changed",
			  G_CALLBACK (duplicates_list_view_selection_changed_cb),
			  self);
	g_signal_connect (gtk_tree_view_get_selection (GTK_TREE_VIEW (GET_WIDGET ("files_treeview"))),
			  "changed",
			  G_CALLBACK (files_tree_view_selection_changed_cb),
			  self);
	g_signal_connect (GET_WIDGET ("file_cellrenderertoggle"),
			  "toggled",
			  G_CALLBACK (file_cellrenderertoggle_toggled_cb),
			  self);
	g_signal_connect (GET_WIDGET ("file_treeviewcolumn"),
			  "clicked",
			  G_CALLBACK (file_treeviewcolumn_clicked_cb),
			  self);
	g_signal_connect (GET_WIDGET ("modified_treeviewcolumn"),
			  "clicked",
			  G_CALLBACK (modified_treeviewcolumn_clicked_cb),
			  self);
	g_signal_connect (GET_WIDGET ("position_treeviewcolumn"),
			  "clicked",
			  G_CALLBACK (position_treeviewcolumn_clicked_cb),
			  self);
	g_signal_connect (GET_WIDGET ("view_button"),
			  "clicked",
			  G_CALLBACK (view_button_clicked_cb),
			  self);
	g_signal_connect (GET_WIDGET ("delete_button"),
			  "clicked",
			  G_CALLBACK (delete_button_clicked_cb),
			  self);

	gtk_widget_show (self->priv->dialog);
	search_directory (self, self->priv->location);
}
