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

#ifndef _GIO_UTILS_H
#define _GIO_UTILS_H

#include <glib.h>
#include <gio/gio.h>
#include "gth-file-data.h"
#include "gth-overwrite-dialog.h"
#include "typedefs.h"

G_BEGIN_DECLS

/* callback types */

typedef enum { /*< skip >*/
	DIR_OP_CONTINUE,
	DIR_OP_SKIP,
	DIR_OP_STOP
} DirOp;

typedef enum {
	GTH_LIST_DEFAULT = 0,
	GTH_LIST_RECURSIVE = 1 << 0,
	GTH_LIST_NO_FOLLOW_LINKS = 1 << 1,
	GTH_LIST_NO_BACKUP_FILES = 1 << 2,
	GTH_LIST_NO_HIDDEN_FILES = 1 << 3
} GthListFlags;

typedef DirOp (*StartDirCallback)    (GFile                *directory,
				      GFileInfo            *info,
				      GError              **error,
				      gpointer              user_data);
typedef void (*ForEachChildCallback) (GFile                *file,
				      GFileInfo            *info,
				      gpointer              user_data);
typedef void (*ListReadyCallback)    (GList                *files,
				      GList                *dirs,
				      GError               *error,
				      gpointer              user_data);
typedef void (*BufferReadyCallback)  (void                **buffer,
				      gsize                 count,
				      GError               *error,
				      gpointer              user_data);
typedef void (*InfoReadyCallback)    (GList                *files,
				      GError               *error,
				      gpointer              user_data);
typedef void (*CopyReadyCallback)    (GthOverwriteResponse  default_response,
				      GError               *error,
			 	      gpointer              user_data);

/* asynchronous recursive list functions */

void   g_directory_foreach_child     (GFile                 *directory,
				      gboolean               recursive,
				      gboolean               follow_links,
				      const char            *attributes,
				      GCancellable          *cancellable,
				      StartDirCallback       start_dir_func,
				      ForEachChildCallback   for_each_file_func,
				      ReadyFunc              done_func,
				      gpointer               user_data);
void   g_directory_list_async        (GFile                 *directory,
				      const char            *base_dir,
				      gboolean               recursive,
				      gboolean               follow_links,
				      gboolean               no_backup_files,
				      gboolean               no_dot_files,
				      const char            *include_files,
				      const char            *exclude_files,
				      const char            *exclude_folders,
				      gboolean               ignorecase,
				      GCancellable          *cancellable,
				      ListReadyCallback      done_func,
				      gpointer               done_data);
void   _g_query_info_async           (GList                 *file_list,    /* GFile * list */
				      GthListFlags           flags,
				      const char            *attributes,
				      GCancellable          *cancellable,
				      InfoReadyCallback      ready_callback,
				      gpointer               user_data);

/* asynchronous copy functions */

void     _g_dummy_file_op_async      (ReadyFunc              callback,
				      gpointer               user_data);
void     _g_copy_file_async          (GthFileData           *source,
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
				      gpointer               user_data);
void     _g_copy_files_async         (GList                 *sources,
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
				      ReadyFunc              callback,
				      gpointer               user_data);
gboolean _g_move_file                (GFile                 *source,
                                      GFile                 *destination,
                                      GFileCopyFlags         flags,
                                      GCancellable          *cancellable,
                                      GFileProgressCallback  progress_callback,
                                      gpointer               progress_callback_data,
                                      GError               **error);
gboolean _g_delete_files             (GList                 *file_list,
				      gboolean               include_metadata,
				      GError               **error);
void     _g_delete_files_async       (GList                  *file_list,
				      gboolean               recursive,
				      gboolean               include_metadata,
				      GCancellable          *cancellable,
				      ReadyFunc              callback,
				      gpointer               user_data);

/* -- load/write/create file  -- */

gboolean _g_input_stream_read_all    (GInputStream          *istream,
				      void                 **buffer,
				      gsize                 *size,
				      GCancellable          *cancellable,
				      GError               **error);
gboolean _g_file_load_in_buffer      (GFile                 *file,
				      void                 **buffer,
				      gsize                 *size,
				      GCancellable          *cancellable,
				      GError               **error);
void     _g_file_load_async          (GFile                 *file,
				      int                    io_priority,
				      GCancellable          *cancellable,
				      BufferReadyCallback    callback,
				      gpointer               user_data);
gboolean _g_file_write               (GFile                 *file,
				      gboolean               make_backup,
			              GFileCreateFlags       flags,
				      void                  *buffer,
				      gsize                  count,
				      GCancellable          *cancellable,
				      GError               **error);
void     _g_file_write_async         (GFile                 *file,
				      void                  *buffer,
		    		      gsize                  count,
		    		      gboolean               replace,
				      int                    io_priority,
				      GCancellable          *cancellable,
				      BufferReadyCallback    callback,
				      gpointer               user_data);
GFile * _g_file_create_unique        (GFile                 *parent,
				      const char            *display_name,
				      const char            *suffix,
				      GError               **error);
GFile * _g_directory_create_unique   (GFile                 *parent,
				      const char            *display_name,
				      const char            *suffix,
				      GError               **error);
GFile * _g_directory_create_tmp      (void);
gboolean _g_file_set_modification_time (GFile               *file,
					GTimeVal            *timeval,
					GCancellable        *cancellable,
					GError             **error);

/* convenience macros */

/**
 * g_directory_list_all_async:
 * @directory:
 * @base_dir:
 * @recursive:
 * @cancellable:
 * @done_func:
 * @done_data:
 *
 */
#define g_directory_list_all_async(directory, base_dir, recursive, cancellable, done_func, done_data) \
    g_directory_list_async ((directory), (base_dir), (recursive), TRUE, FALSE, FALSE, NULL, NULL, NULL, FALSE, (cancellable), (done_func), (done_data))

gboolean _g_directory_make (GFile    *file,
			    guint32   unix_mode,
			    GError  **error);

G_END_DECLS

#endif /* _GIO_UTILS_H */
