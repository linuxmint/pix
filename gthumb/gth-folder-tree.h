/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2008 The Free Software Foundation, Inc.
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

#ifndef GTH_FOLDER_TREE_H
#define GTH_FOLDER_TREE_H

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define GTH_TYPE_FOLDER_TREE            (gth_folder_tree_get_type ())
#define GTH_FOLDER_TREE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTH_TYPE_FOLDER_TREE, GthFolderTree))
#define GTH_FOLDER_TREE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GTH_TYPE_FOLDER_TREE, GthFolderTreeClass))
#define GTH_IS_FOLDER_TREE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTH_TYPE_FOLDER_TREE))
#define GTH_IS_FOLDER_TREE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GTH_TYPE_FOLDER_TREE))
#define GTH_FOLDER_TREE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), GTH_TYPE_FOLDER_TREE, GthFolderTreeClass))

typedef struct _GthFolderTree         GthFolderTree;
typedef struct _GthFolderTreePrivate  GthFolderTreePrivate;
typedef struct _GthFolderTreeClass    GthFolderTreeClass;

struct _GthFolderTree {
	GtkTreeView __parent;
	GthFolderTreePrivate *priv;
};

struct _GthFolderTreeClass {
	GtkTreeViewClass __parent;

	/* -- signals -- */

	void (*folder_popup)  (GthFolderTree *folder_tree,
			       GFile         *file,
			       guint32        time);
	void (*list_children) (GthFolderTree *folder_tree,
			       GFile         *file);
	void (*list_popup)    (GthFolderTree *folder_tree,
			       guint32        time);
	void (*load)          (GthFolderTree *folder_tree,
			       GFile         *file);
	void (*open)          (GthFolderTree *folder_tree,
			       GFile         *file);
	void (*open_parent)   (GthFolderTree *folder_tree);
	void (*rename)        (GthFolderTree *folder_tree,
			       GFile         *file,
			       const char    *new_name);
};

GType         gth_folder_tree_get_type           (void);
GtkWidget *   gth_folder_tree_new                (const char           *root);
void          gth_folder_tree_set_list           (GthFolderTree        *folder_tree,
						  GFile                *root,
						  GList                *files,
						  gboolean              open_parent);
void          gth_folder_tree_set_children       (GthFolderTree        *folder_tree,
						  GFile                *parent,
						  GList                *files /* GthFileData */);
void          gth_folder_tree_loading_children   (GthFolderTree        *folder_tree,
						  GFile                *parent);
void          gth_folder_tree_add_children       (GthFolderTree        *folder_tree,
						  GFile                *parent,
						  GList                *files /* GthFileData */);
void          gth_folder_tree_update_children    (GthFolderTree        *folder_tree,
						  GFile                *parent,
						  GList                *files /* GthFileData */);
void          gth_folder_tree_update_child       (GthFolderTree        *folder_tree,
						  GFile                *file,
						  GthFileData          *file_data);
void          gth_folder_tree_delete_children    (GthFolderTree        *folder_tree,
						  GFile                *parent,
						  GList                *files /* GFile */);
void          gth_folder_tree_start_editing      (GthFolderTree        *folder_tree,
						  GFile                *file);
GtkTreePath * gth_folder_tree_get_path           (GthFolderTree        *folder_tree,
						  GFile                *file);
gboolean      gth_folder_tree_is_loaded          (GthFolderTree        *folder_tree,
						  GtkTreePath          *path);
gboolean      gth_folder_tree_has_no_child       (GthFolderTree        *folder_tree,
						  GtkTreePath          *path);
void          gth_folder_tree_reset_loaded       (GthFolderTree        *folder_tree);
void          gth_folder_tree_expand_row         (GthFolderTree        *folder_tree,
						  GtkTreePath          *path,
						  gboolean              open_all);
void          gth_folder_tree_collapse_all       (GthFolderTree        *folder_tree);
void          gth_folder_tree_select_path        (GthFolderTree        *folder_tree,
						  GtkTreePath          *path);
GFile *       gth_folder_tree_get_root           (GthFolderTree        *folder_tree);
gboolean      gth_folder_tree_is_root            (GthFolderTree        *folder_tree,
						  GFile                *folder);
GthFileData * gth_folder_tree_get_file           (GthFolderTree        *folder_tree,
						  GtkTreePath          *path);
GthFileData * gth_folder_tree_get_selected       (GthFolderTree        *folder_tree);
GthFileData * gth_folder_tree_get_selected_or_parent
					         (GthFolderTree        *folder_tree);
void          gth_folder_tree_enable_drag_source (GthFolderTree        *self,
						  GdkModifierType       start_button_mask,
						  const GtkTargetEntry *targets,
						  int                   n_targets,
						  GdkDragAction         actions);
void          gth_folder_tree_unset_drag_source  (GthFolderTree        *self);

G_END_DECLS

#endif /* GTH_FOLDER_TREE_H */
