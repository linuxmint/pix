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
#include "gth-browser.h"
#include "gth-delete-task.h"
#include "gth-file-source-vfs.h"
#include "gth-main.h"
#include "gth-preferences.h"
#include "gth-progress-dialog.h"
#include "gth-trash-task.h"
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
static guint volume_changed_event_id = 0;
static guint volume_added_event_id = 0;
static guint volume_removed_event_id = 0;


G_DEFINE_TYPE_WITH_CODE (GthFileSourceVfs,
			 gth_file_source_vfs,
			 GTH_TYPE_FILE_SOURCE,
			 G_ADD_PRIVATE (GthFileSourceVfs))


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
			     const char    *uri)
{
	GFile *file;

	file = g_file_new_for_uri (uri);
	if (gth_file_data_list_find_file (list, file) == NULL) {
		GFileInfo *info;

		info = gth_file_source_get_file_info (file_source, file, GFILE_BASIC_ATTRIBUTES ",access::*");
		list = g_list_append (list, gth_file_data_new (file, info));

		g_object_unref (info);
	}
	g_object_unref (file);

	return list;
}


static GList *
gth_file_source_vfs_get_entry_points (GthFileSource *file_source)
{
	GList          *list;
	GVolumeMonitor *monitor;
	GList          *mounts;
	GList          *volumes;
	GList          *scan;

	list = NULL;

	list = gth_file_source_vfs_add_uri (list, file_source, _g_uri_get_home ());
	list = gth_file_source_vfs_add_special_dir (list, file_source, G_USER_DIRECTORY_PICTURES);
	list = gth_file_source_vfs_add_special_dir (list, file_source, G_USER_DIRECTORY_VIDEOS);
	list = gth_file_source_vfs_add_special_dir (list, file_source, G_USER_DIRECTORY_DOWNLOAD);
	list = gth_file_source_vfs_add_uri (list, file_source, "file:///");

	monitor = g_volume_monitor_get ();
	mounts = g_volume_monitor_get_mounts (monitor);
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

		icon = g_mount_get_symbolic_icon (mount);
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

		info = g_file_info_new ();
		g_file_info_set_file_type (info, G_FILE_TYPE_DIRECTORY);
		g_file_info_set_attribute_boolean (info, G_FILE_ATTRIBUTE_ACCESS_CAN_READ, TRUE);
		g_file_info_set_attribute_boolean (info, G_FILE_ATTRIBUTE_ACCESS_CAN_WRITE, FALSE);
		g_file_info_set_attribute_boolean (info, G_FILE_ATTRIBUTE_ACCESS_CAN_DELETE, FALSE);
		g_file_info_set_attribute_boolean (info, G_FILE_ATTRIBUTE_ACCESS_CAN_TRASH, FALSE);
		g_file_info_set_symbolic_icon (info, icon);
		g_file_info_set_display_name (info, name);
		g_file_info_set_name (info, name);

		list = g_list_append (list, gth_file_data_new (file, info));

		g_object_unref (info);
		g_object_unref (file);
		g_free (name);
		_g_object_unref (icon);
	}
	_g_object_list_unref (mounts);

	/* Not mounted mountable volumes. */

	volumes = g_volume_monitor_get_volumes (monitor);
	for (scan = volumes; scan; scan = scan->next) {
		GVolume   *volume = scan->data;
		GMount    *mount;
		GFile     *file;
		GIcon     *icon;
		char      *name;
		GFileInfo *info;

		if (! g_volume_can_mount (volume))
			continue;

		mount = g_volume_get_mount (volume);
		if (mount != NULL) {
			/* Already mounted, ignore. */
			g_object_unref (mount);
			continue;
		}

		file = g_volume_get_activation_root (volume);
		if (file == NULL) {
			char *device;

			device = g_volume_get_identifier (volume, G_VOLUME_IDENTIFIER_KIND_UNIX_DEVICE);
			if (device == NULL)
				continue;
			file = g_file_new_for_path (device);

			g_free (device);
		}

		if (gth_file_data_list_find_file (list, file) != NULL) {
			/* Already mounted, ignore. */
			g_object_unref (file);
			continue;
		}

		icon = g_volume_get_symbolic_icon (volume);
		name = g_volume_get_name (volume);

		info = g_file_info_new ();
		g_file_info_set_file_type (info, G_FILE_TYPE_MOUNTABLE);
		g_file_info_set_attribute_object (info, GTH_FILE_ATTRIBUTE_VOLUME, G_OBJECT (volume));
		g_file_info_set_attribute_boolean (info, G_FILE_ATTRIBUTE_ACCESS_CAN_READ, TRUE);
		g_file_info_set_attribute_boolean (info, G_FILE_ATTRIBUTE_ACCESS_CAN_WRITE, FALSE);
		g_file_info_set_attribute_boolean (info, G_FILE_ATTRIBUTE_ACCESS_CAN_DELETE, FALSE);
		g_file_info_set_attribute_boolean (info, G_FILE_ATTRIBUTE_ACCESS_CAN_TRASH, FALSE);
		g_file_info_set_symbolic_icon (info, icon);
		g_file_info_set_display_name (info, name);
		g_file_info_set_name (info, name);

		list = g_list_append (list, gth_file_data_new (file, info));

		g_object_unref (info);
		g_object_unref (file);
		g_free (name);
		_g_object_unref (icon);
	}
	_g_object_list_unref (volumes);

	g_object_unref (monitor);

	return list;
}


static GFile *
gth_file_source_vfs_to_gio_file (GthFileSource *file_source,
				 GFile         *file)
{
	return g_file_dup (file);
}


static GFileInfo *
gth_file_source_vfs_get_file_info (GthFileSource *file_source,
				   GFile         *file,
				   const char    *attributes)
{
	GFileInfo *file_info;
	char      *uri;

	file_info = g_file_query_info (file,
				       attributes,
				       G_FILE_QUERY_INFO_NONE,
				       NULL,
				       NULL);

	uri = g_file_get_uri (file);
	if (g_strcmp0 (uri, "file:///") == 0) {
		GIcon *icon;

		g_file_info_set_display_name (file_info, _("Computer"));
		icon = g_themed_icon_new ("drive-harddisk-symbolic");
		g_file_info_set_symbolic_icon (file_info, icon);

		g_object_unref (icon);
	}
	else if (g_strcmp0 (uri, _g_uri_get_home ()) == 0)
		g_file_info_set_display_name (file_info, _("Home Folder"));

	g_free (uri);

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

	gth_file_source_set_active (file_source, TRUE);
	g_cancellable_reset (gth_file_source_get_cancellable (file_source));
	g_hash_table_remove_all (file_source_vfs->priv->hidden_files);

	performance (DEBUG_INFO, "gth_file_source_vfs_for_each_child start");

	file_source_vfs->priv->start_dir_func = start_dir_func;
	file_source_vfs->priv->for_each_file_func = for_each_file_func;
	file_source_vfs->priv->ready_func = ready_func;
	file_source_vfs->priv->user_data = user_data;
	file_source_vfs->priv->check_hidden_files = _g_file_attributes_matches_any (attributes, G_FILE_ATTRIBUTE_STANDARD_IS_HIDDEN);

	_g_directory_foreach_child (parent,
				   recursive,
				   TRUE,
				   attributes,
				   gth_file_source_get_cancellable (file_source),
				   fec__start_dir_func,
				   fec__for_each_file_func,
				   fec__done_func,
				   file_source);
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

	_g_file_list_copy_async (file_list,
				 destination->file,
				 move,
				 GTH_FILE_COPY_ALL_METADATA | GTH_FILE_COPY_RENAME_SAME_FILE,
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
gth_file_source_vfs_can_cut (GthFileSource *file_source)
{
	return TRUE;
}


static void
mount_monitor_mountpoints_changed_cb (GVolumeMonitor *volume_monitor,
				      gpointer        mount_or_volume,
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
	if (volume_changed_event_id == 0)
		volume_changed_event_id = g_signal_connect (file_source_vfs->priv->mount_monitor,
							    "volume-changed",
							    G_CALLBACK (mount_monitor_mountpoints_changed_cb),
							    file_source_vfs);
	if (volume_added_event_id == 0)
		volume_added_event_id = g_signal_connect (file_source_vfs->priv->mount_monitor,
							  "volume-added",
							  G_CALLBACK (mount_monitor_mountpoints_changed_cb),
							  file_source_vfs);
	if (volume_removed_event_id == 0)
		volume_removed_event_id = g_signal_connect (file_source_vfs->priv->mount_monitor,
							    "volume-removed",
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
		GFile *list_parent = NULL;
		GList *list = NULL;
		GList *scan;

		for (scan = monitor_queue[event_type]; scan; scan = scan->next) {
			GFile *file = scan->data;
			GFile *parent;

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

			/* Compress the events: emits a single event if the
			 * parent is the same. */

			parent = g_file_get_parent (file);
			if (_g_file_equal (parent, list_parent)) {
				list = g_list_prepend (list, g_object_ref (file));
			}
			else {
				if (list != NULL) {
					gth_monitor_folder_changed (monitor,
								    list_parent,
								    list,
								    event_type);

					_g_object_list_unref (list);
					_g_object_unref (list_parent);
				}
				list = g_list_prepend (NULL, g_object_ref (file));
				list_parent = g_object_ref (parent);
			}
			g_object_unref (parent);
		}

		if (list != NULL) {
			gth_monitor_folder_changed (monitor,
						    list_parent,
						    list,
						    event_type);
			_g_object_list_unref (list);
			_g_object_unref (list_parent);
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


typedef struct {
	GtkWindow *window;
	GList     *files;
} DeleteData;


static void
delete_data_free (DeleteData *ddata)
{
	_g_object_list_unref (ddata->files);
	g_free (ddata);
}


static void
delete_task_completed_cb (GthTask  *task,
		          GError   *error,
			  gpointer  user_data)
{
	DeleteData *ddata = user_data;

	if (error == NULL)
		gth_monitor_files_deleted (gth_main_get_default_monitor (), ddata->files);
	else if (! g_error_matches (error, G_IO_ERROR,  G_IO_ERROR_CANCELLED))
		_gtk_error_dialog_from_gerror_show (ddata->window, _("Could not delete the files"), error);

	delete_data_free (ddata);
	g_object_unref (task);
}


static void
delete_file_permanently (GtkWindow *window,
			 GList     *file_list /* GthFileData list */)
{
	DeleteData *ddata;
	GthTask    *task;

	ddata = g_new0 (DeleteData, 1);
	ddata->window = window;
	ddata->files = gth_file_data_list_to_file_list (file_list);
	task = gth_delete_task_new (ddata->files);
	g_signal_connect (task,
			  "completed",
			  G_CALLBACK (delete_task_completed_cb),
			  ddata);

	if (GTH_IS_BROWSER (window)) {
		gth_browser_exec_task (GTH_BROWSER (window), task, GTH_TASK_FLAGS_DEFAULT);
	}
	else {
		GtkWidget *dialog;

		dialog = gth_progress_dialog_new (window);
		gth_progress_dialog_destroy_with_tasks (GTH_PROGRESS_DIALOG (dialog), TRUE);
		gth_progress_dialog_add_task (GTH_PROGRESS_DIALOG (dialog), task, GTH_TASK_FLAGS_DEFAULT);
	}
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


/* -- gth_file_mananger_trash_files -- */


typedef struct {
	GtkWindow *window;
	GList     *file_data_list;
	GList     *file_list;
} TrashData;


static void
trash_data_free (TrashData *tdata)
{
	_g_object_list_unref (tdata->file_data_list);
	_g_object_list_unref (tdata->file_list);
	g_free (tdata);
}


static void
trash_failed_delete_permanently_response_cb (GtkDialog *dialog,
					     int        response_id,
					     gpointer   user_data)
{
	TrashData *tdata = user_data;

	if (response_id == GTK_RESPONSE_YES)
		delete_file_permanently (tdata->window, tdata->file_data_list);

	gtk_widget_destroy (GTK_WIDGET (dialog));
	trash_data_free (tdata);
}


static void
trash_task_completed_cb (GthTask  *task,
		         GError   *error,
			 gpointer  user_data)
{
	TrashData *tdata = user_data;

	if (error == NULL) {
		gth_monitor_files_deleted (gth_main_get_default_monitor (), tdata->file_list);
		trash_data_free (tdata);
	}
	else if (g_error_matches (error, G_IO_ERROR,  G_IO_ERROR_CANCELLED)) {
		trash_data_free (tdata);
	}
	else if (g_error_matches (error, G_IO_ERROR,  G_IO_ERROR_NOT_SUPPORTED)) {
		GtkWidget *d;

		d = _gtk_yesno_dialog_new (tdata->window,
					   GTK_DIALOG_MODAL,
					   _("The files cannot be moved to the Trash. Do you want to delete them permanently?"),
					   _GTK_LABEL_CANCEL,
					   _GTK_LABEL_DELETE);
		_gtk_dialog_add_class_to_response (GTK_DIALOG (d), GTK_RESPONSE_YES, GTK_STYLE_CLASS_DESTRUCTIVE_ACTION);
		g_signal_connect (d,
				  "response",
				  G_CALLBACK (trash_failed_delete_permanently_response_cb),
				  tdata);
		gtk_widget_show (d);
	}
	else {
		_gtk_error_dialog_from_gerror_show (tdata->window, _("Could not move the files to the Trash"), error);
		trash_data_free (tdata);
	}

	g_object_unref (task);
}


void
gth_file_mananger_trash_files (GtkWindow *window,
			       GList     *file_list /* GthFileData list */)
{
	TrashData *tdata;
	GthTask   *task;

	tdata = g_new0 (TrashData, 1);
	tdata->window = window;
	tdata->file_data_list = gth_file_data_list_dup (file_list);
	tdata->file_list = gth_file_data_list_to_file_list (file_list);
	task = gth_trash_task_new (tdata->file_list);
	g_signal_connect (task,
			  "completed",
			  G_CALLBACK (trash_task_completed_cb),
			  tdata);

	gth_browser_exec_task (GTH_BROWSER (window), task, GTH_TASK_FLAGS_IGNORE_ERROR);
}


void
gth_file_mananger_delete_files (GtkWindow *window,
				GList     *file_list /* GthFileData list */)
{
	GSettings *settings;

	settings = g_settings_new (GTHUMB_MESSAGES_SCHEMA);
	if (g_settings_get_boolean (settings, PREF_MSG_CONFIRM_DELETION)) {
		int        file_count;
		char      *prompt;
		GtkWidget *d;

		file_list = _g_object_list_ref (file_list);
		file_count = g_list_length (file_list);
		if (file_count == 1) {
			GthFileData *file_data = file_list->data;
			prompt = g_strdup_printf (_("Are you sure you want to permanently delete “%s”?"), g_file_info_get_display_name (file_data->info));
		}
		else
			prompt = g_strdup_printf (ngettext("Are you sure you want to permanently delete "
							   "the %'d selected file?",
							   "Are you sure you want to permanently delete "
							   "the %'d selected files?", file_count),
						  file_count);

		d = _gtk_message_dialog_new (window,
					     GTK_DIALOG_MODAL,
					     _GTK_ICON_NAME_DIALOG_QUESTION,
					     prompt,
					     _("If you delete a file, it will be permanently lost."),
					     _GTK_LABEL_CANCEL, GTK_RESPONSE_CANCEL,
					     _GTK_LABEL_DELETE, GTK_RESPONSE_YES,
					     NULL);
		_gtk_dialog_add_class_to_response (GTK_DIALOG (d), GTK_RESPONSE_YES, GTK_STYLE_CLASS_DESTRUCTIVE_ACTION);
		g_signal_connect (d, "response", G_CALLBACK (delete_permanently_response_cb), file_list);
		gtk_widget_show (d);

		g_free (prompt);
	}
	else
		delete_file_permanently (window, file_list);

	g_object_unref (settings);
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


static GdkDragAction
gth_file_source_vfs_get_drop_actions (GthFileSource *file_source,
				      GFile         *destination,
				      GFile         *file)
{
	GdkDragAction  actions = 0;
	char          *dest_uri;
	char          *file_uri;
	char          *dest_scheme;
	char          *file_scheme;

	dest_uri = g_file_get_uri (destination);
	dest_scheme = gth_main_get_source_scheme (dest_uri);
	file_uri = g_file_get_uri (file);
	file_scheme = gth_main_get_source_scheme (file_uri);

	if (_g_str_equal (dest_scheme, "file")
		&& _g_str_equal (file_scheme, "file"))
	{
		actions = GDK_ACTION_COPY | GDK_ACTION_MOVE;
	}

	g_free (file_scheme);
	g_free (file_uri);
	g_free (dest_scheme);
	g_free (dest_uri);

	return actions;
}


static void
gth_file_source_vfs_finalize (GObject *object)
{
	GthFileSourceVfs *file_source_vfs = GTH_FILE_SOURCE_VFS (object);
	int               i;

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
	file_source_class->get_drop_actions = gth_file_source_vfs_get_drop_actions;
}


static void
gth_file_source_vfs_init (GthFileSourceVfs *file_source)
{
	int i;

	file_source->priv = gth_file_source_vfs_get_instance_private (file_source);
	file_source->priv->files = NULL;
	file_source->priv->list_ready_func = NULL;
	file_source->priv->start_dir_func = NULL;
	file_source->priv->for_each_file_func = NULL;
	file_source->priv->ready_func = NULL;
	file_source->priv->user_data = NULL;
	file_source->priv->hidden_files = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);
	file_source->priv->monitors = g_hash_table_new_full (g_file_hash, (GEqualFunc) g_file_equal, g_object_unref, g_object_unref);
	for (i = 0; i < GTH_MONITOR_N_EVENTS; i++)
		file_source->priv->monitor_queue[i] = NULL;
	file_source->priv->monitor_update_id = 0;
	file_source->priv->mount_monitor = NULL;
	file_source->priv->check_hidden_files = FALSE;

	gth_file_source_add_scheme (GTH_FILE_SOURCE (file_source), "file");
}
