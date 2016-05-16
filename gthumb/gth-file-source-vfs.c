/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2008-2009 Free Software Foundation, Inc.
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
#include <string.h>
#include <glib/gi18n.h>
#include <glib.h>
#include "gth-file-data.h"
#include "gio-utils.h"
#include "glib-utils.h"
#include "gth-file-source-vfs.h"
#include "gth-main.h"
#include "gth-preferences.h"
#include "gtk-utils.h"

#define GTH_MONITOR_N_EVENTS 3
#define MONITOR_UPDATE_DELAY 500
#undef  DEBUG_MONITOR

struct _GthFileSourceVfsPrivate
{
	GList                *files;
	ListReady             list_ready_func;
	StartDirCallback      start_dir_func;
	ForEachChildCallback  for_each_file_func;
	ReadyCallback         ready_func;
	gpointer              user_data;
	GHashTable           *hidden_files;
	GHashTable           *monitors;
	GList                *monitor_queue[GTH_MONITOR_N_EVENTS];
	guint                 monitor_update_id;
	GVolumeMonitor       *mount_monitor;
	gboolean              check_hidden_files;
};


static guint mount_changed_event_id = 0;
static guint mount_added_event_id = 0;
static guint mount_removed_event_id = 0;


G_DEFINE_TYPE (GthFileSourceVfs, gth_file_source_vfs, GTH_TYPE_FILE_SOURCE)


static GList *
gth_file_source_vfs_add_special_dir (GList         *list, 
				     GthFileSource *file_source,
				     GUserDirectory special_dir)
{
	const gchar *path;

	path = g_get_user_special_dir (special_dir);
	if (path != NULL) {
		GFile *file;

		file = g_file_new_for_path (path);
		if ((gth_file_data_list_find_file (list, file) == NULL) && g_file_query_exists (file, NULL)) {
			GFileInfo *info;
			info = gth_file_source_get_file_info (file_source, file, GFILE_BASIC_ATTRIBUTES ",access::*");
			list = g_list_append (list, gth_file_data_new (file, info));
			g_object_unref (info);
		}

		g_object_unref (file);
	}

	return list;
}


static GList *
gth_file_source_vfs_add_uri (GList         *list,
			     GthFileSource *file_source,
			     const char    *uri,
			     char          *display_name)
{
	GFile *file;

	file = g_file_new_for_uri (uri);
	if (gth_file_data_list_find_file (list, file) == NULL) {
		GFileInfo *info;

		info = gth_file_source_get_file_info (file_source, file, GFILE_BASIC_ATTRIBUTES ",access::*");
		g_file_info_set_display_name (info, display_name);
		list = g_list_append (list, gth_file_data_new (file, info));
		g_object_unref (info);
	}
	g_object_unref (file);

	return list;
}


static GList *
gth_file_source_vfs_get_entry_points (GthFileSource *file_source)
{
	GList       *list;
	GList       *mounts;
	GList       *scan;

	list = NULL;

	list = gth_file_source_vfs_add_uri (list, file_source, get_home_uri (), _("Home Folder"));
	list = gth_file_source_vfs_add_special_dir (list, file_source, G_USER_DIRECTORY_PICTURES);
	list = gth_file_source_vfs_add_special_dir (list, file_source, G_USER_DIRECTORY_VIDEOS);
	list = gth_file_source_vfs_add_special_dir (list, file_source, G_USER_DIRECTORY_DESKTOP);
	list = gth_file_source_vfs_add_special_dir (list, file_source, G_USER_DIRECTORY_DOCUMENTS);
        list = gth_file_source_vfs_add_uri (list, file_source, "file:///", _("File System"));

	mounts = g_volume_monitor_get_mounts (g_volume_monitor_get ());
	for (scan = mounts; scan; scan = scan->next) {
		GMount    *mount = scan->data;
		GIcon     *icon;
		char      *name;
		GDrive    *drive;
		GFile     *file;
		GFileInfo *info;

		if (g_mount_is_shadowed (mount))
			continue;

		file = g_mount_get_root (mount);

		if (gth_file_data_list_find_file (list, file) != NULL) {
			g_object_unref (file);
			continue;
		}

		info = g_file_query_info (file, GFILE_BASIC_ATTRIBUTES ",access::*", G_FILE_QUERY_INFO_NONE, NULL, NULL);

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

		g_file_info_set_icon (info, icon);
		g_file_info_set_display_name (info, name);

		list = g_list_append (list, gth_file_data_new (file, info));
		g_object_unref (info);
		g_object_unref (file);
	}

	_g_object_list_unref (mounts);

	return list;
}


static GFile *
gth_file_source_vfs_to_gio_file (GthFileSource *file_source,
				 GFile         *file)
{
	char  *uri;
	GFile *gio_file;

	uri = g_file_get_uri (file);
	gio_file = g_file_new_for_uri (g_str_has_prefix (uri, "vfs+") ? uri + 4 : uri);
	g_free (uri);

	return gio_file;
}


static GFileInfo *
gth_file_source_vfs_get_file_info (GthFileSource *file_source,
				   GFile         *file,
				   const char    *attributes)
{
	GFile     *gio_file;
	GFileInfo *file_info;

	gio_file = gth_file_source_to_gio_file (file_source, file);
	file_info = g_file_query_info (gio_file,
				       attributes,
				       G_FILE_QUERY_INFO_NONE,
				       NULL,
				       NULL);

	g_object_unref (gio_file);

	return file_info;
}


/* -- gth_file_source_vfs_for_each_child -- */


static void
fec__done_func (GError   *error,
		gpointer  user_data)
{
	GthFileSourceVfs *file_source_vfs = user_data;

	performance (DEBUG_INFO, "gth_file_source_vfs_for_each_child end");

	gth_file_source_set_active (GTH_FILE_SOURCE (file_source_vfs), FALSE);
	file_source_vfs->priv->ready_func (G_OBJECT (file_source_vfs),
					   error,
					   file_source_vfs->priv->user_data);
}


static void
fec__for_each_file_func (GFile       *file,
		         GFileInfo   *info,
		         gpointer     user_data)
{
	GthFileSourceVfs *file_source_vfs = user_data;

	if (g_hash_table_lookup (file_source_vfs->priv->hidden_files, g_file_info_get_name (info)) != NULL)
		g_file_info_set_is_hidden (info, TRUE);
	file_source_vfs->priv->for_each_file_func (file, info, file_source_vfs->priv->user_data);
}


static DirOp
fec__start_dir_func (GFile       *directory,
		     GFileInfo   *info,
		     GError     **error,
		     gpointer     user_data)
{
	GthFileSourceVfs *file_source_vfs = user_data;
	DirOp             op;

	op = file_source_vfs->priv->start_dir_func (directory, info, error, file_source_vfs->priv->user_data);

	if ((op == DIR_OP_CONTINUE) && file_source_vfs->priv->check_hidden_files) {
		GFile            *dot_hidden_file;
		GFileInputStream *input_stream;

		g_hash_table_remove_all (file_source_vfs->priv->hidden_files);

		dot_hidden_file = g_file_get_child (directory, ".hidden");
		input_stream = g_file_read (dot_hidden_file, NULL, NULL);
		if (input_stream != NULL) {
			GDataInputStream *data_stream;
			char             *line;

			data_stream = g_data_input_stream_new (G_INPUT_STREAM (input_stream));
			while ((line = g_data_input_stream_read_line (data_stream, NULL, NULL, NULL)) != NULL)
				g_hash_table_insert (file_source_vfs->priv->hidden_files,
						     line,
						     GINT_TO_POINTER (1));

			g_object_unref (input_stream);
		}

		g_object_unref (dot_hidden_file);
	}

	return op;
}


static void
gth_file_source_vfs_for_each_child (GthFileSource        *file_source,
				    GFile                *parent,
				    gboolean              recursive,
				    const char           *attributes,
				    StartDirCallback      start_dir_func,
				    ForEachChildCallback  for_each_file_func,
				    ReadyCallback         ready_func,
				    gpointer              user_data)
{
	GthFileSourceVfs *file_source_vfs = (GthFileSourceVfs *) file_source;
	GFile            *gio_folder;

	gth_file_source_set_active (file_source, TRUE);
	g_cancellable_reset (gth_file_source_get_cancellable (file_source));
	g_hash_table_remove_all (file_source_vfs->priv->hidden_files);

	performance (DEBUG_INFO, "gth_file_source_vfs_for_each_child start");

	file_source_vfs->priv->start_dir_func = start_dir_func;
	file_source_vfs->priv->for_each_file_func = for_each_file_func;
	file_source_vfs->priv->ready_func = ready_func;
	file_source_vfs->priv->user_data = user_data;
	file_source_vfs->priv->check_hidden_files = _g_file_attributes_matches_any (attributes, G_FILE_ATTRIBUTE_STANDARD_IS_HIDDEN);

	gio_folder = gth_file_source_to_gio_file (file_source, parent);
	g_directory_foreach_child (gio_folder,
				   recursive,
				   TRUE,
				   attributes,
				   gth_file_source_get_cancellable (file_source),
				   fec__start_dir_func,
				   fec__for_each_file_func,
				   fec__done_func,
				   file_source);

	g_object_unref (gio_folder);
}


/* -- gth_file_source_vfs_copy -- */


typedef struct {
	GthFileSource *file_source;
	ReadyCallback  ready_callback;
	gpointer       user_data;
} CopyOpData;


static void
copy_done_cb (GError   *error,
	      gpointer  user_data)
{
	CopyOpData *cod = user_data;

	cod->ready_callback (G_OBJECT (cod->file_source), error, cod->user_data);

	g_object_unref (cod->file_source);
	g_free (cod);
}


static void
gth_file_source_vfs_copy (GthFileSource    *file_source,
			  GthFileData      *destination,
			  GList            *file_list, /* GFile * list */
			  gboolean          move,
			  int               destination_position,
			  ProgressCallback  progress_callback,
			  DialogCallback    dialog_callback,
		          ReadyCallback     ready_callback,
			  gpointer          data)
{
	CopyOpData *cod;

	cod = g_new0 (CopyOpData, 1);
	cod->file_source = g_object_ref (file_source);
	cod->ready_callback = ready_callback;
	cod->user_data = data;

	_g_copy_files_async (file_list,
			     destination->file,
			     move,
			     G_FILE_COPY_ALL_METADATA,
			     GTH_OVERWRITE_RESPONSE_UNSPECIFIED,
			     G_PRIORITY_DEFAULT,
			     gth_file_source_get_cancellable (file_source),
			     progress_callback,
			     data,
			     dialog_callback,
			     data,
			     copy_done_cb,
			     cod);
}


static gboolean
gth_file_source_vfs_can_cut (GthFileSource *file_source,
			     GFile         *file)
{
	return TRUE;
}


static void
mount_monitor_mountpoints_changed_cb (GVolumeMonitor *volume_monitor,
				      GMount         *mount,
				      gpointer        user_data)
{
	call_when_idle ((DataFunc) gth_monitor_entry_points_changed, gth_main_get_default_monitor ());
}


static void
gth_file_source_vfs_monitor_entry_points (GthFileSource *file_source)
{
	GthFileSourceVfs *file_source_vfs = (GthFileSourceVfs *) file_source;

	if (file_source_vfs->priv->mount_monitor == NULL)
		file_source_vfs->priv->mount_monitor = g_volume_monitor_get ();

	if (mount_changed_event_id == 0)
		mount_changed_event_id = g_signal_connect (file_source_vfs->priv->mount_monitor,
							   "mount-changed",
							   G_CALLBACK (mount_monitor_mountpoints_changed_cb),
							   file_source_vfs);
	if (mount_added_event_id == 0)
		mount_added_event_id = g_signal_connect (file_source_vfs->priv->mount_monitor,
							 "mount-added",
							 G_CALLBACK (mount_monitor_mountpoints_changed_cb),
							 file_source_vfs);
	if (mount_removed_event_id == 0)
		mount_removed_event_id = g_signal_connect (file_source_vfs->priv->mount_monitor,
							   "mount-removed",
							   G_CALLBACK (mount_monitor_mountpoints_changed_cb),
							   file_source_vfs);
}


static gboolean
process_event_queue (gpointer data)
{
	GthFileSourceVfs *file_source_vfs = data;
	GthMonitor       *monitor;
	GList            *monitor_queue[GTH_MONITOR_N_EVENTS];
	int               event_type;

	if (file_source_vfs->priv->monitor_update_id != 0)
		g_source_remove (file_source_vfs->priv->monitor_update_id);
	file_source_vfs->priv->monitor_update_id = 0;

	for (event_type = 0; event_type < GTH_MONITOR_N_EVENTS; event_type++) {
		monitor_queue[event_type] = file_source_vfs->priv->monitor_queue[event_type];
		file_source_vfs->priv->monitor_queue[event_type] = NULL;
	}

	monitor = gth_main_get_default_monitor ();
	for (event_type = 0; event_type < GTH_MONITOR_N_EVENTS; event_type++) {
		GList *scan;

		for (scan = monitor_queue[event_type]; scan; scan = scan->next) {
			GFile *file = scan->data;
			GFile *parent;
			GList *list;

#ifdef DEBUG_MONITOR
			switch (event_type) {
			case GTH_MONITOR_EVENT_CREATED:
				g_print ("GTH_MONITOR_EVENT_CREATED");
				break;
			case GTH_MONITOR_EVENT_DELETED:
				g_print ("GTH_MONITOR_EVENT_DELETED");
				break;
			case GTH_MONITOR_EVENT_CHANGED:
				g_print ("GTH_MONITOR_EVENT_CHANGED");
				break;
			}
			g_print (" ==> %s\n", g_file_get_uri (file));
#endif

			parent = g_file_get_parent (file);
			list = g_list_prepend (NULL, g_object_ref (file));
			gth_monitor_folder_changed (monitor,
						    parent,
						    list,
						    event_type);

			_g_object_list_unref (list);
			g_object_unref (parent);
		}
		_g_object_list_unref (monitor_queue[event_type]);
		monitor_queue[event_type] = NULL;
	}

	return FALSE;
}


static gboolean
remove_if_present (GthFileSourceVfs *file_source_vfs,
		   GthMonitorEvent   event_type,
		   GFile            *file)
{
	GList *link;

	link = _g_file_list_find_file (file_source_vfs->priv->monitor_queue[event_type], file);
	if (link != NULL) {
		file_source_vfs->priv->monitor_queue[event_type] = g_list_remove_link (file_source_vfs->priv->monitor_queue[event_type], link);
		_g_object_list_unref (link);
		return TRUE;
	}

	return FALSE;
}


static void
monitor_changed_cb (GFileMonitor      *file_monitor,
		    GFile             *file,
		    GFile             *other_file,
		    GFileMonitorEvent  file_event_type,
		    gpointer           user_data)
{
	GthFileSourceVfs *file_source_vfs = user_data;
	GthMonitorEvent   event_type;

	switch (file_event_type) {
	case G_FILE_MONITOR_EVENT_CREATED:
	default:
		event_type = GTH_MONITOR_EVENT_CREATED;
		break;

	case G_FILE_MONITOR_EVENT_DELETED:
		event_type = GTH_MONITOR_EVENT_DELETED;
		break;

	case G_FILE_MONITOR_EVENT_CHANGED:
	case G_FILE_MONITOR_EVENT_ATTRIBUTE_CHANGED:
		event_type = GTH_MONITOR_EVENT_CHANGED;
		break;
	}

#ifdef DEBUG_MONITOR
	g_print ("[RAW] ");
	switch (event_type) {
	case GTH_MONITOR_EVENT_CREATED:
		g_print ("GTH_MONITOR_EVENT_CREATED");
		break;
	case GTH_MONITOR_EVENT_DELETED:
		g_print ("GTH_MONITOR_EVENT_DELETED");
		break;
	case GTH_MONITOR_EVENT_CHANGED:
		g_print ("GTH_MONITOR_EVENT_CHANGED");
		break;
	}
	g_print (" ==> %s\n", g_file_get_uri (file));
#endif

	if (event_type == GTH_MONITOR_EVENT_CREATED) {
		if (remove_if_present (file_source_vfs, GTH_MONITOR_EVENT_DELETED, file))
			event_type = GTH_MONITOR_EVENT_CHANGED;
	}
	else if (event_type == GTH_MONITOR_EVENT_DELETED) {
		remove_if_present (file_source_vfs, GTH_MONITOR_EVENT_CREATED, file);
		remove_if_present (file_source_vfs, GTH_MONITOR_EVENT_CHANGED, file);
	}
	else if (event_type == GTH_MONITOR_EVENT_CHANGED) {
		if (_g_file_list_find_file (file_source_vfs->priv->monitor_queue[GTH_MONITOR_EVENT_CREATED], file))
			return;
		remove_if_present (file_source_vfs, GTH_MONITOR_EVENT_CHANGED, file);
	}

	file_source_vfs->priv->monitor_queue[event_type] = g_list_prepend (file_source_vfs->priv->monitor_queue[event_type], g_file_dup (file));

	if (file_source_vfs->priv->monitor_update_id != 0)
		g_source_remove (file_source_vfs->priv->monitor_update_id);
	file_source_vfs->priv->monitor_update_id = g_timeout_add (MONITOR_UPDATE_DELAY,
								  process_event_queue,
								  file_source_vfs);
}


static void
remove_monitor_for_directory (GthFileSourceVfs *file_source_vfs,
			      GFile            *file)
{
	GFile *parent;

	parent = g_object_ref (file);
	while (parent != NULL) {
		GFile *tmp;

		g_hash_table_remove (file_source_vfs->priv->monitors, parent);

		tmp = g_file_get_parent (parent);
		g_object_unref (parent);
		parent = tmp;
	}
}


static void
add_monitor_for_directory (GthFileSourceVfs *file_source_vfs,
			   GFile            *file)
{
	GFile *parent;

	parent = g_object_ref (file);
	while (parent != NULL) {
		GFile *tmp;

		if (g_hash_table_lookup (file_source_vfs->priv->monitors, parent) == NULL) {
			GFileMonitor *monitor;

			monitor = g_file_monitor_directory (parent, 0, NULL, NULL);
			if (monitor != NULL) {
				g_hash_table_insert (file_source_vfs->priv->monitors, g_object_ref (parent), monitor);
				g_signal_connect (monitor, "changed", G_CALLBACK (monitor_changed_cb), file_source_vfs);
			}
		}

		tmp = g_file_get_parent (parent);
		g_object_unref (parent);
		parent = tmp;
	}
}


static void
gth_file_source_vfs_monitor_directory (GthFileSource *file_source,
				       GFile         *file,
				       gboolean       activate)
{
	GthFileSourceVfs *file_source_vfs = (GthFileSourceVfs *) file_source;

	if (activate)
		add_monitor_for_directory (file_source_vfs, file);
	else
		remove_monitor_for_directory (file_source_vfs, file);
}


/* -- gth_file_source_vfs_remove -- */


static void
parent_table_value_free (gpointer key,
			 gpointer value,
			 gpointer user_data)
{
	_g_object_list_unref (value);
}


static void
notify_files_delete (GtkWindow *window,
		     GList     *files)
{
	GHashTable *parent_table;
	GList      *scan;
	GList      *parent_list;

	parent_table = g_hash_table_new_full (g_file_hash, (GEqualFunc) g_file_equal, g_object_unref, NULL);
	for (scan = files; scan; scan = scan->next) {
		GFile *file = scan->data;
		GFile *parent;
		GList *children;

		parent = g_file_get_parent (file);
		if (parent == NULL)
			return;

		children = g_hash_table_lookup (parent_table, parent);
		children = g_list_prepend (children, g_object_ref (file));
		g_hash_table_replace (parent_table, g_object_ref (parent), children);

		g_object_unref (parent);
	}

	parent_list = g_hash_table_get_keys (parent_table);
	for (scan = parent_list; scan; scan = scan->next) {
		GFile *parent = scan->data;
		GList *children;

		children = g_hash_table_lookup (parent_table, parent);
		if (children != NULL)
			gth_monitor_folder_changed (gth_main_get_default_monitor (),
						    parent,
						    children,
						    GTH_MONITOR_EVENT_DELETED);
	}

	g_list_free (parent_list);
	g_hash_table_foreach (parent_table, parent_table_value_free, NULL);
	g_hash_table_unref (parent_table);
}


static void
delete_file_permanently (GtkWindow *window,
			 GList     *file_list)
{
	GList  *files;
	GError *error = NULL;

	files = gth_file_data_list_to_file_list (file_list);
	if (! _g_delete_files (files, TRUE, &error)) {
		_gtk_error_dialog_from_gerror_show (window, _("Could not delete the files"), error);
		g_clear_error (&error);
	}
	else
		notify_files_delete (window, files);

	_g_object_list_unref (files);
}


static void
delete_permanently_response_cb (GtkDialog *dialog,
				int        response_id,
				gpointer   user_data)
{
	GList *file_list = user_data;

	if (response_id == GTK_RESPONSE_YES)
		delete_file_permanently (gtk_window_get_transient_for (GTK_WINDOW (dialog)), file_list);

	gtk_widget_destroy (GTK_WIDGET (dialog));
	_g_object_list_unref (file_list);
}


static void
trash_files (GtkWindow *window,
	     GList     *file_list)
{
	GList    *scan;
	gboolean  moved_to_trash = TRUE;
	GError   *error = NULL;

	for (scan = file_list; scan; scan = scan->next) {
		GthFileData *file_data = scan->data;

		if (! g_file_trash (file_data->file, NULL, &error)) {
			moved_to_trash = FALSE;
			if (g_error_matches (error, G_IO_ERROR,  G_IO_ERROR_NOT_SUPPORTED)) {
				GtkWidget *d;

				g_clear_error (&error);

				d = _gtk_yesno_dialog_new (window,
							   GTK_DIALOG_MODAL,
							   _("The files cannot be moved to the Trash. Do you want to delete them permanently?"),
							   GTK_STOCK_CANCEL,
							   GTK_STOCK_DELETE);
				g_signal_connect (d,
						  "response",
						  G_CALLBACK (delete_permanently_response_cb),
						  gth_file_data_list_dup (file_list));
				gtk_widget_show (d);

				break;
			}
			_gtk_error_dialog_from_gerror_show (window, _("Could not move the files to the Trash"), error);
			g_clear_error (&error);
			break;
		}
	}

	if (moved_to_trash) {
		GList  *files;

		files = gth_file_data_list_to_file_list (file_list);
		notify_files_delete (window, files);

		_g_object_list_unref (files);
	}
}


static void
trash_files_response_cb (GtkDialog *dialog,
			 int        response_id,
			 gpointer   user_data)
{
	GList *file_list = user_data;

	if (response_id == GTK_RESPONSE_YES)
		trash_files (gtk_window_get_transient_for (GTK_WINDOW (dialog)), file_list);

	gtk_widget_destroy (GTK_WIDGET (dialog));
	_g_object_list_unref (file_list);
}


void
gth_file_mananger_trash_files (GtkWindow *window,
			       GList     *file_list /* GthFileData list */)
{
	GSettings *settings;

	settings = g_settings_new (GTHUMB_MESSAGES_SCHEMA);
	if (g_settings_get_boolean (settings, PREF_MSG_CONFIRM_DELETION)) {
		int        file_count;
		char      *prompt;
		GtkWidget *d;

		file_count = g_list_length (file_list);
		if (file_count == 1) {
			GthFileData *file_data = file_list->data;
			prompt = g_strdup_printf (_("Are you sure you want to move \"%s\" to trash?"),
						  g_file_info_get_display_name (file_data->info));
		}
		else
			prompt = g_strdup_printf (ngettext("Are you sure you want to move to trash "
							   "the %'d selected file?",
							   "Are you sure you want to move to trash "
							   "the %'d selected files?", file_count),
						  file_count);

		d = _gtk_message_dialog_new (window,
					     GTK_DIALOG_MODAL,
					     GTK_STOCK_DIALOG_QUESTION,
					     prompt,
					     NULL,
					     GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
					     _("Mo_ve to Trash"), GTK_RESPONSE_YES,
					     NULL);
		g_signal_connect (d, "response",
				  G_CALLBACK (trash_files_response_cb),
				  gth_file_data_list_dup (file_list));
		gtk_widget_show (d);

		g_free (prompt);
	}
	else
		trash_files (window, file_list);

	g_object_unref (settings);
}


void
gth_file_mananger_delete_files (GtkWindow *window,
				GList     *file_list /* GthFileData list */)
{
	int        file_count;
	char      *prompt;
	GtkWidget *d;

	file_list = _g_object_list_ref (file_list);
	file_count = g_list_length (file_list);
	if (file_count == 1) {
		GthFileData *file_data = file_list->data;
		prompt = g_strdup_printf (_("Are you sure you want to permanently delete \"%s\"?"), g_file_info_get_display_name (file_data->info));
	}
	else
		prompt = g_strdup_printf (ngettext("Are you sure you want to permanently delete "
						   "the %'d selected file?",
						   "Are you sure you want to permanently delete "
					  	   "the %'d selected files?", file_count),
					  file_count);

	d = _gtk_message_dialog_new (window,
				     GTK_DIALOG_MODAL,
				     GTK_STOCK_DIALOG_QUESTION,
				     prompt,
				     _("If you delete a file, it will be permanently lost."),
				     GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
				     GTK_STOCK_DELETE, GTK_RESPONSE_YES,
				     NULL);
	g_signal_connect (d, "response", G_CALLBACK (delete_permanently_response_cb), file_list);
	gtk_widget_show (d);

	g_free (prompt);
}


static void
gth_file_source_vfs_remove (GthFileSource *file_source,
			    GthFileData   *location,
	       	       	    GList         *file_list /* GthFileData list */,
	       	       	    gboolean       permanently,
	       	       	    GtkWindow     *parent)
{
	if (permanently)
		gth_file_mananger_delete_files (parent, file_list);
	else
		gth_file_mananger_trash_files (parent, file_list);
}


static void
gth_file_source_vfs_finalize (GObject *object)
{
	GthFileSourceVfs *file_source_vfs = GTH_FILE_SOURCE_VFS (object);

	if (file_source_vfs->priv != NULL) {
		int i;

		if (file_source_vfs->priv->monitor_update_id != 0) {
			g_source_remove (file_source_vfs->priv->monitor_update_id);
			file_source_vfs->priv->monitor_update_id = 0;
		}

		g_hash_table_destroy (file_source_vfs->priv->hidden_files);
		g_hash_table_destroy (file_source_vfs->priv->monitors);

		for (i = 0; i < GTH_MONITOR_N_EVENTS; i++) {
			_g_object_list_unref (file_source_vfs->priv->monitor_queue[i]);
			file_source_vfs->priv->monitor_queue[i] = NULL;
		}
		_g_object_list_unref (file_source_vfs->priv->files);

		g_free (file_source_vfs->priv);
		file_source_vfs->priv = NULL;
	}

	G_OBJECT_CLASS (gth_file_source_vfs_parent_class)->finalize (object);
}


static void
gth_file_source_vfs_class_init (GthFileSourceVfsClass *class)
{
	GObjectClass       *object_class;
	GthFileSourceClass *file_source_class;

	object_class = (GObjectClass*) class;
	object_class->finalize = gth_file_source_vfs_finalize;

	file_source_class = (GthFileSourceClass*) class;
	file_source_class->get_entry_points = gth_file_source_vfs_get_entry_points;
	file_source_class->to_gio_file = gth_file_source_vfs_to_gio_file;
	file_source_class->get_file_info = gth_file_source_vfs_get_file_info;
	file_source_class->for_each_child = gth_file_source_vfs_for_each_child;
	file_source_class->copy = gth_file_source_vfs_copy;
	file_source_class->can_cut = gth_file_source_vfs_can_cut;
	file_source_class->monitor_entry_points = gth_file_source_vfs_monitor_entry_points;
	file_source_class->monitor_directory = gth_file_source_vfs_monitor_directory;
	file_source_class->remove = gth_file_source_vfs_remove;
}


static void
gth_file_source_vfs_init (GthFileSourceVfs *file_source)
{
	int i;

	file_source->priv = g_new0 (GthFileSourceVfsPrivate, 1);
	gth_file_source_add_scheme (GTH_FILE_SOURCE (file_source), "vfs+");
	file_source->priv->hidden_files = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);
	file_source->priv->monitors = g_hash_table_new_full (g_file_hash, (GEqualFunc) g_file_equal, g_object_unref, g_object_unref);
	for (i = 0; i < GTH_MONITOR_N_EVENTS; i++)
		file_source->priv->monitor_queue[i] = NULL;
}
