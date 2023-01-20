/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2008-2019 Free Software Foundation, Inc.
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

typedef enum { /*< skip >*/
	DIR_OP_CONTINUE,
	DIR_OP_SKIP,
	DIR_OP_STOP
} DirOp;

typedef enum {
	GTH_LIST_DEFAULT		= 0,
	GTH_LIST_RECURSIVE		= 1 << 0,
	GTH_LIST_NO_FOLLOW_LINKS	= 1 << 1,
	GTH_LIST_NO_BACKUP_FILES	= 1 << 2,
	GTH_LIST_NO_HIDDEN_FILES	= 1 << 3,
	GTH_LIST_INCLUDE_SIDECARS	= 1 << 4
} GthListFlags;

typedef enum { /*< skip >*/
	GTH_FILE_COPY_DEFAULT		= 0,
	GTH_FILE_COPY_ALL_METADATA	= 1 << 1,
	GTH_FILE_COPY_RENAME_SAME_FILE	= 1 << 2
} GthFileCopyFlags;

/* Callback types */

typedef DirOp	(*StartDirCallback)		(GFile			 *directory,
						 GFileInfo		 *info,
						 GError		**error,
						 gpointer		  user_data);
typedef void	(*ForEachChildCallback)	(GFile			 *file,
						 GFileInfo		 *info,
						 gpointer		  user_data);
typedef void	(*ListReadyCallback)		(GList			 *files,
						 GList			 *dirs,
						 GError		 *error,
						 gpointer		  user_data);
typedef void	(*BufferReadyCallback)		(void			**buffer,
						 gsize			  count,
						 GError		 *error,
						 gpointer		  user_data);
typedef void	(*InfoReadyCallback)		(GList			 *files,
						 GError		 *error,
						 gpointer		  user_data);
typedef void	(*CopyReadyCallback)		(GthOverwriteResponse	  default_response,
						 GList			 *other_files,
						 GError		 *error,
						 gpointer		  user_data);

/* GFile utils */

gboolean	_g_file_move			(GFile			 *source,
						 GFile			 *destination,
						 GFileCopyFlags	  flags,
						 GCancellable		 *cancellable,
						 GFileProgressCallback	  progress_callback,
						 gpointer		  progress_callback_data,
						 GError		**error);

gboolean	_g_file_load_in_buffer		(GFile			 *file,
						 void			**buffer,
						 gsize			 *size,
						 GCancellable		 *cancellable,
						 GError		**error);
void		_g_file_load_async		(GFile			 *file,
						 int			  io_priority,
						 GCancellable		 *cancellable,
						 BufferReadyCallback	  callback,
						 gpointer		  user_data);
gboolean	_g_file_write			(GFile			 *file,
						 gboolean		  make_backup,
						 GFileCreateFlags	  flags,
						 void			 *buffer,
						 gsize			  count,
						 GCancellable		 *cancellable,
						 GError		**error);
void		_g_file_write_async		(GFile			 *file,
						 void			 *buffer,
						 gsize			  count,
						 gboolean		  replace,
						 int			  io_priority,
						 GCancellable		 *cancellable,
						 BufferReadyCallback	  callback,
						 gpointer		  user_data);
GFile *	_g_file_create_unique		(GFile			 *parent,
						 const char		 *display_name,
						 const char		 *suffix,
						 GError		**error);
gboolean	_g_file_set_modification_time	(GFile			 *file,
						 GTimeVal		 *timeval,
						 GCancellable		 *cancellable,
						 GError		**error);
GFileInfo *	_g_file_get_info_for_display	(GFile			 *file);

/* Directory utils */

void		_g_directory_foreach_child	(GFile			 *directory,
						 gboolean		  recursive,
						 gboolean		  follow_links,
						 const char		 *attributes,
						 GCancellable		 *cancellable,
						 StartDirCallback	  start_dir_func,
						 ForEachChildCallback	  for_each_file_func,
						 ReadyFunc		  done_func,
						 gpointer		  user_data);
GFile *	_g_directory_create_tmp	(void);

/* GFile list utils */

void		_g_file_list_query_info_async	(GList			 *file_list, /* GFile list */
						 GthListFlags		  flags,
						 const char		 *attributes,
						 GCancellable		 *cancellable,
						 InfoReadyCallback	  ready_callback,
						 gpointer		  user_data);
void		_g_file_list_copy_async	(GList			 *sources, /* GFile list */
						 GFile			 *destination,
						 gboolean		  move,
						 GthFileCopyFlags	  flags,
						 GthOverwriteResponse	  default_response,
						 int			  io_priority,
						 GCancellable		 *cancellable,
						 ProgressCallback	  progress_callback,
						 gpointer		  progress_callback_data,
						 DialogCallback	  dialog_callback,
						 gpointer		  dialog_callback_data,
						 ReadyFunc		  callback,
						 gpointer		  user_data);
gboolean	_g_file_list_delete		(GList			 *file_list, /* GFile list */
						 gboolean		  include_metadata,
						 GError		**error);
void		_g_file_list_delete_async	(GList			 *file_list, /* GFile list */
						 gboolean		  recursive,
						 gboolean		  include_metadata,
						 GCancellable		 *cancellable,
						 ProgressCallback	  progress_callback,
						 ReadyFunc		  callback,
						 gpointer		  user_data);
void		_g_file_list_trash_async	(GList			 *file_list, /* GFile list */
						 GCancellable		 *cancellable,
						 ProgressCallback	  progress_callback,
						 ReadyFunc		  callback,
						 gpointer		  user_data);

/* Misc utils */

void		_gth_file_data_copy_async	(GthFileData		 *source,
						 GFile			 *destination,
						 gboolean		  move,
						 GthFileCopyFlags	  flags,
						 GthOverwriteResponse	  default_response,
						 int			  io_priority,
						 GCancellable		 *cancellable,
						 ProgressCallback	  progress_callback,
						 gpointer		  progress_callback_data,
						 DialogCallback	  dialog_callback,
						 gpointer		  dialog_callback_data,
						 CopyReadyCallback	  ready_callback,
						 gpointer		  user_data);
gboolean	_g_input_stream_read_all	(GInputStream		 *istream,
						 void			**buffer,
						 gsize			 *size,
						 GCancellable		 *cancellable,
						 GError		**error);
GMenuItem *	_g_menu_item_new_for_file	(GFile			 *file,
						 const char		 *custom_label);
GMenuItem *	_g_menu_item_new_for_file_data	(GthFileData		 *file_data);

G_END_DECLS

#endif /* _GIO_UTILS_H */
