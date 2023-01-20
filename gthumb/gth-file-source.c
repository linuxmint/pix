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
#include <gtk/gtk.h>
#include "gth-file-data.h"
#include "glib-utils.h"
#include "gth-file-source.h"
#include "gth-main.h"


struct _GthFileSourcePrivate {
	GList        *schemes;
	gboolean      active;
	GList        *queue;
	GCancellable *cancellable;
};


G_DEFINE_TYPE_WITH_CODE (GthFileSource,
			 gth_file_source,
			 G_TYPE_OBJECT,
			 G_ADD_PRIVATE (GthFileSource))


/* -- queue -- */


typedef enum {
	FILE_SOURCE_OP_WRITE_METADATA,
	FILE_SOURCE_OP_READ_METADATA,
	FILE_SOURCE_OP_LIST,
	FILE_SOURCE_OP_FOR_EACH_CHILD,
	FILE_SOURCE_OP_READ_ATTRIBUTES,
	FILE_SOURCE_OP_RENAME,
	FILE_SOURCE_OP_COPY,
	FILE_SOURCE_OP_REORDER,
	FILE_SOURCE_OP_REMOVE,
	FILE_SOURCE_OP_DELETED_FROM_DISK,
	FILE_SOURCE_OP_GET_FREE_SPACE
} FileSourceOp;


typedef struct {
	GFile      *folder;
	const char *attributes;
	ListReady   func;
	gpointer    data;
} ListData;


typedef struct {
	GList      *files;
	const char *attributes;
	ListReady   func;
	gpointer    data;
} ReadAttributesData;


typedef struct {
	GFile         *file;
	char          *edit_name;
	ReadyCallback  callback;
	gpointer       data;
} RenameData;


typedef struct {
	GthFileData      *destination;
	GList            *file_list;
	gboolean          move;
	int               destination_position;
	ProgressCallback  progress_callback;
	DialogCallback    dialog_callback;
	ReadyCallback     ready_callback;
	gpointer          data;
} CopyData;


typedef struct {
	GthFileData   *destination;
	GList         *visible_files;
	GList         *files_to_move;
	int            dest_pos;
	ReadyCallback  ready_callback;
	gpointer       data;
} ReorderData;


typedef struct {
	GthFileData   *file_data;
	char          *attributes;
	ReadyCallback  ready_callback;
	gpointer       data;
} WriteMetadataData;


typedef struct {
	GthFileData   *file_data;
	char          *attributes;
	ReadyCallback  ready_callback;
	gpointer       data;
} ReadMetadataData;


typedef struct {
	GFile                *parent;
	gboolean              recursive;
	const char           *attributes;
	StartDirCallback      dir_func;
	ForEachChildCallback  child_func;
	ReadyCallback         ready_func;
	gpointer              data;
} ForEachChildData;


typedef struct {
	GthFileData *location;
	GList       *file_list;
	gboolean     permanently;
	GtkWindow   *parent;
} RemoveData;


typedef struct {
	GthFileData *location;
	GList       *file_list;
} DeletedFromDiskData;


typedef struct {
	GFile              *location;
	SpaceReadyCallback  callback;
	gpointer            data;
} GetFreeSpaceData;


typedef struct {
	GthFileSource *file_source;
	FileSourceOp   op;
	union {
		ListData            list;
		ForEachChildData    fec;
		ReadAttributesData  read_attributes;
		RenameData          rename;
		CopyData            copy;
		ReorderData         reorder;
		WriteMetadataData   write_metadata;
		ReadMetadataData    read_metadata;
		RemoveData          remove;
		DeletedFromDiskData deleted_from_disk;
		GetFreeSpaceData    get_free_space;
	} data;
} FileSourceAsyncOp;


static void
file_source_async_op_free (FileSourceAsyncOp *async_op)
{
	switch (async_op->op) {
	case FILE_SOURCE_OP_WRITE_METADATA:
		g_free (async_op->data.write_metadata.attributes);
		g_object_unref (async_op->data.write_metadata.file_data);
		break;
	case FILE_SOURCE_OP_READ_METADATA:
		g_free (async_op->data.write_metadata.attributes);
		g_object_unref (async_op->data.read_metadata.file_data);
		break;
	case FILE_SOURCE_OP_LIST:
		g_object_unref (async_op->data.list.folder);
		break;
	case FILE_SOURCE_OP_FOR_EACH_CHILD:
		g_object_unref (async_op->data.fec.parent);
		break;
	case FILE_SOURCE_OP_READ_ATTRIBUTES:
		_g_object_list_unref (async_op->data.read_attributes.files);
		break;
	case FILE_SOURCE_OP_RENAME:
		g_object_unref (async_op->data.rename.file);
		g_free (async_op->data.rename.edit_name);
		break;
	case FILE_SOURCE_OP_COPY:
		g_object_unref (async_op->data.copy.destination);
		_g_object_list_unref (async_op->data.copy.file_list);
		break;
	case FILE_SOURCE_OP_REORDER:
		g_object_unref (async_op->data.copy.destination);
		_g_object_list_unref (async_op->data.copy.file_list);
		break;
	case FILE_SOURCE_OP_REMOVE:
		_g_object_unref (async_op->data.remove.location);
		_g_object_list_unref (async_op->data.remove.file_list);
		break;
	case FILE_SOURCE_OP_DELETED_FROM_DISK:
		_g_object_unref (async_op->data.deleted_from_disk.location);
		_g_object_list_unref (async_op->data.deleted_from_disk.file_list);
		break;
	case FILE_SOURCE_OP_GET_FREE_SPACE:
		_g_object_unref (async_op->data.get_free_space.location);
		break;
	}

	g_free (async_op);
}


static void
gth_file_source_queue_write_metadata (GthFileSource *file_source,
			              GthFileData   *file_data,
			              const char    *attributes,
			              ReadyCallback  callback,
			              gpointer       data)
{
	FileSourceAsyncOp *async_op;

	async_op = g_new0 (FileSourceAsyncOp, 1);
	async_op->file_source = file_source;
	async_op->op = FILE_SOURCE_OP_WRITE_METADATA;
	async_op->data.write_metadata.file_data = g_object_ref (file_data);
	async_op->data.write_metadata.attributes = g_strdup (attributes);
	async_op->data.write_metadata.ready_callback = callback;
	async_op->data.write_metadata.data = data;

	file_source->priv->queue = g_list_append (file_source->priv->queue, async_op);
}


static void
gth_file_source_queue_read_metadata (GthFileSource *file_source,
			             GthFileData   *file_data,
			             const char    *attributes,
			             ReadyCallback  callback,
			             gpointer       data)
{
	FileSourceAsyncOp *async_op;

	async_op = g_new0 (FileSourceAsyncOp, 1);
	async_op->file_source = file_source;
	async_op->op = FILE_SOURCE_OP_READ_METADATA;
	async_op->data.read_metadata.file_data = g_object_ref (file_data);
	async_op->data.write_metadata.attributes = g_strdup (attributes);
	async_op->data.read_metadata.ready_callback = callback;
	async_op->data.read_metadata.data = data;

	file_source->priv->queue = g_list_append (file_source->priv->queue, async_op);
}


static void
gth_file_source_queue_list (GthFileSource *file_source,
			    GFile         *folder,
			    const char    *attributes,
			    ListReady      func,
			    gpointer       data)
{
	FileSourceAsyncOp *async_op;

	async_op = g_new0 (FileSourceAsyncOp, 1);
	async_op->file_source = file_source;
	async_op->op = FILE_SOURCE_OP_LIST;
	async_op->data.list.folder = g_file_dup (folder);
	async_op->data.list.attributes = attributes;
	async_op->data.list.func = func;
	async_op->data.list.data = data;

	file_source->priv->queue = g_list_append (file_source->priv->queue, async_op);
}


static void
gth_file_source_queue_for_each_child (GthFileSource        *file_source,
				      GFile                *parent,
				      gboolean              recursive,
				      const char           *attributes,
				      StartDirCallback      dir_func,
				      ForEachChildCallback  child_func,
				      ReadyCallback         ready_func,
				      gpointer              data)
{
	FileSourceAsyncOp *async_op;

	async_op = g_new0 (FileSourceAsyncOp, 1);
	async_op->file_source = file_source;
	async_op->op = FILE_SOURCE_OP_FOR_EACH_CHILD;
	async_op->data.fec.parent = g_file_dup (parent);
	async_op->data.fec.recursive = recursive;
	async_op->data.fec.attributes = attributes;
	async_op->data.fec.dir_func = dir_func;
	async_op->data.fec.child_func = child_func;
	async_op->data.fec.ready_func = ready_func;
	async_op->data.fec.data = data;

	file_source->priv->queue = g_list_append (file_source->priv->queue, async_op);
}


static void
gth_file_source_queue_read_attributes (GthFileSource *file_source,
				       GList         *files,
				       const char    *attributes,
				       ListReady      func,
				       gpointer       data)
{
	FileSourceAsyncOp *async_op;

	async_op = g_new0 (FileSourceAsyncOp, 1);
	async_op->file_source = file_source;
	async_op->op = FILE_SOURCE_OP_READ_ATTRIBUTES;
	async_op->data.read_attributes.files = _g_object_list_ref (files);
	async_op->data.read_attributes.attributes = attributes;
	async_op->data.read_attributes.func = func;
	async_op->data.read_attributes.data = data;

	file_source->priv->queue = g_list_append (file_source->priv->queue, async_op);
}


static void
gth_file_source_queue_rename (GthFileSource *file_source,
			      GFile         *file,
			      const char    *edit_name,
			      ReadyCallback  callback,
			      gpointer       data)
{
	FileSourceAsyncOp *async_op;

	async_op = g_new0 (FileSourceAsyncOp, 1);
	async_op->file_source = file_source;
	async_op->op = FILE_SOURCE_OP_RENAME;
	async_op->data.rename.file = g_file_dup (file);
	async_op->data.rename.edit_name = g_strdup (edit_name);
	async_op->data.rename.callback = callback;
	async_op->data.rename.data = data;

	file_source->priv->queue = g_list_append (file_source->priv->queue, async_op);
}


static void
gth_file_source_queue_copy (GthFileSource    *file_source,
			    GthFileData      *destination,
			    GList            *file_list,
			    gboolean          move,
			    int               destination_position,
			    ProgressCallback  progress_callback,
			    DialogCallback    dialog_callback,
			    ReadyCallback     ready_callback,
			    gpointer          data)
{
	FileSourceAsyncOp *async_op;

	async_op = g_new0 (FileSourceAsyncOp, 1);
	async_op->file_source = file_source;
	async_op->op = FILE_SOURCE_OP_COPY;
	async_op->data.copy.destination = gth_file_data_dup (destination);
	async_op->data.copy.file_list = _g_file_list_dup (file_list);
	async_op->data.copy.move = move;
	async_op->data.copy.destination_position = destination_position;
	async_op->data.copy.progress_callback = progress_callback;
	async_op->data.copy.dialog_callback = dialog_callback;
	async_op->data.copy.ready_callback = ready_callback;
	async_op->data.copy.data = data;

	file_source->priv->queue = g_list_append (file_source->priv->queue, async_op);
}


static void
gth_file_source_queue_reorder (GthFileSource    *file_source,
			       GthFileData      *destination,
			       GList            *visible_files, /* GFile list */
			       GList            *files_to_move, /* GFile list */
			       int               dest_pos,
			       ReadyCallback     ready_callback,
			       gpointer          data)
{
	FileSourceAsyncOp *async_op;

	async_op = g_new0 (FileSourceAsyncOp, 1);
	async_op->file_source = file_source;
	async_op->op = FILE_SOURCE_OP_REORDER;
	async_op->data.reorder.destination = gth_file_data_dup (destination);
	async_op->data.reorder.visible_files = _g_file_list_dup (visible_files);
	async_op->data.reorder.files_to_move = _g_file_list_dup (files_to_move);
	async_op->data.reorder.dest_pos = dest_pos;
	async_op->data.reorder.ready_callback = ready_callback;
	async_op->data.reorder.data = data;

	file_source->priv->queue = g_list_append (file_source->priv->queue, async_op);
}


static void
gth_file_source_queue_remove (GthFileSource *file_source,
			      GthFileData   *location,
			      GList         *file_list,
			      gboolean       permanently,
			      GtkWindow     *parent)
{
	FileSourceAsyncOp *async_op;

	async_op = g_new0 (FileSourceAsyncOp, 1);
	async_op->file_source = file_source;
	async_op->op = FILE_SOURCE_OP_REMOVE;
	async_op->data.remove.location = gth_file_data_dup (location);
	async_op->data.remove.file_list = gth_file_data_list_dup (file_list);
	async_op->data.remove.permanently = permanently;
	async_op->data.remove.parent = parent;

	file_source->priv->queue = g_list_append (file_source->priv->queue, async_op);
}


static void
gth_file_source_queue_deleted_from_disk (GthFileSource *file_source,
					 GthFileData   *location,
					 GList         *file_list)
{
	FileSourceAsyncOp *async_op;

	async_op = g_new0 (FileSourceAsyncOp, 1);
	async_op->file_source = file_source;
	async_op->op = FILE_SOURCE_OP_DELETED_FROM_DISK;
	async_op->data.deleted_from_disk.location = gth_file_data_dup (location);
	async_op->data.deleted_from_disk.file_list = _g_file_list_dup (file_list);

	file_source->priv->queue = g_list_append (file_source->priv->queue, async_op);
}


static void
gth_file_source_queue_get_free_space (GthFileSource      *file_source,
				      GFile              *location,
				      SpaceReadyCallback  callback,
				      gpointer            data)
{
	FileSourceAsyncOp *async_op;

	async_op = g_new0 (FileSourceAsyncOp, 1);
	async_op->file_source = file_source;
	async_op->op = FILE_SOURCE_OP_GET_FREE_SPACE;
	async_op->data.get_free_space.location = g_file_dup (location);
	async_op->data.get_free_space.callback = callback;
	async_op->data.get_free_space.data = data;

	file_source->priv->queue = g_list_append (file_source->priv->queue, async_op);
}


static void
gth_file_source_exec_next_in_queue (GthFileSource *file_source)
{
	GList             *head;
	FileSourceAsyncOp *async_op;

	if (file_source->priv->queue == NULL)
		return;

	g_cancellable_reset (file_source->priv->cancellable);

	head = file_source->priv->queue;
	file_source->priv->queue = g_list_remove_link (file_source->priv->queue, head);

	async_op = head->data;
	switch (async_op->op) {
	case FILE_SOURCE_OP_WRITE_METADATA:
		gth_file_source_write_metadata (file_source,
					        async_op->data.write_metadata.file_data,
					        async_op->data.write_metadata.attributes,
					        async_op->data.write_metadata.ready_callback,
					        async_op->data.write_metadata.data);
		break;
	case FILE_SOURCE_OP_READ_METADATA:
		gth_file_source_read_metadata (file_source,
					       async_op->data.read_metadata.file_data,
					       async_op->data.write_metadata.attributes,
					       async_op->data.read_metadata.ready_callback,
					       async_op->data.read_metadata.data);
		break;
	case FILE_SOURCE_OP_LIST:
		gth_file_source_list (file_source,
				      async_op->data.list.folder,
				      async_op->data.list.attributes,
				      async_op->data.list.func,
				      async_op->data.list.data);
		break;
	case FILE_SOURCE_OP_FOR_EACH_CHILD:
		gth_file_source_for_each_child (file_source,
						async_op->data.fec.parent,
						async_op->data.fec.recursive,
						async_op->data.fec.attributes,
						async_op->data.fec.dir_func,
						async_op->data.fec.child_func,
						async_op->data.fec.ready_func,
						async_op->data.fec.data);
		break;
	case FILE_SOURCE_OP_READ_ATTRIBUTES:
		gth_file_source_read_attributes (file_source,
						 async_op->data.read_attributes.files,
						 async_op->data.read_attributes.attributes,
						 async_op->data.read_attributes.func,
						 async_op->data.read_attributes.data);
		break;
	case FILE_SOURCE_OP_RENAME:
		gth_file_source_rename (file_source,
					async_op->data.rename.file,
					async_op->data.rename.edit_name,
					async_op->data.rename.callback,
					async_op->data.rename.data);
		break;
	case FILE_SOURCE_OP_COPY:
		gth_file_source_copy (file_source,
				      async_op->data.copy.destination,
				      async_op->data.copy.file_list,
				      async_op->data.copy.move,
				      async_op->data.copy.destination_position,
				      async_op->data.copy.progress_callback,
				      async_op->data.copy.dialog_callback,
				      async_op->data.copy.ready_callback,
				      async_op->data.copy.data);
		break;
	case FILE_SOURCE_OP_REORDER:
		gth_file_source_reorder (file_source,
					 async_op->data.reorder.destination,
					 async_op->data.reorder.visible_files,
					 async_op->data.reorder.files_to_move,
					 async_op->data.reorder.dest_pos,
				         async_op->data.reorder.ready_callback,
					 async_op->data.reorder.data);
		break;
	case FILE_SOURCE_OP_REMOVE:
		gth_file_source_remove (file_source,
					async_op->data.remove.location,
					async_op->data.remove.file_list,
					async_op->data.remove.permanently,
					async_op->data.remove.parent);
		break;

	case FILE_SOURCE_OP_DELETED_FROM_DISK:
		gth_file_source_queue_deleted_from_disk (file_source,
							 async_op->data.deleted_from_disk.location,
							 async_op->data.deleted_from_disk.file_list);
		break;

	case FILE_SOURCE_OP_GET_FREE_SPACE:
		gth_file_source_get_free_space (file_source,
						async_op->data.get_free_space.location,
						async_op->data.get_free_space.callback,
						async_op->data.get_free_space.data);
		break;
	}

	file_source_async_op_free (async_op);
	g_list_free (head);
}


static void
gth_file_source_clear_queue (GthFileSource  *file_source)
{
	g_list_foreach (file_source->priv->queue, (GFunc) file_source_async_op_free, NULL);
	g_list_free (file_source->priv->queue);
	file_source->priv->queue = NULL;
}


/* -- */


static GList *
base_get_entry_points (GthFileSource  *file_source)
{
	return NULL;
}


static GList *
base_get_current_list (GthFileSource  *file_source,
		       GFile          *file)
{
	GList *list = NULL;
	GFile *parent;

	if (file == NULL)
		return NULL;

	parent = g_file_dup (file);
	while (parent != NULL) {
		GFile *tmp;

		list = g_list_prepend (list, g_object_ref (parent));
		tmp = g_file_get_parent (parent);
		g_object_unref (parent);
		parent = tmp;
	}

	return g_list_reverse (list);
}


static GFile *
base_to_gio_file (GthFileSource *file_source,
		  GFile         *file)
{
	return g_file_dup (file);
}


static GFileInfo *
base_get_file_info (GthFileSource *file_source,
		    GFile         *file,
		    const char    *attributes)
{
	return NULL;
}


static GthFileData *
base_get_file_data (GthFileSource  *file_source,
		    GFile          *file,
		    GFileInfo      *info)
{
	return gth_file_data_new (file, info);
}


static void
base_write_metadata (GthFileSource *file_source,
		     GthFileData   *file_data,
		     const char    *attributes,
		     ReadyCallback  callback,
		     gpointer       data)
{
	object_ready_with_error (file_source, callback, data, NULL);
}


typedef struct {
	GFile              *location;
	GthFileSource      *file_source;
	SpaceReadyCallback  callback;
	gpointer            data;
} BaseFreeSpaceData;


static void
get_free_space_ready_cb (GObject      *source_object,
			 GAsyncResult *res,
			 gpointer      user_data)
{
	BaseFreeSpaceData *free_space_data = user_data;
	GFileInfo         *info;
	GError            *error = NULL;
	guint64            total_size = 0;
	guint64            free_space = 0;

	info = g_file_query_filesystem_info_finish (free_space_data->location, res, &error);
	if (info != NULL) {
		total_size = g_file_info_get_attribute_uint64 (info, G_FILE_ATTRIBUTE_FILESYSTEM_SIZE);
		free_space = g_file_info_get_attribute_uint64 (info, G_FILE_ATTRIBUTE_FILESYSTEM_FREE);
		g_object_unref (info);
	}

	free_space_data->callback (free_space_data->file_source,
				   total_size,
				   free_space,
				   error,
				   free_space_data->data);

	if (error != NULL)
		g_error_free (error);
	g_object_unref (free_space_data->location);
	g_free (free_space_data);
}


static void
base_get_free_space (GthFileSource        *file_source,
		     GFile                *location,
		     SpaceReadyCallback    callback,
		     gpointer              data)
{
	BaseFreeSpaceData *free_space_data;

	free_space_data = g_new0 (BaseFreeSpaceData, 1);
	free_space_data->location = g_object_ref (location);
	free_space_data->file_source = file_source;
	free_space_data->callback = callback;
	free_space_data->data = data;
	g_file_query_filesystem_info_async (location,
					    G_FILE_ATTRIBUTE_FILESYSTEM_SIZE "," G_FILE_ATTRIBUTE_FILESYSTEM_FREE,
					    G_PRIORITY_DEFAULT,
					    file_source->priv->cancellable,
					    get_free_space_ready_cb,
					    free_space_data);
}


/* -- base_read_metadata -- */


typedef struct {
	GthFileSource *file_source;
	GthFileData   *file_data;
	ReadyCallback  callback;
	gpointer       data;
} ReadMetadataOpData;


static void
read_metadata_free (ReadMetadataOpData *read_metadata)
{
	g_object_unref (read_metadata->file_source);
	g_object_unref (read_metadata->file_data);
	g_free (read_metadata);
}


static void
read_metadata_info_ready_cb (GList    *files,
			     GError   *error,
			     gpointer  user_data)
{
	ReadMetadataOpData *read_metadata = user_data;
	GthFileData        *result;

	if (error != NULL) {
		read_metadata->callback (G_OBJECT (read_metadata->file_source), error, read_metadata->data);
		read_metadata_free (read_metadata);
		return;
	}

	result = files->data;
	g_file_info_copy_into (result->info, read_metadata->file_data->info);
	read_metadata->callback (G_OBJECT (read_metadata->file_source), NULL, read_metadata->data);

	read_metadata_free (read_metadata);
}


static void
base_read_metadata (GthFileSource *file_source,
		    GthFileData   *file_data,
		    const char    *attributes,
		    ReadyCallback  callback,
		    gpointer       data)
{
	ReadMetadataOpData *read_metadata;
	GList              *files;

	read_metadata = g_new0 (ReadMetadataOpData, 1);
	read_metadata->file_source = g_object_ref (file_source);
	read_metadata->file_data = g_object_ref (file_data);
	read_metadata->callback = callback;
	read_metadata->data = data;

	files = g_list_prepend (NULL, file_data->file);
	_g_query_all_metadata_async (files,
				     GTH_LIST_DEFAULT,
				     attributes,
				     file_source->priv->cancellable,
				     read_metadata_info_ready_cb,
				     read_metadata);

	g_list_free (files);
}


static void
base_rename (GthFileSource *file_source,
	     GFile         *file,
	     const char    *edit_name,
	     ReadyCallback  callback,
	     gpointer       data)
{
	GFile  *parent;
	GFile  *new_file;
	GFile  *source;
	GFile  *destination;
	GError *error = NULL;

	parent = g_file_get_parent (file);
	new_file = g_file_get_child_for_display_name (parent, edit_name, &error);
	if (new_file == NULL) {
		object_ready_with_error (file_source, callback, data, error);
		return;
	}

	source = gth_file_source_to_gio_file (file_source, file);
	destination = gth_file_source_to_gio_file (file_source, new_file);

	if (g_file_move (source,
			 destination,
			 0,
			 gth_file_source_get_cancellable (file_source),
			 NULL,
			 NULL,
			 &error))
	{
		GthMonitor *monitor;

		monitor = gth_main_get_default_monitor ();
		gth_monitor_file_renamed (monitor, file, new_file);
	}
	object_ready_with_error (file_source, callback, data, error);

	g_object_unref (destination);
	g_object_unref (source);
}


static gboolean
base_can_cut (GthFileSource *file_source)
{
	return FALSE;
}


static void
base_monitor_entry_points (GthFileSource *file_source)
{
	/* void */
}


static void
base_monitor_directory (GthFileSource  *file_source,
			GFile          *file,
			gboolean        activate)
{
	/* void */
}


static gboolean
base_is_reorderable (GthFileSource *file_source)
{
	return FALSE;
}


static void
base_reorder (GthFileSource *file_source,
	      GthFileData   *destination,
	      GList         *visible_files, /* GFile list */
	      GList         *files_to_move, /* GFile list */
	      int            dest_pos,
	      ReadyCallback  callback,
	      gpointer       data)
{
	/* void */
}


static void
base_remove (GthFileSource *file_source,
	     GthFileData   *location,
	     GList         *file_list, /* GFile list */
	     gboolean       permanently,
	     GtkWindow     *parent)
{
	/* void */
}


static void
base_deleted_from_disk (GthFileSource *file_source,
			GthFileData   *location,
			GList         *file_list /* GFile list */)
{
	/* void */
}


static gboolean
base_shows_extra_widget (GthFileSource *file_source)
{
	return FALSE;
}


static GdkDragAction
base_get_drop_actions (GthFileSource *file_source,
		       GFile         *destination,
		       GFile         *file)
{
	return 0; /* no action supported by default. */
}


static void
gth_file_source_finalize (GObject *object)
{
	GthFileSource *file_source = GTH_FILE_SOURCE (object);

	gth_file_source_clear_queue (file_source);
	_g_string_list_free (file_source->priv->schemes);
	_g_object_unref (file_source->priv->cancellable);

	G_OBJECT_CLASS (gth_file_source_parent_class)->finalize (object);
}


static void
gth_file_source_class_init (GthFileSourceClass *class)
{
	GObjectClass *object_class;

	object_class = (GObjectClass*) class;
	object_class->finalize = gth_file_source_finalize;

	class->get_entry_points = base_get_entry_points;
	class->get_current_list = base_get_current_list;
	class->to_gio_file = base_to_gio_file;
	class->get_file_info = base_get_file_info;
	class->get_file_data = base_get_file_data;
	class->write_metadata = base_write_metadata;
	class->read_metadata = base_read_metadata;
	class->rename = base_rename;
	class->can_cut = base_can_cut;
	class->monitor_entry_points = base_monitor_entry_points;
	class->monitor_directory = base_monitor_directory;
	class->is_reorderable = base_is_reorderable;
	class->reorder = base_reorder;
	class->remove = base_remove;
	class->deleted_from_disk = base_deleted_from_disk;
	class->get_free_space = base_get_free_space;
	class->shows_extra_widget = base_shows_extra_widget;
	class->get_drop_actions = base_get_drop_actions;
}


static void
gth_file_source_init (GthFileSource *file_source)
{
	file_source->priv = gth_file_source_get_instance_private (file_source);
	file_source->priv->schemes = NULL;
	file_source->priv->active = FALSE;
	file_source->priv->queue = NULL;
	file_source->priv->cancellable = g_cancellable_new ();
}


void
gth_file_source_add_scheme (GthFileSource  *file_source,
			    const char     *scheme)
{
	file_source->priv->schemes = g_list_prepend (file_source->priv->schemes, g_strdup (scheme));
}


gboolean
gth_file_source_supports_scheme (GthFileSource *file_source,
				 const char    *uri)
{
	GList *scan;

	for (scan = file_source->priv->schemes; scan; scan = scan->next) {
		const char *scheme = scan->data;

		if (g_str_has_prefix (uri, scheme))
			return TRUE;
	}

	return FALSE;
}


void
gth_file_source_set_active (GthFileSource *file_source,
			    gboolean       active)
{
	file_source->priv->active = active;
	if (! active)
		gth_file_source_exec_next_in_queue (file_source);
}


void
gth_file_source_set_cancellable (GthFileSource *file_source,
				 GCancellable  *cancellable)
{
	_g_object_unref (file_source->priv->cancellable);
	file_source->priv->cancellable = NULL;
	if (cancellable != NULL)
		file_source->priv->cancellable = g_object_ref (cancellable);
}


GList *
gth_file_source_get_entry_points (GthFileSource *file_source)
{
	return GTH_FILE_SOURCE_GET_CLASS (G_OBJECT (file_source))->get_entry_points (file_source);
}


GList *
gth_file_source_get_current_list (GthFileSource *file_source,
				  GFile         *file)
{
	return GTH_FILE_SOURCE_GET_CLASS (G_OBJECT (file_source))->get_current_list (file_source, file);
}


GFile *
gth_file_source_to_gio_file (GthFileSource *file_source,
			     GFile         *file)
{
	return GTH_FILE_SOURCE_GET_CLASS (G_OBJECT (file_source))->to_gio_file (file_source, file);
}


GList *
gth_file_source_to_gio_file_list (GthFileSource *file_source,
				  GList         *files)
{
	GList *gio_files = NULL;
	GList *scan;

	for (scan = files; scan; scan = scan->next)
		gio_files = g_list_prepend (gio_files, gth_file_source_to_gio_file (file_source, (GFile *) scan->data));

	return g_list_reverse (gio_files);
}


GFileInfo *
gth_file_source_get_file_info (GthFileSource *file_source,
			       GFile         *file,
			       const char    *attributes)
{
	return GTH_FILE_SOURCE_GET_CLASS (G_OBJECT (file_source))->get_file_info (file_source, file, attributes);
}


GthFileData *
gth_file_source_get_file_data (GthFileSource *file_source,
			       GFile         *file,
			       GFileInfo     *info)
{
	return GTH_FILE_SOURCE_GET_CLASS (G_OBJECT (file_source))->get_file_data (file_source, file, info);
}


gboolean
gth_file_source_is_active (GthFileSource *file_source)
{
	return file_source->priv->active;
}


GCancellable *
gth_file_source_get_cancellable (GthFileSource *file_source)
{
	return file_source->priv->cancellable;
}


void
gth_file_source_cancel (GthFileSource *file_source)
{
	gth_file_source_clear_queue (file_source);
	g_cancellable_cancel (file_source->priv->cancellable);
}


void
gth_file_source_write_metadata (GthFileSource *file_source,
				GthFileData   *file_data,
				const char    *attributes,
				ReadyCallback  callback,
				gpointer       data)
{
	if (gth_file_source_is_active (file_source)) {
		gth_file_source_queue_write_metadata (file_source, file_data, attributes, callback, data);
		return;
	}
	g_cancellable_reset (file_source->priv->cancellable);
	GTH_FILE_SOURCE_GET_CLASS (G_OBJECT (file_source))->write_metadata (file_source, file_data, attributes, callback, data);
}


void
gth_file_source_read_metadata (GthFileSource *file_source,
			       GthFileData   *file_data,
			       const char    *attributes,
			       ReadyCallback  callback,
			       gpointer       data)
{
	if (gth_file_source_is_active (file_source)) {
		gth_file_source_queue_read_metadata (file_source, file_data, attributes, callback, data);
		return;
	}
	g_cancellable_reset (file_source->priv->cancellable);
	GTH_FILE_SOURCE_GET_CLASS (G_OBJECT (file_source))->read_metadata (file_source, file_data, attributes, callback, data);
}


/* -- gth_file_source_list -- */


typedef struct {
	GthFileSource *file_source;
	ListReady      ready_func;
	gpointer       user_data;
	GList         *files;
} ListOpData;


static void
list__done_func (GObject  *source,
		 GError   *error,
		 gpointer  user_data)
{
	ListOpData *data = user_data;

	data->files = g_list_reverse (data->files);
	data->ready_func (data->file_source, data->files, error, data->user_data);

	_g_object_list_unref (data->files);
	g_object_unref (data->file_source);
	g_free (data);
}


static void
list__for_each_file_func (GFile     *file,
			  GFileInfo *info,
			  gpointer   user_data)
{
	ListOpData *data = user_data;

	switch (g_file_info_get_file_type (info)) {
	case G_FILE_TYPE_REGULAR:
	case G_FILE_TYPE_DIRECTORY:
		data->files = g_list_prepend (data->files, gth_file_data_new (file, info));
		break;
	default:
		break;
	}
}


static DirOp
list__start_dir_func (GFile       *directory,
		      GFileInfo   *info,
		      GError     **error,
		      gpointer     user_data)
{
	return DIR_OP_CONTINUE;
}


void
gth_file_source_list (GthFileSource *file_source,
		      GFile         *folder,
		      const char    *attributes,
		      ListReady      func,
		      gpointer       user_data)
{
	ListOpData *data;

	if (gth_file_source_is_active (file_source)) {
		gth_file_source_queue_list (file_source, folder, attributes, func, user_data);
		return;
	}

	g_cancellable_reset (file_source->priv->cancellable);

	data = g_new0 (ListOpData, 1);
	data->file_source = g_object_ref (file_source);
	data->ready_func = func;
	data->user_data = user_data;

	gth_file_source_for_each_child (file_source,
				        folder,
				        FALSE,
				        attributes,
				        list__start_dir_func,
				        list__for_each_file_func,
				        list__done_func,
				        data);
}


void
gth_file_source_for_each_child (GthFileSource        *file_source,
				GFile                *parent,
				gboolean              recursive,
				const char           *attributes,
				StartDirCallback      dir_func,
				ForEachChildCallback  child_func,
				ReadyCallback         ready_func,
				gpointer              data)
{
	if (gth_file_source_is_active (file_source)) {
		gth_file_source_queue_for_each_child (file_source, parent, recursive, attributes, dir_func, child_func, ready_func, data);
		return;
	}
	g_cancellable_reset (file_source->priv->cancellable);
	GTH_FILE_SOURCE_GET_CLASS (G_OBJECT (file_source))->for_each_child (file_source, parent, recursive, attributes, dir_func, child_func, ready_func, data);
}


/* -- gth_file_source_read_attributes -- */


typedef struct {
	GthFileSource *file_source;
	ListReady      ready_func;
	gpointer       ready_data;
} ReadAttributesOpData;



static void
read_attributes_op_data_free (ReadAttributesOpData *data)
{
	g_object_unref (data->file_source);
	g_free (data);
}


static void
metadata_ready_cb (GList    *files,
	           GError   *error,
	           gpointer  user_data)
{
	ReadAttributesOpData *data = user_data;
	GList                *scan;
	GList                *result_files;

	gth_file_source_set_active (data->file_source, FALSE);

	result_files = NULL;
	for (scan = files; scan; scan = scan->next) {
		GthFileData *file_data = scan->data;
		result_files = g_list_prepend (result_files, gth_file_source_get_file_data (data->file_source, file_data->file, file_data->info));
	}
	result_files = g_list_reverse (result_files);

	data->ready_func (data->file_source,
			  result_files,
			  error,
			  data->ready_data);

	_g_object_list_unref (result_files);
	read_attributes_op_data_free (data);
}


void
gth_file_source_read_attributes (GthFileSource  *file_source,
				 GList          *files,
				 const char     *attributes,
				 ListReady       func,
				 gpointer        user_data)
{
	ReadAttributesOpData *data;
	GList                *gio_files;

	if (gth_file_source_is_active (file_source)) {
		gth_file_source_queue_read_attributes (file_source, files, attributes, func, user_data);
		return;
	}

	g_cancellable_reset (file_source->priv->cancellable);

	data = g_new0 (ReadAttributesOpData, 1);
	data->file_source = g_object_ref (file_source);
	data->ready_func = func;
	data->ready_data = user_data;

	gio_files = gth_file_source_to_gio_file_list (file_source, files);
	_g_query_all_metadata_async (gio_files,
				     GTH_LIST_DEFAULT,
				     attributes,
				     file_source->priv->cancellable,
				     metadata_ready_cb,
				     data);

	_g_object_list_unref (gio_files);
}


void
gth_file_source_rename (GthFileSource  *file_source,
			GFile          *file,
			const char     *edit_name,
			ReadyCallback   ready_callback,
			gpointer        data)
{
	if (gth_file_source_is_active (file_source)) {
		gth_file_source_queue_rename (file_source, file, edit_name, ready_callback, data);
		return;
	}
	g_cancellable_reset (file_source->priv->cancellable);
	GTH_FILE_SOURCE_GET_CLASS (G_OBJECT (file_source))->rename (file_source, file, edit_name, ready_callback, data);
}


void
gth_file_source_copy (GthFileSource    *file_source,
		      GthFileData      *destination,
		      GList            *file_list, /* GFile * list */
		      gboolean          move,
		      int               destination_position,
		      ProgressCallback  progress_callback,
		      DialogCallback    dialog_callback,
		      ReadyCallback     ready_callback,
		      gpointer          data)
{
	if (gth_file_source_is_active (file_source)) {
		gth_file_source_queue_copy (file_source, destination, file_list, move, destination_position, progress_callback, dialog_callback, ready_callback, data);
		return;
	}
	g_cancellable_reset (file_source->priv->cancellable);
	GTH_FILE_SOURCE_GET_CLASS (G_OBJECT (file_source))->copy (file_source, destination, file_list, move, destination_position, progress_callback, dialog_callback, ready_callback, data);
}


gboolean
gth_file_source_can_cut (GthFileSource *file_source)
{
	return GTH_FILE_SOURCE_GET_CLASS (G_OBJECT (file_source))->can_cut (file_source);
}


void
gth_file_source_monitor_entry_points (GthFileSource *file_source)
{
	GTH_FILE_SOURCE_GET_CLASS (G_OBJECT (file_source))->monitor_entry_points (file_source);
}


void
gth_file_source_monitor_directory (GthFileSource *file_source,
				   GFile         *file,
				   gboolean       activate)
{
	g_return_if_fail (file != NULL);

	GTH_FILE_SOURCE_GET_CLASS (G_OBJECT (file_source))->monitor_directory (file_source, file, activate);
}


gboolean
gth_file_source_is_reorderable (GthFileSource *file_source)
{
	return GTH_FILE_SOURCE_GET_CLASS (G_OBJECT (file_source))->is_reorderable (file_source);
}


void
gth_file_source_reorder (GthFileSource *file_source,
			 GthFileData   *destination,
		         GList         *visible_files, /* GFile list */
		         GList         *files_to_move, /* GFile list */
		         int            dest_pos,
		         ReadyCallback  callback,
		         gpointer       data)
{
	if (gth_file_source_is_active (file_source)) {
		gth_file_source_queue_reorder (file_source, destination, visible_files, files_to_move, dest_pos, callback, data);
		return;
	}
	g_cancellable_reset (file_source->priv->cancellable);
	GTH_FILE_SOURCE_GET_CLASS (G_OBJECT (file_source))->reorder (file_source, destination, visible_files, files_to_move, dest_pos, callback, data);
}


void
gth_file_source_remove (GthFileSource *file_source,
			GthFileData   *location,
		        GList         *file_list, /* GthFileData list */
		        gboolean       permanently,
		        GtkWindow     *parent)
{
	if (gth_file_source_is_active (file_source)) {
		gth_file_source_queue_remove (file_source, location, file_list, permanently, parent);
		return;
	}
	g_cancellable_reset (file_source->priv->cancellable);
	GTH_FILE_SOURCE_GET_CLASS (G_OBJECT (file_source))->remove (file_source, location, file_list, permanently, parent);
}


void
gth_file_source_deleted_from_disk (GthFileSource *file_source,
				   GthFileData   *location,
				   GList         *file_list /* GFile list */)
{
	if (gth_file_source_is_active (file_source)) {
		gth_file_source_queue_deleted_from_disk (file_source, location, file_list);
		return;
	}
	g_cancellable_reset (file_source->priv->cancellable);
	GTH_FILE_SOURCE_GET_CLASS (G_OBJECT (file_source))->deleted_from_disk (file_source, location, file_list);
}


void
gth_file_source_get_free_space (GthFileSource      *file_source,
				GFile              *location,
				SpaceReadyCallback  callback,
				gpointer            data)
{
	g_return_if_fail (location != NULL);
	g_return_if_fail (callback != NULL);

	if (gth_file_source_is_active (file_source)) {
		gth_file_source_queue_get_free_space (file_source, location, callback, data);
		return;
	}
	g_cancellable_reset (file_source->priv->cancellable);
	GTH_FILE_SOURCE_GET_CLASS (G_OBJECT (file_source))->get_free_space (file_source, location, callback, data);
}


gboolean
gth_file_source_shows_extra_widget (GthFileSource *file_source)
{
	return GTH_FILE_SOURCE_GET_CLASS (G_OBJECT (file_source))->shows_extra_widget (file_source);
}


GdkDragAction
gth_file_source_get_drop_actions (GthFileSource *file_source,
				  GFile         *destination,
				  GFile         *file)
{
	return GTH_FILE_SOURCE_GET_CLASS (G_OBJECT (file_source))->get_drop_actions (file_source, destination, file);
}
