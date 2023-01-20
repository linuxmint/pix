/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2012 Free Software Foundation, Inc.
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
#include <gthumb.h>
#include "gth-file-source-selections.h"
#include "gth-selections-manager.h"


struct _GthFileSourceSelectionsPrivate {
	ListReady  ready_func;
	gpointer   ready_data;
};


G_DEFINE_TYPE_WITH_CODE (GthFileSourceSelections,
			 gth_file_source_selections,
			 GTH_TYPE_FILE_SOURCE,
			 G_ADD_PRIVATE (GthFileSourceSelections))


static GList *
get_entry_points (GthFileSource *file_source)
{
	GList     *list = NULL;
	GFile     *file;
	GFileInfo *info;

	file = g_file_new_for_uri ("selection:///");
	info = gth_file_source_get_file_info (file_source, file, GFILE_BASIC_ATTRIBUTES);
	list = g_list_append (list, gth_file_data_new (file, info));

	g_object_unref (info);
	g_object_unref (file);

	return list;
}


static GFile *
gth_file_source_selections_to_gio_file (GthFileSource *file_source,
					GFile         *file)
{
	return g_file_dup (file);
}


static void
update_file_info (GthFileSource *file_source,
		  GFile         *file,
		  GFileInfo     *info)
{
	gth_selections_manager_update_file_info (file, info);
}


static GFileInfo *
gth_file_source_selections_get_file_info (GthFileSource *file_source,
					  GFile         *file,
					  const char    *attributes)
{
	GFileInfo *file_info;

	file_info = g_file_info_new ();
	update_file_info (file_source, file, file_info);

	return file_info;
}


static GthFileData *
gth_file_source_selections_get_file_data (GthFileSource *file_source,
					  GFile         *file,
					  GFileInfo     *info)
{
	GthFileData *file_data = NULL;

	switch (g_file_info_get_file_type (info)) {
	case G_FILE_TYPE_REGULAR:
		file_data = gth_file_data_new (file, info);
		break;

	case G_FILE_TYPE_DIRECTORY:
		update_file_info (file_source, file, info);
		file_data = gth_file_data_new (file, info);
		break;

	default:
		break;
	}

	return file_data;
}


static void
gth_file_source_selections_write_metadata (GthFileSource *file_source,
					   GthFileData   *file_data,
					   const char    *attributes,
					   ReadyCallback  callback,
					   gpointer       user_data)
{
	if (_g_file_attributes_matches_any (attributes, "sort::*"))
		gth_selections_manager_set_sort_type (file_data->file,
						      g_file_info_get_attribute_string (file_data->info, "sort::type"),
						      g_file_info_get_attribute_boolean (file_data->info, "sort::inverse"));

	object_ready_with_error (file_source, callback, user_data, NULL);
}


static void
gth_file_source_selections_read_metadata (GthFileSource *file_source,
					  GthFileData   *file_data,
					  const char    *attributes,
					  ReadyCallback  callback,
					  gpointer       user_data)
{
	int n_selection;

	n_selection = _g_file_get_n_selection (file_data->file);
	if (n_selection < 0) {
		GError *error = g_error_new_literal (G_IO_ERROR, G_IO_ERROR_NOT_FOUND, "Invalid location." /* FIXME: mark as translatable after string freeze */);
		object_ready_with_error (file_source, callback, user_data, error);
		return;
	}

	update_file_info (file_source, file_data->file, file_data->info);
	object_ready_with_error (file_source, callback, user_data, NULL);
}


static void
gth_file_source_selections_rename (GthFileSource *file_source,
				   GFile         *file,
				   const char    *edit_name,
				   ReadyCallback  callback,
				   gpointer       user_data)
{
	object_ready_with_error (file_source, callback, user_data, NULL);
}


static void
gth_file_source_selections_for_each_child (GthFileSource        *file_source,
					   GFile                *parent,
					   gboolean              recursive,
					   const char           *attributes,
					   StartDirCallback      start_dir_func,
					   ForEachChildCallback  for_each_file_func,
					   ReadyCallback         ready_func,
					   gpointer              user_data)
{
	if (start_dir_func != NULL) {
		GFileInfo *info;
		GError    *error = NULL;

		info = gth_file_source_selections_get_file_info (file_source, parent, "");

		switch (start_dir_func (parent, info, &error, user_data)) {
		case DIR_OP_CONTINUE:
			break;
		case DIR_OP_SKIP:
			object_ready_with_error (file_source, ready_func, user_data, NULL);
			return;
		case DIR_OP_STOP:
			object_ready_with_error (file_source, ready_func, user_data, error);
			g_object_unref (info);
			return;
		}

		g_object_unref (info);
	}


	gth_selections_manager_for_each_child (parent,
					       attributes,
					       gth_file_source_get_cancellable (file_source),
					       for_each_file_func,
					       ready_func,
					       user_data);
}


static void
gth_file_source_selections_copy (GthFileSource    *file_source,
				 GthFileData      *destination,
				 GList            *file_list, /* GFile * list */
				 gboolean          move,
				 int               destination_position,
				 ProgressCallback  progress_callback,
				 DialogCallback    dialog_callback,
				 ReadyCallback     ready_callback,
				 gpointer          user_data)
{
	if (gth_selections_manager_add_files (destination->file,
					      file_list,
					      destination_position))
	{
		gth_monitor_folder_changed (gth_main_get_default_monitor (),
					    destination->file,
					    file_list,
				            GTH_MONITOR_EVENT_CREATED);
	}

	object_ready_with_error (file_source, ready_callback, user_data, NULL);
}


static gboolean
gth_file_source_selections_is_reorderable (GthFileSource *file_source)
{
	return TRUE;
}


static void
gth_file_source_selections_reorder (GthFileSource *file_source,
				    GthFileData   *destination,
				    GList         *visible_files, /* GFile list */
				    GList         *files_to_move, /* GFile list */
				    int            dest_pos,
				    ReadyCallback  callback,
				    gpointer       data)
{
	gth_selections_manager_reorder (destination->file,
					visible_files,
					files_to_move,
					dest_pos);
	object_ready_with_error (file_source, callback, data, NULL);
}


static void
gth_file_source_selections_remove (GthFileSource *file_source,
				   GthFileData   *location,
				   GList         *file_list /* GthFileData list */,
				   gboolean       permanently,
				   GtkWindow     *parent)
{
	GList *files;

	files = gth_file_data_list_to_file_list (file_list);
	gth_selections_manager_remove_files (location->file, files, TRUE);

	_g_object_list_unref (files);
}


static void
gth_file_source_selections_deleted_from_disk (GthFileSource *file_source,
					      GthFileData   *location,
					      GList         *file_list)
{
	gth_selections_manager_remove_files (location->file, file_list, FALSE);
}


static gboolean
gth_file_source_selections_shows_extra_widget (GthFileSource *file_source)
{
	return TRUE;
}


static GdkDragAction
gth_file_source_selections_get_drop_actions (GthFileSource *file_source,
					     GFile         *destination,
					     GFile         *file)
{
	GdkDragAction  actions = 0;
	char          *file_uri;
	char          *file_scheme;

	file_uri = g_file_get_uri (file);
	file_scheme = gth_main_get_source_scheme (file_uri);

	if (_g_file_has_scheme (destination, "selection")
		&& _g_str_equal (file_scheme, "file"))
	{
		/* Copy files into a selection. */
		actions = GDK_ACTION_COPY;
	}

	g_free (file_scheme);
	g_free (file_uri);

	return actions;
}


static void
gth_file_source_selections_class_init (GthFileSourceSelectionsClass *class)
{
	GthFileSourceClass *file_source_class;

	file_source_class = (GthFileSourceClass*) class;
	file_source_class->get_entry_points = get_entry_points;
	file_source_class->to_gio_file = gth_file_source_selections_to_gio_file;
	file_source_class->get_file_info = gth_file_source_selections_get_file_info;
	file_source_class->get_file_data = gth_file_source_selections_get_file_data;
	file_source_class->write_metadata = gth_file_source_selections_write_metadata;
	file_source_class->read_metadata = gth_file_source_selections_read_metadata;
	file_source_class->rename = gth_file_source_selections_rename;
	file_source_class->for_each_child = gth_file_source_selections_for_each_child;
	file_source_class->copy = gth_file_source_selections_copy;
	file_source_class->is_reorderable  = gth_file_source_selections_is_reorderable;
	file_source_class->reorder = gth_file_source_selections_reorder;
	file_source_class->remove = gth_file_source_selections_remove;
	file_source_class->deleted_from_disk = gth_file_source_selections_deleted_from_disk;
	file_source_class->shows_extra_widget = gth_file_source_selections_shows_extra_widget;
	file_source_class->get_drop_actions = gth_file_source_selections_get_drop_actions;
}


static void
gth_file_source_selections_init (GthFileSourceSelections *self)
{
	gth_file_source_add_scheme (GTH_FILE_SOURCE (self), "selection");
	self->priv = gth_file_source_selections_get_instance_private (self);
	self->priv->ready_func = NULL;
	self->priv->ready_data = NULL;
}
