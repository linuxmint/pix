/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2019 Free Software Foundation, Inc.
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

#ifndef GTH_VFS_TREE_H
#define GTH_VFS_TREE_H

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>
#include "gth-folder-tree.h"

G_BEGIN_DECLS

#define GTH_TYPE_VFS_TREE            (gth_vfs_tree_get_type ())
#define GTH_VFS_TREE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTH_TYPE_VFS_TREE, GthVfsTree))
#define GTH_VFS_TREE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GTH_TYPE_VFS_TREE, GthVfsTreeClass))
#define GTH_IS_VFS_TREE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTH_TYPE_VFS_TREE))
#define GTH_IS_VFS_TREE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GTH_TYPE_VFS_TREE))
#define GTH_VFS_TREE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GTH_TYPE_VFS_TREE, GthVfsTreeClass))

typedef struct _GthVfsTree GthVfsTree;
typedef struct _GthVfsTreeClass GthVfsTreeClass;
typedef struct _GthVfsTreePrivate GthVfsTreePrivate;

struct _GthVfsTree {
	GthFolderTree parent_instance;
	GthVfsTreePrivate *priv;
};

struct _GthVfsTreeClass {
	GthFolderTreeClass parent_class;

	/* -- signals -- */

	void (*changed) (GthVfsTree *vfs_tree);
};

GType        gth_vfs_tree_get_type		(void);
GtkWidget *  gth_vfs_tree_new			(const char *root,
						 const char *folder);
void         gth_vfs_tree_set_folder		(GthVfsTree *vfs_tree,
						 GFile      *folder);
GFile *      gth_vfs_tree_get_folder		(GthVfsTree *vfs_tree);
void         gth_vfs_tree_set_show_hidden	(GthVfsTree *self,
						 gboolean    show_hidden);
gboolean     gth_vfs_tree_get_show_hidden	(GthVfsTree *self);

G_END_DECLS

#endif /* GTH_VFS_TREE_H */
