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

#ifndef GTH_FILE_SOURCE_VFS_H
#define GTH_FILE_SOURCE_VFS_H

#include "gth-file-source.h"

G_BEGIN_DECLS

#define GTH_TYPE_FILE_SOURCE_VFS         (gth_file_source_vfs_get_type ())
#define GTH_FILE_SOURCE_VFS(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), GTH_TYPE_FILE_SOURCE_VFS, GthFileSourceVfs))
#define GTH_FILE_SOURCE_VFS_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST ((k), GTH_TYPE_FILE_SOURCE_VFS, GthFileSourceVfsClass))
#define GTH_IS_FILE_SOURCE_VFS(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), GTH_TYPE_FILE_SOURCE_VFS))
#define GTH_IS_FILE_SOURCE_VFS_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), GTH_TYPE_FILE_SOURCE_VFS))
#define GTH_FILE_SOURCE_VFS_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS((o), GTH_TYPE_FILE_SOURCE_VFS, GthFileSourceVfsClass))

typedef struct _GthFileSourceVfs         GthFileSourceVfs;
typedef struct _GthFileSourceVfsPrivate  GthFileSourceVfsPrivate;
typedef struct _GthFileSourceVfsClass    GthFileSourceVfsClass;

struct _GthFileSourceVfs
{
	GthFileSource __parent;
	GthFileSourceVfsPrivate *priv;
};

struct _GthFileSourceVfsClass
{
	GthFileSourceClass __parent_class;
};

GType gth_file_source_vfs_get_type   (void) G_GNUC_CONST;
void  gth_file_mananger_trash_files  (GtkWindow *window,
				      GList     *file_list /* GthFileData list */);
void  gth_file_mananger_delete_files (GtkWindow *window,
				      GList     *file_list /* GthFileData list */);

G_END_DECLS

#endif /* GTH_FILE_SOURCE_VFS_H */
