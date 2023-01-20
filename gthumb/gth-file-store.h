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

#ifndef GTH_FILE_STORE_H
#define GTH_FILE_STORE_H

#include <gtk/gtk.h>
#include "gth-file-data.h"
#include "gth-test.h"

G_BEGIN_DECLS

#define GTH_TYPE_FILE_STORE         (gth_file_store_get_type ())
#define GTH_FILE_STORE(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), GTH_TYPE_FILE_STORE, GthFileStore))
#define GTH_FILE_STORE_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST ((k), GTH_TYPE_FILE_STORE, GthFileStoreClass))
#define GTH_IS_FILE_STORE(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), GTH_TYPE_FILE_STORE))
#define GTH_IS_FILE_STORE_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), GTH_TYPE_FILE_STORE))
#define GTH_FILE_STORE_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS((o), GTH_TYPE_FILE_STORE, GthFileStoreClass))

typedef struct _GthFileStore         GthFileStore;
typedef struct _GthFileStorePrivate  GthFileStorePrivate;
typedef struct _GthFileStoreClass    GthFileStoreClass;

enum {
	GTH_FILE_STORE_FILE_DATA_COLUMN,
	GTH_FILE_STORE_THUMBNAIL_COLUMN,
	GTH_FILE_STORE_IS_ICON_COLUMN,
	GTH_FILE_STORE_THUMBNAIL_STATE_COLUMN,
	GTH_FILE_STORE_EMBLEMS_COLUMN,
	GTH_FILE_STORE_N_COLUMNS
};

struct _GthFileStore
{
	GObject __parent;
	GthFileStorePrivate *priv;
};

struct _GthFileStoreClass
{
	GObjectClass __parent_class;

	void (*visibility_changed) (GthFileStore *self);
	void (*thumbnail_changed)  (GtkTreeModel *tree_model,
				    GtkTreePath  *path,
				    GtkTreeIter  *iter);
};

GType           gth_file_store_get_type          (void) G_GNUC_CONST;
GthFileStore *  gth_file_store_new               (void);
void            gth_file_store_set_filter        (GthFileStore         *file_store,
						  GthTest              *filter);
void            gth_file_store_set_sort_func     (GthFileStore         *file_store,
						  GthFileDataCompFunc   cmp_func,
						  gboolean              inverse_sort);
GList *         gth_file_store_get_all           (GthFileStore         *file_store);
int             gth_file_store_n_files           (GthFileStore         *file_store);
GList *         gth_file_store_get_visibles      (GthFileStore         *file_store);
int             gth_file_store_n_visibles        (GthFileStore         *file_store);
GthFileData *   gth_file_store_get_file          (GthFileStore         *file_store,
						  GtkTreeIter          *iter);
gboolean        gth_file_store_find              (GthFileStore         *file_store,
						  GFile                *file,
						  GtkTreeIter          *iter);
gboolean        gth_file_store_find_visible      (GthFileStore         *file_store,
						  GFile                *file,
						  GtkTreeIter          *iter);
int             gth_file_store_get_pos           (GthFileStore         *file_store,
						  GFile                *file);
#define         gth_file_store_get_first(file_store, iter) gth_file_store_get_nth (file_store, 0, iter)
gboolean        gth_file_store_get_nth           (GthFileStore         *file_store,
						  int                   n,
						  GtkTreeIter          *iter);
gboolean        gth_file_store_get_next          (GthFileStore         *file_store,
						  GtkTreeIter          *iter);
#define         gth_file_store_get_first_visible(file_store, iter) gth_file_store_get_nth_visible(file_store, 0, iter)
gboolean        gth_file_store_get_nth_visible   (GthFileStore         *file_store,
						  int                   n,
						  GtkTreeIter          *iter);
gboolean        gth_file_store_get_last_visible  (GthFileStore         *file_store,
						  GtkTreeIter          *iter);
gboolean        gth_file_store_get_next_visible  (GthFileStore         *file_store,
						  GtkTreeIter          *iter);
gboolean        gth_file_store_get_prev_visible  (GthFileStore         *file_store,
						  GtkTreeIter          *iter);
void            gth_file_store_add               (GthFileStore         *file_store,
						  GthFileData          *file,
						  cairo_surface_t      *thumbnail,
						  gboolean              is_icon,
						  GthThumbnailState     state,
						  int                   position);
void            gth_file_store_queue_add         (GthFileStore         *file_store,
						  GthFileData          *file,
						  cairo_surface_t      *thumbnail,
						  gboolean              is_icon,
						  GthThumbnailState     state);
void            gth_file_store_exec_add          (GthFileStore         *file_store,
						  int                   position);
void            gth_file_store_set               (GthFileStore         *file_store,
						  GtkTreeIter          *iter,
						  ...);
void            gth_file_store_queue_set         (GthFileStore         *file_store,
						  GtkTreeIter          *iter,
						  ...);
void            gth_file_store_exec_set          (GthFileStore         *file_store);
void            gth_file_store_remove            (GthFileStore         *file_store,
						  GtkTreeIter          *iter);
void            gth_file_store_queue_remove      (GthFileStore         *file_store,
						  GtkTreeIter          *iter);
void            gth_file_store_exec_remove       (GthFileStore         *file_store);
void            gth_file_store_clear             (GthFileStore         *file_store);
void            gth_file_store_reorder           (GthFileStore         *file_store,
						  int                  *new_order);
gboolean        gth_file_store_iter_is_valid     (GthFileStore         *file_store,
						  GtkTreeIter          *iter);

G_END_DECLS

#endif /* GTH_FILE_STORE_H */
