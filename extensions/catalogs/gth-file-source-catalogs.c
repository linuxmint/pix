/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2008 Free Software Foundation, Inc.
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
#include "gth-catalog.h"
#include "gth-file-source-catalogs.h"


struct _GthFileSourceCatalogsPrivate {
	GList        *files;
	GthCatalog   *catalog;
	ListReady     ready_func;
	gpointer      ready_data;
};


G_DEFINE_TYPE_WITH_CODE (GthFileSourceCatalogs,
			 gth_file_source_catalogs,
			 GTH_TYPE_FILE_SOURCE,
			 G_ADD_PRIVATE (GthFileSourceCatalogs))


static GList *
get_entry_points (GthFileSource *file_source)
{
	GList       *list = NULL;
	GFile       *file;
	GFileInfo   *info;

	file = g_file_new_for_uri ("catalog:///");
	info = gth_file_source_get_file_info (file_source, file, GFILE_BASIC_ATTRIBUTES);
	list = g_list_append (list, gth_file_data_new (file, info));

	g_object_unref (info);
	g_object_unref (file);

	return list;
}


static GFile *
gth_file_source_catalogs_to_gio_file (GthFileSource *file_source,
				      GFile         *file)
{
	return gth_catalog_file_to_gio_file (file);
}


static void
update_file_info (GthFileSource *file_source,
		  GFile         *catalog_file,
		  GFileInfo     *info)
{
	char  *uri;
	GIcon *icon = NULL;

	uri = g_file_get_uri (catalog_file);

	if (g_str_has_suffix (uri, ".gqv") || g_str_has_suffix (uri, ".catalog")) {
		g_file_info_set_file_type (info, G_FILE_TYPE_DIRECTORY);
		g_file_info_set_content_type (info, "gthumb/catalog");
		icon = g_themed_icon_new ("file-catalog-symbolic");
		g_file_info_set_symbolic_icon (info, icon);
		g_file_info_set_sort_order (info, 1);
		g_file_info_set_attribute_boolean (info, "gthumb::no-child", TRUE);
		gth_catalog_update_standard_attributes (catalog_file, info);
	}
	else if (g_str_has_suffix (uri, ".search")) {
		g_file_info_set_file_type (info, G_FILE_TYPE_DIRECTORY);
		g_file_info_set_content_type (info, "gthumb/search");
		icon = g_themed_icon_new ("file-search-symbolic");
		g_file_info_set_symbolic_icon (info, icon);
		g_file_info_set_sort_order (info, 1);
		g_file_info_set_attribute_boolean (info, "gthumb::no-child", TRUE);
		gth_catalog_update_standard_attributes (catalog_file, info);
	}
	else {
		g_file_info_set_file_type (info, G_FILE_TYPE_DIRECTORY);
		g_file_info_set_content_type (info, "gthumb/library");
		icon = g_themed_icon_new ("file-library-symbolic");
		g_file_info_set_symbolic_icon (info, icon);
		g_file_info_set_sort_order (info, 0);
		g_file_info_set_attribute_boolean (info, "gthumb::no-child", FALSE);
		gth_catalog_update_standard_attributes (catalog_file, info);
	}

	_g_object_unref (icon);
	g_free (uri);
}


static GFileInfo *
gth_file_source_catalogs_get_file_info (GthFileSource *file_source,
					GFile         *file,
					const char    *attributes)
{
	GFile     *gio_file;
	GFileInfo *file_info;

	gio_file = gth_catalog_file_to_gio_file (file);
	file_info = g_file_query_info (gio_file,
				       attributes,
				       G_FILE_QUERY_INFO_NONE,
				       NULL,
				       NULL);
	if (file_info == NULL)
		file_info = g_file_info_new ();
	update_file_info (file_source, file, file_info);

	g_object_unref (gio_file);

	return file_info;
}


static GthFileData *
gth_file_source_catalogs_get_file_data (GthFileSource *file_source,
					GFile         *file,
					GFileInfo     *info)
{
	GthFileData *file_data = NULL;
	char        *uri;
	GFile       *catalog_file;

	uri = g_file_get_uri (file);

	switch (g_file_info_get_file_type (info)) {
	case G_FILE_TYPE_REGULAR:
		if (! g_str_has_suffix (uri, ".gqv")
		    && ! g_str_has_suffix (uri, ".catalog")
		    && ! g_str_has_suffix (uri, ".search"))
		{
			file_data = gth_file_data_new (file, info);
			break;
		}

		catalog_file = gth_catalog_file_from_gio_file (file, NULL);
		update_file_info (file_source, catalog_file, info);
		file_data = gth_file_data_new (catalog_file, info);

		g_object_unref (catalog_file);
		break;

	case G_FILE_TYPE_DIRECTORY:
		catalog_file = gth_catalog_file_from_gio_file (file, NULL);
		update_file_info (file_source, catalog_file, info);
		file_data = gth_file_data_new (catalog_file, info);

		g_object_unref (catalog_file);
		break;

	default:
		break;
	}

	g_free (uri);

	return file_data;
}


/* -- gth_file_source_catalogs_write_metadata -- */


typedef struct {
	GthFileSourceCatalogs *file_souce;
	GthFileData           *file_data;
	char                  *attributes;
	ReadyCallback          ready_callback;
	gpointer               user_data;
	GFile                 *gio_file;
} MetadataOpData;


static void
metadata_op_free (MetadataOpData *metadata_op)
{
	gth_file_source_set_active (GTH_FILE_SOURCE (metadata_op->file_souce), FALSE);
	g_object_unref (metadata_op->file_data);
	g_free (metadata_op->attributes);
	g_object_unref (metadata_op->gio_file);
	g_object_unref (metadata_op->file_souce);
	g_free (metadata_op);
}


static void
write_metadata_write_buffer_ready_cb (void     **buffer,
				      gsize      count,
				      GError    *error,
				      gpointer   user_data)
{
	MetadataOpData        *metadata_op = user_data;
	GthFileSourceCatalogs *catalogs = metadata_op->file_souce;

	metadata_op->ready_callback (G_OBJECT (catalogs), error, metadata_op->user_data);
	metadata_op_free (metadata_op);
}


static void
write_metadata_load_buffer_ready_cb (void     **buffer,
				     gsize      count,
				     GError    *error,
				     gpointer   user_data)
{
	MetadataOpData *metadata_op = user_data;
	GthCatalog     *catalog;
	void           *catalog_buffer;
	gsize           catalog_size;

	if (error != NULL) {
		metadata_op->ready_callback (G_OBJECT (metadata_op->file_souce), error, metadata_op->user_data);
		metadata_op_free (metadata_op);
		return;
	}

	catalog = gth_catalog_new_from_data (*buffer, count, &error);
	if (catalog == NULL) {
		metadata_op->ready_callback (G_OBJECT (metadata_op->file_souce), error, metadata_op->user_data);
		metadata_op_free (metadata_op);
		return;
	}

	gth_catalog_set_file (catalog, metadata_op->gio_file);

	if (error != NULL) {
		metadata_op->ready_callback (G_OBJECT (metadata_op->file_souce), error, metadata_op->user_data);
		g_object_unref (catalog);
		metadata_op_free (metadata_op);
		return;
	}

	if (_g_file_attributes_matches_any (metadata_op->attributes, "sort::*"))
		gth_catalog_set_order (catalog,
				       g_file_info_get_attribute_string (metadata_op->file_data->info, "sort::type"),
				       g_file_info_get_attribute_boolean (metadata_op->file_data->info, "sort::inverse"));

	gth_hook_invoke ("gth-catalog-read-metadata", catalog, metadata_op->file_data);

	catalog_buffer = gth_catalog_to_data (catalog, &catalog_size);
	_g_file_write_async (metadata_op->gio_file,
			     catalog_buffer,
			     catalog_size,
			     TRUE,
			     G_PRIORITY_DEFAULT,
			     gth_file_source_get_cancellable (GTH_FILE_SOURCE (metadata_op->file_souce)),
			     write_metadata_write_buffer_ready_cb,
			     metadata_op);

	g_object_unref (catalog);
}


static void
gth_file_source_catalogs_write_metadata (GthFileSource *file_source,
					 GthFileData   *file_data,
					 const char    *attributes,
					 ReadyCallback  callback,
					 gpointer       user_data)
{
	GthFileSourceCatalogs *catalogs = (GthFileSourceCatalogs *) file_source;
	char                  *uri;
	MetadataOpData        *metadata_op;

	uri = g_file_get_uri (file_data->file);
	if (! g_str_has_suffix (uri, ".gqv")
	    && ! g_str_has_suffix (uri, ".catalog")
	    && ! g_str_has_suffix (uri, ".search"))
	{
		g_free (uri);
		object_ready_with_error (file_source, callback, user_data, NULL);
		return;
	}

	metadata_op = g_new0 (MetadataOpData, 1);
	metadata_op->file_souce = g_object_ref (catalogs);
	metadata_op->file_data = g_object_ref (file_data);
	metadata_op->attributes = g_strdup (attributes);
	metadata_op->ready_callback = callback;
	metadata_op->user_data = user_data;

	gth_file_source_set_active (GTH_FILE_SOURCE (catalogs), TRUE);
	g_cancellable_reset (gth_file_source_get_cancellable (file_source));

	metadata_op->gio_file = gth_file_source_to_gio_file (file_source, file_data->file);
	_g_file_load_async (metadata_op->gio_file,
			    G_PRIORITY_DEFAULT,
			    gth_file_source_get_cancellable (file_source),
			    write_metadata_load_buffer_ready_cb,
			    metadata_op);

	g_free (uri);
}


/* -- gth_file_source_catalogs_read_metadata -- */


typedef struct {
	GthFileSource *file_source;
	GthFileData   *file_data;
	char          *attributes;
	ReadyCallback  callback;
	gpointer       data;
} ReadMetadataOpData;


static void
read_metadata_free (ReadMetadataOpData *read_metadata)
{
	g_object_unref (read_metadata->file_source);
	g_object_unref (read_metadata->file_data);
	g_free (read_metadata->attributes);
	g_free (read_metadata);
}


static void
read_metadata_catalog_ready_cb (GObject  *object,
				GError   *error,
				gpointer  user_data)
{
	ReadMetadataOpData *read_metadata = user_data;

	if (error != NULL) {
		/* ignore errors */
		g_clear_error (&error);
	}
	else {
		g_assert (object != NULL);
		gth_catalog_update_metadata (GTH_CATALOG (object), read_metadata->file_data);
		g_object_unref (object);
	}

	read_metadata->callback (G_OBJECT (read_metadata->file_source), error, read_metadata->data);
	read_metadata_free (read_metadata);
}


static void
read_metadata_info_ready_cb (GList    *files,
			     GError   *error,
			     gpointer  user_data)
{
	ReadMetadataOpData *read_metadata = user_data;
	GthFileData        *result;
	GFile              *gio_file;

	if (error != NULL) {
		read_metadata->callback (G_OBJECT (read_metadata->file_source), error, read_metadata->data);
		read_metadata_free (read_metadata);
		return;
	}

	result = files->data;
	g_file_info_copy_into (result->info, read_metadata->file_data->info);
	update_file_info (read_metadata->file_source, read_metadata->file_data->file, read_metadata->file_data->info);

	gio_file = gth_catalog_file_to_gio_file (read_metadata->file_data->file);
	gth_catalog_load_from_file_async (gio_file,
					  gth_file_source_get_cancellable (read_metadata->file_source),
					  read_metadata_catalog_ready_cb,
					  read_metadata);

	g_object_unref (gio_file);
}


static void
gth_file_source_catalogs_read_metadata (GthFileSource *file_source,
					GthFileData   *file_data,
					const char    *attributes,
					ReadyCallback  callback,
					gpointer       data)
{
	ReadMetadataOpData *read_metadata;
	GFile              *gio_file;
	GList              *files;

	read_metadata = g_new0 (ReadMetadataOpData, 1);
	read_metadata->file_source = g_object_ref (file_source);
	read_metadata->file_data = g_object_ref (file_data);
	read_metadata->attributes = g_strdup (attributes);
	read_metadata->callback = callback;
	read_metadata->data = data;

	gio_file = gth_catalog_file_to_gio_file (file_data->file);
	files = g_list_prepend (NULL, gio_file);
	_g_query_all_metadata_async (files,
				     GTH_LIST_DEFAULT,
				     attributes,
				     gth_file_source_get_cancellable (file_source),
				     read_metadata_info_ready_cb,
				     read_metadata);

	_g_object_list_unref (files);
}


static void
gth_file_source_catalogs_rename (GthFileSource *file_source,
				 GFile         *file,
				 const char    *edit_name,
				 ReadyCallback  callback,
				 gpointer       data)
{
	GFile      *parent;
	GFile      *new_file;
	GthCatalog *catalog;
	GError     *error = NULL;

	parent = g_file_get_parent (file);

	catalog = gth_catalog_load_from_file (file);
	if (catalog != NULL) {
		char  *uri;
		char  *clean_name;
		char  *ext;
		char  *name;
		GFile *gio_new_file;
		char  *data;
		gsize  size;
		GFileOutputStream *fstream;

		uri = g_file_get_uri (file);
		clean_name = _g_filename_clear_for_file (edit_name);
		ext = _g_uri_get_extension (uri);
		name = g_strconcat (clean_name, ext, NULL);
		new_file = g_file_get_child_for_display_name (parent, name, &error);
		gth_catalog_set_file (catalog, new_file);
		gth_catalog_set_name (catalog, edit_name);

		gio_new_file = gth_catalog_file_to_gio_file (new_file);
		data = gth_catalog_to_data (catalog, &size);
		fstream = g_file_create (gio_new_file,
					 G_FILE_CREATE_NONE,
					 gth_file_source_get_cancellable (file_source),
					 &error);
		if (fstream != NULL) {
		     if (g_output_stream_write_all (G_OUTPUT_STREAM (fstream),
						    data,
						    size,
						    NULL,
						    gth_file_source_get_cancellable (file_source),
						    &error))
		     {
			     GFile *gio_old_file;

			     gio_old_file = gth_catalog_file_to_gio_file (file);
			     if (g_file_delete (gio_old_file, gth_file_source_get_cancellable (file_source), &error))
				     gth_monitor_file_renamed (gth_main_get_default_monitor (), file, new_file);

			     g_object_unref (gio_old_file);
		     }

		     g_object_unref (fstream);
		}

		g_free (data);
		g_object_unref (gio_new_file);
		g_free (clean_name);
		g_free (ext);
		g_free (name);
		g_free (uri);
	}
	else {
		new_file = g_file_get_child_for_display_name (parent, edit_name, &error);
		if (new_file != NULL) {
			GFile *gio_file;
			GFile *gio_new_file;

			gio_file = gth_file_source_to_gio_file (file_source, file);
			gio_new_file = gth_file_source_to_gio_file (file_source, new_file);

			if (g_file_move (gio_file,
					 gio_new_file,
					 0,
					 gth_file_source_get_cancellable (file_source),
					 NULL,
					 NULL,
					 &error))
			{
				gth_monitor_file_renamed (gth_main_get_default_monitor (), file, new_file);
			}

			g_object_unref (gio_new_file);
			g_object_unref (gio_file);
		}
	}

	if (g_error_matches (error, G_IO_ERROR, G_IO_ERROR_EXISTS)) {
		g_clear_error (&error);
		error = g_error_new_literal (G_IO_ERROR, G_IO_ERROR_EXISTS, _("Name already used"));
	}

	object_ready_with_error (file_source, callback, data, error);

	_g_object_unref (new_file);
	_g_object_unref (catalog);
}


/* -- gth_file_source_catalogs_for_each_child -- */


typedef struct {
	GthFileSource         *file_source;
	gboolean               recursive;
	char                  *attributes;
	StartDirCallback       start_dir_func;
	ForEachChildCallback   for_each_file_func;
	ReadyCallback          ready_func;
	gpointer               user_data;
	GList                 *to_visit;
} ForEachChildData;


static void
for_each_child_data_free (ForEachChildData *data)
{
	_g_object_list_unref (data->to_visit);
	g_free (data->attributes);
	g_object_ref (data->file_source);
}


static void
for_each_child_data_done (ForEachChildData *data,
			  GError           *error)
{
	gth_file_source_set_active (data->file_source, FALSE);
	data->ready_func (G_OBJECT (data->file_source), error, data->user_data);

	for_each_child_data_free (data);
}


static void for_each_child__visit_file (ForEachChildData *data,
				        GthFileData      *library);


static void
for_each_child__continue (ForEachChildData *data)
{
	GthFileData *file;
	GList       *tmp;

	if (! data->recursive || (data->to_visit == NULL)) {
		for_each_child_data_done (data, NULL);
		return;
	}

	file = data->to_visit->data;
	tmp = data->to_visit;
	data->to_visit = g_list_remove_link (data->to_visit, tmp);
	g_list_free (tmp);

	for_each_child__visit_file (data, file);

	g_object_unref (file);
}


static void
for_each_child__done_func (GError   *error,
			   gpointer  user_data)
{
	for_each_child__continue ((ForEachChildData *) user_data);
}


static void
for_each_child__for_each_file_func (GFile     *file,
				    GFileInfo *info,
				    gpointer   user_data)
{
	ForEachChildData *data = user_data;
	GthFileData      *file_data;

	file_data = gth_file_source_get_file_data (data->file_source, file, info);
	if (file_data == NULL)
		return;

	data->for_each_file_func (file_data->file, file_data->info, data->user_data);

	if (data->recursive && (g_file_info_get_file_type (file_data->info) == G_FILE_TYPE_DIRECTORY))
		data->to_visit = g_list_append (data->to_visit, g_object_ref (file_data));

	g_object_unref (file_data);
}


static DirOp
for_each_child__start_dir_func (GFile       *directory,
				GFileInfo   *info,
				GError     **error,
				gpointer     user_data)
{
	if (g_file_info_get_is_hidden (info))
		return DIR_OP_SKIP;
	else
		return DIR_OP_CONTINUE;
}


static void
for_each_child__catalog_list_ready_cb (GthCatalog *catalog,
				       GList      *files,
				       GError     *error,
				       gpointer    user_data)
{
	ForEachChildData *data = user_data;
	GList            *scan;

	for (scan = files; scan; scan = scan->next) {
		GthFileData *file_data = scan->data;

		if (g_file_info_get_is_hidden (file_data->info))
			continue;

		data->for_each_file_func (file_data->file,
					  file_data->info,
					  data->user_data);
	}

	for_each_child__continue (data);
}


static void
for_each_child__visit_file (ForEachChildData *data,
			    GthFileData      *file_data)
{
	GFile *gio_file;
	char  *uri;

	if (data->start_dir_func != NULL) {
		GError *error = NULL;

		switch (data->start_dir_func (file_data->file, file_data->info, &error, data->user_data)) {
		case DIR_OP_CONTINUE:
			break;
		case DIR_OP_SKIP:
			for_each_child__continue (data);
			return;
		case DIR_OP_STOP:
			for_each_child_data_done (data, NULL);
			return;
		}
	}

	gio_file = gth_file_source_to_gio_file (data->file_source, file_data->file);
	uri = g_file_get_uri (file_data->file);
	if (g_str_has_suffix (uri, ".gqv")
	    || g_str_has_suffix (uri, ".catalog")
	    || g_str_has_suffix (uri, ".search"))
	{
		gth_catalog_list_async (gio_file,
					data->attributes,
					gth_file_source_get_cancellable (data->file_source),
					for_each_child__catalog_list_ready_cb,
					data);
	}
	else
		_g_directory_foreach_child (gio_file,
					   FALSE,
					   TRUE,
					   GFILE_STANDARD_ATTRIBUTES_WITH_FAST_CONTENT_TYPE,
					   gth_file_source_get_cancellable (data->file_source),
					   for_each_child__start_dir_func,
					   for_each_child__for_each_file_func,
					   for_each_child__done_func,
					   data);

	g_object_unref (gio_file);
	g_free (uri);
}


static void
for_each_child__parent_info_ready_cb (GObject      *source_object,
				      GAsyncResult *result,
				      gpointer      user_data)
{
	ForEachChildData *data = user_data;
	GFile            *file;
	GFileInfo        *info;
	GError           *error = NULL;
	GthFileData      *file_data;

	file = G_FILE (source_object);
	info = g_file_query_info_finish (file, result, &error);
	if (info == NULL) {
		for_each_child_data_done (data, error);
		return;
	}

	file_data = gth_file_source_get_file_data (data->file_source, file, info);
	for_each_child__visit_file (data, file_data);

	g_object_unref (file_data);
}


static void
gth_file_source_catalogs_for_each_child (GthFileSource        *file_source,
					 GFile                *parent,
					 gboolean              recursive,
					 const char           *attributes,
					 StartDirCallback      start_dir_func,
					 ForEachChildCallback  for_each_file_func,
					 ReadyCallback         ready_func,
					 gpointer              user_data)
{
	ForEachChildData *data;
	GFile            *gio_parent;

	data = g_new0 (ForEachChildData, 1);
	data->file_source = g_object_ref (file_source);
	data->recursive = recursive;
	data->attributes = g_strdup (attributes);
	data->start_dir_func = start_dir_func;
	data->for_each_file_func = for_each_file_func;
	data->ready_func = ready_func;
	data->user_data = user_data;

	gio_parent = gth_file_source_to_gio_file (file_source, parent);
	g_file_query_info_async (gio_parent,
			         GFILE_STANDARD_ATTRIBUTES_WITH_FAST_CONTENT_TYPE,
				 G_FILE_QUERY_INFO_NONE,
				 G_PRIORITY_DEFAULT,
				 gth_file_source_get_cancellable (data->file_source),
				 for_each_child__parent_info_ready_cb,
				 data);

	g_object_unref (gio_parent);
}


/* -- gth_file_source_catalogs_copy -- */


typedef struct {
	GthFileSource    *file_source;
	GthFileData      *destination;
	GList            *file_list;
	int               destination_position;
	ProgressCallback  progress_callback;
	DialogCallback    dialog_callback;
	ReadyCallback     ready_callback;
	gpointer          user_data;
	GList            *files;
	GthCatalog       *catalog;
} CopyOpData;


static void
copy_op_data_free (CopyOpData *cod)
{
	_g_object_unref (cod->catalog);
	_g_object_list_unref (cod->files);
	_g_object_list_unref (cod->file_list);
	g_object_unref (cod->destination);
	g_object_unref (cod->file_source);
	g_free (cod);
}


static void
copy__catalog_save_done_cb (void     **buffer,
			    gsize      count,
			    GError    *error,
			    gpointer   user_data)
{
	CopyOpData *cod = user_data;

	if (error == NULL) {
		gth_monitor_files_created_with_pos (gth_main_get_default_monitor (),
						    cod->destination->file,
						    cod->files,
						    cod->destination_position);
	}

	cod->ready_callback (G_OBJECT (cod->file_source), error, cod->user_data);
	copy_op_data_free (cod);
}


static void
catalog_ready_cb (GObject  *catalog,
		  GError   *error,
		  gpointer  user_data)
{
	CopyOpData *cod = user_data;
	int         position;
	GList      *scan;
	char       *buffer;
	gsize       size;
	GFile      *gio_file;

	if (error != NULL) {
		cod->ready_callback (G_OBJECT (cod->file_source), error, cod->user_data);
		copy_op_data_free (cod);
		return;
	}

	g_assert (catalog != NULL);
	cod->catalog = (GthCatalog *) catalog;

	if (cod->destination_position >= 0)
		gth_catalog_set_order (cod->catalog, "general::unsorted", FALSE);

	position = cod->destination_position;
	for (scan = cod->files; scan; scan = scan->next) {
		gth_catalog_insert_file (cod->catalog, (GFile *) scan->data, position);
		if (cod->destination_position >= 0) /* always append to the end if destination_position is -1 */
			position += 1;
	}

	buffer = gth_catalog_to_data (cod->catalog, &size);
	gio_file = gth_catalog_file_to_gio_file (cod->destination->file);
	_g_file_write_async (gio_file,
			     buffer,
			     size,
			     TRUE,
			     G_PRIORITY_DEFAULT,
			     NULL,
			     copy__catalog_save_done_cb,
			     cod);

	g_object_unref (gio_file);
}


static void
copy__file_list_info_ready_cb (GList    *files,
			       GError   *error,
			       gpointer  user_data)
{
	CopyOpData *cod = user_data;
	GList      *scan;

	for (scan = files; scan; scan = scan->next) {
		GthFileData *file_data = scan->data;

		switch (g_file_info_get_file_type (file_data->info)) {
		case G_FILE_TYPE_REGULAR:
		case G_FILE_TYPE_SYMBOLIC_LINK:
			cod->files = g_list_prepend (cod->files, g_object_ref (file_data->file));
			break;
		default:
			break;
		}
	}
	cod->files = g_list_reverse (cod->files);

	gth_catalog_load_from_file_async (cod->destination->file,
					  gth_file_source_get_cancellable (cod->file_source),
					  catalog_ready_cb,
					  cod);
}


/* -- _gth_file_source_catalogs_copy_catalog -- */


typedef struct {
	GthFileSource    *file_source;
	gboolean          move;
	ProgressCallback  progress_callback;
	DialogCallback    dialog_callback;
	ReadyCallback     ready_callback;
	gpointer          user_data;
	GthFileData      *destination;
	GList            *file_list;
} CopyCatalogData;


static void
copy_catalog_data_free (CopyCatalogData *ccd)
{
	_g_object_list_unref (ccd->file_list);
	_g_object_unref (ccd->destination);
	_g_object_unref (ccd->file_source);
	g_free (ccd);
}


static void
_gth_file_source_catalogs_copy_catalog (CopyCatalogData      *ccd,
					GthOverwriteResponse  default_response);


static void
copy_catalog_overwrite_dialog_response_cb (GtkDialog *dialog,
					   int        response,
					   gpointer   user_data)
{
	CopyCatalogData *ccd = user_data;

	gtk_widget_destroy (GTK_WIDGET (dialog));

	if (response == GTK_RESPONSE_OK) {
		_gth_file_source_catalogs_copy_catalog (ccd, GTH_OVERWRITE_RESPONSE_ALWAYS_YES);
		return;
	}

	ccd->ready_callback (G_OBJECT (ccd->file_source), NULL, ccd->user_data);

	copy_catalog_data_free (ccd);
}


static void
copy_catalog_ready_cb (GError   *error,
		       gpointer  user_data)
{
	CopyCatalogData *ccd = user_data;
	GFile           *first_file;
	GFile           *parent;
	GList           *new_file_list;
	GList           *scan;

	first_file = ccd->file_list->data;

	if (g_error_matches (error, G_IO_ERROR, G_IO_ERROR_EXISTS)) {
		char      *uri;
		char      *extension;
		char      *msg;
		GtkWidget *d;

		uri = g_file_get_uri (first_file);
		extension = _g_uri_get_extension (uri);
		if ((g_strcmp0 (extension, ".catalog") == 0) || (g_strcmp0 (extension, ".search") == 0))
			msg = g_strdup_printf (_("The catalog “%s” already exists, do you want to overwrite it?"), g_file_info_get_display_name (ccd->destination->info));
		else
			msg = g_strdup_printf (_("The library “%s” already exists, do you want to overwrite it?"), g_file_info_get_display_name (ccd->destination->info));

		d = _gtk_message_dialog_new (NULL,
					     GTK_DIALOG_MODAL,
					     _GTK_ICON_NAME_DIALOG_QUESTION,
					     msg,
					     NULL,
					     _GTK_LABEL_CANCEL, GTK_RESPONSE_CANCEL,
					     _("Over_write"), GTK_RESPONSE_OK,
					     NULL);
		g_signal_connect (d,
				  "response",
				  G_CALLBACK (copy_catalog_overwrite_dialog_response_cb),
				  ccd);
		ccd->dialog_callback (TRUE, d, ccd->user_data);
		gtk_widget_show (d);

		g_free (msg);
		g_free (extension);
		g_free (uri);

		return;
	}

	parent = g_file_get_parent (first_file);
	if (parent != NULL) {
		gth_monitor_folder_changed (gth_main_get_default_monitor (),
					    parent,
					    ccd->file_list,
					    GTH_MONITOR_EVENT_DELETED);
		g_object_unref (parent);
	}

	new_file_list = NULL;
	for (scan = ccd->file_list; scan; scan = scan->next) {
		GFile *old_file = scan->data;
		char  *basename;
		GFile *new_file;

		basename = g_file_get_basename (old_file);
		new_file = g_file_get_child (ccd->destination->file, basename);
		new_file_list = g_list_prepend (new_file_list, new_file);
		g_free (basename);
	}
	new_file_list = g_list_reverse (new_file_list);
	gth_monitor_folder_changed (gth_main_get_default_monitor (),
			            ccd->destination->file,
			            new_file_list,
			            GTH_MONITOR_EVENT_CREATED);

	ccd->ready_callback (G_OBJECT (ccd->file_source), error, ccd->user_data);

	_g_object_list_unref (new_file_list);
	copy_catalog_data_free (ccd);
}


static void
_gth_file_source_catalogs_copy_catalog (CopyCatalogData      *ccd,
					GthOverwriteResponse  default_response)
{
	GList *gio_list;
	GFile *gio_destination;

	gio_list = gth_file_source_to_gio_file_list (ccd->file_source, ccd->file_list);
	gio_destination = gth_file_source_to_gio_file (ccd->file_source, ccd->destination->file);

	_g_file_list_copy_async (gio_list,
				 gio_destination,
				 ccd->move,
				 GTH_FILE_COPY_DEFAULT,
				 default_response,
				 G_PRIORITY_DEFAULT,
				 gth_file_source_get_cancellable (ccd->file_source),
				 ccd->progress_callback,
				 ccd->user_data,
				 ccd->dialog_callback,
				 ccd->user_data,
				 copy_catalog_ready_cb,
				 ccd);

	g_object_unref (gio_destination);
	_g_object_list_unref (gio_list);
}


static void
copy_catalog_error_dialog_response_cb (GtkDialog *dialog,
				       int        response,
				       gpointer   user_data)
{
	CopyCatalogData *ccd = user_data;

	gtk_widget_destroy (GTK_WIDGET (dialog));
	ccd->dialog_callback (FALSE, NULL, ccd->user_data);
	ccd->ready_callback (G_OBJECT (ccd->file_source), NULL, ccd->user_data);
	copy_catalog_data_free (ccd);
}


static void
gth_file_source_catalogs_copy (GthFileSource    *file_source,
			       GthFileData      *destination,
			       GList            *file_list, /* GFile * list */
			       gboolean          move,
			       int               destination_position,
			       ProgressCallback  progress_callback,
			       DialogCallback    dialog_callback,
			       ReadyCallback     ready_callback,
			       gpointer          data)
{
	GFile *first_file;

	first_file = file_list->data;
	if (g_file_has_uri_scheme (first_file, "catalog")) {
		if (g_strcmp0 (g_file_info_get_content_type (destination->info), "gthumb/catalog") == 0) {
			CopyCatalogData *ccd;
			const char      *msg;
			GtkWidget       *d;

			ccd = g_new0 (CopyCatalogData, 1);
			ccd->file_source = g_object_ref (file_source);
			ccd->dialog_callback = dialog_callback;
			ccd->ready_callback = ready_callback;
			ccd->user_data = data;

			if (move)
				msg = _("Cannot move the files");
			else
				msg = _("Cannot copy the files");
			d = _gtk_message_dialog_new (NULL,
						     GTK_DIALOG_MODAL,
						     _GTK_ICON_NAME_DIALOG_ERROR,
						     msg,
						     _("Invalid destination."),
						     _GTK_LABEL_CLOSE, GTK_RESPONSE_CLOSE,
						     NULL);
			g_signal_connect (d,
					  "response",
					  G_CALLBACK (copy_catalog_error_dialog_response_cb),
					  ccd);
			dialog_callback (TRUE, d, data);
			gtk_widget_show (d);

			return;
		}
		else {
			/* copy / move a catalog or library into another library */

			CopyCatalogData *ccd;

			ccd = g_new0 (CopyCatalogData, 1);
			ccd->file_source = g_object_ref (file_source);
			ccd->destination = gth_file_data_dup (destination);
			ccd->file_list = _g_object_list_ref (file_list);
			ccd->move = move;
			ccd->progress_callback = progress_callback;
			ccd->dialog_callback = dialog_callback;
			ccd->ready_callback = ready_callback;
			ccd->user_data = data;
			_gth_file_source_catalogs_copy_catalog (ccd, GTH_OVERWRITE_RESPONSE_ALWAYS_NO);
		}
	}
	else {
		/* copy / move files to a catalog */

		CopyOpData *cod;

		cod = g_new0 (CopyOpData, 1);
		cod->file_source = g_object_ref (file_source);
		cod->destination = g_object_ref (destination);
		cod->file_list = _g_object_list_ref (file_list);
		cod->destination_position = destination_position;
		cod->progress_callback = progress_callback;
		cod->dialog_callback = dialog_callback;
		cod->ready_callback = ready_callback;
		cod->user_data = data;

		if (cod->progress_callback != NULL) {
			char *message;

			message = g_strdup_printf (_("Copying files to “%s”"), g_file_info_get_display_name (destination->info));
			(cod->progress_callback) (G_OBJECT (file_source), message, NULL, TRUE, 0.0, cod->user_data);

			g_free (message);
		}

		_g_file_list_query_info_async (cod->file_list,
					       GTH_LIST_DEFAULT,
					       GFILE_NAME_TYPE_ATTRIBUTES,
					       gth_file_source_get_cancellable (file_source),
					       copy__file_list_info_ready_cb,
					       cod);
	}
}


static gboolean
gth_file_source_catalogs_is_reorderable (GthFileSource *file_source)
{
	return TRUE;
}


typedef struct {
	GthFileSource *file_source;
	GthFileData   *destination;
	GList         *visible_files; /* GFile list */
	GList         *files_to_move; /* GFile list */
	int            dest_pos;
	ReadyCallback  callback;
	gpointer       data;
	int           *new_order;
} ReorderData;


static void
reorder_data_free (ReorderData *reorder_data)
{
	gth_file_source_set_active (reorder_data->file_source, FALSE);
	_g_object_list_unref (reorder_data->visible_files);
	_g_object_list_unref (reorder_data->files_to_move);
	_g_object_unref (reorder_data->destination);
	_g_object_unref (reorder_data->file_source);
	g_free (reorder_data->new_order);
	g_free (reorder_data);
}


static void
reorder_buffer_ready_cb (void     **buffer,
		         gsize      count,
		         GError    *error,
		         gpointer   user_data)
{
	ReorderData *reorder_data = user_data;

	gth_monitor_order_changed (gth_main_get_default_monitor (),
				   reorder_data->destination->file,
				   reorder_data->new_order);
	reorder_data->callback (G_OBJECT (reorder_data->file_source), error, reorder_data->data);

	reorder_data_free (reorder_data);
}


static int *
reorder_catalog_list (GthCatalog *catalog,
		      GList      *visible_files,
		      GList      *files_to_move,
		      int         dest_pos)
{
	int   *new_order;
	GList *new_file_list;

	_g_list_reorder (gth_catalog_get_file_list (catalog),
			 visible_files,
			 files_to_move,
			 dest_pos,
			 &new_order,
			 &new_file_list);
	gth_catalog_set_file_list (catalog, new_file_list);

	_g_object_list_unref (new_file_list);

	return new_order;
}


static void
reorder_catalog_ready_cb (GObject  *object,
			  GError   *error,
			  gpointer  user_data)
{
	ReorderData *reorder_data = user_data;
	GthCatalog  *catalog;
	char        *buffer;
	gsize        size;
	GFile       *gio_file;

	if (error != NULL) {
		reorder_data->callback (G_OBJECT (reorder_data->file_source), error, reorder_data->data);
		reorder_data_free (reorder_data);
		return;
	}

	g_assert (object != NULL);
	catalog = (GthCatalog *) object;
	reorder_data->new_order = reorder_catalog_list (catalog,
							reorder_data->visible_files,
							reorder_data->files_to_move,
							reorder_data->dest_pos);
	gth_catalog_set_order (catalog, "general::unsorted", FALSE);

	buffer = gth_catalog_to_data (catalog, &size);
	gio_file = gth_file_source_to_gio_file (reorder_data->file_source, reorder_data->destination->file);
	_g_file_write_async (gio_file,
			     buffer,
			     size,
			     TRUE,
			     G_PRIORITY_DEFAULT,
			     gth_file_source_get_cancellable (reorder_data->file_source),
			     reorder_buffer_ready_cb,
			     reorder_data);

	g_object_unref (gio_file);
}


static void
gth_file_source_catalogs_reorder (GthFileSource *file_source,
				  GthFileData   *destination,
				  GList         *visible_files, /* GFile list */
				  GList         *files_to_move, /* GFile list */
				  int            dest_pos,
				  ReadyCallback  callback,
				  gpointer       data)
{
	GthFileSourceCatalogs *catalogs = (GthFileSourceCatalogs *) file_source;
	ReorderData           *reorder_data;
	GFile                 *gio_file;

	gth_file_source_set_active (GTH_FILE_SOURCE (catalogs), TRUE);
	g_cancellable_reset (gth_file_source_get_cancellable (file_source));

	reorder_data = g_new0 (ReorderData, 1);
	reorder_data->file_source = g_object_ref (file_source);
	reorder_data->destination = g_object_ref (destination);
	reorder_data->visible_files = _g_object_list_ref (visible_files);
	reorder_data->files_to_move = _g_object_list_ref (files_to_move);
	reorder_data->dest_pos = dest_pos;
	reorder_data->callback = callback;
	reorder_data->data = data;

	gio_file = gth_file_source_to_gio_file (file_source, destination->file);
	gth_catalog_load_from_file_async (gio_file,
					  gth_file_source_get_cancellable (file_source),
					  reorder_catalog_ready_cb,
					  reorder_data);

	g_object_unref (gio_file);
}


/* -- gth_catalog_manager_remove_files -- */


typedef struct {
	GtkWindow  *parent;
	GList      *file_list;
	GFile      *gio_file;
	GthCatalog *catalog;
	gboolean    notify;
} RemoveFromCatalogData;


static void
remove_from_catalog_end (GError                *error,
			 RemoveFromCatalogData *data)
{
	if ((data->catalog != NULL) && (error != NULL))
		_gtk_error_dialog_from_gerror_show (data->parent, _("Could not remove the files from the catalog"), error);

	_g_object_unref (data->catalog);
	_g_object_unref (data->gio_file);
	_g_object_list_unref (data->file_list);
	g_free (data);
}


static void
remove_files__catalog_save_done_cb (void     **buffer,
				    gsize      count,
				    GError    *error,
				    gpointer   user_data)
{
	RemoveFromCatalogData *data = user_data;

	if ((error == NULL) && data->notify) {
		GFile *catalog_file;

		catalog_file = gth_catalog_file_from_gio_file (data->gio_file, NULL);
		gth_monitor_folder_changed (gth_main_get_default_monitor (),
					    catalog_file,
					    data->file_list,
					    GTH_MONITOR_EVENT_REMOVED);

		g_object_unref (catalog_file);
	}

	remove_from_catalog_end (error, data);
}


static void
catalog_buffer_ready_cb (void     **buffer,
			 gsize      count,
			 GError    *error,
			 gpointer   user_data)
{
	RemoveFromCatalogData *data = user_data;
	GList                 *scan;
	void                  *catalog_buffer;
	gsize                  catalog_size;

	if (error != NULL) {
		remove_from_catalog_end (error, data);
		return;
	}

	data->catalog = gth_catalog_new_from_data (*buffer, count, &error);
	if (data->catalog == NULL) {
		remove_from_catalog_end (error, data);
		return;
	}

	for (scan = data->file_list; scan; scan = scan->next) {
		GFile *file = scan->data;
		gth_catalog_remove_file (data->catalog, file);
	}

	catalog_buffer = gth_catalog_to_data (data->catalog, &catalog_size);
	if (error != NULL) {
		remove_from_catalog_end (error, data);
		return;
	}

	_g_file_write_async (data->gio_file,
			     catalog_buffer,
			     catalog_size,
			     TRUE,
			     G_PRIORITY_DEFAULT,
			     NULL,
			     remove_files__catalog_save_done_cb,
			     data);
}


void
gth_catalog_manager_remove_files (GtkWindow   *parent,
				  GthFileData *location,
				  GList       *file_list,
				  gboolean     notify)
{
	RemoveFromCatalogData *data;

	data = g_new0 (RemoveFromCatalogData, 1);
	data->parent = parent;
	data->file_list = _g_file_list_dup (file_list);
	data->gio_file = gth_main_get_gio_file (location->file);
	data->notify = notify;
	data->catalog = NULL;

	_g_file_load_async (data->gio_file,
			    G_PRIORITY_DEFAULT,
			    NULL,
			    catalog_buffer_ready_cb,
			    data);
}


static void
gth_file_source_catalogs_remove (GthFileSource *file_source,
				 GthFileData   *location,
	       	       	         GList         *file_data_list /* GthFileData list */,
	       	       	         gboolean       permanently,
	       	       	         GtkWindow     *parent)
{
	GList *file_list;

	file_list = gth_file_data_list_to_file_list (file_data_list);
	gth_catalog_manager_remove_files (parent, location, file_list, TRUE);

	_g_object_list_unref (file_list);
}


static void
gth_file_source_catalogs_deleted_from_disk (GthFileSource *file_source,
					    GthFileData   *location,
					    GList         *file_list)
{
	gth_catalog_manager_remove_files (NULL, location, file_list, FALSE);
}


static GdkDragAction
gth_file_source_catalogs_get_drop_actions (GthFileSource *file_source,
					   GFile         *destination,
					   GFile         *file)
{
	GdkDragAction  actions = 0;
	char          *dest_uri;
	char          *dest_scheme;
	char          *dest_ext;
	gboolean       dest_is_catalog;
	char          *file_uri;
	char          *file_scheme;
	char          *file_ext;
	gboolean       file_is_catalog;

	dest_uri = g_file_get_uri (destination);
	dest_scheme = gth_main_get_source_scheme (dest_uri);
	dest_ext = _g_uri_get_extension (dest_uri);
	dest_is_catalog = _g_str_equal (dest_ext, ".catalog") || _g_str_equal (dest_ext, ".search");

	file_uri = g_file_get_uri (file);
	file_scheme = gth_main_get_source_scheme (file_uri);
	file_ext = _g_uri_get_extension (file_uri);
	file_is_catalog = _g_str_equal (file_ext, ".catalog") || _g_str_equal (file_ext, ".search");

	if (_g_str_equal (dest_scheme, "catalog")
		&& dest_is_catalog
		&& _g_str_equal (file_scheme, "file"))
	{
		/* Copy files into a catalog. */
		actions = GDK_ACTION_COPY;
	}

	else if (_g_str_equal (file_scheme, "catalog")
		&& file_is_catalog
		&& _g_str_equal (dest_scheme, "catalog")
		&& ! dest_is_catalog)
	{
		/* Move a catalog into a library. */
		actions = GDK_ACTION_MOVE;
	}

	else if (_g_str_equal (file_scheme, "catalog")
		&& ! file_is_catalog
		&& _g_str_equal (dest_scheme, "catalog")
		&& ! dest_is_catalog)
	{
		/* Move a library into another library. */
		actions = GDK_ACTION_MOVE;
	}

	g_free (file_ext);
	g_free (file_scheme);
	g_free (file_uri);
	g_free (dest_ext);
	g_free (dest_scheme);
	g_free (dest_uri);

	return actions;
}


static void
gth_file_source_catalogs_finalize (GObject *object)
{
	GthFileSourceCatalogs *catalogs = GTH_FILE_SOURCE_CATALOGS (object);

	g_object_unref (catalogs->priv->catalog);
	_g_object_list_unref (catalogs->priv->files);
	catalogs->priv->files = NULL;

	G_OBJECT_CLASS (gth_file_source_catalogs_parent_class)->finalize (object);
}


static void
gth_file_source_catalogs_class_init (GthFileSourceCatalogsClass *class)
{
	GObjectClass       *object_class;
	GthFileSourceClass *file_source_class;

	object_class = (GObjectClass*) class;
	object_class->finalize = gth_file_source_catalogs_finalize;

	file_source_class = (GthFileSourceClass*) class;
	file_source_class->get_entry_points = get_entry_points;
	file_source_class->to_gio_file = gth_file_source_catalogs_to_gio_file;
	file_source_class->get_file_info = gth_file_source_catalogs_get_file_info;
	file_source_class->get_file_data = gth_file_source_catalogs_get_file_data;
	file_source_class->write_metadata = gth_file_source_catalogs_write_metadata;
	file_source_class->read_metadata = gth_file_source_catalogs_read_metadata;
	file_source_class->rename = gth_file_source_catalogs_rename;
	file_source_class->for_each_child = gth_file_source_catalogs_for_each_child;
	file_source_class->copy = gth_file_source_catalogs_copy;
	file_source_class->is_reorderable  = gth_file_source_catalogs_is_reorderable;
	file_source_class->reorder = gth_file_source_catalogs_reorder;
	file_source_class->remove = gth_file_source_catalogs_remove;
	file_source_class->deleted_from_disk = gth_file_source_catalogs_deleted_from_disk;
	file_source_class->get_drop_actions = gth_file_source_catalogs_get_drop_actions;
}


static void
gth_file_source_catalogs_init (GthFileSourceCatalogs *catalogs)
{
	gth_file_source_add_scheme (GTH_FILE_SOURCE (catalogs), "catalog");

	catalogs->priv = gth_file_source_catalogs_get_instance_private (catalogs);
	catalogs->priv->files = NULL;
	catalogs->priv->catalog = gth_catalog_new ();
	catalogs->priv->ready_func = NULL;
	catalogs->priv->ready_data = NULL;
}
