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

#include <string.h>
#include <glib.h>
#include <glib/gi18n.h>
#include <gio/gio.h>
#include "gth-file-data.h"
#include "gth-hook.h"
#include "gth-metadata-provider.h"
#include "gth-overwrite-dialog.h"
#include "glib-utils.h"
#include "gio-utils.h"


#define N_FILES_PER_REQUEST 128


/* -- filter -- */


typedef enum {
	FILTER_DEFAULT = 0,
	FILTER_NODOTFILES = 1 << 1,
	FILTER_IGNORECASE = 1 << 2,
	FILTER_NOBACKUPFILES = 1 << 3
} FilterOptions;


typedef struct {
	char           *pattern;
	FilterOptions   options;
	GRegex        **regexps;
} Filter;


static Filter *
filter_new (const char    *pattern,
	    FilterOptions  options)
{
	Filter             *filter;
	GRegexCompileFlags  flags;

	filter = g_new0 (Filter, 1);

	if ((pattern != NULL) && (strcmp (pattern, "*") != 0))
		filter->pattern = g_strdup (pattern);

	filter->options = options;
	if (filter->options & FILTER_IGNORECASE)
		flags = G_REGEX_CASELESS;
	else
		flags = 0;
	filter->regexps = get_regexps_from_pattern (pattern, flags);

	return filter;
}


static void
filter_destroy (Filter *filter)
{
	if (filter == NULL)
		return;

	g_free (filter->pattern);
	if (filter->regexps != NULL)
		free_regexps (filter->regexps);
	g_free (filter);
}


static gboolean
filter_matches (Filter     *filter,
		const char *name)
{
	const char *file_name;
	char       *utf8_name;
	gboolean    matched;

	g_return_val_if_fail (name != NULL, FALSE);

	file_name = _g_uri_get_basename (name);

	if ((filter->options & FILTER_NODOTFILES)
	    && ((file_name[0] == '.') || (strstr (file_name, "/.") != NULL)))
		return FALSE;

	if ((filter->options & FILTER_NOBACKUPFILES)
	    && (file_name[strlen (file_name) - 1] == '~'))
		return FALSE;

	if (filter->pattern == NULL)
		return TRUE;

	utf8_name = g_filename_to_utf8 (file_name, -1, NULL, NULL, NULL);
	matched = string_matches_regexps (filter->regexps, utf8_name, 0);
	g_free (utf8_name);

	return matched;
}


static gboolean
filter_empty (Filter *filter)
{
	return ((filter->pattern == NULL) || (strcmp (filter->pattern, "*") == 0));
}


/* -- g_directory_foreach_child -- */


typedef struct {
	GFile     *file;
	GFileInfo *info;
} ChildData;


static ChildData *
child_data_new (GFile     *file,
		GFileInfo *info)
{
	ChildData *data;

	data = g_new0 (ChildData, 1);
	data->file = g_file_dup (file);
	data->info = g_file_info_dup (info);

	return data;
}


static void
child_data_free (ChildData *data)
{
	if (data == NULL)
		return;
	_g_object_unref (data->file);
	_g_object_unref (data->info);
	g_free (data);
}


static void
clear_child_data (ChildData **data)
{
	if (*data != NULL)
		child_data_free (*data);
	*data = NULL;
}


typedef struct {
	GFile                *base_directory;
	gboolean              recursive;
	gboolean              follow_links;
	StartDirCallback      start_dir_func;
	ForEachChildCallback  for_each_file_func;
	ReadyFunc             done_func;
	gpointer              user_data;

	/* private */

	ChildData            *current;
	GHashTable           *already_visited;
	GList                *to_visit;
	char                 *attributes;
	GCancellable         *cancellable;
	GFileEnumerator      *enumerator;
	GError               *error;
	guint                 source_id;
	gboolean              metadata_attributes;
	GList                *children;
	GList                *current_child;
} ForEachChildData;


static void
for_each_child_data_free (ForEachChildData *fec)
{
	if (fec == NULL)
		return;

	g_object_unref (fec->base_directory);
	if (fec->already_visited != NULL)
		g_hash_table_destroy (fec->already_visited);
	clear_child_data (&(fec->current));
	g_free (fec->attributes);
	if (fec->to_visit != NULL) {
		g_list_foreach (fec->to_visit, (GFunc) child_data_free, NULL);
		g_list_free (fec->to_visit);
	}
	_g_object_unref (fec->cancellable);
	g_free (fec);
}


static gboolean
for_each_child_done_cb (gpointer user_data)
{
	ForEachChildData *fec = user_data;

	g_source_remove (fec->source_id);
	clear_child_data (&(fec->current));
	if (fec->done_func)
		fec->done_func (fec->error, fec->user_data);
	for_each_child_data_free (fec);

	return FALSE;
}


static void
for_each_child_done (ForEachChildData *fec)
{
	fec->source_id = g_idle_add (for_each_child_done_cb, fec);
}


static void for_each_child_start_current (ForEachChildData *fec);


static void
for_each_child_start (ForEachChildData *fec)
{
	for_each_child_start_current (fec);
}


static void
for_each_child_set_current (ForEachChildData *fec,
			    ChildData        *data)
{
	clear_child_data (&(fec->current));
	fec->current = data;
}


static void
for_each_child_start_next_sub_directory (ForEachChildData *fec)
{
	ChildData *child = NULL;

	if (fec->to_visit != NULL) {
		GList *tmp;

		child = (ChildData *) fec->to_visit->data;
		tmp = fec->to_visit;
		fec->to_visit = g_list_remove_link (fec->to_visit, tmp);
		g_list_free (tmp);
	}

	if (child != NULL) {
		for_each_child_set_current (fec, child);
		for_each_child_start (fec);
	}
	else
		for_each_child_done (fec);
}


static void
for_each_child_close_enumerator (GObject      *source_object,
				 GAsyncResult *result,
		      		 gpointer      user_data)
{
	ForEachChildData *fec = user_data;
	GError           *error = NULL;

	if (! g_file_enumerator_close_finish (fec->enumerator,
					      result,
					      &error))
	{
		if (fec->error == NULL)
			fec->error = g_error_copy (error);
		else
			g_clear_error (&error);
	}

	if ((fec->error == NULL) && fec->recursive)
		for_each_child_start_next_sub_directory (fec);
	else
		for_each_child_done (fec);
}


static void for_each_child_next_files_ready (GObject      *source_object,
					     GAsyncResult *result,
					     gpointer      user_data);


static void
for_each_child_read_next_files (ForEachChildData *fec)
{
	_g_object_list_unref (fec->children);
	fec->children = NULL;
	g_file_enumerator_next_files_async (fec->enumerator,
					    N_FILES_PER_REQUEST,
					    G_PRIORITY_DEFAULT,
					    fec->cancellable,
					    for_each_child_next_files_ready,
					    fec);
}


static void for_each_child_read_current_child_metadata (ForEachChildData *fec);


static void
for_each_child_read_next_child_metadata (ForEachChildData *fec)
{
	fec->current_child = fec->current_child->next;
	if (fec->current_child != NULL)
		for_each_child_read_current_child_metadata (fec);
	else
		for_each_child_read_next_files (fec);
}


static void
for_each_child_compute_child (ForEachChildData *fec,
			      GFile            *file,
			      GFileInfo        *info)
{
	if (g_file_info_get_file_type (info) == G_FILE_TYPE_DIRECTORY) {
		char *id;

		/* avoid to visit a directory more than ones */

		id = g_strdup (g_file_info_get_attribute_string (info, G_FILE_ATTRIBUTE_ID_FILE));
		if (id == NULL)
			id = g_file_get_uri (file);

		if (g_hash_table_lookup (fec->already_visited, id) == NULL) {
			g_hash_table_insert (fec->already_visited, g_strdup (id), GINT_TO_POINTER (1));
			fec->to_visit = g_list_append (fec->to_visit, child_data_new (file, info));
		}

		g_free (id);
	}

	fec->for_each_file_func (file, info, fec->user_data);
}


static void
for_each_child_metadata_ready_func (GObject      *source_object,
                		    GAsyncResult *result,
                		    gpointer      user_data)
{
	ForEachChildData *fec = user_data;
	GList            *files;
	GError           *error = NULL;

	files = _g_query_metadata_finish (result, &error);
	if (files != NULL) {
		GthFileData *child_data = files->data;
		for_each_child_compute_child (fec, child_data->file, child_data->info);
	}

	for_each_child_read_next_child_metadata (fec);
}


static void
for_each_child_read_current_child_metadata (ForEachChildData *fec)
{
	GFileInfo   *child_info = fec->current_child->data;
	GFile       *child_file;
	GList       *file_list;
	GthFileData *child_data;

	child_file = g_file_get_child (fec->current->file, g_file_info_get_name (child_info));
	child_data = gth_file_data_new (child_file, child_info);
	file_list = g_list_append (NULL, child_data);
	_g_query_metadata_async  (file_list,
				  fec->attributes,
				  NULL, /* FIXME: cannot use fec->cancellable here */
				  for_each_child_metadata_ready_func,
				  fec);

	_g_object_list_unref (file_list);
	g_object_unref (child_file);
}


static void
for_each_child_next_files_ready (GObject      *source_object,
				 GAsyncResult *result,
				 gpointer      user_data)
{
	ForEachChildData *fec = user_data;

	fec->children = g_file_enumerator_next_files_finish (fec->enumerator,
							     result,
							     &(fec->error));

	if (fec->children == NULL) {
		g_file_enumerator_close_async (fec->enumerator,
					       G_PRIORITY_DEFAULT,
					       fec->cancellable,
					       for_each_child_close_enumerator,
					       fec);
		return;
	}

	if (fec->metadata_attributes) {
		fec->current_child = fec->children;
		for_each_child_read_current_child_metadata (fec);
	}
	else {
		GList *scan;

		for (scan = fec->children; scan; scan = scan->next) {
			GFileInfo *child_info = scan->data;
			GFile     *child_file;

			child_file = g_file_get_child (fec->current->file, g_file_info_get_name (child_info));
			for_each_child_compute_child (fec, child_file, child_info);

			g_object_unref (child_file);
		}

		for_each_child_read_next_files (fec);
	}
}


static void
for_each_child_ready (GObject      *source_object,
		      GAsyncResult *result,
		      gpointer      user_data)
{
	ForEachChildData *fec = user_data;

	fec->enumerator = g_file_enumerate_children_finish (G_FILE (source_object), result, &(fec->error));
	if (fec->enumerator == NULL) {
		for_each_child_done (fec);
		return;
	}

	g_file_enumerator_next_files_async (fec->enumerator,
					    N_FILES_PER_REQUEST,
					    G_PRIORITY_DEFAULT,
					    fec->cancellable,
					    for_each_child_next_files_ready,
					    fec);
}


static void
for_each_child_start_current (ForEachChildData *fec)
{
	if (fec->start_dir_func != NULL) {
		DirOp  op;

		op = fec->start_dir_func (fec->current->file, fec->current->info, &(fec->error), fec->user_data);
		switch (op) {
		case DIR_OP_SKIP:
			for_each_child_start_next_sub_directory (fec);
			return;
		case DIR_OP_STOP:
			for_each_child_done (fec);
			return;
		case DIR_OP_CONTINUE:
			break;
		}
	}

	g_file_enumerate_children_async (fec->current->file,
					 fec->attributes,
					 fec->follow_links ? G_FILE_QUERY_INFO_NONE : G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS,
					 G_PRIORITY_DEFAULT,
					 fec->cancellable,
					 for_each_child_ready,
					 fec);
}


static void
directory_info_ready_cb (GObject      *source_object,
			 GAsyncResult *result,
			 gpointer      user_data)
{
	ForEachChildData *fec = user_data;
	GFileInfo        *info;
	ChildData        *child;

	info = g_file_query_info_finish (G_FILE (source_object), result, &(fec->error));
	if (info == NULL) {
		for_each_child_done (fec);
		return;
	}

	child = child_data_new (fec->base_directory, info);
	g_object_unref (info);

	for_each_child_set_current (fec, child);
	for_each_child_start_current (fec);
}


/**
 * g_directory_foreach_child:
 * @directory: The directory to visit.
 * @recursive: Whether to traverse the @directory recursively.
 * @follow_links: Whether to dereference the symbolic links.
 * @attributes: The GFileInfo attributes to read.
 * @cancellable: An optional @GCancellable object, used to cancel the process.
 * @start_dir_func: the function called for each sub-directory, or %NULL if
 *   not needed.
 * @for_each_file_func: the function called for each file.  Can't be %NULL.
 * @done_func: the function called at the end of the traversing process.
 *   Can't be %NULL.
 * @user_data: data to pass to @done_func
 *
 * Traverse the @directory's filesystem structure calling the
 * @for_each_file_func function for each file in the directory; the
 * @start_dir_func function on each directory before it's going to be
 * traversed, this includes @directory too; the @done_func function is
 * called at the end of the process.
 * Some traversing options are available: if @recursive is TRUE the
 * directory is traversed recursively; if @follow_links is TRUE, symbolic
 * links are dereferenced, otherwise they are returned as links.
 * Each callback uses the same @user_data additional parameter.
 */
void
g_directory_foreach_child (GFile                *directory,
			   gboolean              recursive,
			   gboolean              follow_links,
			   const char           *attributes,
			   GCancellable         *cancellable,
			   StartDirCallback      start_dir_func,
			   ForEachChildCallback  for_each_file_func,
			   ReadyFunc             done_func,
			   gpointer              user_data)
{
	ForEachChildData *fec;

	g_return_if_fail (for_each_file_func != NULL);

	fec = g_new0 (ForEachChildData, 1);

	fec->base_directory = g_file_dup (directory);
	fec->recursive = recursive;
	fec->follow_links = follow_links;
	fec->attributes = g_strconcat (attributes, ",standard::name,standard::type,id::file", NULL);
	fec->cancellable = _g_object_ref (cancellable);
	fec->start_dir_func = start_dir_func;
	fec->for_each_file_func = for_each_file_func;
	fec->done_func = done_func;
	fec->user_data = user_data;
	fec->already_visited = g_hash_table_new_full (g_str_hash,
						      g_str_equal,
						      g_free,
						      NULL);
	fec->metadata_attributes = ! _g_file_attributes_matches_all (fec->attributes, GIO_ATTRIBUTES);

	g_file_query_info_async (fec->base_directory,
				 fec->attributes,
				 G_FILE_QUERY_INFO_NONE,
				 G_PRIORITY_DEFAULT,
				 fec->cancellable,
				 directory_info_ready_cb,
				 fec);
}


/* -- get_file_list_data -- */


typedef struct {
	GList             *files;
	GList             *dirs;
	GFile             *directory;
	char              *base_dir;
	GCancellable      *cancellable;
	ListReadyCallback  done_func;
	gpointer           done_data;
	GList             *to_visit;
	GList             *current_dir;
	Filter            *include_filter;
	Filter            *exclude_filter;
	Filter            *exclude_folders_filter;
	guint              visit_timeout;
} GetFileListData;


static void
get_file_list_data_free (GetFileListData *gfl)
{
	if (gfl == NULL)
		return;

	filter_destroy (gfl->include_filter);
	filter_destroy (gfl->exclude_filter);
	filter_destroy (gfl->exclude_folders_filter);
	_g_string_list_free (gfl->files);
	_g_string_list_free (gfl->dirs);
	_g_string_list_free (gfl->to_visit);
	g_object_unref (gfl->directory);
	g_free (gfl->base_dir);
	g_free (gfl);
}


/* -- g_directory_list_async -- */


static GList*
get_relative_file_list (GList      *rel_list,
			GList      *file_list,
			const char *base_dir)
{
	GList *scan;
	int    base_len;

	if (base_dir == NULL)
		return NULL;

	base_len = 0;
	if (strcmp (base_dir, "/") != 0)
		base_len = strlen (base_dir);

	for (scan = file_list; scan; scan = scan->next) {
		char *uri = scan->data;
		if (_g_uri_parent_of_uri (base_dir, uri)) {
			char *rel_uri = g_strdup (uri + base_len + 1);
			rel_list = g_list_prepend (rel_list, rel_uri);
		}
	}

	return rel_list;
}


static GList*
get_dir_list_from_file_list (GHashTable *h_dirs,
			     const char *base_dir,
			     GList      *files,
			     gboolean    is_dir_list)
{
	GList *scan;
	GList *dir_list = NULL;
	int    base_dir_len;

	if (base_dir == NULL)
		base_dir = "";
	base_dir_len = strlen (base_dir);

	for (scan = files; scan; scan = scan->next) {
		char *filename = scan->data;
		char *dir_name;

		if (strlen (filename) <= base_dir_len)
			continue;

		if (is_dir_list)
			dir_name = g_strdup (filename + base_dir_len + 1);
		else
			dir_name = _g_uri_get_parent (filename + base_dir_len + 1);

		while ((dir_name != NULL) && (dir_name[0] != '\0') && (strcmp (dir_name, "/") != 0)) {
			char *tmp;
			char *dir;

			/* avoid to insert duplicated folders */

			dir = g_strconcat (base_dir, "/", dir_name, NULL);
			if (g_hash_table_lookup (h_dirs, dir) == NULL) {
				g_hash_table_insert (h_dirs, dir, GINT_TO_POINTER (1));
				dir_list = g_list_prepend (dir_list, dir);
			}
			else
				g_free (dir);

			tmp = dir_name;
			dir_name = _g_uri_get_parent (tmp);
			g_free (tmp);
		}

		g_free (dir_name);
	}

	return dir_list;
}


static void
get_file_list_done (GError   *error,
		    gpointer  user_data)
{
	GetFileListData *gfl = user_data;
	GHashTable      *h_dirs;
	GList           *scan;

	gfl->files = g_list_reverse (gfl->files);
	gfl->dirs = g_list_reverse (gfl->dirs);

	if (! filter_empty (gfl->include_filter) || (gfl->exclude_filter->pattern != NULL)) {
		_g_string_list_free (gfl->dirs);
		gfl->dirs = NULL;
	}

	h_dirs = g_hash_table_new (g_str_hash, g_str_equal);

	/* Always include the base directory, this way empty base
 	 * directories are added to the archive as well.  */

	if (gfl->base_dir != NULL) {
		char *dir;

		dir = g_strdup (gfl->base_dir);
		gfl->dirs = g_list_prepend (gfl->dirs, dir);
		g_hash_table_insert (h_dirs, dir, GINT_TO_POINTER (1));
	}

	/* Add all the parent directories in gfl->files/gfl->dirs to the
	 * gfl->dirs list, the hash table is used to avoid duplicated
	 * entries. */

	for (scan = gfl->dirs; scan; scan = scan->next)
		g_hash_table_insert (h_dirs, (char*)scan->data, GINT_TO_POINTER (1));

	gfl->dirs = g_list_concat (gfl->dirs, get_dir_list_from_file_list (h_dirs, gfl->base_dir, gfl->files, FALSE));

	if (filter_empty (gfl->include_filter))
		gfl->dirs = g_list_concat (gfl->dirs, get_dir_list_from_file_list (h_dirs, gfl->base_dir, gfl->dirs, TRUE));

	/**/

	if (error == NULL) {
		GList *rel_files, *rel_dirs;

		if (gfl->base_dir != NULL) {
			rel_files = get_relative_file_list (NULL, gfl->files, gfl->base_dir);
			rel_dirs = get_relative_file_list (NULL, gfl->dirs, gfl->base_dir);
		}
		else {
			rel_files = gfl->files;
			rel_dirs = gfl->dirs;
			gfl->files = NULL;
			gfl->dirs = NULL;
		}

		/* rel_files/rel_dirs must be deallocated in done_func */
		gfl->done_func (rel_files, rel_dirs, NULL, gfl->done_data);
	}
	else
		gfl->done_func (NULL, NULL, error, gfl->done_data);

	g_hash_table_destroy (h_dirs);
	get_file_list_data_free (gfl);
}


static void
get_file_list_for_each_file (GFile     *file,
			     GFileInfo *info,
			     gpointer   user_data)
{
	GetFileListData *gfl = user_data;
	char            *uri;

	uri = g_file_get_uri (file);

	switch (g_file_info_get_file_type (info)) {
	case G_FILE_TYPE_REGULAR:
		if (filter_matches (gfl->include_filter, uri))
			if ((gfl->exclude_filter->pattern == NULL) || ! filter_matches (gfl->exclude_filter, uri))
				gfl->files = g_list_prepend (gfl->files, g_strdup (uri));
		break;
	default:
		break;
	}

	g_free (uri);
}


static DirOp
get_file_list_start_dir (GFile       *directory,
			 GFileInfo   *info,
			 GError     **error,
			 gpointer     user_data)
{
	DirOp            dir_op = DIR_OP_CONTINUE;
	GetFileListData *gfl = user_data;
	char            *uri;

	uri = g_file_get_uri (directory);
	if ((gfl->exclude_folders_filter->pattern == NULL) || ! filter_matches (gfl->exclude_folders_filter, uri)) {
		gfl->dirs = g_list_prepend (gfl->dirs, g_strdup (uri));
		dir_op = DIR_OP_CONTINUE;
	}
	else
		dir_op = DIR_OP_SKIP;

	g_free (uri);

	return dir_op;
}


void
g_directory_list_async (GFile             *directory,
			const char        *base_dir,
			gboolean           recursive,
			gboolean           follow_links,
			gboolean           no_backup_files,
			gboolean           no_dot_files,
			const char        *include_files,
			const char        *exclude_files,
			const char        *exclude_folders,
			gboolean           ignorecase,
			GCancellable      *cancellable,
			ListReadyCallback  done_func,
			gpointer           done_data)
{
	GetFileListData *gfl;
	FilterOptions    filter_options;

	gfl = g_new0 (GetFileListData, 1);
	gfl->directory = g_file_dup (directory);
	gfl->base_dir = g_strdup (base_dir);
	gfl->done_func = done_func;
	gfl->done_data = done_data;

	filter_options = FILTER_DEFAULT;
	if (no_backup_files)
		filter_options |= FILTER_NOBACKUPFILES;
	if (no_dot_files)
		filter_options |= FILTER_NODOTFILES;
	if (ignorecase)
		filter_options |= FILTER_IGNORECASE;
	gfl->include_filter = filter_new (include_files, filter_options);
	gfl->exclude_filter = filter_new (exclude_files, ignorecase ? FILTER_IGNORECASE : FILTER_DEFAULT);
	gfl->exclude_folders_filter = filter_new (exclude_folders, ignorecase ? FILTER_IGNORECASE : FILTER_DEFAULT);

	g_directory_foreach_child (directory,
				   recursive,
				   follow_links,
				   "standard::name,standard::type",
				   cancellable,
				   get_file_list_start_dir,
				   get_file_list_for_each_file,
				   get_file_list_done,
				   gfl);
}


/* -- _g_query_info_async -- */


typedef struct {
	GList             *file_list;
	GthListFlags       flags;
	char              *attributes;
	GCancellable      *cancellable;
	InfoReadyCallback  callback;
	gpointer           user_data;
	GList             *current;
	GList             *files;
} QueryInfoData;


static void
query_data_free (QueryInfoData *query_data)
{
	_g_object_list_unref (query_data->file_list);
	_g_object_list_unref (query_data->files);
	_g_object_unref (query_data->cancellable);
	g_free (query_data->attributes);
	g_free (query_data);
}


static void query_info__query_current (QueryInfoData *query_data);


static void
query_info__query_next (QueryInfoData *query_data)
{
	query_data->current = query_data->current->next;
	query_info__query_current (query_data);
}


static void
query_data__done_cb (GError   *error,
		     gpointer  user_data)
{
	QueryInfoData *query_data = user_data;

	if (error != NULL) {
		query_data->callback (NULL, error, query_data->user_data);
		query_data_free (query_data);
		return;
	}

	query_info__query_next (query_data);
}


static void
query_data__for_each_file_cb (GFile     *file,
			      GFileInfo *info,
			      gpointer   user_data)
{
	QueryInfoData *query_data = user_data;

	if (g_file_info_get_file_type (info) == G_FILE_TYPE_DIRECTORY)
		return;
	if ((query_data->flags & GTH_LIST_NO_BACKUP_FILES) && g_file_info_get_is_backup (info))
		return;
	if ((query_data->flags & GTH_LIST_NO_HIDDEN_FILES) && g_file_info_get_is_hidden (info))
		return;

	query_data->files = g_list_prepend (query_data->files, gth_file_data_new (file, info));
}


static DirOp
query_data__start_dir_cb (GFile       *directory,
		          GFileInfo   *info,
		          GError     **error,
		          gpointer     user_data)
{
	QueryInfoData *query_data = user_data;

	if ((query_data->flags & GTH_LIST_NO_BACKUP_FILES) && g_file_info_get_is_backup (info))
		return DIR_OP_SKIP;
	if ((query_data->flags & GTH_LIST_NO_HIDDEN_FILES) && g_file_info_get_is_hidden (info))
		return DIR_OP_SKIP;

	query_data->files = g_list_prepend (query_data->files, gth_file_data_new (directory, info));

	return DIR_OP_CONTINUE;
}


static void
query_data_info_ready_cb (GObject      *source_object,
                          GAsyncResult *result,
                          gpointer      user_data)
{
	QueryInfoData *query_data = user_data;
	GError        *error = NULL;
	GFileInfo     *info;

	info = g_file_query_info_finish ((GFile *) source_object, result, &error);
	if (info == NULL) {
		query_data->callback (NULL, error, query_data->user_data);
		query_data_free (query_data);
		return;
	}

	if ((query_data->flags & GTH_LIST_RECURSIVE) && (g_file_info_get_file_type (info) == G_FILE_TYPE_DIRECTORY)) {
		g_directory_foreach_child ((GFile *) query_data->current->data,
					   TRUE,
					   (query_data->flags & GTH_LIST_NO_FOLLOW_LINKS) == 0,
					   query_data->attributes,
					   query_data->cancellable,
					   query_data__start_dir_cb,
					   query_data__for_each_file_cb,
					   query_data__done_cb,
					   query_data);
	}
	else {
		query_data->files = g_list_prepend (query_data->files, gth_file_data_new ((GFile *) query_data->current->data, info));
		query_info__query_next (query_data);
	}

	g_object_unref (info);
}


static void
query_info__query_current (QueryInfoData *query_data)
{
	GFileQueryInfoFlags flags;

	if (query_data->current == NULL) {
		query_data->files = g_list_reverse (query_data->files);
		query_data->callback (query_data->files, NULL, query_data->user_data);
		query_data_free (query_data);
		return;
	}

	flags = G_FILE_QUERY_INFO_NONE;
	if (query_data->flags & GTH_LIST_NO_FOLLOW_LINKS)
		flags |= G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS;

	g_file_query_info_async ((GFile *) query_data->current->data,
				 query_data->attributes,
				 flags,
				 G_PRIORITY_DEFAULT,
				 query_data->cancellable,
				 query_data_info_ready_cb,
				 query_data);
}


void
_g_query_info_async (GList             *file_list,
		     GthListFlags       flags,
		     const char        *attributes,
		     GCancellable      *cancellable,
		     InfoReadyCallback  ready_callback,
		     gpointer           user_data)
{
	QueryInfoData *query_data;

	query_data = g_new0 (QueryInfoData, 1);
	query_data->file_list = _g_object_list_ref (file_list);
	query_data->flags = flags;
	query_data->attributes = g_strconcat ("standard::name,standard::type,standard::is-hidden,standard::is-backup,id::file",
					      (((attributes != NULL) && (strcmp (attributes, "") != 0)) ? "," : NULL),
					      attributes,
					      NULL);
	query_data->cancellable = _g_object_ref (cancellable);
	query_data->callback = ready_callback;
	query_data->user_data = user_data;

	query_data->current = query_data->file_list;
	query_info__query_current (query_data);
}


/* -- _g_dummy_file_op_async  -- */


typedef struct {
	ReadyFunc callback;
	gpointer  user_data;
	gulong    dummy_event;
} DummyFileCopy;


gboolean
_g_dummy_file_op_completed (gpointer data)
{
	DummyFileCopy *dfd = data;

	if (dfd->dummy_event != 0) {
		g_source_remove (dfd->dummy_event);
		dfd->dummy_event = 0;
	}

	if (dfd->callback)
		dfd->callback (NULL, dfd->user_data);

	g_free (dfd);

	return FALSE;
}


void
_g_dummy_file_op_async (ReadyFunc callback,
			gpointer  user_data)
{
	DummyFileCopy *dfd;

	dfd = g_new0 (DummyFileCopy, 1);
	dfd->callback = callback;
	dfd->user_data = user_data;
	dfd->dummy_event = g_idle_add (_g_dummy_file_op_completed, dfd);
}



/* -- _g_copy_file_async -- */


typedef struct {
	GthFileData          *source;
	GFile                *destination;
	gboolean              move;
	GFileCopyFlags        flags;
	int                   io_priority;
	goffset               tot_size;
	goffset               copied_size;
	gsize                 tot_files;
	GCancellable         *cancellable;
	ProgressCallback      progress_callback;
	gpointer              progress_callback_data;
	DialogCallback        dialog_callback;
	gpointer              dialog_callback_data;
	CopyReadyCallback     ready_callback;
	gpointer              user_data;

	GFile                *current_destination;
	char                 *message;
	GthOverwriteResponse  default_response;

	GList                *source_sidecars;  /* GFile list */
	GList                *destination_sidecars;  /* GFile list */
	GList                *current_source_sidecar;
	GList                *current_destination_sidecar;
} CopyFileData;


static void
copy_file_data_free (CopyFileData *copy_file_data)
{
	g_object_unref (copy_file_data->source);
	g_object_unref (copy_file_data->destination);
	_g_object_unref (copy_file_data->cancellable);
	_g_object_unref (copy_file_data->current_destination);
	g_free (copy_file_data->message);
	_g_object_list_unref (copy_file_data->destination_sidecars);
	_g_object_list_unref (copy_file_data->source_sidecars);
	g_free (copy_file_data);
}


static void
copy_file__delete_source (CopyFileData *copy_file_data)
{
	GError *error = NULL;

	if (copy_file_data->move)
		g_file_delete (copy_file_data->source->file, copy_file_data->cancellable, &error);

	copy_file_data->ready_callback (copy_file_data->default_response, error, copy_file_data->user_data);
	copy_file_data_free (copy_file_data);
}


static void copy_file__copy_current_sidecar (CopyFileData *copy_file_data);


static void
copy_file__copy_next_sidecar (CopyFileData *copy_file_data)
{
	copy_file_data->current_source_sidecar = copy_file_data->current_source_sidecar->next;
	copy_file_data->current_destination_sidecar = copy_file_data->current_destination_sidecar->next;
	copy_file__copy_current_sidecar (copy_file_data);
}


static void
copy_file__copy_current_sidecar_ready_cb (GObject      *source_object,
				          GAsyncResult *result,
				          gpointer      user_data)
{
	CopyFileData *copy_file_data = user_data;

	if (g_file_copy_finish ((GFile *) source_object, result, NULL)) {
		if (copy_file_data->move)
			g_file_delete ((GFile *) copy_file_data->current_source_sidecar->data, copy_file_data->cancellable, NULL);
	}

	copy_file__copy_next_sidecar (copy_file_data);
}


static void
copy_file__copy_current_sidecar (CopyFileData *copy_file_data)
{
	GFile *source;
	GFile *destination;
	GFile *destination_parent;

	if (copy_file_data->current_source_sidecar == NULL) {
		copy_file__delete_source (copy_file_data);
		return;
	}

	source = copy_file_data->current_source_sidecar->data;
	if (! g_file_query_exists (source, copy_file_data->cancellable)) {
		copy_file__copy_next_sidecar (copy_file_data);
		return;
	}

	destination = copy_file_data->current_destination_sidecar->data;
	destination_parent = g_file_get_parent (destination);
	g_file_make_directory (destination_parent, copy_file_data->cancellable, NULL);

	g_file_copy_async (source,
			   destination,
			   G_FILE_COPY_OVERWRITE,
			   copy_file_data->io_priority,
			   copy_file_data->cancellable,
			   NULL,
			   NULL,
			   copy_file__copy_current_sidecar_ready_cb,
			   copy_file_data);

	g_object_unref (destination_parent);
}


static void _g_copy_file_to_destination (CopyFileData   *copy_file_data,
					 GFile          *destination,
					 GFileCopyFlags  flags);


static void
copy_file__overwrite_dialog_response_cb (GtkDialog *dialog,
				         int        response_id,
				         gpointer   user_data)
{
	CopyFileData *copy_file_data = user_data;

	if (response_id != GTK_RESPONSE_OK)
		copy_file_data->default_response = GTH_OVERWRITE_RESPONSE_CANCEL;
	else
		copy_file_data->default_response = gth_overwrite_dialog_get_response (GTH_OVERWRITE_DIALOG (dialog));

	gtk_widget_hide (GTK_WIDGET (dialog));

	if (copy_file_data->dialog_callback != NULL)
		copy_file_data->dialog_callback (FALSE, NULL, copy_file_data->dialog_callback_data);

	switch (copy_file_data->default_response) {
	case GTH_OVERWRITE_RESPONSE_NO:
	case GTH_OVERWRITE_RESPONSE_ALWAYS_NO:
	case GTH_OVERWRITE_RESPONSE_UNSPECIFIED:
	case GTH_OVERWRITE_RESPONSE_CANCEL:
		{
			GError *error = NULL;

			if (copy_file_data->default_response == GTH_OVERWRITE_RESPONSE_CANCEL)
				error = g_error_new_literal (G_IO_ERROR, G_IO_ERROR_CANCELLED, "");
			else
				error = g_error_new_literal (G_IO_ERROR, G_IO_ERROR_EXISTS, "");
			copy_file_data->ready_callback (copy_file_data->default_response, error, copy_file_data->user_data);
			copy_file_data_free (copy_file_data);
			return;
		}

	case GTH_OVERWRITE_RESPONSE_YES:
	case GTH_OVERWRITE_RESPONSE_ALWAYS_YES:
		_g_copy_file_to_destination (copy_file_data, copy_file_data->current_destination, G_FILE_COPY_OVERWRITE);
		break;

	case GTH_OVERWRITE_RESPONSE_RENAME:
		{
			GFile *parent;
			GFile *new_destination;

			parent = g_file_get_parent (copy_file_data->current_destination);
			new_destination = g_file_get_child_for_display_name (parent, gth_overwrite_dialog_get_filename (GTH_OVERWRITE_DIALOG (dialog)), NULL);
			_g_copy_file_to_destination (copy_file_data, new_destination, 0);

			g_object_unref (new_destination);
			g_object_unref (parent);
		}
		break;
	}

	gtk_widget_destroy (GTK_WIDGET (dialog));
}


static void
copy_file_ready_cb (GObject      *source_object,
		    GAsyncResult *result,
		    gpointer      user_data)
{
	CopyFileData *copy_file_data = user_data;
	GError       *error = NULL;

	if (! g_file_copy_finish ((GFile *) source_object, result, &error)) {
		if (g_error_matches (error, G_IO_ERROR, G_IO_ERROR_EXISTS)) {
			if (copy_file_data->default_response != GTH_OVERWRITE_RESPONSE_ALWAYS_NO) {
				GtkWidget *dialog;

				dialog = gth_overwrite_dialog_new (copy_file_data->source->file,
								   NULL,
								   copy_file_data->current_destination,
								   copy_file_data->default_response,
								   copy_file_data->tot_files == 1);

				if (copy_file_data->dialog_callback != NULL)
					copy_file_data->dialog_callback (TRUE, dialog, copy_file_data->dialog_callback_data);

				g_signal_connect (dialog,
						  "response",
						  G_CALLBACK (copy_file__overwrite_dialog_response_cb),
						  copy_file_data);
				gtk_widget_show (dialog);
			}
			else {
				copy_file_data->ready_callback (copy_file_data->default_response, error, copy_file_data->user_data);
				copy_file_data_free (copy_file_data);
			}
			return;
		}
		copy_file_data->ready_callback (copy_file_data->default_response, error, copy_file_data->user_data);
		copy_file_data_free (copy_file_data);
		return;
	}

	/* copy the metadata sidecars if requested */

	_g_object_list_unref (copy_file_data->source_sidecars);
	_g_object_list_unref (copy_file_data->destination_sidecars);
	copy_file_data->source_sidecars = NULL;
	copy_file_data->destination_sidecars = NULL;
	if (copy_file_data->flags && G_FILE_COPY_ALL_METADATA) {
		gth_hook_invoke ("add-sidecars", copy_file_data->source->file, &copy_file_data->source_sidecars);
		gth_hook_invoke ("add-sidecars", copy_file_data->destination, &copy_file_data->destination_sidecars);
		copy_file_data->source_sidecars = g_list_reverse (copy_file_data->source_sidecars);
		copy_file_data->destination_sidecars = g_list_reverse (copy_file_data->destination_sidecars);
	}

	copy_file_data->current_source_sidecar = copy_file_data->source_sidecars;
	copy_file_data->current_destination_sidecar = copy_file_data->destination_sidecars;
	copy_file__copy_current_sidecar (copy_file_data);
}


static void
copy_file_progress_cb (goffset  current_num_bytes,
                       goffset  total_num_bytes,
                       gpointer user_data)
{
	CopyFileData *copy_file_data = user_data;
	char         *s1;
	char         *s2;
	char         *details;

	if (copy_file_data->progress_callback == NULL)
		return;

	s1 = g_format_size (copy_file_data->copied_size + current_num_bytes);
	s2 = g_format_size (copy_file_data->tot_size);
	/* For translators: This is a progress size indicator, for example: 230.4 MB of 512.8 MB */
	details = g_strdup_printf (_("%s of %s"), s1, s2);

	copy_file_data->progress_callback (NULL,
					   copy_file_data->message,
					   details,
					   FALSE,
					   (double) (copy_file_data->copied_size + current_num_bytes) / copy_file_data->tot_size,
					   copy_file_data->progress_callback_data);

	g_free (details);
	g_free (s2);
	g_free (s1);
}


static void
_g_copy_file_to_destination (CopyFileData   *copy_file_data,
			     GFile          *destination,
			     GFileCopyFlags  flags)
{
	_g_object_unref (copy_file_data->current_destination);
	copy_file_data->current_destination = g_file_dup (destination);

	if (copy_file_data->default_response == GTH_OVERWRITE_RESPONSE_ALWAYS_YES)
		flags = G_FILE_COPY_OVERWRITE;

	if (copy_file_data->progress_callback != NULL) {
		GFile *destination_parent;
		char  *destination_name;

		g_free (copy_file_data->message);

		destination_parent = g_file_get_parent (copy_file_data->current_destination);
		destination_name = g_file_get_parse_name (destination_parent);
		if (copy_file_data->move)
			copy_file_data->message = g_strdup_printf (_("Moving \"%s\" to \"%s\""), g_file_info_get_display_name (copy_file_data->source->info), destination_name);
		else
			copy_file_data->message = g_strdup_printf (_("Copying \"%s\" to \"%s\""), g_file_info_get_display_name (copy_file_data->source->info), destination_name);

		copy_file_progress_cb (0, 0, copy_file_data);

		g_free (destination_name);
		g_object_unref (destination_parent);
	}

	if (g_file_info_get_file_type (copy_file_data->source->info) == G_FILE_TYPE_DIRECTORY) {
		GError *error = NULL;

		if (! g_file_make_directory (copy_file_data->current_destination, copy_file_data->cancellable, &error)) {
			if (g_error_matches (error, G_IO_ERROR, G_IO_ERROR_EXISTS))
				g_clear_error (&error);
		}
		copy_file_data->ready_callback (copy_file_data->default_response, error, copy_file_data->user_data);
		copy_file_data_free (copy_file_data);
		return;
	}
	else
		g_file_copy_async (copy_file_data->source->file,
				   copy_file_data->current_destination,
				   copy_file_data->flags | flags,
				   copy_file_data->io_priority,
				   copy_file_data->cancellable,
				   copy_file_progress_cb,
				   copy_file_data,
				   copy_file_ready_cb,
				   copy_file_data);
}


static void
_g_copy_file_async_private (GthFileData           *source,
			    GFile                 *destination,
			    gboolean               move,
			    GFileCopyFlags         flags,
			    GthOverwriteResponse   default_response,
			    int                    io_priority,
			    goffset                tot_size,
			    goffset                copied_size,
			    gsize                  tot_files,
			    GCancellable          *cancellable,
			    ProgressCallback       progress_callback,
			    gpointer               progress_callback_data,
			    DialogCallback         dialog_callback,
			    gpointer               dialog_callback_data,
			    CopyReadyCallback      ready_callback,
			    gpointer               user_data)
{
	CopyFileData *copy_file_data;

	copy_file_data = g_new0 (CopyFileData, 1);
	copy_file_data->source = g_object_ref (source);
	copy_file_data->destination = g_object_ref (destination);
	copy_file_data->move = move;
	copy_file_data->flags = flags;
	copy_file_data->io_priority = io_priority;
	copy_file_data->tot_size = tot_size;
	copy_file_data->copied_size = copied_size;
	copy_file_data->tot_files = tot_files;
	copy_file_data->cancellable = _g_object_ref (cancellable);
	copy_file_data->progress_callback = progress_callback;
	copy_file_data->progress_callback_data = progress_callback_data;
	copy_file_data->dialog_callback = dialog_callback;
	copy_file_data->dialog_callback_data = dialog_callback_data;
	copy_file_data->ready_callback = ready_callback;
	copy_file_data->user_data = user_data;
	copy_file_data->default_response = default_response;

	_g_copy_file_to_destination (copy_file_data, copy_file_data->destination, 0);
}


void
_g_copy_file_async (GthFileData           *source,
		    GFile                 *destination,
		    gboolean               move,
		    GFileCopyFlags         flags,
		    GthOverwriteResponse   default_response,
		    int                    io_priority,
		    GCancellable          *cancellable,
		    ProgressCallback       progress_callback,
		    gpointer               progress_callback_data,
		    DialogCallback         dialog_callback,
		    gpointer               dialog_callback_data,
		    CopyReadyCallback      ready_callback,
		    gpointer               user_data)
{
	_g_copy_file_async_private (source,
				    destination,
				    move,
				    flags,
				    default_response,
				    io_priority,
				    g_file_info_get_size (source->info),
				    0,
				    1,
				    cancellable,
				    progress_callback,
				    progress_callback_data,
				    dialog_callback,
				    dialog_callback_data,
				    ready_callback,
				    user_data);
}


/* -- _g_copy_files_async -- */


typedef struct {
	GHashTable           *source_hash;
	GFile                *destination;

	GList                *files;  /* GthFileData list */
	GList                *current;
	GList                *copied_directories; /* GFile list */
	GFile                *source_base;
	GFile                *current_destination;

	GList                *source_sidecars;  /* GFile list */
	GList                *destination_sidecars;  /* GFile list */
	GList                *current_source_sidecar;
	GList                *current_destination_sidecar;

	goffset               tot_size;
	goffset               copied_size;
	gsize                 tot_files;

	char                 *message;
	GthOverwriteResponse  default_response;

	gboolean              move;
	GFileCopyFlags        flags;
	int                   io_priority;
	GCancellable         *cancellable;
	ProgressCallback      progress_callback;
	gpointer              progress_callback_data;
	DialogCallback        dialog_callback;
	gpointer              dialog_callback_data;
	ReadyFunc             done_callback;
	gpointer              user_data;
} CopyData;


static void
copy_data_free (CopyData *copy_data)
{
	g_free (copy_data->message);
	_g_object_list_unref (copy_data->destination_sidecars);
	_g_object_list_unref (copy_data->source_sidecars);
	_g_object_unref (copy_data->current_destination);
	_g_object_list_unref (copy_data->copied_directories);
	_g_object_list_unref (copy_data->files);
	_g_object_unref (copy_data->source_base);
	g_hash_table_destroy (copy_data->source_hash);
	g_object_unref (copy_data->destination);
	_g_object_unref (copy_data->cancellable);
	g_free (copy_data);
}


static void
copy_data__done (CopyData *copy_data,
		 GError   *error)
{
	copy_data->done_callback (error, copy_data->user_data);
	copy_data_free (copy_data);
}


static void copy_data__copy_current_file (CopyData *copy_data);


static void
copy_data__copy_next_file (CopyData *copy_data)
{
	GthFileData *source = (GthFileData *) copy_data->current->data;

	copy_data->copied_size += g_file_info_get_size (source->info);
	copy_data->current = copy_data->current->next;
	copy_data__copy_current_file (copy_data);
}


static void
copy_data__copy_current_file_ready_cb (GthOverwriteResponse  response,
				       GError               *error,
			 	       gpointer              user_data)
{
	CopyData *copy_data = user_data;

	if (error == NULL) {
		/* save the correctly copied directories in order to delete
		 * them after moving their content. */
		if (copy_data->move) {
			GthFileData *source;

			source = (GthFileData *) copy_data->current->data;
			if (g_file_info_get_file_type (source->info) == G_FILE_TYPE_DIRECTORY)
				copy_data->copied_directories = g_list_prepend (copy_data->copied_directories, g_file_dup (source->file));
		}
	}
	else if ((response == GTH_OVERWRITE_RESPONSE_ALWAYS_NO) || ! g_error_matches (error, G_IO_ERROR, G_IO_ERROR_EXISTS)) {
		copy_data__done (copy_data, error);
		return;
	}

	copy_data->default_response = response;
	copy_data__copy_next_file (copy_data);
}


static void
copy_data__delete_source_directories (CopyData *copy_data)
{
	GError *error = NULL;

	if (copy_data->move) {
		GList *scan;

		for (scan = copy_data->copied_directories; scan; scan = scan->next) {
			GFile *file = scan->data;

			if (! g_file_delete (file, copy_data->cancellable, &error))
				break;
		}
	}

	copy_data__done (copy_data, error);
}


static void
copy_data__copy_current_file (CopyData *copy_data)
{
	GthFileData *source;
	gboolean     explicitly_requested;
	GFile       *destination;

	if (copy_data->current == NULL) {
		copy_data__delete_source_directories (copy_data);
		return;
	}

	source = (GthFileData *) copy_data->current->data;
	explicitly_requested = (g_hash_table_lookup (copy_data->source_hash, source->file) != NULL);

	/* Ignore non-existent files that weren't explicitly requested,
	 * they are children of some requested directory and if they don't
	 * exist anymore they have been already moved to the destination
	 * because they are metadata of other files. */
	if (! explicitly_requested) {
		if (! g_file_query_exists (source->file, copy_data->cancellable)) {
			call_when_idle ((DataFunc) copy_data__copy_next_file, copy_data);
			return;
		}
	}

	/* compute the destination */
	if (explicitly_requested) {
		_g_object_unref (copy_data->source_base);
		copy_data->source_base = g_file_get_parent (source->file);
	}
	destination = _g_file_get_destination (source->file, copy_data->source_base, copy_data->destination);

	if (g_file_equal (source->file, destination)) {
		g_object_unref (destination);
		call_when_idle ((DataFunc) copy_data__copy_next_file, copy_data);
		return;
	}

	_g_copy_file_async_private (source,
				    destination,
				    copy_data->move,
				    copy_data->flags,
				    copy_data->default_response,
				    copy_data->io_priority,
				    copy_data->tot_size,
				    copy_data->copied_size,
				    copy_data->tot_files,
				    copy_data->cancellable,
				    copy_data->progress_callback,
				    copy_data->progress_callback_data,
				    copy_data->dialog_callback,
				    copy_data->dialog_callback_data,
				    copy_data__copy_current_file_ready_cb,
				    copy_data);

	g_object_unref (destination);
}


static void
copy_files__sources_info_ready_cb (GList    *files,
			           GError   *error,
			           gpointer  user_data)
{
	CopyData *copy_data = user_data;
	GList    *scan;

	if (error != NULL) {
		copy_data__done (copy_data, error);
		return;
	}

	copy_data->files = _g_object_list_ref (files);
	copy_data->tot_size = 0;
	copy_data->tot_files = 0;
	for (scan = copy_data->files; scan; scan = scan->next) {
		GthFileData *file_data = (GthFileData *) scan->data;

		copy_data->tot_size += g_file_info_get_size (file_data->info);
		copy_data->tot_files += 1;
	}

	copy_data->copied_size = 0;
	copy_data->current = copy_data->files;
	copy_data__copy_current_file (copy_data);
}


void
_g_copy_files_async (GList                *sources, /* GFile list */
		     GFile                *destination,
		     gboolean              move,
		     GFileCopyFlags        flags,
		     GthOverwriteResponse  default_response,
		     int                   io_priority,
		     GCancellable         *cancellable,
		     ProgressCallback      progress_callback,
		     gpointer              progress_callback_data,
		     DialogCallback        dialog_callback,
		     gpointer              dialog_callback_data,
		     ReadyFunc             done_callback,
		     gpointer              user_data)
{
	CopyData *copy_data;
	GList    *scan;

	copy_data = g_new0 (CopyData, 1);
	copy_data->destination = g_object_ref (destination);
	copy_data->move = move;
	copy_data->flags = flags;
	copy_data->io_priority = io_priority;
	copy_data->cancellable = _g_object_ref (cancellable);
	copy_data->progress_callback = progress_callback;
	copy_data->progress_callback_data = progress_callback_data;
	copy_data->dialog_callback = dialog_callback;
	copy_data->dialog_callback_data = dialog_callback_data;
	copy_data->done_callback = done_callback;
	copy_data->user_data = user_data;
	copy_data->default_response = default_response;

	/* save the explicitly requested files */
	copy_data->source_hash = g_hash_table_new_full ((GHashFunc) g_file_hash, (GEqualFunc) g_file_equal, (GDestroyNotify) g_object_unref, NULL);
	for (scan = sources; scan; scan = scan->next)
		g_hash_table_insert (copy_data->source_hash, g_object_ref (scan->data), GINT_TO_POINTER (1));

	if (copy_data->progress_callback != NULL)
		copy_data->progress_callback (NULL,
					      copy_data->move ? _("Moving files") : _("Copying files"),
					      _("Getting file information"),
					      TRUE,
					      0.0,
					      copy_data->progress_callback_data);

	/* for each directory in 'source' this query will add all of its content
	 * to the file list. */
	_g_query_info_async (sources,
			     GTH_LIST_RECURSIVE,
			     "standard::name,standard::display-name,standard::type,standard::size",
			     copy_data->cancellable,
			     copy_files__sources_info_ready_cb,
			     copy_data);
}


gboolean
_g_move_file (GFile                 *source,
              GFile                 *destination,
              GFileCopyFlags         flags,
              GCancellable          *cancellable,
              GFileProgressCallback  progress_callback,
              gpointer               progress_callback_data,
              GError               **error)
{
	GList *source_sidecars = NULL;
	GList *destination_sidecars = NULL;
	GList *scan1;
	GList *scan2;

	if (! g_file_move (source,
			   destination,
			   flags,
			   cancellable,
			   progress_callback,
			   progress_callback_data,
			   error))
	{
		return FALSE;
	}

	if (flags && G_FILE_COPY_ALL_METADATA == 0)
		return TRUE;

	/* move the metadata sidecars if requested */

	gth_hook_invoke ("add-sidecars", source, &source_sidecars);
	source_sidecars = g_list_reverse (source_sidecars);

	gth_hook_invoke ("add-sidecars", destination, &destination_sidecars);
	destination_sidecars = g_list_reverse (destination_sidecars);

	for (scan1 = source_sidecars, scan2 = destination_sidecars;
	     scan1 && scan2;
	     scan1 = scan1->next, scan2 = scan2->next)
	{
		source = scan1->data;
		destination = scan2->data;

		g_file_move (source, destination, 0, cancellable, NULL, NULL, NULL);
	}

	_g_object_list_unref (destination_sidecars);
	_g_object_list_unref (source_sidecars);

	return TRUE;
}


gboolean
_g_delete_files (GList     *file_list,
		 gboolean   include_metadata,
		 GError   **error)
{
	GList *scan;

	for (scan = file_list; scan; scan = scan->next) {
		GFile *file = scan->data;

		if (! g_file_delete (file, NULL, error))
			return FALSE;
	}

	if (include_metadata) {
		GList *sidecars;
		GList *scan;

		sidecars = NULL;
		for (scan = file_list; scan; scan = scan->next)
			gth_hook_invoke ("add-sidecars", scan->data, &sidecars);
		sidecars = g_list_reverse (sidecars);

		for (scan = sidecars; scan; scan = scan->next) {
			GFile *file = scan->data;
			g_file_delete (file, NULL, NULL);
		}

		_g_object_list_unref (sidecars);
	}

	return TRUE;
}


/* -- _g_delete_files_async -- */


typedef struct {
	gboolean      include_metadata;
	GCancellable *cancellable;
	ReadyFunc     callback;
	gpointer      user_data;
} DeleteData;


static void
delete_data_free (DeleteData *delete_data)
{
	_g_object_unref (delete_data->cancellable);
	g_free (delete_data);
}


static void
delete_files__info_ready_cb (GList    *files,
			     GError   *error,
			     gpointer  user_data)
{
	DeleteData *delete_data = user_data;

	if (error == NULL) {
		GList *file_list;
		GList *scan;

		file_list = _g_object_list_ref (files);
		file_list = g_list_reverse (file_list);

		for (scan = file_list; scan; scan = scan->next) {
			GthFileData *file_data = scan->data;

			if (! g_file_delete (file_data->file, delete_data->cancellable, &error))
				break;
		}

		_g_object_list_unref (file_list);
	}

	delete_data->callback (error, delete_data->user_data);
	delete_data_free (delete_data);
}


void
_g_delete_files_async (GList        *file_list,
		       gboolean      recursive,
		       gboolean      include_metadata,
		       GCancellable *cancellable,
		       ReadyFunc     callback,
		       gpointer      user_data)
{
	DeleteData   *delete_data;
	GthListFlags  flags;

	delete_data = g_new0 (DeleteData, 1);
	delete_data->include_metadata = include_metadata;
	delete_data->cancellable = _g_object_ref (cancellable);
	delete_data->callback = callback;
	delete_data->user_data = user_data;

	flags = GTH_LIST_NO_FOLLOW_LINKS;
	if (recursive)
		flags |= GTH_LIST_RECURSIVE;

	_g_query_info_async (file_list,
			     flags,
			     GFILE_NAME_TYPE_ATTRIBUTES,
			     delete_data->cancellable,
			     delete_files__info_ready_cb,
			     delete_data);
}


#define BUFFER_SIZE (64 * 1024)


gboolean
_g_input_stream_read_all (GInputStream  *istream,
			  void         **buffer,
			  gsize         *size,
			  GCancellable  *cancellable,
			  GError       **error)
{
	gboolean  retval = FALSE;
	void     *local_buffer = NULL;
	char     *tmp_buffer;
	gsize     count;
	gssize    n;

	tmp_buffer = g_new (char, BUFFER_SIZE);
	count = 0;
	for (;;) {
		n = g_input_stream_read (istream, tmp_buffer, BUFFER_SIZE, cancellable, error);
		if (n < 0) {
			g_free (local_buffer);
			retval = FALSE;
			break;
		}
		else if (n == 0) {
			*buffer = local_buffer;
			*size = count;
			retval = TRUE;
			break;
		}

		/* the '+ 1' here is used to allow to add a NULL character at the
		 * end of the buffer. */
		local_buffer = g_realloc (local_buffer, count + n + 1);
		memcpy (local_buffer + count, tmp_buffer, n);
		count += n;
	}

	g_free (tmp_buffer);

	return retval;
}


gboolean
_g_file_load_in_buffer (GFile         *file,
		        void         **buffer,
		        gsize         *size,
		        GCancellable  *cancellable,
		        GError       **error)
{
	GInputStream *istream;
	gboolean      retval = FALSE;

	istream = (GInputStream *) g_file_read (file, cancellable, error);
	if (istream != NULL) {
		retval = _g_input_stream_read_all (istream, buffer, size, cancellable, error);
		g_object_unref (istream);
	}

	return retval;
}


typedef struct {
	int                  io_priority;
	GCancellable        *cancellable;
	BufferReadyCallback  callback;
	gpointer             user_data;
	GInputStream        *stream;
	guchar               tmp_buffer[BUFFER_SIZE];
	void                *buffer;
	gsize                count;
} LoadData;


static void
load_data_free (LoadData *load_data)
{
	if (load_data->stream != NULL)
		g_object_unref (load_data->stream);
	g_free (load_data->buffer);
	g_free (load_data);
}


static void
load_file__stream_read_cb (GObject      *source_object,
			   GAsyncResult *result,
			   gpointer      user_data)
{
	LoadData *load_data = user_data;
	GError   *error = NULL;
	gssize    count;

	count = g_input_stream_read_finish (load_data->stream, result, &error);
	if (count < 0) {
		load_data->callback (&load_data->buffer, -1, error, load_data->user_data);
		load_data_free (load_data);
		return;
	}
	else if (count == 0) {
		if (load_data->buffer != NULL)
			((char *)load_data->buffer)[load_data->count] = 0;
		load_data->callback (&load_data->buffer, load_data->count, NULL, load_data->user_data);
		load_data_free (load_data);
		return;
	}

	load_data->buffer = g_realloc (load_data->buffer, load_data->count + count + 1);
	memcpy (load_data->buffer + load_data->count, load_data->tmp_buffer, count);
	load_data->count += count;

	g_input_stream_read_async (load_data->stream,
				   load_data->tmp_buffer,
				   BUFFER_SIZE,
				   load_data->io_priority,
				   load_data->cancellable,
				   load_file__stream_read_cb,
				   load_data);
}


static void
load_file__file_read_cb (GObject      *source_object,
			 GAsyncResult *result,
			 gpointer      user_data)
{
	LoadData *load_data = user_data;
	GError   *error = NULL;

	load_data->stream = (GInputStream *) g_file_read_finish (G_FILE (source_object), result, &error);
	if (load_data->stream == NULL) {
		load_data->callback (&load_data->buffer, -1, error, load_data->user_data);
		load_data_free (load_data);
		return;
	}

	load_data->count = 0;
	g_input_stream_read_async (load_data->stream,
				   load_data->tmp_buffer,
				   BUFFER_SIZE,
				   load_data->io_priority,
				   load_data->cancellable,
				   load_file__stream_read_cb,
				   load_data);
}


void
_g_file_load_async (GFile               *file,
		    int                  io_priority,
		    GCancellable        *cancellable,
		    BufferReadyCallback  callback,
		    gpointer             user_data)
{
	LoadData *load_data;

	load_data = g_new0 (LoadData, 1);
	load_data->io_priority = io_priority;
	load_data->cancellable = cancellable;
	load_data->callback = callback;
	load_data->user_data = user_data;

	g_file_read_async (file, io_priority, cancellable, load_file__file_read_cb, load_data);
}


/* -- _g_file_write_async -- */


typedef struct {
	int                  io_priority;
	GCancellable        *cancellable;
	BufferReadyCallback  callback;
	gpointer             user_data;
	void                *buffer;
	gsize                count;
	gsize                written;
	GError              *error;
} WriteData;


static void
write_data_free (WriteData *write_data)
{
	g_free (write_data->buffer);
	g_free (write_data);
}


static void
write_file__notify (gpointer user_data)
{
	WriteData *write_data = user_data;

	write_data->callback (&write_data->buffer, write_data->count, write_data->error, write_data->user_data);
	write_data_free (write_data);
}


static void
write_file__stream_flush_cb (GObject      *source_object,
			     GAsyncResult *result,
			     gpointer      user_data)
{
	GOutputStream *stream = (GOutputStream *) source_object;
	WriteData     *write_data = user_data;
	GError        *error = NULL;

	g_output_stream_flush_finish (stream, result, &error);
	write_data->error = error;
	g_object_unref (stream);

	call_when_idle (write_file__notify, write_data);
}


static void
write_file__stream_write_ready_cb (GObject      *source_object,
				   GAsyncResult *result,
				   gpointer      user_data)
{
	GOutputStream *stream = (GOutputStream *) source_object;
	WriteData     *write_data = user_data;
	GError        *error = NULL;
	gssize         count;

	count = g_output_stream_write_finish (stream, result, &error);
	write_data->written += count;

	if ((count == 0) || (write_data->written == write_data->count)) {
		g_output_stream_flush_async (stream,
					     write_data->io_priority,
					     write_data->cancellable,
					     write_file__stream_flush_cb,
					     user_data);
		return;
	}

	g_output_stream_write_async (stream,
				     write_data->buffer + write_data->written,
				     write_data->count - write_data->written,
				     write_data->io_priority,
				     write_data->cancellable,
				     write_file__stream_write_ready_cb,
				     write_data);
}


static void
write_file__replace_ready_cb (GObject      *source_object,
			      GAsyncResult *result,
			      gpointer      user_data)
{
	WriteData     *write_data = user_data;
	GOutputStream *stream;
	GError        *error = NULL;

	stream = (GOutputStream*) g_file_replace_finish ((GFile*) source_object, result, &error);
	if (stream == NULL) {
		write_data->callback (&write_data->buffer, write_data->count, error, write_data->user_data);
		write_data_free (write_data);
		return;
	}

	write_data->written = 0;
	g_output_stream_write_async (stream,
				     write_data->buffer,
				     write_data->count,
				     write_data->io_priority,
				     write_data->cancellable,
				     write_file__stream_write_ready_cb,
				     write_data);
}


static void
write_file__create_ready_cb (GObject      *source_object,
			     GAsyncResult *result,
			     gpointer      user_data)
{
	WriteData     *write_data = user_data;
	GOutputStream *stream;
	GError        *error = NULL;

	stream = (GOutputStream*) g_file_create_finish ((GFile*) source_object, result, &error);
	if (stream == NULL) {
		write_data->callback (&write_data->buffer, write_data->count, error, write_data->user_data);
		write_data_free (write_data);
		return;
	}

	write_data->written = 0;
	g_output_stream_write_async (stream,
				     write_data->buffer,
				     write_data->count,
				     write_data->io_priority,
				     write_data->cancellable,
				     write_file__stream_write_ready_cb,
				     write_data);
}


void
_g_file_write_async (GFile               *file,
		     void                *buffer,
		     gsize                count,
		     gboolean             replace,
		     int                  io_priority,
		     GCancellable        *cancellable,
		     BufferReadyCallback  callback,
		     gpointer             user_data)
{
	WriteData *write_data;

	write_data = g_new0 (WriteData, 1);
	write_data->buffer = buffer;
	write_data->count = count;
	write_data->io_priority = io_priority;
	write_data->cancellable = cancellable;
	write_data->callback = callback;
	write_data->user_data = user_data;

	if (replace)
		g_file_replace_async (file,
				      NULL,
				      FALSE,
				      0,
				      io_priority,
				      cancellable,
				      write_file__replace_ready_cb,
				      write_data);
	else
		g_file_create_async (file,
				     0,
				     io_priority,
				     cancellable,
				     write_file__create_ready_cb,
				     write_data);
}


GFile *
_g_file_create_unique (GFile       *parent,
		       const char  *display_name,
		       const char  *suffix,
		       GError     **error)
{
	GFile             *file = NULL;
	GError            *local_error = NULL;
	int                n;
	GFileOutputStream *stream = NULL;

	n = 0;
	do {
		char *new_display_name;

		if (file != NULL)
			g_object_unref (file);

		n++;
		if (n == 1)
			new_display_name = g_strdup_printf ("%s%s", display_name, suffix);
		else
			new_display_name = g_strdup_printf ("%s %d%s", display_name, n, suffix);

		file = g_file_get_child_for_display_name (parent, new_display_name, &local_error);
		if (local_error == NULL)
			stream = g_file_create (file, 0, NULL, &local_error);

		if ((stream == NULL) && g_error_matches (local_error, G_IO_ERROR, G_IO_ERROR_EXISTS))
			g_clear_error (&local_error);

		g_free (new_display_name);
	}
	while ((stream == NULL) && (local_error == NULL));

	if (stream == NULL) {
		if (error != NULL)
			*error = local_error;
		g_object_unref (file);
		file = NULL;
	}
	else
		g_object_unref (stream);

	return file;
}


GFile *
_g_directory_create_unique (GFile       *parent,
			    const char  *display_name,
			    const char  *suffix,
			    GError     **error)
{
	GFile    *file = NULL;
	gboolean  created = FALSE;
	GError   *local_error = NULL;
	int       n;

	file = g_file_get_child_for_display_name (parent, display_name, &local_error);
	if (file == NULL) {
		g_propagate_error (error, local_error);
		return NULL;
	}

	n = 0;
	do {
		char *new_display_name;

		if (file != NULL)
			g_object_unref (file);

		n++;
		if (n == 1)
			new_display_name = g_strdup_printf ("%s%s", display_name, suffix);
		else
			new_display_name = g_strdup_printf ("%s %d%s", display_name, n, suffix);

		file = g_file_get_child_for_display_name (parent, new_display_name, &local_error);
		if (local_error == NULL)
			created = g_file_make_directory (file, NULL, &local_error);

		if (! created && g_error_matches (local_error, G_IO_ERROR, G_IO_ERROR_EXISTS))
			g_clear_error (&local_error);

		g_free (new_display_name);
	}
	while (! created && (local_error == NULL));

	if (local_error != NULL) {
		g_object_unref (file);
		file = NULL;
	}

	if (local_error != NULL)
		g_propagate_error (error, local_error);

	return file;
}


#define MAX_ATTEMPS 10


GFile *
_g_directory_create_tmp (void)
{
	GFile *tmp_dir;
	GFile *dir = NULL;
	int    n;

	tmp_dir = g_file_new_for_path (g_get_tmp_dir ());
	if (tmp_dir == NULL)
		return NULL;

	for (n = 0; n < MAX_ATTEMPS; n++) {
		char  *name;

		name = _g_rand_string (12);
		dir = g_file_get_child (tmp_dir, name);
		g_free (name);

		if (g_file_make_directory (dir, NULL, NULL))
			break;

		g_object_unref (dir);
	}

	g_object_unref (tmp_dir);

	return dir;
}


/* -- _g_file_write -- */


gboolean
_g_file_write (GFile             *file,
	       gboolean           make_backup,
	       GFileCreateFlags   flags,
	       void              *buffer,
	       gsize              count,
	       GCancellable      *cancellable,
	       GError           **error)
{
	gboolean       success;
	GOutputStream *stream;

	stream = (GOutputStream *) g_file_replace (file, NULL, make_backup, flags, cancellable, error);
	if (stream != NULL)
		success = g_output_stream_write_all (stream, buffer, count, NULL, cancellable, error);
	else
		success = FALSE;

	_g_object_unref (stream);

	return success;
}


gboolean
_g_directory_make (GFile    *file,
		   guint32   unix_mode,
		   GError  **error)
{
	if (! g_file_make_directory (file, NULL, error)) {
		if (! (*error)->code == G_IO_ERROR_EXISTS)
			return FALSE;
		g_clear_error (error);
	}

	return g_file_set_attribute_uint32 (file,
					    G_FILE_ATTRIBUTE_UNIX_MODE,
					    unix_mode,
					    G_FILE_QUERY_INFO_NONE,
					    NULL,
					    error);
}


gboolean
_g_file_set_modification_time (GFile         *file,
			       GTimeVal      *timeval,
			       GCancellable  *cancellable,
			       GError       **error)
{
	GFileInfo *info;
	gboolean   result;

	info = g_file_info_new ();
        g_file_info_set_modification_time (info, timeval);
        result = g_file_set_attributes_from_info (file, info, G_FILE_QUERY_INFO_NONE, cancellable, error);

        g_object_unref (info);

        return result;
}
