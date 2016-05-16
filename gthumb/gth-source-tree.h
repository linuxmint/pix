/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2009 Free Software Foundation, Inc.
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
 
#ifndef GTH_SOURCE_TREE_H
#define GTH_SOURCE_TREE_H

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>
#include "gth-file-data.h"
#include "gth-folder-tree.h"

G_BEGIN_DECLS

#define GTH_TYPE_SOURCE_TREE            (gth_source_tree_get_type ())
#define GTH_SOURCE_TREE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTH_TYPE_SOURCE_TREE, GthSourceTree))
#define GTH_SOURCE_TREE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GTH_TYPE_SOURCE_TREE, GthSourceTreeClass))
#define GTH_IS_SOURCE_TREE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTH_TYPE_SOURCE_TREE))
#define GTH_IS_SOURCE_TREE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GTH_TYPE_SOURCE_TREE))
#define GTH_SOURCE_TREE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GTH_TYPE_SOURCE_TREE, GthSourceTreeClass))

typedef struct _GthSourceTree GthSourceTree;
typedef struct _GthSourceTreeClass GthSourceTreeClass;
typedef struct _GthSourceTreePrivate GthSourceTreePrivate;

struct _GthSourceTree {
	GthFolderTree parent_instance;
	GthSourceTreePrivate *priv;
};

struct _GthSourceTreeClass {
	GthFolderTreeClass parent_class;
};

GType        gth_source_tree_get_type      (void);
GtkWidget *  gth_source_tree_new           (GFile         *root);
void         gth_source_tree_set_root      (GthSourceTree *source_tree,
					    GFile         *root);

G_END_DECLS

#endif /* GTH_SOURCE_TREE_H */
