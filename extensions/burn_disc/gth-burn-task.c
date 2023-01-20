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
#include <brasero-burn-dialog.h>
#include <brasero-burn-lib.h>
#include <brasero-burn-options.h>
#include <brasero-media.h>
#include <brasero-session.h>
#include <brasero-session-cfg.h>
#include <brasero-track-data-cfg.h>
#include <gthumb.h>
#include "gth-burn-task.h"


struct _GthBurnTaskPrivate {
	GthBrowser          *browser;
	GFile               *location;
	GList               *selected_files;
	GtkWidget           *dialog;
	GtkBuilder          *builder;
	GthTest             *test;
	GthFileSource       *file_source;
	char                *base_directory;
	char                *current_directory;
	GHashTable          *content;
	GHashTable          *parents;
	BraseroSessionCfg   *session;
	BraseroTrackDataCfg *track;
};


G_DEFINE_TYPE_WITH_CODE (GthBurnTask,
			 gth_burn_task,
			 GTH_TYPE_TASK,
			 G_ADD_PRIVATE (GthBurnTask))


static void
free_file_list_from_content (gpointer key,
			     gpointer value,
			     gpointer user_data)
{
	_g_object_list_unref (value);
}


static void
gth_burn_task_finalize (GObject *object)
{
	GthBurnTask *task;

	task = GTH_BURN_TASK (object);

	gtk_widget_destroy (task->priv->dialog);
	g_hash_table_foreach (task->priv->content, free_file_list_from_content, NULL);
	g_hash_table_unref (task->priv->content);
	g_hash_table_unref (task->priv->parents);
	g_free (task->priv->current_directory);
	_g_object_unref (task->priv->file_source);
	_g_object_unref (task->priv->test);
	_g_object_unref (task->priv->builder);
	_g_object_list_unref (task->priv->selected_files);
	g_free (task->priv->base_directory);
	g_object_unref (task->priv->location);
	g_object_unref (task->priv->browser);

	G_OBJECT_CLASS (gth_burn_task_parent_class)->finalize (object);
}



static void
add_file_to_track (GthBurnTask *task,
		   const char  *parent_uri,
		   const char  *relative_subfolder,
		   GFile       *file)
{
	char        *relative_parent;
	GtkTreePath *tree_path;
	char        *uri;

	relative_parent = g_build_path ("/", parent_uri + strlen (task->priv->base_directory), relative_subfolder, NULL);
	if (relative_parent != NULL) {
		char **subfolders;
		int    i;
		char  *subfolder;

		/* add all the subfolders to the track data */

		subfolder = NULL;
		subfolders = g_strsplit (relative_parent, "/", -1);
		for (i = 0; subfolders[i] != NULL; i++) {
			char *subfolder_parent;

			subfolder_parent = subfolder;
			if (subfolder_parent != NULL)
				subfolder = g_strconcat (subfolder_parent, "/", subfolders[i], NULL);
			else
				subfolder = g_strdup (subfolders[i]);

			if ((strcmp (subfolder, "") != 0) && g_hash_table_lookup (task->priv->parents, subfolder) == NULL) {
				GtkTreePath *subfolder_parent_tpath;
				GtkTreePath *subfolder_tpath;
				char        *basename;

				if (subfolder_parent != NULL)
					subfolder_parent_tpath = g_hash_table_lookup (task->priv->parents, subfolder_parent);
				else
					subfolder_parent_tpath = NULL;
				basename = _g_uri_get_basename (subfolder);
				subfolder_tpath = brasero_track_data_cfg_add_empty_directory (task->priv->track, basename, subfolder_parent_tpath);
				g_hash_table_insert (task->priv->parents, g_strdup (subfolder), subfolder_tpath);

				g_free (basename);
			}

			g_free (subfolder_parent);
		}

		g_free (subfolder);
		g_strfreev (subfolders);
	}

	tree_path = NULL;
	if (relative_parent != NULL)
		tree_path = g_hash_table_lookup (task->priv->parents, relative_parent);
	uri = g_file_get_uri (file);
	brasero_track_data_cfg_add (task->priv->track, uri, tree_path);

	g_free (uri);
	g_free (relative_parent);
}


static void
add_content_list (gpointer key,
		  gpointer value,
		  gpointer user_data)
{
	GthBurnTask *task = user_data;
	char        *parent_uri = key;
	GList       *files = value;
	GList       *scan;

	for (scan = files; scan; scan = scan->next)
		add_file_to_track (task, parent_uri, NULL, (GFile *) scan->data);

	for (scan = files; scan; scan = scan->next) {
		GFile *file = scan->data;
		GFile *file_parent;
		GList *file_sidecars = NULL;
		GList *scan_sidecars;

		file_parent = g_file_get_parent (file);
		gth_hook_invoke ("add-sidecars", file, &file_sidecars);
		for (scan_sidecars = file_sidecars; scan_sidecars; scan_sidecars = scan_sidecars->next) {
			GFile *sidecar = scan_sidecars->data;
			char  *relative_path;
			char  *subfolder_path;

			if (! g_file_query_exists (sidecar, NULL))
				continue;

			relative_path = g_file_get_relative_path (file_parent, sidecar);
			subfolder_path = _g_uri_get_parent (relative_path);
			if (g_strcmp0 (subfolder_path, "") == 0) {
				g_free (subfolder_path);
				subfolder_path = NULL;
			}
			add_file_to_track (task, parent_uri, subfolder_path, sidecar);
		}

		_g_object_list_unref (file_sidecars);
		g_object_unref (file_parent);
	}
}


static void
label_entry_changed_cb (GtkEntry           *entry,
			BraseroBurnSession *session)
{
	brasero_burn_session_set_label (session, gtk_entry_get_text (entry));
}


static void
burn_content_to_disc (GthBurnTask *task)
{
	static gboolean  initialized = FALSE;
	GdkCursor       *cursor;
	GtkWidget       *dialog;
	GtkBuilder      *builder;
	GtkWidget       *options;
	GtkResponseType  result;

	cursor = _gdk_cursor_new_for_widget (GTK_WIDGET (task->priv->browser), GDK_WATCH);
	gdk_window_set_cursor (gtk_widget_get_window (GTK_WIDGET (task->priv->browser)), cursor);
	g_object_unref (cursor);

	if (! initialized) {
		brasero_media_library_start ();
		brasero_burn_library_start (NULL, NULL);
		initialized = TRUE;
	}

	task->priv->session = brasero_session_cfg_new ();
	task->priv->track = brasero_track_data_cfg_new ();
	brasero_burn_session_add_track (BRASERO_BURN_SESSION (task->priv->session),
					BRASERO_TRACK (task->priv->track),
					NULL);
	g_object_unref (task->priv->track);

	g_hash_table_foreach (task->priv->content, add_content_list, task);

	dialog = brasero_burn_options_new (task->priv->session);
	gtk_window_set_icon_name (GTK_WINDOW (dialog), gtk_window_get_icon_name (GTK_WINDOW (task->priv->browser)));
	gtk_window_set_title (GTK_WINDOW (dialog), _("Write to Disc"));
	gtk_window_set_transient_for (GTK_WINDOW (dialog), GTK_WINDOW (task->priv->browser));

	builder = _gtk_builder_new_from_file ("burn-disc-options.ui", "burn_disc");
	options = _gtk_builder_get_widget (builder, "options");
	gtk_entry_set_text (GTK_ENTRY (_gtk_builder_get_widget (builder, "label_entry")),
			    g_file_info_get_display_name (gth_browser_get_location_data (task->priv->browser)->info));
	g_signal_connect (_gtk_builder_get_widget (builder, "label_entry"),
			  "changed",
			  G_CALLBACK (label_entry_changed_cb),
			  task->priv->session);
	gtk_widget_show (options);
	brasero_burn_options_add_options (BRASERO_BURN_OPTIONS (dialog), options);

	gth_task_dialog (GTH_TASK (task), TRUE, dialog);
	result = gtk_dialog_run (GTK_DIALOG (dialog));
	gtk_widget_destroy (dialog);

	if (result == GTK_RESPONSE_OK) {
		dialog = brasero_burn_dialog_new ();
		gtk_window_set_icon_name (GTK_WINDOW (dialog), gtk_window_get_icon_name (GTK_WINDOW (task->priv->browser)));
		gtk_window_set_title (GTK_WINDOW (dialog), _("Write to Disc"));
		brasero_session_cfg_disable (task->priv->session);
		gtk_window_set_transient_for (GTK_WINDOW (dialog), GTK_WINDOW (task->priv->browser));
		gtk_window_present (GTK_WINDOW (dialog));
		brasero_burn_dialog_run (BRASERO_BURN_DIALOG (dialog),
					 BRASERO_BURN_SESSION (task->priv->session));

		gtk_widget_destroy (dialog);
	}

	gdk_window_set_cursor (gtk_widget_get_window (GTK_WIDGET (task->priv->browser)), NULL);

	g_object_unref (task->priv->session);
	gth_task_completed (GTH_TASK (task), NULL);
}


static void
done_func (GObject  *object,
	   GError   *error,
	   gpointer  user_data)
{
	GthBurnTask *task = user_data;

	if (error != NULL) {
		gth_task_completed (GTH_TASK (task), error);
		return;
	}

	burn_content_to_disc (task);
}


static void
for_each_file_func (GFile     *file,
		    GFileInfo *info,
		    gpointer   user_data)
{
	GthBurnTask *task = user_data;
	GthFileData *file_data;

	if (g_file_info_get_file_type (info) != G_FILE_TYPE_REGULAR)
		return;

	file_data = gth_file_data_new (file, info);
	if (gth_test_match (task->priv->test, file_data)) {
		GList *list;

		list = g_hash_table_lookup (task->priv->content, task->priv->current_directory);
		list = g_list_prepend (list, g_file_dup (file));
		g_hash_table_insert (task->priv->content, g_strdup (task->priv->current_directory), list);
	}

	g_object_unref (file_data);
}


static DirOp
start_dir_func (GFile      *directory,
		GFileInfo  *info,
		GError    **error,
		gpointer    user_data)
{
	GthBurnTask *task = user_data;
	GFile       *parent;
	char        *escaped;
	GFile       *destination;
	char        *uri;

	g_free (task->priv->current_directory);

	parent = g_file_get_parent (directory);
	escaped = _g_utf8_replace_str (g_file_info_get_display_name (info), "/", "-");
	destination = g_file_get_child_for_display_name (parent, escaped, NULL);
	uri = g_file_get_uri (destination);
	task->priv->current_directory = g_uri_unescape_string (uri, NULL);
	g_hash_table_insert (task->priv->content, g_strdup (task->priv->current_directory), NULL);

	g_free (uri);
	g_object_unref (destination);
	g_free (escaped);
	g_object_unref (parent);

	return DIR_OP_CONTINUE;
}


static void
source_dialog_response_cb (GtkDialog   *dialog,
			   int          response,
			   GthBurnTask *task)
{
	gtk_widget_hide (task->priv->dialog);
	gth_task_dialog (GTH_TASK (task), FALSE, NULL);

	if (response == GTK_RESPONSE_OK) {
		if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (_gtk_builder_get_widget (task->priv->builder, "selection_radiobutton")))) {
			g_hash_table_replace (task->priv->content, g_file_get_uri (task->priv->location), g_list_reverse (task->priv->selected_files));
			task->priv->selected_files = NULL;
			burn_content_to_disc (task);
		}
		else {
			GSettings *settings;
			gboolean   recursive;

			_g_object_list_unref (task->priv->selected_files);
			task->priv->selected_files = NULL;

			settings = g_settings_new (GTHUMB_BROWSER_SCHEMA);
			recursive = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (_gtk_builder_get_widget (task->priv->builder, "folder_recursive_radiobutton")));
			task->priv->test = gth_main_get_general_filter ();
			task->priv->file_source = gth_main_get_file_source (task->priv->location);
			gth_file_source_for_each_child (task->priv->file_source,
							task->priv->location,
							recursive,
							g_settings_get_boolean (settings, PREF_BROWSER_FAST_FILE_TYPE) ? GFILE_STANDARD_ATTRIBUTES_WITH_FAST_CONTENT_TYPE : GFILE_STANDARD_ATTRIBUTES_WITH_CONTENT_TYPE,
							start_dir_func,
							for_each_file_func,
							done_func,
							task);

			g_object_unref (settings);
		}
	}
	else
		gth_task_completed (GTH_TASK (task), NULL);
}

static void
gth_burn_task_exec (GthTask *base)
{
	GthBurnTask *task = (GthBurnTask *) base;

	task->priv->builder = _gtk_builder_new_from_file ("burn-source-selector.ui", "burn_disc");

	task->priv->dialog = g_object_new (GTK_TYPE_DIALOG,
					   "title", _("Write to Disc"),
					   "transient-for", GTK_WINDOW (task->priv->browser),
					   "modal", FALSE,
					   "use-header-bar", _gtk_settings_get_dialogs_use_header (),
					   NULL);
	gtk_container_add (GTK_CONTAINER (gtk_dialog_get_content_area (GTK_DIALOG (task->priv->dialog))),
			   _gtk_builder_get_widget (task->priv->builder, "source_selector"));
	gtk_dialog_add_buttons (GTK_DIALOG (task->priv->dialog),
				_GTK_LABEL_CANCEL, GTK_RESPONSE_CANCEL,
				_("_Continue"), GTK_RESPONSE_OK,
				NULL);
	_gtk_dialog_add_class_to_response (GTK_DIALOG (task->priv->dialog), GTK_RESPONSE_OK, GTK_STYLE_CLASS_SUGGESTED_ACTION);

	if (task->priv->selected_files == NULL)
		gtk_widget_set_sensitive (_gtk_builder_get_widget (task->priv->builder, "selection_radiobutton"), FALSE);
	else if ((task->priv->selected_files != NULL) && (task->priv->selected_files->next != NULL))
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (_gtk_builder_get_widget (task->priv->builder, "selection_radiobutton")), TRUE);

	gth_task_dialog (GTH_TASK (task), TRUE, task->priv->dialog);

	g_signal_connect (task->priv->dialog,
			  "response",
			  G_CALLBACK (source_dialog_response_cb),
			  task);
	gtk_widget_show_all (task->priv->dialog);
}


static void
gth_burn_task_cancelled (GthTask *task)
{
	/* FIXME */
}


static void
gth_burn_task_class_init (GthBurnTaskClass *class)
{
	GObjectClass *object_class;
	GthTaskClass *task_class;

	object_class = (GObjectClass*) class;
	object_class->finalize = gth_burn_task_finalize;

	task_class = (GthTaskClass*) class;
	task_class->exec = gth_burn_task_exec;
	task_class->cancelled = gth_burn_task_cancelled;
}


static void
gth_burn_task_init (GthBurnTask *task)
{
	task->priv = gth_burn_task_get_instance_private (task);
	task->priv->browser = NULL;
	task->priv->location = NULL;
	task->priv->selected_files = NULL;
	task->priv->dialog = NULL;
	task->priv->builder = NULL;
	task->priv->test = NULL;
	task->priv->file_source = NULL;
	task->priv->base_directory = NULL;
	task->priv->current_directory = NULL;
	task->priv->content = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);
	task->priv->parents = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, (GDestroyNotify) gtk_tree_path_free);
	task->priv->session = NULL;
	task->priv->track = NULL;
}


GthTask *
gth_burn_task_new (GthBrowser *browser,
                   GList      *selected_files)
{
	GthBurnTask *task;

	task = (GthBurnTask *) g_object_new (GTH_TYPE_BURN_TASK, NULL);

	task->priv->browser = g_object_ref (browser);
	task->priv->location = g_file_dup (gth_browser_get_location (browser));
	task->priv->base_directory = g_file_get_uri (task->priv->location);
	task->priv->selected_files = _g_object_list_ref (selected_files);

	return (GthTask*) task;
}
